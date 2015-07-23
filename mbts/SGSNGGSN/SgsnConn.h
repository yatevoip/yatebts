/**
 * SgsnConn.h
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * Declaration for SGSN connection processing functions
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

#ifndef SGSNCONN_H
#define SGSNCONN_H

#include "Sgsn.h"

namespace SGSN {

enum GprsConnState {
    GprsConnNone  = -1,
    GprsConnLocal = -2,
};

class SgsnConn {
public:
    static void attachLBO(SgsnInfo* si);
    static void identityReq(SgsnInfo* si);
    static void authRequest(SgsnInfo* si, const char* text);
    static void attachAccept(SgsnInfo* si, const char* text);
    static void attachRej(SgsnInfo* si, unsigned char cause);
    static void detach(SgsnInfo* si, unsigned char cause, const char* text);
    static void pdpActivate(SgsnInfo* si, bool reply, const char* text);
    static void pdpModify(SgsnInfo* si, bool reply, const char* text);
    static void pdpDeactivate(SgsnInfo* si, const char* text);
    static void userData(SgsnInfo* si, const unsigned char* data, size_t len);
};

}; // namespace SGSN

#endif /* SGSNCONN_H */

/* vi: set ts=8 sw=4 sts=4 noet: */
