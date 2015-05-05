/**
 * MediaConnection.cpp
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * Media socket connection
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

#include "MediaConnection.h"
#include "ConnectionMap.h"
#include "GprsConnMap.h"

#include <Logger.h>
#include <Globals.h>
#include <GSMLogicalChannel.h>
#include <SgsnConn.h>

using namespace Connection;
using namespace GSM;
using namespace SGSN;

bool MediaConnection::send(unsigned int id, const void* data, size_t len, const void* data2, size_t len2)
{
    if (!valid())
	return false;
    if (!data2)
	len2 = 0;
    unsigned char buf[len + len2 + 2];
    buf[0] = (unsigned char)(id >> 8);
    buf[1] = (unsigned char)id;
    ::memcpy(buf + 2,data,len);
    if (len2)
	::memcpy(buf + len + 2,data2,len2);
    return GenConnection::send(buf,len + len2 + 2);
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
#ifdef XDEBUG
    Timeval timer;
#endif
    TCHFACCHLogicalChannel* tch = gConnMap.findMedia(id);
    if (tch) {
	// TODO: check frame size and blocking operation
	tch->sendTCH(data);
#ifdef XDEBUG
	long ms = timer.elapsed();
	if (ms > 50)
	    LOG(ERR) << "connection " << id << " sent " << len << " bytes voice frame in " << ms << " ms";
#endif
	return;
    }
    SgsnInfo* si = gGprsMap.find(id);
    if (si) {
	SgsnConn::userData(si,data,len);
#ifdef XDEBUG
	long ms = timer.elapsed();
	if (ms > 250)
	    LOG(ERR) << "connection " << id << " sent " << len << " bytes of data in " << ms << " ms";
#endif
	return;
    }
    LOG(ERR) << "received media frame for unmapped id " << id;
}

/* vi: set ts=8 sw=4 sts=4 noet: */
