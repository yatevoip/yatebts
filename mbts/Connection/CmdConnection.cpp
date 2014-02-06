/**
 * CmdConnection.cpp
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * Command socket connection
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

#include "CmdConnection.h"

#include <Logger.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>

using namespace Connection;

bool CmdConnection::write(const char* text, size_t len)
{
    if (text && !len)
	len = ::strlen(text);
    return text && len && send(text,len);
}

char* CmdConnection::read()
{
    char buf[mBufSize];
    for (;;) {
	int fd = mSockFd;
	if (fd < 0)
	    return 0;
	ssize_t len = ::read(fd,buf,mBufSize);
	if (!len) {
	    LOG(DEBUG) << "received EOF on socket";
	    return 0;
	}
	if (len > 0) {
	    if (len >= mBufSize)
		len = mBufSize - 1;
	    buf[len] = 0;
	    return ::strdup(buf);
	}
	else if (errno != EAGAIN && errno != EINTR) {
	    LOG(ERR) << "recv() error " << errno << ": " << strerror(errno);
	    return 0;
	}
    }
}

void CmdConnection::process(const unsigned char* data, size_t len)
{
    assert(false);
}

/* vi: set ts=8 sw=4 sts=4 noet: */
