/**
 * GenConnection.cpp
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * Generic socket connection
 *
 * Yet Another Telephony Engine - Base Transceiver Station
 * Copyright (C) 2013-2014 Null Team Impex SRL
 * Copyright (C) 2014 Legba, Inc
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

#include "GenConnection.h"

#include <Logger.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

using namespace Connection;

#define BUF_LEN 260
#define SLEEP_US 20000

GenConnection::~GenConnection()
{
    clear();
}

void GenConnection::clear()
{
    int fd = mSockFd;
    mSockFd = -1;
    if (fd >= 0)
	::close(fd);
}

bool GenConnection::initialize(int fileDesc)
{
    if (fileDesc < 0)
	return valid();
    linger l;
    l.l_onoff = 1;
    l.l_linger = 0;
    if (::setsockopt(fileDesc,SOL_SOCKET,SO_LINGER,&l,sizeof(l)))
	return false;
    mSockFd = fileDesc;
    if (mBufSize < BUF_LEN)
	mBufSize = BUF_LEN;
    return true;
}

bool GenConnection::start(const char* name)
{
    struct Local {
	static void* runFunc(void* ptr) {
	    static_cast<GenConnection*>(ptr)->run();
	    return 0;
	}
    };

    if (!valid())
	return false;
    mRecvThread.start(Local::runFunc,this,name);
    return true;
}

bool GenConnection::send(const void* buffer, size_t len)
{
    int fd = mSockFd;
    return (fd >= 0) && (::send(fd,buffer,len,0) == (int)len);
}

void GenConnection::run()
{
    unsigned char buf[mBufSize];
    struct timeval tOut;
    fd_set fSet;
    FD_ZERO(&fSet);
    LOG(INFO) << "starting thread loop";
    started();
    while (valid()) {
	pthread_testcancel();
	int fd = mSockFd;
	if (fd < 0)
	    break;
	tOut.tv_sec = 0;
	tOut.tv_usec = SLEEP_US;
	FD_SET(fd,&fSet);
	if (::select(fd + 1,&fSet,0,0,&tOut) < 0) {
	    if (errno != EAGAIN && errno != EINTR) {
		LOG(ERR) << "select() error " << errno << ": " << strerror(errno);
		break;
	    }
	    ::usleep(SLEEP_US);
	    idle();
	    continue;
	}
	if (!FD_ISSET(fd,&fSet)) {
	    idle();
	    continue;
	}
	ssize_t len = ::recv(fd,buf,mBufSize,MSG_DONTWAIT);
	if (!len) {
	    LOG(DEBUG) << "received EOF on socket";
	    break;
	}
	if (len > 0) {
	    if (len < mBufSize)
		buf[len] = 0;
	    process(buf,len);
	}
	else if (errno != EAGAIN && errno != EINTR) {
	    LOG(ERR) << "recv() error " << errno << ": " << strerror(errno);
	    break;
	}
    }
}

void GenConnection::started()
{
}

void GenConnection::idle()
{
}

/* vi: set ts=8 sw=4 sts=4 noet: */
