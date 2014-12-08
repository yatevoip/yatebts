//#define RN_DEVELOPER_MODE 1
/**@file Global system parameters. */
/*
* Copyright 2008, 2009 Free Software Foundation, Inc.
* Copyright 2011 Range Networks, Inc.
* Copyright (C) 2013-2014 Null Team Impex SRL
* Copyright (C) 2014 Legba, Inc
*
* This software is distributed under multiple licenses;
* see the COPYING file in the main directory for licensing
* information for this specific distribuion.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

*/

/*
	This file keeps global system parameters.
*/

#ifndef GLOBALS_H
#define GLOBALS_H

#include <GSMCommon.h>
#include <Configuration.h>
#include <CLI.h>
#include <PhysicalStatus.h>
//#include <TMSITable.h>
//#include <SubscriberRegistry.h>
#include <TRXManager.h>
#include <Reporting.h>
#include <LogConnection.h>
#include <CmdConnection.h>
#include <SigConnection.h>
#include <MediaConnection.h>
#include <ConnectionMap.h>
#include <GprsConnMap.h>


/** Date-and-time string, defined in OpenBTS.cpp. */
extern const char* gDateTime;

/**
	Just about everything goes into the configuration table.
	This should be defined in the main body of the top-level application.
*/
extern ConfigurationTable gConfig;

/** The OpenBTS welcome message. */
extern const char* gOpenBTSWelcome;

/** The OpenBTS version string. */
extern const char *gVersionString;

/** The central parser. */
extern CommandLine::Parser gParser;

/** The global TMSI table. */
//extern Control::TMSITable gTMSITable;

/** The physical status reporting table */
extern GSM::PhysicalStatus gPhysStatus;

/** The subscriber registry and authenticator */
//extern SubscriberRegistry gSubscriberRegistry;

/** The global transceiver interface. */
extern TransceiverManager gTRX;

/** Connections to YBTS */
extern Connection::LogConnection gLogConn;
extern Connection::CmdConnection gCmdConn;
extern Connection::SigConnection gSigConn;
extern Connection::MediaConnection gMediaConn;
extern Connection::ConnectionMap gConnMap;
extern Connection::GprsConnMap gGprsMap;

extern ReportingTable gReports;
#endif
