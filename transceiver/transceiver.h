/**
 * tranceiver.h
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * Tranceiver related classes
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

#ifndef TRANSCEIVER_H
#define TRANSCEIVER_H

#include <yateclass.h>
#include <yateradio.h>
#include "gsmutil.h"
#include "sigproc.h"

namespace TelEngine {

class GenQueue;                          // A queue of GenObject
class TransceiverObj;                    // A tranceiver related object
class TransceiverSockIface;              // Transceiver socket interface
class TrxRadioIO;                        // Radio tx/rx related data
class RadioRxData;                       // Radio read data
class Transceiver;                       // A transceiver
class TransceiverQMF;                    // A QMF transceiver
class TxFillerTable;                     // A transmit filler table
class ARFCN;                             // A transceiver ARFCN
class ARFCNSocket;                       // A transceiver ARFCN with socket interface
class TransceiverWorker;                 // Private worker thread


/**
 * This class implements a generic queue
 * @short A generic queue
 */
class GenQueue
{
public:
    /**
     * Constructor
     * @param maxLen Optional maximum queue length
     * @param name Queue name
     */
    GenQueue(unsigned int maxLen = 0, const char* name = "GenQueue");

    /**
     * Check if the queue is empty
     * @return True if the queue is empty
     */
    inline bool empty() const
	{ return m_empty; }

    /**
     * Add an object to the queue.
     * Wait for space to be freed if there is maximum queue length
     * Don't consume the given object on failure
     * @param obj Object to add
     * @param trx Optional transceiver to check for exiting condition
     * @return True on success, false on failure (queue full, calling thread was cancelled or
     *  given transceiver is exiting)
     */
    bool add(GenObject* obj, Transceiver* trx = 0);

    /**
     * Wait for an object to be put in the queue.
     * Extract the first element
     * Returns when there is something in the queue or the calling thread was
     *  cancelled or the given transceiver is exiting
     * @param obj Destination for extracted object
     * @param trx Optional transceiver to check for exiting condition
     * @return True on success, false if the calling thread should exit
     *  (the returned object is always 0 if false is returned)
     */
    bool waitPop(GenObject*& obj, Transceiver* trx = 0);

    /**
     * Clear the queue
     */
    void clear();

private:
    String m_name;

protected:
    /**
     * Pop an object from queue head
     * @return Queue head object, 0 if empty
     */
    GenObject* internalPop();

    Mutex m_mutex;                       // Queue mutex
    Semaphore m_free;                    // 
    Semaphore m_used;                    // 
    ObjList m_data;                      // The queue
    bool m_empty;                        // Queue is empty
    unsigned int m_count;                // The number of objects in queue
    unsigned int m_max;                  // Maximum length
};


/**
 * Base class for objects owned by transceiver
 * @short An object owned by a transceiver
 */
class TransceiverObj : public DebugEnabler , public RefObject
{
public:
    /**
     * Constructor
     */
    TransceiverObj()
	: m_transceiver(0)
	{}

    /**
     * Retrieve the transceiver
     * @return The transceiver owning this object
     */
    inline Transceiver* transceiver() const
	{ return m_transceiver; }

    /**
     * Retrieve the object debug prefix
     * @return Debug prefix
     */
    inline const char* prefix() const
	{ return m_prefix.safe(); }

    /**
     * Set the transceiver
     * @param t The transceiver
     * @param name Extra debug name
     */
    void setTransceiver(Transceiver& t, const char* name);

    /**
     * Worker terminated notification
     * @param th Worker thread
     */
    virtual void workerTerminated(Thread* th);

    /**
     * Dump to output a received burst
     * @param str String to print before data
     * @param time Burst time
     * @param c Pointer to data to dump
     * @param len The number of elements to dump
     */
    void dumpRecvBurst(const char* str, const GSMTime& time,
	const Complex* c, unsigned int len);

    /**
     * Dump RX Debug data
     * @param prefix Data prefix
     * @param index OMF or ARFCN index
     * @param postfix Data name postfix
     * @param c The data to dump
     * @param len The data length
     */
    void dumpRxData(const char* prefix, unsigned int index, const char* postfix,
	    const Complex* c, unsigned int len);

    /**
     * Dump RX Debug data
     * @param prefix Data prefix
     * @param index OMF or ARFCN index
     * @param postfix Data name postfix
     * @param f The data to dump
     * @param len The data length
     */
    void dumpRxData(const char* prefix, unsigned int index, const char* postfix,
	    const float* f, unsigned int len);

private:
    Transceiver* m_transceiver;          // The transceiver owning this object
    String m_name;                       // Debug name
    String m_prefix;                     // Debug prefix
};


/**
 * Holds socket, addresses and worker (read) thread
 * @short Transceiver socket interface
 */
class TransceiverSockIface
{
public:
    /**
     * Constructor
     * @param name Interface name
     * @param txt True if this a text interface
     * @param writeAttempts Optional write attempts
     */
    inline TransceiverSockIface(const char* name, bool txt = true,
	unsigned int writeAttempts = 1)
	: m_readBuffer(0,1500), m_text(txt),
	m_writeAttempts(writeAttempts ? writeAttempts : 1), m_name(name),
	m_printOne(false)
	{}

    /**
     * Destructor
     */
    ~TransceiverSockIface()
	{ terminate(); }

    /**
     * Retrieve the interface name
     * @return Interface name
     */
    inline const String& name() const
	{ return m_name; }

    /**
     * Set a socket address
     * @param local True to set the local, false to set the remote address
     * @param addr Address to set (can't be all interface address for remote)
     * @param port Port to set
     * @param dbg Debug holder (for debug purposes)
     * @return True on success, false on failure
     */
    bool setAddr(bool local, const char* addr, int port, TransceiverObj& dbg);

    /**
     * Prepare the socket: create, set reuse, bind, set unblocking mode.
     * Don't terminate the socket on failure
     * @param dbg Debug holder (for debug purposes)
     * @return True on success, false on failure
     */
    bool initSocket(TransceiverObj& dbg);

