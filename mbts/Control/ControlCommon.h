/**@file Declarations for common-use control-layer functions. */
/*
* Copyright 2008, 2009, 2010 Free Software Foundation, Inc.
* Copyright 2010 Kestrel Signal Processing, Inc.
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



#ifndef CONTROLCOMMON_H
#define CONTROLCOMMON_H


#include <stdio.h>
#include <list>

#include <Logger.h>
#include <Interthread.h>
#include <Timeval.h>


#include <GSML3CommonElements.h>
#include <GSML3RRMessages.h>

//#include "TMSITable.h"


// Enough forward refs to prevent "kitchen sick" includes and circularity.

namespace GSM {
class L3Message;
class LogicalChannel;
class SDCCHLogicalChannel;
class SACCHLogicalChannel;
class TCHFACCHLogicalChannel;
class L3CMServiceRequest;
};


/**@namespace Control This namepace is for use by the control layer. */
namespace Control {


/**@name Common-use functions from the control layer. */
//@{

/**
	Get a message from a LogicalChannel.
	Close the channel with abnormal release on timeout.
	Caller must delete the returned pointer.
	Throws ChannelReadTimeout, UnexpecedPrimitive or UnsupportedMessage on timeout.
	@param LCH The channel to receive on.
	@param SAPI The service access point.
	@return Pointer to message.
*/
// FIXME -- This needs an adjustable timeout.
GSM::L3Message* getMessage(GSM::LogicalChannel* LCH, unsigned SAPI=0);


//@}


/**@name Dispatch controllers for specific channel types. */
//@{
//void FACCHDispatcher(GSM::TCHFACCHLogicalChannel *TCHFACCH);
//void SDCCHDispatcher(GSM::SDCCHLogicalChannel *SDCCH);
void DCCHDispatcher(GSM::LogicalChannel *DCCH);
//@}





/**
	SMSCB sender function
*/
void *SMSCBSender(void*);





/**@name Control-layer exceptions. */
//@{

/**
	A control layer excpection includes a pointer to a transaction.
	The transaction might require some clean-up action, depending on the exception.
*/
class ControlLayerException {

	private:

	unsigned mConnectionID;

	public:

	ControlLayerException(unsigned wConnectionID=0)
		:mConnectionID(wConnectionID)
	{}

	unsigned connectionID() { return mConnectionID; }
};

/** Thrown when the control layer gets the wrong message */
class UnexpectedMessage : public ControlLayerException {
	public:
	UnexpectedMessage(unsigned wConnectionID=0)
		:ControlLayerException(wConnectionID)
	{}
};

/** Thrown when recvL3 returns NULL */
class ChannelReadTimeout : public ControlLayerException {
	public:
	ChannelReadTimeout(unsigned wConnectionID=0)
		:ControlLayerException(wConnectionID)
	{}
};

/** Thrown when L3 can't parse an incoming message */
class UnsupportedMessage : public ControlLayerException {
	public:
	UnsupportedMessage(unsigned wConnectionID=0)
		:ControlLayerException(wConnectionID)
	{}
};

/** Thrown when the control layer gets the wrong primitive */
class UnexpectedPrimitive : public ControlLayerException {
	public:
	UnexpectedPrimitive(unsigned wConnectionID=0)
		:ControlLayerException(wConnectionID)
	{}
};



//@}


}	//Control



/**@addtogroup Globals */
//@{
/** A single global transaction table in the global namespace. */
//extern Control::TransactionTable gTransactionTable;
//@}



#endif

// vim: ts=4 sw=4
