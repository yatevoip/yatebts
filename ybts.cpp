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
#include <ybts.h>

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
class YBTSLog;                           // Log interface
class YBTSSignalling;                    // Signalling interface
class YBTSMedia;                         // Media interface
class YBTSUE;                            // A registered equipment
class YBTSLocationUpd;                   // Pending location update from UE
class YBTSMM;                            // Mobility management entity
class YBTSDataSource;
class YBTSDataConsumer;
class YBTSChan;
class YBTSDriver;

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
    inline YBTSMessage(uint8_t pri = 0, uint8_t info = 0, uint16_t cid = 0)
        : YBTSConnIdHolder(cid),
        m_primitive(pri), m_info(info), m_xml(0)
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
    // Parse message. Return 0 on failure
    static YBTSMessage* parse(YBTSSignalling* receiver, uint8_t* data, unsigned int len);

    static const TokenDict s_priName[];

protected:
    uint8_t m_primitive;
    uint8_t m_info;
    GenObject* m_xml;
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
    inline void waitHandshake() {
	    Lock lck(this);
	    changeState(WaitHandshake);
	}
    inline bool shouldCheckTimers()
	{ return m_state == Running || m_state == WaitHandshake; }
    int checkTimers(const Time& time = Time());
    // Send a message
    bool send(YBTSMessage& msg);
    bool start();
    void stop();
    // Read socket. Parse and handle received data
    virtual void processLoop();
    void init(Configuration& cfg);

protected:
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
    enum LocationUpdStatus {
	Idle,
	Incoming,                        // Start updating
	ReqIMSI,                         // Requested IMSI
	Updating,                        // Sent to registrar
    };
    YBTSUE();
    inline const String& imsi() const
	{ return m_imsi; }
    // This method is not thread safe
    inline const String& tmsi() const
	{ return m_imsi; }

protected:
    bool m_registered;
    String m_imsi;
    String m_tmsi;
    int m_locUpdStatus;
};

class YBTSMM : public GenObject, public DebugEnabler, public Mutex
{
public:
    YBTSMM(unsigned int hashLen);
    void handlePDU(YBTSMessage& msg);

protected:
    void handleLocationUpdate(YBTSMessage& msg);

    String m_name;
    ObjList m_ue;                        // List of UEs
    HashList m_ueTMSI;                   // List of UEs grouped by TMSI
};

class YBTSChan : public Channel, public YBTSConnIdHolder
{
public:
    // Incoming
    YBTSChan(YBTSMessage& msg, YBTSUE* ue = 0);
    inline bool startRouter() {
	    Message* m = m_route;
	    m_route = 0;
	    return Channel::startRouter(m);
	}

protected:
    virtual void callAccept(Message& msg);
    virtual void callRejected(const char* error, const char* reason, const Message* msg);
#if 0
    virtual void disconnected(bool final, const char *reason);
    virtual bool msgProgress(Message& msg);
    virtual bool msgRinging(Message& msg);
    virtual bool msgAnswered(Message& msg);
    virtual bool msgTone(Message& msg, const char* tone);
    virtual bool msgText(Message& msg, const char* text);
    virtual bool msgDrop(Message& msg, const char* reason);
    virtual bool msgUpdate(Message& msg);
#endif
    virtual void destroyed();