    /**
     * Read from socket. Call Thread::idle() if nothing is read
     * @param dbg Debug holder (for debug purposes)
     * @return The number of bytes read, negative on non retryiable error
     */
    int readSocket(TransceiverObj& dbg);

    /**
     * Write to socket. Put an alarm if dbg is non 0
     * @param buf Buffer to send
     * @param len The number of bytes to send
     * @param dbg Debug holder (for debug purposes)
     * @return The number of bytes wrote, negative on non retryiable error
     */
    int writeSocket(const void* buf, unsigned int len, TransceiverObj& dbg);

    /**
     * Terminate the socket
     * @param linger Linger interval
     */
    inline void terminate(int linger = -1) {
	    if (m_socket.valid()) {
		m_socket.setLinger(linger);
		m_socket.terminate();
	    }
	}

    /**
     * Print one burst sent to the socket.
     */
    inline void printOne()
	{ m_printOne = true; }

    Socket m_socket;                     // The socket
    SocketAddr m_local;                  // Socket local address
    SocketAddr m_remote;                 // Socket remote address
    DataBlock m_readBuffer;              // Socket read buffer

private:
    bool m_text;                         // Text interface
    unsigned int m_writeAttempts;        // Counter for write attempts
    String m_name;
    bool m_printOne;                     // Test flag used to print one packet sent to socket
};


/**
 * This structure holds data for a slot in ARFCN
 * @short An ARFCN slot
 */
struct ArfcnSlot
{
    inline ArfcnSlot()
	: slot(0), type(0), modulus(26), burstType(0), warnRecvDrop(true), delay(0),
	lastPowerWarn(0), powerErrors(0), lastTOAWarn(0), toaErrors(0)
	{}

    inline void addDelay(float d)
	{ delay += (delay - d) / 100; }

    uint8_t slot;                        // Timeslot number
    int type;                            // Timeslot (channel) type
    int modulus;                         // Modulus (used to build idle slot filler) in frames
    int burstType;                       // Expected burst type
    bool warnRecvDrop;                   // Warn dropping recv frame
    float delay;                         // Average delay spread
    GSMTime lastPowerWarn;               // Last time for power warn (used by demodulator)
    unsigned int powerErrors;            // The nmber of power errors
    GSMTime lastTOAWarn;                 // Last time for TOA error warn (used by demodulator)
    unsigned int toaErrors;              // Number of TOA errors (used by demodulator)
};


/**
 * This class holds radio Rx/Tx related data
 * @short Radio tx/rx related data
 */
class TrxRadioIO
{
public:
    /**
     * Constructor
     */
    inline TrxRadioIO()
	: timestamp(0), bursts(0),
	m_errCount(0), m_errMax(30), m_errInc(10)
	{}

    /**
     * Initialize error related data. Reset error counter
     * @param m Maximum number of errors allowed (minimum value: 2)
     * @param inc Error counter increment (it will be set to m_max / inc)
     */
    inline void initErrors(unsigned int m, unsigned int inc) {
	    m_errCount = 0;
	    m_errMax = (m > 2) ? m : 2;
	    m_errInc = (inc && inc < m_errMax) ? (m_errMax / inc) : 1;
	}

    /**
     * Update data
     * @param ok True on success, false on error
     * @return True if the counter is less then maximum allowed value, false otherwise
     */
    inline bool updateError(bool ok) {
	    if (ok) {
		if (m_errCount)
		    m_errCount--;
	    }
	    else {
		m_errCount += m_errInc;
		if (m_errCount >= m_errMax) {
		    m_errCount = 0;
		    return false;
		}
	    }
	    return true;
	}

    uint64_t timestamp;                  // Current I/O timestamp
    uint64_t bursts;                     // I/O bursts

protected:
    unsigned int m_errCount;             // Current number of errors
    unsigned int m_errMax;               // Maximum number of errors
    unsigned int m_errInc;               // Value to increase error counter on failure
};


/**
 * This class holds data read from radio device
 * @short Radio read data
 */
class RadioRxData : public GenObject
{
    YCLASS(RadioRxData,GenObject)
    YNOCOPY(RadioRxData);
public:
    /**
     * Constructor
     */
    inline RadioRxData()
	{}

    GSMTime m_time;
    ComplexVector m_data;
};

typedef ObjStore<RadioRxData> RadioRxDataStore;


/**
 * This class implements the interface to a transceiver
 * @short A transceiver
 */
class Transceiver : public TransceiverObj
{
    YCLASS(Transceiver,GenObject)
    YNOCOPY(Transceiver);
public:
    /**
     * Transceiver state
     */
    enum State {
	Invalid = 0,                     // Init failed, release the object
	Idle,                            // Not started
	PowerOff,                        // Upper layer interface started, radio Power OFF
	PowerOn,                         // Upper layer interface started, radio Power ON
    };

    /**
     * Command failure codes
     */
    enum CmdError {
	CmdEOk = 0,
	CmdEUnkCmd,
	CmdEFailure,
	CmdEInvalidParam,
	CmdEInvalidState,
	CmdEInvalidARFCN,
    };

    /**
     * Constructor
     * @param name Transceiver name
     */
    Transceiver(const char* name = "transceiver");

    /**
     * Destructor
     */
    ~Transceiver();

    /**
     * Retrieve the transceiver state
     * @return Transceiver state
     */
    inline int state() const
	{ return m_state; }

    /**
     * Retrieve the number of ARFCNs
     * @return The number of ARFCNs
     */
    inline unsigned int arfcnCount() const
	{ return m_arfcnCount; }

    /**
     * Retrieve an ARFCN
     * @return ARFCN pointer or 0
     */
    inline ARFCN* arfcn(unsigned int index) const
	{ return index < m_arfcnCount ? m_arfcn[index] : 0; }

    /**
     * Initialize the transceiver. This method should be called after construction
     * @param radio The radio interface. The transceiver will own of the object
     * @param params Transceiver parameters
     */
    virtual bool init(RadioInterface* radio, const NamedList& params);

