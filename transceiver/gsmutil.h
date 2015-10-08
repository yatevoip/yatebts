/**
 * gsmutil.h
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * GSM related utilities
 *
 * Yet Another Telephony Engine - Base Transceiver Station
 * Copyright (C) 2014 Null Team Impex SRL
 *
 * This software is distributed under multiple licenses;
 * see the COPYING file in the main directory for licensing
 * information for this specific distribution.
 *
 * This use of this software may be subject to additional restrictions.
 * See the LEGAL file in the main directory for details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef GSMUTIL_H
#define GSMUTIL_H

#include <yateclass.h>
#include "sigproc.h"

namespace TelEngine {

class GSMTime;                           // GSM time
class GSMBurst;                          // A GSM burst
class GSMTxBurst;                        // A GSM burst to send
class GSMRxBurst;                        // A received GSM burst

// TDMA frame number max value (see TS 100 908 (GSM 05.02) Section 4.3.3)
// The actual maximum value is GSM_HYPERFRAME_MAX - 1 (2715647)
#define GSM_HYPERFRAME_MAX (26U * 51U * 2048U)
// The number of timeslots in a hyperframe
#define GSM_HYPERFRAME_TS (GSM_HYPERFRAME_MAX * 8U)
// The number of timeslots in 1 second (216(6) frames)
#define GSM_SLOTS_SEC (217 * 8)

// Burst length in symbols
#define GSM_BURST_LENGTH 148
// TX burst lengths
#define GSM_BURST_TXHEADER 6
#define GSM_BURST_TXPACKET (GSM_BURST_TXHEADER + GSM_BURST_LENGTH)

// The number of GSM TSC (Training Sequence Code) Normal Burst
#define GSM_NB_TSC_LEN 26
// The number of GSM Sync bits for Access Burst
#define GSM_AB_SYNC_LEN 41


/**
 * This class holds GSM related utility data and methods
 * @short GSM utilities
 */
class GSMUtils
{
public:
    /**
     * Retrieve the TSC (Training Sequence Code) table for Normal Burst
     * @return Pointer to 8 by GSM_NB_TSC_LEN Normal Burst TSC table
     */
    static const int8_t* nbTscTable();

    /**
     * Retrieve the Sync table for Access Burst
     * @return Pointer to GSM_AB_SYNC_LEN Access Burst Sync table
     */
    static const int8_t* abSyncTable();
};


/**
 * This class implements GSM time as defined in GSM 05.02 4.3
 * The time is held in timeslots
 * @short GSM time
 */
class GSMTime
{
public:
    /**
     * Constructor
     */
   inline GSMTime()
	: m_time(0)
	{}

    /**
     * Constructor
     * @param fn Frame number
     * @param tn Timeslot number
     */
    inline GSMTime(uint64_t time)
	{ *this = time; }

    /**
     * Constructor
     * @param fn Frame number
     * @param tn Timeslot number
     */
    inline GSMTime(uint32_t fn, uint8_t tn)
	{ assign(fn,tn); }

    /**
     * Copy constructor
     * @param src Source object
     */
    inline GSMTime(const GSMTime& src)
	: m_time(src.m_time)
	{}

    /**
     * Assign absolute time.
     * @param fn Frame number
     * @param tn Timeslot number
     */
    inline void assign(uint32_t fn, uint8_t tn)
	{ m_time = fn * 8 + tn; }

    /**
     * Retrieve the time
     * @return Time value
     */
    inline uint64_t time() const
	{ return m_time; }

    /**
     * Retrieve the frame number
     * @return Frame number
     */
    inline uint32_t fn() const
	{ return time2fn(m_time); }

    /**
     * Retrieve the timeslot number
     * @return Timeslot number
     */
    inline uint8_t tn() const
	{ return m_time % 8; }

    /**
     * Retrieve the timeslot number in hyperframe
     * @return Timeslot number
     */
    inline uint32_t timeslot() const
	{ return m_time % GSM_HYPERFRAME_TS; }

    /**
     * Advance time
     * @param fn Frame number to advance
     * @param tn Timeslot number to advance
     */
    inline void advance(uint32_t fn, uint8_t tn = 0)
	{ m_time += fn * 8 + tn; }

