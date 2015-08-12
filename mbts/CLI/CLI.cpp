/*
* Copyright 2009, 2010 Free Software Foundation, Inc.
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
#include <iomanip>
#include <fstream>
#include <iterator>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <algorithm>

#include <config.h>

// #include <Config.h>
#include <Logger.h>

#include <GSMConfig.h>
#include <GSMLogicalChannel.h>
#include <ControlCommon.h>
//#include <TransactionTable.h>
#include <TRXManager.h>
#include <PowerManager.h>
//#include <TMSITable.h>
#include <RadioResource.h>
//#include <CallControl.h>
#include <NeighborTable.h>
#include <Defines.h>
#include <GPRSExport.h>
#include <MAC.h>

namespace SGSN {
	extern int sgsnCLI(int argc, char **argv, std::ostream &os);
};

#include <Globals.h>

#include "CLI.h"

#undef WARNING

using namespace std;
using namespace CommandLine;

#define SUCCESS 0
#define BAD_NUM_ARGS 1
#define BAD_VALUE 2
#define NOT_FOUND 3
#define TOO_MANY_ARGS 4
#define FAILURE 5

extern TransceiverManager gTRX;

/** Standard responses in the CLI, much mach erorrCode enum. */
static const char* standardResponses[] = {
	"success", // 0
	"wrong number of arguments", // 1
	"bad argument(s)", // 2
	"command not found", // 3
	"too many arguments for parser", // 4
	"command failed", // 5
};




int Parser::execute(char* line, ostream& os) const
{
	LOG(INFO) << "executing console command: " << line;

	// tokenize
	char *argv[mMaxArgs];
	int argc = 0;
	char **ap;
	// This is (almost) straight from the man page for strsep.
	for (ap=argv; (*ap=strsep(&line," ")) != NULL; ) {
		if (**ap != '\0') {
			if (++ap >= &argv[mMaxArgs]) break;
			else argc++;
		}
	}
	// Blank line?
	if (!argc) return SUCCESS;
	// Find the command.
	ParseTable::const_iterator cfp = mParseTable.find(argv[0]);
	if (cfp == mParseTable.end()) {
		return NOT_FOUND;
	}
	int (*func)(int,char**,ostream&);
	func = cfp->second;
	// Do it.
	int retVal = (*func)(argc,argv,os);
	// Give hint on bad # args.
	if (retVal==BAD_NUM_ARGS) os << help(argv[0]) << endl;
	return retVal;
}


int Parser::process(const char* line, ostream& os) const
{
	char *newLine = strdup(line);
	int retVal = execute(newLine,os);
	free(newLine);
	if (retVal>0) os << standardResponses[retVal] << endl;
	return retVal;
}


const char * Parser::help(const string& cmd) const
{
	HelpTable::const_iterator hp = mHelpTable.find(cmd);
	if (hp==mHelpTable.end()) return "no help available";
	return hp->second.c_str();
}



/**@name Commands for the CLI. */
//@{

// forward refs
int printStats(int argc, char** argv, ostream& os);

/*
	A CLI command takes the argument in an array.
	It returns 0 on success.
*/

/** Display system uptime and current GSM frame number. */
int uptime(int argc, char** argv, ostream& os)
{
	if (argc!=1) return BAD_NUM_ARGS;
	os.precision(2);

	time_t now = time(NULL);
	const char* timestring = ctime(&now);
	// no endl since ctime includes a "\n" in the string
	os << "Unix time " << now << ", " << timestring << endl;

	int seconds = gBTS.uptime();
	if (seconds<120) {
		os << "uptime " << seconds << " seconds, frame " << gBTS.time() << endl;
		return SUCCESS;
	}
	float minutes = seconds / 60.0F;
	if (minutes<120) {
		os << "uptime " << minutes << " minutes, frame " << gBTS.time() << endl;
		return SUCCESS;
	}
	float hours = minutes / 60.0F;
	if (hours<48) {
		os << "uptime " << hours << " hours, frame " << gBTS.time() << endl;
		return SUCCESS;
	}
	float days = hours / 24.0F;
	os << "uptime " << days << " days, frame " << gBTS.time() << endl;

	return SUCCESS;
}


/** Give a list of available commands or describe a specific command. */
int showHelp(int argc, char** argv, ostream& os)
{
	if (argc==2) {
		os << argv[1] << " " << gParser.help(argv[1]) << endl;
		return SUCCESS;
	}
	if (argc!=1) return BAD_NUM_ARGS;
	ParseTable::const_iterator cp = gParser.begin();
	os << endl << "Type \"help\" followed by the command name for help on that command." << endl << endl;
	int c=0;
	const int cols = 3;
	while (cp != gParser.end()) {
		const string& wd = cp->first;
		os << wd << '\t';
		if (wd.size()<8) os << '\t';
		++cp;
		c++;
		if (c%cols==0) os << endl;
	}
	if (c%cols!=0) os << endl;
	return SUCCESS;
}



