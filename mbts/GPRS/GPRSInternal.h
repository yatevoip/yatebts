/*
* Copyright 2011 Range Networks, Inc.
* All Rights Reserved.
*
* This software is distributed under multiple licenses;
* see the COPYING file in the main directory for licensing
* information for this specific distribuion.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef GPRSINTERNAL_H
#define GPRSINTERNAL_H
#include <stdint.h>
#include "GPRSRLC.h"


namespace GPRS {
	// FEC.h:
	class PDCHL1FEC;
	//class PTCCHL1Uplink;
	//class PDIdleL1Uplink;
	class PDCHL1Uplink;
	class PDCHL1Downlink;

	// MAC.h:
	class RLCBSN_t;
	class MSInfo;
	class L2MAC;
	class L1UplinkReservation;
	class L1USFTable;

	// GPRSL2RLCEngine.h:
	class RLCEngine;

	//TBF.h:
	class TBF;


	// RLCEngine.h:
	class RLCUpEngine;
	class RLCDownEngine;

	// RLCHdr.h:
	class RLCDownlinkBlock;
	class RLCUplinkDataBlock;
	class RLCDownlinkDataBlock;
	struct MACDownlinkHeader;
	struct MACUplinkHeader;
	struct RLCDownlinkControlBlockHeader;
	struct RLCUplinkControlBlockHeader;
	struct RLCSubBlockHeader;
	struct RLCSubBlockTLLI;
	struct RLCDownlinkDataBlockHeader;
	struct RLCUplinkDataBlockHeader;

	// RLCMessages.h:
	class RLCMessage;
	class RLCDownlinkMessage;
	class RLCUplinkMessage;
	struct RLCMsgPacketControlAcknowledgement;
	struct RLCMsgElementPacketAckNackDescription;
	struct RLCMsgElementChannelRequestDescription;
	struct RLCMsgElementRACapabilityValuePart;
	struct RLCMsgPacketDownlinkAckNack;
	struct RLCMsgPacketResourceRequest;
	struct RLCMsgPacketUplinkDummyControlBlock;
	struct RLCMsgPacketResourceRequest;
	//struct RLCMsgPacketMobileTBFStatus;
	class RLCMsgPacketUplinkAssignment;
	class RLCMsgPacketDownlinkAssignment;
	class RLCMsgPacketAccessReject;
	class RLCMsgPacketTBFRelease;
	class RLCMsgPacketUplinkAckNack;

	// LLC.h:
	class LLCFrame;

	//GPRSL2RLCElements.h:class RLCElement
	//GPRSL2RLCElements.h:class RLCAckNackDescription : public RLCElement
	//GPRSL2RLCElements.h:class RLCChannelQualityReport : public RLCElement
	//GPRSL2RLCElements.h:class RLCPacketTimingAdvance : public RLCElement
	//GPRSL2RLCElements.h:class RLCPacketPowerControlParameters : public RLCElement
	//GPRSL2RLCElements.h:class RLCChannelRequestDescription : public RLCElement

	// GPRSL2RLCMessages.h:
	//class RLCDownlinkMessage;
	//class RLCPacketDownlinkAckNack;
	//class RLCPacketUplinkAckNack;
	//class RLCPacketDownlinkControlBlock;
	//class RLCPacketUplinkControlBlock;

	int GetTimingAdvance(float timingError);
	int GetPowerAlpha();
	int GetPowerGamma();
	extern unsigned GPRSDebug;
	extern unsigned gGprsWatch;
	extern std::string fmtfloat2(float num);
};

#include "Defines.h"
#include "GSMConfig.h"		// For Time
#include "GSMCommon.h"		// For ChannelType
#include "GPRSExport.h"
#include "Utils.h"

#include "Logger.h"
// Redefine GPRSLOG to include the current RLC BSN when called in this directory.
#ifdef GPRSLOG
#undef GPRSLOG
#endif

// BITS for activating levels of debug through GPRSDebug
#define GPRS_ERR        0x01  // report an error, default value for GPRSDebug
#define GPRS_OK         0x02  // report a succesful outcome
#define GPRS_CHECK_FAIL 0x04  // report that a condition check has failed
#define GPRS_CHECK_OK   0x08  // report that a condition check has passed
#define GPRS_LOOP       0x10  // print debug information in loops
#define GPRS_MSG        0x20  // report sending/receiving of messages

#define IS_SET_GPRSDEBUG(bitmask) (GPRS::GPRSDebug & (bitmask))

#define PS_LOG(level,bitmask) if (IS_SET_GPRSDEBUG(bitmask) && IS_LOG_LEVEL(level)) \
	_LOG(level) << ": "

#define GPRSLOG(level,bitmask) PS_LOG(level,bitmask) << "GPRS::gBSNNext=" << GPRS::gBSNNext << ": "


#define LOGWATCHF(...) if (GPRS::gGprsWatch&1) printf(__VA_ARGS__); GPRSLOG(INFO,GPRS_MSG)<<"watch:"<<format(__VA_ARGS__);

// If gprs debugging is on, print these messages regardless of Log.Level.
#define GLOG(wLevel) if (GPRS::GPRSDebug || IS_LOG_LEVEL(wLevel)) _LOG(wLevel) << " "<<timestr()<<","<<GPRS::gBSNNext<<":"

// Like assert() but dont core dump unless we are testing.
#define devassert(code) {if (GPRS::GPRSDebug||IS_LOG_LEVEL(DEBUG)) {assert(code);} else if (!(code)) {LOG(ERR)<<"assertion failed:"<< #code;}}


#endif
