/**
 * SigConnection.h
 * This file is part of YateBTS
 *
 * Declaration for Signaling socket connection
 *
 * Yet Another Telephony Engine - a fully featured software PBX and IVR
 * Copyright (C) 2004-2014 Null Team
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

#ifndef SIGCONNECTION_H
#define SIGCONNECTION_H

#include "GenConnection.h"

#include <Timeval.h>
#include <GSMTransfer.h>

namespace Connection {

class SigConnection : public GenConnection
{
public:
    enum Primitive {
	SigL3Message      =   0, // Connection related L3 message
	SigConnLost       =   1, // Connection to UE lost
	SigConnRelease    =   2, // Request to release a connection
	SigStartMedia     =   3, // Start or assign traffic channel
	SigStopMedia      =   4, // Stop sending media
	SigAllocMedia     =   5, // Reserve a traffic channel
	SigMediaError     =   6, // Report an allocation of mode change failure
	SigHandshake      = 128, // Initial handshake
	SigRadioReady     = 129, // Radio side is functioning
	SigHeartbeat      = 255  // Idle heartbeat message
    };
    inline SigConnection(int fileDesc = -1)
	: GenConnection(fileDesc), mHbRecv(0,0), mHbSend(0,0)
	{ }
    bool send(Primitive prim, unsigned char info = 0);
    bool send(Primitive prim, unsigned char info, unsigned int id);
    bool send(Primitive prim, unsigned char info, unsigned int id, const void* data, size_t len);
    bool send(unsigned char sapi, unsigned int id, const GSM::L3Frame* frame);
protected:
    virtual bool send(const void* buffer, size_t len);
    virtual void process(const unsigned char* data, size_t len);
    virtual void started();
    virtual void idle();
private:
    void process(Primitive prim, unsigned char info);
    void process(Primitive prim, unsigned char info, const unsigned char* data, size_t len);
    void process(Primitive prim, unsigned char info, unsigned int id);
    void process(Primitive prim, unsigned char info, unsigned int id, const unsigned char* data, size_t len);
    Timeval mHbRecv;
    Timeval mHbSend;
};

}; // namespace Connection

#endif /* SIGCONNECTION_H */

/* vi: set ts=8 sw=4 sts=4 noet: */
