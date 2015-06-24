/**
 * Tranceiver.h
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

#include "transceiver.h"
#include <yatengine.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>

#ifdef HAVE_BRF
#include "bladerf/bladerf.h"
#endif

#ifdef HAVE_TEST
#include "test/test.h"
#endif

using namespace TelEngine;

#define TRX_SWITCH_SET_VAL_BREAK(check,dest,src) case check: dest = src; break
#define TRX_SWITCH_INSTR_BREAK(check,instr) case check: instr; break

// Buffer used by log connection
// Avoid using Yate classes for it: if memory alloc fails the DebugFail would
//  lead to deadlock: the debug mutex is already locked
class Buffer
{
public:
    inline Buffer()
	: m_data(0), m_length(0)
	{}
    inline ~Buffer()
	{ reset(); }
    inline uint8_t* data() const
	{ return m_data; }
    // Resize the buffer if requested length is greater the current one
    inline bool resize(unsigned int len)
	{ return len <= m_length ? true : reset(len); }
    inline bool reset(unsigned int len = 0) {
	    if (m_data)
		::free(m_data);
	    if (len)
		m_data = (uint8_t*)::malloc(len);
	    else
		m_data = 0;
	    m_length = m_data ? len : 0;
	    return !len || (m_length != 0);
	}
private:
     uint8_t* m_data;
     unsigned int m_length;
};

class YateLogConnection
{
public:
    inline YateLogConnection()
	: m_socket(STDERR_FILENO + 1)
	{ m_socket.setLinger(-1); }
    ~YateLogConnection()
	{ resetRelayHook(this); }
    // Send relayed debug data
    // Return 0 on success, negative on partial write, positive on error
    int write(int level, const char* buffer, const char* component, const char* info,
	Buffer* buf = 0);
    static inline void setRelayHook(YateLogConnection* conn) {
	    if (conn && !s_self)
		s_self = conn;
	    if (s_self)
		Debugger::setRelayHook(relayFunc);
	}
    static inline void resetRelayHook(YateLogConnection* conn = 0) {
	    if (conn && s_self != conn)
		return;
	    Debugger::setRelayHook(0);
	    s_self = 0;
	}
    static void relayFunc(int level, const char* buffer, const char* component,
	const char* info);

protected:
    Socket m_socket;
    static YateLogConnection* s_self;
    static Buffer s_sendBuffer;
};

class LocalTransceiver : public TransceiverQMF
{
public:
    inline LocalTransceiver()
	: m_shuttingDown(false)
	{}
    virtual void fatalError();
protected:
    bool m_shuttingDown;
};

enum ExitCode
{
    ExitNone = -1,
    ExitOk = 0,
    ExitTrxStartFail,
    ExitTrxInitFail,
    ExitLogFail,
    ExitTrxError,
};

static const char* s_appName = "Transceiver";
static Configuration s_cfg;
static NamedList* s_trxParams = 0;
YateLogConnection* YateLogConnection::s_self = 0;
Buffer YateLogConnection::s_sendBuffer;

// Retrieve the length of a string including NULL character
// return 0 if the string is empty
static inline unsigned int strLen(const char* s)
{
    return !TelEngine::null(s) ? (::strlen(s) + 1) : 0;
}

// Set a list parameter is missing or empty
static inline void setMissingParam(NamedList& nl, const String& name, const char* value,
    bool emptyOk = false)
{
    NamedString* ns = nl.getParam(name);
    if (!ns)
	nl.addParam(name,value,emptyOk);
    else if (!*ns)
	*ns = value;
}

// Set a list parameter, clear it if value is empty
static inline void setValidParam(NamedList& nl, const String& name, const char* value)
{
    if (TelEngine::null(value))
	nl.clearParam(name);
    else
	nl.setParam(name,value);
}

// Parse the command line:
// [<ARFCNS> [<RADIO-PARAMS>]] yate-args...
static void parseCmdLine(int argc, const char** argv, String& arfcns,
    String& radioArgs, String& yateArgs)
{
    if (argc && !TelEngine::null(argv[0]))
	s_appName = argv[0];
    if (argc < 2)
	return;
    int i = 1;
    if (argv[1][0] != '-') {
	arfcns = argv[i++];
	if (i < argc && argv[i][0] != '-')
	    radioArgs = argv[i++];
    }
    for (; i < argc; i++)
	yateArgs.append(argv[i]," ");
}

static void loadConfig()
{
    s_cfg.load();
    s_trxParams = s_cfg.createSection("transceiver");
    // Set engine debug level
    const String* str = s_trxParams->getParam(YSTRING("debug_engine"));
    if (str)
	TelEngine::debugLevel(str->toInteger(TelEngine::debugLevel()));
    // Fill missing params
    setMissingParam(*s_trxParams,"RadioFrequencyOffset","128");
    setMissingParam(*s_trxParams,"TxAttenOffset","0");
    setMissingParam(*s_trxParams,"MinOversampling","1");
    setMissingParam(*s_trxParams,"remoteaddr","127.0.0.1");
    setMissingParam(*s_trxParams,"port","5700");
#ifdef HAVE_BRF
    // bladeRF specific parameters
    //p.addParam("bladerf_load_fpga","check"); // true/false/check, default: true
    //p.addParam("bladerf_fpga_40k","bladerf/hostedx40.rbf"); // default: hostedx40.rbf
    //p.addParam("bladerf_fpga_115k","bladerf/hostedx115.rbf"); // default: hostedx115.rbf
    //p.addParam("io_timeout","100"); // default: 500, minimum: Thread::idleMsec()
    //p.addParam("rx_dc_autocorrect","false"); // default: true
    //setMissingParam(*s_trxParams,"bladerf_debug_level",String(BLADERF_LOG_LEVEL_WARNING),false);
#endif
}

static Transceiver* startTransceiver(const String& arfcns, const String& radioArgs)
{
    NamedList p(*s_trxParams);
    setValidParam(p,YSTRING("arfcns"),arfcns);
    setValidParam(p,YSTRING("radio.device_args"),radioArgs);
    RadioIface* radio = 0;
    Transceiver* trx = 0;
#ifdef HAVE_TEST
    if (!radio)
	radio = new TestIface;
#elif defined(HAVE_BRF)
    if (!radio)
	radio = new BrfIface;
#endif
    trx = new LocalTransceiver();
    if (trx->init(radio,p)) {
	bool hardcodeInit = false;
#ifdef HAVE_TEST
	hardcodeInit = true;
#endif
#ifdef CALLGRIND_CHECK
	hardcodeInit = true;
#endif
	if (trx->start()) {
	    if (hardcodeInit) {
		trx->command("CMD RXTUNE 825000");
		trx->command("CMD TXTUNE 870000");
		trx->command("CMD POWERON");
		// TEST: Configure slots
		for (unsigned int i = 0; i < trx->arfcnCount(); i++)
		    for (int j = 0; j < 8; j++)
			trx->command("CMD SETSLOT " + String(j) + " 1",0,i);
	    }
	    return trx;
	}
	Engine::halt(ExitTrxStartFail);
    }
    else
	Engine::halt(ExitTrxInitFail);
    TelEngine::destruct(trx);
    return 0;
}

static void sigHandler(int sig)
{
    switch (sig) {
	case SIGINT:
	case SIGTERM:
	    Engine::halt(ExitOk);
	    break;
    }
}

// Send relayed debug data
// Return 0 on success, negative on partial write,
//  1: socket retriable error, 2: socket non retriable error, 3: out of memory
// Send buffer format:
// - 1 byte header: 2 MSB bits (debug/alarm/output) + 4 LSB bits with level
// - [TEXT + ] NULL + NULL + NULL
// - [COMPONENT + NULL [ + INFO + NULL]]
int YateLogConnection::write(int level, const char* buffer, const char* component,
    const char* info, Buffer* buf)
{
    if (!buffer)
	return 0;
    Buffer tmp;
    if (!buf)
	buf = &tmp;
    unsigned int nBuf = ::strlen(buffer);
    unsigned int nComp = (level >= 0) ? strLen(component) : 0;
    unsigned int nInfo = nComp ? strLen(info) : 0;
    unsigned int n = 1 + (nBuf + 3) + nComp + nInfo;
    if (!buf->resize(n + 50))
	return 2;
    uint8_t* b = buf->data();
    if (level >= 0)
	*b++ = (!nComp ? 0x80 : 0x40) | (0x0f & (uint8_t)level);
    else
	*b++ = 0xc0;
    if (nBuf) {
	::memcpy(b,buffer,nBuf);
	b += nBuf;
    }
    *b++ = 0;
    *b++ = 0;
    *b++ = 0;
    if (nComp) {
	::memcpy(b,component,nComp);
	if (nInfo)
	    ::memcpy(b + nComp,info,nInfo);
    }
    int wr = m_socket.send(buf->data(),n);
    if (wr == (int)n)
	return 0;
    if (wr != Socket::socketError())
	return 1;
    return m_socket.canRetry() ? -1 : -2;
}

void YateLogConnection::relayFunc(int level, const char* buffer, const char* component,
    const char* info)
{
    YateLogConnection* conn = s_self;
    if (!conn)
	return;
    // Debug: this function is serialized by debug indent mutex
    // Output: unsafe to use the global buffer
    Buffer* d = (level >= 0) ? &s_sendBuffer : 0;
    int res = conn->write(level,buffer,component,info,d);
    if (res == 0) {
	if (level == DebugFail)
	    TelEngine::abortOnBug();
	return;
    }
    // Failure: terminate. Put a syslog message if not already exiting
    s_self = 0;
    if (Engine::exiting())
	return;
    ::openlog(s_appName,LOG_NDELAY | LOG_PID,LOG_USER);
    if (res == -1 || res == -2)
	::syslog(LOG_EMERG,"LogConn socket write failure (error=%d)",
	    conn->m_socket.error());
    else if (res == 1)
	::syslog(LOG_EMERG,"LogConn partial write");
    else if (res == 2)
	::syslog(LOG_EMERG,"LogConn out of memory");
    else
	::syslog(LOG_EMERG,"LogConn unknown error");
    ::closelog();
    Engine::halt(ExitLogFail);
    if (level == DebugFail)
	TelEngine::abortOnBug();
}

void LocalTransceiver::fatalError()
{
    Transceiver::fatalError();
    unsigned int intervals = 0;
    if (shutdown()) {
	if (!m_shuttingDown) {
	    m_shuttingDown = true;
	    intervals = (unsigned int)((100 + Thread::idleMsec() - 1) / Thread::idleMsec());
	}
	Engine::halt(ExitOk);
    }
    else
	Engine::halt(ExitTrxError);
    // Wait for some time to allow the main thread to stop us
    // This would avoid our peer to close debug socket and
    //  loose some of our debug messages
    while (intervals) {
	Thread::idle();
	if (intervals > 3 && Thread::check(false))
	    intervals = 3;
	else
	    intervals--;
    }
}

extern "C" int main(int argc, const char** argv, const char** envp)
{
    YateLogConnection* logConn = 0;
    String error, arfcns, radioArgs;
    String yateArgs = ::getenv("LibYateCmdLine");
    // Install debug relay hook if a environment variable was given
    bool debugRelay = !yateArgs.null();
    s_cfg = ::getenv("MBTSConfigFile");
    parseCmdLine(argc,argv,arfcns,radioArgs,yateArgs);
    Engine::initLibrary(yateArgs,&error);
    if (debugRelay) {
	logConn = new YateLogConnection;
	YateLogConnection::setRelayHook(logConn);
    }
    ::signal(SIGINT,sigHandler);
    ::signal(SIGTERM,sigHandler);
    int pid = (int)::getpid();
    String tmp;
    if (error)
	tmp << "\r\n-----" << error << "\r\n-----";
    Output("Transceiver application (%d) is starting%s",pid,tmp.safe());
    loadConfig();
    Transceiver* trx = startTransceiver(arfcns,radioArgs);
    if (trx) {
	while (!Engine::exiting())
	    Thread::idle();
	Output("Transceiver application (%d) is exiting",pid);
	trx->stop();
	TelEngine::destruct(trx);
    }
    int code = Engine::cleanupLibrary();
    Output("Transceiver application (%d) terminated with code %d",pid,code);
    YateLogConnection::resetRelayHook();
    if (logConn)
	delete logConn;
    return code;
}

/* vi: set ts=8 sw=4 sts=4 noet: */
