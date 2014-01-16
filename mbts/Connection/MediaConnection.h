/**
 * MediaConnection.h
 * This file is part of YateBTS
 *
 * Declaration for Media socket connection
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

#ifndef MEDIACONNECTION_H
#define MEDIACONNECTION_H

#include "GenConnection.h"

namespace Connection {

class MediaConnection : public GenConnection
{
public:
    inline MediaConnection(int fileDesc = -1)
	: GenConnection(fileDesc)
	{ }
    bool send(unsigned int id, const void* data, size_t len);
private:
    virtual void process(const unsigned char* data, size_t len);
    void process(unsigned int id, const unsigned char* data, size_t len);
};

}; // namespace Connection

#endif /* MEDIACONNECTION_H */

/* vi: set ts=8 sw=4 sts=4 noet: */
