/**
 * CmdConnection.h
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * Declaration for Command socket connection
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

#ifndef CMDCONNECTION_H
#define CMDCONNECTION_H

#include "GenConnection.h"

namespace Connection {

class CmdConnection : public GenConnection
{
public:
    inline CmdConnection(int fileDesc = -1)
	: GenConnection(fileDesc,1024)
	{ }
    bool write(const char* text, size_t len = 0);
    char* read();
private:
    virtual void process(const unsigned char* data, size_t len);
};

}; // namespace Connection

#endif /* CMDCONNECTION_H */

/* vi: set ts=8 sw=4 sts=4 noet: */
