/*
* Copyright 2008, 2010 Free Software Foundation, Inc.
* Copyright 2012 Range Networks, Inc.
* Copyright (C) 2014 Null Team Impex SRL
* Copyright (C) 2014 Legba, Inc
*
* This software is distributed under multiple licenses; see the COPYING file in the main directory for licensing information for this specific distribuion.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

*/




#include <Logger.h>

#include "TRXManager.h"

#include <Globals.h>
#include <GSMCommon.h>
#include <GSMTransfer.h>
#include <GSMLogicalChannel.h>
#include <GSMConfig.h>
#include <GSML1FEC.h>

#include <Reporting.h>

#include <string>
#include <string.h>
#include <stdlib.h>

#undef WARNING


using namespace GSM;
using namespace std;

static inline void addAddr(std::string& dest, const char* ip, int port, const char* prefix = "")
{
    char tmp[256];
    sprintf(tmp,"%s%s:%d",prefix,ip,port);
    dest.append(tmp);
}

// Send terminate signal to upper layer
// Wait for thread to be cancelled (give upper layer some time to terminate)
// exit/abort application
static void terminate(unsigned int info, int doExit = 1, const void* data = 0, int len = 0,
	unsigned int waitMs = 3000)
{
	if (info < 256 && gSigConn.valid()) {
		if (data && len)
			gSigConn.send(Connection::SigStop,info,data,len);
		else
			gSigConn.send(Connection::SigStop,info);
		// Make sure the upper layer reads our notification
		if (!waitMs)
			waitMs = 200;
	}
	for (waitMs += waitMs & 19; waitMs; waitMs -= 20) {
		pthread_testcancel();
		msleep(20);
	}
	if (doExit > 0)
		exit(0);
	else if (doExit < 0)
		abort();
}


TransceiverManager::TransceiverManager(int numARFCNs, int wBasePort,
		const char* wTRXAddress, const char* wLocalAddr)
	:mHaveClock(false),
	mClockSocket(wBasePort+3,wLocalAddr),
	mControlSocket(wBasePort+1,wTRXAddress,wBasePort,wLocalAddr),
	mExitRecv(false), m_statistics(false)
{
	addAddr(mInitData,wLocalAddr,wBasePort + 1,"\r\nControl: ");
	addAddr(mInitData,wTRXAddress,wBasePort," - ");
	addAddr(mInitData,wLocalAddr,wBasePort + 3,"\r\nClock: ");
	// set up the ARFCN managers
	int p = wBasePort + 4;
	for (int i=0; i<numARFCNs; i++, p += 2) {
		mARFCNs.push_back(new ::ARFCNManager(i,wTRXAddress,p,wLocalAddr,*this));
		char tmp[50];
		sprintf(tmp,"\r\nARFCN[%d]: ",i);
		addAddr(mInitData,wLocalAddr,p + 1,tmp);
		addAddr(mInitData,wTRXAddress,p," - ");
	}
}

void TransceiverManager::logInit()
{
	LOG(INFO) << "Initialized:" <<
		"\r\n-----" <<
		"\r\nARFCNS: " << mARFCNs.size() <<
		mInitData <<
		"\r\n-----";
}

void TransceiverManager::start()
{
	mClockThread.start((void*(*)(void*))ClockLoopAdapter,this,"bts:clock");
	for (unsigned i=0; i<mARFCNs.size(); i++) {
		mARFCNs[i]->start();
	}
}


int TransceiverManager::sendCommandPacket(const char* command, char* response)
{
	if (exiting())
		return -2;

	int msgLen = 0;
	response[0] = '\0';

	int maxRetries = gConfig.getNum("TRX.MaxRetries");
	mControlLock.lock();
	LOG(INFO) << "command " << command;

	int n = mControlSocket.write(command);
	if (n <= 0)
		maxRetries = 0;
	for (int retry=0; retry<maxRetries && !exiting(); retry++) {
		msgLen = mControlSocket.read(response,1000);
		if (msgLen>0) {
			response[msgLen] = '\0';
			break;
		}
		LOG(NOTICE) << "TRX link timeout on attempt " << retry+1;
	}

	LOG(INFO) << "response " << response;
	mControlLock.unlock();

	if (exiting())
		return 0;

	if ((msgLen>4) && (strncmp(response,"RSP ",4)==0)) {
		return msgLen;
	}

	LOG(WARNING) << "lost control link to transceiver";
	return 0;
}

