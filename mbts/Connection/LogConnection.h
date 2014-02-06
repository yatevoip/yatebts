/**
 * LogConnection.h
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * Declaration for Logging socket connection
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

#ifndef LOGCONNECTION_H
#define LOGCONNECTION_H

#include "GenConnection.h"

namespace Connection {

class LogConnection : public GenConnection
{
public:
    inline LogConnection(int fileDesc = -1)
	: GenConnection(fileDesc)
	{ if (!gSelf) gSelf = this; }
    inline bool write(const char* text)
	{ return writeLog(0xff,text); }
    inline bool write(unsigned char level, const char* text)
	{ return writeLog(0x07 & level,text); }
    inline static LogConnection* self()
	{ return gSelf; }
    static bool hook(int level, const char* text, int offset);
private:
    virtual void process(const unsigned char* data, size_t len);
    bool writeLog(unsigned char level, const char* text);
    static LogConnection* gSelf;
};

}; // namespace Connection

#endif /* LOGCONNECTION_H */

/* vi: set ts=8 sw=4 sts=4 noet: */
