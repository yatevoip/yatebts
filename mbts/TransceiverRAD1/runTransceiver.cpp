/*
* Copyright 2008, 2009 Free Software Foundation, Inc.
* Copyright 2010 Kestrel Signal Processing, Inc.
* Copyright (C) 2013-2014 Null Team Impex SRL
* Copyright (C) 2014 Legba, Inc

* This software is distributed under multiple licenses; see the COPYING file in the main directory for licensing information for this specific distribuion.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

*/


#include <stdlib.h>

#include "Transceiver.h"
#include "DummyLoad.h"

#include <time.h>
#include <signal.h>

#include <GSMCommon.h>
#include <Logger.h>
#include <Configuration.h>

#include "../Connection/GenConnection.h"
#include "../Connection/LogConnection.h"

using namespace std;

static const char *cOpenBTSConfigEnv = "MBTSConfigFile";
// Load configuration from a file.
ConfigurationTable gConfig(getenv(cOpenBTSConfigEnv)?getenv(cOpenBTSConfigEnv):"/etc/OpenBTS/OpenBTS.db","transceiver", getConfigurationKeys());

/** Connection to YBTS */
Connection::LogConnection gLogConn(STDERR_FILENO + 1);

volatile bool gbShutdown = false;
static void ctrlCHandler(int signo)
{
   cout << "Received shutdown signal" << endl;;
   gbShutdown = true;
}


int main(int argc, char *argv[])
{
  if ( signal( SIGINT, ctrlCHandler ) == SIG_ERR )
  {
    cerr << "Couldn't install signal handler for SIGINT" << endl;
    exit(1);
  }

  if ( signal( SIGTERM, ctrlCHandler ) == SIG_ERR )
  {
    cerr << "Couldn't install signal handler for SIGTERM" << endl;
    exit(1);
  }
  // Configure logger.
  gLogInit("transceiver",gConfig.getStr("Log.Level").c_str(),LOG_LOCAL7);
  if (gLogConn.valid())
    Log::gHook = Connection::LogConnection::hook;

  // Device specific global initialization
  RadioDevice::staticInit();

  int numARFCN=1;
  if (argc>1) numARFCN = atoi(argv[1]);

  std::string deviceArgs = "";
  if (argc>2) deviceArgs = argv[2];

  srandom(time(NULL));

  int mOversamplingRate = 1;
  switch(numARFCN) {

	  // DAVID COMMENT: I have no way to test this, but I would bet that you can
	  // just change these numbers to get different oversampling rates for single-ARFCN
	  // operation..

  case 1: 
	mOversamplingRate = 1;
	break;
  case 2:
	mOversamplingRate = 6;
 	break;
  case 3:
	mOversamplingRate = 8;
	break;
  case 4:
	mOversamplingRate = 12;
	break;
  case 5:
	mOversamplingRate = 16;
	break;
  default:
	break;
  }
  int minOver = gConfig.getNum("TRX.MinOversampling");
  if (mOversamplingRate < minOver)
    mOversamplingRate = minOver;
  //int mOversamplingRate = numARFCN/2 + numARFCN;
  //mOversamplingRate = 15; //mOversamplingRate*2;
  //if ((numARFCN > 1) && (mOversamplingRate % 2)) mOversamplingRate++;

/*
  RAD1Device *usrp = new RAD1Device(mOversamplingRate*1625.0e3/6.0);
  //DummyLoad *usrp = new DummyLoad(mOversamplingRate*1625.0e3/6.0);
  usrp->make(false, deviceID); 
*/
  RadioDevice* usrp = RadioDevice::make(mOversamplingRate);
  if (!usrp->open(deviceArgs)) {
    LOG(ALERT) << "Transceiver exiting..." << std::endl;
    return EXIT_FAILURE;
  }

  RadioInterface* radio = new RadioInterface(usrp,3,SAMPSPERSYM,SAMPSPERSYM*mOversamplingRate,false,numARFCN);
  Transceiver *trx = new Transceiver(gConfig.getNum("TRX.Port"),gConfig.getStr("TRX.IP").c_str(),SAMPSPERSYM,GSM::Time(2,0),radio,
				     numARFCN,mOversamplingRate,false);
  trx->receiveFIFO(radio->receiveFIFO());

/*
  signalVector *gsmPulse = generateGSMPulse(2,1);
  BitVector normalBurstSeg = "0000101010100111110010101010010110101110011000111001101010000";
  BitVector normalBurst(BitVector(normalBurstSeg,gTrainingSequence[0]),normalBurstSeg);
  signalVector *modBurst = modulateBurst(normalBurst,*gsmPulse,8,1);
  signalVector *modBurst9 = modulateBurst(normalBurst,*gsmPulse,9,1);
  signalVector *interpolationFilter = createLPF(0.6/mOversamplingRate,6*mOversamplingRate,1);
  signalVector totalBurst1(*modBurst,*modBurst9);
  signalVector totalBurst2(*modBurst,*modBurst);
  signalVector totalBurst(totalBurst1,totalBurst2);
  scaleVector(totalBurst,usrp->fullScaleInputValue());
  double beaconFreq = -1.0*(numARFCN-1)*200e3;
  signalVector finalVec(625*mOversamplingRate);
  for (int j = 0; j < numARFCN; j++) {
	signalVector *frequencyShifter = new signalVector(625*mOversamplingRate);
	frequencyShifter->fill(1.0);
	frequencyShift(frequencyShifter,frequencyShifter,2.0*M_PI*(beaconFreq+j*400e3)/(1625.0e3/6.0*mOversamplingRate));
  	signalVector *interpVec = polyphaseResampleVector(totalBurst,mOversamplingRate,1,interpolationFilter);
	multVector(*interpVec,*frequencyShifter);
	addVector(finalVec,*interpVec); 	
  }
  signalVector::iterator itr = finalVec.begin();
  short finalVecShort[2*finalVec.size()];
  short *shortItr = finalVecShort;
  while (itr < finalVec.end()) {
	*shortItr++ = (short) (itr->real());
	*shortItr++ = (short) (itr->imag());
	itr++;
  }
  usrp->loadBurst(finalVecShort,finalVec.size());
*/
  trx->start();
  gLogConn.write("Starting transceiver");
  //int i = 0;
  while(!gbShutdown) { sleep(1); } //i++; if (i==60) exit(1);}

  cout << "Shutting down transceiver..." << endl;

//  trx->stop();
  delete trx;
//  delete radio;
}

