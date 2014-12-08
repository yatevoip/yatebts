/**
 * ybts.h
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * Yet Another BTS Channel
 *
 * Yet Another Telephony Engine - Base Transceiver Station
 * Copyright (C) 2013-2014 Null Team Impex SRL
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

#ifndef __YBTS_H
#define __YBTS_H

// File descriptors for sockets
// The enum is used as index starting from STDERR_FILENO + 1
enum BtsFileDesc
{
    FDLogTransceiver = 0,
    FDLogBts,
    FDCommand,
    FDSignalling,
    FDMedia,
    FDCount                              // Number of file descriptors
};

// Signalling interface protocol
enum BtsPrimitive {
    // CS primitives with connection ID
    SigL3Message        =   0,           // Connection related L3 message
    SigConnLost         =   1,           // Connection to UE lost
    SigConnRelease      =   2,           // Request to release a connection
    SigStartMedia       =   3,           // Start or assign traffic channel
    SigStopMedia        =   4,           // Stop sending media
    SigAllocMedia       =   5,           // Reserve a traffic channel
    SigMediaError       =   6,           // Report an allocation of mode change failure
    SigMediaStarted     =   7,           // Notify that the traffic channel has started
    SigEstablishSAPI    =   8,           // Request / notification about SAPI establishment
    SigPhysicalInfo     =   9,           // Physical channel information
    SigHandoverRequired =  10,           // Outbound handover required
    SigHandoverAck      =  11,           // Inbound handover acknowledge
    // PS primitives with connection ID
    SigGprsAttachReq    =  64,           // Attach Request
    SigGprsAttachLBO    =  65,           // Attach Local Breakout
    SigGprsAttachOk     =  66,           // Attach Accept
    SigGprsAttachRej    =  67,           // Attach Reject
    SigGprsIdentityReq  =  68,           // Identification Request
    SigGprsAuthRequest  =  69,           // Authentication Request
    SigGprsDetach       =  70,           // Detach Request
    SigPdpActivate      =  71,           // Activate PDP context
    SigPdpModify        =  72,           // Modify PDP context
    SigPdpDeactivate    =  73,           // Deactivate PDP context
    // Primitives without connection ID
    SigHandshake        = 128,           // Handshake
    SigRadioReady       = 129,           // Radio side is functioning
    SigStartPaging      = 130,           // Start paging an UE
    SigStopPaging       = 131,           // Stop paging an UE
    SigNeighborsList    = 132,           // Update the list of neighbor cells
    SigHandoverRequest  = 133,           // Requests resources for inbound handover
    SigHandoverReject   = 134,           // Handover resources allocation failed
    SigHeartbeat        = 255            // Heartbeat
};

// Paging channel types
enum BtsPagingChanType {
    ChanTypeVoice  = 0,
    ChanTypeSMS    = 1,
    ChanTypeSS     = 2,
};

// Traffic channel errors (subset of CC)
enum BtsErrors {
    ErrCongestion       = 0x22,          // No circuit/channel available
    ErrInterworking     = 0x7f           // Interworking, unspecified
};

#define YBTS_MCC_DEFAULT "001"
#define YBTS_MNC_DEFAULT "01"
#define YBTS_LAC_DEFAULT "1000"

#endif // __YBTS_H

/* vi: set ts=8 sw=4 sts=4 noet: */