/** A function to return -1, the exit code for the caller. */
int exit_function(int argc, char** argv, ostream& os)
{
	unsigned wait =0;
	if (argc>2) return BAD_NUM_ARGS;
	if (argc==2) wait = atoi(argv[1]);

	if (wait!=0)
		 os << "waiting up to " << wait << " seconds for clearing of "
		<< gBTS.TCHActive() << " active calls" << endl;

	// Block creation of new channels.
	gBTS.hold(true);
	// Wait up to the timeout for active channels to release.
	time_t finish = time(NULL) + wait;
	while (time(NULL)<finish) {
		unsigned load = gBTS.SDCCHActive() + gBTS.TCHActive();
		if (load==0) break;
		sleep(1);
	}
	bool loads = false;
	if (gBTS.SDCCHActive()>0) {
		LOG(WARNING) << "dropping " << gBTS.SDCCHActive() << " control transactions on exit";
		loads = true;
	}
	if (gBTS.TCHActive()>0) {
		LOG(WARNING) << "dropping " << gBTS.TCHActive() << " calls on exit";
		loads = true;
	}
	if (loads) {
		os << endl << "exiting with loads:" << endl;
		printStats(1,NULL,os);
	}
	os << endl << "exiting..." << endl;
	return -1;
}



/** Print current usage loads. */
int printStats(int argc, char** argv, ostream& os)
{
	// FIXME -- This needs to take GPRS channels into account. See #762.
	if (argc!=1) return BAD_NUM_ARGS;
	os << "== GSM ==" << endl;
	os << "SDCCH load: " << gBTS.SDCCHActive() << '/' << gBTS.SDCCHTotal() << endl;
	os << "TCH/F load: " << gBTS.TCHActive() << '/' << gBTS.TCHTotal() << endl;
	os << "AGCH/PCH load: " << gBTS.AGCHLoad() << ',' << gBTS.PCHLoad() << endl;
	// paging table size
	os << "Paging table size: " << gBTS.pager().pagingEntryListSize() << endl;
	// 3122 timer current value (the number of seconds an MS should hold off the next RACH)
	os << "T3122: " << gBTS.T3122() << " ms (target " << gConfig.getNum("GSM.Radio.PowerManager.TargetT3122") << " ms)" << endl;
	os << "== GPRS ==" << endl;
	// (pat) We are not using dynamic channel allocation so I removed GPRS.Channels.Max until such time as we do.
	// Also I did not understand what is the point of printing out something that is in the config?
	//os << "chans mn/mx/dn/up: "
	//	<< gConfig.getNum("GPRS.Channels.Min") << '/' << gConfig.getNum("GPRS.Channels.Max")
	//	<< '/' << gConfig.getNum("GPRS.Multislot.Max.Downlink") << '/' << gConfig.getNum("GPRS.Multislot.Max.Uplink") << endl;
	os << "current PDCHs: " << GPRS::gL2MAC.macActiveChannels() << endl;
	os << "utilization: " << 100 * GPRS::gL2MAC.macComputeUtilization() << "%" << endl;
	return SUCCESS;
}



/** Get/Set MCC, MNC, LAC, CI. */
int cellID(int argc, char** argv, ostream& os)
{
	if (argc!=1) return BAD_NUM_ARGS;
	os << "MCC=" << gConfig.getStr("GSM.Identity.MCC")
	<< " MNC=" << gConfig.getStr("GSM.Identity.MNC")
	<< " LAC=" << gConfig.getNum("GSM.Identity.LAC")
	<< " CI=" << gConfig.getNum("GSM.Identity.CI")
	<< endl;
	return SUCCESS;
}

static string s_useReloadTrx[] = {"TRX.MinimumRxRSSI","TRX.RadioFrequencyOffset","TRX.TxAttenOffset",""};
static string s_removed[] = {"TRX.Args","TRX.Timeout.Start","TRX.Path",""};

static bool lookup(string* table, const char* value)
{
    if (!(table && value))
	return false;
    for (; table->length(); ++table)
	if (::strcmp(table->c_str(),value) == 0)
	    return true;
    return false;
}

/** Print or modify the global configuration table. */
int rawconfig(int argc, char** argv, ostream& os)
{
	if (argc > 1) {
		const char* dontHandle = 0;
		if (lookup(s_useReloadTrx,argv[1]))
			dontHandle = "Change config and reload the transceiver";
		else if (lookup(s_removed,argv[1]))
			dontHandle = "It was removed";
		if (dontHandle) {
			os << argv[1] << " can't be used with rawconfig. " << dontHandle << endl;
			return SUCCESS;
		}
	}

	// no args, just print
	if (argc==1) {
		gConfig.find("",os);
		return SUCCESS;
	}

	// one arg, pattern match and print
	if (argc==2) {
		gConfig.find(argv[1],os);
		return SUCCESS;
	}

	// >1 args: set new value
	string val;
	for (int i=2; i<argc; i++) {
		val.append(argv[i]);
		if (i!=(argc-1)) val.append(" ");
	}
	bool existing = gConfig.defines(argv[1]);
	string previousVal;
	if (existing) {
		previousVal = gConfig.getStr(argv[1]);
	}
	if (!gConfig.set(argv[1],val)) {
		os << "DB ERROR: " << argv[1] << " change failed" << endl;
		return FAILURE;
	}
	if (gConfig.isStatic(argv[1])) {
		os << argv[1] << " is static; change takes effect on restart" << endl;
	}
	if (!existing) {
		os << "defined new config " << argv[1] << " as \"" << val << "\"" << endl;
	} else {
		os << argv[1] << " changed from \"" << previousVal << "\" to \"" << val << "\"" << endl;
	}
	return SUCCESS;
}

