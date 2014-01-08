/**
 * ConnectionMap.cpp
 * This file is part of YateBTS
 *
 * Connection to Logical Channel mapper
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

#include "ConnectionMap.h"

#include <Logger.h>

#include <string.h>

using namespace Connection;


ConnectionMap::ConnectionMap()
    : mIndex(0)
{
    memset(mMap,0,sizeof(mMap));
}

int ConnectionMap::map(GSM::LogicalChannel* chan)
{
    int id = find(chan);
    if (id >= 0)
	return id;
    lock();
    unsigned int i = mIndex;
    for (;;) {
	i = (i + 1) & 0xffff;
	if (i == mIndex)
	    break;
	if (!mMap[i]) {
	    id = mIndex = i;
	    break;
	}
    }
    unlock();
    if (id < 0)
	LOG(CRIT) << "connection map is full!";
    return id;
}

void ConnectionMap::unmap(unsigned int id)
{
    if (id < 65536)
	mMap[id] = 0;
}

void ConnectionMap::unmap(const GSM::LogicalChannel* chan)
{
    if (!chan)
	return;
    for (unsigned int i = 0; i < 65536; i++) {
	if (mMap[i] == chan) {
	    lock();
	    if (mMap[i] == chan)
		mMap[i] = 0;
	    unlock();
	    break;
	}
    }
}

int ConnectionMap::find(const GSM::LogicalChannel* chan)
{
    for (unsigned int i = 0; i < 65536; i++) {
	if (mMap[i] == chan)
	    return i;
    }
    return -1;
}

GSM::LogicalChannel* ConnectionMap::find(unsigned int id)
{
    return (id < 65536) ? mMap[id] : 0;
}

/* vi: set ts=8 sw=4 sts=4 noet: */
