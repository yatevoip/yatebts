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

#ifdef _WINDOWS
#error This module is not for Windows
#endif

#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <syslog.h>

using namespace TelEngine;
namespace { // anonymous

#include "ybts.h"

#define BTS_DIR "bts"
#define BTS_CMD "mbts"

// Handshake interval (timeout)
#define YBTS_HK_INTERVAL_DEF 60000
// Heartbeat interval
#define YBTS_HB_INTERVAL_DEF 30000
// Heartbeat timeout
#define YBTS_HB_TIMEOUT_DEF 60000
// Restart time
#define YBTS_RESTART_DEF 120000
#define YBTS_RESTART_MIN 30000
#define YBTS_RESTART_MAX 600000
// Peer check
#define YBTS_PEERCHECK_DEF 3000

// Constant strings
static const String s_message = "Message";
static const String s_type = "type";
static const String s_locAreaIdent = "LAI";
static const String s_PLMNidentity = "PLMNidentity";
static const String s_LAC = "LAC";
static const String s_mobileIdent = "MobileIdentity";
static const String s_imsi = "IMSI";
static const String s_tmsi = "TMSI";
static const String s_cause = "Cause";
static const String s_causeLocation = "location";
static const String s_causeCoding = "coding";
static const String s_cmServType = "CMServiceType";
static const String s_cmMOCall = "MO-call-establishment-or-PM-connection-establishment";
static const String s_ccSetup = "Setup";
static const String s_ccEmergency = "EmergencySetup";
static const String s_ccProceeding = "CallProceeding";
static const String s_ccProgress = "Progress";
static const String s_ccAlerting = "Alerting";
static const String s_ccConnect = "Connect";
static const String s_ccConnectAck = "ConnectAcknowledge";
static const String s_ccDisc = "Disconnect";
static const String s_ccRel = "Release";
static const String s_ccRlc = "ReleaseComplete";
static const String s_ccStatusEnq = "StatusEnquiry";
static const String s_ccStatus = "Status";
static const String s_ccStartDTMF = "StartDTMF";
static const String s_ccStopDTMF = "StopDTMF";
static const String s_ccKeypadFacility = "KeypadFacility";
static const String s_ccHold = "Hold";
static const String s_ccRetrieve = "Retrieve";
static const String s_ccCallRef = "TID";
static const String s_ccTIFlag = "TIFlag";
static const String s_ccCalled = "CalledPartyBCDNumber";
static const String s_ccCallState = "CallState";
//static const String s_ccFacility = "Facility";
static const String s_ccProgressInd = "ProgressIndicator";
//static const String s_ccUserUser = "UserUser";
static const String s_ccSsCLIR = "CLIRInvocation";
static const String s_ccSsCLIP = "CLIRSuppresion";

class YBTSConnIdHolder;                  // A connection id holder
class YBTSThread;
class YBTSThreadOwner;
class YBTSMessage;                       // YBTS <-> BTS PDU
class YBTSTransport;
class YBTSLAI;                           // Holds local area id
class YBTSConn;                          // A logical connection
class YBTSLog;                           // Log interface
class YBTSCommand;                       // Command interface
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
    inline XmlElement* takeXml()
	{ XmlElement* x = m_xml; m_xml = 0; return x; }
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
    friend class YBTSCommand;
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
    inline bool canSelect() const
	{ return m_socket.canSelect(); }

protected:
    bool send(const void* buf, unsigned int len);
    inline bool send(const DataBlock& data)
	{ return send(data.data(),data.length()); }
    // Read socket data. Return 0: nothing read, >1: read data, negative: fatal error
    int recv();
    bool initTransport(bool stream, unsigned int buflen, bool reserveNull);
    void resetTransport();
    void alarmError(Socket& sock, const char* oper);

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
    ~YBTSConn()
	{ TelEngine::destruct(m_xml); }
    inline YBTSUE* ue()
	{ return m_ue; }
    // Peek at the pending XML element
    inline const XmlElement* xml() const
	{ return m_xml; }
    // Take out the pending XML element
    inline XmlElement* takeXml()
	{ XmlElement* x = m_xml; m_xml = 0; return x; }
    // Set pending XML. Return false if another XML element is already pending
    inline bool setXml(XmlElement* xml)
	{ return (!m_xml) && ((m_xml = xml)); }
    // Send an L3 connection related message
    bool sendL3(XmlElement* xml, uint8_t info = 0);
protected:
    YBTSConn(YBTSSignalling* owner, uint16_t connId);
    // Set connection UE. Return false if requested to change an existing, different UE
    bool setUE(YBTSUE* ue);

    YBTSSignalling* m_owner;
    XmlElement* m_xml;
    RefPointer<YBTSUE> m_ue;
};

class YBTSLog : public GenObject, public DebugEnabler, public Mutex,
    public YBTSThreadOwner
{
public:
    YBTSLog(const char* name);
    inline YBTSTransport& transport()
	{ return m_transport; }
    bool start();
    void stop();
    // Read socket
    virtual void processLoop();
    bool setDebug(Message& msg, const String& target);
protected:
    String m_name;
    YBTSTransport m_transport;
};

class YBTSCommand : public GenObject, public DebugEnabler
{
public:
    YBTSCommand();
    bool send(const String& str);
    bool recv(String& str);
    inline YBTSTransport& transport()
	{ return m_transport; }
    bool start();
    void stop();
    bool setDebug(Message& msg, const String& target);
protected:
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
	    YBTSMessage m(SigL3Message,info,connId,xml);
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
    inline bool findConn(RefPointer<YBTSConn>& conn, uint16_t connId, bool create) {
	    Lock lck(m_connsMutex);
	    return findConnInternal(conn,connId,create);
	}
    inline bool findConn(RefPointer<YBTSConn>& conn, const YBTSUE* ue) {
	    Lock lck(m_connsMutex);
	    return findConnInternal(conn,ue);
	}

protected:
    bool sendInternal(YBTSMessage& msg);
    bool findConnInternal(RefPointer<YBTSConn>& conn, uint16_t connId, bool create);
    bool findConnInternal(RefPointer<YBTSConn>& conn, const YBTSUE* ue);
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
	{ setTimer(m_timeout,"Timeout handshake",m_hkIntervalMs,timeUs); }
    inline void setToutHeartbeat(uint64_t timeUs = Time::now())
	{ setTimer(m_timeout,"Timeout heartbeart",m_hbTimeoutMs,timeUs); }
    inline void setHeartbeatTime(uint64_t timeUs = Time::now())
	{ setTimer(m_hbTime,"Heartbeat interval",m_hbIntervalMs,timeUs); }
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
    unsigned int m_hbTimeoutMs;          // Heartbeat timeout in miliseconds
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
    inline const String& imei() const
	{ return m_imei; }
    inline const String& msisdn() const
	{ return m_msisdn; }
    inline const String& paging() const
	{ return m_paging; }
    inline bool registered() const
	{ return m_registered; }
    inline uint32_t expires() const
	{ return m_expires; }
    bool startPaging(BtsPagingChanType type);
    void stopPaging();
    void stopPagingNow();

protected:
    inline YBTSUE(const char* imsi, const char* tmsi)
	: Mutex(false,"YBTSUE"),
	  m_registered(false), m_expires(0), m_pageCnt(0),
	  m_imsi(imsi), m_tmsi(tmsi)
	{ }
    bool m_registered;
    uint32_t m_expires;
    uint32_t m_pageCnt;
    String m_imsi;
    String m_tmsi;
    String m_imei;
    String m_msisdn;
    String m_paging;
};

class YBTSMM : public GenObject, public DebugEnabler, public Mutex
{
    friend class YBTSDriver;
public:
    YBTSMM(unsigned int hashLen);
    ~YBTSMM();
    inline XmlElement* buildMM()
	{ return new XmlElement("MM"); }
    inline XmlElement* buildMM(XmlElement*& ch, const char* type) {
	    XmlElement* mm = buildMM();
	    ch = static_cast<XmlElement*>(mm->addChildSafe(new XmlElement(s_message)));
	    ch->setAttribute(s_type,type);
	    return mm;
	}
    inline void saveUEs()
	{ m_saveUEs = true; }
    void handlePDU(YBTSMessage& msg, YBTSConn* conn);
    void newTMSI(String& tmsi);

protected:
    void handleIdentityResponse(YBTSMessage& m, const XmlElement& xml, YBTSConn* conn);
    void handleLocationUpdate(YBTSMessage& msg, const XmlElement& xml, YBTSConn* conn);
    void handleUpdateComplete(YBTSMessage& m, const XmlElement& xml, YBTSConn* conn);
    void handleIMSIDetach(YBTSMessage& m, const XmlElement& xml, YBTSConn* conn);
    void handleCMServiceRequest(YBTSMessage& msg, const XmlElement& xml, YBTSConn* conn);
    void sendLocationUpdateReject(YBTSMessage& msg, YBTSConn* conn, uint8_t cause);
    void sendCMServiceRsp(YBTSMessage& msg, YBTSConn* conn, uint8_t cause = 0);
    void sendIdentityRequest(YBTSConn* conn, const char* type);
    // Find UE by target identity
    bool findUESafe(RefPointer<YBTSUE>& ue, const String& dest);
    // Find UE by TMSI
    void findUEByTMSISafe(RefPointer<YBTSUE>& ue, const String& tmsi);
    // Find UE by IMEI
    void findUEByIMEISafe(RefPointer<YBTSUE>& ue, const String& imei);
    // Find UE by MSISDN
    void findUEByMSISDNSafe(RefPointer<YBTSUE>& ue, const String& msisdn);
    // Find UE by IMSI. Create it if not found
    void getUEByIMSISafe(RefPointer<YBTSUE>& ue, const String& imsi, bool create = true);
    inline unsigned int hashList(const String& str)
	{ return str.hash() % m_ueHashLen; }
    // Get IMSI/TMSI from request
    uint8_t getMobileIdentTIMSI(YBTSMessage& m, const XmlElement& request,
	const XmlElement& identXml, const String*& ident, bool& isTMSI);
    // Set UE for a connection
    uint8_t setConnUE(YBTSConn& conn, YBTSUE* ue, const XmlElement& req,
	bool* dropConn = 0);
    void updateExpire(YBTSUE* ue);
    void checkTimers(const Time& time = Time());
    void loadUElist();
    void saveUElist();