    /**
     * Re-Initialize the transceiver.
     * Apply parameters that can be changed after construction
     * @param params Parameters list
     */
    virtual void reInit(const NamedList& params);

    /**
     * Wait for PowerOn state
     */
    void waitPowerOn();

    /**
     * Read radio loop
     */
    virtual void runReadRadio();

    /**
     * Process received radio data
     * @param d Radio data, the pointer will be consumed
     */
    virtual void recvRadioData(RadioRxData* d);

    /**
     * Run radio input data process loop
     */
    virtual void runRadioDataProcess();

    /**
     * Extract the bursts for sending.
     * Sum them for each ARFCN and send them to the radio interface.
     * @param time The time of the burst that needs to be sent
     * @return True on success, false on fatal error
     */
    bool sendBurst(GSMTime time);

    /**
     * Process a received radio burst
     * @param arfcn ARFCN requesting it
     * @param slot The slot receiving the burst
     * @param b The burst
     * @return True on success, false to ignore the burst
     */
    virtual bool processRadioBurst(unsigned int arfcn, ArfcnSlot& slot, GSMRxBurst& b);

    /**
     * Get Tx Time
     * @param time The Tx time
     */
    inline void getTxTime(GSMTime& time) {
	    Lock lck(m_clockUpdMutex);
	    time = m_txTime;
	}

    /**
     * Get the SignallProcessing instance used by this tranceiver
     * @return Reference to SignallProcessing instance
     */
    inline const SignalProcessing& signalProcessing() const
	{ return m_signalProcessing; }

    /**
     * Start the transceiver upper layer interface if not already done
     * @return True on success
     */
    bool start();

    /**
     * Stop the transceiver upper layer interface, power off the radio
     * @param dumpStat True to dump transceiver status
     * @param notify Notify exiting event on clock interface
     */
    void stop(bool dumpStat = false, bool notify = true);

    /**
     * Check if the transceiver is collecting statistics
     * @return True if the transceiver is collecting statistics
     */
    inline bool statistics() const
	{ return m_statistics; }

    /**
     * Start/stop collecting statistics
     * This method must be called before upper layer would start operating
     * @param on True to start, false to stop
     */
    inline void statistics(bool on)
	{ m_statistics = on; }

    /**
     * Check if TX time related debug is not silenced
     * @param time Time to check
     * @return True if TX time related debug is not silenced
     */
    inline bool txTimeDebugOk(const GSMTime& time) const
	{ return m_statistics && time.time() > m_txSilenceDebugTime; }

    /**
     * Execute a command
     * @param str Command string
     * @param rsp Optional response
     * @param arfcn Optional ARFCN
     * @return True on success
     */
    bool command(const char* str, String* rsp = 0, unsigned int arfcn = 0);

    /**
     * Handle control commands
     * @param oper Operation to execute
     * @param params Operation parameters
     * @return True on success, false on failure
     */
    bool control(const String& oper, const NamedList& params);

    /**
     * Check if the transceiver is exiting (stopping)
     * @return True if the transceiver is exiting
     */
    inline bool exiting() const
	{ return m_exiting; }

    /**
     * Check if the transceiver error flag is set
     * @return True if the transceiver error flag is set
     */
    inline bool inError() const
	{ return m_error; }

    /**
     * Check if shutdown command was received
     * @return True if shutdown command was received
     */
    inline bool shutdown() const
	{ return m_shutdown; }

    /**
     * Check if the transceiver should stop
     * @return True if the transceiver should stop
     */
    inline bool shouldStop() const
	{ return m_shutdown || m_error || m_exiting; }

    /**
     * Set the minimum power to accept radio bursts
     * @param val New minimum power level to accept
     */
    void setBurstMinPower(int val = 0);

    /**
     * Internal fatal error. Set the error flag
     */
    virtual void fatalError();

    /**
     * Run send data loop
     */
    void runRadioSendData();

    /**
     * Worker terminated notification
     * @param th Worker thread
     */
    virtual void workerTerminated(Thread* th);

    /**
     * Retrieve the state name dictionary
     * @return State name dictionary
     */
    static const TokenDict* dictStateName();

    /**
     * Get a copy of TX test burst
     * @return GSMTxBurst pointer, NULL if not set
     */
    inline GSMTxBurst* getTxTestBurstCopy() {
	    Lock lck(m_testMutex);
	    return GSMTxBurst::getCopy(m_txTestBurst);
	}

protected:
    /**
     * Destroy the object
     */
    virtual void destroyed();

    /**
     * Process received radio data
     * @param d Radio data, the pointer will be consumed
     */
    virtual void processRadioData(RadioRxData* d) = 0;

    /**
     * Starting radio power on notification
     */
    virtual void radioPowerOnStarting();

    /**
     * Set channel (slot) type
     * @param arfcn ARFCN number
     * @param slot Timeslot number
     * @param chanType Channel type
     */
    virtual void setChanType(unsigned int arfcn, unsigned int slot, int chanType);

    /**
     * ARFCN list changed notification
     */
    virtual void arfcnListChanged();

    /**
     * Dump the frequency shift vectors
     * @param index The index of the vector
     */
    virtual void dumpFreqShift(unsigned int index) const
	{ }

    /**
     * Helper method to check if the burst type is matching the timeslot and the time.
     * @param burst, The mburst to be sent.
     * @param time, The time on which the burst is sent,
     * @param isFiller True if the burst is a filler one.
     */
    void checkType(const GSMTxBurst& burst, const GSMTime& time, bool isFiller);

    inline void clampInt(int& val, uint64_t minVal, uint64_t maxVal, const char* loc) {
	    if (val >= (int)minVal && val <= (int)maxVal)
		return;
	    int c = val < (int)minVal ? minVal : maxVal;
	    Debug(this,DebugNote,"%s clamping %d -> %d [%p]",loc,val,c,this);
	    val = c;
	}
	
    inline bool radioCodeOk(unsigned int code,
	unsigned int ignoreMask = RadioInterface::Pending)
	{ return !code || (code & ignoreMask) != 0; }

