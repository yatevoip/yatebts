/**@file Idle-mode dispatcher for dedicated control channels. */

/*
* Copyright 2008, 2009 Free Software Foundation, Inc.
* Copyright 2011 Range Networks, Inc.
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
#include "TransactionTable.h"
#include "RadioResource.h"
#include "MobilityManagement.h"
#include <GSMLogicalChannel.h>
#include <GSML3MMMessages.h>
#include <GSML3RRMessages.h>
#include <SIPUtility.h>
#include <SIPInterface.h>

#include <Logger.h>
#undef WARNING
#include <Reporting.h>
#include <Globals.h>

using namespace std;
using namespace GSM;
using namespace Control;
using namespace Connection;




/**
	Dispatch the appropriate controller for a Mobility Management message.
	@param req A pointer to the initial message.
	@param DCCH A pointer to the logical channel for the transaction.
*/
void DCCHDispatchMM(const L3MMMessage* req, LogicalChannel *DCCH)
{
	assert(req);
	L3MMMessage::MessageType MTI = (L3MMMessage::MessageType)req->MTI();
	switch (MTI) {
		case L3MMMessage::LocationUpdatingRequest:
			LocationUpdatingController(dynamic_cast<const L3LocationUpdatingRequest*>(req),DCCH);
			break;
		case L3MMMessage::IMSIDetachIndication:
			IMSIDetachController(dynamic_cast<const L3IMSIDetachIndication*>(req),DCCH);
			break;
		case L3MMMessage::CMServiceRequest:
			CMServiceResponder(dynamic_cast<const L3CMServiceRequest*>(req),DCCH);
			break;
		default:
			LOG(NOTICE) << "unhandled MM message " << MTI << " on " << *DCCH;
			throw UnsupportedMessage();
	}
}


/**
	Dispatch the appropriate controller for a Radio Resource message.
	@param req A pointer to the initial message.
	@param DCCH A pointer to the logical channel for the transaction.
*/
void DCCHDispatchRR(const L3RRMessage* req, LogicalChannel *DCCH)
{
	LOG(DEBUG) << "checking MTI"<< (L3RRMessage::MessageType)req->MTI();

	assert(req);
	L3RRMessage::MessageType MTI = (L3RRMessage::MessageType)req->MTI();
	switch (MTI) {
		case L3RRMessage::PagingResponse:
			PagingResponseHandler(dynamic_cast<const L3PagingResponse*>(req),DCCH);
			break;
		case L3RRMessage::AssignmentComplete:
			AssignmentCompleteHandler(
				dynamic_cast<const L3AssignmentComplete*>(req),
				dynamic_cast<TCHFACCHLogicalChannel*>(DCCH));
			break;
		default:
			LOG(NOTICE) << "unhandled RR message " << MTI << " on " << *DCCH;
			throw UnsupportedMessage();
	}
}


void DCCHDispatchMessage(const L3Message* msg, LogicalChannel* DCCH)
{
	// Each protocol has it's own sub-dispatcher.
	switch (msg->PD()) {
		case L3MobilityManagementPD:
			DCCHDispatchMM(dynamic_cast<const L3MMMessage*>(msg),DCCH);
			break;
		case L3RadioResourcePD:
			DCCHDispatchRR(dynamic_cast<const L3RRMessage*>(msg),DCCH);
			break;
		default:
			LOG(NOTICE) << "unhandled protocol " << msg->PD() << " on " << *DCCH;
			throw UnsupportedMessage();
	}
}