bool TransceiverManager::sendCommand(const char* cmd, int* iParam, const char* sParam,
	int* rspParam, int arfcn)
{
	if (!cmd)
		return false;
	// Send command and get response.
	char cmdBuf[MAX_UDP_LENGTH];
	char response[MAX_UDP_LENGTH];
	if (arfcn < 0) {
		if (iParam)
			sprintf(cmdBuf,"CMD %s %d", cmd, *iParam);
		else if (sParam)
			sprintf(cmdBuf,"CMD %s %s", cmd, sParam);
		else
			sprintf(cmdBuf,"CMD %s", cmd);
	}
	else {
		if (iParam)
			sprintf(cmdBuf,"CMD %d %s %d", arfcn, cmd, *iParam);
		else if (sParam)
			sprintf(cmdBuf,"CMD %d %s %s", arfcn, cmd, sParam);
		else
			sprintf(cmdBuf,"CMD %d %s", arfcn, cmd);
	}
	int rspLen = sendCommandPacket(cmdBuf,response);
	int status = -1;
	if (rspLen > 0) {
		char cmdNameTest[15];
		cmdNameTest[0]='\0';
		if (!rspParam)
			sscanf(response,"RSP %15s %d", cmdNameTest, &status);
		else
			sscanf(response,"RSP %15s %d %d", cmdNameTest, &status, rspParam);
		if (strcmp(cmdNameTest,cmd) != 0)
			status = -1;
	}
	if (status == 0)
		return true;
	if (exiting())
		return false;
	char* extra = 0;
	if (rspLen > 0) {
		// 'RSP CMD_NAME {FIRST_NUMBER} [SECOND_NUMBER] ...'
		int offset = strlen(cmd) + 5;
		if (offset < rspLen) {
			extra = ::strchr(response + offset,' ');
			if (extra && rspParam)
				extra = ::strchr(extra + 1,' ');
			if (extra && !*extra)
				extra = 0;
		}
	}
	if (extra) {
		LOG(ALERT) << cmd << " failed with status " << status << " extra:" << extra;
	}
	else {
		LOG(ALERT) << cmd << " failed with status " << status;
	}
	// Send notification for errors!
	// Handle some fatal errors during initialize/start
	// status > 0 means we've got error from transceiver!
	if (extra && !m_statistics && status > 0 && ((0 == strcmp(cmd,"RESET")) ||
		(0 == strcmp(cmd,"RXTUNE")) || (0 == strcmp(cmd,"TXTUNE")))) {
		std::string s(extra);
		s.append(" operation=");
		s.append(cmd);
		terminate(Connection::BtsRadioError,0,(const void*)s.c_str(),s.length(),0);
	}
	return false;
}


void* ClockLoopAdapter(TransceiverManager *transceiver)
{
	// This loop checks the clock messages from the transceiver.
	// These messages keep the BTS clock in sync with the hardware,
	// and also serve as a heartbeat for the radiomodem.

	// This is a convenient place for other periodic housekeeping as well.

	// This loop has a period of about 3 seconds.

	while (1) {
		transceiver->clockHandler();
	}
	return NULL;
}