    /**
     * Get the radio clock
     * @param dest The radio clock to be filled
     */
    inline void getRadioClock(GSMTime& dest) {
	    Lock lck(m_radioClockMutex);
	    dest = m_radioClock;
	}

    /**
     * Enable / disable internal loopback
     * @param enable True to enable loopback, false to disable
     */
    inline void setLoopback(bool enabled = false, int sleep = 0) {
	    Lock lck(m_loopbackMutex);
	    m_loopback = enabled;
	    m_loopbackSleep = sleep;
	}

    /**
     * Check if the radio interface is on loopback mode
     * @return True if loopback mode is active.
     */
    inline bool loopback() const
	{ return m_loopback; }

    /**
     * Set / reset the loopback data
     * @param data The loopback data
     */
    inline void setLoopbackData(ComplexVector* data = 0) {
	    Lock lck(m_loopbackMutex);
	    if (m_loopbackData)
		delete m_loopbackData;
	    m_loopbackData = data;
	}

    /**
     * Set test data limits. Data will be fabricated after formula:
     * from x = min to x = max : data = (val,val i), val += inc, if (val >= max) val = min
     * The fabricated data will be sent to bladerf 
     * NOTE this is for test
     * @param min The minimum value for input data
     * @param max The maximum value for input data
     * @param inc The increment value.
     * NOTE The data will not be wrapped to 2047
     */
    inline void setTestData(int min, int max, int inc) {
	    m_testMin = min;
	    m_testMax = max;
	    m_testIncrease = inc;
	    m_testValue = min;
	}

    /**
     * Wait untill we have to push a new buffer to the radio
     * @return True on success, false if the calling thread should exit
     */
    bool waitSendTx();

    /**
     * Dump transceiver status to output
     * @param printBursts Print burst counters
     * @param radioTime Optional pointer to current radio time
     */
    void dumpStatus(bool printBursts, GSMTime* radioTime = 0);

    /**
     * Notification of GSM time sync sent
     * @param time Sent time
     */
    virtual void syncGSMTimeSent(const GSMTime& time);

    int m_state;                         // Transceiver state
    Mutex m_stateMutex;                  // Serialize state change
    RadioInterface* m_radio;             // The radio interface
    Thread* m_radioReadThread;           // Worker (read radio data) thread
    Thread::Priority m_radioReadPrio;    // Read radio worker priority
    Thread* m_radioInThread;             // Worker (process radio input) thread
    Thread* m_radioOutThread;            // Worker (radio feeder) thread
    Thread::Priority m_radioOutPrio;     // Send radio worker priority
    GenQueue m_rxQueue;                  // Pending radio data
    unsigned int m_oversamplingRate;     // Tranceiver oversampling rate
    int m_burstMinPower;                 // Minimum burst power level to accept
    float m_snrThreshold;                // Signal to noise threshold
    float m_wrtFullScaleThreshold;       // WRT Threshold
    float m_peakOverMeanThreshold;       // Peak over mean threshold
    int m_upPowerThreshold;              // Upper power level threshold
    unsigned int m_maxPropDelay;         // Maximum RACH delay to try to track
    ARFCN** m_arfcn;                     // ARFCNs list
    unsigned int m_arfcnCount;           // The number of ARFCNs
    unsigned int m_arfcnConf;            // The number of configured ARFCNs
    TransceiverSockIface m_clockIface;   // Upper layer clock sync socket
    Mutex m_clockUpdMutex;               // Protect clock update and tx time
    GSMTime m_nextClockUpdTime;          // Next clock update time
    GSMTime m_lastClockUpd;              // Last clock value advertised to upper layer
    unsigned int m_clockUpdOffset;       // Offset (in timeslots) for radio time sent to upper layer
    GSMTime m_txTime;                    // Transmit time
    unsigned int m_txSlots;              // The number of timeslots to send in one loop
    unsigned int m_radioLatencySlots;    // Radio latency in timeslots
    int m_printStatus;                   // Print transceiver status
    bool m_printStatusBursts;            // Print bursts counters in status
    bool m_printStatusChanged;           // Print status changed flag
    bool m_radioSendChanged;             // Flag used to signal data changed for radio send data thread
    ComplexVector m_sendBurstBuf;        // Send burst buffer
    SignalProcessing m_signalProcessing; // SignalProcessing class initialized for this tranceiver
    unsigned int m_tsc;                  // GSM TSC index
    RadioRxDataStore m_radioRxStore;     // Radio rx bursts store
    double m_rxFreq;                     // Rx frequency
    double m_txFreq;                     // Tx frequency
    int m_txPower;                       // Tx power level (in dB)
    int m_txAttnOffset;                  // Tx power level attenuation
    float m_txPowerScale;                // Tx power scaling factor
    Mutex m_radioClockMutex;             // Radio clock mutex
    GSMTime m_radioClock;                // Radio clock
    Semaphore m_txSync;                  // Semaphore used to sync TX/RX data
    TrxRadioIO m_txIO;                   // TX related data
    TrxRadioIO m_rxIO;                   // RX related data
    uint64_t m_txSilenceDebugTime;       // TX: threshold used to silence some debug messages (time related)
    // Test data
    Mutex m_loopbackMutex;               // Loopback mutex
    bool m_loopback;                     // Loopback mode flag
    int m_loopbackSleep;                 // Time to sleep between two consecutive bursts sent in loopback mode
    uint64_t m_loopbackNextSend;         // Next time to send a loopback burst
    ComplexVector* m_loopbackData;       // Data to feed the RX part
    int m_testMin;                       // Minimum value for imput data
    int m_testMax;                       // Maximum value for input data
    int m_testIncrease;                  // Increment value
    int m_testValue;                     // Current input data value
    unsigned int m_sendArfcnFS;          // Flags mask used to send test data (send freq shift vectors)
    GSMTxBurst* m_txTestBurst;           // Test burst to be sent.
    Mutex m_testMutex;                   // Mutex used to block the access to test burst
    bool m_dumpOneTx;                    // Flag used to dump random Tx bursts
    bool m_dumpOneRx;                    // Flag used to dump random Rx bursts
    int m_toaShift;                      // TOA shift value to simulate timeing advance

private:
    bool radioSetPower(int p);
    // Power on the radio if not already done
    bool radioPowerOn(String* reason = 0);
    // Power off the radio if not already done
    void radioPowerOff();
    // Change transceiver state
    void changeState(int newState);
    // Initialize the ARFCNs list (set or release)
    bool resetARFCNs(unsigned int arfcns = 0, int port = 0,
	const String* rAddr = 0, const char* lAddr = 0, unsigned int nFillers = 0);
    // Stop all ARFCNs
    void stopARFCNs();
    // Sync upper layer GSM clock (update time)
    bool syncGSMTime(const char* msg = 0);
    // Handle (NO)HANDOVER commands. Return status code
    int handleCmdHandover(bool on, unsigned int arfcn, String& cmd, String* rspParam);
    // Handle SETSLOT command. Return status code
    int handleCmdSetSlot(unsigned int arfcn, String& cmd, String* rspParam);
    // Handle SETPOWER command. Return status code
    int handleCmdSetPower(unsigned int arfcn, String& cmd, String* rspParam);
    // Handle ADJPOWER command. Return status code.
    int handleCmdAdjustPower(unsigned int arfcn, String& cmd, String* rspParam);
    // Handle SETTXATTEN command. Resturn status code.
    int handleCmdSetTxAttenuation(unsigned int arfcn, String& cmd, String* rspParam);
    // Handle RXTUNE/TXTUNE commands. Return status code
    int handleCmdTune(bool rx, unsigned int arfcn, String& cmd, String* rspParam);
    // Handle SETTSC commands. Return status code
    int handleCmdSetTsc(unsigned int arfcn, String& cmd, String* rspParam);
    // Handle SETRXGAIN command. Return status code
    int handleCmdSetGain(bool rx, String& cmd, String* rspParam);
    // Handle CUSTOM command. Return status code
    int handleCmdCustom(String& cmd, String* rspParam, String* reason);
    // Handle loopback related custom commands
    bool handleCmdLoopback(int& code, String& cmd, String* rspParam, String* reason);
    // Handle MANAGETX command. NOTE: this is for test purposes
    int handleCmdManageTx(unsigned int arfcn, String& cmd, String* rspParam);
    int handleCmdFreqCorr(String& cmd, String* rspParam);
    // Send loopback data
    void sendLoopback(ComplexVector& buf, const GSMTime& time);
    // Adjust data to send (from test or dumper)
    void adjustSendData(ComplexVector& buf, const GSMTime& time);

