/**@file
	@brief Common elements for L3 messages, GSM 04.08 10.5.1.
*/
/*
* Copyright 2008-2010 Free Software Foundation, Inc.
* Copyright 2010 Kestrel Signal Processing, Inc.
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




#ifndef GSMCOMMONELEMENTS_H
#define GSMCOMMONELEMENTS_H

#include "GSML3Message.h"
#include <Globals.h>


namespace GSM {



/** Cell Identity, GSM 04.08 10.5.1.1 */
class L3CellIdentity : public L3ProtocolElement {

	private:

	unsigned mID;

	public:

	L3CellIdentity(unsigned wID=gConfig.getNum("GSM.Identity.CI"))
		:mID(wID)
	{ }

	size_t lengthV() const { return 2; }
	void writeV(L3Frame& dest, size_t &wp) const;
	void parseV(const L3Frame&, size_t&) { abort(); }
	void parseV(const L3Frame&, size_t&, size_t) { abort(); }
	void text(std::ostream&) const;
};





/** LAI, GSM 04.08 10.5.1.3 */
class L3LocationAreaIdentity : public L3ProtocolElement {

	private:

	unsigned mMCC[3];		///< mobile country code digits
	unsigned mMNC[3];		///< mobile network code digits
	unsigned mLAC;			///< location area code


	public:

	/**
		Initialize the LAI with real values, drawn from gConfig by default.
		@param wMCC MCC as a string (3 digits).
		@param wMNC MNC as a string (2 or 3 digits).
		@param wLAC LAC as a number.
	*/
	L3LocationAreaIdentity(
		const char*wMCC = gConfig.getStr("GSM.Identity.MCC").c_str(),
		const char* wMNC = gConfig.getStr("GSM.Identity.MNC").c_str(),
		unsigned wLAC = gConfig.getNum("GSM.Identity.LAC")
	);

	/** Sometimes we need to compare these things. */
	bool operator==(const L3LocationAreaIdentity&) const;

	size_t lengthV() const { return 5; }
	void parseV(const L3Frame& source, size_t &rp);
	void parseV(const L3Frame&, size_t&, size_t) { abort(); }
	void writeV(L3Frame& dest, size_t &wp) const;
	void text(std::ostream&) const;

	int MCC() const { return mMCC[0]*100 + mMCC[1]*10 + mMCC[2]; }
	int MNC() const;
	int LAC() const { return mLAC; }
};






/** Mobile Identity, GSM 04.08, 10.5.1.4 */
class L3MobileIdentity : public L3ProtocolElement {

	private:

	
	MobileIDType mType;					///< IMSI, TMSI, or IMEI?
	char mDigits[16];					///< GSM 03.03 2.2 limits the IMSI or IMEI to 15 digits.
	uint32_t mTMSI;						///< GSM 03.03 2.4 specifies the TMSI as 32 bits

	public:

	/** Empty ID */
	L3MobileIdentity()
		:L3ProtocolElement(),
		mType(NoIDType),
		mTMSI(0xffffffffu)
	{ mDigits[0]='\0'; } 

	/** TMSI initializer. */
	L3MobileIdentity(unsigned int wTMSI, const char* wDigits = 0)
		:L3ProtocolElement(),
		mType(TMSIType), mTMSI(wTMSI)
	{
	    if (!wDigits)
		mDigits[0]='\0'; 
	    else {
		assert(strlen(wDigits)<=15);
		strcpy(mDigits,wDigits);
	    }
	}

	/** IMSI initializer. */
	L3MobileIdentity(const char* wDigits)
		:L3ProtocolElement(),
		mType(IMSIType), mTMSI(0xffffffffu)
	{ assert(strlen(wDigits)<=15); strcpy(mDigits,wDigits); }

	/**@name Accessors. */
	//@{
	MobileIDType type() const { return mType; }
	const char* digits() const 
	{
		if (mDigits[0] == '\0') return 0;
		return mDigits; 
	}
	unsigned int TMSI() const { return mTMSI; }
	//@}

	/** Comparison. */
	bool operator==(const L3MobileIdentity&) const;

	/** Comparison. */
	bool operator!=(const L3MobileIdentity& other) const { return !operator==(other); }

	/** Comparison. */
	bool operator<(const L3MobileIdentity&) const;

	size_t lengthV() const;
	void writeV(L3Frame& dest, size_t &wp) const;
	void parseV( const L3Frame& src, size_t &rp, size_t expectedLength );
	void parseV(const L3Frame&, size_t&) { abort(); }
	void text(std::ostream&) const;
};



/**
	Mobile Station Classmark 1, GSM 04.08 10.5.1.5
*/
class L3MobileStationClassmark1 : public L3ProtocolElement {

