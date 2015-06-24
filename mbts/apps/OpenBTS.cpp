/*
* Copyright 2008, 2009, 2010 Free Software Foundation, Inc.
* Copyright 2010 Kestrel Signal Processing, Inc.
* Copyright 2011, 2012 Range Networks, Inc.
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

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "config.h"

#include <Configuration.h>
std::vector<std::string> configurationCrossCheck(const std::string& key);
static const char *cOpenBTSConfigEnv = "MBTSConfigFile";
// Load configuration from a file.
ConfigurationTable gConfig(getenv(cOpenBTSConfigEnv)?getenv(cOpenBTSConfigEnv):"/etc/OpenBTS/OpenBTS.db","mbts", getConfigurationKeys());
#include <Logger.h>
Log dummy("openbts",gConfig.getStr("Log.Level").c_str(),LOG_LOCAL7);

// Set up the performance reporter.
#include <Reporting.h>
ReportingTable gReports(gConfig.getStr("Control.Reporting.StatsTable").c_str());

#include <TRXManager.h>
#include <GSML1FEC.h>
#include <GSMConfig.h>
#include <GSMSAPMux.h>
#include <GSML3RRMessages.h>
#include <GSMLogicalChannel.h>

#include <ControlCommon.h>
//#include <TransactionTable.h>

#include <Globals.h>

#include <CLI.h>
#include <PowerManager.h>
#include <Configuration.h>
#include <PhysicalStatus.h>
//#include <SubscriberRegistry.h>
#include "NeighborTable.h"

#include <sys/wait.h>

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

// (pat) mcheck.h is for mtrace, which permits memory leak detection.
// Set env MALLOC_TRACE=logfilename
// Call mtrace() in the program.
// post-process the logfilename with mtrace (a perl script.)
// #include <mcheck.h>

using namespace std;
using namespace GSM;

int gBtsXg = 0;		// Enable gprs

const char* gDateTime = __DATE__ " " __TIME__;


// All of the other globals that rely on the global configuration file need to
// be declared here.

// The TMSI Table.
//Control::TMSITable gTMSITable;

// The transaction table.
//Control::TransactionTable gTransactionTable;

// Physical status reporting
GSM::PhysicalStatus gPhysStatus;

// Configure the BTS object based on the config file.
// So don't create this until AFTER loading the config file.
GSMConfig gBTS;

// Note to all from pat:
// It is inadvisable to statically initialize any non-trivial entity here because
// the underlying dependencies may not yet have undergone their static initialization.
// For example, if any of these throw an alarm, the system will crash because
// the Logger may not have been initialized yet.

// Our interface to the software-defined radio.
TransceiverManager gTRX(gConfig.getNum("GSM.Radio.ARFCNs"), gConfig.getStr("TRX.IP").c_str(), gConfig.getNum("TRX.Port"));

// Subscriber registry and http authentication
//SubscriberRegistry gSubscriberRegistry;

/** The global neighbor table. */
Peering::NeighborTable gNeighborTable;

/** Connections to YBTS */
Connection::LogConnection gLogConn(STDERR_FILENO + 2);
Connection::CmdConnection gCmdConn(STDERR_FILENO + 3);
Connection::SigConnection gSigConn(STDERR_FILENO + 4);
Connection::MediaConnection gMediaConn(STDERR_FILENO + 5);
Connection::ConnectionMap gConnMap;
Connection::GprsConnMap gGprsMap;

