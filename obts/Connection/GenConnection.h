/**
 * GenConnection.h
 * This file is part of YateBTS
 *
 * Declaration for a generic socket connection
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
    bool start();
protected:
    inline GenConnection(int fileDesc = -1)
	: mSockFd(-1)
	{ initialize(fileDesc); }
    virtual ~GenConnection();
    bool initialize(int fileDesc);
    bool send(const void* buffer, size_t len) const;
    virtual void process(const unsigned char* data, size_t len) = 0;
    virtual void idle();
    virtual void run();
    int mSockFd;
    Thread mRecvThread;
};

}; // namespace Connection

#endif /* GENCONNECTION_H */

/* vi: set ts=8 sw=4 sts=4 noet: */
