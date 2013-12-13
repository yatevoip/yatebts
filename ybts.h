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
    FDLog = 0,
    FDControl,
    FDStatus,
    FDSignalling,
    FDMedia,
    FDCount                          // Number of file descriptors
};


/*
 * Signalling interface protocol
 *
 */
// TODO: set the values
enum YBTSSigPrimitive {
    SigHandshake,
    SigHeartbeat,
};

}; // namespace YBTS

#endif // __YBTS_H

/* vi: set ts=8 sw=4 sts=4 noet: */
