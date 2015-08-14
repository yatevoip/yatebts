/*
* Copyright 2008, 2009 Free Software Foundation, Inc.
*

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

* This software is distributed under multiple licenses; see the COPYING file in the main directory for licensing information for this specific distribuion.
*/


#include "GSMTAPDump.h"
#include "GSMTransfer.h"
#include <Sockets.h>
#include <Globals.h>
#include <Logger.h>


#define MAX_DUMP_LENGTH (MAX_UDP_LENGTH + 32)

UDPSocket GSMTAPSocket;


unsigned int buildHeader(char* buffer, unsigned int len,
				unsigned ARFCN, unsigned TS, unsigned FN,
				GSM::TypeAndOffset to, bool is_saach,
				bool ul_dln, unsigned wType, unsigned int defSCN = 0)
{
	if (len < sizeof(struct gsmtap_hdr)) {
		LOG(NOTICE) << "Not enough space to build GSMTAP header";
		return 0;
	}
	// Decode TypeAndOffset
	uint8_t stype, scn;

	switch (to) {
		case GSM::TDMA_BEACON_BCCH:
			stype = GSMTAP_CHANNEL_BCCH;
			scn = 0;
			break;

		case GSM::TDMA_BEACON_CCCH:
			stype = GSMTAP_CHANNEL_CCCH;
			scn = 0;
			break;

		case GSM::SDCCH_4_0:
		case GSM::SDCCH_4_1:
		case GSM::SDCCH_4_2:
		case GSM::SDCCH_4_3:
			stype = GSMTAP_CHANNEL_SDCCH4;
			scn = to - GSM::SDCCH_4_0;
			break;

		case GSM::SDCCH_8_0:
		case GSM::SDCCH_8_1:
		case GSM::SDCCH_8_2:
		case GSM::SDCCH_8_3:
		case GSM::SDCCH_8_4:
		case GSM::SDCCH_8_5:
		case GSM::SDCCH_8_6:
		case GSM::SDCCH_8_7:
			stype = GSMTAP_CHANNEL_SDCCH8;
			scn = to - GSM::SDCCH_8_0;
			break;

		case GSM::TCHF_0:
			stype = GSMTAP_CHANNEL_TCH_F;
			scn = 0;
			break;

		case GSM::TCHH_0:
		case GSM::TCHH_1:
			stype = GSMTAP_CHANNEL_TCH_H;
			scn = to - GSM::TCHH_0;
			break;

		case GSM::TDMA_PDCH:	// packet data traffic logical channel, full speed.
			stype = GSMTAP_CHANNEL_PACCH;
			//stype = GSMTAP_CHANNEL_PDCH;
			scn = 0;	// Is this correct?
			break;
		case GSM::TDMA_PTCCH:		// packet data timing advance logical channel
			stype = GSMTAP_CHANNEL_PTCCH;
			scn = 0;
			break;
		case GSM::TDMA_PACCH:
			stype = GSMTAP_CHANNEL_PACCH;
			scn = 0;
			break;
		default:
			stype = GSMTAP_CHANNEL_UNKNOWN;
			scn = defSCN;
			break;
	}

	if (is_saach)
		stype |= GSMTAP_CHANNEL_ACCH;

	// Flags in ARFCN
	if (gConfig.getNum("GSM.Radio.Band") == 1900)
		ARFCN |= GSMTAP_ARFCN_F_PCS;

	if (ul_dln)
		ARFCN |= GSMTAP_ARFCN_F_UPLINK;

	// Build header
	struct gsmtap_hdr *header = (struct gsmtap_hdr *)buffer;
	header->version			= GSMTAP_VERSION;
	header->hdr_len			= sizeof(struct gsmtap_hdr) >> 2;
	header->type			= wType;	//GSMTAP_TYPE_UM;
	header->timeslot		= TS;
	header->arfcn			= htons(ARFCN);
	header->signal_dbm		= 0; /* FIXME */
	header->snr_db			= 0; /* FIXME */
	header->frame_number	= htonl(FN);
	header->sub_type		= stype;
	header->antenna_nr		= 0;
	header->sub_slot		= scn;
	header->res			= 0;

	return sizeof(*header);
}

bool socketActive()
{
	// Check if GSMTap is enabled
	if (!gConfig.defines("Control.GSMTAP.TargetIP")) 
		return false;
	// Port configuration
	unsigned port = GSMTAP_UDP_PORT;	// default port for GSM-TAP
	if (gConfig.defines("Control.GSMTAP.TargetPort"))
		port = gConfig.getNum("Control.GSMTAP.TargetPort");

	// Set socket destination
	GSMTAPSocket.destination(port,gConfig.getStr("Control.GSMTAP.TargetIP").c_str());
	return true;
}

void gWriteGSMTAP(unsigned ARFCN, unsigned TS, unsigned FN,
                  GSM::TypeAndOffset to, bool is_saach,
				  bool ul_dln,	// (pat) This flag means uplink
                  const BitVector& frame,
				  unsigned wType)	// Defaults to GSMTAP_TYPE_UM
{
	// Check if GSMTap is enabled
	if (!socketActive()) return;

	char buffer[MAX_DUMP_LENGTH];
	int ofs = 0;
	
	if (!(ofs = buildHeader(buffer,MAX_DUMP_LENGTH,ARFCN,TS,FN,
					to,is_saach,ul_dln,wType,0)))
		return;
	if ((ofs + (frame.size() + 7) / 8) > MAX_DUMP_LENGTH) {
		LOG(NOTICE) << "Built GSMTAP buffer exceeds max length=" << MAX_DUMP_LENGTH 
			<< " header_len=" << ofs << " data_len=" << frame.size() / 8  << ", not dumping";
		return;
	}
	// Add frame data
	frame.pack((unsigned char*)&buffer[ofs]);
	ofs += (frame.size() + 7) >> 3;

	// Write the GSMTAP packet
	GSMTAPSocket.write(buffer, ofs);
}

void gWriteGSMTAP(unsigned ARFCN, unsigned TS, unsigned FN,
                  GSM::TypeAndOffset to, bool is_saach, bool ul_dln,
                  const char* data, unsigned int len,
		  unsigned wType, unsigned int defSCN)
{
	if (!(data && len && socketActive()))
		return;
	char buffer[MAX_DUMP_LENGTH];
	int ofs = 0;
	if (!(ofs = buildHeader(buffer,MAX_DUMP_LENGTH,ARFCN,TS,FN,
					to,is_saach,ul_dln,wType,defSCN)))
		return;
	if (ofs + len > MAX_DUMP_LENGTH) {
		LOG(NOTICE) << "Built GSMTAP buffer exceeds max length=" << MAX_DUMP_LENGTH 
			<< " header_len=" << ofs << " data_len=" << len << ", not dumping";
		return;
	}
	// Add frame data
	::memcpy(&buffer[ofs],data,len);
	ofs += len;

	// Write the GSMTAP packet
	GSMTAPSocket.write(buffer, ofs);
}


// vim: ts=4 sw=4
