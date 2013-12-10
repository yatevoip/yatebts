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
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>

using namespace TelEngine;
namespace { // anonymous

#define YBTS_BTS_CMD "/usr/bin/mobts"

class YBTSConnIdHolder;                  // A connection id holder
class YBTSThread;
class YBTSThreadOwner;
class YBTSMessage;                       // YBTS <-> BTS PDU
class YBTSTransport;
class YBTSSignalling;                    // Signalling transport
class YBTSMedia;                         // Media transport
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

class YBTSMessage : public YBTSConnIdHolder
{
public:
    enum Primitive {
	L3Message,
    };
    inline uint16_t primitive() const
	{ return m_primitive; }
protected:
    uint16_t m_primitive;
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
    friend class YBTSSignalling;
    friend class YBTSMedia;
public:
    inline YBTSTransport()
	: m_socket(0), m_localAddr("127.0.0.1"), m_localPort(0),
	m_remoteAddr("127.0.0.1"), m_remotePort(0)
	{}
    ~YBTSTransport()
	{ setTransport(); }
    bool sendTo(const void* buf, unsigned int len);
    inline bool sendTo(const DataBlock& data)
	{ return sendTo(data.data(),data.length()); }
    // Read socket data. Return 0: nothing read, >1: read data, negative: fatal error
    int recvFrom();
    inline const SocketAddr& local() const
	{ return m_local; }
    inline const SocketAddr& remote() const
	{ return m_remote; }

    DataBlock m_readBuf;

protected:
    bool startUdp();
    void setTransport(Socket* sock = 0, bool updAddrLocal = false,
	bool updAddrRemote = false);

    Socket* m_socket;
    Socket m_readSocket;
    Socket m_writeSocket;
    SocketAddr m_local;
    SocketAddr m_remote;
    String m_localAddr;
    int m_localPort;
    String m_remoteAddr;
    int m_remotePort;
};

class YBTSSignalling : public GenObject, public DebugEnabler, public Mutex,
    public YBTSThreadOwner
{
public:
    YBTSSignalling();
    inline const YBTSTransport& transport() const
	{ return m_transport; }
    // Send a message
    bool send(YBTSMessage& msg);
    bool start();
    void stop();
    // Read socket
    virtual void processLoop();

protected:
    void handlePDU(YBTSMessage& msg);

    String m_name;
    int m_printMsg;
    YBTSTransport m_transport;
};

class YBTSMedia : public GenObject, public DebugEnabler, public Mutex,
    public YBTSThreadOwner
{
    friend class YBTSDataConsumer;
public:
    YBTSMedia();
    inline const YBTSTransport& transport() const
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
    inline void forward(const void* data, unsigned int len) {
	    if (len < 2)
		return;
	    uint16_t* d = (uint16_t*)data;
	    YBTSDataSource* src = find(ntohs(*d));
	    if (src) {
		DataBlock tmp(d + 1,len - 2,false);
		src->Forward(tmp);
		tmp.clear(false);
	    }
	    TelEngine::destruct(src);
	}
    inline void consume(const DataBlock& data, uint16_t connId) {
	    if (!data.length())
		return;
	    uint8_t cid[2] = {(uint8_t)(connId >> 8),(uint8_t)connId};
	    DataBlock tmp(cid,2);
	    tmp += data;
	    m_transport.sendTo(tmp);
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
	Starting,                        // The link with BTS is starting
	WaitHandshake,                   // The BTS was started, waiting for handshake
	Running,                         // BTS hadshake done
	BTSUp
    };
    YBTSDriver();
    ~YBTSDriver();
    inline YBTSMedia* media()
	{ return m_media; }
    inline YBTSSignalling* signalling()
	{ return m_signalling; }
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
	    if (!m_engineStart || m_state != Idle)
		return;
	    lck.drop();
	    start();
	}
    void stop();
    bool startBTS();
    void stopBTS();
    bool handleEngineStop(Message& msg);
    YBTSChan* findChanConnId(uint8_t connId);
    void changeState(int newStat);
    virtual void initialize();
    virtual bool msgExecute(Message& msg, String& dest);
    virtual bool received(Message& msg, int id);

    int m_state;
    Mutex m_stateMutex;
    YBTSMedia* m_media;                  // Media transport
    YBTSSignalling* m_signalling;        // Signalling transport
    YBTSMM* m_mm;                        // Mobility management
    bool m_engineStart;
    unsigned int m_engineStop;
};

INIT_PLUGIN(YBTSDriver);
static Mutex s_globalMutex(false,"YBTSGlobal");
static String s_format = "gsm";          // Format to use
static String s_btsCmd;                  // BTS command
static unsigned int s_bufLen = 1024;     // Read buffer length
// Handshake params
static const String s_signallingPort = "signalling-port";
static const String s_mediaPort = "media-port";
static const String s_controlPort = "control-port";
static const String s_statusPort = "status-port";