int trxfactory(int argc, char** argv, ostream& os)
{
	if (argc!=1) return BAD_NUM_ARGS;

	signed val = gTRX.ARFCN(0)->getFactoryCalibration("sdrsn");
	if (val == 0 || val == 65535) {
		os << "Reading factory calibration not supported on this radio." << endl;
		return SUCCESS;
	}
	os << "Factory Information" << endl;
	os << "  SDR Serial Number = " << val << endl;

	val = gTRX.ARFCN(0)->getFactoryCalibration("rfsn");
	os << "  RF Serial Number = " << val << endl;

	val = gTRX.ARFCN(0)->getFactoryCalibration("band");
	os << "  GSM.Radio.Band = ";
	if (val == 0) {
		os << "multi-band";
	} else {
		os << val;
	}
	os << endl;

	val = gTRX.ARFCN(0)->getFactoryCalibration("rxgain");
	os << "  GSM.Radio.RxGain = " << val << endl;

	val = gTRX.ARFCN(0)->getFactoryCalibration("txgain");
	os << "  TRX.TxAttenOffset = " << val << endl;

	val = gTRX.ARFCN(0)->getFactoryCalibration("freq");
	os << "  TRX.RadioFrequencyOffset = " << val << endl;

	return SUCCESS;
}

/** Audit the current configuration. */
int audit(int argc, char** argv, ostream& os)
{
	ConfigurationKeyMap::iterator mp;
	stringstream ss;

	// value errors
	mp = gConfig.mSchema.begin();
	while (mp != gConfig.mSchema.end()) {
		if (!gConfig.isValidValue(mp->first, gConfig.getStr(mp->first))) {
			ss << mp->first << " \"" << gConfig.getStr(mp->first) << "\" (\"" << mp->second.getDefaultValue() << "\")" << endl;
		}
		mp++;
	}
	if (ss.str().length()) {
		os << "+---------------------------------------------------------------------+" << endl;
		os << "| ERROR : Invalid Values [key current-value (default)]                |" << endl;
		os << "|   To use the default value again, execute: rmconfig key             |" << endl;
		os << "+---------------------------------------------------------------------+" << endl;
		os << ss.str();
		os << endl;
		ss.str("");
	}

	// factory calibration warnings
	signed sdrsn = gTRX.ARFCN(0)->getFactoryCalibration("sdrsn");
	if (sdrsn != 0 && sdrsn != 65535) {
		string factoryValue;
		string configValue;

		factoryValue = gConfig.mSchema["GSM.Radio.Band"].getDefaultValue();
		configValue = gConfig.getStr("GSM.Radio.Band");
		// only warn on band changes if the unit is not multi-band
		if (gTRX.ARFCN(0)->getFactoryCalibration("band") != 0 && configValue.compare(factoryValue) != 0) {
			ss << "GSM.Radio.Band \"" << configValue << "\" (\"" << factoryValue << "\")" << endl;
		}

		factoryValue = gConfig.mSchema["GSM.Radio.RxGain"].getDefaultValue();
		configValue = gConfig.getStr("GSM.Radio.RxGain");
		if (configValue.compare(factoryValue) != 0) {
			ss << "GSM.Radio.RxGain \"" << configValue << "\" (\"" << factoryValue << "\")" << endl;
		}

		factoryValue = gConfig.mSchema["TRX.TxAttenOffset"].getDefaultValue();
		configValue = gConfig.getStr("TRX.TxAttenOffset");
		if (configValue.compare(factoryValue) != 0) {
			ss << "TRX.TxAttenOffset \"" << configValue << "\" (\"" << factoryValue << "\")" << endl;
		}

		factoryValue = gConfig.mSchema["TRX.RadioFrequencyOffset"].getDefaultValue();
		configValue = gConfig.getStr("TRX.RadioFrequencyOffset");
		if (configValue.compare(factoryValue) != 0) {
			ss << "TRX.RadioFrequencyOffset \"" << configValue << "\" (\"" << factoryValue << "\")" << endl;
		}

		if (ss.str().length()) {
			os << "+---------------------------------------------------------------------+" << endl;
			os << "| WARNING : Factory Radio Calibration [key current-value (factory)]   |" << endl;
			os << "|   To use the factory value again, execute: rmconfig key             |" << endl;
			os << "+---------------------------------------------------------------------+" << endl;
			os << ss.str();
			os << endl;
			ss.str("");
		}
	}

	// cross check warnings
	vector<string> allWarnings;
	vector<string> warnings;
	vector<string>::iterator warning;
	mp = gConfig.mSchema.begin();
	while (mp != gConfig.mSchema.end()) {
		warnings = gConfig.crossCheck(mp->first);
		allWarnings.insert(allWarnings.end(), warnings.begin(), warnings.end());
		mp++;
	}
	sort(allWarnings.begin(), allWarnings.end());
	allWarnings.erase(unique(allWarnings.begin(), allWarnings.end() ), allWarnings.end());
	warning = allWarnings.begin();
	while (warning != allWarnings.end()) {
		ss << *warning << endl;
		warning++;
	}
	if (ss.str().length()) {
		os << "+---------------------------------------------------------------------+" << endl;
		os << "| WARNING : Cross-Check Values                                        |" << endl;
		os << "|   To quiet these warnings, follow the advice given.                 |" << endl;
		os << "+---------------------------------------------------------------------+" << endl;
		os << ss.str();
		os << endl;
		ss.str("");
	}

	// site-specific values
	mp = gConfig.mSchema.begin();
	while (mp != gConfig.mSchema.end()) {
		if (mp->second.getVisibility() == ConfigurationKey::CUSTOMERSITE) {
			if (gConfig.getStr(mp->first).compare(gConfig.mSchema[mp->first].getDefaultValue()) == 0) {
				ss << mp->first << " \"" << gConfig.mSchema[mp->first].getDefaultValue() << "\"" << endl;
			}
		}
		mp++;
	}
	if (ss.str().length()) {
		os << "+---------------------------------------------------------------------+" << endl;
		os << "| WARNING : Site Values Which Are Still Default [key current-value]   |" << endl;
		os << "|   These should be set to fit your installation: config key value    |" << endl;
		os << "+---------------------------------------------------------------------+" << endl;
		os << ss.str();
		os << endl;
		ss.str("");
	}

	// non-default values
	mp = gConfig.mSchema.begin();
	while (mp != gConfig.mSchema.end()) {
		if (mp->second.getVisibility() != ConfigurationKey::CUSTOMERSITE) {
			if (gConfig.getStr(mp->first).compare(gConfig.mSchema[mp->first].getDefaultValue()) != 0) {
				ss << mp->first << " \"" << gConfig.getStr(mp->first) << "\" (\"" << mp->second.getDefaultValue() << "\")" << endl;
			}
		}
		mp++;
	}
	if (ss.str().length()) {
		os << "+---------------------------------------------------------------------+" << endl;
		os << "| INFO : Non-Default Values [key current-value (default)]             |" << endl;
		os << "|   To use the default value again, execute: rmconfig key             |" << endl;
		os << "+---------------------------------------------------------------------+" << endl;
		os << ss.str();
		os << endl;
		ss.str("");
	}

	// unknown pairs
	ConfigurationRecordMap pairs = gConfig.getAllPairs();
	ConfigurationRecordMap::iterator mp2 = pairs.begin();
	while (mp2 != pairs.end()) {
		if (!gConfig.keyDefinedInSchema(mp2->first)) {
			// also kindly ignore SIM.Prog keys for now so the users don't kill their ability to program SIMs
			string family = "SIM.Prog.";
			if (mp2->first.substr(0, family.size()) != family) {
				ss << mp2->first << " \"" << mp2->second.value() << "\"" << endl;
			}
		}
		mp2++;
	}
	if (ss.str().length()) {
		os << "+---------------------------------------------------------------------+" << endl;
		os << "| INFO : Custom/Deprecated Key/Value Pairs [key current-value]        |" << endl;
		os << "|   To clean up any extraneous keys, execute: rmconfig key            |" << endl;
		os << "+---------------------------------------------------------------------+" << endl;
		os << ss.str();
		os << endl;
		ss.str("");
	}

	return SUCCESS;
}

