/**@file Idle-mode dispatcher for dedicated control channels. */

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




#include "ControlCommon.h"
#include "RadioResource.h"
//#include "MobilityManagement.h"
#include <GSMLogicalChannel.h>
//#include <GSML3MMMessages.h>
#include <GSML3RRMessages.h>

#include <Sgsn.h>

#include <Logger.h>
#undef WARNING
#include <Reporting.h>
#include <Globals.h>
#include <Timeval.h>

#include <stdio.h>

using namespace std;
using namespace GSM;
using namespace SGSN;
using namespace Control;
using namespace Connection;

#define PHY_INTERVAL 200

// Send physical channel information
static void sendPhyInfo(LogicalChannel* chan, unsigned int id)
{
	char buf[128];
	int len = snprintf(buf, sizeof(buf), "TA=%d TE=%0.3f UpRSSI=%0.0f TxPwr=%d DnRSSIdBm=%d time=%9.3lf",
		chan->actualMSTiming(), chan->timingError(),
		chan->RSSI(), chan->actualMSPower(),
		chan->measurementResults().RXLEV_FULL_SERVING_CELL_dBm(),
		chan->timestamp());
	if (len > 0)
		gSigConn.send(Connection::SigPhysicalInfo,0,id,buf,len);
}

// Attempt to dispatch locally a single RR message, return true if handled
static bool connDispatchRR(LogicalChannel* chan, unsigned int id, L3Frame* frame)
{
	if (frame->PD() != GSM::L3RadioResourcePD)
		return false;
	switch (frame->MTI()) {
		case L3RRMessage::ChannelModeModifyAcknowledge:
			// TODO: check reported channel mode
		case L3RRMessage::AssignmentComplete:
			gSigConn.send(SigMediaStarted,0,id);
			return true;
		case L3RRMessage::AssignmentFailure:
			gSigConn.send(SigMediaError,ErrInterworking,id);
			return true;
		case L3RRMessage::GPRSSuspensionRequest:
			{
				L3GPRSSuspensionRequest* msg = static_cast<L3GPRSSuspensionRequest*>(parseL3RR(*frame));
				if (msg) {
					if (!Sgsn::handleGprsSuspensionRequest(msg->mTLLI,msg->mRaId)) {
						LOG(NOTICE) << " not handled GPRS suspension request on connection " << id;
					}
					else {
						LOG(INFO) << " handled GPRS suspension request on connection " << id
							<< " for tlli  " << LOGVALHEX(msg->mTLLI);
					}
					delete msg;
				}
			}
			return true;
	}
	return false;
}

// Dispatching loop, runs for the lifetime of the connection
static void connDispatchLoop(LogicalChannel* chan, unsigned int id)
{
	TCHFACCHLogicalChannel* tch = dynamic_cast<TCHFACCHLogicalChannel*>(chan);
	LOG(INFO) << "starting dispatch loop for connection " << id << (tch ? " with traffic" : "");
	if (chan->SACCH())
		chan->SACCH()->measurementHoldOff();
	Timeval tPhy(PHY_INTERVAL);
	unsigned int maxQ = gConfig.getNum("GSM.MaxSpeechLatency");
	unsigned int tOut = tch ? 5 : 20; // TODO
	while (gSigConn.valid() && (gConnMap.find(id) == chan)) {
		if (tch) {
			while (tch->queueSize() > maxQ)
				delete[] tch->recvTCH();
			unsigned char* mFrame = tch->recvTCH();
			if (mFrame) {
				gMediaConn.send(id,mFrame,33);
				delete mFrame;
			}
		}
		unsigned char sapi;
		L3Frame* frame = 0;
		for (sapi = 1; sapi < 4; sapi++) {
			if (chan->debugGetL2(sapi))
				frame = chan->recv(0,sapi);
			if (frame)
				break;
		}
		if (!frame) {
			sapi = 0;
			frame = chan->recv(tOut,0);
		}
		if (!frame)
			continue;
		switch (frame->primitive()) {
			case ERROR:
				LOG(NOTICE) << "error reading on connection " << id;
				break;
			case DATA:
			case UNIT_DATA:
				if (tPhy.passed()) {
					tPhy.future(PHY_INTERVAL);
					sendPhyInfo(chan,id);
				}
				if (!connDispatchRR(chan,id,frame))
					gSigConn.send(sapi,id,frame);
			case HANDOVER_ACCESS:
				delete frame;
				continue;
			case RELEASE:
			case HARDRELEASE:
				break;
			case ESTABLISH:
				if (sapi) {
					delete frame;
					gSigConn.send(SigEstablishSAPI,sapi,id);
					continue;
				}
				chan->send(GSM::RELEASE);
				// fall through
			default:
				LOG(ERR) << "unexpected primitive " << frame->primitive();
				break;
		}
		// Frame was not handled - delete it and end connection
		delete frame;
		break;
	}
	const LogicalChannel* ch = gConnMap.find(id);
	if (ch == chan)
		gSigConn.send(Connection::SigConnLost,0,id);
	else if (!ch)
		chan->send(GSM::RELEASE);
	LOG(INFO) << "ending dispatch loop for connection " << id <<
		(ch ? ((ch == chan) ? " (remote release)" : " (reassigned)") : " (local close)");
}

