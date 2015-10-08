/**
 * gsmutil.cpp
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * GSM Utility classes implementation
 *
 * Yet Another Telephony Engine - Base Transceiver Station
 * Copyright (C) 2014 Null Team Impex SRL
 *
 * This software is distributed under multiple licenses;
 * see the COPYING file in the main directory for licensing
 * information for this specific distribution.
 *
 * This use of this software may be subject to additional restrictions.
 * See the LEGAL file in the main directory for details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "gsmutil.h"
#include "math.h"

using namespace TelEngine;

// GSM Training Sequence Code for Normal Burst (See TS 100 908 - GSM 05.02 Section 5.2.3)
static const int8_t s_nbTscTable[8][GSM_NB_TSC_LEN] = {
    {0,0,1,0,0,1,0,1,1,1,0,0,0,0,1,0,0,0,1,0,0,1,0,1,1,1},
    {0,0,1,0,1,1,0,1,1,1,0,1,1,1,1,0,0,0,1,0,1,1,0,1,1,1},
    {0,1,0,0,0,0,1,1,1,0,1,1,1,0,1,0,0,1,0,0,0,0,1,1,1,0},
    {0,1,0,0,0,1,1,1,1,0,1,1,0,1,0,0,0,1,0,0,0,1,1,1,1,0},
    {0,0,0,1,1,0,1,0,1,1,1,0,0,1,0,0,0,0,0,1,1,0,1,0,1,1},
    {0,1,0,0,1,1,1,0,1,0,1,1,0,0,0,0,0,1,0,0,1,1,1,0,1,0},
    {1,0,1,0,0,1,1,1,1,1,0,1,1,0,0,0,1,0,1,0,0,1,1,1,1,1},
    {1,1,1,0,1,1,1,1,0,0,0,1,0,0,1,0,1,1,1,0,1,1,1,1,0,0}
};

static inline uint32_t net2uint32(const unsigned char* buf)
{
    return ((uint32_t)buf[0]) << 24 | ((uint32_t)buf[1]) << 16 |
	((uint32_t)buf[2]) << 8 | ((uint32_t)buf[3]);
}

// GSM Sync Bits for Access Burst (See TS 100 908 - GSM 05.02 Section 5.2.7)
static const int8_t s_abSyncTable[GSM_AB_SYNC_LEN] =
    {0,1,0,0,1,0,1,1,0,1,1,1,1,1,1,1,1,0,0,1,1,0,0,1,1,0,1,0,1,0,1,0,0,0,1,1,1,1,0,0,0};


//
// GSMUtils
//
const int8_t* GSMUtils::nbTscTable()
{
    return (const int8_t*)s_nbTscTable;
}

const int8_t* GSMUtils::abSyncTable()
{
    return s_abSyncTable;
}


//
// GSMTxBurst
//
// Build TX data
void GSMTxBurst::buildTxData(const SignalProcessing& proc,
    FloatVector* tmpV, ComplexVector* tmpW, const DataBlock& buf)
{
    const DataBlock& tmp = buf.length() ? buf : *static_cast<const DataBlock*>(this);
    if (tmp.length())
	proc.modulate(m_txData,tmp.data(0),tmp.length(),tmpV,tmpW);
    // Due testing purposes the frequency shifting is done on send time.
}

// Parse received (from upper layer) TX burst
// Expected format:
// 1 byte:  Time slot number (MSB bit: set if burst is filler)
// 4 bytes: Frame number
// 1 byte:  Power level
// The rest should be burst data of size GSM_BURST_LENGTH
// Expected size: GSM_BURST_TXPACKET
// The power is received in decibels.
// power = amplitude * 2
// amplitude = 10^(decibels / 20)
// power = 10 ^ (decibels / 10)
// Maximum decibels level is 0 so the power level received will be <= 0
// In conclusion  powerLevel = 10^(-receivedPowerLevel/10)
GSMTxBurst* GSMTxBurst::parse(const uint8_t* buf, unsigned int len,
    ObjStore<GSMTxBurst>& store, DataBlock* dataBits)
{
#ifdef XDEBUG
    String tmp;
    tmp.hexify((void*)buf,len);
    Debug(DebugAll,"GSMTxBurst::parse() len=%u: %s",len,tmp.c_str());
#endif
    if (!(buf && len))
	return 0;
    if (len != GSM_BURST_TXPACKET) {
	Debug(DebugMild,"GSMTxBurst::parse() invalid len=%u expected=%u",
	    len,GSM_BURST_TXPACKET);
	return 0;
    }
    GSMTxBurst* burst = store.get();
    burst->m_filler = (buf[0] & 0x10) != 0;
    burst->m_time.assign(net2uint32(buf + 1),buf[0] & 0x0f);
    burst->m_type = buf[5];
    burst->m_powerLevel = ::pow(10, -(float)buf[5] / 10);
    // This should be between 0 and 1
    if (burst->m_powerLevel < 0 || burst->m_powerLevel > 1) {
	Debug(DebugGoOn,"Clamping received TX burst power level %u (%g) to [0..1]",
	    buf[5],burst->m_powerLevel);
	if (burst->m_powerLevel < 0)
	    burst->m_powerLevel = 0;
	else
	    burst->m_powerLevel = 1;
    }
    if (dataBits)
	dataBits->assign((void*)(buf + GSM_BURST_TXHEADER),GSM_BURST_LENGTH,false);
    else
	burst->assign((void*)(buf + GSM_BURST_TXHEADER),GSM_BURST_LENGTH);
    return burst;
}

// Build a default dummy burst
GSMTxBurst* GSMTxBurst::buildFiller()
{
    static const uint8_t s_dummyBurst[GSM_BURST_TXPACKET] = {
	0,0,0,0,0,0,
	0,0,0,1,1,1,1,1,0,1,1,0,1,1,1,0,1,1,0,0,0,0,0,1,0,1,0,0,1,0,0,1,1,1,0,0,0,
	0,0,1,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,1,1,1,0,0,0,1,0,1,1,1,0,
	0,0,1,0,1,1,1,0,0,0,1,0,1,0,1,1,1,0,1,0,0,1,0,1,0,0,0,1,1,0,0,1,1,0,0,1,1,
	1,0,0,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,1,0,0,1,0,1,1,1,1,1,0,1,0,1,0,0,0,0
    };

    GSMTxBurstStore store(1,"GSMTxBurst::buildFiller()");
    GSMTxBurst* b = parse(s_dummyBurst,sizeof(s_dummyBurst),store);
    if (b)
	b->m_filler = true;
    return b;
}


//
// GSMRxBurst
//
// Setup a buffer from burst
void GSMRxBurst::fillEstimatesBuffer()
{
    uint8_t* buf = m_bitEstimate;
    // Buffer format:
    //   1 byte timeslot index
    //   4 bytes GSM frame number, big endian
    //   1 byte power level
    //   2 bytes correlator timing offset (timing advance error)
    int16_t toa = m_timingError * 256;
    *buf++ = m_time.tn();
    uint32_t fn = m_time.fn();
    *buf++ = (int8_t)(fn >> 24);
    *buf++ = (int8_t)(fn >> 16);
    *buf++ = (int8_t)(fn >> 8);
    *buf++ = (int8_t)fn;
    *buf++ = (int8_t)-m_powerLevel;
    *buf++ = (int8_t)(toa >> 8);
    *buf++ = (int8_t)toa;
}

/* vi: set ts=8 sw=4 sts=4 noet: */