void TransceiverManager::clockHandler()
{
	char buffer[MAX_UDP_LENGTH];
	int msgLen = mClockSocket.read(buffer,gConfig.getNum("TRX.Timeout.Clock")*1000);

	// Did the transceiver die??
	if (msgLen<0) {
		LOG(EMERG) << "TRX clock interface timed out, assuming TRX is dead.";
		gReports.incr("OpenBTS.Exit.Error.TransceiverHeartbeat");
		// (paul) Made this configurable at runtime
		if (gConfig.getBool("TRX.IgnoreDeath")) {
			// (pat) Added so you can keep debugging without the radio.
			static int foo = 0;
			pthread_exit(&foo);
		}
		else {
			mHaveClock = false;
			terminate(Connection::BtsRadioLost,-1);
		}
	}

	if (msgLen==0) {
		LOG(ALERT) << "read error on TRX clock interface, return " << msgLen;
		terminate(Connection::BtsInternalError);
		return;
	}

	if (strncmp(buffer,"IND CLOCK",9)==0) {
		uint32_t FN;
		sscanf(buffer,"IND CLOCK %u", &FN);
		LOG(INFO) << "CLOCK indication, current clock = " << gBTS.clock().get() << " new clock ="<<FN;
		gBTS.clock().set(FN);
		mHaveClock = true;
		return;
	}

	if (strncmp(buffer,"EXITING",7)==0) {
		LOG(NOTICE) << "TRX clock 'EXITING' indication";
		mExitRecv = true;
		mHaveClock = false;
		if (buffer[7] == 32)
			terminate(Connection::BtsRadioExiting,1,&buffer[8],msgLen - 8);
		else
			terminate(Connection::BtsRadioExiting);
	}

	buffer[msgLen]='\0';
	LOG(ALERT) << "bogus message " << buffer << " on clock interface";
}




unsigned TransceiverManager::C0() const
{
	return mARFCNs.at(0)->ARFCN();
}







::ARFCNManager::ARFCNManager(unsigned int wArfcnPos, const char* wTRXAddress, int wBasePort,
	const char* localIP, TransceiverManager &wTransceiver)
	:mTransceiver(wTransceiver),
	mDataSocket(wBasePort+1,wTRXAddress,wBasePort,localIP),
	mArfcnPos(wArfcnPos)
{
	// The default demux table is full of NULL pointers.
	for (int i=0; i<8; i++) {
		for (unsigned j=0; j<maxModulus; j++) {
			mDemuxTable[i][j] = NULL;
		}
	}
}




void ::ARFCNManager::start()
{
	mRxThread.start((void*(*)(void*))ReceiveLoopAdapter,this,"bts:arfcnmgr");
}


void ::ARFCNManager::installDecoder(GSM::L1Decoder *wL1d)
{
	unsigned TN = wL1d->TN();
	const TDMAMapping& mapping = wL1d->mapping();

	// Is this mapping a valid uplink on this slot?
	assert(mapping.uplink());
	assert(mapping.allowedSlot(TN));

	LOG(DEBUG) << "ARFCNManager::installDecoder TN: " << TN << " repeatLength: " << mapping.repeatLength();

	mTableLock.lock();
	for (unsigned i=0; i<mapping.numFrames(); i++) {
		unsigned FN = mapping.frameMapping(i);
		while (FN<maxModulus) {
			// Don't overwrite existing entries.
			assert(mDemuxTable[TN][FN]==NULL);
			mDemuxTable[TN][FN] = wL1d;
			FN += mapping.repeatLength();
		}
	}
	mTableLock.unlock();
}




// (pat) renamed overloaded function to clarify code
void ::ARFCNManager::writeHighSideTx(const GSM::TxBurst& burst,const char *culprit)
{
	LOG(DEBUG) << culprit << " transmit at time " << gBTS.clock().get() << ": " << burst 
		<<" steal="<<(int)burst.peekField(60,1)<<(int)burst.peekField(87,1);
	// format the transmission request message
	static const int bufferSize = gSlotLen+1+4+1;
	char buffer[bufferSize];
	unsigned char *wp = (unsigned char*)buffer;
	// slot
	*wp++ = burst.time().TN();
	// frame number
	uint32_t FN = burst.time().FN();
	*wp++ = (FN>>24) & 0x0ff;
	*wp++ = (FN>>16) & 0x0ff;
	*wp++ = (FN>>8) & 0x0ff;
	*wp++ = (FN) & 0x0ff;
	// power level
	/// FIXME -- We hard-code gain to 0 dB for now.
	*wp++ = 0;
	// copy data
	const char *dp = burst.begin();
	for (unsigned i=0; i<gSlotLen; i++) {
		*wp++ = (unsigned char)((*dp++) & 0x01);
	}
	// write to the socket
	mDataSocketLock.lock();
	mDataSocket.write(buffer,bufferSize);
	mDataSocketLock.unlock();
}