void createStats()
{
	// count of OpenBTS start events
	gReports.create("OpenBTS.Starts");

	// count of watchdog restarts
	gReports.create("OpenBTS.Exit.Error.Watchdog");
	// count of aborts due to problems with CLI socket
	gReports.create("OpenBTS.Exit.Error.CLISocket");
	// count of aborts due to loss of transceiver heartbeat
	gReports.create("OpenBTS.Exit.Error.TransceiverHeartbeat");
	// count of aborts due to underfined nono-optional configuration parameters
	gReports.create("OpenBTS.Exit.Error.ConfigurationParameterNotFound");

	// count of CLI commands sent to OpenBTS
	gReports.create("OpenBTS.CLI.Command");
	// count of CLI commands where responses could not be returned
	gReports.create("OpenBTS.CLI.Command.ResponseFailure");

	// count of CS (non-GPRS) channel assignments
	gReports.create("OpenBTS.GSM.RR.ChannelAssignment");
	//gReports.create("OpenBTS.GSM.RR.ChannelRelease");
	// count of number of times the beacon was regenerated
	gReports.create("OpenBTS.GSM.RR.BeaconRegenerated");
	// count of successful channel assignments
	gReports.create("OpenBTS.GSM.RR.ChannelSiezed");
	//gReports.create("OpenBTS.GSM.RR.LinkFailure");
	//gReports.create("OpenBTS.GSM.RR.Paged.IMSI");
	//gReports.create("OpenBTS.GSM.RR.Paged.TMSI");
	//gReports.create("OpenBTS.GSM.RR.Handover.Inbound.Request");
	//gReports.create("OpenBTS.GSM.RR.Handover.Inbound.Accept");
	//gReports.create("OpenBTS.GSM.RR.Handover.Inbound.Success");
	//gReports.create("OpenBTS.GSM.RR.Handover.Outbound.Request");
	//gReports.create("OpenBTS.GSM.RR.Handover.Outbound.Accept");
	//gReports.create("OpenBTS.GSM.RR.Handover.Outbound.Success");
	// histogram of timing advance for accepted RACH bursts
	gReports.create("OpenBTS.GSM.RR.RACH.TA.Accepted",0,63);

	//gReports.create("Transceiver.StaleBurst");
	//gReports.create("Transceiver.Command.Received");
	//gReports.create("OpenBTS.TRX.Command.Sent");
	//gReports.create("OpenBTS.TRX.Command.Failed");
	//gReports.create("OpenBTS.TRX.FailedStart");
	//gReports.create("OpenBTS.TRX.LostLink");

	// GPRS
	// number of RACH bursts processed for GPRS
	gReports.create("GPRS.RACH");
	// number of TBFs assigned
	gReports.create("GPRS.TBF");
	// number of MSInfo records generated
	gReports.create("GPRS.MSInfo");
}




