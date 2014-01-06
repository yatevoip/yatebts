/**
 * MediaConnection.cpp
 * This file is part of YateBTS
 *
 * Media socket connection
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

#include "MediaConnection.h"

#include <Logger.h>
#include <Threads.h>

#include <sys/socket.h>
#include <errno.h>
#include <string.h>

using namespace Connection;

#define BUF_LEN 164

static int gSockFd = -1;
static Thread gRecvThread;

bool MediaConnection::initialize(int fileDesc)
{
    // TODO
    return false;
}

bool MediaConnection::start()
{
    if (gSockFd < 0)
	return false;
    gRecvThread.start(run,0);
    gRecvThread.join();
    return true;
}

bool MediaConnection::send(unsigned int id, const void* data, size_t len)
{
    if (gSockFd < 0)
	return false;
    unsigned char buf[len + 2];
    buf[0] = (unsigned char)(id >> 8);
    buf[1] = (unsigned char)id;
    ::memcpy(buf + 2,data,len);
    return send(buf,len + 2);
}

void MediaConnection::process(unsigned int id, const unsigned char* data, size_t len)
{
    // TODO
}

bool MediaConnection::send(const void* buffer, size_t len)
{
    if (gSockFd < 0)
	return false;
    return ::send(gSockFd,buffer,len,0) == (int)len;
}

void* MediaConnection::run(void*)
{
    unsigned char buf[BUF_LEN];
    LOG(INFO) << "starting thread loop";
    while (gSockFd >= 0) {
	pthread_testcancel();
	ssize_t len = ::recv(gSockFd,buf,BUF_LEN,MSG_DONTWAIT);
	if (!len)
	    break;
	if (len > 0) {
	    if (len < 3) {
		LOG(ERR) << "received short message of length " << len;
		continue;
	    }
	    unsigned int id = (((unsigned int)buf[0]) << 8) | buf[1];
	    process(id,buf + 2,len - 2);
	}
	else if (errno != EAGAIN && errno != EINTR)
	    break;
	// TODO: check timeouts, send heartbeat
    }
    LOG(INFO) << "exited thread loop";
    return 0;
}

/* vi: set ts=8 sw=4 sts=4 noet: */
