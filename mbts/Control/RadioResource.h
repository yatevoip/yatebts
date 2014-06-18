/**@file GSM Radio Resource procedures, GSM 04.18 and GSM 04.08. */
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

#ifndef RADIORESOURCE_H
#define RADIORESOURCE_H

#include <list>
#include <GSML3CommonElements.h>
#include <Interthread.h>


namespace GSM {
class Time;
class TCHFACCHLogicalChannel;
class SACCHLogicalChannel;
class L3HandoverComplete;
class L3HandoverAccess;
};

namespace Control {

//class TransactionEntry;




/**
	Determine whether or not to initiate a handover.
	@param measurements The measurement results from the SACCH.
	@param SACCH The SACCH in question.
*/
void HandoverDetermination(const GSM::L3MeasurementResults &measurements, GSM::SACCHLogicalChannel* SACCH);

/** Save handover parameters from L1 in the proper transaction record. */
bool SaveHandoverAccess(unsigned handoverReference, GSM::L1FEC* l1, float RSSI, float timingError, const GSM::Time& timestamp);

/** Process the handover access; returns true to continue. */
bool ProcessHandoverAccess(GSM::LogicalChannel *chan);

/**@ Access Grant mechanisms */
//@{


/** Decode RACH bits and send an immediate assignment; may block waiting for a channel. */
//void AccessGrantResponder(
//	unsigned requestReference, const GSM::Time& when,
//	float RSSI, float timingError);


/** This record carries all of the parameters associated with a RACH burst. */
class ChannelRequestRecord {

	private:

	unsigned mRA;		///< request reference
	GSM::Time mFrame;	///< receive timestamp
	float mRSSI;		///< dB wrt full scale
	float mTimingError;	///< correlator timing error in symbol periods

	public:

	ChannelRequestRecord(
		unsigned wRA, const GSM::Time& wFrame,
		float wRSSI, float wTimingError)
		:mRA(wRA), mFrame(wFrame),
		mRSSI(wRSSI), mTimingError(wTimingError)
	{ }

	unsigned RA() const { return mRA; }
	const GSM::Time& frame() const { return mFrame; }
	float RSSI() const { return mRSSI; }
	float timingError() const { return mTimingError; }

};


/** A thread to process contents of the channel request queue. */
void* AccessGrantServiceLoop(void*);


//@}

/**@ Paging mechanisms */
//@{

/** An entry in the paging list. */
class PagingEntry {

	private:

	GSM::L3MobileIdentity mID;		///< The mobile ID.
	GSM::ChannelType mType;			///< The needed channel type.
	unsigned mTransactionID;		///< The associated transaction ID.
	Timeval mExpiration;			///< The expiration time for this entry.

	public:

	/**
		Create a new entry, with current timestamp.
		@param wID The ID to be paged.
		@param wLife The number of milliseconds to keep paging.
	*/
	PagingEntry(const GSM::L3MobileIdentity& wID, GSM::ChannelType wType,
			unsigned wTransactionID, unsigned wLife)
		:mID(wID),mType(wType),mTransactionID(wTransactionID),mExpiration(wLife)
	{}

	/** Access the ID. */
	const GSM::L3MobileIdentity& ID() const { return mID; }

	/** Access the channel type needed. */
	GSM::ChannelType type() const { return mType; }

	unsigned transactionID() const { return mTransactionID; }

	/** Renew the timer. */
	void renew(unsigned wLife) { mExpiration = Timeval(wLife); }

	/** Returns true if the entry is expired. */
	bool expired() const { return mExpiration.passed(); }

};

typedef std::list<PagingEntry> PagingEntryList;


/**
	The pager is a global object that generates paging messages on the CCCH.
	To page a mobile, add the mobile ID to the pager.
	The entry will be deleted automatically when it expires.
	All pager operations are linear time.
	Not much point in optimizing since the main operation is inherently linear.
*/
class Pager {

	private:

	PagingEntryList mPageIDs;				///< List of ID's to be paged.
	mutable Mutex mLock;					///< Lock for thread-safe access.
	Signal mPageSignal;						///< signal to wake the paging loop
	Thread mPagingThread;					///< Thread for the paging loop.
	volatile bool mRunning;

	public:

	Pager()
		:mRunning(false)
	{}

	/** Set the output FIFO and start the paging loop. */
	void start();

	/**
		Add a mobile ID to the paging list.
		@param addID The mobile ID to be paged.
		@param chanType The channel type to be requested.
		@param transaction The transaction record, which will be modified.
		@param wLife The paging duration in ms, default is SIP Timer B.
	*/
	void addID(
		const GSM::L3MobileIdentity& addID,
		GSM::ChannelType chanType,
		unsigned wLife=gConfig.getNum("GSM.Timer.T3113")
	);

	/**
		Remove a mobile ID.
		This is used to stop the paging when a phone responds.
		@return The transaction ID associated with this entry.
	*/
	unsigned removeID(const GSM::L3MobileIdentity&);

	private:

	/**
		Traverse the paging list, paging all IDs.
		@return Number of IDs paged.
	*/
	unsigned pageAll();

	/** A loop that repeatedly calls pageAll. */
	void serviceLoop();

	/** C-style adapter. */
	friend void *PagerServiceLoopAdapter(Pager*);

public:

	/** return size of PagingEntryList */
	size_t pagingEntryListSize();

	/** Dump the paging list to an ostream. */
	void dump(std::ostream&) const;
};


void *PagerServiceLoopAdapter(Pager*);



//@}	// paging mech

}


#endif