    bool m_statistics;                   // Collect statistics
    bool m_error;                        // Fatal error occured
    bool m_exiting;                      // Stopping flag
    bool m_shutdown;                     // Shutdown command was received
};


/**
 * This structure holds data for a QMF block
 * @short A QMF block
 */
struct QmfBlock
{
    inline QmfBlock()
	: chans(false), arfcn(0xffffffff), freqShiftValue(0), power(0)
	{}
    bool chans;                          // ARFCN channels availablity in subtree
    unsigned int arfcn;                  // ARFCN
    float freqShiftValue;                // Frequency shift parameter
    ComplexVector data;                  // Input data
    ComplexVector freqShift;             // Frequency shifting vector
    ComplexVector halfBandFilter;        // Used when applying the half band filter
    float power;                         // Used to calculate the power level
};


/**
 * This class implements a Quadrature Mirror Filter transceiver
 * @short A QMF transceiver
 */
class TransceiverQMF : public Transceiver
{
    YCLASS(TransceiverQMF,Transceiver)
    YNOCOPY(TransceiverQMF);
public:
    /**
     * Constructor
     * @param name Transceiver name
     */
    TransceiverQMF(const char* name = "transceiver");

    /**
     * Initialize the transceiver. This method should be called after construction
     * @param radio The radio interface. The transceiver will own of the object
     * @param params Transceiver parameters
     */
    virtual bool init(RadioInterface* radio, const NamedList& params);

    /**
     * Re-Initialize the transceiver.
     * Apply parameters that can be changed after construction
     * @param params Parameters list
     */
    virtual void reInit(const NamedList& params);

    /**
     * Process a received radio burst
     * @param arfcn ARFCN requesting it
     * @param slot The slot receiving the burst
     * @param b The burst
     * @return True on success, false to ignore the burst
     */
    virtual bool processRadioBurst(unsigned int arfcn, ArfcnSlot& slot, GSMRxBurst& b);

protected:
    /**
     * Process received radio data
     * @param d Radio data, the pointer will be consumed
     */
    virtual void processRadioData(RadioRxData* d);

    /**
     * Starting radio power on notification
     */
    virtual void radioPowerOnStarting();

    /**
     * Set channel (slot) type
     * @param arfcn ARFCN number
     * @param slot Timeslot number
     * @param chanType Channel type
     */
    virtual void setChanType(unsigned int arfcn, unsigned int slot, int chanType);

    /**
     * ARFCN list changed notification
     */
    virtual void arfcnListChanged();

    /**
     * Dump the frequency shift vectors
     * @param index The index of the vector
     */
    virtual void dumpFreqShift(unsigned int index) const;

private:
    // Run the QMF algorithm on node at given index
    void qmf(const GSMTime& time, unsigned int index = 0);
    inline void qmfApplyFreqShift(QmfBlock& b) {
	    if (b.freqShift.length() < b.data.length())
		SignalProcessing::setFreqShifting(&b.freqShift,b.freqShiftValue,
		    b.data.length() + 10);

	    Complex::multiply(b.data.data(),b.data.length(),
		b.freqShift.data(),b.data.length());
	}
    void qmfBuildHalfBandFilter(QmfBlock& b);
    void qmfBuildOutputLowBand(QmfBlock& b, ComplexVector& y, float* power);
    void qmfBuildOutputHighBand(QmfBlock& b, ComplexVector& y, float* power);
    void initNormalBurstTSC(unsigned int len = 16);
    void initAccessBurstSync();
    // Check demodulator performance
    void checkDemodPerf(const ARFCN* a, const GSMRxBurst& b, int bType);