ConfigurationKeyMap getConfigurationKeys()
{
	ConfigurationKeyMap map;
	ConfigurationKey *tmp;

	tmp = new ConfigurationKey("TRX.RadioFrequencyOffset","128",
		"~170Hz steps",
		ConfigurationKey::FACTORY,
		ConfigurationKey::VALRANGE,
		"96:160",// educated guess
		true,
		"Fine-tuning adjustment for the transceiver master clock.  "
			"Roughly 170 Hz/step.  "
			"Set at the factory.  "
			"Do not adjust without proper calibration."
	);
	map[tmp->getName()] = *tmp;
	delete tmp;

	tmp = new ConfigurationKey("TRX.TxAttenOffset","0",
		"dB of attenuation",
		ConfigurationKey::FACTORY,
		ConfigurationKey::VALRANGE,
		"0:100",// educated guess
		true,
		"Hardware-specific gain adjustment for transmitter, matched to the power amplifier, expessed as an attenuationi in dB.  "
			"Set at the factory.  "
			"Do not adjust without proper calibration."
	);
	map[tmp->getName()] = *tmp;
	delete tmp;

	tmp = new ConfigurationKey("TRX.MinOversampling","1",
		"",
		ConfigurationKey::CUSTOMERWARN,
		ConfigurationKey::VALRANGE,
		"1:20",// educated guess
		true,
		"Minimum signal oversampling unless overriden by device or ARFCNs.  "
		"This parameter increases computational requirements of the transceiver."
	);
	map[tmp->getName()] = *tmp;
	delete tmp;

	tmp = new ConfigurationKey("TRX.IP","127.0.0.1",
		"",
		ConfigurationKey::CUSTOMERWARN,
		ConfigurationKey::IPADDRESS,
		"",
		true,
		"IP address of the transceiver application."
	);
	map[tmp->getName()] = *tmp;
	delete tmp;

	tmp = new ConfigurationKey("TRX.Port","5700",
		"",
		ConfigurationKey::FACTORY,
		ConfigurationKey::PORT,
		"",
		true,
		"IP port of the transceiver application."
	);
	map[tmp->getName()] = *tmp;
	delete tmp;

	return map;
}
