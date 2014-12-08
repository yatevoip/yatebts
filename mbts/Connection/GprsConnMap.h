/**
 * GprsConnMap.h
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * Declaration for Connection to GPRS info mapper
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

#ifndef GPRSCONNMAP_H
#define GPRSCONNMAP_H

#include <Threads.h>

#ifndef BTS_GPRS_MAP_BASE
#define BTS_GPRS_MAP_BASE 8192
#endif
#ifndef BTS_GPRS_MAP_SIZE
#define BTS_GPRS_MAP_SIZE 1024
#endif

namespace SGSN {
    class SgsnInfo;
};

namespace Connection {

class GprsConnMap : public Mutex
{
public:
    struct Conn {
	SGSN::SgsnInfo* mInfo;
    };
    GprsConnMap();
    int map(SGSN::SgsnInfo* info);
    void remap(SGSN::SgsnInfo* info, unsigned int id);
    SGSN::SgsnInfo* unmap(unsigned int id);
    bool unmap(const SGSN::SgsnInfo* info);
    int find(const SGSN::SgsnInfo* info);
    SGSN::SgsnInfo* find(unsigned int id);
private:
    unsigned int mIndex;
    Conn mMap[BTS_GPRS_MAP_SIZE];
};

}; // namespace Connection

#endif /* GPRSCONNMAP_H */

/* vi: set ts=8 sw=4 sts=4 noet: */