    QmfBlock m_qmf[15];                  // QMF tree
    unsigned int m_halfBandFltCoeffLen;  // Half band filter coefficients length
    FloatVector m_halfBandFltCoeff;      // Half band filter coefficients vector
    unsigned int m_tscSamples;           // The number of TSC samples used to build the channel estimate
    FloatVector m_nbTSC[8];              // GSM Normal Burst TSC vectors
    FloatVector m_abSync;                // Access burst sync vector
    bool m_checkDemodPerf;               // Check demodulator performance
};

/**
 * Helper class to show mbts data excenge
 */
class TrafficShower
{
public:
    /**
     * Constructor
     * @param in True for RX false for TX
     * @param arfcn The arfcn index
     */
    TrafficShower(bool in,unsigned int arfcn);

    /**
     * Parse a list of arguments
     * @param args List of arguments
     */
    void parse(const ObjList& args);

    /**
     * Show a burst
     */
    void show(GSMBurst* burst);
private:
    bool m_show;
    bool m_in;
    unsigned int m_arfcn;
    int m_modulus;
    bool m_table[8][102];
};

/**
 * Helper class to shape the tx traffic
 */
class TrafficShaper
{
public:
    /**
     * Constructor
     * @param arfcn The ARFCN which owns this shaper
     * @param index The ARFCN index
     */
    TrafficShaper(ARFCN* arfcn, unsigned int index);

    /**
     * Get shaped traffic.
     * @param t The time of the shaped burst.
     * @return 0 If the traffic is not shaped. The shaping burst otherwise.
     */
    GSMTxBurst* getShaped(const GSMTime& t);

    /**
     * Initialize this shaper.
     * @param args List of arguments
     */
    void parse(ObjList* args);
private:
    int m_table[8];
    ARFCN* m_arfcn;
};

typedef SigProcVector<GSMTxBurst*> GSMTxBurstPtrVector;

/**
 * This class implements a transmit filler table
 * @short A transmit filler table
 */
class TxFillerTable
{
public:
    /**
     * Constructor
     * Allocates 
     */
    inline TxFillerTable(ARFCN* owner)
	: m_filler(0), m_owner(owner)
	{ init(1); }

    /**
     * Destructor
     */
    ~TxFillerTable()
	{ clear(); }

    /**
     * Set filler at burst time
     * @param burst The burst to set, it will be consumed
     */
    inline void set(GSMTxBurst* burst) {
	    if (!burst)
		return;
	    GSMTxBurst** tmp = fillerHolder(burst->time());
	    if (*tmp == burst)
		return;
	    if (*tmp != m_filler)
		store(*tmp);
	    *tmp = burst;
	}
	
    /**
     * Get a burst from the filler table
     * @param time The time of the filler burst
     * @return 0 if no filler burst is stored for the given time
     */
    inline GSMTxBurst* get(const GSMTime& time)
	{ return *(fillerHolder(time)); }

    /**
     * Initialize the filler table
     * @param len Table length (it will be forced to minimum value if less than it)
     * @param filler Filler burst. The table will take ownership of it
     */
    void init(unsigned int len, GSMTxBurst* filler = 0);

private:
    GSMTxBurst** fillerHolder(const GSMTime& time);
    void store(GSMTxBurst* burst);
    void clear();

    GSMTxBurst* m_filler;                // Filler
    SigProcVector<GSMTxBurstPtrVector,false> m_fillers; // Table of filler bursts
    ARFCN* m_owner;
};


class ARFCNStatsTx
{
public:
    inline ARFCNStatsTx()
	: burstsDwIn(0),
	burstsMissedSyncOnSend(0), burstsExpiredOnSend(0), burstsExpiredOnRecv(0),
	burstsFutureOnRecv(0), burstsDupOnRecv(0) {
		for (uint8_t i = 0; i < 8; i++)
		    burstsDwInSlot[i] = 0;
	    }
    GSMTime burstLastInTime;             // Time of last downlink burst received from upper layer
    uint64_t burstsDwIn;                 // The number of bursts received from upper layer for TX
    uint64_t burstsDwInSlot[8];          // The number of bursts received from upper layer for TX per timeslot
    uint64_t burstsMissedSyncOnSend;     // The number a SYNC burst is missing on send time
    uint64_t burstsExpiredOnSend;        // The number of expired bursts on send time
    uint64_t burstsExpiredOnRecv;        // The number of already expired bursts on receive time
    uint64_t burstsFutureOnRecv;         // The number of bursts in future on receive time
    uint64_t burstsDupOnRecv;            // The number of duplicate bursts on receive time
};


/**
 * This class implements a transceiver ARFCN
 * @short An ARFCN
 */
class ARFCN : public TransceiverObj
{
    YCLASS(ARFCN,GenObject)
    YNOCOPY(ARFCN);
    friend class Transceiver;
    friend class TransceiverQMF;
public:
    /**
     * Channel type
     */
    enum ChannelType {
	ChanFill,                        // Channel is transmitted, but unused
	ChanI,                           // TCH/FS
	ChanII,                          // TCH/HS, idle every other slot
	ChanIII,                         // TCH/HS
	ChanIV,                          // FCCH+SCH+CCCH+BCCH, uplink RACH
	ChanV,                           // FCCH+SCH+CCCH+BCCH+SDCCH/4+SACCH/4, uplink RACH+SDCCH/4
	ChanVI,                          // CCCH+BCCH, uplink RACH
	ChanVII,                         // SDCCH/8 + SACCH/8
	ChanNone,                        // Channel is inactive
	ChanLoopback,                    // Similar to VII, used in loopback testing
	ChanIGPRS                        // GPRS channel, like I with static filler frames
    };