    /**
     * Greater then comparison operator
     * @param other Object to compare with
     * @return True if this time is greater than given time
     */
    inline bool operator>(const GSMTime& other) const
	{ return diff(other) > 0; }

    /**
     * Less then comparison operator
     * @param other Object to compare with
     * @return True if this time is less than given time
     */
    inline bool operator<(const GSMTime& other) const
	{ return diff(other) < 0; }

    /**
     * Greater then or equal to comparison operator
     * @param other Object to compare with
     * @return True if this time is greater or equal than given time
     */
    inline bool operator>=(const GSMTime& other) const
	{ return diff(other) >= 0; }

    /**
     * Less then or equal to comparison operator
     * @param other Object to compare with
     * @return True if this time is less or equal than given time
     */
    inline bool operator<=(const GSMTime& other) const
	{ return diff(other) <= 0; }

    /**
     * Equality operator, compares modulo hyperframe length
     * @param other Object to compare with
     * @return True if this time is equal with given time
     */
    inline bool operator==(const GSMTime& other) const
	{ return diff(other) == 0; }

    /**
     * Inequality operator
     * @param other Object to compare with
     * @return True if this time is not equal with given time
     */
    inline bool operator!=(const GSMTime& other) const
	{ return diff(other) != 0; }

    /**
     * Asignment operator
     * @param src Source object
     */
    inline GSMTime& operator=(const GSMTime& src) {
	    m_time = src.m_time;
	    return *this;
	}

    /**
     * Asignment operator
     * @param time Time to assign
     */
    inline GSMTime& operator=(uint64_t time) {
	    m_time = time;
	    return *this;
	}

    /**
     * Postfix increment timeslot operator
     */
    inline GSMTime& operator++(int) {
	    m_time++;
	    return *this;
	}

    /**
     * Cast to uint64_t operator
     */
    inline operator uint64_t() const
	{ return m_time; }

    /**
     * Compute difference in timeslots to another time in same hyperframe
     * @param other Object to substract from this
     * @return Difference in timeslots taking into account hyperframe wraparound
     */
    inline int32_t diff(const GSMTime& other) const {
	uint32_t t = (GSM_HYPERFRAME_TS + timeslot() - other.timeslot()) % GSM_HYPERFRAME_TS;
	return (t < (GSM_HYPERFRAME_TS / 2)) ? t : (t - GSM_HYPERFRAME_TS);
    }

    /**
     * Append this time to a String
     * @param buf Destination string
     * @return Destination string reference
     */
    inline String& appendTo(String& buf) const {
	    buf << time() << " " << fn() << "/" << tn();
	    return buf;
	}

    /**
     * Append this time to a String. Clear it before
     * @param buf Destination string
     * @return Destination string c_str
     */
    inline const char* c_str(String& buf) const {
	    buf.clear();
	    return appendTo(buf);
	}

    /**
     * Convert an absolute time in timeslots to frame number
     * @param time Value to convert
     * @return Frame number
     */
    static inline uint32_t time2fn(uint64_t time)
	{ return (uint32_t)((time / 8) % GSM_HYPERFRAME_MAX); }

private:
    uint64_t m_time;
};


/**
 * Append operator: append a GSMTime to a String
 * @param str Destination string
 * @param time GSMTime to append
 * @return Destination string reference
 */
inline String& operator<<(String& str, const GSMTime& time)
{
    return time.appendTo(str);
}


/**
 * This class implements a GSM burst
 * @short A GSM Burst
 */
class GSMBurst : public DataBlock
{
    YCLASS(GSMBurst,DataBlock)
    YNOCOPY(GSMBurst);
public:
    /**
     * Constructor
     */
    inline GSMBurst()
	{}

    /**
     * Retrieve burst time
     * @return Burst time
     */
    inline const GSMTime& time() const
	{ return m_time; }

    /**
     * Set burst time
     * @param t Burst time
     */
    inline void time(const GSMTime& t)
	{ m_time = t; }

protected:
    GSMTime m_time;                      // Burst time
};


/**
 * This class implements a GSM burst to be sent
 * @short A GSM burst to send
 */