/** Print or modify the global configuration table. */
int _config(string mode, int argc, char** argv, ostream& os)
{
	// no args, just print
	if (argc==1) {
		ConfigurationKeyMap::iterator mp = gConfig.mSchema.begin();
		while (mp != gConfig.mSchema.end()) {
			if (mode.compare("customer") == 0) {
				if (mp->second.getVisibility() == ConfigurationKey::CUSTOMER ||
					mp->second.getVisibility() == ConfigurationKey::CUSTOMERSITE ||
					mp->second.getVisibility() == ConfigurationKey::CUSTOMERTUNE ||
					mp->second.getVisibility() == ConfigurationKey::CUSTOMERWARN) {
						ConfigurationKey::printKey(mp->second, gConfig.getStr(mp->first), os);
				}
			} else if (mode.compare("developer") == 0) {
				ConfigurationKey::printKey(mp->second, gConfig.getStr(mp->first), os);
			}
			mp++;
		}
		return SUCCESS;
	}

	// one arg
	if (argc==2) {
		// matches exactly? print single key
		if (gConfig.keyDefinedInSchema(argv[1])) {
			ConfigurationKey::printKey(gConfig.mSchema[argv[1]], gConfig.getStr(argv[1]), os);
			ConfigurationKey::printDescription(gConfig.mSchema[argv[1]], os);
			os << endl;
		// ...otherwise print all similar keys
		} else {
			int foundCount = 0;
			ConfigurationKeyMap matches = gConfig.getSimilarKeys(argv[1]);
			ConfigurationKeyMap::iterator mp = matches.begin();
			while (mp != matches.end()) {
				if (mode.compare("customer") == 0) {
					if (mp->second.getVisibility() == ConfigurationKey::CUSTOMER ||
						mp->second.getVisibility() == ConfigurationKey::CUSTOMERSITE ||
						mp->second.getVisibility() == ConfigurationKey::CUSTOMERTUNE ||
						mp->second.getVisibility() == ConfigurationKey::CUSTOMERWARN) {
							ConfigurationKey::printKey(mp->second, gConfig.getStr(mp->first), os);
							foundCount++;
					}
				} else if (mode.compare("developer") == 0) {
					ConfigurationKey::printKey(mp->second, gConfig.getStr(mp->first), os);
					foundCount++;
				}
				mp++;
			}
			if (!foundCount) {
				os << argv[1] << " - no keys matched";
				if (mode.compare("customer") == 0) {
					os << ", developer/factory keys can be accessed with \"devconfig.\"";
				} else if (mode.compare("developer") == 0) {
					os << ", custom keys can be accessed with \"rawconfig.\"";
				}
				os << endl;
			}
		}
		return SUCCESS;
	}

	// >1 args: set new value
	string val;
	for (int i=2; i<argc; i++) {
		val.append(argv[i]);
		if (i!=(argc-1)) val.append(" ");
	}
	if (!gConfig.keyDefinedInSchema(argv[1])) {
		os << argv[1] << " is not a valid key, change failed. If you're trying to define a custom key/value pair (e.g. the Log.Level.Filename.cpp pairs), use \"rawconfig.\"" << endl;
		return SUCCESS;
	}
	if (mode.compare("customer") == 0) {
		if (gConfig.mSchema[argv[1]].getVisibility() == ConfigurationKey::DEVELOPER) {
			os << argv[1] << " should only be changed by developers. Use \"devconfig\" if you are ABSOLUTELY sure this needs to be changed." << endl;
			return SUCCESS;
		}
		if (gConfig.mSchema[argv[1]].getVisibility() == ConfigurationKey::FACTORY) {
			os << argv[1] << " should only be set once by the factory. Use \"devconfig\" if you are ABSOLUTELY sure this needs to be changed." << endl;
			return SUCCESS;
		}
	}
	if (!gConfig.isValidValue(argv[1], val)) {
		os << argv[1] << " new value \"" << val << "\" is invalid, change failed.";
		if (mode.compare("developer") == 0) {
			os << " To override the configuration value checks, use \"rawconfig.\"";
		}
		os << endl;
		return SUCCESS;
	}

	string previousVal = gConfig.getStr(argv[1]);
	if (val.compare(previousVal) == 0) {
		os << argv[1] << " is already set to \"" << val << "\", nothing changed" << endl;
		return SUCCESS;
	}
// TODO : removing of default values from DB disabled for now. Breaks webui.
//	if (val.compare(gConfig.mSchema[argv[1]].getDefaultValue()) == 0) {
//		if (!gConfig.remove(argv[1])) {
//			os << argv[1] << " storing new value (default) failed" << endl;
//			return SUCCESS;
//		}
//	} else {
		if (!gConfig.set(argv[1],val)) {
			os << "DB ERROR: " << argv[1] << " could not be updated" << endl;
			return FAILURE;
		}
//	}
	vector<string> warnings = gConfig.crossCheck(argv[1]);
	vector<string>::iterator warning = warnings.begin();
	while (warning != warnings.end()) {
		os << "WARNING: " << *warning << endl;
		warning++;
	}
	if (gConfig.isStatic(argv[1])) {
		os << argv[1] << " is static; change takes effect on restart" << endl;
	}
	os << argv[1] << " changed from \"" << previousVal << "\" to \"" << val << "\"" << endl;

	return SUCCESS;
}

