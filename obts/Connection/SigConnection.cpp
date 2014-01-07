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
#include <assert.h>
#include <string.h>

using namespace Connection;

#define PROTO_VER 1

bool SigConnection::send(Primitive prim, unsigned char info) const
{
    assert((prim & 0x80) != 0);
    if (!valid())
	return false;
    unsigned char buf[2];
    buf[0] = (unsigned char)prim;
    buf[1] = info;
    return GenConnection::send(buf,2);
}

bool SigConnection::send(Primitive prim, unsigned char info, unsigned int id) const
{
    assert((prim & 0x80) == 0);
    if (!valid())
	return false;
    unsigned char buf[4];
    buf[0] = (unsigned char)prim;
    buf[1] = info;
    buf[2] = (unsigned char)(id >> 8);
    buf[3] = (unsigned char)id;
    return GenConnection::send(buf,4);
}

bool SigConnection::send(Primitive prim, unsigned char info, unsigned int id, const void* data, size_t len) const
{
    assert((prim & 0x80) == 0);
    if (!valid())
	return false;
    unsigned char buf[len + 4];
    buf[0] = (unsigned char)prim;
    buf[1] = info;
    buf[2] = (unsigned char)(id >> 8);
    buf[3] = (unsigned char)id;
    ::memcpy(buf + 4,data,len);
    return GenConnection::send(buf,len + 4);
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

void SigConnection::process(const unsigned char* data, size_t len)
{
    if (len < 2) {
	LOG(ERR) << "received short message of length " << len;
	return;
    }
    if (data[0] & 0x80) {
	len -= 2;
	if (len)
	    process((Primitive)data[0],data[1],data + 2,len);
	else
	    process((Primitive)data[0],data[1]);
    }
    else {
	if (len < 4) {
	    LOG(ERR) << "received short message of length " << len << " for primitive " << (unsigned int)data[0];
	    return;
	}
	unsigned int id = (((unsigned int)data[2]) << 8) | data[3];
	len -= 4;
	if (len)
	    process((Primitive)data[0],data[1],id,data + 4,len);
	else
	    process((Primitive)data[0],data[1],id);
    }
}

/* vi: set ts=8 sw=4 sts=4 noet: */
