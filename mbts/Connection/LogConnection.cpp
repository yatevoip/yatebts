/**
 * LogConnection.cpp
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * Logging socket connection
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

#include "LogConnection.h"

#include <Logger.h>

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

using namespace Connection;

LogConnection* LogConnection::gSelf = 0;

bool LogConnection::writeLog(unsigned char level, const char* text)
{
    if (!text)
	return false;
    size_t len = ::strlen(text);
    if (!len)
	return false;
    char* buf = (char*)::malloc(len + 1);
    if (!buf)
	return false;
    buf[0] = level;
    ::memcpy(buf + 1,text,len);
    bool ok = send(buf,len + 1);
    ::free(buf);
    return ok;
}

void LogConnection::process(const unsigned char* data, size_t len)
{
    assert(false);
}

bool LogConnection::hook(int level, const char* text, int offset)
{
    return self() && self()->write(level,text + offset);
}

/* vi: set ts=8 sw=4 sts=4 noet: */
