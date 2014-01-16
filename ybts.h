/**
 * ybts.h
 * This file is part of the 
 *
 * Yet Another BTS Channel
 *
 * Yet Another Telephony Engine - a fully featured software PBX and IVR
 * Copyright (C) 2004-2013 Null Team
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

namespace YBTS {

// File descriptors for sockets
// The enum is used as index starting from STDERR_FILENO + 1
enum YBTSFileDesc
{
    FDLogTransceiver = 0,
    FDLogBts,
    FDCommand,
    FDSignalling,
    FDMedia,
    FDCount                              // Number of file descriptors
};


/*
 * Signalling interface protocol
 *
 */
enum YBTSSigPrimitive {
    SigL3Message   =   0,                // Connection related L3 message
    SigConnLost    =   1,                // Connection to UE lost
    SigConnRelease =   2,                // Request to release a connection
    SigStartMedia  =   3,                // Start or assign traffic channel
    SigStopMedia   =   4,                // Stop sending media
    SigAllocMedia  =   5,                // Reserve a traffic channel
    SigMediaError  =   6,                // Report an allocation of mode change failure
    SigHandshake   = 128,                // Handshake
    SigRadioReady  = 129,                // Radio side is functioning
    SigStartPaging = 130,                // Start paging an UE
    SigStopPaging  = 131,                // Stop paging an UE
    SigHeartbeat   = 255                 // Heartbeat
};

enum YBTSPagingChanType {
    ChanTypeVoice  = 0,
    ChanTypeSMS    = 1,
    ChanTypeSS     = 2,
};

}; // namespace YBTS

#endif // __YBTS_H

/* vi: set ts=8 sw=4 sts=4 noet: */