static void connDispatchLoop(LogicalChannel* chan, unsigned int id)
{
	TCHFACCHLogicalChannel* tch = dynamic_cast<TCHFACCHLogicalChannel*>(chan);
	LOG(INFO) << "starting dispatch loop for connection " << id << (tch ? " with traffic" : "");
	unsigned int maxQ = gConfig.getNum("GSM.MaxSpeechLatency");
	unsigned int tOut = tch ? 5 : 100; // TODO
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
		L3Frame* frame = chan->recv(tOut,0); // What about different SAPI?
		if (!frame)
			continue;
		switch (frame->primitive()) {
			case ERROR:
				LOG(NOTICE) << "error reading on connection " << id;
				break;
			case DATA:
			case UNIT_DATA:
				gSigConn.send(0,id,frame);
				delete frame;
				continue;
			case RELEASE:
			case HARDRELEASE:
				break;
			default:
				LOG(ERR) << "unexpected primitive " << frame->primitive();
				break;
		}
		// Frame was not handled - delete it and end connection
		delete frame;
		break;
	}
	if (gConnMap.find(id) == chan)
		gSigConn.send(SigConnection::SigConnLost,0,id);
	LOG(INFO) << "ending dispatch loop for connection " << id;
}

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
			L3RRMessage* req = GSM::parseL3RR(*frame);
			delete frame;
			LOG(INFO) << *chan << " received RR establishing message " << *req;
			DCCHDispatchRR(req,chan);
			delete req;
		}
		else {
			int id = gConnMap.map(chan);
			if (id >= 0) {
				if (gSigConn.send(0,id,frame))
					connDispatchLoop(chan,id);
				gConnMap.unmap(id);
			}
			delete frame;
		}
	}
	else
		LOG(NOTICE) << "L3 read timeout";

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
			gResetWatchdog();
			Primitive prim = frame->primitive();
			delete frame;
			LOG(DEBUG) << "received primtive " << prim;
			switch (prim) {
				case ESTABLISH: {
					// Pull the first message and dispatch a new transaction.
					gReports.incr("OpenBTS.GSM.RR.ChannelSiezed");
					if (connDispatchEstablish(DCCH))
						break;
					const L3Message *message = getMessage(DCCH);
					LOG(INFO) << *DCCH << " received establishing messaage " << *message;
					DCCHDispatchMessage(message,DCCH);
					delete message;
					break;
				}
				case HANDOVER_ACCESS: {
					ProcessHandoverAccess(dynamic_cast<GSM::TCHFACCHLogicalChannel*>(DCCH));
					break;
				}
				default: assert(0);
			}
		}

		// Catch the various error cases.

		catch (RemovedTransaction except) {
			LOG(ERR) << "attempt to use removed transaciton " << except.transactionID();
		}
		catch (ChannelReadTimeout except) {
			LOG(NOTICE) << "ChannelReadTimeout";
			// Cause 0x03 means "abnormal release, timer expired".
			DCCH->send(L3ChannelRelease(0x03));
			gTransactionTable.remove(except.transactionID());
		}
		catch (UnexpectedPrimitive except) {
			LOG(NOTICE) << "UnexpectedPrimitive";
			// Cause 0x62 means "message type not not compatible with protocol state".
			DCCH->send(L3ChannelRelease(0x62));
			if (except.transactionID()) gTransactionTable.remove(except.transactionID());
		}
		catch (UnexpectedMessage except) {
			LOG(NOTICE) << "UnexpectedMessage";
			// Cause 0x62 means "message type not not compatible with protocol state".
			DCCH->send(L3ChannelRelease(0x62));
			if (except.transactionID()) gTransactionTable.remove(except.transactionID());
		}
		catch (UnsupportedMessage except) {
			LOG(NOTICE) << "UnsupportedMessage";
			// Cause 0x61 means "message type not implemented".
			DCCH->send(L3ChannelRelease(0x61));
			if (except.transactionID()) gTransactionTable.remove(except.transactionID());
		}
		catch (Q931TimerExpired except) {
			LOG(NOTICE) << "Q.931 T3xx timer expired";
			// Cause 0x03 means "abnormal release, timer expired".
			// TODO -- Send diagnostics.
			DCCH->send(L3ChannelRelease(0x03));
			if (except.transactionID()) gTransactionTable.remove(except.transactionID());
		}
		catch (SIP::SIPTimeout except) {
			// FIXME -- The transaction ID should be an argument here.
			LOG(WARNING) << "Uncaught SIPTimeout, will leave a stray transcation";
			// Cause 0x03 means "abnormal release, timer expired".
			DCCH->send(L3ChannelRelease(0x03));
			if (except.transactionID()) gTransactionTable.remove(except.transactionID());
		}
		catch (SIP::SIPError except) {
			// FIXME -- The transaction ID should be an argument here.
			LOG(WARNING) << "Uncaught SIPError, will leave a stray transcation";
			// Cause 0x01 means "abnormal release, unspecified".
			DCCH->send(L3ChannelRelease(0x01));
			if (except.transactionID()) gTransactionTable.remove(except.transactionID());
		}
	}
}




// vim: ts=4 sw=4