void ::ARFCNManager::driveRx()
{
	// read the message
	char buffer[MAX_UDP_LENGTH];
	int msgLen = mDataSocket.read(buffer);
	if (msgLen<=0) SOCKET_ERROR;
	// decode
	unsigned char *rp = (unsigned char*)buffer;
	// timeslot number
	unsigned TN = *rp++;
	// frame number
	int32_t FN = *rp++;
	FN = (FN<<8) + (*rp++);
	FN = (FN<<8) + (*rp++);
	FN = (FN<<8) + (*rp++);
	// physcial header data
	signed char* srp = (signed char*)rp++;
	// reported RSSI is negated dB wrt full scale
	int RSSI = *srp;
	srp = (signed char*)rp++;
	// timing error comes in 1/256 symbol steps
	// because that fits nicely in 2 bytes
	int timingError = *srp;
	timingError = (timingError<<8) | (*rp++);
	// soft symbols
	float data[gSlotLen];
	for (unsigned i=0; i<gSlotLen; i++) data[i] = (*rp++) / 256.0F;
	// demux
	receiveBurst(RxBurst(data,GSM::Time(FN,TN),timingError/256.0F,-RSSI));
}


void* ReceiveLoopAdapter(::ARFCNManager* manager){
	while (true) {
		manager->driveRx();
		pthread_testcancel();
	}
	return NULL;
}

// TODO : lots of duplicate code in these sendCommand()s
int ::ARFCNManager::sendCommand(const char*command, const char*param, int *responseParam)
{
	// Send command and get response.
	char cmdBuf[MAX_UDP_LENGTH];
	char response[MAX_UDP_LENGTH];
	sprintf(cmdBuf,"CMD %u %s %s", mArfcnPos, command, param);
	int rspLen = mTransceiver.sendCommandPacket(cmdBuf,response);
	if (rspLen<=0) return -1;
	// Parse and check status.
	char cmdNameTest[15];
	int status;
	cmdNameTest[0]='\0';
        if (!responseParam)
	  sscanf(response,"RSP %15s %d", cmdNameTest, &status);
        else
          sscanf(response,"RSP %15s %d %d", cmdNameTest, &status, responseParam);
	if (strcmp(cmdNameTest,command)!=0) return -1;
	return status;
}

int ::ARFCNManager::sendCommand(const char*command, int param, int *responseParam)
{
	// Send command and get response.
	char cmdBuf[MAX_UDP_LENGTH];
	char response[MAX_UDP_LENGTH];
	sprintf(cmdBuf,"CMD %u %s %d", mArfcnPos, command, param);
	int rspLen = mTransceiver.sendCommandPacket(cmdBuf,response);
	if (rspLen<=0) return -1;
	// Parse and check status.
	char cmdNameTest[15];
	int status;
	cmdNameTest[0]='\0';
        if (!responseParam)
	  sscanf(response,"RSP %15s %d", cmdNameTest, &status);
        else
          sscanf(response,"RSP %15s %d %d", cmdNameTest, &status, responseParam);
	if (strcmp(cmdNameTest,command)!=0) return -1;
	return status;
}


int ::ARFCNManager::sendCommand(const char*command, const char* param)
{
	// Send command and get response.
	char cmdBuf[MAX_UDP_LENGTH];
	char response[MAX_UDP_LENGTH];
	sprintf(cmdBuf,"CMD %u %s %s", mArfcnPos, command, param);
	int rspLen = mTransceiver.sendCommandPacket(cmdBuf,response);
	if (rspLen<=0) return -1;
	// Parse and check status.
	char cmdNameTest[15];
	int status;
	cmdNameTest[0]='\0';
	sscanf(response,"RSP %15s %d", cmdNameTest, &status);
	if (strcmp(cmdNameTest,command)!=0) return -1;
	return status;
}