/** Print or modify the global configuration table. Customer access. */
int config(int argc, char** argv, ostream& os)
{
	return _config("customer", argc, argv, os);
}

/** Print or modify the global configuration table. Developer/factory access. */
int devconfig(int argc, char** argv, ostream& os)
{
	return _config("developer", argc, argv, os);
}

/** Disable a configuration key. */
int unconfig(int argc, char** argv, ostream& os)
{
	if (argc!=2) return BAD_NUM_ARGS;

	if (!gConfig.defines(argv[1])) {
		os << argv[1] << " is not in the table" << endl;
		return BAD_VALUE;
	}

	if (gConfig.keyDefinedInSchema(argv[1]) && !gConfig.isValidValue(argv[1], "")) {
		os << argv[1] << " is not disableable" << endl;
		return BAD_VALUE;
	}

	if (!gConfig.set(argv[1], "")) {
		os << "DB ERROR: " << argv[1] << " could not be disabled" << endl;
		return FAILURE;
	}

	os << argv[1] << " disabled" << endl;

	return SUCCESS;
}


/** Set a configuration value back to default or remove from table if custom key. */
int rmconfig(int argc, char** argv, ostream& os)
{
	if (argc!=2) return BAD_NUM_ARGS;

	if (!gConfig.defines(argv[1])) {
		os << argv[1] << " is not in the table" << endl;
		return BAD_VALUE;
	}

	// TODO : removing of default values from DB disabled for now. Breaks webui.
	if (gConfig.keyDefinedInSchema(argv[1])) {
		if (!gConfig.set(argv[1],gConfig.mSchema[argv[1]].getDefaultValue())) {
			os << "DB ERROR: " << argv[1] << " could not be set back to the default value" << endl;
			return FAILURE;
		}

		os << argv[1] << " set back to its default value" << endl;
		vector<string> warnings = gConfig.crossCheck(argv[1]);
		vector<string>::iterator warning = warnings.begin();
		while (warning != warnings.end()) {
			os << "WARNING: " << *warning << endl;
			warning++;
		}
		if (gConfig.isStatic(argv[1])) {
			os << argv[1] << " is static; change takes effect on restart" << endl;
		}
		return SUCCESS;
	}

	if (!gConfig.remove(argv[1])) {
		os << "DB ERROR: " << argv[1] << " could not be removed from the configuration table" << endl;
		return FAILURE;
	}

	os << argv[1] << " removed from the configuration table" << endl;

	return SUCCESS;
}

/** Reload configuration */
int reload(int argc, char** argv, ostream& os)
{
	if (argc!=1) return BAD_NUM_ARGS;

	if (gConfig.load()) {
		gBTS.regenerateBeacon();
		os << "Configuration reloaded" << endl;
	}
	else
		os << "Failed to reload configuration" << endl;
	return SUCCESS;
}