	protected:

	unsigned mRevisionLevel;
	unsigned mES_IND;
	unsigned mA5_1;
	unsigned mRFPowerCapability;

	public:

	size_t lengthV() const { return 1; }	
	void writeV(L3Frame&, size_t&) const { assert(0); }
	void parseV(const L3Frame &src, size_t &rp);
	void parseV(const L3Frame&, size_t&, size_t) { assert(0); }
	void text(std::ostream&) const;

};

/**
	Mobile Station Classmark 2, GSM 04.08 10.5.1.5
*/
class L3MobileStationClassmark2 : public L3ProtocolElement {

	protected:

	unsigned mRevisionLevel;
	unsigned mES_IND;
	unsigned mA5_1;
	unsigned mA5_3;
	unsigned mA5_2;
	unsigned mRFPowerCapability;
	unsigned mPSCapability;
	unsigned mSSScreenIndicator;
	unsigned mSMCapability;
	unsigned mVBS;
	unsigned mVGCS;
	unsigned mFC;
	unsigned mCM3;
	unsigned mLCSVACapability;
	unsigned mSoLSA;
	unsigned mCMSF;

	public:

	size_t lengthV() const { return 3; }	
	void writeV(L3Frame&, size_t&) const { assert(0); }
	void parseV(const L3Frame &src, size_t &rp);
	void parseV(const L3Frame&, size_t&, size_t);
	void text(std::ostream&) const;

	// These return true if the encryption type is supported.
	bool A5_1() const { return mA5_1==0; }
	bool A5_2() const { return mA5_2!=0; }
	bool A5_3() const { return mA5_3!=0; }

	// Returns the power class, based on power capability encoding.
	int powerClass() const { return mRFPowerCapability+1; }
};


/**
	Mobile Station Classmark 3, GSM 04.08 10.5.1.7
	FIXME -- We are only parsing the A5/x bits.
*/
class L3MobileStationClassmark3 : public L3ProtocolElement {

	protected:

	unsigned mMultiband;
	unsigned mA5_4;
	unsigned mA5_5;
	unsigned mA5_6;
	unsigned mA5_7;

	public:

	L3MobileStationClassmark3()
		:mA5_4(0),mA5_5(0),mA5_6(0),mA5_7(0)
	{ }

	size_t lengthV() const { return 14; }	// HACK -- Return maximum length.
	void writeV(L3Frame&, size_t&) const { assert(0); }
	void parseV(const L3Frame &, size_t &);
	void parseV(const L3Frame&, size_t&, size_t);
	void text(std::ostream&) const;

};



class L3CipheringKeySequenceNumber : public L3ProtocolElement {

	protected:

	unsigned mCIValue;

	public:

	L3CipheringKeySequenceNumber(unsigned wCIValue):
		mCIValue(wCIValue)
	{ }

	size_t lengthV() const { return 0; }
	void writeV(L3Frame&, size_t&) const;
	void parseV(const L3Frame &, size_t &) { assert(0); }
	void parseV(const L3Frame&, size_t&, size_t) { assert(0); }
	void text(std::ostream&) const;
};


/** CM Service Type, GSM 04.08 10.5.3.3 */
class L3CMServiceType : public L3ProtocolElement {

	public:

	enum TypeCode {
		UndefinedType=0,
		MobileOriginatedCall=1,
		ShortMessage=4,							///< specifically, MO-SMS
		SupplementaryService=8,
		VoiceCallGroup=9,
		VoiceBroadcast=10,
		LocationService=11,
		MobileTerminatedCall=100,				///< non-standard code
		MobileTerminatedShortMessage=101,		///< non-standard code
		TestCall=102,			///< non-standard code
		HandoverCall=103,		///< non-standard code
		FuzzCall=104,			///< non-standard code
	};
		
	private:

	TypeCode mType;

	public:

	L3CMServiceType(TypeCode wType=UndefinedType)
		:L3ProtocolElement(),mType(wType)
	{}

	TypeCode type() const { return mType; }

	bool operator==(const L3CMServiceType& other) const
		{ return mType == other.mType; }

	bool operator!=(const L3CMServiceType& other) const
		{ return mType != other.mType; }
	
	size_t lengthV() const { return 0; }	
	void writeV(L3Frame&, size_t&) const { assert(0); }
	void parseV(const L3Frame &src, size_t &rp);
	void parseV(const L3Frame&, size_t&, size_t) { assert(0); }
	void text(std::ostream&) const;

};


std::ostream& operator<<(std::ostream& os, L3CMServiceType::TypeCode code);




} // GSM

#endif

// vim: ts=4 sw=4
