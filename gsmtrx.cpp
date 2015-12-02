/**
 * gsmtrx.cpp
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * GSM tranceiver module
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
#include <yatephone.h>

using namespace TelEngine;
namespace { // anonymous

class GsmTrxQMF;                         // The transceiver
class GsmTrxModule;                      // The module


class GsmTrxThread : public Thread
{
public:
    inline GsmTrxThread(const char* name)
	: Thread(name)
	{}
    ~GsmTrxThread()
	{ notify(); }
    virtual void cleanup()
	{ notify(); }
    virtual void run();
    void notify();
};

class GsmTrxQMF : public TransceiverQMF
{
public:
    GsmTrxQMF();
    virtual void fatalError();
protected:
    virtual void syncGSMTimeSent(const GSMTime& time);
};

class GsmTrxModule : public Module
{
public:
    enum Relay {
	EngineStop = Private
    };
    enum State {
	Idle,
	Waiting,
	Running
    };
    GsmTrxModule();
    ~GsmTrxModule();
    inline void setCheckTrx() {
	    Lock lck(m_stateMutex);
	    m_checkTrx = true;
	}
    inline void syncGSMTimeSent(const GSMTime& time) {
	    if (!m_syncGsmTimeUs)
		return;
	    Lock lck(m_stateMutex);
	    m_syncGsmTimeLast = time;
	    m_syncGsmTimeTout = Time::now() + m_syncGsmTimeUs;
	}
    void readCtrlLoop();
    void workerTerminated(GsmTrxThread* thread);
    inline bool getTransceiver(RefPointer<GsmTrxQMF>& trx, bool take = false) {
	    Lock lck(m_stateMutex);
	    trx = m_trx;
	    if (take && m_trx) {
		TelEngine::destruct(m_trx);
		if (m_state == Running)
		    changeState(Waiting);
	    }
	    return trx != 0;
	}

protected:
    virtual void initialize();
    virtual bool received(Message& msg, int id);
    virtual void statusParams(String& str);
    virtual bool commandComplete(Message& msg, const String& partLine,
	const String& partWord);
    void onTimer(const Time& time = Time());
    bool onCmdControl(Message& msg);
    // Control management (start/stop)
    bool ctrlStart();
    void ctrlStop();
    // Transceiver management (create/stop)
    int trxStart(String& cmdLine);
    void trxStop(const char* reason, int level = DebugNote, bool dumpStat = true,
	bool notify = true);
    void changeState(int newState);
    RadioInterface* createRadio(const NamedList& params);

private:
    Mutex m_stateMutex;                  // Protect state changes
    int m_state;                         // Current state
    Configuration m_cfg;                 // The configuration file
    GsmTrxQMF* m_trx;                    // The transceiver
    TransceiverSockIface m_ctrl;         // Control interface
    TransceiverQMF* m_dummy;             // Dummy transceiver used for socket iface
    GsmTrxThread* m_ctrlThread;          // Control thread
    int m_port;                          // Base port for transceiver socket interface(s)
    bool m_checkTrx;                     // Flag used to check the transceiver in timer handler
    uint64_t m_nextCtrlStart;            // Next time to try control start
    uint64_t m_ctrlStartInterval;        // Last interval used for control start
    GSMTime m_syncGsmTimeLast;           // GSM time sync last sent value
    uint64_t m_syncGsmTimeTout;          // GSM time sync timeout
    uint64_t m_syncGsmTimeUs;            // GSM time sync interval
};

INIT_PLUGIN(GsmTrxModule);
static unsigned int s_engineStop = 0;
static const String s_modCmds[] = {"help",""};
static const String s_trxCmds[] = {"print-status",""};

static const TokenDict s_stateName[] = {
    {"Idle",    GsmTrxModule::Idle},
    {"Waiting", GsmTrxModule::Waiting},
    {"Running", GsmTrxModule::Running},
    {0,0},
};

static bool completeStrList(String& dest, const String& partWord, const String* ptr)
{
    if (!ptr)
	return false;
    while (*ptr)
	Module::itemComplete(dest,*ptr++,partWord);
    return false;
}


//
// GsmTrxThread
//
void GsmTrxThread::run()
{
    __plugin.readCtrlLoop();
    notify();
}

void GsmTrxThread::notify()
{
    __plugin.workerTerminated(this);
}


//
// GsmTrxQMF
//
GsmTrxQMF::GsmTrxQMF()
    : TransceiverQMF("gsmtrx")
{
}

void GsmTrxQMF::fatalError()
{
    TransceiverQMF::fatalError();
    __plugin.setCheckTrx();
}

void GsmTrxQMF::syncGSMTimeSent(const GSMTime& time)
{
    __plugin.syncGSMTimeSent(time);
}


//
// GsmTrxModule
//
GsmTrxModule::GsmTrxModule()
    : Module("gsmtrx"),
    m_stateMutex(false,"GsmTrxState"),
    m_state(Idle),
    m_trx(0),
    m_ctrl("control"),
    m_dummy(0),
    m_ctrlThread(0),
    m_port(0),
    m_checkTrx(false),
    m_nextCtrlStart(0),
    m_ctrlStartInterval(0),
    m_syncGsmTimeTout(0),
    m_syncGsmTimeUs(0)
{
    Output("Loaded module GSM Transceiver");
    Engine::extraPath("radio");
}

GsmTrxModule::~GsmTrxModule()
{
    Output("Unloading module GSM Transceiver");
}

void GsmTrxModule::readCtrlLoop()
{
    bool startRecv = false;
    bool stopRecv = false;
    unsigned int invalid = 0;
    while (!Thread::check(false)) {
	int r = m_ctrl.readSocket(*m_dummy);
	if (Thread::check(false))
	    break;
	if (r < 0)
	    break;
	if (!r)
	    continue;
	String rsp;
	String s((const char*)m_ctrl.m_readBuffer.data(),r);
	if (!s)
	    continue;
	XDebug(this,DebugAll,"CTRL received: %s",s.c_str());
	if (s.startSkip("CMD RESET ",false)) {
	    trxStop("received RESET command",DebugNote,true,false);
	    rsp << "RSP RESET " << trxStart(s);
	    startRecv = true;
	    stopRecv = (m_trx == 0);
	}
	else if (s == YSTRING("CMD STOP")) {
	    trxStop("received STOP command",DebugNote,!startRecv);
	    rsp << "RSP STOP 0";
	    stopRecv = true;
	}
	else {
	    RefPointer<GsmTrxQMF> trx;
	    if (getTransceiver(trx)) {
		if (s.startSkip("CMD STATISTICS ",false)) {
		    trx->statistics(s.toLower().toBoolean());
		    rsp << "RSP STATISTICS 0";
		}
		else
		    trx->command(s,&rsp,0xffffffff);
	    }
	    else if (s.startSkip("CMD ",false)) {
		int level = DebugInfo;
		if (!(startRecv || stopRecv))
		    level = invalid ? DebugNote : DebugWarn;
		invalid++;
		Debug(this,level,"Received command '%s': transceiver not running",
		    s.c_str());
		int pos = s.find(' ');
		if (pos > 0)
		    rsp << "RSP " << s.substr(0,pos) << " " <<
			Transceiver::CmdEInvalidState;
	    }
	}
	XDebug(this,DebugAll,"CTRL sending response: %s",rsp.c_str());
	if (!rsp)
	    continue;
	r = m_ctrl.writeSocket((const void*)rsp.c_str(),rsp.length(),*m_dummy);
	// Don't stop on socket write failure: remote party may be terminated
	// Stop the transceiver
	if (r <= 0)
	    trxStop("Control socket write failure",DebugWarn);
    }
}

void GsmTrxModule::workerTerminated(GsmTrxThread* thread)
{
    if (!thread)
	return;
    Lock lck(m_stateMutex);
    bool ok = true;
    String what;
    bool stop = false;
    if (thread == m_ctrlThread) {
	m_ctrlThread = 0;
	ok = Engine::exiting() || s_engineStop || (m_state == Idle);
	what = m_ctrl.name();
	stop = true;
    }
    else
	return;
    lck.drop();
    if (ok)
	Debug(this,DebugAll,"'%s' thread terminated",what.c_str());
    else {
	Alarm(this,"system",DebugWarn,"'%s' thread abnormally terminated",what.c_str());
	if (stop) {
	    trxStop("Worker thread abnormally terminated",DebugWarn);
	    ctrlStop();
	}
    }
}

void GsmTrxModule::initialize()
{
    Output("Initializing module GSM Transceiver");
    Lock lck(m_stateMutex);
    m_cfg = Engine::configFile("ybts");
    m_cfg.load();
    lck.drop();
    if (m_state == Idle) {
	if (!m_dummy) {
	    m_dummy = new TransceiverQMF(name());
	    m_dummy->debugChain(this);
	}
	if (!ctrlStart())
	    return;
	setup();
	installRelay(Control);
	installRelay(Halt);
	installRelay(EngineStop,"engine.stop");
    }
    if (m_state == Running) {
	Lock lck(m_stateMutex);
	if (m_trx && m_state == Running)
	    m_trx->reInit(*m_cfg.createSection("transceiver"));
    }
}

bool GsmTrxModule::received(Message& msg, int id)
{
    if (id == Timer)
	onTimer(msg.msgTime());
    else if (id == Control) {
	const String& comp = msg[YSTRING("component")];
	if (comp == name())
	    return onCmdControl(msg);
	return false;
    }
    else if (id == EngineStop)
	s_engineStop++;
    else if (id == Halt) {
	trxStop("Exiting",DebugAll);
	ctrlStop();
	TelEngine::destruct(m_dummy);
    }
    return Module::received(msg,id);
}

void GsmTrxModule::statusParams(String& str)
{
    str << "state=" << lookup(m_state,s_stateName);
}

bool GsmTrxModule::commandComplete(Message& msg, const String& partLine,
    const String& partWord)
{
    if (partLine == YSTRING("control")) {
	itemComplete(msg.retValue(),name(),partWord);
	return false;
    }
    String tmp = partLine;
    if (tmp.startSkip("control") && tmp == name()) {
	completeStrList(msg.retValue(),partWord,s_trxCmds);
	return completeStrList(msg.retValue(),partWord,s_modCmds);
    }
    return Module::commandComplete(msg,partLine,partWord);
}

void GsmTrxModule::onTimer(const Time& time)
{
    if (m_checkTrx) {
	Lock lck(m_stateMutex);
	if (m_checkTrx) {
	    m_checkTrx = false;
	    lck.drop();
	    RefPointer<GsmTrxQMF> trx;
	    if (getTransceiver(trx)) {
		if (trx->inError())
		    trxStop("In error");
		else if (trx->shutdown() || trx->exiting())
		    trxStop("Shutdown",DebugAll);
	    }
	}
    }
    if (m_state == Idle) {
	trxStop("Invalid state",DebugGoOn);
	Lock lck(m_stateMutex);
	if (m_state == Idle) {
	    bool schedule = true;
	    if (m_nextCtrlStart) {
		if (m_nextCtrlStart >= time) {
		    lck.drop();
		    schedule = !ctrlStart();
		    lck.acquire(m_stateMutex);
		}
		else
		    schedule = false;
	    }
	    if (schedule) {
		if (m_ctrlStartInterval) {
		    if (m_ctrlStartInterval < 120000000)
			m_ctrlStartInterval *= 2;
		}
		else
		    m_ctrlStartInterval = 1000000;
		m_nextCtrlStart = Time::now() + m_ctrlStartInterval;
	    }
	}
    }
    else if (m_syncGsmTimeTout && m_syncGsmTimeTout < time) {
	Lock lck(m_stateMutex);
	if (m_syncGsmTimeTout && m_syncGsmTimeTout < time) {
	    m_syncGsmTimeTout = 0;
	    if (m_trx) {
		String l;
		Debug(this,DebugFail,"GSM time sync send gap %ums last sent %s",
		    (unsigned int)(m_syncGsmTimeUs / 1000),m_syncGsmTimeLast.c_str(l));
		// Avoid too much debug if we don't terminate
		if (m_syncGsmTimeUs)
		    m_syncGsmTimeUs += 1000000;
	    }
	}
    }
}

bool GsmTrxModule::onCmdControl(Message& msg)
{

    static const char* s_help =
	"\r\ncontrol gsmtrx print-status [count=1] [nobursts=no]"
	"\r\n  Set transceiver print status."
	    "\r\n    count: negative to print until stopped, number of print operations otherwise."
	    "\r\n    nobursts=yes inhibits bursts counters printing."
	"\r\ncontrol gsmtrx help"
	"\r\n  Display control commands help";

    const String& oper = msg[YSTRING("operation")];
    // Module commands
    if (oper == YSTRING("help")) {
	msg.retValue() << s_help;
	return true;
    }
    // Transceiver commands
    RefPointer<GsmTrxQMF> trx;
    if (getTransceiver(trx))
	return trx->control(oper,msg);
    return false;
}

bool GsmTrxModule::ctrlStart()
{
    ctrlStop();
    Lock lck(m_stateMutex);
    NamedList* trx = m_cfg.createSection("transceiver");
    String localAddr = trx->getValue(YSTRING("TRX.IP"),"127.0.0.1");
    String remoteAddr = trx->getValue(YSTRING("remoteaddr"),localAddr);
    m_port = trx->getIntValue(YSTRING("TRX.PORT"),5700);
    while (true) {
	if (!(m_ctrl.setAddr(true,localAddr,m_port + 1,*m_dummy) &&
	    m_ctrl.setAddr(false,remoteAddr,m_port + 101,*m_dummy)))
	    break;
	if (!m_ctrl.initSocket(*m_dummy))
	    break;
	GsmTrxThread* tmp = new GsmTrxThread("GsmTrxCtrl");
	if (!tmp->startup()) {
	    delete tmp;
	    Alarm(this,"system",DebugWarn,"Failed to start control worker thread");
	    break;
	}
	m_ctrlThread = tmp;
	m_nextCtrlStart = m_ctrlStartInterval = 0;
	changeState(Waiting);
	return true;
    }
    lck.drop();
    ctrlStop();
    return false;
}

void GsmTrxModule::ctrlStop()
{
    Lock lck(m_stateMutex);
    changeState(Idle);
    if (m_ctrlThread) {
	m_ctrlThread->cancel();
	lck.drop();
	while (m_ctrlThread && !Thread::check(false))
	    Thread::idle();
	lck.acquire(m_stateMutex);
	if (m_ctrlThread) {
	    Debug(this,DebugMild,"Hard cancelling '%s' worker thread",
		m_ctrl.name().c_str());
	    GsmTrxThread* tmp = m_ctrlThread;
	    m_ctrlThread = 0;
	    tmp->cancel(true);
	}
    }
    m_ctrl.terminate();
}

int GsmTrxModule::trxStart(String& cmdLine)
{
    Lock lck(m_stateMutex);
    if (Engine::exiting())
	return Transceiver::CmdEInvalidState;
    NamedList params(*m_cfg.createSection("transceiver"));
    m_syncGsmTimeLast = 0;
    m_syncGsmTimeTout = 0;
    m_syncGsmTimeUs = params.getIntValue("gsm_time_sync_check",0,0,10000);
    if (m_syncGsmTimeUs && m_syncGsmTimeUs < 2000)
	m_syncGsmTimeUs = 2000;
    m_syncGsmTimeUs *= 1000;
    lck.drop();
    RadioInterface* r = createRadio(params);
    if (!r)
	return Transceiver::CmdEFailure;
    lck.acquire(m_stateMutex);
    if (m_state != Waiting) {
	TelEngine::destruct(r);
	return Transceiver::CmdEInvalidState;
    }
    int pos = cmdLine.find(' ');
    unsigned int arfcns = cmdLine.substr(0,pos).toInteger(1,0,1);
    params.setParam(YSTRING("arfcns"),String(arfcns));
    params.setParam(YSTRING("remoteaddr"),m_ctrl.m_remote.host());
    params.setParam(YSTRING("localaddr"),m_ctrl.m_local.host());
    params.setParam(YSTRING("port"),String(m_port));
    TelEngine::destruct(m_trx);
    m_trx = new GsmTrxQMF;
    m_trx->debugChain(this);
    bool ok = m_trx->init(r,params);
    if (ok && m_trx->start()) {
	changeState(Running);
	return 0;
    }
    TelEngine::destruct(m_trx);
    Alarm(this,"system",DebugWarn,"Failed to %s transceiver",
	ok ? "start" : "initialize");
    return Transceiver::CmdEFailure;
}

void GsmTrxModule::trxStop(const char* reason, int level, bool dumpStat, bool notify)
{
    RefPointer<GsmTrxQMF> trx;
    if (!getTransceiver(trx,true))
	return;
    Debug(this,level,"Stopping transceiver: %s",reason);
    trx->stop(dumpStat && level < DebugAll,notify);
    trx = 0;
    m_syncGsmTimeTout = 0;
}

void GsmTrxModule::changeState(int newState)
{
    if (newState == m_state)
	return;
    Debug(this,DebugInfo,"Module state changed %s -> %s",lookup(m_state,s_stateName),
	lookup(newState,s_stateName));
    m_state = newState;
}

RadioInterface* GsmTrxModule::createRadio(const NamedList& params)
{
    Message m("radio.create");
    m.addParam("module",name());
    const NamedString* drv = params.getParam(YSTRING("radio_driver"));
    if (!TelEngine::null(drv))
	m.addParam(drv->name(),*drv);
    // Set some radio related params
    for (const ObjList* o = params.paramList()->skipNull(); o; o = o->skipNext()) {
	const NamedString* ns = static_cast<const NamedString*>(o->get());
	if (ns->name() == YSTRING("RadioFrequencyOffset") ||
	    ns->name() == YSTRING("TX.OffsetI") ||
	    ns->name() == YSTRING("TX.OffsetQ") ||
	    ns->name() == YSTRING("RX.OffsetI") ||
	    ns->name() == YSTRING("RX.OffsetQ"))
	    m.addParam("radio." + ns->name(),*ns);
    }
    bool ok = Engine::dispatch(m);
    NamedPointer* np = YOBJECT(NamedPointer,m.getParam(YSTRING("interface")));
    RadioInterface* r = np ? YOBJECT(RadioInterface,np) : 0;
    if (r) {
	np->takeData();
	return r;
    }
    const String& e = m[YSTRING("error")];
    Debug(this,DebugGoOn,"Failed to create radio interface: %s",
	e.safe(ok ? "Unknown error" : "Message not handled"));
    return 0;
}

}; // anonymous namespace

/* vi: set ts=8 sw=4 sts=4 noet: */