int ::ARFCNManager::sendCommand(const char*command)
{
	// Send command and get response.
	char cmdBuf[MAX_UDP_LENGTH];
	char response[MAX_UDP_LENGTH];
	sprintf(cmdBuf,"CMD %u %s", mArfcnPos, command);
	int rspLen = mTransceiver.sendCommandPacket(cmdBuf,response);
	if (rspLen<=0) return -1;
	// Parse and check status.
	char cmdNameTest[15];
	int status;
	cmdNameTest[0]='\0';
	sscanf(response,"RSP %15s %d", cmdNameTest, &status);
	if (strcmp(cmdNameTest,command)!=0) return -1;
	return status;
}

bool ::ARFCNManager::tune(int wARFCN, bool c0)
{
	mARFCN=wARFCN;
	if (!c0)
		return true;
	// convert ARFCN number to a frequency
	int rxFreq = uplinkFreqKHz(gBTS.band(),wARFCN);
	int txFreq = downlinkFreqKHz(gBTS.band(),wARFCN);
	int rsp = 0;
	if (!mTransceiver.sendCommand("RXTUNE",&rxFreq,0,&rsp,mArfcnPos))
		return false;
	if (!mTransceiver.sendCommand("TXTUNE",&txFreq,0,&rsp,mArfcnPos))
		return false;
	return true;
}



bool ::ARFCNManager::tuneLoopback(int wARFCN)
{
	// convert ARFCN number to a frequency
	unsigned txFreq = downlinkFreqKHz(gBTS.band(),wARFCN);
	// tune rx
	int status = sendCommand("RXTUNE",txFreq);
	if (status!=0) {
		LOG(ALERT) << "RXTUNE failed with status " << status;
		return false;
	}
	// tune tx
	status = sendCommand("TXTUNE",txFreq);
	if (status!=0) {
		LOG(ALERT) << "TXTUNE failed with status " << status;
		return false;
	}
	// done
	mARFCN=wARFCN;
	return true;
}


bool ::ARFCNManager::powerOff()
{
	int status = sendCommand("POWEROFF");
	if (status!=0) {
		LOG(ALERT) << "POWEROFF failed with status " << status;
		return false;
	}
	return true;
}


bool ::ARFCNManager::powerOn(bool warn)
{
	int status = sendCommand("POWERON");
	if (status!=0) {
		if (warn) {
			LOG(ALERT) << "POWERON failed with status " << status;
		} else {
			LOG(INFO) << "POWERON failed with status " << status;
		}
		
		return false;
	}
	return true;
}





bool ::ARFCNManager::setPower(int dB)
{
	int status = sendCommand("SETPOWER",dB);
	if (status!=0) {
		if (!mTransceiver.exiting()) {
			LOG(ALERT) << "SETPOWER failed with status " << status;
		}
		return false;
	}
	return true;
}


bool ::ARFCNManager::setTSC(unsigned TSC) 
{
	assert(TSC<8);
	int status = sendCommand("SETTSC",TSC);
	if (status!=0) {
		if (!mTransceiver.exiting()) {
			LOG(ALERT) << "SETTSC failed with status " << status;
		}
		return false;
	}
	return true;
}


bool ::ARFCNManager::setBSIC(unsigned BSIC)
{
	assert(BSIC < 64);
	int status = sendCommand("SETBSIC",BSIC);
	if (status!=0) {
		if (!mTransceiver.exiting()) {
			LOG(ALERT) << "SETBSIC failed with status " << status;
		}
		return false;
	}
	return true;
}


bool ::ARFCNManager::setSlot(unsigned TN, unsigned combination)
{
	assert(TN<8);
	// (pat) had to remove assertion here:
	//assert(combination<8);
	char paramBuf[MAX_UDP_LENGTH];
	sprintf(paramBuf,"%d %d", TN, combination);
	int status = sendCommand("SETSLOT",paramBuf);
	if (status!=0) {
		if (!mTransceiver.exiting()) {
			LOG(ALERT) << "SETSLOT failed with status " << status;
		}
		return false;
	}
	return true;
}