    /**
     * Expected channel burst type
     */
    enum BurstType {
	BurstNormal,                     // Normal burst (with Training Sequence Code)
	BurstAccess,                     // Access burst (RACH, handover)
	BurstIdle,                       // Idle burst
	BurstNone,                       // Timeslot is inactive
	BurstUnknown,                    // Unknown
	BurstCheck                       // Check slot type and frame number
    };

    /**
     * RX burst drop reason
     */
    enum RxDropReason {
	RxDropLowSNR = 0,
	RxDropLowPeakMin,
	RxDropLowPower,
	RxDropSlotUnk,
	RxDropNullRMS,
	RxDropNegativeMaxIndex,
	RxDropQueueFull,
	RxDropInvalidTimeslot,
	RxDropInvalidLength,
	RxDropCount
    };

    /**
     * Constructor
     * @param index Carrier index.
     */
    ARFCN(unsigned int index);

    /**
     * Destructor, stop the ARFCN
     */
    ~ARFCN();

    /**
     * Retrieve the channel number
     * @return ARFCN number
     */
    inline unsigned int arfcn() const
	{ return m_arfcn; }

    /**
     * Retrieve the number of configured channels
     * @return The number of configured channels
     */
    inline uint8_t chans() const
	{ return m_chans; }

    /**
     * Check if TX time related debug is not silenced
     * @param time Time to check
     * @return True if TX time related debug is not silenced
     */
    inline bool txTimeDebugOk(const GSMTime& time) const
	{ return !transceiver() || transceiver()->txTimeDebugOk(time); }

    /**
     * Dump channels type
     * @param buf Destination buffer
     */
    void dumpChanType(String& buf, const char* sep = ",");

    /**
     * Add a burst to this ARFQN's queue
     * @param burst The burst to add.
     */
    void addBurst(GSMTxBurst* burst);

    /**
     * Obtain the burst that has to be sent at the specified time
     * @param time The burst time
     * @param owner Set to true on exit if the returned burst will be owned by the caller
     * @return GSMTxBurst pointer or 0
     */
    GSMTxBurst* getBurst(const GSMTime& time, bool& owner);

    /**
     * Process received radio data
     * @param d Radio data, the pointer will be consumed
     */
    void recvRadioData(RadioRxData* d);

    /**
     * Worker terminated notification
     * @param th Worker thread
     */
    virtual void workerTerminated(Thread* th);

    /**
     * Run radio input data process loop
     */
    virtual void runRadioDataProcess();

    /**
     * Dump the TxQueue bursts.
     */
    void dumpBursts();

    /**
     * Retrieve the channel type dictionary
     * @return TokenDict pointer
     */
    static const TokenDict* chanTypeName();

    /**
     * Retrieve the string associated with a channel type
     * @param t Channel type
     * @return Requested channel type name
     */
    static inline const char* chanType(int t)
	{ return lookup(t,chanTypeName()); }

    /**
     * Retrieve the burst type dictionary
     * @return TokenDict pointer
     */
    static const TokenDict* burstTypeName();

    /**
     * Retrieve the string associated with a burst type
     * @param t Burst type
     * @return Requested burst type name
     */
    static inline const char* burstType(int t)
	{ return lookup(t,burstTypeName()); }

    /**
     * Add current noise level to averege
     * @param current The curent noise level
     */
    void addAverageNoise(float current);

    /**
     * Obtain the averege noise level
     * @return The average noise level for this arfcn
     */
    float getAverageNoiseLevel()
	{ return m_averegeNoiseLevel; }

    /**
     * Dump the delay spread for each slot
     * @param dest The destination buffer for data to be dumped
     */
    void dumpSlotsDelay(String& dest);

    /**
     * Add delaySpread to slot
     * @param index Slot index
     * @param delay The last delay spread 
     */
    inline void addSlotDelay(int index, float delay)
    {
	if (index > 7)
	    return;
	m_slots[index].addDelay(delay);
    }

    /**
     * Get filler frame modulus to determine it's place into the filler table
     * @param tn The timeslot number of the filler frame.
     * @return The filler frame modulus
     */
    inline unsigned int getFillerModulus(unsigned int tn)
	{ return m_slots[tn].modulus; }

    /**
     * Set traffic showing arguments
     * @param in True to set the arguments for the traffic received from mbts into tranceiver
     * @param args List of arguments
     */
    inline void showTraffic(bool in,const ObjList& args)
	{ (!in ? m_txTraffic : m_rxTraffic).parse(args); }

    /**
     * Helper method to get a filler burst.
     * @param t Time of the filler burst
     * @return The filler burst.
     */
    inline GSMTxBurst* getFiller(const GSMTime& t)
	{ return m_fillerTable.get(t); }

    /**
     * Retrieve TX statistics
     * @param dest Destination
     */
    inline void getTxStats(ARFCNStatsTx& dest) {
	    Lock lck(m_txMutex);
	    dest = m_txStats;
	}

    GSMTxBurstStore m_txBurstStore;

protected:
    /**
     * Initialize a timeslot and related data
     * @param slot Timeslot number
     * @param chanType Channel type
     */
    void initTimeslot(unsigned int slot, int chanType);

    /**
     * Set timeslot access burst type
     * @param slot Timeslot number
     * @param on True to set access burst type, false to chan type default
     */
    inline void setBurstAccess(unsigned int slot, bool on) {
	    if (slot < 8)
		setSlotBurstType(m_slots[slot],
		    on ? BurstAccess : getChanBurstType(m_slots[slot].type));
	}

    /**
     * Start the ARFCN, stop it if already started
     * @return True on success
     */
    virtual bool start();

    /**
     * Stop the ARFCN
     */
    virtual void stop();

    /**
     * Initialize/start radio power on related data
     * @param reason Optional failure reason
     * @return True on success
     */
    virtual bool radioPowerOn(String* reason = 0);

    /**
     * Stop radio power on related data
     */
    virtual void radioPowerOff();

    /**
     * Forward a burst to upper layer
     * @param burst The burst to process. The pointer must be reset if consumed
     * @return True on success, false on fatal error
     */
    virtual bool recvBurst(GSMRxBurst*& burst);