// Dispatch new channel establishing frame
static void connDispatchChannel(LogicalChannel* chan, L3Frame* frame)
{
	int id = gConnMap.map(chan,chan->SACCH());
	if (id < 0) {
		LOG(ERR) << "failed to map channel " << *chan;
		return;
	}
	if (id >= 0) {
		sendPhyInfo(chan,id);
		if (connDispatchRR(chan,id,frame) || gSigConn.send(0,id,frame))
			connDispatchLoop(chan,id);
		gConnMap.unmap(chan);
	}
}

// Dispatch reassigned channel establishing frame
static void connDispatchReassigned(LogicalChannel* chan, L3Frame* frame)
{
	TCHFACCHLogicalChannel* tch = dynamic_cast<TCHFACCHLogicalChannel*>(chan);
	if (!tch) {
		LOG(ERR) << "reassigned to non-traffic channel " << *chan;
		return;
	}
	int id = gConnMap.remap(chan,tch,chan->SACCH());
	if (id < 0) {
		LOG(ERR) << "failed to remap channel " << *chan;
		return;
	}
	LOG(INFO) << "channel " << *chan << " reallocated to connection " << id;
	if (id >= 0) {
		sendPhyInfo(chan,id);
		if (connDispatchRR(chan,id,frame) || gSigConn.send(0,id,frame))
			connDispatchLoop(chan,id);
		gConnMap.unmap(chan);
	}
}

// Dispatch channel establishment
static bool connDispatchEstablish(LogicalChannel* chan)
{
	if (!gSigConn.valid())
		return false;
	unsigned timeout_ms = chan->N200() * T200ms;
	L3Frame* frame = chan->recv(timeout_ms,0); // What about different SAPI?
	if (frame) {
		LOG(DEBUG) << "received " << *frame;
		Primitive primitive = frame->primitive();
		if (primitive != DATA) {
			LOG(ERR) << "unexpected primitive " << primitive;
			delete frame;
			throw UnexpectedPrimitive();
		}
		if (frame->PD() == GSM::L3RadioResourcePD) {
			switch (frame->MTI()) {
				case L3RRMessage::AssignmentComplete:
					connDispatchReassigned(chan,frame);
					break;
				case L3RRMessage::PagingResponse:
				case L3RRMessage::HandoverComplete:
					connDispatchChannel(chan,frame);
					break;
				default:
					LOG(INFO) << *chan <<
						" unexpected RR establishing frame " << *frame;
			}
		}
		else
			connDispatchChannel(chan,frame);
		delete frame;
	}
	else
		LOG(NOTICE) << "L3 read timeout";
	gConnMap.unmap(chan);
	return true;
}



/** Example of a closed-loop, persistent-thread control function for the DCCH. */
// (pat) DCCH is a TCHFACCHLogicalChannel or SDCCHLogicalChannel
void Control::DCCHDispatcher(LogicalChannel *DCCH)
{
	while (1) {
		try {
			// Wait for a transaction to start.
			LOG(DEBUG) << "waiting for " << *DCCH << " ESTABLISH or HANDOVER_ACCESS";
			L3Frame *frame = DCCH->waitForEstablishOrHandover();
			LOG(DEBUG) << *DCCH << " received " << *frame;
			Primitive prim = frame->primitive();
			delete frame;
			LOG(DEBUG) << "received primtive " << prim;
			switch (prim) {
				case ESTABLISH: {
					// Pull the first message and dispatch a new transaction.
					gReports.incr("OpenBTS.GSM.RR.ChannelSiezed");
					connDispatchEstablish(DCCH);
					break;
					// These should no longer be called.
					//const L3Message *message = getMessage(DCCH);
					//LOG(INFO) << *DCCH << " received establishing messaage " << *message;
					//DCCHDispatchMessage(message,DCCH);
					//delete message;
					//break;
				}
				case HANDOVER_ACCESS: {
					if (ProcessHandoverAccess(DCCH))
						connDispatchEstablish(DCCH);
					break;
				}
				// We should never come here.
				default: assert(0);
			}
		}

		// Catch the various error cases.

		catch (ChannelReadTimeout except) {
			LOG(NOTICE) << "ChannelReadTimeout";
			// Cause 0x03 means "abnormal release, timer expired".
			DCCH->send(L3ChannelRelease(0x03));
		}
		catch (UnexpectedPrimitive except) {
			LOG(NOTICE) << "UnexpectedPrimitive";
			// Cause 0x62 means "message type not not compatible with protocol state".
			DCCH->send(L3ChannelRelease(0x62));
		}
		catch (UnexpectedMessage except) {
			LOG(NOTICE) << "UnexpectedMessage";
			// Cause 0x62 means "message type not not compatible with protocol state".
			DCCH->send(L3ChannelRelease(0x62));
		}
		catch (UnsupportedMessage except) {
			LOG(NOTICE) << "UnsupportedMessage";
			// Cause 0x61 means "message type not implemented".
			DCCH->send(L3ChannelRelease(0x61));
		}
	}
}




// vim: ts=4 sw=4
