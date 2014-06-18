/**
 * ConnectionMap.cpp
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * Connection to Logical Channel mapper
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

#include "ConnectionMap.h"

#include <Logger.h>

#include <string.h>

using namespace Connection;


ConnectionMap::ConnectionMap()
    : mIndex(0)
{
    memset(mMap,0,sizeof(mMap));
}

int ConnectionMap::map(GSM::LogicalChannel* chan, GSM::SACCHLogicalChannel* sacch, int ref)
{
    if (!chan)
	return -1;
    int id = find(chan);
    if (id >= 0)
	return id;
    lock();
    unsigned int i = mIndex;
    for (;;) {
	i = (i + 1) % BTS_CONN_MAP_SIZE;
	if (i == mIndex)
	    break;
	if (!mMap[i].mChan) {
	    mMap[i].mChan = chan;
	    mMap[i].mMedia = 0;
	    mMap[i].mSACCH = sacch;
	    mMap[i].mHoRef = ref;
	    mMap[i].mTA = 0;
	    id = mIndex = i;
	    break;
	}
    }
    unlock();
    if (id < 0)
	LOG(CRIT) << "connection map is full!";
    return id;
}

void ConnectionMap::mapMedia(unsigned int id, GSM::TCHFACCHLogicalChannel* media)
{
    if ((id < BTS_CONN_MAP_SIZE) && mMap[id].mChan)
	mMap[id].mMedia = media;
}

GSM::LogicalChannel* ConnectionMap::unmap(unsigned int id)
{
    if ((id < BTS_CONN_MAP_SIZE) && mMap[id].mChan) {
	GSM::LogicalChannel* chan = mMap[id].mChan;
	mMap[id].mChan = 0;
	mMap[id].mMedia = 0;
	mMap[id].mSACCH = 0;
	return chan;
    }
    return 0;
}

bool ConnectionMap::unmap(const GSM::LogicalChannel* chan)
{
    if (!chan)
	return false;
    for (unsigned int i = 0; i < BTS_CONN_MAP_SIZE; i++) {
	Conn& c = mMap[i];
	if (c.mChan == chan) {
	    lock();
	    if (c.mChan == chan) {
		c.mChan = 0;
		c.mMedia = 0;
		c.mSACCH = 0;
		c.mHoRef = -1;
		c.mTA = 0;
	    }
	    unlock();
	    return true;
	}
    }
    return false;
}

int ConnectionMap::remap(GSM::LogicalChannel* chan, GSM::TCHFACCHLogicalChannel* media, GSM::SACCHLogicalChannel* sacch)
{
    int id = -1;
    lock();
    for (unsigned int i = 0; i < BTS_CONN_MAP_SIZE; i++) {
	Conn& c = mMap[i];
	if (c.mMedia == media) {
	    if (chan != c.mChan) {
		c.mChan = chan;
		c.mSACCH = sacch;
		c.mHoRef = -1;
		c.mTA = 0;
		id = i;
	    }
	    break;
	}
    }
    unlock();
    return id;
}

int ConnectionMap::find(const GSM::LogicalChannel* chan)
{
    for (unsigned int i = 0; i < BTS_CONN_MAP_SIZE; i++) {
	if (mMap[i].mChan == chan)
	    return i;
    }
    return -1;
}

int ConnectionMap::find(const GSM::SACCHLogicalChannel* chan)
{
    for (unsigned int i = 0; i < BTS_CONN_MAP_SIZE; i++) {
	if (mMap[i].mSACCH == chan)
	    return i;
    }
    return -1;
}

GSM::LogicalChannel* ConnectionMap::find(unsigned int id)
{
    return (id < BTS_CONN_MAP_SIZE) ? mMap[id].mChan : 0;
}

GSM::TCHFACCHLogicalChannel* ConnectionMap::findMedia(unsigned int id)
{
    return (id < BTS_CONN_MAP_SIZE) ? mMap[id].mMedia : 0;
}

GSM::TCHFACCHLogicalChannel* ConnectionMap::findMedia(const GSM::LogicalChannel* chan)
{
    for (unsigned int i = 0; i < BTS_CONN_MAP_SIZE; i++) {
	if (mMap[i].mChan == chan)
	    return mMap[i].mMedia;
    }
    return 0;
}

int ConnectionMap::findRef(int ref)
{
    if (ref < 0 || ref > 255)
	return -1;
    for (unsigned int i = 0; i < BTS_CONN_MAP_SIZE; i++) {
	if (mMap[i].mHoRef == ref)
	    return i;
    }
    return -1;
}

/* vi: set ts=8 sw=4 sts=4 noet: */
