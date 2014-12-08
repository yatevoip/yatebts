/**
 * GprsConnMap.cpp
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * Connection to GPRS info mapper
 *
 * Yet Another Telephony Engine - Base Transceiver Station
 * Copyright (C) 2014 Null Team Impex SRL
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

#include "GprsConnMap.h"

#include <Logger.h>

#include <string.h>

using namespace Connection;


GprsConnMap::GprsConnMap()
    : mIndex(0)
{
    memset(mMap,0,sizeof(mMap));
}

int GprsConnMap::map(SGSN::SgsnInfo* info)
{
    if (!info)
	return -1;
    int id = find(info);
    if (id >= 0)
	return id;
    lock();
    unsigned int i = mIndex;
    for (;;) {
	i = (i + 1) % BTS_GPRS_MAP_SIZE;
	if (i == mIndex)
	    break;
	if (!mMap[i].mInfo) {
	    mMap[i].mInfo = info;
	    mIndex = i;
	    id = BTS_GPRS_MAP_BASE + i;
	    break;
	}
    }
    unlock();
    if (id < 0)
	LOG(CRIT) << "connection map is full!";
    return id;
}

void GprsConnMap::remap(SGSN::SgsnInfo* info, unsigned int id)
{
    if (id < BTS_GPRS_MAP_BASE)
	return;
    id -= BTS_GPRS_MAP_BASE;
    if (id < BTS_GPRS_MAP_SIZE)
	mMap[id].mInfo = info;
}

SGSN::SgsnInfo* GprsConnMap::unmap(unsigned int id)
{
    if (id < BTS_GPRS_MAP_BASE)
	return 0;
    id -= BTS_GPRS_MAP_BASE;
    if ((id < BTS_GPRS_MAP_SIZE) && mMap[id].mInfo) {
	SGSN::SgsnInfo* info = mMap[id].mInfo;
	mMap[id].mInfo = 0;
	return info;
    }
    return 0;
}

bool GprsConnMap::unmap(const SGSN::SgsnInfo* info)
{
    if (!info)
	return false;
    for (unsigned int i = 0; i < BTS_GPRS_MAP_SIZE; i++) {
	Conn& c = mMap[i];
	if (c.mInfo == info) {
	    lock();
	    if (c.mInfo == info) {
		c.mInfo = 0;
	    }
	    unlock();
	    return true;
	}
    }
    return false;
}

int GprsConnMap::find(const SGSN::SgsnInfo* info)
{
    for (unsigned int i = 0; i < BTS_GPRS_MAP_SIZE; i++) {
	if (mMap[i].mInfo == info)
	    return BTS_GPRS_MAP_BASE + i;
    }
    return -1;
}

SGSN::SgsnInfo* GprsConnMap::find(unsigned int id)
{
    if (id < BTS_GPRS_MAP_BASE)
	return 0;
    id -= BTS_GPRS_MAP_BASE;
    return (id < BTS_GPRS_MAP_SIZE) ? mMap[id].mInfo : 0;
}

/* vi: set ts=8 sw=4 sts=4 noet: */
