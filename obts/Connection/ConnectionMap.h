/**
 * ConnectionMap.h
 * This file is part of YateBTS
 *
 * Declaration for Connection to Logical Channel mapper
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

#ifndef CONNECTIONMAP_H
#define CONNECTIONMAP_H

#include <Threads.h>

namespace GSM {
    class LogicalChannel;
};

namespace Connection {

class ConnectionMap : public Mutex
{
public:
    ConnectionMap();
    int map(GSM::LogicalChannel* chan);
    void unmap(unsigned int id);
    void unmap(const GSM::LogicalChannel* chan);
    int find(const GSM::LogicalChannel* chan);
    GSM::LogicalChannel* find(unsigned int id);
private:
    unsigned int mIndex;
    GSM::LogicalChannel* mMap[65536];
};

}; // namespace Connection

#endif /* CONNECTIONMAP_H */

/* vi: set ts=8 sw=4 sts=4 noet: */