    Mutex m_mutex;
    Message* m_route;
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
    // Handshake done notification. Return false if state is invalid
    bool handshakeDone();
    void restart(unsigned int stopIntervalMs = 0);
    void stopNoRestart();
    inline bool findChan(uint8_t connId, RefPointer<YBTSChan>& chan) {
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
    YBTSChan* findChanConnId(uint8_t connId);
    void changeState(int newStat);
    void setRestart(int resFlag, bool on = true);
    void checkStop(const Time& time);
    void checkRestart(const Time& time);
    inline void setStop(unsigned int stopIntervalMs) {
	    m_stop = true;
	    if (m_stopTime)
		return;
	    m_stopTime = Time::now() + (uint64_t)stopIntervalMs * 1000;
	    Debug(this,DebugAll,"Stop scheduled in %ums",stopIntervalMs);
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
    bool m_engineStart;
    unsigned int m_engineStop;
};

INIT_PLUGIN(YBTSDriver);
static Mutex s_globalMutex(false,"YBTSGlobal");
static String s_format = "gsm";          // Format to use
static String s_peerPath = "path_to_app_to_run"; // Peer path
static unsigned int s_bufLenLog = 4096;  // Read buffer length for log interface
static unsigned int s_bufLenSign = 1024; // Read buffer length for signalling interface
static unsigned int s_bufLenMedia = 1024;// Read buffer length for media interface
static unsigned int s_restartMs = YBTS_RESTART_DEF; // Time (in miliseconds) to wait for restart


#define YBTS_MAKENAME(x) {#x, x}

const TokenDict YBTSMessage::s_priName[] =
{
    {"Handshake",            YBTS::SigHandshake},
    {"Heartbeat",            YBTS::SigHeartbeat},
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
// Parse message. Return 0 on failure
YBTSMessage* YBTSMessage::parse(YBTSSignalling* recv, uint8_t* data, unsigned int len)
{
    if (!len)
	return 0;
    if (len < 2) {
	Debug(recv,DebugNote,"Received short message length %u [%p]",len,recv);
	return 0;
    }
#ifdef XDEBUG
    String tmp;
    tmp.hexify(data,len,' ');
    Debug(recv,DebugAll,"Parse data: %s [%p]",tmp.c_str(),recv);
#endif
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
    switch (m->primitive()) {
	case YBTS::SigHandshake:
	case YBTS::SigHeartbeat:
	    break;
	default:
	    len = 0;
	    Debug(recv,DebugNote,"Unhandled data decode for message %u (%s) [%p]",
	        m->primitive(),m->name(),recv);
    }
    if (len)
	Debug(recv,DebugInfo,"Got %u garbage bytes at message %u (%s) end [%p]",
	    len,m->primitive(),m->name(),recv);
    return m;
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
	Debug(m_enabler,DebugAll,"Sent %d/%u [%p]",wr,len,m_ptr);
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
    int rd = m_readSocket.recv((void*)m_readBuf.data(),m_maxRead);
    if (rd >= 0) {
	if (m_maxRead < m_readBuf.length())
	    ((uint8_t*)m_readBuf.data())[rd] = 0;
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

// Send a message
bool YBTSSignalling::send(YBTSMessage& msg)
{
    if (m_printMsg && debugAt(DebugInfo))
	printMsg(msg,false);
    Debug(this,DebugStub,"send() not implemented [%p]",this);
    return false;
}

bool YBTSSignalling::start()
{
    stop();
    while (true) {
	Lock lck(this);
	if (!m_transport.initTransport(false,s_bufLenSign,false))
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
    Lock lck(this);
    changeState(Idle);
    if (!m_transport.m_socket.valid())
	return;
    m_transport.resetTransport();
    Debug(this,DebugInfo,"Stopped [%p]",this);
}

// Read socket. Parse and handle received data
void YBTSSignalling::processLoop()
{
    while (!Thread::check(false)) {
	int rd = m_transport.recv();
	if (rd > 0) {
	    lock();
	    setToutHeartbeat();
	    unlock();
	    uint8_t* buf = (uint8_t*)m_transport.readBuf().data();
	    YBTSMessage* m = YBTSMessage::parse(this,buf,rd);
	    if (m) {
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
	case YBTS::SigHandshake: return handleHandshake(msg);
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
	    if (send(m))
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
	// TODO: print packet data
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
    m_ueTMSI(hashLen)
{
    m_name = "ybts-mm";
    debugName(m_name);
    debugChain(&__plugin);
}

void YBTSMM::handlePDU(YBTSMessage& msg)
{
    Debug(&__plugin,DebugStub,"YBTSMM::handlePDU() not implemented");
}

void YBTSMM::handleLocationUpdate(YBTSMessage& msg)
{
    Debug(&__plugin,DebugStub,"YBTSMM::handleLocationUpdate() not implemented");
}


//
// YBTSChan
//
// Incoming
YBTSChan::YBTSChan(YBTSMessage& msg, YBTSUE* ue)
    : Channel(__plugin),
    YBTSConnIdHolder(msg.connId()),
    m_mutex(false,"YBTSChan"),
    m_route(0)
{
    String imsi;
    String called;
    if (ue) {
	imsi = ue->imsi();
    }
    Debug(this,DebugCall,"Incoming %s -> %s [%p]",imsi.c_str(),called.c_str(),this);
    m_address = imsi;
    m_route = message("call.preroute");
    m_route->addParam("caller",imsi,false);
    m_route->addParam("called",called,false);
    Message* s = message("chan.startup");
    s->addParam("caller",imsi,false);
    s->addParam("called",called,false);
    Engine::enqueue(s);
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
    // TODO: hangup
}

void YBTSChan::destroyed()
{
    TelEngine::destruct(m_route);
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
    Lock lck(m_stateMutex);
    if (state() != Idle)
	Debug(this,DebugAll,"Stopping ...");
    m_stop = false;
    m_stopTime = 0;
    m_error = false;
    channels().clear();
    m_signalling->stop();
    m_media->stop();
    stopPeer();
    m_log->stop();
    changeState(Idle);
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

YBTSChan* YBTSDriver::findChanConnId(uint8_t connId)
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

void YBTSDriver::initialize()
{
    static bool s_first = true;
    Output("Initializing module YBTS");
    Configuration cfg(Engine::configFile("bts"));
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
    s_globalMutex.lock();
    s_restartMs = ybts.getIntValue("restart_interval",
	YBTS_RESTART_DEF,YBTS_RESTART_MIN,YBTS_RESTART_MAX);
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
    }
    return Driver::received(msg,id);
}

}; // anonymous namespace

/* vi: set ts=8 sw=4 sts=4 noet: */
