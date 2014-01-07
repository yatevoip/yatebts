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

using namespace Connection;

bool MediaConnection::send(unsigned int id, const void* data, size_t len) const
{
    if (!valid())
	return false;
    unsigned char buf[len + 2];
    buf[0] = (unsigned char)(id >> 8);
    buf[1] = (unsigned char)id;
    ::memcpy(buf + 2,data,len);
    return GenConnection::send(buf,len + 2);
}

void MediaConnection::process(const unsigned char* data, size_t len)
{
    if (len < 3) {
	LOG(ERR) << "received short message of length " << len;
	return;
    }
    unsigned int id = (((unsigned int)data[0]) << 8) | data[1];
    process(id,data + 2,len - 2);
}

void MediaConnection::process(unsigned int id, const unsigned char* data, size_t len)
{
    // TODO
}

/* vi: set ts=8 sw=4 sts=4 noet: */