/** Change the registration timers. */
int regperiod(int argc, char** argv, ostream& os)
{
	if (argc==1) {
		os << "T3212 is " << gConfig.getNum("GSM.Timer.T3212") << " minutes" << endl;
		return SUCCESS;
	}

	if (argc>3) return BAD_NUM_ARGS;

	unsigned newT3212 = strtol(argv[1],NULL,10);
	if (!gConfig.isValidValue("GSM.Timer.T3212", argv[1])) {
		os << "valid T3212 range is 6..1530 minutes" << endl;
		return BAD_VALUE;
	}

	// Set the values in the table and on the GSM beacon.
	gConfig.set("GSM.Timer.T3212",newT3212);
	// Done.
	return SUCCESS;
}


/** Print the list of alarms kept by the logger, i.e. the last LOG(ALARM) << <text> */
int alarms(int argc, char** argv, ostream& os)
{
	std::ostream_iterator<std::string> output( os, "\n" );
	std::list<std::string> alarms = gGetLoggerAlarms();
	std::copy( alarms.begin(), alarms.end(), output );
	return SUCCESS;
}


/** Version string. */
int version(int argc, char **argv, ostream& os)
{
	if (argc!=1) return BAD_NUM_ARGS;
	os << gVersionString << endl;
	return SUCCESS;
}

/** Show start-up notices. */
int notices(int argc, char **argv, ostream& os)
{
	if (argc!=1) return BAD_NUM_ARGS;
	os << endl << gOpenBTSWelcome << endl;
	return SUCCESS;
}

int page(int argc, char **argv, ostream& os)
{
	if (argc==1) {
		gBTS.pager().dump(os);
		return SUCCESS;
	}
	return BAD_NUM_ARGS;
}





void printChanInfo(unsigned transID, const GSM::LogicalChannel* chan, ostream& os)
{
	os << setw(2) << chan->CN() << " " << chan->TN();
	os << " " << setw(9) << chan->typeAndOffset();
	os << " " << setw(12) << transID;
	os << " " << setw(6) << chan->active();
	os << " " << setw(5) << chan->recyclable();
	char buffer[200];
	snprintf(buffer,199,"%5.2f %4d %5d %4d",
		100.0*chan->FER(), (int)round(chan->RSSI()),
		chan->actualMSPower(), chan->actualMSTiming());
	os << " " << buffer;

	if (!chan->SACCH()) {
		os << endl;
		return;
	}

	const GSM::L3MeasurementResults& meas = chan->SACCH()->measurementResults();

	if (meas.MEAS_VALID()) {
		os << endl;
		return;
	}

	snprintf(buffer,199,"%5d %5.2f",
		meas.RXLEV_FULL_SERVING_CELL_dBm(),
		100.0*meas.RXQUAL_FULL_SERVING_CELL_BER());
	os << " " << buffer;

	if (meas.NO_NCELL()==0) {
		os << endl;
		return;
	}
	unsigned CN = meas.BCCH_FREQ_NCELL(0);
	std::vector<unsigned> ARFCNList = gNeighborTable.ARFCNList();
	if (CN>=ARFCNList.size()) {
		LOG(NOTICE) << "BCCH index " << CN << " does not match ARFCN list of size " << ARFCNList.size();
		os << endl;
		return;
	}
	snprintf(buffer,199,"%8u %8d",ARFCNList[CN],meas.RXLEV_NCELL_dBm(0));
	os << " " << buffer;
	os << endl;
}



int chans(int argc, char **argv, ostream& os)
{
	bool showAll = false;
	if (argc==2) showAll = true;
	if (argc>2) return BAD_NUM_ARGS;

	os << "CN TN chan      transaction active recyc UPFER RSSI TXPWR TXTA DNLEV DNBER Neighbor Neighbor" << endl;
	os << "CN TN type      id                       pct    dB   dBm  sym   dBm   pct    ARFCN    dBm" << endl;

	//gPhysStatus.dump(os);
	//os << endl << "Old data reporting: " << endl;

	// SDCCHs
	GSM::SDCCHList::const_iterator sChanItr = gBTS.SDCCHPool().begin();
	while (sChanItr != gBTS.SDCCHPool().end()) {
		const GSM::SDCCHLogicalChannel* sChan = *sChanItr;
		if (sChan->active() || showAll) {
			//Control::TransactionEntry *trans = gTransactionTable.find(sChan);
			int tid = 0;
			//if (trans) tid = trans->ID();
			printChanInfo(tid,sChan,os);
			//if (showAll) printChanInfo(tid,sChan->SACCH(),os);
		}
		++sChanItr;
	}

	// TCHs
	GSM::TCHList::const_iterator tChanItr = gBTS.TCHPool().begin();
	while (tChanItr != gBTS.TCHPool().end()) {
		const GSM::TCHFACCHLogicalChannel* tChan = *tChanItr;
		if (tChan->active() || showAll) {
			//Control::TransactionEntry *trans = gTransactionTable.find(tChan);
			int tid = 0;
			//if (trans) tid = trans->ID();
			printChanInfo(tid,tChan,os);
			//if (showAll) printChanInfo(tid,tChan->SACCH(),os);
		}
		++tChanItr;
	}

	os << endl;

	return SUCCESS;
}