class GSMTxBurst : public GSMBurst
{
    YCLASS(GSMTxBurst,GSMBurst)
    YNOCOPY(GSMTxBurst);
public:
    /**
     * Constructor
     */
    inline GSMTxBurst()
	: m_powerLevel(0), m_filler(false), m_type(0)
	{}

    /**
     * Check if this burst is a filler one
     * @return True if this burst is a filler
     */
    inline bool filler() const
	{ return m_filler; }

    /**
     * Retrieve the TX data
     * @return TX data
     */
    inline const ComplexVector& txData() const
	{ return m_txData; }

    /**
     * Build TX data
     * @param proc The signal processor
     * @param tmpV Optional temporary buffer to be passed to signal processing
     * @param tmpW Optional temporary buffer to be passed to signal processing
     * @param buf Optional buffer to parse, use this burst if empty
     */
    void buildTxData(const SignalProcessing& proc,
	FloatVector* tmpV = 0, ComplexVector* tmpW = 0,
	const DataBlock& buf = DataBlock::empty());

    /**
     * Obtain a burst from the given data.
     * Copy burst data (excluding the header) in DataBlock if dataBits is 0
     * @param buf The data to parse
     * @param len The data length
     * @param store Store to retrieve a new burst from
     * @param dataBits Optional destination for burst data bits.
     *  If set it will be filled with burst data (bits) without owning the buffer.
     *  The caller is responsable of calling DataBlock::clear(false) on it
     * @return A new burst if the data is valid
     */
    static GSMTxBurst* parse(const uint8_t* buf, unsigned int len,
	ObjStore<GSMTxBurst>& store, DataBlock* dataBits = 0);

    /**
     * Obtain a burst from the given data
     * @param data The data to parse
     * @param store Store to retrieve a new burst from
     * @return A new burst if the data is valid
     */
    static inline GSMTxBurst* parse(const DataBlock& data, ObjStore<GSMTxBurst>& store)
	{ return parse(data.data(0),data.length(),store); }

    /**
     * Build a filler burst
     * @return A new burst with valid data
     */
    static GSMTxBurst* buildFiller();

    /**
     * Get burst type
     * NOTE this is for testing and is not reliable
     * MBTS modifications must be done!
     * @return Burst type.
     */
    inline int type() const
	{ return m_type; }

    /**
     * Get "void air: burst; A burst with txdata containing only 0
     * @param sigproc The signal processor
     * @return The void air burst.
     */
    static GSMTxBurst* getVoidAirBurst(const SignalProcessing& sig) {
	    GSMTxBurst* t = new GSMTxBurst();
	    t->m_txData.resize(sig.gsmSlotLen());
	    return t;
	}

    /**
     * Get a copy of the burst.
     * @param burst The original burst or 0
     * @return A copy of the burst or 0.
     */
    static GSMTxBurst* getCopy(const GSMTxBurst* burst) {
	    if (!burst)
		return 0;
	    GSMTxBurst* ret = new GSMTxBurst();
	    ret->m_txData.copy(burst->m_txData);
	    ret->m_powerLevel = burst->m_powerLevel;
	    ret->m_filler = burst->m_filler;
	    ret->m_type = burst->m_type;
	    return ret;
	}

private:
    ComplexVector m_txData;
    float m_powerLevel;
    bool m_filler;                       // Filler burst
    int m_type;
};

typedef ObjStore<GSMTxBurst> GSMTxBurstStore;


/**
 * This class implements a received GSM burst
 * @short A received GSM burst
 */
class GSMRxBurst : public GSMBurst
{
    YCLASS(GSMRxBurst,GSMBurst)
    YNOCOPY(GSMRxBurst);
public:
    /**
     * Constructor
     */
    inline GSMRxBurst()
	: m_powerLevel(0), m_timingError(0)
	{}

    /**
     * Fill the buffer start with time TN/FN, RSSI and timing offset
     */
    void fillEstimatesBuffer();

    float m_powerLevel;                  // Received power level
    float m_timingError;                 // Timing advance error
    ComplexVector m_data;                // Data
    uint8_t m_bitEstimate[156];          // Output
};

}; // namespace TelEngine

#endif // GSMUTIL_H

/* vi: set ts=8 sw=4 sts=4 noet: */