    String m_name;
    Mutex m_ueMutex;
    uint32_t m_tmsiIndex;                // Index used to generate TMSI
    ObjList* m_ueIMSI;                   // List of UEs grouped by IMSI
    ObjList* m_ueTMSI;                   // List of UEs grouped by TMSI
    unsigned int m_ueHashLen;            // Length of UE lists
    bool m_saveUEs;                      // UE list needs saving
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
    // Incoming (MO)
    YBTSCallDesc(YBTSChan* chan, const XmlElement& xml, bool regular, const String* callRef);
    // Outgoing (MT)
    YBTSCallDesc(YBTSChan* chan, const XmlElement* xml, const String& callRef);
    inline bool incoming() const
	{ return m_incoming; }
    inline const String& callRef() const
	{ return m_callRef; }
    inline bool active() const
	{ return Active == m_state; }
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
	    sendGSMRel(true,reason(),connId());
	}
    inline void releaseComplete() {
	    changeState(Null);
	    sendGSMRel(false,reason(),connId());
	}
    void sendCC(const String& tag, XmlElement* c1 = 0, XmlElement* c2 = 0);
    void changeState(int newState);
    inline void setTimeout(unsigned int intervalMs, const Time& time = Time())
	{ m_timeout = time + (uint64_t)intervalMs * 1000; }
    // Send CC REL or RLC
    static void sendGSMRel(bool rel, const String& callRef, bool tiFlag, const char* reason,
	uint16_t connId);
    inline void sendGSMRel(bool rel, const char* reason, uint16_t connId)
	{ sendGSMRel(rel,callRef(),incoming(),reason,connId); }
    static const char* prefix(bool tiFlag)
	{ return tiFlag ? "o" : "i"; }
    static inline const char* stateName(int stat)
	{ return lookup(stat,s_stateName); }
    static const TokenDict s_stateName[];

    int m_state;
    bool m_incoming;
    bool m_regular;
    uint64_t m_timeout;
    uint8_t m_relSent;
    YBTSChan* m_chan;
    String m_callRef;
    String m_reason;
    String m_called;
};

