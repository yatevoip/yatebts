/**
 * SigConnection.cpp
 * This file is part of YateBTS
 *
 * Signaling socket connection
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

#include "SigConnection.h"

#include <Logger.h>
#include <Threads.h>

#include <sys/socket.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

using namespace Connection;

#define BUF_LEN 164

static int gSockFd = -1;
static Thread gRecvThread;

bool SigConnection::initialize(int fileDesc, unsigned char ident)
{
    // TODO
    return false;
}

bool SigConnection::start()
{
    if (gSockFd < 0)
	return false;
    gRecvThread.start(run,0);
    gRecvThread.join();
    return true;
}

bool SigConnection::send(Primitive prim, unsigned char info)
{
    assert((prim & 0x80) != 0);
    if (gSockFd < 0)
	return false;
    unsigned char buf[2];
    buf[0] = (unsigned char)prim;
    buf[1] = info;
    return send(buf,2);
}

bool SigConnection::send(Primitive prim, unsigned char info, unsigned int id)
{
    assert((prim & 0x80) == 0);
    if (gSockFd < 0)
	return false;
    unsigned char buf[4];
    buf[0] = (unsigned char)prim;
    buf[1] = info;
    buf[2] = (unsigned char)(id >> 8);
    buf[3] = (unsigned char)id;
    return send(buf,4);
}

bool SigConnection::send(Primitive prim, unsigned char info, unsigned int id, const void* data, size_t len)
{
    assert((prim & 0x80) == 0);
    if (gSockFd < 0)
	return false;
    unsigned char buf[len + 4];
    buf[0] = (unsigned char)prim;
    buf[1] = info;
    buf[2] = (unsigned char)(id >> 8);
    buf[3] = (unsigned char)id;
    ::memcpy(buf + 4,data,len);
    return send(buf,len + 4);
}

void SigConnection::process(Primitive prim, unsigned char info)
{
    switch (prim) {
	case SigHandshake:
	    // TODO
	    break;
	case SigHeartbeat:
	    // TODO
	    break;
	default:
	    LOG(ERR) << "unexpected primitive " << prim;
    }
}

void SigConnection::process(Primitive prim, unsigned char info, const unsigned char* data, size_t len)
{
    LOG(ERR) << "unexpected primitive " << prim << " with data";
}

void SigConnection::process(Primitive prim, unsigned char info, unsigned int id)
{
    switch (prim) {
	case SigConnRelease:
	    // TODO
	    break;
	default:
	    LOG(ERR) << "unexpected primitive " << prim << " with id " << id;
    }
}

void SigConnection::process(Primitive prim, unsigned char info, unsigned int id, const unsigned char* data, size_t len)
{
    switch (prim) {
	case SigL3Message:
	    // TODO
	    break;
	default:
	    LOG(ERR) << "unexpected primitive " << prim << " with id " << id << " and data";
    }
}

bool SigConnection::send(const void* buffer, size_t len)
{
    if (gSockFd < 0)
	return false;
    return ::send(gSockFd,buffer,len,0) == (int)len;
}

void* SigConnection::run(void*)
{
    unsigned char buf[BUF_LEN];
    LOG(INFO) << "starting thread loop";
    while (gSockFd >= 0) {
	pthread_testcancel();
	ssize_t len = ::recv(gSockFd,buf,BUF_LEN,MSG_DONTWAIT);
	if (!len)
	    break;
	if (len > 0) {
	    if (len < 2) {
		LOG(ERR) << "received short message of length " << len;
		continue;
	    }
	    if (buf[0] & 0x80) {
		len -= 2;
		if (len)
		    process((Primitive)buf[0],buf[1],buf + 2,len);
		else
		    process((Primitive)buf[0],buf[1]);
	    }
	    else {
		if (len < 4) {
		    LOG(ERR) << "received short message of length " << len << " for primitive " << (unsigned int)buf[0];
		    continue;
		}
		unsigned int id = (((unsigned int)buf[2]) << 8) | buf[3];
		len -= 4;
		if (len)
		    process((Primitive)buf[0],buf[1],id,buf + 4,len);
		else
		    process((Primitive)buf[0],buf[1],id);
	    }
	}
	else if (errno != EAGAIN && errno != EINTR)
	    break;
	// TODO: check timeouts, send heartbeat
    }
    LOG(INFO) << "exited thread loop";
    return 0;
}

/* vi: set ts=8 sw=4 sts=4 noet: */