bool ::ARFCNManager::setMaxDelay(unsigned km)
{
        int status = sendCommand("SETMAXDLY",km);
        if (status!=0) {
		if (!mTransceiver.exiting()) {
		    LOG(ALERT) << "SETMAXDLY failed with status " << status;
		}
		return false;
        }
        return true;
}

signed ::ARFCNManager::setRxGain(signed rxGain)
{
        signed newRxGain;
        int status = sendCommand("SETRXGAIN",rxGain,&newRxGain);
        if (status!=0) {
		if (!mTransceiver.exiting()) {
			LOG(ALERT) << "SETRXGAIN failed with status " << status;
		}
		return false;
        }
        return newRxGain;
}


bool ::ARFCNManager::setHandover(unsigned TN)
{
	assert(TN<8);
	int status = sendCommand("HANDOVER",TN);
	if (status!=0) {
		if (!mTransceiver.exiting()) {
			LOG(ALERT) << "HANDOVER failed with status " << status;
		}
		return false;
	}
	return true;
}


bool ::ARFCNManager::clearHandover(unsigned TN)
{
	assert(TN<8);
	int status = sendCommand("NOHANDOVER",TN);
	if (status!=0) {
		if (!mTransceiver.exiting()) {
			LOG(WARNING) << "NOHANDOVER failed with status " << status;
		}
		return false;
	}
	return true;
}


signed ::ARFCNManager::setTxAtten(signed txAtten)
{
        signed newTxAtten;
        int status = sendCommand("SETTXATTEN",txAtten,&newTxAtten);
        if (status!=0) {
		if (!mTransceiver.exiting()) {
		    LOG(ALERT) << "SETTXATTEN failed with status " << status;
		}
                return false;
        }
        return newTxAtten;
}

signed ::ARFCNManager::setFreqOffset(signed offset)
{
        signed newFreqOffset;
        int status = sendCommand("SETFREQOFFSET",offset,&newFreqOffset);
        if (status!=0) {
		if (!mTransceiver.exiting()) {
			LOG(ALERT) << "SETFREQOFFSET failed with status " << status;
		}
		return false;
        }
        return newFreqOffset;
}


signed ::ARFCNManager::getNoiseLevel(void)
{
	signed noiselevel;
	int status = sendCommand("NOISELEV",0,&noiselevel);
        if (status!=0) {
		if (!mTransceiver.exiting()) {
			LOG(ALERT) << "NOISELEV failed with status " << status;
		}
		return false;
        }
        return noiselevel;
}

bool ::ARFCNManager::runCustom(const char* command)
{
        int status = sendCommand("CUSTOM",command);
        if (status !=0) {
		if (!mTransceiver.exiting()) {
			LOG(WARNING) << "CUSTOM failed with status " << status;
		}
		return false;
        }
        return true;
}

signed ::ARFCNManager::getFactoryCalibration(const char * param)
{
	signed value;
	int status = sendCommand("READFACTORY", param, &value);
	if (status!=0) {
		if (!mTransceiver.exiting()) {
			LOG(ALERT) << "READFACTORY failed with status " << status;
		}
		return false;
	}
	return value;
}

void ::ARFCNManager::receiveBurst(const RxBurst& inBurst)
{
	LOG(DEBUG) << "processing " << inBurst;
	uint32_t FN = inBurst.time().FN() % maxModulus;
	unsigned TN = inBurst.time().TN();

	ScopedLock lock(mTableLock);
	L1Decoder *proc = mDemuxTable[TN][FN];
	if (proc==NULL) {
		LOG(DEBUG) << "ARFNManager::receiveBurst time " << inBurst.time() << " in unconfigured TDMA position T" << TN << " FN=" << FN << ".";
		return;
	}
	proc->writeLowSideRx(inBurst);
}


// vim: ts=4 sw=4