int power(int argc, char **argv, ostream& os)
{
	os << "current downlink power " << gBTS.powerManager().power() << " dB wrt full scale" << endl;
	os << "current attenuation bounds "
		<< gConfig.getNum("GSM.Radio.PowerManager.MinAttenDB")
		<< " to "
		<< gConfig.getNum("GSM.Radio.PowerManager.MaxAttenDB")
		<< " dB" << endl;

	if (argc==1) return SUCCESS;
	if (argc!=3) return BAD_NUM_ARGS;

	int min = atoi(argv[1]);
	int max = atoi(argv[2]);
	if (min>max) {
		os << "Min is larger than max" << endl;
		return BAD_VALUE;
	}

	if (!gConfig.isValidValue("GSM.Radio.PowerManager.MinAttenDB", argv[1])) {
		os << "Invalid new value for min.  It must be in range (";
		os << gConfig.mSchema["GSM.Radio.PowerManager.MinAttenDB"].getValidValues() << ")" << endl;
		return BAD_VALUE;
	}
	if (!gConfig.isValidValue("GSM.Radio.PowerManager.MaxAttenDB", argv[2])) {
		os << "Invalid new value for max.  It must be in range (";
		os << gConfig.mSchema["GSM.Radio.PowerManager.MaxAttenDB"].getValidValues() << ")" << endl;
		return BAD_VALUE;
	}

	gConfig.set("GSM.Radio.PowerManager.MinAttenDB",argv[1]);
	gConfig.set("GSM.Radio.PowerManager.MaxAttenDB",argv[2]);

	os << "new attenuation bounds "
		<< gConfig.getNum("GSM.Radio.PowerManager.MinAttenDB")
		<< " to "
		<< gConfig.getNum("GSM.Radio.PowerManager.MaxAttenDB")
		<< " dB" << endl;

	return SUCCESS;
}


int rxgain(int argc, char** argv, ostream& os)
{
        os << "current RX gain is " << gConfig.getNum("GSM.Radio.RxGain") << " dB" << endl;
        if (argc==1) return SUCCESS;
	if (argc!=2) return BAD_NUM_ARGS;

	if (!gConfig.isValidValue("GSM.Radio.RxGain", argv[1])) {
		os << "Invalid new value for RX gain.  It must be in range (";
		os << gConfig.mSchema["GSM.Radio.RxGain"].getValidValues() << ")" << endl;
		return BAD_VALUE;
	}

        int newGain = gTRX.ARFCN(0)->setRxGain(atoi(argv[1]));
        os << "new RX gain is " << newGain << " dB" << endl;
  
        gConfig.set("GSM.Radio.RxGain",newGain);

        return SUCCESS;
}

int txatten(int argc, char** argv, ostream& os)
{
        os << "current TX attenuation is " << gConfig.getNum("TRX.TxAttenOffset") << " dB" << endl;
        if (argc==1) return SUCCESS;
        if (argc!=2) return BAD_NUM_ARGS;

		if (!gConfig.isValidValue("TRX.TxAttenOffset", argv[1])) {
			os << "Invalid new value for TX attenuation.  It must be in range (";
			os << gConfig.mSchema["TRX.TxAttenOffset"].getValidValues() << ")" << endl;
			return BAD_VALUE;
		}

        int newAtten = gTRX.ARFCN(0)->setTxAtten(atoi(argv[1]));
        os << "new TX attenuation is " << newAtten << " dB" << endl;

        gConfig.set("TRX.TxAttenOffset",newAtten);

        return SUCCESS;
}


int freqcorr(int argc, char** argv, ostream& os)
{
        os << "current freq. offset is " << gConfig.getNum("TRX.RadioFrequencyOffset") << endl;
        if (argc==1) return SUCCESS;
        if (argc!=2) return BAD_NUM_ARGS;

		if (!gConfig.isValidValue("TRX.RadioFrequencyOffset", argv[1])) {
			os << "Invalid new value for freq. offset  It must be in range (";
			os << gConfig.mSchema["TRX.RadioFrequencyOffset"].getValidValues() << ")" << endl;
			return BAD_VALUE;
		}

        int newOffset = gTRX.ARFCN(0)->setFreqOffset(atoi(argv[1]));
        os << "new freq. offset is " << newOffset << endl;

        gConfig.set("TRX.RadioFrequencyOffset",newOffset);

        return SUCCESS;
}


int noise(int argc, char** argv, ostream& os)
{
        if (argc!=1) return BAD_NUM_ARGS;

        int noise = gTRX.ARFCN(0)->getNoiseLevel();
        os << "noise RSSI is -" << noise << " dB wrt full scale" << endl;
	os << "MS RSSI target is " << gConfig.getNum("GSM.Radio.RSSITarget") << " dB wrt full scale" << endl;
	if (gConfig.getBool("GPRS.Enable"))
		os << "MS GPRS target is " << gConfig.getNum("GPRS.MS.Power.RSSITarget") << " dB wrt full scale" << endl;

        return SUCCESS;
}


int radio(int argc, char** argv, ostream& os)
{
        if (argc==1) return BAD_NUM_ARGS;

        std::string cmd;
        for (argc--, argv++; argc > 0; argc--, argv++) {
                cmd += *argv;
                if (argc > 1)
                        cmd += " ";
        }
        os << (gTRX.ARFCN(0)->runCustom(cmd.c_str()) ? "OK" : "Command rejected") << endl;

        return SUCCESS;
}


