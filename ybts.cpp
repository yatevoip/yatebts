/**
 * ybts.cpp
 * This file is part of the 
 *
 * Yet Another BTS Channel
 *
 * Yet Another Telephony Engine - a fully featured software PBX and IVR
 * Copyright (C) 2004-2013 Null Team
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

#include <yatephone.h>
#include <yategsm.h>
#include "ybts.h"

#ifdef _WINDOWS
#error This module is not for Windows
#endif

#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>

using namespace TelEngine;
namespace { // anonymous

// Handshake interval (timeout)
#define YBTS_HK_INTERVAL_DEF 60000
// Heartbeat interval
#define YBTS_HB_INTERVAL_DEF 60000
// Restart time
#define YBTS_RESTART_DEF 120000
#define YBTS_RESTART_MIN 30000
#define YBTS_RESTART_MAX 600000

class YBTSConnIdHolder;                  // A connection id holder
class YBTSThread;
class YBTSThreadOwner;
class YBTSMessage;                       // YBTS <-> BTS PDU
class YBTSTransport;
class YBTSLAI;                           // Holds local area id
class YBTSConn;                          // A logical connection
class YBTSLog;                           // Log interface
class YBTSSignalling;                    // Signalling interface
class YBTSMedia;                         // Media interface
class YBTSUE;                            // A registered equipment
class YBTSLocationUpd;                   // Pending location update from UE
class YBTSMM;                            // Mobility management entity
class YBTSDataSource;
class YBTSDataConsumer;
class YBTSCallDesc;
class YBTSChan;
class YBTSDriver;

// ETSI TS 100 940
// Section 8.5 / Section 10.5.3.6 / Annex G
enum RejectCause {
    CauseServNotSupp = 32,               // Service option not implemented
    CauseInvalidIE = 96,                 // Invalid mandatory IE
    CauseProtoError = 111,               // Protocol error, unspecified
};

class YBTSConnIdHolder
{
public:
    inline YBTSConnIdHolder(uint16_t connId = 0)
	: m_connId(connId)
	{}
    inline uint16_t connId() const
	{ return m_connId; }
    inline void setConnId(uint16_t cid)
	{ m_connId = cid; }
protected:
    uint16_t m_connId;
};

class YBTSDebug
{
public:
    inline YBTSDebug(DebugEnabler* enabler = 0, void* ptr = 0)
	{ setDebugPtr(enabler,ptr); }
    inline void setDebugPtr(DebugEnabler* enabler, void* ptr) {
	    m_enabler = enabler;
	    m_ptr = ptr;
	}
protected:
    DebugEnabler* m_enabler;
    void* m_ptr;
};

class YBTSThread : public Thread
{
public:
    YBTSThread(YBTSThreadOwner* owner, const char* name, Thread::Priority prio = Thread::Normal);
    virtual void cleanup();
protected:
    virtual void run();
    void notifyTerminate();
    YBTSThreadOwner* m_owner;
};

class YBTSThreadOwner : public YBTSDebug
{
public:
    inline YBTSThreadOwner()
	: m_thread(0), m_mutex(0)
	{}
    void threadTerminated(YBTSThread* th);
    virtual void processLoop() = 0;

protected:
    bool startThread(const char* name, Thread::Priority prio = Thread::Normal);
    void stopThread();
    inline void initThreadOwner(Mutex* mutex)
	{ m_mutex = mutex; }

    YBTSThread* m_thread;
    String m_name;
    Mutex* m_mutex;
};

class YBTSMessage : public GenObject, public YBTSConnIdHolder
{
public:
    inline YBTSMessage(uint8_t pri = 0, uint8_t info = 0, uint16_t cid = 0,
	XmlElement* xml = 0)
        : YBTSConnIdHolder(cid),
        m_primitive(pri), m_info(info), m_xml(xml), m_error(false)
	{}
    ~YBTSMessage()
	{ TelEngine::destruct(m_xml); }
    inline const char* name() const
	{ return lookup(m_primitive,s_priName); }
    inline uint16_t primitive() const
	{ return m_primitive; }
    inline uint16_t info() const
	{ return m_info; }
    inline bool hasConnId() const
	{ return 0 == (m_primitive & 0x80); }
    inline const XmlElement* xml() const
	{ return m_xml; }
    inline bool error() const
	{ return m_error; }
    // Parse message. Return 0 on failure
    static YBTSMessage* parse(YBTSSignalling* receiver, uint8_t* data, unsigned int len);
    // Build a message
    static bool build(YBTSSignalling* sender, DataBlock& buf, const YBTSMessage& msg);

    static const TokenDict s_priName[];

protected:
    uint8_t m_primitive;
    uint8_t m_info;
    XmlElement* m_xml;
    bool m_error;                        // Encode/decode error flag
};

class YBTSDataSource : public DataSource, public YBTSConnIdHolder
{
public:
    inline YBTSDataSource(const String& format, unsigned int connId, YBTSMedia* media)
        : DataSource(format), YBTSConnIdHolder(connId), m_media(media)
	{}
protected:
    virtual void destroyed();
    YBTSMedia* m_media;
};

class YBTSDataConsumer : public DataConsumer, public YBTSConnIdHolder
{
public:
    inline YBTSDataConsumer(const String& format, unsigned int connId, YBTSMedia* media)
        : DataConsumer(format), YBTSConnIdHolder(connId), m_media(media)
	{}
    virtual unsigned long Consume(const DataBlock& data, unsigned long tStamp,
	unsigned long flags);
protected:
    YBTSMedia* m_media;
};

class YBTSTransport : public GenObject, public YBTSDebug
{
    friend class YBTSLog;
    friend class YBTSSignalling;
    friend class YBTSMedia;
public:
    inline YBTSTransport()
	: m_maxRead(0)
	{}
    ~YBTSTransport()
	{ resetTransport(); }
    inline const DataBlock& readBuf() const
	{ return m_readBuf; }
    inline HANDLE detachRemote()
	{ return m_remoteSocket.detach(); }

protected:
    bool send(const void* buf, unsigned int len);
    inline bool send(const DataBlock& data)
	{ return send(data.data(),data.length()); }
    // Read socket data. Return 0: nothing read, >1: read data, negative: fatal error
    int recv();
    bool initTransport(bool stream, unsigned int buflen, bool reserveNull);
    void resetTransport();

    Socket m_socket;
    Socket m_readSocket;
    Socket m_writeSocket;
    Socket m_remoteSocket;
    DataBlock m_readBuf;
    unsigned int m_maxRead;
};

// Holds local area id
class YBTSLAI
{
public:
    inline YBTSLAI(const char* lai = 0)
	: m_lai(lai)
	{}
    YBTSLAI(const XmlElement& xml);
    inline YBTSLAI(const YBTSLAI& other)
	{ *this = other; }
    inline const String& lai() const
	{ return m_lai; }
    inline void set(const char* mnc, const char* mcc, const char* lac) {
	    reset();
	    m_mnc_mcc << mnc << mcc;
	    m_lac = lac;
	    m_lai << m_mnc_mcc << "_" << m_lac;
	}
    inline void reset() {
	    m_mnc_mcc.clear();
	    m_lac.clear();
	    m_lai.clear();
	}
    XmlElement* build() const;
    inline bool operator==(const YBTSLAI& other)
	{ return m_lai == other.m_lai; }
    inline bool operator!=(const YBTSLAI& other)
	{ return m_lai != other.m_lai; }
    inline const YBTSLAI& operator=(const YBTSLAI& other) {
	    m_mnc_mcc = other.m_mnc_mcc;
	    m_lac = other.m_lac;
	    m_lai = other.m_lai;
	    return *this;
	}
protected:
    String m_mnc_mcc;                    // Concatenated MNC + MCC
    String m_lac;                        // LAC
    String m_lai;                        // Concatenated mnc_mcc_lac
};

// A logical connection
// UE retrieve/set methods are not thread safe
class YBTSConn : public RefObject, public Mutex, public YBTSConnIdHolder
{
    friend class YBTSSignalling;
    friend class YBTSMM;
public:
    inline YBTSUE* ue()
	{ return m_ue; }
    // Send an L3 connection related message
    bool sendL3(XmlElement* xml, uint8_t info = 0);
protected:
    YBTSConn(YBTSSignalling* owner, uint16_t connId);
    // Set connection UE. Return false if requested to change an existing, different UE
    bool setUE(YBTSUE* ue);

    YBTSSignalling* m_owner;
    RefPointer<YBTSUE> m_ue;
};

class YBTSLog : public GenObject, public DebugEnabler, public Mutex,
    public YBTSThreadOwner
{
public:
    YBTSLog();
    inline YBTSTransport& transport()
	{ return m_transport; }
    bool start();
    void stop();
    // Read socket
    virtual void processLoop();

protected:
    String m_name;
    YBTSTransport m_transport;
};

class YBTSSignalling : public GenObject, public DebugEnabler, public Mutex,
    public YBTSThreadOwner
{
public:
    enum State {
	Idle,
	Started,
	WaitHandshake,
	Running,
	Closing
    };
    enum Result {
	Ok = 0,
	Error = 1,
	FatalError = 2
    };
    YBTSSignalling();
    inline int state() const
	{ return m_state; }
    inline YBTSTransport& transport()
	{ return m_transport; }
    inline GSML3Codec& codec()
	{ return m_codec; }
    inline const YBTSLAI& lai() const
	{ return m_lai; }
    inline void waitHandshake() {
	    Lock lck(this);
	    changeState(WaitHandshake);
	}
    inline bool shouldCheckTimers()
	{ return m_state == Running || m_state == WaitHandshake; }
    int checkTimers(const Time& time = Time());
    // Send a message
    inline bool send(YBTSMessage& msg) {
	    Lock lck(this);
	    return sendInternal(msg);
	}
    // Send an L3 connection related message
    inline bool sendL3Conn(uint16_t connId, XmlElement* xml, uint8_t info = 0) {
	    YBTSMessage m(YBTS::SigL3Message,info,connId,xml);
	    return send(m);
	}
    bool start();
    void stop();
    // Drop a connection
    void dropConn(uint16_t connId, bool notifyPeer);
    inline void dropConn(YBTSConn* conn, bool notifyPeer) {
	    if (conn)
		dropConn(conn->connId(),notifyPeer);
	}
    // Read socket. Parse and handle received data
    virtual void processLoop();
    void init(Configuration& cfg);

protected:
    bool sendInternal(YBTSMessage& msg);
    bool findConnInternal(RefPointer<YBTSConn>& conn, uint16_t connId, bool create);
    inline bool findConn(RefPointer<YBTSConn>& conn, uint16_t connId, bool create) {
	    Lock lck(m_connsMutex);
	    return findConnInternal(conn,connId,create);
	}
    void changeState(int newStat);
    int handlePDU(YBTSMessage& msg);
    int handleHandshake(YBTSMessage& msg);
    void printMsg(YBTSMessage& msg, bool recv);
    void setTimer(uint64_t& dest, const char* name, unsigned int intervalMs,
	uint64_t timeUs = Time::now()) {
	     if (intervalMs) {
		dest = timeUs + (uint64_t)intervalMs * 1000;
		XDebug(this,DebugAll,"%s scheduled in %ums [%p]",name,intervalMs,this);
	    }
	    else if (dest) {
		dest = 0;
		XDebug(this,DebugAll,"%s reset [%p]",name,this);
	    }
	}
    inline void setToutHandshake(uint64_t timeUs = Time::now())
	{ setTimer(m_timeout,"Timeout",m_hkIntervalMs,timeUs); }
    inline void setToutHeartbeat(uint64_t timeUs = Time::now())
	{ setTimer(m_timeout,"Timeout",m_hbIntervalMs,timeUs); }
    inline void setHeartbeatTime(uint64_t timeUs = Time::now())
	{ setTimer(m_hbTime,"Heartbeat",m_hbIntervalMs,timeUs); }
    inline void resetHeartbeatTime()
	{ setTimer(m_hbTime,"Heartbeat",0,0); }

    String m_name;
    int m_state;
    bool m_printMsg;
    int m_printMsgData;
    YBTSTransport m_transport;
    GSML3Codec m_codec;
    YBTSLAI m_lai;
    Mutex m_connsMutex;
    ObjList m_conns;
    uint64_t m_timeout;
    uint64_t m_hbTime;
    unsigned int m_hkIntervalMs;         // Time (in miliseconds) to wait for handshake
    unsigned int m_hbIntervalMs;         // Heartbeat interval in miliseconds
};

class YBTSMedia : public GenObject, public DebugEnabler, public Mutex,
    public YBTSThreadOwner
{
    friend class YBTSDataConsumer;
public:
    YBTSMedia();
    inline YBTSTransport& transport()
	{ return m_transport; }
    YBTSDataSource* buildSource(unsigned int connId);
    YBTSDataConsumer* buildConsumer(unsigned int connId);
    void addSource(YBTSDataSource* src);
    void removeSource(YBTSDataSource* src);
    void cleanup(bool final);
    bool start();
    void stop();
    // Read socket
    virtual void processLoop();

protected:
    inline void consume(const DataBlock& data, uint16_t connId) {
	    if (!data.length())
		return;
	    uint8_t cid[2] = {(uint8_t)(connId >> 8),(uint8_t)connId};
	    DataBlock tmp(cid,2);
	    tmp += data;
	    m_transport.send(tmp);
	}
    // Find a source object by connection id, return referrenced pointer
    YBTSDataSource* find(unsigned int connId);
    YBTSTransport m_transport;

    String m_name;
    Mutex m_srcMutex;
    ObjList m_sources;
};

class YBTSUE : public RefObject, public Mutex, public YBTSConnIdHolder
{
    friend class YBTSMM;
public:
    inline const String& imsi() const
	{ return m_imsi; }
    inline const String& tmsi() const
	{ return m_tmsi; }

protected:
    inline YBTSUE(const char* imsi, const char* tmsi)
	: Mutex(false,"YBTSUE"),
	m_registered(false), m_imsi(imsi), m_tmsi(tmsi)
	{}
    bool m_registered;
    String m_imsi;
    String m_tmsi;
};

class YBTSMM : public GenObject, public DebugEnabler, public Mutex
{
public:
    YBTSMM(unsigned int hashLen);
    ~YBTSMM();
    inline XmlElement* buildMM()
	{ return new XmlElement("MM"); }
    inline XmlElement* buildMM(XmlElement*& ch, const char* tag) {
	    XmlElement* mm = buildMM();
	    ch = static_cast<XmlElement*>(mm->addChildSafe(new XmlElement(tag)));
	    return mm;
	}
    void handlePDU(YBTSMessage& msg, YBTSConn* conn);

protected:
    void handleLocationUpdate(YBTSMessage& msg, const XmlElement& xml, YBTSConn* conn);
    void handleCMServiceRequest(YBTSMessage& msg, const XmlElement& xml, YBTSConn* conn);
    void sendLocationUpdateReject(YBTSMessage& msg, YBTSConn* conn, uint8_t cause);
    void sendCMServiceRsp(YBTSMessage& msg, YBTSConn* conn, uint8_t cause = 0);
    // Find UE by TMSI
    void findUEByTMSISafe(RefPointer<YBTSUE>& ue, const String& tmsi);
    // Find UE by IMSI. Create it if not found
    void getUEByIMSISafe(RefPointer<YBTSUE>& ue, const String& imsi);
    inline unsigned int hashList(const String& str)
	{ return str.hash() % m_ueHashLen; }
    // Get IMSI/TMSI from request
    uint8_t getMobileIdentTIMSI(YBTSMessage& m, const XmlElement& request,
	const XmlElement& identXml, const String*& ident, bool& isTMSI);
    // Set UE for a connection
    uint8_t setConnUE(YBTSConn& conn, YBTSUE* ue, const XmlElement& req,
	bool& dropConn);

    String m_name;
    Mutex m_ueMutex;
    uint16_t m_tmsiIndex;                // Index used to generate TMSI
    ObjList* m_ueIMSI;                   // List of UEs grouped by IMSI
    ObjList* m_ueTMSI;                   // List of UEs grouped by TMSI
    unsigned int m_ueHashLen;            // Length of UE lists
};

class YBTSCallDesc : public String, public YBTSConnIdHolder
{
public:
    // Call state
    // ETSI TS 100 940, Section 5.1.2.2
    // Section 10.5.4.6: call state IE
    enum State {
	Null = 0,
	CallInitiated = 1,               // N1: Call initiated
	ConnPending = 2,                 // N0.1: MT call, waiting for MM connection
	CallProceeding = 3,              // N3: MO call proceeding
	CallDelivered = 4,               // N4: Call delivered
	CallPresent = 5,                 // N6: Call present
	CallReceived = 6,                // N7: Call received
	ConnectReq = 8,                  // N8: Connect request
	CallConfirmed = 9,               // N9: MT call confirmed
	Active = 10,                     // N10: Active (answered)
	Disconnect = 12,                 // N12: Disconnect indication
	Release = 19,                    // N19: Release request
	Connect = 28,                    // N28: Connect indication
    };
    // Incoming
    YBTSCallDesc(YBTSChan* chan, const XmlElement& xml, bool regular);
    inline const char* stateName() const
	{ return stateName(m_state); }
    inline const char* reason()
	{ return m_reason.safe("normal"); }
    void proceeding();
    void progressing(XmlElement* indicator);
    void alert(XmlElement* indicator);
    void connect(XmlElement* indicator);
    void hangup();
    void sendStatus(const char* cause);
    inline void sendWrongState()
	{ sendStatus("wrong-state-message"); }
    inline void release() {
	    changeState(Release);
	    m_relSent++;
	    sendGSMRel(true,*this,reason(),connId());
	}
    inline void releaseComplete() {
	    changeState(Null);
	    sendGSMRel(false,*this,reason(),connId());
	}
    void sendCC(const String& tag, XmlElement* c1 = 0, XmlElement* c2 = 0);
    void changeState(int newState);
    inline void setTimeout(unsigned int intervalMs, const Time& time = Time())
	{ m_timeout = time + (uint64_t)intervalMs * 1000; }
    // Send CC REL or RLC
    static void sendGSMRel(bool rel, const String& callRef, const char* reason,
	uint16_t connId);
    static inline const char* stateName(int stat)
	{ return lookup(stat,s_stateName); }
    static const TokenDict s_stateName[];

    int m_state;
    bool m_incoming;
    bool m_regular;
    uint64_t m_timeout;
    uint8_t m_relSent;
    YBTSChan* m_chan;
    String m_reason;
    String m_called;
};

class YBTSChan : public Channel, public YBTSConnIdHolder
{
public:
    // Incoming
    YBTSChan(YBTSConn* conn);
    inline YBTSConn* conn() const
	{ return m_conn; }
    // Init incoming chan. Return false to destruct the channel
    bool initIncoming(const XmlElement& xml, bool regular);
    // Handle CC messages
    bool handleCC(const XmlElement& xml);
    // Connection released notification
    void connReleased();
    // BTS stopping notification
    inline void stop()
	{ hangup("interworking"); }

protected:
    inline YBTSCallDesc* firstCall() {
	    ObjList* o = m_calls.skipNull();
	    return o ? static_cast<YBTSCallDesc*>(o->get()) : 0;
	}
    void handleSetup(const XmlElement& xml, bool regular);
    void hangup(const char* reason = 0, bool final = false);
    inline void setReason(const char* reason, Mutex* mtx = 0) {
	    if (!reason)
		return;
	    Lock lck(mtx);
	    m_reason = reason;
	}
    inline void setTout(uint64_t tout) {
	    if (tout && (!m_tout || m_tout > tout)) {
		m_tout = tout;
		m_haveTout = true;
	    }
	}
    virtual void disconnected(bool final, const char *reason);
    virtual bool callRouted(Message& msg);
    virtual void callAccept(Message& msg);
    virtual void callRejected(const char* error, const char* reason, const Message* msg);
    virtual bool msgProgress(Message& msg);
    virtual bool msgRinging(Message& msg);
    virtual bool msgAnswered(Message& msg);
    virtual void checkTimers(Message& msg, const Time& tmr);
#if 0
    virtual bool msgTone(Message& msg, const char* tone);
    virtual bool msgText(Message& msg, const char* text);
    virtual bool msgDrop(Message& msg, const char* reason);
    virtual bool msgUpdate(Message& msg);
#endif
    virtual void destroyed();

    Mutex m_mutex;
    RefPointer<YBTSConn> m_conn;
    ObjList m_calls;
    String m_reason;
    bool m_hungup;
    bool m_haveTout;
    uint64_t m_tout;
};

class YBTSDriver : public Driver
{
public:
    enum Relay {
	Stop = Private,
	Start = Private << 1,
    };
    enum State {
	Idle = 0,
	Starting,                        // The link with peer is starting
	WaitHandshake,                   // The peer was started, waiting for handshake
	Running,                         // Peer hadshake done
	RadioUp
    };
    YBTSDriver();
    ~YBTSDriver();
    inline int state() const
	{ return m_state; }
    inline const char* stateName() const
	{ return lookup(m_state,s_stateName); }
    inline YBTSMedia* media()
	{ return m_media; }
    inline YBTSSignalling* signalling()
	{ return m_signalling; }
    inline YBTSMM* mm()
	{ return m_mm; }
    inline XmlElement* buildCC()
	{ return new XmlElement("CC"); }
    XmlElement* buildCC(XmlElement*& ch, const char* tag, const char* callRef);
    // Handle call control messages
    void handleCC(YBTSMessage& m, YBTSConn* conn);
    // Add a pending (wait termination) call
    void addTerminatedCall(YBTSCallDesc* call);
    // Check terminated calls timeout
    void checkTerminatedCalls(const Time& time = Time());
    // Clear terminated calls for a given connection
    void removeTerminatedCall(uint16_t connId);
    // Handshake done notification. Return false if state is invalid
    bool handshakeDone();
    void restart(unsigned int stopIntervalMs = 0);
    void stopNoRestart();
    inline bool findChan(uint16_t connId, RefPointer<YBTSChan>& chan) {
	    Lock lck(this);
	    chan = findChanConnId(connId);
	    return chan != 0;
	}

    static const TokenDict s_stateName[];

protected:
    void start();
    inline void startIdle() {
	    Lock lck(m_stateMutex);
	    if (!m_engineStart || m_state != Idle || Engine::exiting())
		return;
	    lck.drop();
	    start();
	}
    void stop();
    bool startPeer();
    void stopPeer();
    bool handleEngineStop(Message& msg);
    YBTSChan* findChanConnId(uint16_t connId);
    void changeState(int newStat);
    void setRestart(int resFlag, bool on = true);
    void checkStop(const Time& time);
    void checkRestart(const Time& time);
    inline void setStop(unsigned int stopIntervalMs) {
	    m_stop = true;
	    if (m_stopTime)
		return;
	    m_stopTime = Time::now() + (uint64_t)stopIntervalMs * 1000;
	    Debug(this,DebugAll,"Scheduled stop in %ums",stopIntervalMs);
	}
    virtual void initialize();
    virtual bool msgExecute(Message& msg, String& dest);
    virtual bool received(Message& msg, int id);

    int m_state;
    Mutex m_stateMutex;
    pid_t m_peerPid;                     // Peer PID
    bool m_error;                        // Error flag, ignore restart
    bool m_stop;                         // Stop flag
    uint64_t m_stopTime;                 // Stop time
    bool m_restart;                      // Restart flag
    uint64_t m_restartTime;              // Restart time
    YBTSLog* m_log;                      // Log
    YBTSMedia* m_media;                  // Media
    YBTSSignalling* m_signalling;        // Signalling
    YBTSMM* m_mm;                        // Mobility management
    ObjList m_terminatedCalls;           // Terminated calls list
    bool m_haveCalls;                    // Empty terminated calls list flag
    bool m_engineStart;
    unsigned int m_engineStop;
};

INIT_PLUGIN(YBTSDriver);
static Mutex s_globalMutex(false,"YBTSGlobal");
static YBTSLAI s_lai;
static String s_format = "gsm";          // Format to use
static String s_peerPath = "path_to_app_to_run"; // Peer path
static unsigned int s_bufLenLog = 4096;  // Read buffer length for log interface
static unsigned int s_bufLenSign = 1024; // Read buffer length for signalling interface
static unsigned int s_bufLenMedia = 1024;// Read buffer length for media interface
static unsigned int s_restartMs = YBTS_RESTART_DEF; // Time (in miliseconds) to wait for restart
// Call Control Timers (in milliseconds)
// ETSI TS 100 940 Section 11.3
static unsigned int s_t305 = 30000;      // DISC sent, no inband tone available
                                         // Stop when recv REL/DISC, send REL on expire
static unsigned int s_t308 = 5000;       // REL sent (operator specific, no default in spec)
                                         // First expire: re-send REL, second expire: release call
static unsigned int s_t313 = 5000;       // Send Connect, expect Connect Ack, clear call on expire
// Strings
static const String s_locAreaIdent = "LAI";
static const String s_MNC_MCC = "MNC_MCC";
static const String s_LAC = "LAC";
static const String s_mobileIdent = "MobileIdentity";
static const String s_imsi = "IMSI";
static const String s_tmsi = "TMSI";
static const String s_cause = "Cause";
static const String s_cmServType = "CMServiceType";
static const String s_cmMOCall = "mocall";
static const String s_ccSetup = "Setup";
static const String s_ccEmergency = "EmergencySetup";
static const String s_ccProceeding = "CallProceeding";
static const String s_ccProgress = "Progress";
static const String s_ccAlerting = "Alerting";
static const String s_ccConnect = "Connect";
static const String s_ccConnectAck = "ConnectAck";
static const String s_ccDisc = "Disconnect";
static const String s_ccRel = "Release";
static const String s_ccRlc = "ReleaseComplete";
static const String s_ccStatusEnq = "StatusEnquiry";
static const String s_ccStatus = "Status";
static const String s_ccCallRef = "TransactionIdentifier";
static const String s_ccCalled = "CalledPartyBCDNumber";
static const String s_ccCallState = "CallState";
//static const String s_ccFacility = "Facility";
static const String s_ccProgressInd = "ProgressIndicator";
//static const String s_ccUserUser = "UserUser";

#define YBTS_MAKENAME(x) {#x, x}
#define YBTS_XML_GETCHILD_PTR_CONTINUE(x,tag,ptr) \
    if (x->getTag() == tag) { \
	ptr = x; \
	continue; \
    }
#define YBTS_XML_GETCHILDTEXT_CHOICE_RETURN(x,tag,ptr) \
    if (x->getTag() == tag) { \
	ptr = &x->getText(); \
	return; \
    }
#define YBTS_XML_GETCHILDTEXT_CHOICE_CONTINUE(x,tag,ptr) \
    if (x->getTag() == tag) { \
	ptr = &x->getText(); \
	continue; \
    }

const TokenDict YBTSMessage::s_priName[] =
{
    {"L3Message",            YBTS::SigL3Message},
    {"ConnRelease",          YBTS::SigConnRelease},
    {"Handshake",            YBTS::SigHandshake},
    {"Heartbeat",            YBTS::SigHeartbeat},
    {0,0}
};

const TokenDict YBTSCallDesc::s_stateName[] =
{
    YBTS_MAKENAME(Null),
    YBTS_MAKENAME(CallInitiated),
    YBTS_MAKENAME(ConnPending),
    YBTS_MAKENAME(CallProceeding),
    YBTS_MAKENAME(CallDelivered),
    YBTS_MAKENAME(CallPresent),
    YBTS_MAKENAME(CallReceived),
    YBTS_MAKENAME(ConnectReq),
    YBTS_MAKENAME(CallConfirmed),
    YBTS_MAKENAME(Active),
    YBTS_MAKENAME(Disconnect),
    YBTS_MAKENAME(Release),
    YBTS_MAKENAME(Connect),
    {0,0}
};

const TokenDict YBTSDriver::s_stateName[] =
{
    YBTS_MAKENAME(Idle),
    YBTS_MAKENAME(Starting),
    YBTS_MAKENAME(WaitHandshake),
    YBTS_MAKENAME(Running),
    YBTS_MAKENAME(RadioUp),
    {0,0}
};

// Call termination cause
// ETSI TS 100 940, Section 10.5.4.11
static const TokenDict s_ccErrorDict[] = {
  {"unallocated",                     1}, // Unallocated (unassigned) number
  {"noroute",                         3}, // No route to destination
  {"channel-unacceptable",            6}, // Channel unacceptable
  {"forbidden",                       8}, // Operator determined barring
  {"normal-clearing",                16}, // Normal Clearing
  {"busy",                           17}, // User busy
  {"noresponse",                     18}, // No user responding
  {"noanswer",                       19}, // No answer from user (user alerted)
  {"rejected",                       21}, // Call Rejected
  {"moved",                          22}, // Number changed
  {"preemption",                     25}, // Preemption
  {"answered",                       26}, // Non-selected user clearing (answered elsewhere)
  {"out-of-order",                   27}, // Destination out of order
  {"invalid-number",                 28}, // Invalid number format
  {"facility-rejected",              29}, // Facility rejected
  {"status-enquiry-rsp",             30}, // Response to STATUS ENQUIRY
  {"normal",                         31}, // Normal, unspecified
  {"congestion",                     34}, // No circuit/channel available
  {"net-out-of-order",               38}, // Network out of order
  {"noconn",                         41},
  {"temporary-failure",              41}, // Temporary failure
  {"congestion",                     42}, // Switching equipment congestion
  {"switch-congestion",              42},
  {"access-info-discarded",          43}, // Access information discarded
  {"channel-unavailable",            44}, // Requested channel not available
  {"noresource",                     47}, // Resource unavailable, unspecified
  {"qos-unavailable",                49}, // Quality of service unavailable
  {"facility-not-subscribed",        50}, // Requested facility not subscribed
  {"forbidden-in",                   55}, // Incoming call barred within CUG
  {"bearer-cap-not-auth",            57}, // Bearer capability not authorized
  {"bearer-cap-not-available",       58}, // Bearer capability not presently available
  {"nomedia",                        58},
  {"service-unavailable",            63}, // Service or option not available
  {"bearer-cap-not-implemented",     65}, // Bearer capability not implemented
//TODO: {"",     68}, // ACM equal to or greater then ACMmax
  {"restrict-bearer-cap-avail",      70}, // Only restricted digital information bearer capability is available
  {"service-not-implemented",        79}, // Service or option not implemented, unspecified
  {"invalid-callref",                81}, // Invalid transaction identifier value
  {"not-subscribed",                 87}, // User not member of CUG
  {"incompatible-dest",              88}, // Incompatible destination
  {"invalid-transit-net",            91}, // Invalid transit network selection
  {"invalid-message",                95}, // Invalid message, unspecified
  {"invalid-ie",                     96}, // Invalid information element contents
  {"unknown-message",                97}, // Message type non-existent or not implemented
  {"wrong-state-message",            98}, // Message not compatible with call state
  {"unknown-ie",                     99}, // Information element non-existent or not implemented
  {"conditional-ie",                100}, // Conditional IE error
  {"wrong-proto-message",           101}, // Message not compatible with protocol state
  {"timeout",                       102}, // Recovery on timer expiry
  {"protocol-error",                111}, // Protocol error, unspecified
  {"interworking",                  127}, // Interworking, unspecified
  {0,0}
};

// safely retrieve a global string
static inline void getGlobalStr(String& buf, const String& value)
{
    Lock lck(s_globalMutex);
    buf = value;
}

// Add a socket error to a buffer
static inline String& addLastError(String& buf, int error, const char* prefix = ": ")
{
    String tmp;
    Thread::errorString(tmp,error);
    buf << prefix << error << " " << tmp;
    return buf;
}

// Calculate a number of thread idle intervals from a period
// Return a non 0 value
static inline unsigned int threadIdleIntervals(unsigned int intervalMs)
{
    intervalMs = (unsigned int)(intervalMs / Thread::idleMsec());
    return intervalMs ? intervalMs : 1;
}

static inline const NamedList& safeSect(Configuration& cfg, const String& name)
{
    const NamedList* sect = cfg.getSection(name);
    return sect ? *sect : NamedList::empty();
}

// Check if a string is composed of digits in interval 0..9
static inline bool isDigits09(const String& str)
{
    if (!str)
	return false;
    for (const char* s = str.c_str(); *s; s++)
	if (*s < '0' || *s > '9')
	    return false;
    return true;
}

// Retrieve XML children
static inline void findXmlChildren(const XmlElement& xml,
    XmlElement*& ptr1, const String& tag1,
    XmlElement*& ptr2, const String& tag2)
{
    for (const ObjList* o = xml.getChildren().skipNull(); o; o = o->skipNext()) {
	XmlElement* x = static_cast<XmlChild*>(o->get())->xmlElement();
	if (!x)
	    continue;
	YBTS_XML_GETCHILD_PTR_CONTINUE(x,tag1,ptr1);
	YBTS_XML_GETCHILD_PTR_CONTINUE(x,tag2,ptr2);
    }
}

// Retrieve the text of an xml child
// Use this function for choice XML
static inline void getXmlChildTextChoice(const XmlElement& xml,
    const String*& ptr1, const String& tag1,
    const String*& ptr2, const String& tag2)
{
    for (const ObjList* o = xml.getChildren().skipNull(); o; o = o->skipNext()) {
	XmlElement* x = static_cast<XmlChild*>(o->get())->xmlElement();
	if (!x)
	    continue;
	YBTS_XML_GETCHILDTEXT_CHOICE_RETURN(x,tag1,ptr1);
	YBTS_XML_GETCHILDTEXT_CHOICE_RETURN(x,tag2,ptr2);
    }
}

// Retrieve the text of a requested xml children
static inline void getXmlChildTextAll(const XmlElement& xml,
    const String*& ptr1, const String& tag1,
    const String*& ptr2, const String& tag2)
{
    for (const ObjList* o = xml.getChildren().skipNull(); o; o = o->skipNext()) {
	XmlElement* x = static_cast<XmlChild*>(o->get())->xmlElement();
	if (!x)
	    continue;
	YBTS_XML_GETCHILDTEXT_CHOICE_CONTINUE(x,tag1,ptr1);
	YBTS_XML_GETCHILDTEXT_CHOICE_CONTINUE(x,tag2,ptr2);
    }
}

// Build an xml element with a child
static inline XmlElement* buildXmlWithChild(const String& tag, const String& childTag,
    const char* childText = 0)
{
    XmlElement* xml = new XmlElement(tag);
    xml->addChildSafe(new XmlElement(childTag,childText));
    return xml;
}

// Retrieve IMSI or TMSI from xml
static inline const String* getIdentTIMSI(const XmlElement& xml, bool& isTMSI, bool& found)
{
    const String* imsi = 0;
    const String* tmsi = 0;
    getXmlChildTextChoice(xml,imsi,s_imsi,tmsi,s_tmsi);
    found = (imsi || tmsi);
    isTMSI = (tmsi != 0);
    if (!found)
	return 0;
    if (tmsi)
	return *tmsi ? tmsi : 0;
    return *imsi ? imsi : 0;
}

static inline void getCCCause(String& dest, const XmlElement& xml)
{
    // TODO: check it when codec will be implemented
    dest = xml.childText(s_cause);
}

static inline XmlElement* buildCCCause(const char* cause)
{
    // TODO: check it when codec will be implemented
    if (TelEngine::null(cause))
	cause = "normal";
    return new XmlElement(s_cause,cause);
}

static inline void getCCCallState(String& dest, const XmlElement& xml)
{
    // TODO: check it when codec will be implemented
    dest = xml.childText(s_ccCallState);
}

static inline XmlElement* buildCCCallState(int stat)
{
    // TODO: check it when codec will be implemented
    return new XmlElement(s_ccCallState,String(stat));
}

// Build Progress Indicator IE
static inline XmlElement* buildProgressIndicator(const Message& msg,
    bool earlyMedia = true)
{
    const String& s = msg[YSTRING("progress.indicator")];
    if (s)
	return new XmlElement(s_ccProgressInd,s);
    if (earlyMedia && msg.getBoolValue("earlymedia"))
	return new XmlElement(s_ccProgressInd,"inband");
    return 0;
}


//
// YBTSThread
//
YBTSThread::YBTSThread(YBTSThreadOwner* owner, const char* name, Thread::Priority prio)
    : Thread(name,prio),
    m_owner(owner)
{
}

void YBTSThread::cleanup()
{
    notifyTerminate();
}

void YBTSThread::run()
{
    if (!m_owner)
	return;
    m_owner->processLoop();
    notifyTerminate();
}

void YBTSThread::notifyTerminate()
{
    if (!m_owner)
	return;
    m_owner->threadTerminated(this);
    m_owner = 0;
}


//
// YBTSThreadOwner
//
bool YBTSThreadOwner::startThread(const char* name, Thread::Priority prio)
{
    m_thread = new YBTSThread(this,name,prio);
    if (m_thread->startup()) {
	Debug(m_enabler,DebugAll,"Started worker thread [%p]",m_ptr);
	return true;
    }
    m_thread = 0;
    Alarm(m_enabler,"system",DebugWarn,"Failed to start worker thread [%p]",m_ptr);
    return false;
}

void YBTSThreadOwner::stopThread()
{
    Lock lck(m_mutex);
    if (!m_thread)
	return;
    DDebug(m_enabler,DebugAll,"Stopping worker thread [%p]",m_ptr);
    m_thread->cancel();
    while (m_thread) {
	lck.drop();
	Thread::idle();
	lck.acquire(m_mutex);
    }
}

void YBTSThreadOwner::threadTerminated(YBTSThread* th)
{
    Lock lck(m_mutex);
    if (!(m_thread && th && m_thread == th))
	return;
    m_thread = 0;
    Debug(m_enabler,DebugAll,"Worker thread terminated [%p]",m_ptr);
}


//
// YBTSMessage
//
// Utility used in YBTSMessage::parse()
static inline void decodeMsg(GSML3Codec& codec, uint8_t* data, unsigned int len,
    XmlElement*& xml, String& reason)
{
    unsigned int e = codec.decode(data,len,xml);
    if (e)
	reason << "Codec error " << e;
}

// Parse message. Return 0 on failure
YBTSMessage* YBTSMessage::parse(YBTSSignalling* recv, uint8_t* data, unsigned int len)
{
    if (!len)
	return 0;
    if (len < 2) {
	Debug(recv,DebugNote,"Received short message length %u [%p]",len,recv);
	return 0;
    }
    YBTSMessage* m = new YBTSMessage;
    m->m_primitive = *data++;
    m->m_info = *data++;
    len -= 2;
    if (m->hasConnId()) {
	if (len < 2) {
	    Debug(recv,DebugNote,
		"Received message %u (%s) with missing connection id [%p]",
		m->primitive(),m->name(),recv);
	    TelEngine::destruct(m);
	    return 0;
	}
	m->setConnId(ntohs(*(uint16_t*)data));
	data += 2;
	len -= 2;
    }
    String reason;
    switch (m->primitive()) {
	case YBTS::SigL3Message:
	    decodeMsg(recv->codec(),data,len,m->m_xml,reason);
	    break;
	case YBTS::SigHandshake:
	case YBTS::SigHeartbeat:
	    break;
	default:
	    reason = "No decoder";
    }
    if (reason) {
	Debug(recv,DebugNote,"Failed to parse %u (%s): %s [%p]",
	    m->primitive(),m->name(),reason.c_str(),recv);
	m->m_error = true;
    }
    return m;
}

// Utility used in YBTSMessage::parse()
static inline bool encodeMsg(GSML3Codec& codec, const YBTSMessage& msg, DataBlock& buf,
    String& reason)
{
    if (msg.xml()) {
	unsigned int e = codec.encode(msg.xml(),buf);
	if (!e)
	    return true;
	reason << "Codec error " << e;
    }
    else
	reason = "Empty XML";
    return false;
}

// Build a message
bool YBTSMessage::build(YBTSSignalling* sender, DataBlock& buf, const YBTSMessage& msg)
{
    uint8_t b[4] = {msg.primitive(),msg.info()};
    if (msg.hasConnId()) {
	uint8_t* p = b;
	*(uint16_t*)(p + 2) = htons(msg.connId());
	buf.append(b,4);
    }
    else
	buf.append(b,2);
    String reason;
    switch (msg.primitive()) {
	case YBTS::SigL3Message:
	    if (encodeMsg(sender->codec(),msg,buf,reason))
		return true;
	    break;
	case YBTS::SigHeartbeat:
	case YBTS::SigConnRelease:
	case YBTS::SigHandshake:
	    return true;
	default:
	    reason = "No encoder";
    }
    Debug(sender,DebugNote,"Failed to build %s (%u): %s [%p]",
	msg.name(),msg.primitive(),reason.c_str(),sender);
    return false;
}


//
// YBTSDataSource
//
void YBTSDataSource::destroyed()
{
    if (m_media)
	m_media->removeSource(this);
    DataSource::destroyed();
}


//
// YBTSDataConsumer
//
unsigned long YBTSDataConsumer::Consume(const DataBlock& data, unsigned long tStamp,
    unsigned long flags)
{
    if (m_media)
	m_media->consume(data,connId());
    return invalidStamp();
}


//
// YBTSTransport
//
bool YBTSTransport::send(const void* buf, unsigned int len)
{
    if (!len)
	return true;
    int wr = m_writeSocket.send(buf,len);
    if (wr == (int)len)
	return true;
    if (wr >= 0)
	Debug(m_enabler,DebugNote,"Sent %d/%u [%p]",wr,len,m_ptr);
    else if (!m_writeSocket.canRetry()) {
	String tmp;
	addLastError(tmp,m_writeSocket.error());
	Alarm(m_enabler,"socket",DebugWarn,"Socket send error%s [%p]",
	    tmp.c_str(),m_ptr);
    }
    return false;
}

// Read socket data. Return 0: nothing read, >1: read data, negative: fatal error
int YBTSTransport::recv()
{
    if (!m_readSocket.valid())
	return 0;
    uint8_t* buf = (uint8_t*)m_readBuf.data();
    int rd = m_readSocket.recv(buf,m_maxRead);
    if (rd >= 0) {
	if (rd) {
	    if (m_maxRead < m_readBuf.length())
		buf[rd] = 0;
#ifdef XDEBUG
	    String tmp;
	    tmp.hexify(buf,rd,' ');
	    Debug(m_enabler,DebugAll,"Read %d bytes: %s [%p]",rd,tmp.c_str(),m_ptr);
#endif
	}
	return rd;
    }
    if (m_readSocket.canRetry())
	return 0;
    String tmp;
    addLastError(tmp,m_readSocket.error());
    Alarm(m_enabler,"socket",DebugWarn,"Socket read error%s [%p]",
	tmp.c_str(),m_ptr);
    return -1;
}

bool YBTSTransport::initTransport(bool stream, unsigned int buflen, bool reserveNull)
{
    resetTransport();
    String error;
#ifdef PF_UNIX
    SOCKET pair[2];
    if (::socketpair(PF_UNIX,stream ? SOCK_STREAM : SOCK_DGRAM,0,pair) == 0) {
	m_socket.attach(pair[0]);
	m_remoteSocket.attach(pair[1]);
	if (m_socket.setBlocking(false)) {
	    m_readSocket.attach(m_socket.handle());
	    m_writeSocket.attach(m_socket.handle());
	    m_readBuf.assign(0,reserveNull ? buflen + 1 : buflen);
	    m_maxRead = reserveNull ? m_readBuf.length() - 1 : m_readBuf.length();
	    return true;
	}
	error << "Failed to set non blocking mode";
        addLastError(error,m_socket.error());
    }
    else {
	error << "Failed to create pair";
        addLastError(error,errno);
    }
#else
    error = "UNIX sockets not supported";
#endif
    Alarm(m_enabler,"socket",DebugWarn,"%s [%p]",error.c_str(),m_ptr);
    return false;
}

void YBTSTransport::resetTransport()
{
    m_socket.terminate();
    m_readSocket.detach();
    m_writeSocket.detach();
    m_remoteSocket.terminate();
}


//
// YBTSLAI
//
YBTSLAI::YBTSLAI(const XmlElement& xml)
{
    const String* mnc_mcc = &String::empty();
    const String* lac = &String::empty();
    getXmlChildTextAll(xml,mnc_mcc,s_MNC_MCC,lac,s_LAC);
    m_mnc_mcc = *mnc_mcc;
    m_lac = *lac;
    m_lai << m_mnc_mcc << "_" << m_lac;
}

XmlElement* YBTSLAI::build() const
{
    XmlElement* xml = new XmlElement(s_locAreaIdent);
    if (m_mnc_mcc)
	xml->addChildSafe(new XmlElement(s_MNC_MCC,m_mnc_mcc));
    if (m_lac)
	xml->addChildSafe(new XmlElement(s_LAC,m_lac));
    return xml;
}


//
// YBTSConn
//
YBTSConn::YBTSConn(YBTSSignalling* owner, uint16_t connId)
    : Mutex(false,"YBTSConn"),
    YBTSConnIdHolder(connId),
    m_owner(owner)
{
}

// Send an L3 connection related message
bool YBTSConn::sendL3(XmlElement* xml, uint8_t info)
{
    return m_owner->sendL3Conn(connId(),xml,info);
}

// Set connection UE. Return false if requested to change an existing, different UE
bool YBTSConn::setUE(YBTSUE* ue)
{
    if (!ue)
	return true;
    if (!m_ue) {
	m_ue = ue;
	Debug(m_owner,DebugAll,
	    "Connection %u set UE (%p) imsi=%s tmsi=%s [%p]",
	    connId(),ue,ue->imsi().c_str(),ue->tmsi().c_str(),this);
	return true;
    }
    if (m_ue == ue)
	return true;
    Debug(m_owner,DebugMild,
	"Can't replace UE on connection %u: existing=(%p,%s,%s) new=(%p,%s,%s) [%p]",
	connId(),(YBTSUE*)m_ue,m_ue->imsi().c_str(),m_ue->tmsi().c_str(),
	ue,ue->imsi().c_str(),ue->tmsi().c_str(),this);
    return false;
}


//
// YBTSLog
//
YBTSLog::YBTSLog()
    : Mutex(false,"YBTSLog")
{
    m_name = "ybts-log";
    debugName(m_name);
    debugChain(&__plugin);
    YBTSThreadOwner::initThreadOwner(this);
    YBTSThreadOwner::setDebugPtr(this,this);
    m_transport.setDebugPtr(this,this);
}

bool YBTSLog::start()
{
    stop();
    while (true) {
	Lock lck(this);
	if (!m_transport.initTransport(false,s_bufLenLog,true))
	    break;
	if (!startThread("YBTSLog"))
	    break;
	Debug(this,DebugInfo,"Started [%p]",this);
	return true;
    }
    stop();
    return false;
}

void YBTSLog::stop()
{
    stopThread();
    Lock lck(this);
    if (!m_transport.m_socket.valid())
	return;
    m_transport.resetTransport();
    Debug(this,DebugInfo,"Stopped [%p]",this);
}

// Read socket
void YBTSLog::processLoop()
{
    while (!Thread::check(false)) {
	int rd = m_transport.recv();
	if (rd > 0) {
	    // TODO: decode data, output the debug message
	    Debug(this,DebugStub,"process recv not implemented");
	    continue;
	}
	if (!rd) {
	    Thread::idle();
	    continue;
	}
	// Socket non retryable error
	__plugin.restart();
	break;
    }
}


//
// YBTSSignalling
//
YBTSSignalling::YBTSSignalling()
    : Mutex(false,"YBTSSignalling"),
    m_state(Idle),
    m_printMsg(true),
    m_printMsgData(1),
    m_codec(0),
    m_connsMutex(false,"YBTSConns"),
    m_timeout(0),
    m_hbTime(0),
    m_hkIntervalMs(YBTS_HK_INTERVAL_DEF),
    m_hbIntervalMs(YBTS_HB_INTERVAL_DEF)
{
    m_name = "ybts-signalling";
    debugName(m_name);
    debugChain(&__plugin);
    YBTSThreadOwner::initThreadOwner(this);
    YBTSThreadOwner::setDebugPtr(this,this);
    m_transport.setDebugPtr(this,this);
    m_codec.setCodecDebug(this,this);
}

int YBTSSignalling::checkTimers(const Time& time)
{
    // Check timers
    Lock lck(this);
    if (m_state == Closing || m_state == Idle)
	return Ok;
    if (m_timeout && m_timeout <= time) {
	Alarm(this,"system",DebugWarn,"Timeout while waiting for %s [%p]",
	    (m_state != WaitHandshake  ? "heartbeat" : "handshake"),this);
	changeState(Closing);
	return Error;
    }
    if (m_hbTime && m_hbTime <= time) {
	static uint8_t buf[2] = {YBTS::SigHeartbeat,0};
	Debug(this,DebugAll,"Sending heartbeat [%p]",this);
	lck.drop();
	bool ok = m_transport.send(buf,2);
	lck.acquire(this);
	if (ok)
	    setToutHeartbeat(time);
	else if (m_state == Running) {
	    changeState(Closing);
	    return Error;
	}
    }
    return Ok;
}

bool YBTSSignalling::start()
{
    stop();
    while (true) {
	s_globalMutex.lock();
	m_lai = s_lai;
	s_globalMutex.unlock();
	if (!m_lai.lai()) {
	    Debug(this,DebugNote,"Failed to start: invalid LAI [%p]",this);
	    break;
	}
	Lock lck(this);
	if (!m_transport.initTransport(false,s_bufLenSign,true))
	    break;
	if (!startThread("YBTSSignalling"))
	    break;
	changeState(Started);
	Debug(this,DebugInfo,"Started [%p]",this);
	return true;
    }
    stop();
    return false;
}

void YBTSSignalling::stop()
{
    stopThread();
    m_connsMutex.lock();
    m_conns.clear();
    m_connsMutex.unlock();
    Lock lck(this);
    changeState(Idle);
    if (!m_transport.m_socket.valid())
	return;
    m_transport.resetTransport();
    Debug(this,DebugInfo,"Stopped [%p]",this);
}

// Drop a connection
void YBTSSignalling::dropConn(uint16_t connId, bool notifyPeer)
{
    bool found = false;
    Lock lck(m_connsMutex);
    for (ObjList* o = m_conns.skipNull(); o; o = o->skipNext()) {
	YBTSConn* c = static_cast<YBTSConn*>(o->get());
	if (c->connId() != connId)
	    continue;
	found = true;
	Debug(this,DebugAll,"Removing connection (%p,%u) [%p]",c,connId,this);
	o->remove();
	break;
    }
    lck.drop();
    if (notifyPeer && found) {
	YBTSMessage m(YBTS::SigConnRelease,0,connId);
	send(m);
    }
    RefPointer<YBTSChan> chan;
    __plugin.findChan(connId,chan);
    if (chan) {
	chan->connReleased();
	chan = 0;
    }
    __plugin.removeTerminatedCall(connId);
    // TODO:
    // Drop pending location update
    Debug(this,DebugStub,"dropConn() incomplete");
}

// Read socket. Parse and handle received data
void YBTSSignalling::processLoop()
{
    while (!Thread::check(false)) {
	int rd = m_transport.recv();
	if (rd > 0) {
	    uint8_t* buf = (uint8_t*)m_transport.readBuf().data();
	    YBTSMessage* m = YBTSMessage::parse(this,buf,rd);
	    if (m) {
		lock();
		setToutHeartbeat();
		unlock();
		int res = Ok;
		if (m->primitive() != YBTS::SigHeartbeat)
		    res = handlePDU(*m);
		else
		    XDebug(this,DebugAll,"Received heartbeat [%p]",this);
		TelEngine::destruct(m);
		if (res) {
		    if (res == FatalError)
			__plugin.stopNoRestart();
		    else
			__plugin.restart();
		    break;
		}
	    }
	    continue;
	}
	if (rd) {
	    // Socket non retryable error
	    __plugin.restart();
	    break;
	}
	Thread::idle();
	continue;
    }
}

void YBTSSignalling::init(Configuration& cfg)
{
    const NamedList& sect = safeSect(cfg,"signalling");
    m_printMsg = sect.getBoolValue(YSTRING("print_msg"),true);
    const String& md = sect[YSTRING("print_msg_data")];
    if (!md.isBoolean())
	m_printMsgData = 1;
    else
	m_printMsgData = md.toBoolean() ? -1 : 0;
#ifdef DEBUG
    // Allow changing some data for debug purposes
    m_hkIntervalMs = sect.getIntValue("handshake_timeout",60000,5000,120000);

    String s;
    s << "\r\nprint_msg=" << String::boolText(m_printMsg);
    s << "\r\nprint_msg_data=" <<
	(m_printMsgData > 0 ? "verbose" : String::boolText(m_printMsgData < 0));
    s << "\r\nhandshake_timeout=" << m_hkIntervalMs;
    s << "\r\nheartbeat_interval=" << m_hbIntervalMs;

    Debug(this,DebugAll,"Initialized [%p]\r\n-----%s\r\n-----",this,s.c_str());
#endif
}

// Send a message
bool YBTSSignalling::sendInternal(YBTSMessage& msg)
{
    if (m_printMsg && debugAt(DebugInfo))
	printMsg(msg,false);
    DataBlock tmp;
    YBTSMessage::build(this,tmp,msg);
    if (!m_transport.send(tmp))
	return false;
    setHeartbeatTime();
    return true;
}

bool YBTSSignalling::findConnInternal(RefPointer<YBTSConn>& conn, uint16_t connId, bool create)
{
    conn = 0;
    for (ObjList* o = m_conns.skipNull(); o; o = o->skipNext()) {
	YBTSConn* c = static_cast<YBTSConn*>(o->get());
	if (c->connId() == connId) {
	    conn = c;
	    return conn != 0;
	}
    }
    if (create) {
	conn = new YBTSConn(this,connId);
	m_conns.append(conn);
	Debug(this,DebugAll,"Added connection (%p,%u) [%p]",
	    (YBTSConn*)conn,conn->connId(),this);
    }
    return conn != 0;
}

void YBTSSignalling::changeState(int newStat)
{
    if (m_state == newStat)
	return;
    DDebug(this,DebugInfo,"State changed %d -> %d [%p]",m_state,newStat,this);
    m_state = newStat;
    switch (m_state) {
	case Idle:
	case Closing:
	    setTimer(m_timeout,"Timeout",0,0);
	    resetHeartbeatTime();
	    break;
	case WaitHandshake:
	    setToutHandshake();
	    resetHeartbeatTime();
	    break;
	case Running:
	case Started:
	    break;
    }
}

int YBTSSignalling::handlePDU(YBTSMessage& msg)
{
    if (m_printMsg && debugAt(DebugInfo))
	printMsg(msg,true);
    switch (msg.primitive()) {
	case YBTS::SigL3Message:
	    if (m_state != Running) {
		Debug(this,DebugNote,"Ignoring %s in non running state [%p]",
		    msg.name(),this);
		return Ok;
	    }
	    if (msg.xml()) {
		const String& proto = msg.xml()->getTag();
		RefPointer<YBTSConn> conn;
		if (proto == YSTRING("MM")) {
		    findConn(conn,msg.connId(),true);
		    __plugin.mm()->handlePDU(msg,conn);
		}
		else if (proto == YSTRING("CC")) {
		    findConn(conn,msg.connId(),false);
		    __plugin.handleCC(msg,conn);
		}
		else
		    Debug(this,DebugNote,"Unknown '%s' protocol in %s [%p]",
			proto.c_str(),msg.name(),this);
	    }
	    else if (msg.error()) {
		// TODO: close message connection ?
	    }
	    else {
		// TODO: put some error
	    }
	    return Ok;
	case YBTS::SigHandshake:
	    return handleHandshake(msg);
    }
    Debug(this,DebugNote,"Unhandled message %u (%s) [%p]",msg.primitive(),msg.name(),this);
    return Ok;
}

int YBTSSignalling::handleHandshake(YBTSMessage& msg)
{
    Lock lck(this);
    if (state() != WaitHandshake) {
	Debug(this,DebugNote,"Unexpected handshake [%p]",this);
	return Ok;
    }
    lck.drop();
    if (!msg.info()) {
	if (__plugin.handshakeDone()) {
	    lck.acquire(this);
	    if (state() != WaitHandshake)
		return Ok;
	    changeState(Running);
	    YBTSMessage m(YBTS::SigHandshake);
	    if (sendInternal(m))
		return Ok;
	}
    }
    else {
	Alarm(this,"system",DebugWarn,"Unknown version %u in handshake [%p]",
	    msg.info(),this);
	return FatalError;
    }
    return Error;
}

void YBTSSignalling::printMsg(YBTSMessage& msg, bool recv)
{
    String s;
    s << "\r\n-----";
    const char* tmp = msg.name();
    s << "\r\nPrimitive: ";
    if (tmp)
	s << tmp;
    else
	s << msg.primitive();
    s << "\r\nInfo: " << msg.info();
    if (msg.hasConnId())
	s << "\r\nConnection: " << msg.connId();
    if (m_printMsgData) {
	String data;
	if (msg.xml()) {
	    String indent;
	    String origindent;
	    if (m_printMsgData > 0) {
		indent << "\r\n";
		origindent << "  ";
	    }
	    msg.xml()->toString(data,false,indent,origindent);
	}
	s.append(data,"\r\n");
    }
    s << "\r\n-----";
    Debug(this,DebugInfo,"%s [%p]%s",recv ? "Received" : "Sending",this,s.safe());
}


//
// YBTSMedia
//
YBTSMedia::YBTSMedia()
    : Mutex(false,"YBTSMedia"),
    m_srcMutex(false,"YBTSMediaSourceList")
{
    m_name = "ybts-media";
    debugName(m_name);
    debugChain(&__plugin);
    YBTSThreadOwner::initThreadOwner(this);
    YBTSThreadOwner::setDebugPtr(this,this);
    m_transport.setDebugPtr(this,this);
}

YBTSDataSource* YBTSMedia::buildSource(unsigned int connId)
{
    String format;
    getGlobalStr(format,s_format);
    YBTSDataSource* src = new YBTSDataSource(format,connId,this);
    addSource(src);
    return src;
}

YBTSDataConsumer* YBTSMedia::buildConsumer(unsigned int connId)
{
    String format;
    getGlobalStr(format,s_format);
    return new YBTSDataConsumer(format,connId,this);
}

void YBTSMedia::addSource(YBTSDataSource* src)
{
    if (!src)
	return;
    Lock lck(m_srcMutex);
    ObjList* append = &m_sources;
    for (ObjList* o = m_sources.skipNull(); o; o = o->skipNext()) {
	YBTSDataSource* crt = static_cast<YBTSDataSource*>(o->get());
	if (crt->connId() < src->connId()) {
	    append = o;
	    continue;
	}
	if (src->connId() > src->connId()) {
	    o->insert(src)->setDelete(false);
	    DDebug(this,DebugInfo,"Inserted data source (%p,%u) [%p]",
		src,src->connId(),this);
	}
	else if (src != crt) {
	    o->set(src,false);
	    o->setDelete(false);
	    Debug(this,DebugInfo,"Replaced data source id=%u (%p) with (%p) [%p]",
		src->connId(),crt,src,this);
	}
	return;
    }
    append->append(src)->setDelete(false);
    DDebug(this,DebugInfo,"Added data source (%p,%u) [%p]",src,src->connId(),this);
}

void YBTSMedia::removeSource(YBTSDataSource* src)
{
    if (!src)
	return;
    Lock lck(m_srcMutex);
    if (m_sources.remove(src,false))
	DDebug(this,DebugInfo,"Removed data source (%p,%u) [%p]",src,src->connId(),this);
}

void YBTSMedia::cleanup(bool final)
{
    Lock lck(m_srcMutex);
    m_sources.clear();
}

bool YBTSMedia::start()
{
    stop();
    while (true) {
	Lock lck(this);
	if (!m_transport.initTransport(false,s_bufLenMedia,false))
	    break;
	if (!startThread("YBTSMedia"))
	    break;
	Debug(this,DebugInfo,"Started [%p]",this);
	return true;
    }
    stop();
    return false;
}

void YBTSMedia::stop()
{
    cleanup(true);
    stopThread();
    Lock lck(this);
    if (!m_transport.m_socket.valid())
	return;
    m_transport.resetTransport();
    Debug(this,DebugInfo,"Stopped [%p]",this);
}

// Read socket
void YBTSMedia::processLoop()
{
    while (__plugin.state() == YBTSDriver::WaitHandshake && !Thread::check())
	Thread::idle();
    while (!Thread::check(false)) {
	int rd = m_transport.recv();
	if (rd > 0) {
	    uint16_t* d = (uint16_t*)m_transport.m_readBuf.data();
	    YBTSDataSource* src = rd >= 2 ? find(ntohs(*d)) : 0;
	    if (!src)
		continue;
	    DataBlock tmp(d + 1,rd - 2,false);
	    src->Forward(tmp);
	    tmp.clear(false);
	    TelEngine::destruct(src);
	}
	else if (!rd)
	    Thread::idle();
	else {
	    // Socket non retryable error
	    __plugin.restart();
	    break;
	}
    }
}

// Find a source object by connection id, return referrenced pointer
YBTSDataSource* YBTSMedia::find(unsigned int connId)
{
    Lock lck(m_srcMutex);
    for (ObjList* o = m_sources.skipNull(); o; o = o->skipNext()) {
	YBTSDataSource* crt = static_cast<YBTSDataSource*>(o->get());
	if (crt->connId() < connId)
	    continue;
	if (crt->connId() > connId || !crt->ref())
	    crt = 0;
	return crt;
    }
    return 0;
}


//
// YBTSMM
//
YBTSMM::YBTSMM(unsigned int hashLen)
    : Mutex(false,"YBTSMM"),
    m_ueMutex(false,"YBTSMMUEList"),
    m_tmsiIndex(0),
    m_ueIMSI(0),
    m_ueTMSI(0),
    m_ueHashLen(hashLen ? hashLen : 17)
{
    m_name = "ybts-mm";
    debugName(m_name);
    debugChain(&__plugin);
    m_ueIMSI = new ObjList[m_ueHashLen];
    m_ueTMSI = new ObjList[m_ueHashLen];
}

YBTSMM::~YBTSMM()
{
    delete[] m_ueIMSI;
    delete[] m_ueTMSI;
}

void YBTSMM::handlePDU(YBTSMessage& m, YBTSConn* conn)
{
    XmlElement* ch = m.xml() ? m.xml()->findFirstChild() : 0;
    if (!ch) {
	Debug(this,DebugNote,"Empty xml in %s [%p]",m.name(),this);
	return;
    }
    const String& s = ch->getTag();
    if (s == YSTRING("LocationUpdatingRequest"))
	handleLocationUpdate(m,*ch,conn);
    else if (s == YSTRING("CMServiceRequest"))
	handleCMServiceRequest(m,*ch,conn);
    else if (s == YSTRING("CMServiceAbort"))
	__plugin.signalling()->dropConn(conn,true);
    else
	// TODO: send MM status
	Debug(this,DebugNote,"Unhandled '%s' in %s [%p]",s.c_str(),m.name(),this);
}

// Handle location updating requests
void YBTSMM::handleLocationUpdate(YBTSMessage& m, const XmlElement& xml, YBTSConn* conn)
{
    if (!conn) {
	Debug(this,DebugGoOn,"Rejecting %s conn=%u: no connection [%p]",
	    xml.tag(),m.connId(),this);
	sendLocationUpdateReject(m,0,CauseProtoError);
	return;
    }
    XmlElement* laiXml = 0;
    XmlElement* identity = 0;
    findXmlChildren(xml,laiXml,s_locAreaIdent,identity,s_mobileIdent);
    if (!(laiXml && identity)) {
	Debug(this,DebugNote,
	    "Rejecting %s conn=%u: missing LAI or mobile identity [%p]",
	    xml.tag(),m.connId(),this);
	sendLocationUpdateReject(m,conn,CauseInvalidIE);
	return;
    }
    bool haveTMSI = false;
    const String* ident = 0;
    uint8_t cause = getMobileIdentTIMSI(m,xml,*identity,ident,haveTMSI);
    if (cause) {
	sendLocationUpdateReject(m,conn,cause);
	return;
    }
    YBTSLAI lai(*laiXml);
    bool haveLAI = (lai == __plugin.signalling()->lai());
    Debug(this,DebugAll,"Handling %s conn=%u: ident=%s/%s LAI=%s [%p]",
	xml.tag(),conn->connId(),(haveTMSI ? "TMSI" : "IMSI"),
	ident->c_str(),lai.lai().c_str(),this);
    // TODO: handle concurrent requests, check if we have a pending location updating
    // This should never happen, but we should handle it
    RefPointer<YBTSUE> ue;
    if (haveTMSI) {
	// Got TMSI
	// Same LAI: check if we know the IMSI, request it if unknown
	// Different LAI: request IMSI
	if (haveLAI)
	    findUEByTMSISafe(ue,*ident);
	else
	    lai = __plugin.signalling()->lai();
	if (!ue) {
	    Debug(this,DebugNote,"Rejecting %s: IMSI request not implemented [%p]",
		xml.tag(),this);
	    sendLocationUpdateReject(m,conn,CauseServNotSupp);
	    return;
	}
    }
    else {
	// Got IMSI: Create/retrieve TMSI
	getUEByIMSISafe(ue,*ident);
	if (!haveLAI)
	    lai = __plugin.signalling()->lai();
    }
    bool dropConn = false;
    Lock lck(conn);
    cause = setConnUE(*conn,ue,xml,dropConn);
    lck.drop();
    if (cause) {
	sendLocationUpdateReject(m,conn,cause);
	if (dropConn)
	    __plugin.signalling()->dropConn(conn,true);
	return;
    }
    // Accept all for now
    Lock lckUE(ue);
    ue->m_registered = true;
    XmlElement* ch = 0;
    XmlElement* mm = buildMM(ch,"LocationUpdatingAccept");
    if (ch) {
	ch->addChildSafe(lai.build());
	ch->addChildSafe(buildXmlWithChild(s_mobileIdent,s_tmsi,ue->tmsi()));
    }
    lckUE.drop();
    ue = 0;
    conn->sendL3(mm);
}

void YBTSMM::handleCMServiceRequest(YBTSMessage& m, const XmlElement& xml, YBTSConn* conn)
{
    if (!conn) {
	Debug(this,DebugGoOn,"Rejecting %s conn=%u: no connection [%p]",
	    xml.tag(),m.connId(),this);
	sendCMServiceRsp(m,0,CauseProtoError);
	return;
    }
    XmlElement* cmServType = 0;
    XmlElement* identity = 0;
    findXmlChildren(xml,cmServType,s_cmServType,identity,s_mobileIdent);
    if (!(cmServType && identity)) {
	Debug(this,DebugNote,
	    "Rejecting %s conn=%u: missing service type or mobile identity [%p]",
	    xml.tag(),m.connId(),this);
	sendCMServiceRsp(m,conn,CauseInvalidIE);
	return;
    }
    // TODO: Properly check service type, reject if not supported
    const String& type = cmServType->getText();
    if (type == s_cmMOCall)
	;
    else {
	Debug(this,DebugNote,
	    "Rejecting %s conn=%u: service type '%s' not supported/subscribed [%p]",
	    xml.tag(),m.connId(),type.c_str(),this);
	sendCMServiceRsp(m,conn,CauseServNotSupp);
	return;
    }
    bool haveTMSI = false;
    const String* ident = 0;
    uint8_t cause = getMobileIdentTIMSI(m,xml,*identity,ident,haveTMSI);
    if (cause) {
	sendCMServiceRsp(m,conn,cause);
	return;
    }
    Debug(this,DebugAll,"Handling %s conn=%u: ident=%s/%s type=%s [%p]",
	xml.tag(),conn->connId(),(haveTMSI ? "TMSI" : "IMSI"),
	ident->c_str(),type.c_str(),this);
    RefPointer<YBTSUE> ue;
    if (haveTMSI) {
	// Got TMSI
	findUEByTMSISafe(ue,*ident);
	if (!ue) {
	    Debug(this,DebugNote,"Rejecting %s: IMSI request not implemented [%p]",
		xml.tag(),this);
	    sendCMServiceRsp(m,conn,CauseServNotSupp);
	    return;
	}
    }
    else
	// Got IMSI: Create/retrieve TMSI
	getUEByIMSISafe(ue,*ident);
    bool dropConn = false;
    Lock lck(conn);
    cause = setConnUE(*conn,ue,xml,dropConn);
    lck.drop();
    ue = 0;
    sendCMServiceRsp(m,conn,cause);
    if (dropConn)
	__plugin.signalling()->dropConn(conn,true);
}

void YBTSMM::sendLocationUpdateReject(YBTSMessage& msg, YBTSConn* conn, uint8_t cause)
{
    XmlElement* ch = 0;
    XmlElement* mm = buildMM(ch,"LocationUpdatingReject");
    if (ch) {
	// TODO: Use a proper dictionary to retrieve the value
	String tmp(cause);
	const char* reason = tmp;
	ch->addChildSafe(new XmlElement("RejectCause",reason));
    }
    if (conn)
	conn->sendL3(mm);
    else
	__plugin.signalling()->sendL3Conn(msg.connId(),mm);
}

void YBTSMM::sendCMServiceRsp(YBTSMessage& msg, YBTSConn* conn, uint8_t cause)
{
    XmlElement* ch = 0;
    XmlElement* mm = buildMM(ch,!cause ? "CMServiceAccept" : "CMServiceReject");
    if (ch && cause) {
	// TODO: Use a proper dictionary to retrieve the value
	String tmp(cause);
	const char* reason = tmp;
	ch->addChildSafe(new XmlElement("RejectCause",reason));
    }
    if (conn)
	conn->sendL3(mm);
    else
	__plugin.signalling()->sendL3Conn(msg.connId(),mm);
}

// Find UE by TMSI
void YBTSMM::findUEByTMSISafe(RefPointer<YBTSUE>& ue, const String& tmsi)
{
    if (!tmsi)
	return;
    Lock lck(m_ueMutex);
    YBTSUE* tmpUE = 0;
    ObjList& list = m_ueTMSI[hashList(tmsi)];
    for (ObjList* o = list.skipNull(); o ; o = o->skipNext()) {
	YBTSUE* u = static_cast<YBTSUE*>(o->get());
	if (tmsi == u->tmsi()) {
	    tmpUE = u;
	    break;
	}
    }
    XDebug(this,DebugAll,"findUEByTMSISafe(%s) found (%p) [%p]",
	tmsi.c_str(),tmpUE,this);
    ue = tmpUE;
}

// Find UE by IMSI. Create it if not found
void YBTSMM::getUEByIMSISafe(RefPointer<YBTSUE>& ue, const String& imsi)
{
    if (!imsi)
	return;
    Lock lck(m_ueMutex);
    YBTSUE* tmpUE = 0;
    ObjList& list = m_ueIMSI[hashList(imsi)];
    for (ObjList* o = list.skipNull(); o ; o = o->skipNext()) {
	YBTSUE* u = static_cast<YBTSUE*>(o->get());
	if (imsi == u->imsi()) {
	    tmpUE = u;
	    break;
	}
    }
    if (!tmpUE) {
	// TODO: find a better way to allocate TMSI
	String tmsi(++m_tmsiIndex);
	tmpUE = new YBTSUE(imsi,tmsi);
	list.append(tmpUE);
	m_ueTMSI[hashList(tmsi)].append(tmpUE)->setDelete(false);
	Debug(this,DebugInfo,"Added UE imsi=%s tmsi=%s [%p]",
	    tmpUE->imsi().c_str(),tmpUE->tmsi().c_str(),this);
    }
    ue = tmpUE;
}

// Get IMSI/TMSI from request
uint8_t YBTSMM::getMobileIdentTIMSI(YBTSMessage& m, const XmlElement& request,
    const XmlElement& identXml, const String*& ident, bool& isTMSI)
{
    bool found = false;
    ident = getIdentTIMSI(identXml,isTMSI,found);
    if (ident)
	return 0;
    Debug(this,DebugNote,"Rejecting %s conn=%u: %s IMSI/TMSI [%p]",
	request.tag(),m.connId(),(found ? "empty" : "missing"),this);
    return CauseInvalidIE;
}

// Set UE for a connection
uint8_t YBTSMM::setConnUE(YBTSConn& conn, YBTSUE* ue, const XmlElement& req,
    bool& dropConn)
{
    if (!ue) {
	Debug(this,DebugGoOn,"Rejecting %s: no UE object [%p]",req.tag(),this);
	return CauseProtoError;
    }
    if (conn.setUE(ue))
	return 0;
    Debug(this,DebugGoOn,"Rejecting %s: UE mismatch on connection %u [%p]",
	req.tag(),conn.connId(),this);
    dropConn = true;
    return CauseProtoError;
}


//
// YBTSCallDesc
//
// Incoming
YBTSCallDesc::YBTSCallDesc(YBTSChan* chan, const XmlElement& xml, bool regular)
    : YBTSConnIdHolder(chan->connId()),
    m_state(Null),
    m_incoming(true),
    m_regular(regular),
    m_timeout(0),
    m_relSent(0),
    m_chan(chan)
{
    for (const ObjList* o = xml.getChildren().skipNull(); o; o = o->skipNext()) {
	XmlElement* x = static_cast<XmlChild*>(o->get())->xmlElement();
	if (!x)
	    continue;
	const String& s = x->getTag();
	if (s == s_ccCallRef)
	    assign(x->getText());
	else if (s == s_ccCalled)
	    m_called = x->getText();
    }
}

void YBTSCallDesc::proceeding()
{
    changeState(CallProceeding);
    sendCC(s_ccProceeding);
}

void YBTSCallDesc::progressing(XmlElement* indicator)
{
    sendCC(s_ccProgress,indicator);
}

void YBTSCallDesc::alert(XmlElement* indicator)
{
    changeState(CallDelivered);
    sendCC(s_ccAlerting,indicator);
}

void YBTSCallDesc::connect(XmlElement* indicator)
{
    changeState(ConnectReq);
    sendCC(s_ccConnect,indicator);
}

void YBTSCallDesc::hangup()
{
    changeState(Disconnect);
    sendCC(s_ccDisc,buildCCCause(reason()));
}

void YBTSCallDesc::sendStatus(const char* cause)
{
    sendCC(s_ccStatus,buildCCCause(cause),buildCCCallState(m_state));
}

void YBTSCallDesc::sendCC(const String& tag, XmlElement* c1, XmlElement* c2)
{
    XmlElement* ch = 0;
    XmlElement* cc = __plugin.buildCC(ch,tag,*this);
    if (ch) {
	ch->addChildSafe(c1);
	ch->addChildSafe(c2);
    }
    else {
	TelEngine::destruct(c1);
	TelEngine::destruct(c2);
    }
    __plugin.signalling()->sendL3Conn(connId(),cc);
}

void YBTSCallDesc::changeState(int newState)
{
    if (m_state == newState)
	return;
    if (m_chan)
	Debug(m_chan,DebugInfo,"Call '%s' changed state %s -> %s [%p]",
	    c_str(),stateName(),stateName(newState),m_chan);
    else
	Debug(&__plugin,DebugInfo,"Call '%s' changed state %s -> %s",
	    c_str(),stateName(),stateName(newState));
    m_state = newState;
}

// Send CC REL or RLC
void YBTSCallDesc::sendGSMRel(bool rel, const String& callRef, const char* reason,
    uint16_t connId)
{
    XmlElement* ch = 0;
    XmlElement* cc = __plugin.buildCC(ch,rel ? s_ccRel : s_ccRlc,callRef);
    if (ch)
	ch->addChildSafe(buildCCCause(reason));
    __plugin.signalling()->sendL3Conn(connId,cc);
}


//
// YBTSChan
//
// Incoming
YBTSChan::YBTSChan(YBTSConn* conn)
    : Channel(__plugin),
    YBTSConnIdHolder(conn->connId()),
    m_mutex(false,"YBTSChan"),
    m_conn(conn),
    m_hungup(false),
    m_haveTout(false),
    m_tout(0)
{
    if (!m_conn)
	return;
    m_address = m_conn->ue()->imsi();
    Debug(this,DebugCall,"Incoming imsi=%s conn=%u [%p]",
	m_address.c_str(),connId(),this);
}

// Init incoming chan. Return false to destruct the channel
bool YBTSChan::initIncoming(const XmlElement& xml, bool regular)
{
    if (!m_conn)
	return false;
    Lock lck(driver());
    handleSetup(xml,regular);
    YBTSCallDesc* call = firstCall();
    if (!call)
	return false;
    Message* r = message("call.preroute");
    r->addParam("caller",m_conn->ue()->imsi(),false);
    r->addParam("called",call->m_called,false);
    if (!call->m_regular)
	r->addParam("emergency",String::boolText(true));
    Message* s = message("chan.startup");
    s->addParam("caller",m_conn->ue()->imsi(),false);
    s->addParam("called",call->m_called,false);
    lck.drop();
    Engine::enqueue(s);
    initChan();
    startRouter(r);
    return true;
}

// Handle CC messages
bool YBTSChan::handleCC(const XmlElement& xml)
{
    const String* callRef = xml.childText(s_ccCallRef);
    if (TelEngine::null(callRef)) {
	Debug(this,DebugNote,"%s with empty transaction identifier [%p]",
	    xml.tag(),this);
	return true;
    }
    const String& tag = xml.getTag();
    bool regular = (tag == s_ccSetup);
    bool emergency = !regular && (tag == s_ccEmergency);
    Lock lck(m_mutex);
    ObjList* o = m_calls.find(*callRef);
    if (!o) {
	lck.drop();
	if (regular || emergency) {
	    handleSetup(xml,regular);
	    return true;
	}
	return false;
    }
    YBTSCallDesc* call = static_cast<YBTSCallDesc*>(o->get());
    if (regular || emergency)
	call->sendWrongState();
    else if (tag == s_ccRel || tag == s_ccRlc || tag == s_ccDisc) {
	Debug(this,DebugInfo,"Removing call '%s' [%p]",call->c_str(),this);
	getCCCause(call->m_reason,xml);
	String reason = call->m_reason;
	bool final = (tag != s_ccDisc);
	if (final) {
	    if (tag == s_ccRel)
		call->releaseComplete();
	    else
		call->changeState(YBTSCallDesc::Null);
	}
	else {
	    call->release();
	    call->setTimeout(s_t308);
	}
	call = static_cast<YBTSCallDesc*>(o->remove(final));
	bool disc = (m_calls.skipNull() == 0);
	lck.drop();
	if (call)
	    __plugin.addTerminatedCall(call);
	if (disc)
	    hangup(reason);
    }
    else if (tag == s_ccConnectAck) {
	if (call->m_state == YBTSCallDesc::ConnectReq) {
	    call->changeState(YBTSCallDesc::Active);
	    call->m_timeout = 0;
	}
	else
	    call->sendWrongState();
    }
    else if (tag == s_ccStatusEnq)
	call->sendStatus("status-enquiry-rsp");
    else if (tag == s_ccStatus) {
	String cause, cs;
	getCCCause(cause,xml);
	getCCCallState(cs,xml);
	Debug(this,(cause != "status-enquiry-rsp") ? DebugWarn : DebugAll,
	    "Received status cause='%s' call_state='%s' [%p]",
	    cause.c_str(),cs.c_str(),this);
    }
    else
	call->sendStatus("unknown-message");
    return true;
}

// Connection released notification
void YBTSChan::connReleased()
{
    Debug(this,DebugInfo,"Connection released [%p]",this);
    Lock lck(m_mutex);
    m_calls.clear();
    setReason("net-out-of-order");
    lck.drop();
    hangup();
}

void YBTSChan::handleSetup(const XmlElement& xml, bool regular)
{
    Lock lck(m_mutex);
    YBTSCallDesc* call = new YBTSCallDesc(this,xml,regular);
    if (call->null()) {
	TelEngine::destruct(call);
	Debug(this,DebugNote,"%s with empty call ref [%p]",xml.tag(),this);
	return;
    }
    if (!m_calls.skipNull()) {
	call->proceeding();
	m_calls.append(call);
	Debug(this,DebugInfo,"Added call '%s' [%p]",call->c_str(),this);
	return;
    }
    lck.drop();
    Debug(this,DebugNote,"Refusing subsequent call '%s' [%p]",call->c_str(),this);
    call->releaseComplete();
    TelEngine::destruct(call);
}

void YBTSChan::hangup(const char* reason, bool final)
{
    ObjList calls;
    Lock lck(m_mutex);
    setReason(reason);
    for (ObjList* o = m_calls.skipNull(); o; o = o->skipNull())
	calls.append(o->remove(false));
    bool done = m_hungup;
    m_hungup = true;
    String res = m_reason;
    lck.drop();
    for (ObjList* o = calls.skipNull(); o; o = o->skipNull()) {
	YBTSCallDesc* call = static_cast<YBTSCallDesc*>(o->remove(false));
	call->m_reason = res;
	if (call->m_state == YBTSCallDesc::Active) {
	    call->hangup();
	    call->setTimeout(s_t305);
	}
	else {
	    call->release();
	    call->setTimeout(s_t308);
	}
	__plugin.addTerminatedCall(call);
    }
    if (done)
	return;
    Debug(this,DebugCall,"Hangup reason='%s' [%p]",res.c_str(),this);
    Message* m = message("chan.hangup");
    if (res)
	m->setParam("reason",res);
    Engine::enqueue(m);
    if (!final)
	disconnect(res,parameters());
}

void YBTSChan::disconnected(bool final, const char *reason)
{
    Debug(this,DebugAll,"Disconnected '%s' [%p]",reason,this);
    setReason(reason,&m_mutex);
    Channel::disconnected(final,reason);
}

bool YBTSChan::callRouted(Message& msg)
{
    if (!Channel::callRouted(msg))
	return false;
    Lock lck(m_mutex);
    if (!m_conn)
	return false;
    return true;
}

void YBTSChan::callAccept(Message& msg)
{
    Channel::callAccept(msg);
    if (__plugin.media()) {
	setSource(__plugin.media()->buildSource(connId()));
	setConsumer(__plugin.media()->buildConsumer(connId()));
    }
}

void YBTSChan::callRejected(const char* error, const char* reason, const Message* msg)
{
    Channel::callRejected(error,reason,msg);
    setReason(error,&m_mutex);
}

bool YBTSChan::msgProgress(Message& msg)
{
    Channel::msgProgress(msg);
    Lock lck(m_mutex);
    if (m_hungup)
	return false;
    YBTSCallDesc* call = firstCall();
    if (!call)
	return false;
    if (call->m_incoming)
	call->progressing(buildProgressIndicator(msg));
    return true;
}

bool YBTSChan::msgRinging(Message& msg)
{
    Channel::msgRinging(msg);
    Lock lck(m_mutex);
    if (m_hungup)
	return false;
    YBTSCallDesc* call = firstCall();
    if (!call)
	return false;
    if (call->m_incoming && call->m_state == YBTSCallDesc::CallProceeding)
	call->alert(buildProgressIndicator(msg));
    return true;
}

bool YBTSChan::msgAnswered(Message& msg)
{
    Channel::msgAnswered(msg);
    Lock lck(m_mutex);
    if (m_hungup)
	return false;
    YBTSCallDesc* call = firstCall();
    if (!call)
	return false;
    call->connect(buildProgressIndicator(msg,false));
    call->setTimeout(s_t313);
    setTout(call->m_timeout);
    return true;
}

void YBTSChan::checkTimers(Message& msg, const Time& tmr)
{
    if (!m_haveTout)
	return;
    Lock lck(m_mutex);
    if (!m_tout) {
	m_haveTout = false;
	return;
    }
    if (m_tout >= tmr)
	return;
    m_tout = 0;
    m_haveTout = false;
    YBTSCallDesc* call = firstCall();
    if (call) {
	 if (call->m_state == YBTSCallDesc::ConnectReq) {
	    if (call->m_timeout <= tmr) {
		Debug(this,DebugNote,"Call '%s' expired in state %s [%p]",
		    call->c_str(),call->stateName(),this);
		call->m_reason = "timeout";
		call->releaseComplete();
		m_calls.remove(call);
		if (!m_calls.skipNull()) {
		    lck.drop();
		    hangup("timeout");
		    return;
		}
	    }
	}
	setTout(call->m_timeout);
    }
}

void YBTSChan::destroyed()
{
    hangup(0,true);
    Debug(this,DebugCall,"Destroyed [%p]",this);
    Channel::destroyed();
}


//
// YBTSDriver
//
YBTSDriver::YBTSDriver()
    : Driver("ybts","varchans"),
    m_state(Idle),
    m_stateMutex(false,"YBTSState"),
    m_peerPid(0),
    m_stop(false),
    m_stopTime(0),
    m_restart(false),
    m_restartTime(0),
    m_log(0),
    m_media(0),
    m_signalling(0),
    m_mm(0),
    m_haveCalls(false),
    m_engineStart(false),
    m_engineStop(0)
{
    Output("Loaded module YBTS");
}

YBTSDriver::~YBTSDriver()
{
    Output("Unloading module YBTS");
    TelEngine::destruct(m_log);
    TelEngine::destruct(m_media);
    TelEngine::destruct(m_signalling);
    TelEngine::destruct(m_mm);
}

XmlElement* YBTSDriver::buildCC(XmlElement*& ch, const char* tag, const char* callRef)
{
    XmlElement* mm = buildCC();
    ch = static_cast<XmlElement*>(mm->addChildSafe(new XmlElement(tag)));
    if (ch)
	ch->addChildSafe(new XmlElement(s_ccCallRef,callRef));
    return mm;
}

// Handle call control messages
void YBTSDriver::handleCC(YBTSMessage& m, YBTSConn* conn)
{
    XmlElement* xml = m.xml() ? m.xml()->findFirstChild() : 0;
    if (!xml) {
	Debug(this,DebugNote,"%s with empty xml [%p]",m.name(),this);
	return;
    }
    if (conn) {
	RefPointer<YBTSChan> chan;
	findChan(conn->connId(),chan);
	if (chan && chan->handleCC(*xml))
	    return;
    }
    bool regular = (xml->getTag() == s_ccSetup);
    bool emergency = !regular && (xml->getTag() == s_ccEmergency);
    if (regular || emergency) {
	if (conn && conn->ue()) {
	    if (canAccept()) {
		YBTSChan* chan = new YBTSChan(conn);
		if (!chan->initIncoming(*xml,regular))
		    TelEngine::destruct(chan);
		return;
	    }
	    Debug(this,DebugNote,"Refusing new GSM call, full or exiting");
	}
	else if (!conn)
	    Debug(this,DebugNote,"Refusing new GSM call, no connection");
	else
	    Debug(this,DebugNote,
		"Refusing new GSM call, no user associated with connection %u",
		conn->connId());
	const String* callRef = xml->childText(s_ccCallRef);
	if (!TelEngine::null(callRef))
	    YBTSCallDesc::sendGSMRel(false,*callRef,"noconn",m.connId());
	return;
    }
    const String* callRef = xml->childText(s_ccCallRef);
    if (TelEngine::null(callRef))
	return;
    bool rlc = (xml->getTag() == s_ccRlc);
    Lock lck(this);
    // Handle pending calls
    for (ObjList* o = m_terminatedCalls.skipNull(); o; o = o->skipNext()) {
	YBTSCallDesc* call = static_cast<YBTSCallDesc*>(o->get());
	if (*callRef != *call || call->connId() != m.connId())
	    continue;
	if (rlc || xml->getTag() == s_ccRel || xml->getTag() == s_ccDisc) {
	    Debug(this,DebugNote,"Removing terminated call '%s' conn=%u",
		call->c_str(),call->connId());
	    if (!rlc) {
		if (xml->getTag() == s_ccDisc &&
		    call->m_state == YBTSCallDesc::Disconnect) {
		    call->release();
		    call->setTimeout(s_t308);
		    return;
		}
		call->releaseComplete();
	    }
	    o->remove();
	    m_haveCalls = (0 != m_terminatedCalls.skipNull());
	}
	else if (xml->getTag() == s_ccStatusEnq)
	    call->sendStatus("status-enquiry-rsp");
	else if (xml->getTag() != s_ccStatus)
	    call->sendWrongState();
	return;
    }
    if (rlc)
	return;
    lck.drop();
    DDebug(this,DebugInfo,"Unhandled CC %s for callref=%s conn=%p",
	xml->tag(),TelEngine::c_safe(callRef),conn);
    YBTSCallDesc::sendGSMRel(false,*callRef,"invalid-callref",m.connId());
}

// Add a pending (wait termination) call
// Send release if it must
void YBTSDriver::addTerminatedCall(YBTSCallDesc* call)
{
    if (!call)
	return;
    call->m_chan = 0;
    if (!call->m_timeout) {
	Debug(this,DebugNote,"Setting terminated call '%s' conn=%u timeout to %ums",
	    call->c_str(),call->connId(),s_t305);
	call->setTimeout(s_t305);
    }
    call->m_timeout = Time::now() + 1000000;
    m_terminatedCalls.append(call);
    m_haveCalls = true;
}

// Check terminated calls timeout
void YBTSDriver::checkTerminatedCalls(const Time& time)
{
    Lock lck(this);
    for (ObjList* o = m_terminatedCalls.skipNull(); o;) {
	YBTSCallDesc* call = static_cast<YBTSCallDesc*>(o->get());
	if (call->m_timeout > time) {
	    o = o->skipNext();
	    continue;
	}
	// Disconnect: send release, start T308
	// Release: check for resend, restart T308
	if (call->m_state == YBTSCallDesc::Disconnect ||
	    (call->m_state == YBTSCallDesc::Release && call->m_relSent == 1)) {
	    call->release();
	    call->setTimeout(s_t308);
	    o = o->skipNext();
	    continue;
	}
	Debug(this,DebugNote,"Terminated call '%s' conn=%u timed out",
	    call->c_str(),call->connId());
	o->remove();
	o = o->skipNull();
    }
    m_haveCalls = (0 != m_terminatedCalls.skipNull());
}

// Clear terminated calls for a given connection
void YBTSDriver::removeTerminatedCall(uint16_t connId)
{
    Lock lck(this);
    for (ObjList* o = m_terminatedCalls.skipNull(); o;) {
	YBTSCallDesc* call = static_cast<YBTSCallDesc*>(o->get());
	if (call->connId() != connId) {
	    o = o->skipNext();
	    continue;
	}
	Debug(this,DebugInfo,"Removing terminated call '%s' conn=%u: connection released",
	    call->c_str(),call->connId());
	o->remove();
	o = o->skipNull();
    }
    m_haveCalls = (0 != m_terminatedCalls.skipNull());
}

// Handshake done notification. Return false if state is invalid
bool YBTSDriver::handshakeDone()
{
    Lock lck(m_stateMutex);
    if (state() != WaitHandshake) {
	if (state() != Idle) {
	    Debug(this,DebugNote,"Handshake done in %s state !!!",stateName());
	    lck.drop();
	    restart();
	}
	return false;
    }
    changeState(Running);
    return true;
}

void YBTSDriver::restart(unsigned int stopIntervalMs)
{
    Lock lck(m_stateMutex);
    if (m_error)
	return;
    if (m_restartTime)
	m_restart = true;
    else
	setRestart(1);
    if (state() != Idle)
	setStop(stopIntervalMs);
}

void YBTSDriver::stopNoRestart()
{
    Lock lck(m_stateMutex);
    setStop(0);
    m_restart = false;
    m_restartTime = 0;
    m_error = true;
}

void YBTSDriver::start()
{
    stop();
    Lock lck(m_stateMutex);
    changeState(Starting);
    while (true) {
	// Log interface
	if (!m_log->start())
	    break;
	// Signalling interface
	if (!m_signalling->start())
	    break;
	// Media interface
	if (!m_media->start())
	    break;
	// Start control interface
	// Start status interface
	// Start peer application
	if (!startPeer())
	    break;
	changeState(WaitHandshake);
	m_signalling->waitHandshake();
	setRestart(0);
	return;
    }
    setRestart(1);
    Alarm(this,"system",DebugWarn,"Failed to start");
    lck.drop();
    stop();
}

void YBTSDriver::stop()
{
    lock();
    ListIterator iter(channels());
    while (true) {
	GenObject* gen = iter.get();
	if (!gen)
	    break;
	RefPointer<YBTSChan> chan = static_cast<YBTSChan*>(gen);
	if (!chan)
	    continue;
	unlock();
	chan->stop();
	chan = 0;
	lock();
    }
    m_terminatedCalls.clear();
    m_haveCalls = false;
    unlock();
    Lock lck(m_stateMutex);
    if (state() != Idle)
	Debug(this,DebugAll,"Stopping ...");
    m_stop = false;
    m_stopTime = 0;
    m_error = false;
    m_signalling->stop();
    m_media->stop();
    stopPeer();
    m_log->stop();
    changeState(Idle);
    lck.drop();
    channels().clear();
}

bool YBTSDriver::startPeer()
{
    String cmd;
    getGlobalStr(cmd,s_peerPath);
    if (!cmd) {
	Alarm(this,"config",DebugConf,"Empty peer path");
	return false;
    }
    Debug(this,DebugAll,"Starting peer");
    Socket s[YBTS::FDCount];
    s[YBTS::FDLog].attach(m_log->transport().detachRemote());
    s[YBTS::FDSignalling].attach(m_signalling->transport().detachRemote());
    s[YBTS::FDMedia].attach(m_media->transport().detachRemote());
    int pid = ::fork();
    if (pid < 0) {
	String s;
	Alarm(this,"system",DebugWarn,"Failed to fork(): %s",addLastError(s,errno).c_str());
	return false;
    }
    if (pid) {
	Debug(this,DebugInfo,"Started peer pid=%d",pid);
	m_peerPid = pid;
	return true;
    }
    // In child - terminate all other threads if needed
    Thread::preExec();
    // Try to immunize child from ^C and ^backslash
    ::signal(SIGINT,SIG_IGN);
    ::signal(SIGQUIT,SIG_IGN);
    // Restore default handlers for other signals
    ::signal(SIGTERM,SIG_DFL);
    ::signal(SIGHUP,SIG_DFL);
    // Attach socket handles
    int i = STDERR_FILENO + 1;
    for (int j = 0; j < YBTS::FDCount; j++, i++)
	if (::dup2(s[j].handle(),i) == -1) {
	    ::fprintf(stderr,"Failed to set socket handle at index %d: %d %s\n",
		j,errno,strerror(errno));
	    ::close(i);
	}
    // Close all other handles
    for (; i < 1024; i++)
	::close(i);
    // Start
    ::execl(cmd.c_str(),cmd.c_str(),(const char*)NULL);
    ::fprintf(stderr,"Failed to execute '%s': %d %s\n",
	cmd.c_str(),errno,strerror(errno));
    // Shit happened. Die !!!
    ::_exit(1);
    return false;
}

static int waitPid(pid_t pid, unsigned int interval)
{
    int ret = -1;
    for (unsigned int i = threadIdleIntervals(interval); i && ret < 0; i--) {
	Thread::idle();
	ret = ::waitpid(pid,0,WNOHANG);
    }
    return ret;
}

void YBTSDriver::stopPeer()
{
    if (!m_peerPid)
	return;
    int w = ::waitpid(m_peerPid,0,WNOHANG);
    if (w > 0) {
	Debug(this,DebugInfo,"Peer pid %d already terminated",m_peerPid);
	m_peerPid = 0;
	return;
    }
    if (w == 0) {
	Debug(this,DebugNote,"Peer pid %d has not exited - we'll kill it",m_peerPid);
	::kill(m_peerPid,SIGTERM);
	w = waitPid(m_peerPid,100);
    }
    if (w == 0) {
	Debug(this,DebugWarn,"Peer pid %d has still not exited yet?",m_peerPid);
	::kill(m_peerPid,SIGKILL);
	w = waitPid(m_peerPid,60);
	if (w <= 0)
	    Alarm(this,"system",DebugWarn,"Failed to kill peer pid %d, leaving zombie",m_peerPid);
    }
    else if (w < 0 && errno != ECHILD)
	Debug(this,DebugMild,"Failed waitpid on peer pid %d: %d %s",
	    m_peerPid,errno,::strerror(errno));
    else
	Debug(this,DebugInfo,"Peer pid %d terminated",m_peerPid);
    m_peerPid = 0;
}

bool YBTSDriver::handleEngineStop(Message& msg)
{
    m_engineStop++;
    m_media->cleanup(false);
    // TODO: unregister
    // TODO: Drop all channels
    return false;
}

YBTSChan* YBTSDriver::findChanConnId(uint16_t connId)
{
    for (ObjList* o = channels().skipNull(); o ; o = o->skipNext()) {
	YBTSChan* ch = static_cast<YBTSChan*>(o->get());
	if (ch->connId() == connId)
	    return ch;
    }
    return 0;
}

void YBTSDriver::changeState(int newStat)
{
    if (m_state == newStat)
	return;
    Debug(this,DebugNote,"State changed %s -> %s",
	stateName(),lookup(newStat,s_stateName));
    m_state = newStat;
}

void YBTSDriver::setRestart(int resFlag, bool on)
{
    if (resFlag >= 0)
	m_restart = (resFlag > 0);
    if (on) {
	m_restartTime = Time::now() + (uint64_t)s_restartMs * 1000;
	Debug(this,DebugAll,"Restart scheduled in %ums [%p]",s_restartMs,this);
    }
    else if (m_restartTime) {
	m_restartTime = 0;
	Debug(this,DebugAll,"Restart timer reset [%p]",this);
    }
}

// Check stop conditions
void YBTSDriver::checkStop(const Time& time)
{
    Lock lck(m_stateMutex);
    if (m_stop && m_stopTime) {
	if (m_stopTime <= time) {
	    lck.drop();
	    stop();
	}
    }
    else
	m_stop = false;
}

// Check restart conditions
void YBTSDriver::checkRestart(const Time& time)
{
    Lock lck(m_stateMutex);
    if (m_restart && m_restartTime) {
	if (m_restartTime <= time) {
	    lck.drop();
	    start();
	}
    }
    else
	m_restart = false;
}


static void parseLAI(YBTSLAI& dest, const String& lai)
{
    String l = lai;
    l.trimBlanks();
    if (!l)
	return;
    ObjList* list = l.split(',');
    if (list->count() == 3) {
	ObjList* o = list->skipNull();
	// MNC: 3 digits
	String* mnc = static_cast<String*>(o->get());
	String* mcc = static_cast<String*>((o = o->skipNext())->get());
	String* lac = static_cast<String*>((o = o->skipNext())->get());
	if (mnc->length() == 3 && isDigits09(*mnc) &&
	    ((mcc->length() == 3 || mcc->length() == 2) && isDigits09(*mcc))) {
	    // LAC: 16 bits long
	    int tmp = lac->toInteger(-1);
	    if (tmp >= 0 && tmp < 0xffff)
		dest.set(*mnc,*mcc,*lac);
	}
    }
    TelEngine::destruct(list);
}

void YBTSDriver::initialize()
{
    static bool s_first = true;
    Output("Initializing module YBTS");
    Configuration cfg(Engine::configFile("bts"));
    const NamedList& general = safeSect(cfg,"general");
    const NamedList& ybts = safeSect(cfg,"ybts");
    if (s_first) {
	s_first = false;
	setup();
	installRelay(Halt);
	installRelay(Stop,"engine.stop");
	installRelay(Start,"engine.start");
        m_log = new YBTSLog;
	m_media = new YBTSMedia;
	m_signalling = new YBTSSignalling;
	m_mm = new YBTSMM(ybts.getIntValue("ue_hash_size",31,5));
    }
    s_restartMs = ybts.getIntValue("restart_interval",
	YBTS_RESTART_DEF,YBTS_RESTART_MIN,YBTS_RESTART_MAX);
    s_t305 = general.getIntValue("t305",30000,20000,60000);
    s_t308 = general.getIntValue("t308",5000,4000,20000);
    s_t313 = general.getIntValue("t313",5000,4000,20000);
    s_globalMutex.lock();
    YBTSLAI lai;
    const String& laiStr = general[YSTRING("lai")];
    parseLAI(lai,laiStr);
    if (lai != s_lai || !s_lai.lai()) {
	if (lai.lai())
	    Debug(this,DebugInfo,"LAI changed %s -> %s",
		s_lai.lai().c_str(),lai.lai().c_str());
	else
	    Debug(this,DebugConf,"Invalid LAI '%s'",laiStr.c_str());
	s_lai = lai;
    }
#ifdef DEBUG
    // Allow changing some data for debug purposes
    s_peerPath = ybts.getValue("peer_path",s_peerPath);
#endif
    s_globalMutex.unlock();
    m_signalling->init(cfg);
    startIdle();
}

bool YBTSDriver::msgExecute(Message& msg, String& dest)
{
    if (!msg.userData()) {
	Debug(this,DebugWarn,"GSM call found but no data channel!");
	return false;
    }
    if (m_state != RadioUp) {
	Debug(this,DebugWarn,"GSM call: Radio is not up!");
	msg.setParam(YSTRING("error"),"interworking");
	return false;
    }
    Debug(this,DebugStub,"msgExecute not implemented");
    return false;
}

bool YBTSDriver::received(Message& msg, int id)
{
    if (id == Start) {
	if (!m_engineStart) {
	    m_engineStart = true;
	    startIdle();
	}
	return false;
    }
    if (id == Stop)
	return handleEngineStop(msg);
    if (id == Halt) {
	dropAll(msg);
	stop();
	return false;
    }
    if (id == Timer) {
	// Handle stop/restart
	// Don't handle both in the same tick: wait some time between stop and restart
	if (m_stop)
	    checkStop(msg.msgTime());
	else if (m_restart)
	    checkRestart(msg.msgTime());
	if (m_signalling->shouldCheckTimers()) {
	    int res = m_signalling->checkTimers(msg.msgTime());
	    if (res) {
		if (res == YBTSSignalling::FatalError)
		    stopNoRestart();
		else
		    restart();
	    }
	}
	if (m_haveCalls)
	    checkTerminatedCalls(msg.msgTime());
    }
    return Driver::received(msg,id);
}

}; // anonymous namespace

/* vi: set ts=8 sw=4 sts=4 noet: */