class YBTSChan : public Channel, public YBTSConnIdHolder
{
public:
    // Incoming
    YBTSChan(YBTSConn* conn);
    // Outgoing
    YBTSChan(YBTSUE* ue, YBTSConn* conn = 0);
    inline YBTSConn* conn() const
	{ return m_conn; }
    inline YBTSUE* ue() const
	{ return m_conn ? m_conn->ue() : (YBTSUE*)m_ue; }
    // Init incoming chan. Return false to destruct the channel
    bool initIncoming(const XmlElement& xml, bool regular, const String* callRef);
    // Init outgoing chan. Return false to destruct the channel
    bool initOutgoing(Message& msg);
    // Handle CC messages
    bool handleCC(const XmlElement& xml, const String* callRef, bool tiFlag);
    // Connection released notification
    void connReleased();
    // Check if MT call can be accepted
    bool canAcceptMT();
    // Start a MT call after UE connected
    void startMT(YBTSConn* conn = 0);
    // BTS stopping notification
    inline void stop()
	{ hangup("interworking"); }

protected:
    inline YBTSCallDesc* activeCall()
	{ return m_activeCall; }
    inline YBTSCallDesc* firstCall() {
	    ObjList* o = m_calls.skipNull();
	    return o ? static_cast<YBTSCallDesc*>(o->get()) : 0;
	}
    YBTSCallDesc* handleSetup(const XmlElement& xml, bool regular, const String* callRef);
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
    void allocTraffic();
    void startTraffic(uint8_t mode = 1);
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
    RefPointer<YBTSUE> m_ue;
    ObjList m_calls;
    String m_reason;
    uint8_t m_traffic;
    uint8_t m_cref;
    char m_dtmf;
    bool m_mpty;
    bool m_hungup;
    bool m_paging;
    bool m_haveTout;
    uint64_t m_tout;
    YBTSCallDesc* m_activeCall;
    XmlElement* m_pending;
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
    inline bool isPeerCheckState() const
	{ return m_state >= WaitHandshake && m_peerPid; }
    inline void setPeerAlive()
	{ m_peerAlive = true; }
    inline YBTSMedia* media()
	{ return m_media; }
    inline YBTSSignalling* signalling()
	{ return m_signalling; }
    inline YBTSMM* mm()
	{ return m_mm; }
    inline XmlElement* buildCC()
	{ return new XmlElement("CC"); }
    XmlElement* buildCC(XmlElement*& ch, const char* type, const char* callRef, bool tiFlag = false);
    // Handle call control messages
    void handleCC(YBTSMessage& m, YBTSConn* conn);
    // Check if a MT service is pending for new connection and start it
    void checkMtService(YBTSUE* ue, YBTSConn* conn);
    // Add a pending (wait termination) call
    void addTerminatedCall(YBTSCallDesc* call);
    // Check terminated calls timeout
    void checkTerminatedCalls(const Time& time = Time());
    // Clear terminated calls for a given connection
    void removeTerminatedCall(uint16_t connId);
    // Handshake done notification. Return false if state is invalid
    bool handshakeDone();
    // Radio ready notification. Return false if state is invalid
    bool radioReady();
    void restart(unsigned int restartMs = 1, unsigned int stopIntervalMs = 0);
    void stopNoRestart();
    inline bool findChan(uint16_t connId, RefPointer<YBTSChan>& chan) {
	    Lock lck(this);
	    chan = findChanConnId(connId);
	    return chan != 0;
	}
    inline bool findChan(const YBTSUE* ue, RefPointer<YBTSChan>& chan) {
	    Lock lck(this);
	    chan = findChanUE(ue);
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
    YBTSChan* findChanUE(const YBTSUE* ue);
    void changeState(int newStat);
    void setRestart(int resFlag, bool on = true, unsigned int interval = 0);
    void checkStop(const Time& time);
    void checkRestart(const Time& time);
    inline void setStop(unsigned int stopIntervalMs) {
	    m_stop = true;
	    if (m_stopTime)
		return;
	    m_stopTime = Time::now() + (uint64_t)stopIntervalMs * 1000;
	    Debug(this,DebugAll,"Scheduled stop in %ums",stopIntervalMs);
	}
    void btsStatus(Message& msg);
    virtual void initialize();
    virtual bool msgExecute(Message& msg, String& dest);
    virtual bool received(Message& msg, int id);
    virtual bool commandExecute(String& retVal, const String& line);
    virtual bool commandComplete(Message& msg, const String& partLine, const String& partWord);
    virtual bool setDebug(Message& msg, const String& target);

    int m_state;
    Mutex m_stateMutex;
    pid_t m_peerPid;                     // Peer PID
    bool m_peerAlive;
    uint64_t m_peerCheckTime;
    unsigned int m_peerCheckIntervalMs;
    bool m_error;                        // Error flag, ignore restart
    bool m_stop;                         // Stop flag
    uint64_t m_stopTime;                 // Stop time
    bool m_restart;                      // Restart flag
    uint64_t m_restartTime;              // Restart time
    YBTSLog* m_logTrans;                 // Log transceiver
    YBTSLog* m_logBts;                   // Log OpenBTS
    YBTSCommand* m_command;              // Command interface
    YBTSMedia* m_media;                  // Media
    YBTSSignalling* m_signalling;        // Signalling
    YBTSMM* m_mm;                        // Mobility management
    ObjList m_terminatedCalls;           // Terminated calls list
    bool m_haveCalls;                    // Empty terminated calls list flag
    bool m_engineStart;
    unsigned int m_engineStop;
    ObjList* m_helpCache;
};

INIT_PLUGIN(YBTSDriver);
static Mutex s_globalMutex(false,"YBTSGlobal");
static YBTSLAI s_lai;
static String s_format = "gsm";          // Format to use
static String s_peerCmd;                 // Peer program command path
static String s_peerArg;                 // Peer program argument
static String s_peerDir;                 // Peer program working directory
static String s_ueFile;                  // File to save UE information
static bool s_askIMEI = true;            // Ask the IMEI identity
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

static uint32_t s_tmsiExpire = 864000;   // TMSI expiration, default 10 days

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
#define MAKE_SIG(x) {#x, Sig##x}
    MAKE_SIG(L3Message),
    MAKE_SIG(ConnLost),
    MAKE_SIG(ConnRelease),
    MAKE_SIG(StartMedia),
    MAKE_SIG(StopMedia),
    MAKE_SIG(AllocMedia),
    MAKE_SIG(MediaError),
    MAKE_SIG(MediaStarted),
    MAKE_SIG(Handshake),
    MAKE_SIG(RadioReady),
    MAKE_SIG(StartPaging),
    MAKE_SIG(StopPaging),
    MAKE_SIG(Heartbeat),
#undef MAKE_SIG
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

static inline XmlElement* buildCCCause(const char* cause, const char* location = "LPN", const char* coding = "GSM-PLMN")
{
    // TODO: check it when codec will be implemented
    if (TelEngine::null(cause))
	cause = "normal";
    XmlElement* xml = new XmlElement(s_cause,cause);
    if (coding)
	xml->setAttribute(s_causeCoding,coding);
    if (location)
	xml->setAttribute(s_causeLocation,location);
    return xml;
}

static inline XmlElement* buildCCCause(const String& cause)
{
    const char* location = "LPN";
    if (cause == YSTRING("busy") || cause == YSTRING("noresponse") || cause == YSTRING("noanswer"))
	location = "U";
    return buildCCCause(cause,location,"GSM-PLMN");
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
	reason << "Codec error " << e << " (" << lookup(e,GSML3Codec::s_errorsDict,"unknown") << ")";
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
	case SigL3Message:
#ifdef DEBUG
	    {
		String tmp;
		tmp.hexify(data,len,' ');
		Debug(recv,DebugAll,"Recv L3 message: %s",tmp.c_str());
	    }
#endif
	    decodeMsg(recv->codec(),data,len,m->m_xml,reason);
	    break;
	case SigHandshake:
	case SigRadioReady:
	case SigHeartbeat:
	case SigConnLost:
	case SigMediaError:
	case SigMediaStarted:
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
	reason << "Codec error " << e << " (" << lookup(e,GSML3Codec::s_errorsDict,"unknown") << ")";
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
	case SigL3Message:
	    if (encodeMsg(sender->codec(),msg,buf,reason)) {
#ifdef DEBUG
		void* data = buf.data(4);
		if (data) {
		    String tmp;
		    tmp.hexify(data,buf.length() - 4,' ');
		    Debug(sender,DebugAll,"Send L3 message: %s",tmp.c_str());
		}
#endif
		return true;
	    }
	    break;
	case SigStartPaging:
	case SigStopPaging:
	    if (!msg.xml()) {
		reason = "Missing XML";
		break;
	    }
	    buf.append(msg.xml()->getText());
	    return true;
	case SigHeartbeat:
	case SigHandshake:
	case SigConnRelease:
	case SigStartMedia:
	case SigStopMedia:
	case SigAllocMedia:
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
    else if (!m_writeSocket.canRetry())
	alarmError(m_writeSocket,"send");
    return false;
}

// Read socket data. Return 0: nothing read, >1: read data, negative: fatal error
int YBTSTransport::recv()
{
    if (!m_readSocket.valid())
	return 0;
    if (canSelect()) {
	bool ok = false;
	if (!m_readSocket.select(&ok,0,0,Thread::idleUsec())) {
	    if (m_readSocket.canRetry())
		return 0;
	    alarmError(m_readSocket,"select");
	    return -1;
	}
	if (!ok)
	    return 0;
    }
    uint8_t* buf = (uint8_t*)m_readBuf.data();
    int rd = m_readSocket.recv(buf,m_maxRead);
    if (rd >= 0) {
	if (rd) {
	    __plugin.setPeerAlive();
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
    alarmError(m_readSocket,"read");
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

void YBTSTransport::alarmError(Socket& sock, const char* oper)
{
    String tmp;
    addLastError(tmp,sock.error());
    Alarm(m_enabler,"socket",DebugWarn,"Socket %s error%s [%p]",
	oper,tmp.c_str(),m_ptr);
}


//
// YBTSLAI
//
YBTSLAI::YBTSLAI(const XmlElement& xml)
{
    const String* mnc_mcc = &String::empty();
    const String* lac = &String::empty();
    getXmlChildTextAll(xml,mnc_mcc,s_PLMNidentity,lac,s_LAC);
    m_mnc_mcc = *mnc_mcc;
    m_lac = *lac;
    m_lai << m_mnc_mcc << "_" << m_lac;
}

XmlElement* YBTSLAI::build() const
{
    XmlElement* xml = new XmlElement(s_locAreaIdent);
    if (m_mnc_mcc)
	xml->addChildSafe(new XmlElement(s_PLMNidentity,m_mnc_mcc));
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
    m_owner(owner), m_xml(0)
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
YBTSLog::YBTSLog(const char* name)
    : Mutex(false,"YBTSLog"),
      m_name(name)
{
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
	if (rd > 1) {
	    int level = -1;
	    switch (m_transport.readBuf().at(0)) {
		case LOG_EMERG:
		    level = DebugGoOn;
		    break;
		case LOG_ALERT:
		case LOG_CRIT:
		    level = DebugWarn;
		    break;
		case LOG_ERR:
		case LOG_WARNING:
		    level = DebugMild;
		    break;
		case LOG_NOTICE:
		    level = DebugNote;
		    break;
		case LOG_INFO:
		    level = DebugInfo;
		    break;
		case LOG_DEBUG:
		    level = DebugAll;
		    break;
	    }
	    if (level >= 0)
		Debug(this,level,"%s",(const char*)m_transport.readBuf().data(1));
	    else
		Output("%s",(const char*)m_transport.readBuf().data(1));
	    continue;
	}
	if (!rd) {
	    if (!m_transport.canSelect())
		Thread::idle();
	    continue;
	}
	// Socket non retryable error
	__plugin.restart();
	break;
    }
}

bool YBTSLog::setDebug(Message& msg, const String& target)
{
    String str = msg.getValue("line");
    if (str.startSkip("level")) {
	int dbg = debugLevel();
	str >> dbg;
	debugLevel(dbg);
    }
    else if (str == "reset")
	debugChain(&__plugin);
    else {
	bool dbg = debugEnabled();
	str >> dbg;
	debugEnabled(dbg);
    }
    msg.retValue() << "Module " << target
	<< " debug " << (debugEnabled() ? "on" : "off")
	<< " level " << debugLevel() << "\r\n";
    return true;
}


//
// YBTSCommand
//
YBTSCommand::YBTSCommand()
{
    debugName("ybts-command");
    debugChain(&__plugin);
    m_transport.setDebugPtr(this,this);
}

bool YBTSCommand::send(const String& str)
{
    if (m_transport.send(str.c_str(),str.length()))
	return true;
    __plugin.restart();
    return false;
}

bool YBTSCommand::recv(String& str)
{
    uint32_t t = Time::secNow() + 10;
    while (!Thread::check(false)) {
	int rd = m_transport.recv();
	if (rd > 0) {
	    str = (const char*)m_transport.readBuf().data();
	    return true;
	}
	if (!rd) {
	    if (!m_transport.canSelect())
		Thread::idle();
	    if (Time::secNow() < t)
		continue;
	}
	// Socket non retryable error
	__plugin.restart();
	return false;
    }
    return false;
}

bool YBTSCommand::start()
{
    stop();
    if (!m_transport.initTransport(false,s_bufLenLog,true))
	return false;
    Debug(this,DebugInfo,"Started [%p]",this);
    return true;
}

void YBTSCommand::stop()
{
    if (!m_transport.m_socket.valid())
	return;
    m_transport.resetTransport();
    Debug(this,DebugInfo,"Stopped [%p]",this);
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
    m_hbIntervalMs(YBTS_HB_INTERVAL_DEF),
    m_hbTimeoutMs(YBTS_HB_TIMEOUT_DEF)
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
	static uint8_t buf[2] = {SigHeartbeat,0};
	DDebug(this,DebugAll,"Sending heartbeat [%p]",this);
	lck.drop();
	bool ok = m_transport.send(buf,2);
	lck.acquire(this);
	if (ok)
	    setHeartbeatTime(time);
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
	YBTSMessage m(SigConnRelease,0,connId);
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
		if (m->primitive() != SigHeartbeat)
		    res = handlePDU(*m);
		else
		    DDebug(this,DebugAll,"Received heartbeat [%p]",this);
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
	if (!m_transport.canSelect())
	    Thread::idle();
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
    s << "\r\nheartbeat_timeout=" << m_hbTimeoutMs;
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
	    return true;
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

bool YBTSSignalling::findConnInternal(RefPointer<YBTSConn>& conn, const YBTSUE* ue)
{
    conn = 0;
    for (ObjList* o = m_conns.skipNull(); o; o = o->skipNext()) {
	YBTSConn* c = static_cast<YBTSConn*>(o->get());
	if (c->ue() == ue) {
	    conn = c;
	    return true;
	}
    }
    return false;
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
	case SigL3Message:
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
	case SigHandshake:
	    return handleHandshake(msg);
	case SigRadioReady:
	    __plugin.radioReady();
	    return Ok;
	case SigConnLost:
	    __plugin.signalling()->dropConn(msg.connId(),false);
	    return Ok;
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
	    YBTSMessage m(SigHandshake);
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
	else if (!rd) {
	    if (!m_transport.canSelect())
		Thread::idle();
	}
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
// YBTSUE
//

// Start paging, return true if already paging
bool YBTSUE::startPaging(BtsPagingChanType type)
{
    Lock lck(this);
    m_pageCnt++;
    if (m_paging)
	return true;
    YBTSSignalling* sig = __plugin.signalling();
    if (!sig)
	return false;
    String tmp;
    if (tmsi())
	tmp << "TMSI" << tmsi();
    else if (imsi())
	tmp << "IMSI" << imsi();
    else if (imei())
	tmp << "IMEI" << imei();
    else
	return false;
    lck.drop();
    YBTSMessage m(SigStartPaging,(uint8_t)type,0,new XmlElement("identity",tmp));
    if (sig->send(m)) {
	Debug(&__plugin,DebugAll,"Started paging %s",tmp.c_str());
	lck.acquire(this);
	if (!m_paging)
	    m_paging = tmp;
	return true;
    }
    return false;
}

// Stop paging when request counter drops to zero
void YBTSUE::stopPaging()
{
    lock();
    bool stop = !--m_pageCnt;
    unlock();
    if (stop)
	stopPagingNow();
}

// Stop paging immediately
void YBTSUE::stopPagingNow()
{
    YBTSSignalling* sig = __plugin.signalling();
    if (!sig || !m_paging)
	return;
    lock();
    String tmp = m_paging;
    unlock();
    if (!tmp)
	return;
    YBTSMessage m(SigStopPaging,0,0,new XmlElement("identity",tmp));
    if (sig->send(m)) {
	Debug(&__plugin,DebugAll,"Stopped paging %s",tmp.c_str());
	lock();
	if (m_paging == tmp)
	    m_paging.clear();
	unlock();
    }
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
    m_ueHashLen(hashLen ? hashLen : 17),
    m_saveUEs(false)
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

void YBTSMM::newTMSI(String& tmsi)
{
    do {
	uint32_t t = ++m_tmsiIndex;
	// TODO: use NNSF compatible TMSI allocation
	//  bits 31-30: 11=P-TMSI else TMSI
	//  bits 29-24: local allocation
	//  bits 23-... (0-10 bits, 0-8 for LTE compatibility): Node Identity
	//  bits ...-0 (14-24 bits, 16-24 for LTE compatibility): local allocation
	if ((t & 0xc000000) == 0xc000000)
	    m_tmsiIndex = (t &= 0x3fffffff);
	uint8_t buf[4];
	buf[0] = (uint8_t)(t >> 24);
	buf[1] = (uint8_t)(t >> 16);
	buf[2] = (uint8_t)(t >> 8);
	buf[3] = (uint8_t)t;
	tmsi.hexify(buf,4);
    } while (m_ueTMSI[hashList(tmsi)].find(tmsi));
    saveUEs();
}

void YBTSMM::loadUElist()
{
    s_globalMutex.lock();
    String f = s_ueFile;
    s_globalMutex.unlock();
    if (!f)
	return;
    Configuration ues(f);
    if (!ues.load(false))
	return;
    m_tmsiIndex = ues.getIntValue("tmsi","index");
    NamedList* tmsis = ues.getSection("ues");
    if (!tmsis)
	return;
    int cnt = 0;
    for (ObjList* l = tmsis->paramList()->skipNull(); l; l = l->skipNext()) {
	const NamedString* s = static_cast<const NamedString*>(l->get());
	if (s->name().length() != 8) {
	    Debug(this,DebugMild,"Invalid TMSI '%s' in file '%s'",s->name().c_str(),f.c_str());
	    continue;
	}
	ObjList* p = s->split(',');
	if (p && (p->count() >= 5) && p->at(0)->toString()) {
	    YBTSUE* ue = new YBTSUE(p->at(0)->toString(),s->name());
	    ue->m_imei = p->at(1)->toString();
	    ue->m_msisdn = p->at(2)->toString();
	    ue->m_expires = p->at(3)->toString().toInt64();
	    ue->m_registered = p->at(4)->toString().toBoolean();
	    m_ueIMSI[hashList(ue->imsi())].append(ue);
	    m_ueTMSI[hashList(ue->tmsi())].append(ue)->setDelete(false);
	    cnt++;
	}
	else
	    Debug(this,DebugMild,"Invalid record for TMSI '%s' in file '%s'",s->name().c_str(),f.c_str());
	TelEngine::destruct(p);
    }
    Debug(this,DebugNote,"Loaded %d TMSI records, index=%u",cnt,m_tmsiIndex);
}

void YBTSMM::saveUElist()
{
    s_globalMutex.lock();
    String f = s_ueFile;
    s_globalMutex.unlock();
    if (!f)
	return;
    Configuration ues;
    ues = f;
    ues.setValue(YSTRING("tmsi"),"index",(int)m_tmsiIndex);
    int cnt = 0;
    NamedList* tmsis = ues.createSection(YSTRING("ues"));
    ObjList* list = tmsis->paramList();
    for (unsigned int i = 0; i < m_ueHashLen; i++) {
	for (ObjList* l = m_ueTMSI[i].skipNull(); l; l = l->skipNext()) {
	    YBTSUE* ue = static_cast<YBTSUE*>(l->get());
	    String s;
	    s << ue->imsi() << "," << ue->imei() << "," << ue->msisdn() << "," << ue->expires() << "," << ue->registered();
	    list = list->append(new NamedString(ue->tmsi(),s));
	    cnt++;
	}
    }
    ues.save();
    Debug(this,DebugNote,"Saved %d TMSI records, index=%u",cnt,m_tmsiIndex);
}

void YBTSMM::updateExpire(YBTSUE* ue)
{
    uint32_t exp = s_tmsiExpire;
    if (!ue || !exp)
	return;
    exp += Time::secNow();
    if (exp <= ue->expires())
	return;
    ue->m_expires = exp;
    saveUEs();
    DDebug(this,DebugAll,"Updated TMSI=%s IMSI=%s expiration time",ue->tmsi().c_str(),ue->imsi().c_str());
}

void YBTSMM::checkTimers(const Time& time)
{
    uint32_t exp = time.sec();
    Lock mylock(m_ueMutex);
    for (unsigned int i = 0; i < m_ueHashLen; i++) {
	for (ObjList* l = m_ueTMSI[i].skipNull(); l; ) {
	    YBTSUE* ue = static_cast<YBTSUE*>(l->get());
	    if (ue->expires() && ue->expires() < exp) {
		l->remove();
		l = l->skipNull();
		saveUEs();
	    }
	    else
		l = l->skipNext();
	}
    }
    if (m_saveUEs) {
	m_saveUEs = false;
	saveUElist();
    }
}

void YBTSMM::handlePDU(YBTSMessage& m, YBTSConn* conn)
{
    XmlElement* ch = m.xml() ? m.xml()->findFirstChild(&s_message) : 0;
    if (!ch) {
	Debug(this,DebugNote,"Empty xml in %s [%p]",m.name(),this);
	return;
    }
    const String* t = ch->getAttribute(s_type);
    if (!t)
	Debug(this,DebugWarn,"Missing 'type' in %s [%p]",m.name(),this);
    else if (*t == YSTRING("LocationUpdatingRequest"))
	handleLocationUpdate(m,*ch,conn);
    else if (*t == YSTRING("TMSIReallocationComplete"))
	handleUpdateComplete(m,*ch,conn);
    else if (*t == YSTRING("IMSIDetachIndication"))
	handleIMSIDetach(m,*ch,conn);
    else if (*t == YSTRING("CMServiceRequest"))
	handleCMServiceRequest(m,*ch,conn);
    else if (*t == YSTRING("CMServiceAbort"))
	__plugin.signalling()->dropConn(m.connId(),true);
    else if (*t == YSTRING("IdentityResponse"))
	handleIdentityResponse(m,*ch,conn);
    else
	// TODO: send MM status
	Debug(this,DebugNote,"Unhandled '%s' in %s [%p]",t->c_str(),m.name(),this);
}

void YBTSMM::handleIdentityResponse(YBTSMessage& m, const XmlElement& xml, YBTSConn* conn)
{
    if (!conn) {
	Debug(this,DebugGoOn,"Oops! IdentityResponse conn=%u: no connection [%p]",
	    m.connId(),this);
	__plugin.signalling()->dropConn(m.connId(),true);
	return;
    }
    XmlElement* xid = xml.findFirstChild(&s_mobileIdent);
    if (!xid)
	return;
    xid = xid->findFirstChild();
    if (!xid)
	return;
    const String& tag = xid->getTag();
    const String& ident = xid->getText();
    RefPointer<YBTSUE> ue = conn->ue();
    if (tag == YSTRING("IMSI")) {
	if (ue) {
	    if (ue->imsi() != ident) {
		Debug(this,DebugWarn,"Got IMSI change %s -> %s on conn=%u [%p]",
		    ue->imsi().c_str(),ident.c_str(),m.connId(),this);
		__plugin.signalling()->dropConn(conn,true);
	    }
	}
	else {
	    getUEByIMSISafe(ue,ident);
	    conn->lock();
	    bool ok = conn->setUE(ue);
	    conn->unlock();
	    if (!ok) {
		Debug(this,DebugGoOn,"Failed to set UE in conn=%u [%p]",m.connId(),this);
		__plugin.signalling()->dropConn(conn,true);
	    }
	    return;
	}
    }
    else {
	if (!ue) {
	    Debug(this,DebugWarn,"Got identity %s=%s but have no UE attached on conn=%u [%p]",
		tag.c_str(),ident.c_str(),m.connId(),this);
	    __plugin.signalling()->dropConn(conn,true);
	    return;
	}
    }
    if (tag == YSTRING("IMEI"))
	ue->m_imei = ident;
    XmlElement* x = conn->takeXml();
    if (x) {
	YBTSMessage m2(SigL3Message,m.info(),m.connId(),x);
	handlePDU(m2,conn);
    }
}

// Handle location updating requests
void YBTSMM::handleLocationUpdate(YBTSMessage& m, const XmlElement& xml, YBTSConn* conn)
{
    if (!conn) {
	Debug(this,DebugGoOn,"Rejecting LocationUpdatingRequest conn=%u: no connection [%p]",
	    m.connId(),this);
	sendLocationUpdateReject(m,0,CauseProtoError);
	return;
    }
    RefPointer<YBTSUE> ue = conn->ue();
    if (!ue) {
	XmlElement* laiXml = 0;
	XmlElement* identity = 0;
	findXmlChildren(xml,laiXml,s_locAreaIdent,identity,s_mobileIdent);
	if (!(laiXml && identity)) {
	    Debug(this,DebugNote,
		"Rejecting LocationUpdatingRequest conn=%u: missing LAI or mobile identity [%p]",
		m.connId(),this);
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
	Debug(this,DebugAll,"Handling LocationUpdatingRequest conn=%u: ident=%s/%s LAI=%s [%p]",
	    conn->connId(),(haveTMSI ? "TMSI" : "IMSI"),
	    ident->c_str(),lai.lai().c_str(),this);
	// TODO: handle concurrent requests, check if we have a pending location updating
	// This should never happen, but we should handle it
	if (haveTMSI) {
	    // Got TMSI
	    // Same LAI: check if we know the IMSI, request it if unknown
	    // Different LAI: request IMSI
	    if (haveLAI)
		findUEByTMSISafe(ue,*ident);
	}
	else
	    // Got IMSI: Create/retrieve TMSI
	    getUEByIMSISafe(ue,*ident);
	if (!ue) {
	    if (!conn->setXml(m.takeXml())) {
		Debug(this,DebugNote,"Rejecting LocationUpdatingRequest: cannot postpone request [%p]",this);
		sendLocationUpdateReject(m,conn,CauseServNotSupp);
		return;
	    }
	    sendIdentityRequest(conn,"IMSI");
	    return;
	}
	conn->lock();
	cause = setConnUE(*conn,ue,xml);
	conn->unlock();
	if (cause) {
	    sendLocationUpdateReject(m,conn,cause);
	    return;
	}
    }
    if (s_askIMEI && ue->imei().null()) {
	if (conn->setXml(m.takeXml())) {
	    sendIdentityRequest(conn,"IMEI");
	    return;
	}
    }
    Message msg("user.register");
    msg.addParam("driver",__plugin.name());
    msg.addParam("username",ue->imsi());
    msg.addParam("number",ue->msisdn(),false);
    msg.addParam("imei",ue->imei(),false);
    msg.addParam("tmsi",ue->tmsi(),false);
    if (!Engine::dispatch(msg)) {
	sendLocationUpdateReject(m,conn,CauseServNotSupp);
	return;
    }
    updateExpire(ue);
    Lock lckUE(ue);
    ue->m_registered = true;
    saveUEs();
    XmlElement* ch = 0;
    XmlElement* mm = buildMM(ch,"LocationUpdatingAccept");
    if (ch) {
	ch->addChildSafe(__plugin.signalling()->lai().build());
	ch->addChildSafe(buildXmlWithChild(s_mobileIdent,s_tmsi,ue->tmsi()));
    }
    lckUE.drop();
    conn->sendL3(mm);
}

// Handle location update (TMSI reallocation) complete
void YBTSMM::handleUpdateComplete(YBTSMessage& m, const XmlElement& xml, YBTSConn* conn)
{
    if (!conn) {
	Debug(this,DebugGoOn,"Rejecting TMSIReallocationComplete conn=%u: no connection [%p]",
	    m.connId(),this);
	sendLocationUpdateReject(m,0,CauseProtoError);
	return;
    }
    __plugin.signalling()->dropConn(conn,true);
}

// Handle IMSI detach indication
void YBTSMM::handleIMSIDetach(YBTSMessage& m, const XmlElement& xml, YBTSConn* conn)
{
    if (!conn) {
	Debug(this,DebugGoOn,"Ignoring IMSIDetachIndication conn=%u: no connection [%p]",
	    m.connId(),this);
	return;
    }
    __plugin.signalling()->dropConn(conn,true);

    const XmlElement* xid = xml.findFirstChild(&s_mobileIdent);
    if (!xid)
	return;
    bool found = false;
    bool haveTMSI = false;
    const String* ident = getIdentTIMSI(*xid,haveTMSI,found);
    if (TelEngine::null(ident))
	return;

    RefPointer<YBTSUE> ue;
    if (haveTMSI)
	findUEByTMSISafe(ue,*ident);
    else
	getUEByIMSISafe(ue,*ident,false);
    if (!ue)
	return;
    ue->stopPagingNow();
    if (!ue->registered())
	return;
    Lock lckUE(ue);
    ue->m_registered = false;
    updateExpire(ue);
    saveUEs();
    if (!ue->imsi())
	return;

    Message* msg = new Message("user.unregister");
    msg->addParam("driver",__plugin.name());
    msg->addParam("username",ue->imsi());
    msg->addParam("number",ue->msisdn(),false);
    msg->addParam("imei",ue->imei(),false);
    msg->addParam("tmsi",ue->tmsi(),false);
    Engine::enqueue(msg);
}

void YBTSMM::handleCMServiceRequest(YBTSMessage& m, const XmlElement& xml, YBTSConn* conn)
{
    if (!conn) {
	Debug(this,DebugGoOn,"Rejecting CMServiceRequest conn=%u: no connection [%p]",
	    m.connId(),this);
	sendCMServiceRsp(m,0,CauseProtoError);
	return;
    }
    RefPointer<YBTSUE> ue = conn->ue();
    if (!ue) {
	XmlElement* cmServType = 0;
	XmlElement* identity = 0;
	findXmlChildren(xml,cmServType,s_cmServType,identity,s_mobileIdent);
	if (!(cmServType && identity)) {
	    Debug(this,DebugNote,
		"Rejecting CMServiceRequest conn=%u: missing service type or mobile identity [%p]",
		m.connId(),this);
	    sendCMServiceRsp(m,conn,CauseInvalidIE);
	    return;
	}
	// TODO: Properly check service type, reject if not supported
	const String& type = cmServType->getText();
	if (type == s_cmMOCall)
	    ;
	else {
	    Debug(this,DebugNote,
		"Rejecting CMServiceRequest conn=%u: service type '%s' not supported/subscribed [%p]",
		m.connId(),type.c_str(),this);
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
	Debug(this,DebugAll,"Handling CMServiceRequest conn=%u: ident=%s/%s type=%s [%p]",
	    conn->connId(),(haveTMSI ? "TMSI" : "IMSI"),
	    ident->c_str(),type.c_str(),this);
	if (haveTMSI)
	    // Got TMSI
	    findUEByTMSISafe(ue,*ident);
	else
	    // Got IMSI: Create/retrieve TMSI
	    getUEByIMSISafe(ue,*ident);
	if (!ue) {
	    if (!conn->setXml(m.takeXml())) {
		Debug(this,DebugNote,"Rejecting CMServiceRequest: cannot postpone request [%p]",this);
		sendCMServiceRsp(m,conn,CauseServNotSupp);
	    }
	    sendIdentityRequest(conn,"IMSI");
	    return;
	}
	bool dropConn = false;
	conn->lock();
	cause = setConnUE(*conn,ue,xml,&dropConn);
	conn->unlock();
	if (cause) {
	    sendCMServiceRsp(m,conn,cause);
	    if (dropConn)
		__plugin.signalling()->dropConn(conn,true);
	    return;
	}
    }
    ue->stopPagingNow();
    sendCMServiceRsp(m,conn,0);
    __plugin.checkMtService(ue,conn);
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
    __plugin.signalling()->dropConn(msg.connId(),true);
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

// Send an Identity Request message
void YBTSMM::sendIdentityRequest(YBTSConn* conn, const char* type)
{
    if (TelEngine::null(type) || !conn)
	return;
    XmlElement* ch = 0;
    XmlElement* mm = buildMM(ch,"IdentityRequest");
    if (ch)
	ch->addChildSafe(new XmlElement("IdentityType",type));
    conn->sendL3(mm);
}

bool YBTSMM::findUESafe(RefPointer<YBTSUE>& ue, const String& dest)
{
    ue = 0;
    String tmp(dest);
    if (tmp.startSkip("+",false))
	findUEByMSISDNSafe(ue,tmp);
    else if (tmp.startSkip("IMSI",false))
	getUEByIMSISafe(ue,tmp,false);
    else if (tmp.startSkip("IMEI",false))
	findUEByIMEISafe(ue,tmp);
    else if (tmp.startSkip("TMSI",false))
	findUEByTMSISafe(ue,tmp);
    return ue != 0;
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

// Find UE by IMEI
void YBTSMM::findUEByIMEISafe(RefPointer<YBTSUE>& ue, const String& imei)
{
    if (!imei)
	return;
    Lock lck(m_ueMutex);
    for (unsigned int n = 0; n < m_ueHashLen; n++) {
	ObjList& list = m_ueTMSI[n];
	for (ObjList* o = list.skipNull(); o ; o = o->skipNext()) {
	    YBTSUE* u = static_cast<YBTSUE*>(o->get());
	    if (imei == u->imei()) {
		ue = u;
		return;
	    }
	}
    }
}

// Find UE by MSISDN
void YBTSMM::findUEByMSISDNSafe(RefPointer<YBTSUE>& ue, const String& msisdn)
{
    if (!msisdn)
	return;
    Lock lck(m_ueMutex);
    for (unsigned int n = 0; n < m_ueHashLen; n++) {
	ObjList& list = m_ueTMSI[n];
	for (ObjList* o = list.skipNull(); o ; o = o->skipNext()) {
	    YBTSUE* u = static_cast<YBTSUE*>(o->get());
	    if (msisdn == u->msisdn()) {
		ue = u;
		return;
	    }
	}
    }
}

// Find UE by IMSI. Create it if not found
void YBTSMM::getUEByIMSISafe(RefPointer<YBTSUE>& ue, const String& imsi, bool create)
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
    if (create && !tmpUE) {
	String tmsi;
	newTMSI(tmsi);
	Debug(this,DebugInfo,"Creating new record for TMSI=%s IMSI=%s",tmsi.c_str(),imsi.c_str());
	tmpUE = new YBTSUE(imsi,tmsi);
	list.append(tmpUE);
	m_ueTMSI[hashList(tmsi)].append(tmpUE)->setDelete(false);
	Debug(this,DebugInfo,"Added UE imsi=%s tmsi=%s [%p]",
	    tmpUE->imsi().c_str(),tmpUE->tmsi().c_str(),this);
	saveUEs();
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
    const String* type = request.getAttribute(s_type);
    Debug(this,DebugNote,"Rejecting %s conn=%u: %s IMSI/TMSI [%p]",
	(type ? type->c_str() : "unknown"),m.connId(),(found ? "empty" : "missing"),this);
    return CauseInvalidIE;
}

// Set UE for a connection
uint8_t YBTSMM::setConnUE(YBTSConn& conn, YBTSUE* ue, const XmlElement& req,
    bool* dropConn)
{
    const String* type = req.getAttribute(s_type);
    if (!ue) {
	Debug(this,DebugGoOn,"Rejecting %s: no UE object [%p]",
	    (type ? type->c_str() : "unknown"),this);
	return CauseProtoError;
    }
    if (conn.setUE(ue))
	return 0;
    Debug(this,DebugGoOn,"Rejecting %s: UE mismatch on connection %u [%p]",
	(type ? type->c_str() : "unknown"),conn.connId(),this);
    if (dropConn)
	*dropConn = true;
    return CauseProtoError;
}


//
// YBTSCallDesc
//
// Incoming
YBTSCallDesc::YBTSCallDesc(YBTSChan* chan, const XmlElement& xml, bool regular, const String* callRef)
    : YBTSConnIdHolder(chan->connId()),
    m_state(Null),
    m_incoming(true),
    m_regular(regular),
    m_timeout(0),
    m_relSent(0),
    m_chan(chan)
{
    if (callRef) {
	m_callRef = *callRef;
	*this << prefix(false) << m_callRef;
    }
    for (const ObjList* o = xml.getChildren().skipNull(); o; o = o->skipNext()) {
	XmlElement* x = static_cast<XmlChild*>(o->get())->xmlElement();
	if (!x)
	    continue;
	const String& s = x->getTag();
	if (s == s_ccCalled)
	    m_called = x->getText();
    }
}

// Outgoing
YBTSCallDesc::YBTSCallDesc(YBTSChan* chan, const XmlElement* xml, const String& callRef)
    : YBTSConnIdHolder(chan->connId()),
    m_state(Null),
    m_incoming(false),
    m_regular(true),
    m_timeout(0),
    m_relSent(0),
    m_chan(chan),
    m_callRef(callRef)
{
    if (!xml)
	return;
    // TODO: pick relevant info from XML
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
    XmlElement* cc = __plugin.buildCC(ch,tag,callRef(),m_incoming);
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
void YBTSCallDesc::sendGSMRel(bool rel, const String& callRef, bool tiFlag, const char* reason,
    uint16_t connId)
{
    XmlElement* ch = 0;
    XmlElement* cc = __plugin.buildCC(ch,rel ? s_ccRel : s_ccRlc,callRef,tiFlag);
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
    m_mutex(true,"YBTSChan"),
    m_conn(conn),
    m_traffic(0),
    m_cref(0),
    m_dtmf(0),
    m_mpty(false),
    m_hungup(false),
    m_paging(false),
    m_haveTout(false),
    m_tout(0),
    m_activeCall(0),
    m_pending(0)
{
    if (!m_conn)
	return;
    m_address = m_conn->ue()->imsi();
    Debug(this,DebugCall,"Incoming imsi=%s conn=%u [%p]",
	m_address.c_str(),connId(),this);
}

// Outgoing
YBTSChan::YBTSChan(YBTSUE* ue, YBTSConn* conn)
    : Channel(__plugin,0,true),
    m_mutex(true,"YBTSChan"),
    m_conn(conn),
    m_ue(ue),
    m_traffic(0),
    m_cref(0),
    m_dtmf(0),
    m_mpty(false),
    m_hungup(false),
    m_paging(false),
    m_haveTout(false),
    m_tout(0),
    m_activeCall(0),
    m_pending(0)
{
    if (!m_ue)
	return;
    m_address = ue->imsi();
    Debug(this,DebugCall,"Outgoing imsi=%s [%p]",m_address.c_str(),this);
}

// Init incoming chan. Return false to destruct the channel
bool YBTSChan::initIncoming(const XmlElement& xml, bool regular, const String* callRef)
{
    if (!ue())
	return false;
    Lock lck(driver());
    YBTSCallDesc* call = handleSetup(xml,regular,callRef);
    if (!call)
	return false;
    String caller;
    if (ue()->msisdn())
	caller << "+" << ue()->msisdn();
    else if (ue()->imsi())
	caller << "IMSI" << ue()->imsi();
    else if (ue()->imei())
	caller << "IMEI" << ue()->imei();
    Message* r = message("call.preroute");
    r->addParam("caller",caller,false);
    r->addParam("called",call->m_called,false);
    r->addParam("username",ue()->imsi(),false);
    r->addParam("imei",ue()->imei(),false);
    r->addParam("emergency",String::boolText(!call->m_regular));
    if (xml.findFirstChild(&s_ccSsCLIR))
	r->addParam("privacy",String::boolText(true));
    else if (xml.findFirstChild(&s_ccSsCLIP))
	r->addParam("privacy",String::boolText(false));
    Message* s = message("chan.startup");
    s->addParam("caller",caller,false);
    s->addParam("called",call->m_called,false);
    lck.drop();
    Engine::enqueue(s);
    initChan();
    return startRouter(r);
}

// Init outgoing chan. Return false to destruct the channel
bool YBTSChan::initOutgoing(Message& msg)
{
    if (m_pending || !ue())
	return false;
/*
    XmlElement* xml = 
    Lock lck(this);
    if (m_pending) {
	TelEngine::destruct(xml);
	return false;
    }
    m_pending = xml;
    lck.drop();
*/
    if (!m_conn)
	m_paging = ue()->startPaging(ChanTypeVoice);
    initChan();
    startMT();
    return true;
}

// Handle CC messages
bool YBTSChan::handleCC(const XmlElement& xml, const String* callRef, bool tiFlag)
{
    const String* type = xml.getAttribute(s_type);
    if (!type) {
	Debug(this,DebugWarn,"Missing 'type' attribute [%p]",this);
	return true;
    }
    if (TelEngine::null(callRef)) {
	Debug(this,DebugNote,"%s with empty transaction identifier [%p]",
	    type->c_str(),this);
	return true;
    }
    bool regular = (*type == s_ccSetup);
    bool emergency = !regular && (*type == s_ccEmergency);
    Lock lck(m_mutex);
    String cref;
    cref << YBTSCallDesc::prefix(tiFlag) << *callRef;
    ObjList* o = m_calls.find(cref);
    DDebug(this,DebugAll,"Handling '%s' in call %s (%p) [%p]",type->c_str(),cref.c_str(),o,this);
    if (!o) {
	lck.drop();
	if (regular || emergency) {
	    handleSetup(xml,regular,callRef);
	    return true;
	}
	return false;
    }
    YBTSCallDesc* call = static_cast<YBTSCallDesc*>(o->get());
    if (regular || emergency)
	call->sendWrongState();
    else if (*type == s_ccRel || *type == s_ccRlc || *type == s_ccDisc) {
	Debug(this,DebugInfo,"Removing call '%s' [%p]",call->c_str(),this);
	if (m_activeCall == call)
	    m_activeCall = 0;
	getCCCause(call->m_reason,xml);
	String reason = call->m_reason;
	bool final = (*type != s_ccDisc);
	if (final) {
	    if (*type == s_ccRel)
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
    else if (*type == s_ccConnectAck) {
	if (call->m_state == YBTSCallDesc::ConnectReq) {
	    call->changeState(YBTSCallDesc::Active);
	    call->m_timeout = 0;
	}
	else
	    call->sendWrongState();
    }
    else if (*type == s_ccStatusEnq)
	call->sendStatus("status-enquiry-rsp");
    else if (*type == s_ccStatus) {
	String cause, cs;
	getCCCause(cause,xml);
	getCCCallState(cs,xml);
	Debug(this,(cause != "status-enquiry-rsp") ? DebugWarn : DebugAll,
	    "Received status cause='%s' call_state='%s' [%p]",
	    cause.c_str(),cs.c_str(),this);
    }
    else if (*type == s_ccStartDTMF) {
	const String* dtmf = xml.childText(s_ccKeypadFacility);
	if (m_dtmf) {
	    Debug(this,DebugMild,"Received DTMF '%s' while still in '%c' [%p]",
		TelEngine::c_str(dtmf),m_dtmf,this);
	    call->sendCC(YSTRING("StartDTMFReject"),buildCCCause("wrong-state-message"));
	}
	else if (TelEngine::null(dtmf)) {
	    Debug(this,DebugMild,"Received no %s in %s [%p]",
		s_ccKeypadFacility.c_str(),s_ccStartDTMF.c_str(),this);
	    call->sendCC(YSTRING("StartDTMFReject"),buildCCCause("missing-mandatory-ie"));
	}
	else {
	    m_dtmf = dtmf->at(0);
	    call->sendCC(YSTRING("StartDTMFAck"),new XmlElement(s_ccKeypadFacility,*dtmf));
	    Message* msg = message("chan.dtmf");
	    msg->addParam("text",*dtmf);
	    msg->addParam("detected","gsm");
	    dtmfEnqueue(msg);
	}
    }
    else if (*type == s_ccStopDTMF) {
	m_dtmf = 0;
	call->sendCC(YSTRING("StopDTMFAck"));
    }
    else if (*type == s_ccHold) {
	if (!m_mpty)
	    call->sendCC(YSTRING("HoldReject"),buildCCCause("service-unavailable"));
	else {
	    // TODO
	}
    }
    else if (*type == s_ccRetrieve) {
	if (!m_mpty)
	    call->sendCC(YSTRING("RetrieveReject"),buildCCCause("service-unavailable"));
	else {
	    // TODO
	}
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
    m_activeCall = 0;
    m_calls.clear();
    setReason("net-out-of-order");
    lck.drop();
    hangup();
}

// Check if MT call can be accepted
bool YBTSChan::canAcceptMT()
{
    if (m_pending || !(m_mpty && m_conn))
	return false;
    Lock lck(m_mutex);
    for (ObjList* l = m_calls.skipNull(); l; l = l->skipNext())
	if (!static_cast<YBTSCallDesc*>(l->get())->active())
	    return false;
    return true;
}

// Start a pending MT call
void YBTSChan::startMT(YBTSConn* conn)
{
    Lock lck(m_mutex);
    if (conn && !m_conn)
	m_conn = conn;
    else
	conn = m_conn;
    if (!conn)
	return;
    XmlElement* xml = m_pending;
    m_pending = 0;
    if (!xml)
	return;
    if (m_cref >= 7) {
	Debug(this,DebugWarn,"Could not allocate new call ref [%p]",this);
	return;
    }
    String cref;
    cref << YBTSCallDesc::prefix(true) << (uint16_t)m_cref++;
    YBTSCallDesc* call = new YBTSCallDesc(this,xml,cref);
    m_calls.append(call);
    lck.drop();
    call->sendCC(s_ccSetup,xml);
    YBTSMessage m(SigL3Message,0,conn->connId(),xml);
    __plugin.signalling()->send(m);
}

void YBTSChan::allocTraffic()
{
    if (m_conn) {
	YBTSMessage m(SigAllocMedia,0,m_conn->connId());
	__plugin.signalling()->send(m);
    }
}

void YBTSChan::startTraffic(uint8_t mode)
{
    if (mode == m_traffic)
	return;
    if (!m_conn)
	return;
    uint16_t cid = 0;
    m_mutex.lock();
    bool send = m_conn && (mode != m_traffic);
    if (send)
	cid = m_conn->connId();
    m_traffic = mode;
    m_mutex.unlock();
    if (send) {
	YBTSMessage m(SigStartMedia,mode,cid);
	__plugin.signalling()->send(m);
    }
}

YBTSCallDesc* YBTSChan::handleSetup(const XmlElement& xml, bool regular, const String* callRef)
{
    const String* type = xml.getAttribute(s_type);
    Lock lck(m_mutex);
    YBTSCallDesc* call = new YBTSCallDesc(this,xml,regular,callRef);
    if (call->null()) {
	TelEngine::destruct(call);
	Debug(this,DebugNote,"%s with empty call ref [%p]",(type ? type->c_str() : "unknown"),this);
	return 0;
    }
    allocTraffic();
    if (!m_calls.skipNull()) {
	call->proceeding();
	m_calls.append(call);
	if (!m_activeCall)
	    m_activeCall = call;
	Debug(this,DebugInfo,"Added call '%s' [%p]",call->c_str(),this);
	startTraffic();
	return call;
    }
    lck.drop();
    Debug(this,DebugNote,"Refusing subsequent call '%s' [%p]",call->c_str(),this);
    call->releaseComplete();
    TelEngine::destruct(call);
    return 0;
}

void YBTSChan::hangup(const char* reason, bool final)
{
    ObjList calls;
    Lock lck(m_mutex);
    setReason(reason);
    m_activeCall = 0;
    while (GenObject* o = m_calls.remove(false))
	calls.append(o);
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
    YBTSCallDesc* call = activeCall();
    if (!call)
	return false;
    if (call->incoming())
	call->progressing(buildProgressIndicator(msg));
    return true;
}

bool YBTSChan::msgRinging(Message& msg)
{
    Channel::msgRinging(msg);
    Lock lck(m_mutex);
    if (m_hungup)
	return false;
    YBTSCallDesc* call = activeCall();
    if (!call)
	return false;
    if (call->incoming() && call->m_state == YBTSCallDesc::CallProceeding)
	call->alert(buildProgressIndicator(msg));
    return true;
}

bool YBTSChan::msgAnswered(Message& msg)
{
    Channel::msgAnswered(msg);
    Lock lck(m_mutex);
    if (m_hungup)
	return false;
    YBTSCallDesc* call = activeCall();
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
		if (m_activeCall == call)
		    m_activeCall = 0;
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
    if (m_paging && ue())
	ue()->stopPaging();
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
    m_peerAlive(false),
    m_peerCheckTime(0),
    m_peerCheckIntervalMs(YBTS_PEERCHECK_DEF),
    m_stop(false),
    m_stopTime(0),
    m_restart(false),
    m_restartTime(0),
    m_logTrans(0),
    m_logBts(0),
    m_command(0),
    m_media(0),
    m_signalling(0),
    m_mm(0),
    m_haveCalls(false),
    m_engineStart(false),
    m_engineStop(0),
    m_helpCache(0)
{
    Output("Loaded module YBTS");
}

YBTSDriver::~YBTSDriver()
{
    Output("Unloading module YBTS");
    TelEngine::destruct(m_logTrans);
    TelEngine::destruct(m_logBts);
    TelEngine::destruct(m_command);
    TelEngine::destruct(m_media);
    TelEngine::destruct(m_signalling);
    TelEngine::destruct(m_mm);
    TelEngine::destruct(m_helpCache);
}

XmlElement* YBTSDriver::buildCC(XmlElement*& ch, const char* type, const char* callRef, bool tiFlag)
{
    XmlElement* mm = buildCC();
    XmlElement* cr = static_cast<XmlElement*>(mm->addChildSafe(new XmlElement(s_ccCallRef,callRef)));
    if (cr)
	cr->setAttribute(s_ccTIFlag,String::boolText(tiFlag));
    ch = static_cast<XmlElement*>(mm->addChildSafe(new XmlElement(s_message)));
    if (ch)
	ch->setAttribute(s_type,type);
    return mm;
}

// Handle call control messages
void YBTSDriver::handleCC(YBTSMessage& m, YBTSConn* conn)
{
    XmlElement* xml = m.xml() ? m.xml()->findFirstChild(&s_message) : 0;
    if (!xml) {
	Debug(this,DebugNote,"Empty xml in %s [%p]",m.name(),this);
	return;
    }
    const String* type = xml->getAttribute(s_type);
    if (!type) {
	Debug(this,DebugWarn,"Missing 'type' in %s [%p]",m.name(),this);
	return;
    }
    const String* callRef = 0;
    bool tiFlag = false;
    const XmlElement* tid = m.xml()->findFirstChild(&s_ccCallRef);
    if (tid) {
	callRef = &tid->getText();
	if (TelEngine::null(callRef))
	    callRef = 0;
	const String* attr = tid->getAttribute(s_ccTIFlag);
	if (attr)
	    tiFlag = attr->toBoolean();
    }
    if (conn) {
	RefPointer<YBTSChan> chan;
	findChan(conn->connId(),chan);
	if (chan && chan->handleCC(*xml,callRef,tiFlag))
	    return;
    }
    String cref;
    if (callRef)
	cref << YBTSCallDesc::prefix(tiFlag) << *callRef;
    else
	cref = "with no reference";
    DDebug(this,DebugAll,"Handling '%s' in call %s conn=%u [%p]",
	type->c_str(),cref.c_str(),m.connId(),this);
    bool regular = (*type == s_ccSetup);
    bool emergency = !regular && (*type == s_ccEmergency);
    if (regular || emergency) {
	if (!conn)
	    Debug(this,DebugNote,"Refusing new GSM call, no connection");
	else if (!conn->ue())
	    Debug(this,DebugNote,
		"Refusing new GSM call, no user associated with connection %u",
		conn->connId());
	else if (tiFlag)
	    Debug(this,DebugNote,"Refusing new GSM call, invalid direction");
	else {
	    if (canAccept()) {
		YBTSChan* chan = new YBTSChan(conn);
		if (!chan->initIncoming(*xml,regular,callRef))
		    TelEngine::destruct(chan);
		return;
	    }
	    Debug(this,DebugNote,"Refusing new GSM call, full or exiting");
	}
	if (!TelEngine::null(callRef))
	    YBTSCallDesc::sendGSMRel(false,*callRef,!tiFlag,"noconn",m.connId());
	return;
    }
    if (TelEngine::null(callRef))
	return;
    bool rlc = (*type == s_ccRlc);
    Lock lck(this);
    // Handle pending calls
    for (ObjList* o = m_terminatedCalls.skipNull(); o; o = o->skipNext()) {
	YBTSCallDesc* call = static_cast<YBTSCallDesc*>(o->get());
	if (cref != *call || call->connId() != m.connId())
	    continue;
	if (rlc || *type == s_ccRel || *type == s_ccDisc) {
	    Debug(this,DebugNote,"Removing terminated call '%s' conn=%u",
		call->c_str(),call->connId());
	    if (!rlc) {
		if (*type == s_ccDisc &&
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
	else if (*type == s_ccStatusEnq)
	    call->sendStatus("status-enquiry-rsp");
	else if (*type != s_ccStatus)
	    call->sendWrongState();
	return;
    }
    if (rlc)
	return;
    lck.drop();
    DDebug(this,DebugInfo,"Unhandled CC %s for callref=%s conn=%p",
	type->c_str(),TelEngine::c_safe(callRef),conn);
    YBTSCallDesc::sendGSMRel(false,*callRef,!tiFlag,"invalid-callref",m.connId());
}

// Check and start pending MT services for new connection
void YBTSDriver::checkMtService(YBTSUE* ue, YBTSConn* conn)
{
    if (!ue || !conn)
	return;
    RefPointer<YBTSChan> chan;
    if (findChan(ue,chan)) {
	chan->startMT(conn);
	return;
    }
    // TODO: check other MT services
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

bool YBTSDriver::radioReady()
{
    Lock lck(m_stateMutex);
    if (state() != Running) {
	Debug(this,DebugNote,"Radio Ready in %s state !!!",stateName());
	lck.drop();
	restart();
	return false;
    }
    changeState(RadioUp);
    return true;
}

void YBTSDriver::restart(unsigned int restartMs, unsigned int stopIntervalMs)
{
    Lock lck(m_stateMutex);
    if (m_error)
	return;
    if (m_restartTime && !restartMs)
	m_restart = true;
    else
	setRestart(1,true,restartMs);
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
	if (!m_logTrans->start())
	    break;
	if (!m_logBts->start())
	    break;
	// Command interface
	if (!m_command->start())
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
    setRestart(0,1);
    Alarm(this,"system",DebugWarn,"Failed to start the BTS");
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
    m_command->stop();
    m_logTrans->stop();
    m_logBts->stop();
    m_peerAlive = false;
    m_peerCheckTime = 0;
    changeState(Idle);
    lck.drop();
    channels().clear();
}

bool YBTSDriver::startPeer()
{
    String cmd,arg,dir;
    getGlobalStr(cmd,s_peerCmd);
    getGlobalStr(arg,s_peerArg);
    getGlobalStr(dir,s_peerDir);
    if (!cmd) {
	Alarm(this,"config",DebugConf,"Empty peer path");
	return false;
    }
    Debug(this,DebugAll,"Starting peer '%s' '%s'",cmd.c_str(),arg.c_str());
    Socket s[FDCount];
    s[FDLogTransceiver].attach(m_logTrans->transport().detachRemote());
    s[FDLogBts].attach(m_logBts->transport().detachRemote());
    s[FDCommand].attach(m_command->transport().detachRemote());
    s[FDSignalling].attach(m_signalling->transport().detachRemote());
    s[FDMedia].attach(m_media->transport().detachRemote());
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
    for (int j = 0; j < FDCount; j++, i++) {
	int h = s[j].handle();
	if (h < 0)
	    ::fprintf(stderr,"Socket handle at index %d (fd=%d) not used, weird...\n",j,i);
	else if (h < i)
	    ::fprintf(stderr,"Oops! Overlapped socket handle old=%d new=%d\n",h,i);
	else if (h == i)
	    continue;
	else if (::dup2(h,i) < 0)
	    ::fprintf(stderr,"Failed to set socket handle at index %d: %d %s\n",
		j,errno,strerror(errno));
	else
	    continue;
	::close(i);
    }
    // Close all other handles
    for (; i < 1024; i++)
	::close(i);
    // Change directory if asked
    if (dir && ::chdir(dir))
    ::fprintf(stderr,"Failed to change directory to '%s': %d %s\n",
	dir.c_str(),errno,strerror(errno));
    // Start
    ::execl(cmd.c_str(),cmd.c_str(),arg.c_str(),(const char*)0);
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

YBTSChan* YBTSDriver::findChanUE(const YBTSUE* ue)
{
    for (ObjList* o = channels().skipNull(); o ; o = o->skipNext()) {
	YBTSChan* ch = static_cast<YBTSChan*>(o->get());
	if (ch->ue() == ue)
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

void YBTSDriver::setRestart(int resFlag, bool on, unsigned int intervalMs)
{
    if (resFlag >= 0)
	m_restart = (resFlag > 0);
    if (on) {
	if (!intervalMs)
	    intervalMs = s_restartMs;
	m_restartTime = Time::now() + (uint64_t)intervalMs  * 1000;
	Debug(this,DebugAll,"Restart scheduled in %ums [%p]",intervalMs,this);
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

void YBTSDriver::btsStatus(Message& msg)
{
    msg.retValue() << "name=" << BTS_CMD << ",type=misc;state=" << stateName() << "\r\n";
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
	    ((mcc->length() == 3 || mcc->length() == 2) && isDigits09(*mcc)) &&
	    (lac->length() == 4)) {
	    // LAC: 16 bits long
	    int tmp = lac->toInteger(-1,16);
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
    Configuration cfg(Engine::configFile("ybts"));
    const NamedList& general = safeSect(cfg,"general");
    const NamedList& ybts = safeSect(cfg,"ybts");
    s_restartMs = ybts.getIntValue("restart_interval",
	YBTS_RESTART_DEF,YBTS_RESTART_MIN,YBTS_RESTART_MAX);
    s_t305 = general.getIntValue("t305",30000,20000,60000);
    s_t308 = general.getIntValue("t308",5000,4000,20000);
    s_t313 = general.getIntValue("t313",5000,4000,20000);
    s_tmsiExpire = general.getIntValue("tmsi_expire",864000,7200,2592000);
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
    s_askIMEI = general.getBoolValue("imei_request",true);
    s_ueFile = general.getValue("datafile",Engine::configFile("ybtsdata"));
    Engine::runParams().replaceParams(s_ueFile);
    s_peerCmd = ybts.getValue("peer_cmd","${modulepath}/" BTS_DIR "/" BTS_CMD);
    s_peerArg = ybts.getValue("peer_arg");
    s_peerDir = ybts.getValue("peer_dir","${modulepath}/" BTS_DIR);
    Engine::runParams().replaceParams(s_peerCmd);
    Engine::runParams().replaceParams(s_peerArg);
    Engine::runParams().replaceParams(s_peerDir);
    s_globalMutex.unlock();
    if (s_first) {
	s_first = false;
	setup();
	installRelay(Halt);
	installRelay(Stop,"engine.stop");
	installRelay(Start,"engine.start");
        m_logTrans = new YBTSLog("transceiver");
        m_logBts = new YBTSLog(BTS_CMD);
        m_command = new YBTSCommand;
	m_media = new YBTSMedia;
	m_signalling = new YBTSSignalling;
	m_mm = new YBTSMM(ybts.getIntValue("ue_hash_size",31,5));
	m_mm->loadUElist();
    }
    m_signalling->init(cfg);
    startIdle();
}

bool YBTSDriver::msgExecute(Message& msg, String& dest)
{
    if (!msg.userData()) {
	Debug(this,DebugWarn,"GSM call found but no data channel!");
	return false;
    }
    if ((m_state != RadioUp) || !m_mm) {
	Debug(this,DebugWarn,"GSM call: Radio is not up!");
	msg.setParam(YSTRING("error"),"interworking");
	return false;
    }
    RefPointer<YBTSUE> ue;
    if (!m_mm->findUESafe(ue,dest)) {
	// We may consider paging UEs that are not in the TMSI table
	msg.setParam(YSTRING("error"),"offline");
	return false;
    }
    Debug(this,DebugCall,"MT call to IMSI=%s TMSI=%s",ue->imsi().c_str(),ue->tmsi().c_str());
    RefPointer<YBTSChan> chan;
    if (findChan(ue,chan)) {
	if (!chan->canAcceptMT()) {
	    msg.setParam(YSTRING("error"),"busy");
	    return false;
	}
	// TODO
	Debug(this,DebugStub,"CW call not implemented");
	return false;
    }
    if (!m_signalling)
	return false;
    RefPointer<YBTSConn> conn;
    if (m_signalling->findConn(conn,ue)) {
	// TODO
	Debug(this,DebugStub,"Call on existing connection not implemented");
	return false;
    }
    chan = new YBTSChan(ue);
    chan->deref();
    if (chan->initOutgoing(msg)) {
	CallEndpoint* ch = YOBJECT(CallEndpoint,msg.userData());
	if (ch && chan->connect(ch,msg.getValue(YSTRING("reason")))) {
	    chan->callConnect(msg);
	    msg.setParam("peerid",chan->id());
	    msg.setParam("targetid",chan->id());
	    return true;
	}
    }
    return false;
}

bool YBTSDriver::received(Message& msg, int id)
{
    switch (id) {
	case Start:
	    if (!m_engineStart) {
		m_engineStart = true;
		startIdle();
	    }
	    return false;
	case Stop:
	    return handleEngineStop(msg);
	case Halt:
	    dropAll(msg);
	    stop();
	    return false;
	case Timer:
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
	    if (m_mm)
		m_mm->checkTimers(msg.msgTime());
	    if (isPeerCheckState()) {
		pid_t pid = 0;
		Lock lck(m_stateMutex);
		if (isPeerCheckState()) {
		    if (m_peerAlive || !m_peerCheckTime)
			m_peerCheckTime = msg.msgTime() +
			    (uint64_t)m_peerCheckIntervalMs * 1000;
		    else if (m_peerCheckTime <= msg.msgTime())
			pid = m_peerPid;
		    m_peerAlive = false;
		}
		lck.drop();
		int res = pid ? ::waitpid(pid,0,WNOHANG) : 0;
		if (res > 0 || (res < 0 && errno == ECHILD)) {
		    lck.acquire(m_stateMutex);
		    if (pid == m_peerPid) {
			Debug(this,DebugInfo,"Peer pid %d vanished",m_peerPid);
			m_restartTime = 0;
			lck.drop();
			restart();
		    }
		}
	    }
	    break;
	case Status:
	    {
		String dest = msg.getValue(YSTRING("module"));
		if (dest.null() || dest == YSTRING(BTS_CMD)) {
		    btsStatus(msg);
		    if (dest)
			return true;
		}
	    }
	    break;
    }
    return Driver::received(msg,id);
}

bool YBTSDriver::commandExecute(String& retVal, const String& line)
{
    String tmp = line;
    if (m_command && tmp.startSkip(BTS_CMD)) {
	if (tmp.trimSpaces().null())
	    return false;
	if (!m_command->send(tmp))
	    return false;
	if (!m_command->recv(tmp))
	    return false;
	// LF -> CR LF
	for (int i = 0; (i = tmp.find('\n',i + 1)) >= 0; ) {
	    if (tmp.at(i - 1) != '\r') {
		tmp = tmp.substr(0,i) + "\r" + tmp.substr(i);
		i++;
	    }
	}
	if (!tmp.endsWith("\n"))
	    tmp << "\r\n";
	retVal = tmp;
	return true;
    }
    return Driver::commandExecute(retVal,line);
}

bool YBTSDriver::commandComplete(Message& msg, const String& partLine, const String& partWord)
{
    if (partLine.null())
	itemComplete(msg.retValue(),YSTRING(BTS_CMD),partWord);
    if (partLine == YSTRING("debug"))
	itemComplete(msg.retValue(),YSTRING("transceiver"),partWord);
    if ((partLine == YSTRING("debug")) || (partLine == YSTRING("status")))
	itemComplete(msg.retValue(),YSTRING(BTS_CMD),partWord);
    if (m_command && ((partLine == YSTRING(BTS_CMD)) || (partLine == YSTRING(BTS_CMD " help")))) {
	if (!m_helpCache && (m_state >= Running) && lock(100000)) {
	    String tmp;
	    if (m_command->send("help") && m_command->recv(tmp) && tmp.trimBlanks()) {
		ObjList* help = new ObjList;
		ObjList* app = help;
		ObjList* lines = tmp.split('\n');
		for (ObjList* l = lines; l; l = l->skipNext()) {
		    ObjList* words = l->get()->toString().split('\t',false);
		    if (words->get() && !words->get()->toString().startsWith("Type")) {
			while (GenObject* o = words->remove(false))
			    app = app->append(o);
		    }
		    TelEngine::destruct(words);
		}
		TelEngine::destruct(lines);
		m_helpCache = help;
	    }
	    unlock();
	}
	if (m_helpCache) {
	    for (ObjList* l = m_helpCache->skipNull(); l; l = l->skipNext())
		itemComplete(msg.retValue(),l->get()->toString(),partWord);
	}
	return true;
    }
    return Driver::commandComplete(msg,partLine,partWord);
}

bool YBTSDriver::setDebug(Message& msg, const String& target)
{
    if (target == YSTRING("transceiver"))
	return m_logTrans && m_logTrans->setDebug(msg,target);
    if (target == YSTRING(BTS_CMD))
	return m_logBts && m_logBts->setDebug(msg,target);
    return Driver::setDebug(msg,target);
}

}; // anonymous namespace

/* vi: set ts=8 sw=4 sts=4 noet: */
