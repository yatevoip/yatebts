/**@file GSM/SIP Call Control -- GSM 04.08, ISDN ITU-T Q.931, SIP IETF RFC-3261, RTP IETF RFC-3550. */
/*
* Copyright 2008, 2009, 2010 Free Software Foundation, Inc.
* Copyright 2010 Kestrel Signal Processing, Inc.
*
* This software is distributed under multiple licenses; see the COPYING file in the main directory for licensing information for this specific distribuion.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

*/

#ifndef CALLCONTROL_H
#define CALLCONTROL_H


namespace GSM {
class LogicalChannel;
class TCHFACCHLogicalChannel;
class L3CMServiceRequest;
};

namespace Control {

class TransactionEntry;



/**@name MOC */
//@{
/** Run the MOC to the point of alerting, doing early assignment if needed. */
void MOCStarter(const GSM::L3CMServiceRequest*, GSM::LogicalChannel*);
/** Complete the MOC connection. */
void MOCController(TransactionEntry*, GSM::TCHFACCHLogicalChannel*);
//@}


/**@name MTC */
//@{
/** Run the MTC to the point of alerting, doing early assignment if needed. */
void MTCStarter(TransactionEntry*, GSM::LogicalChannel*);
/** Complete the MTC connection. */
void MTCController(TransactionEntry*, GSM::TCHFACCHLogicalChannel*);
//@}


/**@name Test Call */
//@{
/** Run the test call. */
void TestCall(TransactionEntry*, GSM::LogicalChannel*);
//@}

/** Create a new transaction entry and start paging. */
void initiateMTTransaction(TransactionEntry* transaction,
		GSM::ChannelType chanType, unsigned pageTime);


/**
	This is the standard call manangement loop, regardless of the origination type.
	This function returns when the call is cleared and the channel is released.
	@param transaction The transaction record for this call, will be cleared on exit.
	@param TCH The TCH+FACCH for the call.
*/
void callManagementLoop(TransactionEntry *transaction, GSM::TCHFACCHLogicalChannel* TCH);

}


#endif