#define YBTS_MAKENAME(x) {#x, x}

const TokenDict YBTSDriver::s_stateName[] =
{
    YBTS_MAKENAME(Idle),
    YBTS_MAKENAME(Starting),
    YBTS_MAKENAME(WaitHandshake),
    YBTS_MAKENAME(Running),
    YBTS_MAKENAME(BTSUp),
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
bool YBTSTransport::sendTo(const void* buf, unsigned int len)
{
    if (!len)
	return true;
    int wr = m_writeSocket.sendTo(buf,len,m_remote);
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
int YBTSTransport::recvFrom()
{
    m_readBuf.resize(s_bufLen);
    if (!m_readSocket.valid())
	return 0;
    int rd = m_readSocket.recvFrom((void*)m_readBuf.data(),m_readBuf.length());
    if (rd >= 0)
	return rd;
    if (m_readSocket.canRetry())
	return 0;
    String tmp;
    addLastError(tmp,m_readSocket.error());
    Alarm(m_enabler,"socket",DebugWarn,"Socket read error%s [%p]",
	tmp.c_str(),m_ptr);
    return -1;
}

bool YBTSTransport::startUdp()
{
    setTransport();
    String error;
    while (true) {
	SocketAddr sa;
	sa.host(m_localAddr);
	sa.port(m_localPort);
	if (!sa.host()) {
	    error << "Failed to resolve local addr '" << m_localAddr << "'";
	    break;
	}
	m_remote.host(m_remoteAddr);
	if (!m_remote.host()) {
	    error << "Failed to resolve remote addr '" << m_remoteAddr << "'";
	    break;
	}
	m_remote.port(m_remotePort);
	setTransport(new Socket(sa.family(),SOCK_DGRAM,IPPROTO_UDP));
	if (m_socket->valid()) {
	    if (m_socket->bind(sa)) {
		if (m_socket->getSockName(m_local)) {
		    if (m_socket->setBlocking(false))
			return true;
		    error << "Failed to set non blocking mode";
		}
		else
		    error << "Failed to retrieve bind address";
	    }
	    else
		error << "Failed to bind on " << sa.addr();
	}
	else
	    error << "Failed to create socket";
	addLastError(error,m_socket->error());
	break;
    }
    setTransport();
    Alarm(m_enabler,"socket",DebugWarn,"%s [%p]",error.c_str(),m_ptr);
    return false;
}

void YBTSTransport::setTransport(Socket* sock, bool updAddrLocal, bool updAddrRemote)
{
    if (sock) {
	if (sock != m_socket) {
	    setTransport();
	    m_socket = sock;
	}
	m_readSocket.attach(m_socket->handle());
	m_writeSocket.attach(m_socket->handle());
	if (updAddrLocal)
	    m_socket->getSockName(m_local);
	if (updAddrRemote)
	    m_socket->getPeerName(m_remote);
	return;
    }
    if (m_socket) {
	m_socket->terminate();
	delete m_socket;
	m_socket = 0;
    }
    m_readSocket.detach();
    m_writeSocket.detach();
    m_local.clear();
    m_remote.clear();
}


//
// YBTSSignalling
//
YBTSSignalling::YBTSSignalling()
    : Mutex(false,"YBTSSignalling"),
    m_printMsg(0)
{
    m_name = "ybts-signalling";
    debugName(m_name);
    debugChain(&__plugin);
    YBTSThreadOwner::initThreadOwner(this);
    YBTSThreadOwner::setDebugPtr(this,this);
    m_transport.setDebugPtr(this,this);
}

// Send a message
bool YBTSSignalling::send(YBTSMessage& msg)
{
    Debug(this,DebugStub,"send() not implemented [%p]",this);
    return false;
}

bool YBTSSignalling::start()
{
    stop();
    while (true) {
	Lock lck(this);
	if (!m_transport.startUdp())
	    break;
	if (!startThread("BTSSignalling"))
	    break;
	Debug(this,DebugInfo,"Started on %s [%p]",
	    transport().local().addr().c_str(),this);
	return true;
    }
    stop();
    return false;
}

void YBTSSignalling::stop()
{
    stopThread();
    Lock lck(this);
    if (!m_transport.m_socket)
	return;
    m_transport.setTransport();
    Debug(this,DebugInfo,"Stopped [%p]",this);
}

// Read socket
void YBTSSignalling::processLoop()
{
    while (!Thread::check(false)) {
	int rd = m_transport.recvFrom();
	if (rd > 0) {
	    // TODO: decode data, call handlePDU()
	    continue;
	}
	if (!rd) {
	    Thread::idle();
	    continue;
	}
	// TODO: what ????
	m_transport.m_readSocket.detach();
    }
}

void YBTSSignalling::handlePDU(YBTSMessage& msg)
{
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
	if (!m_transport.startUdp())
	    break;
	if (!startThread("YBTSMedia"))
	    break;
	Debug(this,DebugInfo,"Started on %s [%p]",
	    transport().local().addr().c_str(),this);
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
    if (!m_transport.m_socket)
	return;
    m_transport.setTransport();
    Debug(this,DebugInfo,"Stopped [%p]",this);
}

// Read socket
void YBTSMedia::processLoop()
{
    while (!Thread::check(false)) {
	int rd = m_transport.recvFrom();
	if (rd > 0) {
	    forward(m_transport.m_readBuf.data(),rd);
	    continue;
	}
	if (!rd) {
	    Thread::idle();
	    continue;
	}
	// TODO: what ????
	m_transport.m_readSocket.detach();
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
    TelEngine::destruct(m_media);
    TelEngine::destruct(m_signalling);
    TelEngine::destruct(m_mm);
}

void YBTSDriver::start()
{
    stop();
    Lock lck(m_stateMutex);
    changeState(Starting);
    while (true) {
	// Signalling interface
	if (!m_signalling->start())
	    break;
	// Media interface
	if (!m_media->start())
	    break;
	// Start control interface
	// Start status interface
	// Start BTS application
	if (!startBTS())
	    break;
	changeState(WaitHandshake);
	return;
    }
    Alarm(this,"system",DebugWarn,"Failed to start");
    lck.drop();
    stop();
}

void YBTSDriver::stop()
{
    Lock lck(m_stateMutex);
    if (m_state == Idle)
	return;
    channels().clear();
    m_signalling->stop();
    m_media->stop();
    stopBTS();
    changeState(Idle);
}

static inline void addCmdLineInt(String& buf, const String& param, int value)
{
    buf.append("--"," ") << param << "=" << value;
}

bool YBTSDriver::startBTS()
{
    String cmd;
    getGlobalStr(cmd,s_btsCmd);
    if (!cmd) {
	Alarm(this,"config",DebugConf,"Empty BTS command");
	return false;
    }
    String s;
    addCmdLineInt(s,s_signallingPort,m_signalling->transport().local().port());
    addCmdLineInt(s,s_mediaPort,m_media->transport().local().port());
    addCmdLineInt(s,s_controlPort,0);
    addCmdLineInt(s,s_statusPort,0);
    Debug(this,DebugAll,"Starting BTS cmd='%s' cmd_line='%s'",cmd.c_str(),s.c_str());
#if 0
    int pid = ::fork();
    if (pid < 0) {
	String s;
	Alarm(this,"system",DebugWarn,"Failed to fork(): %s",addLastError(s,errno).c_str());
	return false;
    }
    if (!pid) {
	// In child - terminate all other threads if needed
	Thread::preExec();
	// Try to immunize child from ^C and ^backslash 
	::signal(SIGINT,SIG_IGN);
	::signal(SIGQUIT,SIG_IGN);
	// Restore default handlers for other signals
	::signal(SIGTERM,SIG_DFL);
	::signal(SIGHUP,SIG_DFL);
	// Close everything but stdin/out/
	for (int i = STDERR_FILENO + 1; i < 1024; i++)
	    ::close(i);
	// Start
	::execl(cmd.c_str(),cmd.c_str(),s.c_str(),(char*)NULL);
	String s;
	addLastError(s,errno);
	::fprintf(stderr,"Failed to execute '%s': %s\n",cmd.c_str(),s.c_str());
	// Shit happened. Die as quick and brutal as possible
	::_exit(1);
    }
//    m_btsPid = pid;
    return true;
#endif
    Debug(this,DebugStub,"BTS start not implemented");
    return false;
}

void YBTSDriver::stopBTS()
{
}

static inline const NamedList& safeSect(Configuration& cfg, String name)
{
    const NamedList* sect = cfg.getSection(name);
    return sect ? *sect : NamedList::empty();
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
	lookup(m_state,s_stateName),lookup(newStat,s_stateName));
    m_state = newStat;
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
	m_media = new YBTSMedia;
	m_signalling = new YBTSSignalling;
	m_mm = new YBTSMM(ybts.getIntValue("ue_hash_size",31,5));
    }
    s_globalMutex.lock();
    s_btsCmd = ybts.getValue("bts_cmd",YBTS_BTS_CMD);
    s_globalMutex.unlock();
    startIdle();
}

bool YBTSDriver::msgExecute(Message& msg, String& dest)
{
    if (!msg.userData()) {
	Debug(this,DebugWarn,"GSM call found but no data channel!");
	return false;
    }
    if (m_state != BTSUp) {
	Debug(this,DebugWarn,"GSM call: BTS is not up!");
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
    return Driver::received(msg,id);
}

}; // anonymous namespace

/* vi: set ts=8 sw=4 sts=4 noet: */