int sysinfo(int argc, char** argv, ostream& os)
{
        if (argc!=1) return BAD_NUM_ARGS;

	ScopedLock lock(gBTS.infoLock());
	const GSM::L3SystemInformationType1 *SI1 = gBTS.SI1();
	if (SI1) os << *SI1 << endl;
	const GSM::L3SystemInformationType2 *SI2 = gBTS.SI2();
	if (SI2) os << *SI2 << endl;
	const GSM::L3SystemInformationType3 *SI3 = gBTS.SI3();
	if (SI3) os << *SI3 << endl;
	const GSM::L3SystemInformationType4 *SI4 = gBTS.SI4();
	if (SI4) os << *SI4 << endl;
	const GSM::L3SystemInformationType5 *SI5 = gBTS.SI5();
	if (SI5) os << *SI5 << endl;
	const GSM::L3SystemInformationType6 *SI6 = gBTS.SI6();
	if (SI6) os << *SI6 << endl;

	return SUCCESS;
}


int crashme(int argc, char** argv, ostream& os)
{
	char *nullp = 0x0;
	// we actually have to output this,
	// or the compiler will optimize it out
	os << *nullp;
	return FAILURE;
}


int stats(int argc, char** argv, ostream& os)
{

	char cmd[200];
	if (argc==2) {
		if (strcmp(argv[1],"clear")==0) {
				gReports.clear();
				os << "stats table (gReporting) cleared" << endl;
				return SUCCESS;
		}
		sprintf(cmd,"sqlite3 %s 'select name||\": \"||value||\" events over \"||((%lu-clearedtime)/60)||\" minutes\" from reporting where name like \"%%%s%%\";' </dev/null",
			gConfig.getStr("Control.Reporting.StatsTable").c_str(), time(NULL), argv[1]);
	}
	else if (argc==1)
		sprintf(cmd,"sqlite3 %s 'select name||\": \"||value||\" events over \"||((%lu-clearedtime)/60)||\" minutes\" from reporting;' </dev/null",
			gConfig.getStr("Control.Reporting.StatsTable").c_str(), time(NULL));
	else return BAD_NUM_ARGS;
	FILE *result = popen(cmd,"r");
	char *line = (char*)malloc(200);
	while (!feof(result)) {
		if (!fgets(line, 200, result)) break;
		os << line;
	}
	free(line);
	os << endl;
	pclose(result);
	return SUCCESS;
}


//@} // CLI commands



void Parser::addCommands()
{
	addCommand("uptime", uptime, "-- show BTS uptime and BTS frame number.");
	addCommand("help", showHelp, "[command] -- list available commands or gets help on a specific command.");
	addCommand("shutdown", exit_function, "[wait] -- shut down or restart OpenBTS, either immediately, or waiting for existing calls to clear with a timeout in seconds");
	addCommand("load", printStats, "-- print the current activity loads.");
	addCommand("cellid", cellID, "-- print location area identity (MCC, MNC, LAC) and cell ID (CI)");
	addCommand("rawconfig", rawconfig, "[] OR [patt] OR [key val(s)] -- print the current configuration, print configuration values matching a pattern, or set/change a configuration value");
	addCommand("trxfactory", trxfactory, "-- print the radio's factory calibration and meta information");
	addCommand("audit", audit, "-- audit the current configuration for troubleshooting");
	addCommand("config", config, "[] OR [patt] OR [key val(s)] -- print the current configuration, print configuration values matching a pattern, or set/change a configuration value");
	addCommand("devconfig", devconfig, "[] OR [patt] OR [key val(s)] -- print the current configuration, print configuration values matching a pattern, or set/change a configuration value");
	addCommand("regperiod", regperiod, "[GSM] -- get/set the registration period (GSM T3212), in MINUTES");
	addCommand("alarms", alarms, "-- show latest alarms");
	addCommand("version", version,"-- print the version string");
	addCommand("page", page, "print the paging table");
	addCommand("chans", chans, "-- report PHY status for active channels");
	addCommand("power", power, "[minAtten maxAtten] -- report current attentuation or set min/max bounds");
        addCommand("rxgain", rxgain, "[newRxgain] -- get/set the RX gain in dB");
        addCommand("txatten", txatten, "[newTxAtten] -- get/set the TX attenuation in dB");
	addCommand("freqcorr", freqcorr, "[newOffset] -- get/set the new radio frequency offset");
        addCommand("noise", noise, "-- report receive noise level in RSSI dB");
        addCommand("radio", radio, "[command] -- execute custom radio command");
        addCommand("reload", reload, "-- reload configuration from file");
	addCommand("rmconfig", rmconfig, "key -- set a configuration value back to its default or remove a custom key/value pair");
	addCommand("unconfig", unconfig, "key -- disable a configuration key by setting an empty value");
	addCommand("notices", notices, "-- show startup copyright and legal notices");
	addCommand("sysinfo", sysinfo, "-- print current system information messages");
	addCommand("gprs", GPRS::gprsCLI,"GPRS mode sub-command.  Type: gprs help for more");
	addCommand("sgsn", SGSN::sgsnCLI,"SGSN mode sub-command.  Type: sgsn help for more");
	addCommand("crashme", crashme, "force crash of OpenBTS for testing purposes");
	addCommand("stats", stats,"[patt] OR clear -- print all, or selected, performance counters, OR clear all counters");
}




// vim: ts=4 sw=4