    Mutex m_mutex;                       // Protect data changes
    GenQueue m_rxQueue;                  // Radio input
    RadioRxDataStore m_radioRxStore;     // Radio rx bursts store
    Thread* m_radioInThread;             // Radio input processor
    uint8_t m_chans;                     // The number of channels whose type is not ChanNone
    ArfcnSlot m_slots[8];                // Channels
    uint64_t m_rxBurstsStart;            // Received bursts start time
    uint64_t m_rxBursts;                 // Received bursts
    uint64_t m_rxDroppedBursts[RxDropCount]; // Dropped Rx bursts
    GSMTime m_lastUplinkBurstOutTime;    // Time of last uplink burst sent to upper layer
    ARFCNStatsTx m_txStats;              // TX statistics
    float m_averegeNoiseLevel;           // Averege noise level
    TrafficShower m_rxTraffic;           // RX traffic parameters
    TrafficShower m_txTraffic;           // TX traffic parameters

private:
    void dropRxBurst(int dropReason, const GSMTime& t = GSMTime(),
	unsigned int len = 0, int level = DebugAll, RadioRxData* d = 0,
	const char* reason = 0);
    uint64_t dumpDroppedBursts(String* buf, const char* sep = " ",
	bool force = false, bool valueOnly = false);
    void setSlotBurstType(ArfcnSlot& slot, int val);
    static inline int burstType(ArfcnSlot& slot, unsigned int fn) {
	    if (slot.burstType != BurstCheck)
		return slot.burstType;
	    return burstTypeCheckFN(slot.type,fn);
	}
    static int getChanBurstType(int chanType);
    static int burstTypeCheckFN(int chanType, unsigned int fn);

    unsigned int m_arfcn;                // ARFCN
    TxFillerTable m_fillerTable;         // Fillers table
    Mutex m_txMutex;                     // Transmit queue blocker
    ObjList m_txQueue;                   // Transmit queue
    ObjList m_expired;                   // List of expired bursts
    TrafficShaper m_shaper;              // Traffic shaper
};


/**
 * This class implements a transceiver ARFCN with socket interface
 * @short A transceiver ARFCN with socket interface
 */
class ARFCNSocket : public ARFCN
{
    YCLASS(ARFCNSocket,ARFCN)
    YNOCOPY(ARFCNSocket);
    friend class Transceiver;
public:
    /**
     * Constructor
     * @param index The arfcn index.
     */
    ARFCNSocket(unsigned int index);

    /**
     * Destructor, stop the ARFCN
     */
    ~ARFCNSocket();

    /**
     * Run read data socket loop
     */
    void runReadDataSocket();

    /**
     * Worker terminated notification
     * @param th Worker thread
     */
    virtual void workerTerminated(Thread* th);

    /**
     * Set the print socket flag
     */
    inline void setPrintSocket()
	{ m_data.printOne(); }

protected:
    /**
     * Start the ARFCN, stop it if already started
     * Start the processing threads
     * @return True on success
     */
    virtual bool start();

    /**
     * Stop the ARFCN.
     * Stop the processing threads
     */
    virtual void stop();

    /**
     * Initialize/start radio power on related data
     * @param reason Optional failure reason
     * @return True on success
     */
    virtual bool radioPowerOn(String* reason = 0);

    /**
     * Stop radio power on related data
     */
    virtual void radioPowerOff();

    /**
     * Forward a burst to upper layer
     * @param burst The burst to process. The pointer must be reset if consumed
     * @return True on success, false on fatal error
     */
    virtual bool recvBurst(GSMRxBurst*& burst);

    /**
     * Initialize UDP addresses
     * @param rPort Remote port
     * @param rAddr Remote address
     * @param lPort Local port to bind on
     * @param lAddr Local address to bind on
     */
    bool initUDP(int rPort, const char* rAddr, int lPort,
	const char* lAddr = "0.0.0.0");

    TransceiverSockIface m_data;         // Data interface
    Thread* m_dataReadThread;            // Worker (read data socket) thread
};


/**
 * This class implements the transceiver interface to a radio device
 * @short A radio interface
 */
class RadioIface : public TransceiverObj
{
    YCLASS(RadioIface,GenObject)
    YNOCOPY(RadioIface);
public:
    /**
     * Constructor
     */
    RadioIface();

    /**
     * Initialize radio
     * @param params Radio parameters
     * @return True on success, false on failure
     */
    virtual bool init(const NamedList& params);

    /**
     * Start the radio
     * @return True on success
     */
    bool powerOn();

    /**
     * Stop the radio
     */
    void powerOff();

    /**
     * Worker terminated notification
     * @param th Worker thread
     */
    virtual void workerTerminated(Thread* th);

    /**
     * Retrieve device factory data
     * @param name Data name to retrieve
     * @param value Destination string
     * @return 0 on success, non 0 on failure
     */
    virtual int getFactoryValue(const String& name, String& value);

    /**
     * Set frequency correction offset
     * @param val The new value
     * @return True on success false on failure
     */
    virtual bool setFreqCorr(int val) = 0;

    /**
     * Execute a command
     * @param cmd Command to execute
     * @param rspParam Optional response
     * @param reason Optional failure reason
     * @return 0 on success, non 0 on failure
     */
    virtual int command(const String& cmd, String* rspParam = 0, String* reason = 0);

    /**
     * Get The last timestamp received from the bord
     * @return The last timestamp received from the board
     */
    virtual u_int64_t getBoardTimestamp()
	{ return 0; }

protected:
    Mutex m_mutex;
    unsigned int m_samplesPerSymbol;     // Samples per symbol
    Thread* m_readThread;                // Read thread

private:
    bool m_powerOn;                      // Powered on
    // Test data
    bool m_showClocks;                   // Flag used to dump all clocks
};

}; // namespace TelEngine

#endif // TRANSCEIVER_H

/* vi: set ts=8 sw=4 sts=4 noet: */
