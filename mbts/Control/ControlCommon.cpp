/**@file Common-use functions for the control layer. */

/*
* Copyright 2008, 2010 Free Software Foundation, Inc.
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
//#include "TransactionTable.h"

#include <GSMLogicalChannel.h>
#include <GSML3Message.h>
#include <GSML3RRMessages.h>
#include <GSMConfig.h>

#include <Sgsn.h>

#include <Logger.h>
#include <Reporting.h>
#undef WARNING


using namespace std;
using namespace GSM;
using namespace Control;





// FIXME -- getMessage should return an L3Frame, not an L3Message.
// This will mean moving all of the parsing into the control layer.
// FIXME -- This needs an adjustable timeout.

L3Message* getMessageCore(LogicalChannel *LCH, unsigned SAPI)
{
	unsigned timeout_ms = LCH->N200() * T200ms;
	L3Frame *rcv = LCH->recv(timeout_ms,SAPI);
	if (rcv==NULL) {
		LOG(NOTICE) << "L3 read timeout";
		throw ChannelReadTimeout();
	}
	LOG(DEBUG) << "received " << *rcv;
	Primitive primitive = rcv->primitive();
	if (primitive!=DATA) {
		LOG(ERR) << "unexpected primitive " << primitive;
		delete rcv;
		throw UnexpectedPrimitive();
	}
	L3Message *msg = parseL3(*rcv);
	if (msg==NULL) {
		LOG(WARNING) << "unparsed message:" << *rcv;
		delete rcv;
		return NULL;
		//throw UnsupportedMessage();
	}
	delete rcv;
	LOG(DEBUG) << *msg;
	return msg;
}


// FIXME -- getMessage should return an L3Frame, not an L3Message.
// This will mean moving all of the parsing into the control layer.
// FIXME -- This needs an adjustable timeout.

L3Message* Control::getMessage(LogicalChannel *LCH, unsigned SAPI)
{
	L3Message *msg = getMessageCore(LCH,SAPI);
	// Handsets should not be sending us GPRS suspension requests when GPRS support is not enabled.
	// But if they do, we should ignore them.
	// They should not send more than one in any case, but we need to be
	// ready for whatever crazy behavior they throw at us.

	// The suspend procedure includes MS<->BSS and BSS<->SGSN messages.
	// GSM44.018 3.4.25 GPRS Suspension Procedure and 9.1.13b: GPRS Suspension Request message.
	// Also 23.060 16.2.1.1 Suspend/Resume procedure general.
	// GSM08.18: Suspend Procedure talks about communication between the BSS and SGSN,
	// and is not applicable to us when using the internal SGSN.
	// Note: When call is finished the RR is supposed to include a GPRS resumption IE, but if it does not,
	// 23.060 16.2.1.1.1 says the MS will do a GPRS RoutingAreaUpdate to get the
	// GPRS service back, so we are not worrying about it.
	// (pat 3-2012) Send the message to the internal SGSN.
	// It returns true if GPRS and the internal SGSN are enabled.
	// If we are using an external SGSN, we could send the GPRS suspend request to the SGSN via the BSSG,
	// but that has no hope of doing anything useful. See ticket #613.
	// First, We are supposed to automatically detect when we should do the Resume procedure.
	// Second: An RA-UPDATE, which gets send to the SGSN, does something to the CC state
	// that I dont understand yet.
	// We dont do any of the above.
	unsigned count = gConfig.getNum("GSM.Control.GPRSMaxIgnore");
	const GSM::L3GPRSSuspensionRequest *srmsg;
	while (count && (srmsg = dynamic_cast<const GSM::L3GPRSSuspensionRequest*>(msg))) {
		if (! SGSN::Sgsn::handleGprsSuspensionRequest(srmsg->mTLLI,srmsg->mRaId)) {
			LOG(NOTICE) << "ignoring GPRS suspension request";
		}
		msg = getMessageCore(LCH,SAPI);
		count--;
	}
	return msg;
}








// vim: ts=4 sw=4
