/**
 * ConnectionMap.h
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * Declaration for Connection to Logical Channel mapper
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

#ifndef CONNECTIONMAP_H
#define CONNECTIONMAP_H

#include <Threads.h>

#ifndef BTS_CONN_MAP_SIZE
#define BTS_CONN_MAP_SIZE 1024
#endif

namespace GSM {
    class LogicalChannel;
    class TCHFACCHLogicalChannel;
    class SACCHLogicalChannel;
};

namespace Connection {

class ConnectionMap : public Mutex
{
public:
    struct Conn {
	GSM::LogicalChannel* mChan;
	GSM::TCHFACCHLogicalChannel* mMedia;
	GSM::SACCHLogicalChannel* mSACCH;
	int mHoRef;
	int mTA;
    };
    ConnectionMap();
    int map(GSM::LogicalChannel* chan, GSM::SACCHLogicalChannel* sacch, int ref = -1);
    void mapMedia(unsigned int id, GSM::TCHFACCHLogicalChannel* media);
    GSM::LogicalChannel* unmap(unsigned int id);
    bool unmap(const GSM::LogicalChannel* chan);
    int remap(GSM::LogicalChannel* chan, GSM::TCHFACCHLogicalChannel* media, GSM::SACCHLogicalChannel* sacch);
    int find(const GSM::LogicalChannel* chan);
    int find(const GSM::SACCHLogicalChannel* chan);
    GSM::LogicalChannel* find(unsigned int id);
    GSM::TCHFACCHLogicalChannel* findMedia(unsigned int id);
    GSM::TCHFACCHLogicalChannel* findMedia(const GSM::LogicalChannel* chan);
    int findRef(int ref);
    inline void putTA(unsigned int id, int TA) { mMap[id].mTA = TA; }
    inline int takeTA(unsigned int id) { mMap[id].mHoRef = -1; return mMap[id].mTA; }
private:
    unsigned int mIndex;
    Conn mMap[BTS_CONN_MAP_SIZE];
};

}; // namespace Connection

#endif /* CONNECTIONMAP_H */

/* vi: set ts=8 sw=4 sts=4 noet: */
