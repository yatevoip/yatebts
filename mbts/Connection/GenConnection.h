/**
 * GenConnection.h
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * Declaration for a generic socket connection
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

#ifndef GENCONNECTION_H
#define GENCONNECTION_H

#include <Threads.h>

#include <sys/types.h>

namespace Connection {

class GenConnection
{
public:
    inline bool valid() const
	{ return mSockFd >= 0; }
    bool start(const char* name = 0);
    void clear();
protected:
    inline GenConnection(int fileDesc = -1, int bufSize = 0)
	: mSockFd(-1), mBufSize(bufSize)
	{ initialize(fileDesc); }
    virtual ~GenConnection();
    bool initialize(int fileDesc);
    virtual bool send(const void* buffer, size_t len);
    virtual void process(const unsigned char* data, size_t len) = 0;
    virtual void started();
    virtual void idle();
    virtual void run();
    int mSockFd;
    int mBufSize;
    Thread mRecvThread;
};

}; // namespace Connection

#endif /* GENCONNECTION_H */

/* vi: set ts=8 sw=4 sts=4 noet: */