int main(int argc, char *argv[])
{
	// mtrace();	// Enable memory leak detection.  Unfortunately, huge amounts of code have been started in the constructors above.
	// TODO: Properly parse and handle any arguments
	if (argc > 1) {
		for (int argi = 1; argi < argc; argi++) {		// Skip argv[0] which is the program name.
			if (!strcmp(argv[argi], "--version") ||
			    !strcmp(argv[argi], "-v")) {
				cout << gVersionString << endl;
				continue;
			}
			if (!strcmp(argv[argi], "--gentex")) {
				cout << gConfig.getTeX(string(argv[0]), gVersionString) << endl;
				continue;
			}

			// (pat) Adding support for specified sql file.
			// Unfortunately, the Config table was inited quite some time ago,
			// so stick this arg in the environment, whence the ConfigurationTable can find it, and then reboot.
			if (!strcmp(argv[argi],"--config")) {
				if (++argi == argc) {
					LOG(ALERT) <<"Missing argument to --config option";
					exit(2);
				}
				setenv(cOpenBTSConfigEnv,argv[argi],1);
				execl(argv[0],"mbts",NULL);
				LOG(ALERT) <<"execl failed?  Exiting...";
				exit(0);
			}
			if (!strcmp(argv[argi],"--help")) {
				printf("OpenBTS [--version --gensql --genex] [--config file.db]\n");
				printf("OpenBTS exiting...\n");
				exit(0);
			}

			printf("OpenBTS: unrecognized argument: %s\nexiting...\n",argv[argi]);
		}

		return 0;
	}

	if (gLogConn.valid() && gCmdConn.valid() && gSigConn.valid() &&
			gMediaConn.valid() && gLogConn.write("MBTS connected to YBTS"))
		Log::gHook = Connection::LogConnection::hook;
	else {
		COUT("\nNot started by YBTS\n");
		exit(1);
	}

	createStats();

	gConfig.setCrossCheckHook(&configurationCrossCheck);

	gReports.incr("OpenBTS.Starts");

	try {

	srandom(time(NULL));

	gLogConn.write(gOpenBTSWelcome);
	gPhysStatus.open(gConfig.getStr("Control.Reporting.PhysStatusTable").c_str());
	gBTS.init();
	//gSubscriberRegistry.init();
	gParser.addCommands();

	if (gCmdConn.valid()) {
		gSigConn.start("bts:signaling");
		gLogConn.write("Starting MBTS...");
	}
	else
		COUT("\nStarting the system...");

	// Reset the transceiver
	if (!gTRX.reset()) {
		LOG(ALERT) << "Failed to reset transceiver";
		return 1;
	}
	gTRX.statistics(false);

	// Start the peer interface
//	gPeerInterface.start();

	// Sync factory calibration as defaults from radio EEPROM
	signed sdrsn = gTRX.ARFCN(0)->getFactoryCalibration("sdrsn");
	if (sdrsn != 0 && sdrsn != 65535) {
		signed val;

		val = gTRX.ARFCN(0)->getFactoryCalibration("band");
		if (gConfig.isValidValue("GSM.Radio.Band", val)) {
			gConfig.mSchema["GSM.Radio.Band"].updateDefaultValue(val);
		}

		val = gTRX.ARFCN(0)->getFactoryCalibration("rxgain");
		if (gConfig.isValidValue("GSM.Radio.RxGain", val)) {
			gConfig.mSchema["GSM.Radio.RxGain"].updateDefaultValue(val);
		}
	}

	//
	// Configure the radio.
	//

	gTRX.start();

	// Set up the interface to the radio.
	// Get a handle to the C0 transceiver interface.
	ARFCNManager* C0radio = gTRX.ARFCN(0);

	// Tuning.
	// Make sure its off for tuning.
	//C0radio->powerOff();
	// Get the ARFCN list.
	unsigned C0 = gConfig.getNum("GSM.Radio.C0");
	unsigned numARFCNs = gConfig.getNum("GSM.Radio.ARFCNs");
	// TODO what to do if tuning faild?
	C0radio->tune(C0,true);
	for (unsigned i=1; i<numARFCNs; i++) {
		// Tune the radios.
		unsigned ARFCN = C0 + i*2;
		LOG(INFO) << "tuning TRX " << i << " to ARFCN " << ARFCN;
		ARFCNManager* radio = gTRX.ARFCN(i);
		radio->tune(ARFCN);
	}

	// Send either TSC or full BSIC depending on radio need
	if (gConfig.getBool("GSM.Radio.NeedBSIC")) {
		// Send BSIC to 
		C0radio->setBSIC(gBTS.BSIC());
	} else {
		// Set TSC same as BCC everywhere.
		C0radio->setTSC(gBTS.BCC());
	}

	// Set maximum expected delay spread.
	C0radio->setMaxDelay(gConfig.getNum("GSM.Radio.MaxExpectedDelaySpread"));

	// Set Receiver Gain
	C0radio->setRxGain(gConfig.getNum("GSM.Radio.RxGain"));

	// Turn on and power up.
	C0radio->powerOn(true);
	C0radio->setPower(gConfig.getNum("GSM.Radio.PowerManager.MinAttenDB"));

	//
	// Create a C-V channel set on C0T0.
	//

	// C-V on C0T0
	C0radio->setSlot(0,5);
	// SCH
	SCHL1FEC SCH;
	SCH.downstream(C0radio);
	SCH.open();
	// FCCH
	FCCHL1FEC FCCH;
	FCCH.downstream(C0radio);
	FCCH.open();
	// BCCH
	BCCHL1FEC BCCH;
	BCCH.downstream(C0radio);
	BCCH.open();
	// RACH
	RACHL1FEC RACH(gRACHC5Mapping);
	RACH.downstream(C0radio);
	RACH.open();
	// CCCHs
	CCCHLogicalChannel CCCH0(gCCCH_0Mapping);
	CCCH0.downstream(C0radio);
	CCCH0.open();
	CCCHLogicalChannel CCCH1(gCCCH_1Mapping);
	CCCH1.downstream(C0radio);
	CCCH1.open();
	CCCHLogicalChannel CCCH2(gCCCH_2Mapping);
	CCCH2.downstream(C0radio);
	CCCH2.open();
	// use CCCHs as AGCHs
	gBTS.addAGCH(&CCCH0);
	gBTS.addAGCH(&CCCH1);
	gBTS.addAGCH(&CCCH2);

	// C-V C0T0 SDCCHs
	SDCCHLogicalChannel C0T0SDCCH[4] = {
		SDCCHLogicalChannel(0,0,gSDCCH_4_0),
		SDCCHLogicalChannel(0,0,gSDCCH_4_1),
		SDCCHLogicalChannel(0,0,gSDCCH_4_2),
		SDCCHLogicalChannel(0,0,gSDCCH_4_3),
	};
	Thread C0T0SDCCHControlThread[4];
	// Subchannel 2 used for CBCH if SMSCB enabled.
	bool SMSCB = (gConfig.getStr("Control.SMSCB.Table").length() != 0);
	CBCHLogicalChannel CBCH(gSDCCH_4_2);
	Thread CBCHControlThread;
	for (int i=0; i<4; i++) {
		if (SMSCB && (i==2)) continue;
		C0T0SDCCH[i].downstream(C0radio);
		C0T0SDCCHControlThread[i].start((void*(*)(void*))Control::DCCHDispatcher,&C0T0SDCCH[i],"bts:sdcch:%d",i);
		C0T0SDCCH[i].open();
		gBTS.addSDCCH(&C0T0SDCCH[i]);
	}
	// Install CBCH if used.
	if (SMSCB) {
		LOG(INFO) << "creating CBCH for SMSCB";
		CBCH.downstream(C0radio);
		CBCH.open();
		gBTS.addCBCH(&CBCH);
		CBCHControlThread.start((void*(*)(void*))Control::SMSCBSender,NULL,"bts:cbch");
	}


	//
	// Configure the other slots.
	//

	// Count configured slots.
	unsigned sCount = 1;


	if (!gConfig.defines("GSM.Channels.NumC1s")) {
		int numChan = numARFCNs*7;
		LOG(CRIT) << "GSM.Channels.NumC1s not defined. Defaulting to " << numChan << ".";
		gConfig.set("GSM.Channels.NumC1s",numChan);
	}
	if (!gConfig.defines("GSM.Channels.NumC7s")) {
		int numChan = numARFCNs-1;
		LOG(CRIT) << "GSM.Channels.NumC7s not defined. Defaulting to " << numChan << ".";
		gConfig.set("GSM.Channels.NumC7s",numChan);
	}

	if (gConfig.getBool("GSM.Channels.C1sFirst")) {
		// Create C-I slots.
		for (int i=0; i<gConfig.getNum("GSM.Channels.NumC1s"); i++) {
			gBTS.createCombinationI(gTRX,sCount/8,sCount%8);
			sCount++;
		}
	}

	// Create C-VII slots.
	for (int i=0; i<gConfig.getNum("GSM.Channels.NumC7s"); i++) {
		gBTS.createCombinationVII(gTRX,sCount/8,sCount%8);
		sCount++;
	}

	if (!gConfig.getBool("GSM.Channels.C1sFirst")) {
		// Create C-I slots.
		for (int i=0; i<gConfig.getNum("GSM.Channels.NumC1s"); i++) {
			gBTS.createCombinationI(gTRX,sCount/8,sCount%8);
			sCount++;
		}
	}

	if (sCount<(numARFCNs*8)) {
		LOG(CRIT) << "Only " << sCount << " timeslots configured in an " << numARFCNs << "-ARFCN system.";
	}

	// Set up idle filling on C0 as needed for unconfigured slots..
	while (sCount<8) {
		gBTS.createCombination0(gTRX,sCount);
		sCount++;
	}

	/* (pat) See GSM 05.02 6.5.2 and 3.3.2.3
		Note: The number of different paging subchannels on       
		the CCCH is:                                        
                                                           
		MAX(1,(3 - BS-AG-BLKS-RES)) * BS-PA-MFRMS           
			if CCCH-CONF = "001"                        
		(9 - BS-AG-BLKS-RES) * BS-PA-MFRMS                  
			for other values of CCCH-CONF               
	*/

	// Set up the pager.
	// Set up paging channels.
	// HACK -- For now, use a single paging channel, since paging groups are broken.
	gBTS.addPCH(&CCCH2);

	// Be sure we are not over-reserving.
	if (gConfig.getNum("GSM.Channels.SDCCHReserve")>=(int)gBTS.SDCCHTotal()) {
		unsigned val = gBTS.SDCCHTotal() - 1;
		LOG(CRIT) << "GSM.Channels.SDCCHReserve too big, changing to " << val;
		gConfig.set("GSM.Channels.SDCCHReserve",val);
	}


	// OK, now it is safe to start the BTS.
	gTRX.statistics(true);
	gBTS.start();

	if (gCmdConn.valid()) {
		gMediaConn.start("bts:media");
		gLogConn.write("MBTS ready");
		gSigConn.send(Connection::SigRadioReady);
		bool doStop = true;
		while (gLogConn.valid() && gSigConn.valid() && gMediaConn.valid()) {
			char* line = gCmdConn.read();
			if (!line)
				break;
			std::ostringstream sout;
			int res = gParser.process(line,sout);
			free(line);
			if (res < 0) {
				gTRX.stop();
				doStop = false;
			}
			if (sout.str().empty())
				gCmdConn.write("\r\n");
			else if (!gCmdConn.write(sout.str().c_str()))
				gCmdConn.write("Oops! Error returning command result!");
			if (res < 0)
				break;
		}
		if (doStop)
			gTRX.stop();
		gLogConn.write("MBTS exiting");
		return 0;
	}


	} // try

	catch (ConfigurationTableKeyNotFound e) {
		LOG(EMERG) << "required configuration parameter " << e.key() << " not defined, aborting";
		gReports.incr("OpenBTS.Exit.Error.ConfigurationParamterNotFound");
	}

	gTRX.stop();
	return 1;
}


