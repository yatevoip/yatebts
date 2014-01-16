/**
 * LogConnection.cpp
 * This file is part of YateBTS
 *
 * Logging socket connection
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

#include "LogConnection.h"

#include <Logger.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>

using namespace Connection;

bool LogConnection::write(const char* text)
{
    if (!text)
	return false;
    size_t len = ::strlen(text);
    return len && send(text,len);
}

void LogConnection::process(const unsigned char* data, size_t len)
{
    assert(false);
}

/* vi: set ts=8 sw=4 sts=4 noet: */