/** Return warning strings about a potential conflicting value */
vector<string> configurationCrossCheck(const string& key) {
	vector<string> warnings;
	ostringstream warning;

	// Control.VEA depends on GSM.CellSelection.NECI
	if (key.compare("Control.VEA") == 0 || key.compare("GSM.CellSelection.NECI") == 0) {
		if (gConfig.getBool("Control.VEA") && gConfig.getStr("GSM.CellSelection.NECI").compare("1") != 0) {
			warning << "Control.VEA is enabled but will not be functional until GSM.CellSelection.NECI is set to \"1\"";
			warnings.push_back(warning.str());
			warning.str(std::string());
		}

	// GSM.Timer.T3212 should be a factor of six
	} else if (key.compare("GSM.Timer.T3212") == 0) {
		int gsm = gConfig.getNum("GSM.Timer.T3212");
		if (key.compare("GSM.Timer.T3212") == 0 && gsm % 6) {
			warning << "GSM.Timer.T3212 should be a factor of 6";
			warnings.push_back(warning.str());
			warning.str(std::string());
		}

	// GPRS.ChannelCodingControl.RSSI should normally be 10db more than GSM.Radio.RSSITarget
	} else if (key.compare("GPRS.ChannelCodingControl.RSSI") == 0 || key.compare("GSM.Radio.RSSITarget") == 0) {
		int gprs = gConfig.getNum("GPRS.ChannelCodingControl.RSSI");
		int gsm = gConfig.getNum("GSM.Radio.RSSITarget");
		if ((gprs - gsm) != 10) {
			warning << "GPRS.ChannelCodingControl.RSSI (" << gprs << ") should normally be 10db higher than GSM.Radio.RSSITarget (" << gsm << ")";
			warnings.push_back(warning.str());
			warning.str(std::string());
		}

	// TODO : This NEEDS to be an error not a warning. OpenBTS will fail to start because of an assert if an invalid value is used.
	// GSM.Radio.C0 needs to be inside the valid range of ARFCNs for GSM.Radio.Band
	} else if (key.compare("GSM.Radio.C0") == 0 || key.compare("GSM.Radio.Band") == 0) {
		int c0 = gConfig.getNum("GSM.Radio.C0");
		string band = gConfig.getStr("GSM.Radio.Band");
		string range;
		if (band.compare("850") == 0 && (c0 < 128 || 251 < c0)) {
			range = "128-251";
		} else if (band.compare("900") == 0 && (c0 < 1 || 124 < c0)) {
			range = "1-124";
		} else if (band.compare("1800") == 0 && (c0 < 512 || 885 < c0)) {
			range = "512-885";
		} else if (band.compare("1900") == 0 && (c0 < 512 || 810 < c0)) {
			range = "512-810";
		}
		if (range.length()) {
			warning << "GSM.Radio.C0 (" << c0 << ") falls outside the valid range of ARFCNs " << range << " for GSM.Radio.Band (" << band << ")";
			warnings.push_back(warning.str());
			warning.str(std::string());
		}

	// SGSN.Timer.ImplicitDetach should be at least 240 seconds greater than SGSN.Timer.RAUpdate"
	} else if (key.compare("SGSN.Timer.ImplicitDetach") == 0 || key.compare("SGSN.Timer.RAUpdate") == 0) {
		int detach = gConfig.getNum("SGSN.Timer.ImplicitDetach");
		int update = gConfig.getNum("SGSN.Timer.RAUpdate");
		if ((detach - update) < 240) {
			warning << "SGSN.Timer.ImplicitDetach (" << detach << ") should be at least 240 seconds greater than SGSN.Timer.RAUpdate (" << update << ")";
			warnings.push_back(warning.str());
			warning.str(std::string());
		}

	// GSM.CellSelection.NCCsPermitted needs to contain our own GSM.Identity.BSIC.NCC
	} else if (key.compare("GSM.CellSelection.NCCsPermitted") == 0 || key.compare("GSM.Identity.BSIC.NCC") == 0) {
		int ourNCCMask = gConfig.getNum("GSM.CellSelection.NCCsPermitted");
		int NCCMaskBit = 1 << gConfig.getNum("GSM.Identity.BSIC.NCC");
		if ((ourNCCMask >= 0) && ((NCCMaskBit & ourNCCMask) == 0)) {
			warning << "GSM.CellSelection.NCCsPermitted is not set to a mask which contains the local network color code defined in GSM.Identity.BSIC.NCC. ";
			warning << "Set GSM.CellSelection.NCCsPermitted to " << NCCMaskBit;
			warnings.push_back(warning.str());
			warning.str(std::string());
		}

	}

	return warnings;
}

// vim: ts=4 sw=4
