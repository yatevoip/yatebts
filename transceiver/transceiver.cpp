/**
 * tranceiver.cpp
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * Tranceiver implementation
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
#include <string.h>

// Socket read/write
#ifdef XDEBUG
#define TRANSCEIVER_DEBUG_SOCKET
#else
//#define TRANSCEIVER_DEBUG_SOCKET
#endif

#ifdef XDEBUG
//#define TRANSCEIVER_DEBUGGER_FUNC
#endif
#ifdef TRANSCEIVER_DEBUGGER_FUNC
#define DBGFUNC_TRXOBJ(func,obj) \
    Debugger debugger(DebugAll,func," %s[%p]",TelEngine::c_safe(obj->prefix()),obj)
#else
#define DBGFUNC_TRXOBJ(func,obj) {}
#endif

#define FILLER_FRAMES_MIN 102
#define BOARD_SHIFT 4

// Dump data when entering in qmf() function
//#define TRANSCEIVER_DUMP_QMF_IN
// Dump data after applying QMF frequency shifting
//#define TRANSCEIVER_DUMP_QMF_FREQSHIFTED
// Dump QMF half band filter data
//#define TRANSCEIVER_DUMP_QMF_HALFBANDFILTER
// Dump input data on ARFCN process start / exit
//#define TRANSCEIVER_DUMP_ARFCN_PROCESS_IN
//#define TRANSCEIVER_DUMP_ARFCN_PROCESS_OUT
// Dump the RX data for testing.
//#define TRANSCEIVER_DUMP_RX_DEBUG
// Dump only the input vector and the output data for RX
//#define TRANSCEIVER_DUMP_RX_INPUT_OUTPUT
// Dump Demodulator performance
//#define TRANSCEIVER_DUMP_DEMOD_PERF

// Minimum data length for radio Rx burst to be processed by an ARFCN
#define ARFCN_RXBURST_LEN 156

#define RX_RSSI_DEF -63
#define RX_RSSI_MIN -90
#define RX_RSSI_MAX 90

#define TX_ATTEN_OFFS_DEF 0
#define TX_ATTEN_OFFS_MIN 0
#define TX_ATTEN_OFFS_MAX 100

#define RADIO_TX_SLOTS_DEF 16
#define RADIO_LATENCY_SLOTS_DEF 5

namespace TelEngine {

class TrxWorker : public Thread, public GenObject
{
public:
    enum Type {
	TrxRadioRead = 0x0001,
	TrxRadioIn = 0x0004,
	ARFCNTx = 0x0008,
	ARFCNRx = 0x0010,
	TrxRadioOut = 0x0020,
	RadioMask = TrxRadioRead | TrxRadioIn | TrxRadioOut | ARFCNTx | ARFCNRx,
    };
    TrxWorker(unsigned int type, TransceiverObj* obj, Thread::Priority prio = Thread::Normal)
	: Thread(buildName(type,obj),prio), m_type(type), m_obj(obj)
	{}
    ~TrxWorker()
	{ notify(); }
    virtual void cleanup()
	{ notify(); }
    static bool create(Thread*& th, int type, TransceiverObj* o,
	Thread::Priority prio = Thread::Normal);
    static void cancelThreads(TransceiverObj* o, unsigned int waitMs1, Thread** th1,
	unsigned int waitMs2 = 0, Thread** th2 = 0,
	unsigned int waitMs3 = 0, Thread** th3 = 0);
    static void softCancel(unsigned int mask = 0xffffffff);
    static inline const char* buildName(unsigned int type, TransceiverObj* obj) {
	    if (type == ARFCNTx || type == ARFCNRx) {
		ARFCN* a = YOBJECT(ARFCN,obj);
		String tmp(a ? lookup(type,s_name) : "");
		if (tmp) {
		    tmp << ":" << a->arfcn();
		    ObjList* o = s_dynamicNames.find(tmp);
		    if (!o)
			o = s_dynamicNames.append(new String(tmp));
		    return static_cast<String*>(o->get())->c_str();
		}
	    }
	    return lookup(type,s_name);
	}
    static const TokenDict s_name[];
    static const TokenDict s_info[];
    static ObjList s_dynamicNames;
    static Mutex s_mutex;
protected:
    virtual void run();
    void notify(bool final = true);

    static ObjList s_threads;

    unsigned int m_type;
    TransceiverObj* m_obj;
};

/** TODO NOTE 
 * The following code is for testing purpose
 */
static ObjList s_indata;
static Mutex s_inDataLoker;

class RxInData : public GenObject
{
public:
    inline RxInData(const ComplexVector& v, const GSMTime& t)
	: m_data(v), m_time(t)
    {}
    ComplexVector m_data;
    GSMTime m_time;

    static void add(const ComplexVector& v, const GSMTime& t)
    {
	Lock myLock(s_inDataLoker);
	RxInData* in = new RxInData(v,t);
	s_indata.append(in);
	if (s_indata.count() > 100) {
	    ObjList* o = s_indata.skipNull();
	    if (o)
		o->remove();
	}
    }

    static void dump(const GSMTime& t) {
	RxInData* d = 0;
	Lock myLock(s_inDataLoker);
	for (ObjList* o = s_indata.skipNull(); o;) {
	    RxInData* rx = static_cast<RxInData*>(o->get());
	    if (t > rx->m_time) {
		o->remove();
		o = o->skipNull();
		continue;
	    }
	    if (rx->m_time == t) {
		TelEngine::destruct(d);
		d = static_cast<RxInData*>(o->remove(false));
		o = o->skipNull();
		continue;
	    }
	    o = o->skipNext();
	}
	if (!d) {
	    Debug(DebugWarn,"Unable to find Input Data!!");
	    return;
	}
	d->m_data.output(20,SigProcUtils::appendComplex,"----"," ","input-data:\r\n");
	TelEngine::destruct(d);
    }
};

class FileDataDumper : public Thread
{
public:
    FileDataDumper(const String& fileName);
    void addData(const ComplexVector& v);
    static bool start(FileDataDumper*& ptr, const String& fileName);
    static inline void stop(FileDataDumper*& ptr) {
	    if (!ptr)
		return;
	    ptr->cancel();
	    ptr = 0;
	}
protected:
    virtual void run();
private:
    ObjList m_list;
    String m_fileName;
    Mutex m_mutex;
};

FileDataDumper::FileDataDumper(const String& fileName)
    : Thread("TrxFileDump"),
    m_fileName(fileName),
    m_mutex(false,"TrxFileDump")
{
}

void FileDataDumper::addData(const ComplexVector& v)
{
    static int s_counter = 0;
    static GSMTime t;
    s_counter++;
    if ((s_counter % 1001) != 0)
	return;
    s_counter = 0;
    Lock myLock(m_mutex);
    m_list.append(new RxInData(v,t));
}

bool FileDataDumper::start(FileDataDumper*& ptr, const String& fileName)
{
    stop(ptr);
    ptr = new FileDataDumper(fileName);
    if (!ptr->startup()) {
	delete ptr;
	ptr = 0;
    }
    return ptr != 0;
}

void FileDataDumper::run()
{
    File f;
    if (!f.openPath(m_fileName,true,false,true)) {
	Debug(DebugNote,"Unable to open dump file %s",m_fileName.c_str());
	return;
    }
    while (!Thread::check(false)) {
	Lock myLock(m_mutex);
	ObjList* o = m_list.skipNull();
	RxInData* in = o ? static_cast<RxInData*>(o->remove(false)) : 0;
	myLock.drop();
	if (!in) {
	    Thread::idle();
	    continue;
	}
	String s;
	Complex::dump(s,in->m_data.data(),in->m_data.length());
	s << "\r\n";
	f.writeData(s.c_str(),s.length());
	TelEngine::destruct(in);
    }
    Debug(DebugTest,"Data write finished!");
}

/** TODO NOTE 
 * End test code */

ObjList TrxWorker::s_threads;
Mutex TrxWorker::s_mutex(false,"TrxWorkers");
static FileDataDumper* s_dumper = 0;

ObjList TrxWorker::s_dynamicNames;

const TokenDict TrxWorker::s_name[] = {
    {"ARFCNTx",       ARFCNTx},
    {"ARFCNRx",       ARFCNRx},
    {"TrxRadioRead",  TrxRadioRead},
    {"TrxRadioIn",    TrxRadioIn},
    {"TrxRadioOut",   TrxRadioOut},
    {0,0},
};

const TokenDict TrxWorker::s_info[] = {
    {"Data socket read",       ARFCNTx},
    {"Radio input process",    ARFCNRx},
    {"Radio device read",      TrxRadioRead},
    {"Radio read process",     TrxRadioIn},
    {"Radio input process",    TrxRadioIn},
    {"Radio device send",      TrxRadioOut},
    {0,0},
};

bool TrxWorker::create(Thread*& th, int type, TransceiverObj* o, Thread::Priority prio)
{
    if (!o)
	return 0;
    if (th)
	cancelThreads(o,20,&th);
    Lock lck(s_mutex);
    TrxWorker* tmp = new TrxWorker(type,o,prio);
    if (tmp->startup()) {
	th = tmp;
	return true;
    }
    th = 0;
    lck.drop();
    delete tmp;
    Alarm(o,"system",DebugWarn,"%sFailed to start '%s' worker thread [%p]",
	(o ? o->prefix() : ""),lookup(type,s_name),o);
    return false;
}

void TrxWorker::softCancel(unsigned int mask)
{
    Lock lck(s_mutex);
    for (ObjList* o = s_threads.skipNull(); o; o = o->skipNext()) {
	TrxWorker* t = static_cast<TrxWorker*>(o->get());
	if (!t->alive())
	    continue;
	if ((t->m_type & mask) != 0)
	    t->cancel(false);
    }
}

// Calculate a number of thread idle intervals from a period
// Return a non 0 value
static inline unsigned int threadIdleIntervals(unsigned int intervalMs)
{
    intervalMs = (unsigned int)(intervalMs / Thread::idleMsec());
    return intervalMs ? intervalMs : 1;
}

static inline void hardCancel(Thread*& t, TransceiverObj* o, Mutex* mtx = 0)
{
    if (!t)
	return;
    Lock lck(mtx);
    if (!t)
	return;
    Debug(o,DebugWarn,"%sHard cancelling worker thread (%p) '%s' [%p]",
	(o ? o->prefix() : ""),t,t->name(),o);
    t->cancel(true);
    t = 0;
}

static inline void checkHardCancel(Thread*& t, unsigned int& wait, TransceiverObj* dbg,
    Mutex* mtx)
{
    if (!(wait && t))
	return;
    wait--;
    if (!wait)
	hardCancel(t,dbg,mtx);
}

// Cancel threads, wait for them to terminate
// waitMs: Interval to wait before hard cancelling them
void TrxWorker::cancelThreads(TransceiverObj* o, unsigned int waitMs1, Thread** th1,
    unsigned int waitMs2, Thread** th2, unsigned int waitMs3, Thread** th3)
{
    DBGFUNC_TRXOBJ("cancelThreads()",o);
    Lock lck(s_mutex);
    Thread* dummy = 0;
    Thread*& t1 = th1 ? *th1 : dummy;
    Thread*& t2 = th2 ? *th2 : dummy;
    Thread*& t3 = th3 ? *th3 : dummy;
    if (!(t1 || t2))
	return;
    if (t1)
	t1->cancel(false);
    if (t2)
	t2->cancel(false);
    if (waitMs1)
	waitMs1 = threadIdleIntervals(waitMs1);
    if (waitMs2)
	waitMs2 = threadIdleIntervals(waitMs2);
    if (waitMs3)
	waitMs3 = threadIdleIntervals(waitMs3);
    bool stop = false;
    while ((t1 || t2) && !stop) {
	lck.drop();
	Thread::idle();
	stop = Thread::check(false);
	if (!stop && (waitMs1 || waitMs2 || waitMs3)) {
	    checkHardCancel(t1,waitMs1,o,&s_mutex);
	    checkHardCancel(t2,waitMs2,o,&s_mutex);
	    checkHardCancel(t3,waitMs2,o,&s_mutex);
	}
	lck.acquire(s_mutex);
    }
    hardCancel(t1,o);
    hardCancel(t2,o);
    hardCancel(t3,o);
}

void TrxWorker::run()
{
    if (!m_obj)
	return;
    s_mutex.lock();
    s_threads.append(this)->setDelete(false);
    s_mutex.unlock();
    DDebug(m_obj,DebugAll,"%s%s thread (%p) started [%p]",
	m_obj->prefix(),lookup(m_type,s_info),this,m_obj);
    switch (m_type) {
	case ARFCNTx:
	    (static_cast<ARFCNSocket*>(m_obj))->runReadDataSocket();
	    break;
	case ARFCNRx:
	    (static_cast<ARFCN*>(m_obj))->runRadioDataProcess();
	    break;
	case TrxRadioIn:
	    (static_cast<Transceiver*>(m_obj))->runRadioDataProcess();
	    break;
	case TrxRadioOut:
	    (static_cast<Transceiver*>(m_obj))->runRadioSendData();
	    break;
	case TrxRadioRead:
	    (static_cast<Transceiver*>(m_obj))->runReadRadio();
	    break;
	default:
	    Debug(m_obj,DebugStub,"TrxWorker::run() type=%d not handled",m_type);
    }
    notify(false);
}

void TrxWorker::notify(bool final)
{
    s_mutex.lock();
    s_threads.remove(this,false);
    s_mutex.unlock();
    TransceiverObj* obj = m_obj;
    m_obj = 0;
    if (!obj)
	return;
    const char* s = lookup(m_type,s_info);
    if (!final)
	Debug(obj,DebugAll,"%s%s thread (%p) terminated [%p]",obj->prefix(),s,this,obj);
    else {
	Alarm(obj,"system",DebugWarn,"%s%s thread (%p) abnormally terminated [%p]",
	    obj->prefix(),s,this,obj);
	if (obj->transceiver())
	    obj->transceiver()->fatalError();
    }
    obj->workerTerminated(this);
}

}; // namespace TelEngine

using namespace TelEngine;

enum Command
{
    CmdUnknown = 0,
    CmdHandover,
    CmdNoHandover,
    CmdSetSlot,
    CmdSetPower,
    CmdAdjPower,
    CmdSetTxAtten,
    CmdRxTune,
    CmdTxTune,
    CmdReadFactory,
    CmdSetTsc,
    CmdSetMaxDelay,
    CmdSetRxGain,
    CmdPowerOn,
    CmdPowerOff,
    CmdCustom,
    CmdManageTx,
    CmdNoise,
    CmdFreqOffset
};

static const TokenDict s_cmdName[] = {
    {"HANDOVER",      CmdHandover},
    {"NOHANDOVER",    CmdNoHandover},
    {"SETSLOT",       CmdSetSlot},
    {"SETPOWER",      CmdSetPower},
    {"ADJPOWER",      CmdAdjPower},
    {"SETTXATTEN",    CmdSetTxAtten},
    {"RXTUNE",        CmdRxTune},
    {"TXTUNE",        CmdTxTune},
    {"READFACTORY",   CmdReadFactory},
    {"SETTSC",        CmdSetTsc},
    {"SETMAXDLY",     CmdSetMaxDelay},
    {"SETRXGAIN",     CmdSetRxGain},
    {"POWERON",       CmdPowerOn},
    {"POWEROFF",      CmdPowerOff},
    {"CUSTOM",        CmdCustom},
    {"MANAGETX",      CmdManageTx},
    {"NOISELEV",      CmdNoise},
    {"SETFREQOFFSET", CmdFreqOffset},
    {0,0}
};

static const TokenDict s_cmdErrorName[] = {
    {"Unknown command",                Transceiver::CmdEUnkCmd},
    {"Failure",                        Transceiver::CmdEFailure},
    {"Invalid/missing parameter(s)",   Transceiver::CmdEInvalidParam},
    {"Invalid state",                  Transceiver::CmdEInvalidState},
    {"Invalid ARFCN",                  Transceiver::CmdEInvalidARFCN},
    {0,0}
};

static const TokenDict s_stateName[] = {
    {"Invalid",  Transceiver::Invalid},
    {"Idle",     Transceiver::Idle},
    {"PowerOff", Transceiver::PowerOff},
    {"PowerOn",  Transceiver::PowerOn},
    {0,0}
};

static const TokenDict s_gsmSlotType[] = {
    {"I",      ARFCN::ChanI},
    {"II",     ARFCN::ChanII},
    {"III",    ARFCN::ChanIII},
    {"IV",     ARFCN::ChanIV},
    {"V",      ARFCN::ChanV},
    {"VI",     ARFCN::ChanVI},
    {"VII",    ARFCN::ChanVII},
    {"Fill",   ARFCN::ChanFill},
    {"None",   ARFCN::ChanNone},
    {"IGPRS",  ARFCN::ChanIGPRS},
    {0,0}
};

static const TokenDict s_gsmBurstType[] = {
    {"Normal",   ARFCN::BurstNormal},
    {"Access",   ARFCN::BurstAccess},
    {"Idle",     ARFCN::BurstIdle},
    {"None",     ARFCN::BurstNone},
    {"Unknown",  ARFCN::BurstUnknown},
    {"Check",    ARFCN::BurstCheck},
    {0,0}
};

static const TokenDict s_rxDropBurstReason[] = {
    {"LowSNR",             ARFCN::RxDropLowSNR},
    {"LowPower",           ARFCN::RxDropLowPower},
    {"NullRMS",            ARFCN::RxDropNullRMS},
    {"LowPeakMin",         ARFCN::RxDropLowPeakMin},
    {"NegativeMaxIndex",   ARFCN::RxDropNegativeMaxIndex},
    {"QueueFull",          ARFCN::RxDropQueueFull},
    {"InvalidTimeslot",    ARFCN::RxDropInvalidTimeslot},
    {"InvalidLength",      ARFCN::RxDropInvalidLength},
    {"UnknownSlot",        ARFCN::RxDropSlotUnk},
    {0,0}
};

struct QMFBlockDesc
{
    const char* name;
    unsigned int arfcn;
    float freqShiftValue;
};

static const QMFBlockDesc s_qmfBlock4[15] = {
    {"Q",    0xffffffff, (float)(PI / 2)},
    {"QL",   0xffffffff, (float)(0.749149017394489)},
    {"QH",   0xffffffff, (float)(2.3924436361953)},
    {"QLL",  0xffffffff, (float)(-0.821647309400407)},
    {"QLH",  0xffffffff, (float)(0.821647309400407)},
    {"QHL",  0xffffffff, (float)(-0.821647309400407)},
    {"QHH",  0xffffffff, (float)(0.821647309400407)},
    {"QLLL", 0,          (float)(-PI / 4)},
    {"QLLH", 0xffffffff, 0},
    {"QLHL", 1,          (float)(-PI / 4)},
    {"QLHH", 0xffffffff, 0},
    {"QHLL", 2,          (float)(-PI / 4)},
    {"QHLH", 0xffffffff, 0},
    {"QHHL", 3,          (float)(-PI / 4)},
    {"QHHH", 0xffffffff, 0}
};

static inline void change(Transceiver* trx, unsigned int& dest, const NamedList& list,
    const String& param, unsigned int defVal, unsigned int minVal, unsigned int maxVal,
    int level = DebugInfo)
{
    unsigned int tmp = list.getIntValue(param,defVal,minVal,maxVal);
    if (dest != tmp) {
	Debug(trx,level,"%s changed %u -> %u [%p]",param.c_str(),dest,tmp,trx);
	dest = tmp;
    }
}

static inline const char* encloseDashes(String& s, bool extra = false)
{
    static const String s1 = "\r\n-----";
    if (s)
	s = s1 + (extra ? "\r\n" : "") + s + s1;
    return s.safe();
}

static inline String& dumpBits(String& buf, const void* b, unsigned int len)
{
    DataBlock db((void*)b,len);
    uint8_t* d = db.data(0);
    for (uint8_t* last = d + db.length(); d != last; d++)
	*d += '0';
    return buf.append((const char*)db.data(),db.length());
}

static inline const char* trxStateName(int stat)
{
    return lookup(stat,Transceiver::dictStateName());
}

static inline bool trxShouldExit(Transceiver* trx)
{
    return trx && trx->shouldStop();
}

static inline bool thShouldExit(Transceiver* trx)
{
    return Thread::check(false) || trxShouldExit(trx);
}

static inline unsigned int getUInt(const NamedList& p, const String& param,
    unsigned int defVal = 1, unsigned int minVal = 1,
    unsigned int maxVal = (unsigned int)INT_MAX)
{
    return (unsigned int)p.getIntValue(param,defVal,minVal,maxVal);
}

static inline bool setReason(String* dest, const char* src, bool ok = false)
{
    if (dest)
	*dest = src;
    return ok;
}

static inline int getCommand(String& buf, String& cmd)
{
    int c = buf.find(" ");
    if (c >= 0) {
	cmd = buf.substr(0,c);
	buf = buf.substr(c + 1);
    }
    else {
	cmd = buf;
	buf.clear();
    }
    return lookup(cmd,s_cmdName);
}

static inline bool buildCmdRsp(String* rsp, const char* cmd, int status,
    const char* param = 0)
{
    if (rsp) {
	*rsp << "RSP " << cmd << " " << status;
	rsp->append(param," ");
    }
    return status == 0;
}


//
// GenQueue
//
GenQueue::GenQueue(unsigned int maxLen, const char* name)
    : m_name(name),
    m_mutex(false,m_name.safe("GenQueue")),
    m_free(maxLen,"GenQueueFree"),
    m_used(maxLen,"GenQueueUsed"),
    m_empty(true),
    m_count(0),
    m_max(maxLen)
{
    clear();
}

bool GenQueue::add(GenObject* obj, Transceiver* trx)
{
    if (!obj)
	return false;
    m_mutex.lock();
    if (m_max <= m_count) {
	m_mutex.unlock();
	while (!m_free.lock(Thread::idleUsec()))
	    if (thShouldExit(trx))
		return false;
	m_mutex.lock();
    }
    m_data.append(obj);
    m_count++;
    m_empty = false;
    m_mutex.unlock();
    if (m_max)
	m_used.unlock();
    return true;
}

// Wait for an object to be put in the queue.
// Returns when there is something in the queue or the calling thread was
//  cancelled or the given transceiver is exiting
bool GenQueue::waitPop(GenObject*& obj, Transceiver* trx)
{
    if (m_max)
	while (true) {
	    obj = internalPop();
	    if (obj) {
		m_free.unlock();
		return true;
	    }
	    if (m_used.lock(Thread::idleUsec())) {
		obj = internalPop();
		if (obj) {
		    m_free.unlock();
		    return true;
		}
	    }
	    if (thShouldExit(trx))
		return false;
	}
    else
	while (true) {
	    if (!empty()) {
		obj = internalPop();
		if (obj)
		    return true;
	    }
	    Thread::idle();
	    if (thShouldExit(trx))
		return false;
	}
    return false;
}

// Clear the queue
void GenQueue::clear()
{
    m_mutex.lock();
    m_empty = true;
    m_count = 0;
    m_data.clear();
    m_mutex.unlock();
    if (!m_max)
	return;
    // Adjust semaphores
    while (m_used.lock(0))
	;
    for (unsigned int n = m_max + 1; n; n--)
	m_free.unlock();
}

GenObject* GenQueue::internalPop()
{
    Lock lck(m_mutex);
    GenObject* ret = m_data.remove(false);
    if (!ret)
	return 0;
    m_count--;
    if (!m_count)
	m_empty = true;
    return ret;
}


//
// TransceiverObj
//
// Set the transceiver
void TransceiverObj::setTransceiver(Transceiver& t, const char* name)
{
    m_transceiver = &t;
    m_prefix.clear();
    bool noTrx = ((&t) != this);
    if (noTrx) {
	m_name = t.debugName();
	m_prefix << name << ": ";
    }
    else
        m_name = name;
    debugName(m_name);
}

// Dump to output a received burst
void TransceiverObj::dumpRecvBurst(const char* str, const GSMTime& time,
    const Complex* c, unsigned int len)
{
    if (!debugAt(DebugAll))
	return;
    String tmp;
    Complex::dump(tmp,c,len);
    Debug(this,DebugAll,"%s%s TN=%u FN=%u len=%u:%s [%p]",
	prefix(),TelEngine::c_safe(str),time.tn(),time.fn(),len,
	tmp.safe(),this);
}

void TransceiverObj::dumpRxData(const char* prefix, unsigned int index, const char* postfix,
    const Complex* c, unsigned int len)
{
#ifdef TRANSCEIVER_DUMP_RX_DEBUG
    if (!debugAt(DebugAll))
	return;
    String info;
    info << prefix << index << postfix << "\r\n";
    ComplexVector::output(c,len,30,SigProcUtils::appendComplex,"-----",",",info);
#endif
}

void TransceiverObj::dumpRxData(const char* prefix, unsigned int index, const char* postfix,
    const float* f, unsigned int len)
{
#ifdef TRANSCEIVER_DUMP_RX_DEBUG
    if (!debugAt(DebugAll))
	return;
    String info;
    info << prefix << index << postfix << "\r\n";
    FloatVector::output(f,len,30,SigProcUtils::appendFloat,"-----",",",info);
#endif
}


//
// TransceiverSockIface
//
// Worker terminated notification
void TransceiverObj::workerTerminated(Thread* th)
{
    Debug(this,DebugStub,"TransceiverObj::workerTerminated() [%p]",this);
}


//
// TransceiverSockIface
//
// Set a socket address
bool TransceiverSockIface::setAddr(bool local, const char* addr, int port,
    TransceiverObj& dbg)
{
    String reason;
    SocketAddr& sa = local ? m_local : m_remote;
    int family = SocketAddr::IPv4;
    if (sa.assign(family)) {
	if ((!addr || sa.host(addr)) && (local || !sa.isNullAddr())) {
	    sa.port(port);
	    DDebug(&dbg,DebugAll,"%sInitialized %s %s address: '%s' [%p]",
		dbg.prefix(),name().c_str(),(local ? "local" : "remote"),
		sa.addr().c_str(),&dbg);
	    return true;
	}
	reason = "invalid address";
    }
    else
	reason << "failed to assign family " << family <<
	    " (" << SocketAddr::lookupFamily(family) << ")";
    Alarm(&dbg,"config",DebugNote,
	"%sFailed to initialize %s %s address (addr='%s' port=%d): %s [%p]",
	dbg.prefix(),name().c_str(),(local ? "local" : "remote"),
	addr,port,reason.c_str(),&dbg);
    return false;
}

// Prepare the socket: create, set reuse, bind, set unblocking mode
// Don't terminate the socket on failure
bool TransceiverSockIface::initSocket(TransceiverObj& dbg)
{
    terminate();
    const char* error = 0;
    while (true) {
	if (!m_socket.create(m_local.family(),SOCK_DGRAM,IPPROTO_UDP)) {
	    error = "create failed";
	    break;
	}
	m_socket.setReuse();
	if (!m_socket.bind(m_local)) {
	    error = "failed to bind";
	    break;
	}
	if (!m_socket.setBlocking(false)) {
	    error = "failed to set non blocking mode";
	    break;
	}
	break;
    }
    if (!error) {
	Debug(&dbg,DebugInfo,"%sSocket(%s) bound on %s (remote: %s) [%p]",
	    dbg.prefix(),name().c_str(),m_local.addr().c_str(),
	    m_remote.addr().c_str(),&dbg);
	return true;
    }
    String s;
    Thread::errorString(s,m_socket.error());
    Alarm(&dbg,"socket",DebugWarn,"%sSocket(%s) %s (addr=%s): %d %s [%p]",
	dbg.prefix(),name().c_str(),error,m_local.addr().c_str(),
	m_socket.error(),s.c_str(),&dbg);
    return false;
}

// Read from socket. Call Thread::idle() if nothing is read
int TransceiverSockIface::readSocket(TransceiverObj& dbg)
{
    const char* error = 0;
    bool sel = m_socket.canSelect();
    if (sel) {
	bool canRead = false;
	bool ok = m_socket.select(&canRead,0,0,Thread::idleUsec());
	if (thShouldExit(dbg.transceiver()))
	    return 0;
	if (ok) {
	    if (!canRead)
		return 0;
	}
	else if (m_socket.canRetry()) {
	    Thread::idle();
	    return 0;
	}
	else
	    error = "select";
    }
    if (!error) {
#ifdef TRANSCEIVER_DEBUG_SOCKET
	SocketAddr remote;
	int r = m_socket.recvFrom((void*)m_readBuffer.data(),m_readBuffer.length(),remote);
#else
	int r = m_socket.recvFrom((void*)m_readBuffer.data(),m_readBuffer.length());
#endif
	if (r > 0) {
#ifdef TRANSCEIVER_DEBUG_SOCKET
	    String tmp;
	    if (m_text)
		tmp.assign((const char*)m_readBuffer.data(),m_readBuffer.length());
	    else
		tmp.hexify((void*)m_readBuffer.data(),r,' ');
	    Debug(&dbg,DebugAll,"%sSocket(%s) read %d bytes (remote=%s): %s [%p]",
		dbg.prefix(),name().c_str(),r,remote.addr().c_str(),tmp.c_str(),&dbg);
#endif
	    return r;
	}
	if (!r || m_socket.canRetry()) {
	    if (!sel)
		Thread::idle();
	    return 0;
	}
	error = "read";
    }
    String s;
    Thread::errorString(s,m_socket.error());
    Alarm(&dbg,"socket",DebugWarn,"%sSocket(%s) %s failed: %d %s [%p]",
	dbg.prefix(),name().c_str(),error,m_socket.error(),s.c_str(),&dbg);
    return -1;
}

// Write to socket
int TransceiverSockIface::writeSocket(const void* buf, unsigned int len,
    TransceiverObj& dbg)
{
    if (!(buf && len))
	return 0;

    // NOTE: Handle partial write for stream sockets
    for (unsigned int i = 0; i < m_writeAttempts; i++) {
	int r = m_socket.sendTo(buf,len,m_remote);
	if (r > 0) {
#ifdef TRANSCEIVER_DEBUG_SOCKET
	    bool dump = true;
#else
	    bool dump = m_printOne;
#endif
	    if (dump) {
		m_printOne = false;
		String tmp;
		if (m_text)
		    tmp.assign((const char*)buf,len);
		else
		    tmp.hexify((void*)buf,len,' ');
		Debug(&dbg,DebugAll,"%sSocket(%s) sent %u bytes: %s [%p]",
		    dbg.prefix(),name().c_str(),len,tmp.c_str(),&dbg);
	    }
	    return len;
	}
	if (thShouldExit(dbg.transceiver()))
	    return 0;
	if (m_socket.canRetry() && i < (m_writeAttempts - 1)) {
	    Thread::idle();
	    continue;
	}
	break;
    }
    String s;
    Thread::errorString(s,m_socket.error());
    Alarm(&dbg,"socket",DebugWarn,"%sSocket(%s) send failed: %d %s [%p]",
	dbg.prefix(),name().c_str(),m_socket.error(),s.c_str(),&dbg);
    return m_socket.canRetry() ? 0 : -1;
}

//
// Transceiver
//
Transceiver::Transceiver(const char* name)
    : m_state(Invalid),
    m_stateMutex(false,"TrxState"),
    m_radio(0),
    m_radioReadThread(0),
    m_radioReadPrio(Thread::Normal),
    m_radioInThread(0),
    m_radioOutThread(0),
    m_radioOutPrio(Thread::Normal),
    m_rxQueue(24,"TrxRxQueue"),
    m_oversamplingRate(1),
    m_burstMinPower(RX_RSSI_DEF),
    m_snrThreshold(2),
    m_wrtFullScaleThreshold(-2),
    m_peakOverMeanThreshold(3),
    m_upPowerThreshold(-2),
    m_maxPropDelay(63),
    m_arfcn(0),
    m_arfcnCount(0),
    m_arfcnConf(1),
    m_clockIface("clock"),
    m_clockUpdMutex(false,"TrxClockUpd"),
    m_clockUpdOffset(16),
    m_txSlots(1),
    m_radioLatencySlots(0),
    m_printStatus(0),
    m_printStatusBursts(true),
    m_printStatusChanged(false),
    m_radioSendChanged(false),
    m_tsc(0),
    m_radioRxStore(100,"TrxRadioRx"),
    m_rxFreq(0),
    m_txFreq(0),
    m_txPower(-10),
    m_txAttnOffset(TX_ATTEN_OFFS_DEF),
    m_txPowerScale(1.0f),
    m_radioClockMutex(false,"TrxRadioClock"),
    m_txSync(1,"TrxTxSync",0),
    m_txSilenceDebugTime(0),
    m_loopbackMutex(false,"TrxLoopbackData"),
    m_loopback(false),
    m_loopbackSleep(0),
    m_loopbackNextSend(0),
    m_loopbackData(0),
    m_testMin(0),
    m_testMax(0),
    m_testIncrease(0),
    m_testValue(0), 
    m_sendArfcnFS(0),
    m_txTestBurst(0),
    m_testMutex(false,"txTest"),
    m_dumpOneTx(false),
    m_dumpOneRx(false),
    m_toaShift(0),
    m_statistics(false),
    m_error(false),
    m_exiting(false),
    m_shutdown(false)
{
    setTransceiver(*this,name);
    DDebug(this,DebugAll,"Transceiver() [%p]",this);
}

Transceiver::~Transceiver()
{
    DDebug(this,DebugAll,"~Transceiver() [%p]",this);
    stop();
    // NOTE: If you need to use m_arfcnCount use it before this line!
    // resetARFCNs will set it to 0.
    resetARFCNs();
    TelEngine::destruct(m_radio);
    TelEngine::destruct(m_txTestBurst);
    setLoopbackData();
}

// Initialize the transceiver. This method should be called after construction
bool Transceiver::init(RadioInterface* radio, const NamedList& params)
{
    if (m_state != Invalid) {
	reInit(params);
	return false;
    }
    m_error = false;
    m_radio = radio;
    if (!m_radio) {
	Debug(this,DebugNote,"No radio interface [%p]",this);
	return false;
    }
    bool ok = false;
    while (true) {
	m_oversamplingRate = getUInt(params,"oversampling");
	unsigned int arfcns = getUInt(params,"arfcns");
	m_arfcnConf = getUInt(params,"conf_arfcns");
	const String* rAddr = params.getParam(YSTRING("remoteaddr"));
	unsigned int nFillers = params.getIntValue(YSTRING("filler_frames"),
	    FILLER_FRAMES_MIN,FILLER_FRAMES_MIN);
	m_signalProcessing.initialize(m_oversamplingRate,arfcns);
	int port = rAddr ? params.getIntValue(YSTRING("port")) : 0;
	const char* lAddr = rAddr ? params.getValue(YSTRING("localaddr"),*rAddr) : 0;
	if (rAddr &&
	    !(m_clockIface.setAddr(true,lAddr,port,*this) &&
	    m_clockIface.setAddr(false,*rAddr,port + 100,*this)))
	    break;
	if (!resetARFCNs(arfcns,port,rAddr,lAddr,nFillers))
	    break;
	m_radioReadPrio = Thread::priority(params[YSTRING("radio_read_priority")],Thread::High);
	m_radioOutPrio = Thread::priority(params[YSTRING("radio_send_priority")]);
	if (debugAt(DebugAll)) {
	    String tmp;
	    tmp << "\r\nARFCNs=" << m_arfcnCount;
	    tmp << "\r\noversampling=" << m_oversamplingRate;
	    Debug(this,DebugAll,"Initialized [%p]%s",this,encloseDashes(tmp));
	}
	// Set GSM sample rate: (13e6 / 48) + 1
	NamedList p("");
	p.addParam("cmd:setSampleRate",
	    String((uint64_t)(m_oversamplingRate * (13e6 / 48) + 1)));
	p.addParam("cmd:setFilter","1500000");
	unsigned int code = m_radio->setParams(p,false);
	if (!radioCodeOk(code))
	    break;
	// I/O errors check
	unsigned int m = getUInt(params,YSTRING("io_errors_max"),30,2);
	unsigned int b = getUInt(params,YSTRING("io_errors_inc"),10,1);
	m_txIO.initErrors(m,b);
	m_rxIO.initErrors(m,b);
	ok = true;
	// TODO test
	String* dumpFile = params.getParam(YSTRING("dump_file"));
	if (dumpFile)
	    FileDataDumper::start(s_dumper,*dumpFile);
	// TODO end test
	unsigned int tmp = params.getIntValue(YSTRING("tx_silence_debug_interval"),5000,0,10000);
	if (tmp)
	    // Round up to frame boundary (~ 4.615ms)
	    tmp = 1 + ((float)tmp / 4.615);
	m_txSilenceDebugTime = tmp * 8;
	break;
    }
    if (!ok) {
	TelEngine::destruct(m_radio);
	return false;
    }
    reInit(params);
    Debug(this,DebugInfo,"Transceiver initialized radio=(%p) '%s' [%p]",
	m_radio,m_radio->debugName(),this);
    m_nextClockUpdTime = 0;
    m_txTime = 0;
    changeState(Idle);
    return true;
}

// Re-initialize the transceiver
void Transceiver::reInit(const NamedList& params)
{
    m_txAttnOffset = params.getIntValue(YSTRING("TxAttenOffset"),
	TX_ATTEN_OFFS_DEF,TX_ATTEN_OFFS_MIN,TX_ATTEN_OFFS_MAX);
    setBurstMinPower(params.getIntValue(YSTRING("MinimumRxRSSI"),
	RX_RSSI_DEF,RX_RSSI_MIN,RX_RSSI_MAX));
    m_snrThreshold = params.getIntValue(YSTRING("snr_threshold"),m_snrThreshold);
    m_wrtFullScaleThreshold = params.getIntValue(YSTRING("wrt_full_scale"),m_wrtFullScaleThreshold);
    m_peakOverMeanThreshold = params.getIntValue(YSTRING("peak_over_mean"),m_peakOverMeanThreshold);
    m_maxPropDelay = params.getIntValue(YSTRING("max_prop_delay"),m_maxPropDelay);
    m_upPowerThreshold = params.getIntValue(YSTRING("up_power_warn"),m_upPowerThreshold);
    change(this,m_clockUpdOffset,params,YSTRING("clock_update_offset"),16,0,256);
    unsigned int latency = RADIO_LATENCY_SLOTS_DEF;
    unsigned int txSlots = RADIO_TX_SLOTS_DEF;
    const RadioCapability* caps = m_radio ? m_radio->capabilities() : 0;
    if (caps && m_signalProcessing.gsmSlotLen()) {
	latency = caps->rxLatency / m_signalProcessing.gsmSlotLen();
	if (!latency && caps->rxLatency)
	    latency = 1;
	txSlots = caps->txLatency / m_signalProcessing.gsmSlotLen();
	if (!txSlots)
	    txSlots = 1;
    }
    change(this,m_radioLatencySlots,params,YSTRING("radio_latency_slots"),latency,0,256);
    change(this,m_txSlots,params,YSTRING("tx_slots"),txSlots,1,1024);
    m_printStatus = params.getIntValue(YSTRING("print_status"));
    m_printStatusBursts = m_printStatus &&
	params.getBoolValue(YSTRING("print_status_bursts"),true);
    // Signal update
    m_radioSendChanged = true;
    m_printStatusChanged = true;
}

// Wait for PowerOn state
void Transceiver::waitPowerOn()
{
    while (m_state < PowerOn && !thShouldExit(this))
	Thread::idle();
}

// Read radio loop
void Transceiver::runReadRadio()
{
    // Offset between uplink and downlink is 3 timeslots
    // The read data method returns the timestamp of the next slot
    // i.e. we must decrease radio clock by 4
    static const uint64_t s_gsmUpDownOffset = 4;

    waitPowerOn();
    TrxRadioIO& io = m_rxIO;
    if (m_radio->getRxTime(io.timestamp) != 0)
	io.timestamp = 0;
    // Round up timestamp to slot boundary
    m_signalProcessing.alignTs(io.timestamp);
    RadioReadBufs bufs(BITS_PER_TIMESLOT * m_oversamplingRate,0);
    ComplexVector crt(bufs.bufSamples());
    ComplexVector aux(bufs.bufSamples());
    ComplexVector extra(bufs.bufSamples());
    bufs.crt.samples = (float*)crt.data();
    bufs.aux.samples = (float*)aux.data();
    bufs.extra.samples = (float*)extra.data();
    while (!thShouldExit(this)) {
	// incTn will be set to the number of skipped buffers (timeslots)
	unsigned int incTn = 0;
	unsigned int code = m_radio->read(io.timestamp,bufs,incTn);
	if (thShouldExit(this))
	    break;
	if (!io.updateError(code == 0)) {
	    Alarm(this,"system",DebugWarn,"Too many read errors, last: 0x%x %s [%p]",
		code,RadioInterface::errorName(code),this);
	    fatalError();
	    break;
	}
	if (code)
	    continue;
	// Process the burst ?
	ComplexVector* v = 0;
	if (bufs.full(bufs.crt)) {
	    incTn++;
	    if (!m_loopback) {
		if (bufs.crt.samples == (float*)crt.data())
		    v = &crt;
		else if (bufs.crt.samples == (float*)aux.data())
		    v = &aux;
		else if (bufs.crt.samples == (float*)extra.data())
		    v = &extra;
		else
		    Debug(this,DebugFail,"Read radio: buffer mismatch [%p]",this);
	    }
	}
	if (!incTn)
	    continue;
	GSMTime time = m_signalProcessing.ts2slots(io.timestamp);
	if (v && time.time() >= s_gsmUpDownOffset) {
	    RadioRxData* r = m_radioRxStore.get();
	    r->m_time = time.time() - s_gsmUpDownOffset;
	    r->m_data.exchange(*v);
	    recvRadioData(r);
	    // Allocate space for used buffer
	    v->resize(bufs.bufSamples());
	    bufs.crt.samples = (float*)v->data();
	}
	// Advance radio clock. Signal TX
	// Sync to next slot boundary if needed
	if ((io.timestamp % m_signalProcessing.gsmSlotLen()) != 0) {
	    Debug(this,DebugNote,"Resyncing RX timestamp " FMT64U " [%p]",
		io.timestamp,this);
	    time++;
	    io.timestamp = time * m_signalProcessing.gsmSlotLen();
	}
	m_radioClockMutex.lock();
	m_radioClock = time;
	m_radioClockMutex.unlock();
	m_txSync.unlock();
    }
}

// Process received radio data
void Transceiver::recvRadioData(RadioRxData* d)
{
    if (!d)
	return;
    String t;
    XDebug(this,DebugAll,"Enqueueing radio data (%p) time=%s len=%u [%p]",
	d,d->m_time.appendTo(t).c_str(),d->m_data.length(),this);

#ifdef TRANSCEIVER_DUMP_RX_DEBUG
    if (debugAt(DebugAll))
	m_dumpOneRx = true;
#endif
    if (m_dumpOneRx) {
	m_dumpOneRx = false;
	t = "Transceiver recv radio data at ";
	t << d->m_time << "\r\n";
	d->m_data.output(6,SigProcUtils::appendComplex,"-----"," ",t);
    }
    m_rxIO.bursts++;
    if (m_rxQueue.add(d,this))
	return;
    if (!thShouldExit(this))
	Debug(this,DebugWarn,"Dropping radio data (%p) time %s: queue full [%p]",
	    d,d->m_time.c_str(t),this);
    m_radioRxStore.store(d);
}

void Transceiver::runRadioDataProcess()
{
    waitPowerOn();
    GenObject* gen = 0;
    while (m_rxQueue.waitPop(gen,this)) {
	if (!gen)
	    continue;
	processRadioData(static_cast<RadioRxData*>(gen));
	gen = 0;
    }
    TelEngine::destruct(gen);
}

void Transceiver::runRadioSendData()
{
    if (!m_radio)
	return;
    waitPowerOn();
    GSMTime radioTime;                    // Current radio time (in timeslots) in loop start
    GSMTime endRadioTime;                 // Current radio time in loop end (check again, avoid waiting to rx signal)
    unsigned int txSlots = 0;             // Send TX slots in 1 loop
    unsigned int radioLatencySlots = 0;   // Radio latency (the diference between radio time,
                                          //  as we know it, and estimated actual radio time)
    int printStatus = 0;
    bool printBursts = m_printStatusBursts;
    bool wait = true;
    m_radioSendChanged = true;
    while (true) {
	if (m_radioSendChanged) {
	    m_radioSendChanged = false;
	    txSlots = m_txSlots;
	    radioLatencySlots = m_radioLatencySlots;
	}
	if (m_printStatusChanged) {
	    m_printStatusChanged = false;
	    printStatus = m_printStatus;
	    printBursts = m_printStatusBursts;
	}
	if (wait) {
	    if (!waitSendTx())
		break;
	    getRadioClock(radioTime);
	}
	else {
	    if (thShouldExit(this))
		break;
	    wait = true;
	}
	if (printStatus) {
	    if (printStatus > 0)
		printStatus--;
	    dumpStatus(printBursts,&radioTime);
	}
	GSMTime radioCurrentTime = radioTime + radioLatencySlots;
	GSMTime txTime = m_txTime;
	unsigned int sendTimeslots = txSlots;
	// Don't send old data
	if (txTime >= radioCurrentTime) {
	    // TX time is at least current radio time
	    // Start sending from current TX time
	    unsigned int diff = (unsigned int)(txTime - radioCurrentTime);
	    if (sendTimeslots > diff)
		sendTimeslots -= diff;
	    else
		sendTimeslots = 0;
	}
	else {
	    // OOPS !!!
	    // Current TX time is before radio time: underrun
	    // Start sending from current radio time
	    txTime = radioCurrentTime;
	    if (statistics())
		Debug(this,m_txTime ? DebugMild : DebugAll,
		    "Transmit underrun by " FMT64U " timeslots [%p]",
		    radioCurrentTime - m_txTime,this);
	}
	while (sendTimeslots--) {
	    if (sendBurst(txTime)) {
		if (loopback())
		    getRadioClock(txTime);
		else
		    txTime++;
		continue;
	    }
	    fatalError();
	    break;
	}
	if (thShouldExit(this))
	    break;
	// Set current TX time. Check if it's time to sync with upper layer
	Lock lck(m_clockUpdMutex);
	m_txTime = txTime;
	bool upd = m_txTime > m_nextClockUpdTime;
	lck.drop();
	if (upd && !syncGSMTime())
	    break;
	// Look again at radio time
	getRadioClock(endRadioTime);
	if (radioTime < endRadioTime) {
	    wait = false;
	    radioTime = endRadioTime;
	}
    }
}

static inline bool shouldBeSync(const GSMTime& time)
{
    if (time.tn() != 0)
	return false;
    int t3 = time.fn() % 51;
    return t3 == 1 || t3 == 11 || t3 == 21 || t3 == 31 || t3 == 41;
}

bool Transceiver::waitSendTx()
{
    unsigned int wait = 3 * Thread::idleUsec();
    while (!m_txSync.lock(wait)) {
	if (thShouldExit(this))
	    return false;
    }
    return true;
}

void Transceiver::dumpStatus(bool printBursts, GSMTime* radioTime)
{
    GSMTime r;
    if (!radioTime) {
	getRadioClock(r);
	radioTime = &r;
    }
    // Note: some values are taken without protection
    String s;
    s << "\r\nRadioClock:\t" << *radioTime;
    s << "\r\nLastSyncUpper:\t" << m_lastClockUpd;
    s << "\r\nTxTime:\t\t" << m_txTime;
    if (printBursts) {
	s << "\r\nTxBursts:\t" << m_txIO.bursts;
	s << "\r\nRxBursts:\t" << m_rxIO.bursts;
    }
    ARFCNStatsTx aStats;
    for (unsigned int i = 0; i < m_arfcnConf; i++) {
	s << "\r\nARFCN[" << i << "]";
	ARFCN* a = m_arfcn[i];
	a->getTxStats(aStats);
	s << "\r\n  UplinkLastOutTime:\t" << a->m_lastUplinkBurstOutTime;
	s << "\r\n  DownlinkLastInTime:\t" << aStats.burstLastInTime;
	if (printBursts) {
	    uint64_t rx = a->m_rxBursts;
	    String tmp;
	    uint64_t dropped = a->dumpDroppedBursts(&tmp);
	    s << "\r\n  RxBursts:\t\t" << rx;
	    s << "\r\n  RxDroppedBursts:\t" << dropped << " " << tmp;
	    tmp.clear();
	    for (uint8_t j = 0; j < 8; j++) {
		if (tmp)
		    tmp << "/";
		tmp << aStats.burstsDwInSlot[j];
	    }
	    s << "\r\n  DownlinkInBursts:\t" << aStats.burstsDwIn << " (" << tmp << ")";
	    s << "\r\n  TxMissSyncOnSend:\t" << aStats.burstsMissedSyncOnSend;
	    s << "\r\n  TxExpiredOnSend:\t" << aStats.burstsExpiredOnSend;
	    s << "\r\n  TxExpiredOnRecv:\t" << aStats.burstsExpiredOnRecv;
	    s << "\r\n  TxFutureOnRecv:\t" << aStats.burstsFutureOnRecv;
	    s << "\r\n  TxDupOnRecv:\t\t" << aStats.burstsDupOnRecv;
	}
    }
    Output("Transceiver(%s) status: [%p]%s",debugName(),this,encloseDashes(s));
}

void Transceiver::syncGSMTimeSent(const GSMTime& time)
{
}

bool Transceiver::sendBurst(GSMTime time)
{
    String t;
    XDebug(this,DebugAll,"sendBurst(%s) T2=%d T3=%d [%p]",
	time.c_str(t),(time.fn() % 26),(time.fn() % 51),this);
    // Build send data
    bool first = true;
    for (unsigned int i = 0; i < m_arfcnCount; i++) {
	bool owner = false;
	GSMTxBurst* burst = m_arfcn[i]->getBurst(time,owner);
	if (!burst)
	    continue;
	const ComplexVector* fs = m_signalProcessing.arfcnFS(i);
	if (first) {
	    first = false;
	    m_sendBurstBuf.resize(burst->txData().length());
	    Complex::multiply(m_sendBurstBuf.data(),m_sendBurstBuf.length(),
		burst->txData().data(),burst->txData().length(),fs->data(),fs->length());
	}
	else
	    Complex::sumMul(m_sendBurstBuf.data(),m_sendBurstBuf.length(),
		burst->txData().data(),burst->txData().length(),fs->data(),fs->length());
	if (owner)
	    m_arfcn[i]->m_txBurstStore.store(burst);
    }
    // Reset TX buffer if nothing was set there
    if (first)
	m_sendBurstBuf.resize(m_signalProcessing.gsmSlotLen(),true);
    // Send frequency shifting vectors only?
    if (m_sendArfcnFS)
	m_signalProcessing.sumFreqShift(m_sendBurstBuf,m_sendArfcnFS);
    if (m_dumpOneTx) {
	m_dumpOneTx = false;
	t = "Transceiver sending ";
	t << m_sendBurstBuf.length() << " at " << time << "\r\n";
	m_sendBurstBuf.output(6,SigProcUtils::appendComplex,"-----"," ",t);
    }
    // Send data
    ComplexVector& data = m_sendBurstBuf;
    // Loopback ?
    if (m_loopback) {
	sendLoopback(m_sendBurstBuf,time);
	return true;
    }
    // Test ?
    if (m_testIncrease || s_dumper)
	adjustSendData(data,time);
    m_txIO.timestamp = m_signalProcessing.slots2ts(time);
    unsigned int code = m_radio->send(m_txIO.timestamp,
	(float*)data.data(),data.length(),&m_txPowerScale);
    m_txIO.timestamp += data.length();
    m_txIO.bursts++;
    if (m_txIO.updateError(code == 0))
	return true;
    if (!thShouldExit(this)) {
	Alarm(this,"system",DebugWarn,"Too many send errors, last: 0x%x %s [%p]",
	    code,RadioInterface::errorName(code),this);
	fatalError();
    }
    return false;
}

// Process a received radio burst
bool Transceiver::processRadioBurst(unsigned int arfcn, ArfcnSlot& slot, GSMRxBurst& b)
{
    Debug(this,DebugStub,"Transceiver::processRadioBurst() not implemented");
    return false;
}

// Start the transceiver, stop it if already started
bool Transceiver::start()
{
    DBGFUNC_TRXOBJ("Transceiver::start()",this);
    Lock lck(m_stateMutex);
    if (!m_radio || m_state == Invalid)
	return false;
    if (m_state > Idle)
	return true;
    Debug(this,DebugAll,"Starting [%p]",this);
    while (true) {
	if (!m_clockIface.initSocket(*this))
	    break;
	unsigned int n = 0;
	for (; n < m_arfcnCount; n++)
	    if (!m_arfcn[n]->start())
		break;
	if (n < m_arfcnCount)
	    break;
	changeState(PowerOff);
	return true;
    }
    lck.drop();
    stop();
    return false;
}

// Stop the transceiver
void Transceiver::stop(bool dumpStat, bool notify)
{
    DBGFUNC_TRXOBJ("Transceiver::stop()",this);
    TrxWorker::softCancel();
    m_stateMutex.lock();
    if (m_exiting) {
	m_stateMutex.unlock();
	return;
    }
    m_exiting = true;
    if (m_state > Idle) {
	Debug(this,DebugAll,"Stopping [%p]",this);
	if (notify)
	    syncGSMTime("EXITING");
	changeState(Idle);
    }
    TrxWorker::cancelThreads(this,0,&m_radioInThread);
    m_stateMutex.unlock();
    if (dumpStat)
	dumpStatus(true);
    radioPowerOff();
    m_stateMutex.lock();
    stopARFCNs();
    m_exiting = false;
    m_stateMutex.unlock();
}

// Execute a command
bool Transceiver::command(const char* str, String* rsp, unsigned int arfcn)
{
    String s = str;
    if (!s)
	return false;
    if (!s.startSkip("CMD ",false)) {
	Debug(this,DebugNote,"Invalid command '%s' [%p]",str,this);
	syncGSMTime();
	return false;
    }
    if (arfcn == 0xffffffff) {
	int pos = s.find(' ');
	if (pos > 0) {
	    arfcn = s.substr(0,pos).toInteger();
	    s = s.substr(pos + 1);
	}
	else {
	    arfcn = s.toInteger();
	    s.clear();
	}
    }
    Debug(this,DebugAll,"Handling command '%s' arfcn=%u [%p]",str,arfcn,this);
    String cmd;
    String reason;
    String rspParam;
    int status = CmdEOk;
    int c = getCommand(s,cmd);
    switch (c) {
	case CmdHandover:
	case CmdNoHandover:
	    status = handleCmdHandover(c == CmdHandover,arfcn,s,&rspParam);
	    break;
	case CmdSetSlot:
	    status = handleCmdSetSlot(arfcn,s,&rspParam);
	    break;
	case CmdSetPower:
	    status = handleCmdSetPower(arfcn,s,&rspParam);
	    break;
	case CmdAdjPower:
	    status = handleCmdAdjustPower(arfcn,s,&rspParam);
	    break;
	case CmdSetTxAtten:
	    status = handleCmdSetTxAttenuation(arfcn,s,&rspParam);
	    break;
	case CmdRxTune:
	case CmdTxTune:
	    status = handleCmdTune(c == CmdRxTune,arfcn,s,&rspParam);
	    break;
	case CmdSetTsc:
	    status = handleCmdSetTsc(arfcn,s,&rspParam);
	    break;
	case CmdSetMaxDelay:
	    status = 0;
	    rspParam = s;
	    break;
	case CmdSetRxGain:
	    status = handleCmdSetGain(c == CmdSetRxGain,s,&rspParam);
	    break;
	case CmdReadFactory:
	    // NOTE: Should we return the requested parameter name?
	    if (m_radio) {
		rspParam = "0";
		if (s != YSTRING("sdrsn"))
		    Debug(this,DebugStub,"Unknown factory value '%s' [%p]",
			s.c_str(),this);
	    }
	    break;
	case CmdCustom:
	    status = handleCmdCustom(s,&rspParam,&reason);
	    break;
	case CmdPowerOn:
	    status = (radioPowerOn(&reason) ? CmdEOk : CmdEFailure);
	    break;
	case CmdPowerOff:
	    radioPowerOff();
	    break;
	case CmdManageTx: // NOTE: this command is just for test
	    handleCmdManageTx(arfcn,s,&rspParam);
	    break;
	case CmdNoise:
	{
	    String dump;
	    for (unsigned int i = 0;i < m_arfcnCount;i++) {
		dump << "ARFCN: ";
		dump <<  i;
		dump << ": noise: ";
		dump << (int)m_arfcn[i]->getAverageNoiseLevel();
		dump << " ; ";
	    }
	    Debug(this,DebugNote,"%s",dump.c_str());
	    rspParam << (int)-m_arfcn[0]->getAverageNoiseLevel();
	    break;
	}
	case CmdFreqOffset:
	    status = handleCmdFreqCorr(s,&rspParam);
	    break;
	default:
	    status = CmdEUnkCmd;
    }
    if (status)
	Debug(this,DebugNote,"Command '%s' (ARFCN=%u) failed in state %s: %d %s [%p]",
	    str,arfcn,trxStateName(state()),status,
	    reason.safe(lookup(status,s_cmdErrorName,"failure")),this);
    else
	Debug(this,DebugAll,"Command '%s' (ARFCN=%u) RSP '%s'",str,arfcn,rspParam.c_str());
    syncGSMTime();
    return buildCmdRsp(rsp,cmd,status,rspParam);
}

bool Transceiver::control(const String& oper, const NamedList& params)
{
    if (oper == YSTRING("print-status")) {
	m_printStatus = params.getIntValue(YSTRING("count"),1);
	m_printStatusBursts = m_printStatus && !params.getBoolValue(YSTRING("nobursts"));
	m_printStatusChanged = true;
    }
    else
	return false;
    return true;
}

// Set the minimum power to accept radio bursts
void Transceiver::setBurstMinPower(int val)
{
    if (m_burstMinPower == val)
	return;
    m_burstMinPower = val;
    Debug(this,DebugInfo,"Burst minimum power level set to %d [%p]",val,this);
}

// Internal fatal error
void Transceiver::fatalError()
{
    m_error = true;
}

// Worker terminated notification
void Transceiver::workerTerminated(Thread* th)
{
    if (!th)
	return;
    Lock lck(TrxWorker::s_mutex);
    if (m_radioInThread == th)
	m_radioInThread = 0;
    else if (m_radioOutThread == th)
	m_radioOutThread = 0;
    else if (m_radioReadThread == th)
	m_radioReadThread = 0;
}

// Destroy the object
void Transceiver::destroyed()
{
    stop();
    TransceiverObj::destroyed();
}

// Starting radio power on notification
void Transceiver::radioPowerOnStarting()
{
    m_rxQueue.clear();
    m_txPower = -20;
}

// Set channel (slot) type
void Transceiver::setChanType(unsigned int arfcn, unsigned int slot, int chanType)
{
    if (arfcn < m_arfcnCount)
	m_arfcn[arfcn]->initTimeslot(slot,chanType);
}

// ARFCN list changed notification
void Transceiver::arfcnListChanged()
{
}

// Retrieve the state name dictionary
const TokenDict* Transceiver::dictStateName()
{
    return s_stateName;
}

bool Transceiver::radioSetPower(int p)
{
    int newVal = 0;
    unsigned int code = m_radio->setGain(true,-p,0,&newVal);
    if (!radioCodeOk(code))
	return false;
    float f = (float)(p + newVal);
    float tmp = ::powf(10.0,0.1 * f);
    m_txPowerScale = (tmp < 1 ? 1.0 : (1.0 / ::sqrt(tmp))) / m_arfcnConf;
    Debug(this,DebugInfo,"Set Tx gain=%d power_scaling=%g [%p]",
	newVal,m_txPowerScale,this);
    return true;
}

// Power on the radio if not already done
bool Transceiver::radioPowerOn(String* reason)
{
    DBGFUNC_TRXOBJ("Transceiver::radioPowerOn()",this);
    Lock lck(m_stateMutex);
    if (m_state == PowerOn)
	return true;
    if (!m_radio || m_state < PowerOff || !m_arfcnCount)
	return setReason(reason,"invalid state");
    if (!m_rxFreq)
	return setReason(reason,"Rx frequency not set");
    if (!m_txFreq)
	return setReason(reason,"Tx frequency not set");
    radioPowerOnStarting();
    const char* reasonStr = "failure";
    while (true) {
	Debug(this,DebugInfo,"Starting radio [%p]",this);
	if (m_radio->initialize() != 0) {
	    reasonStr = "radio failure";
	    break;
	}
	if (!TrxWorker::create(m_radioReadThread,TrxWorker::TrxRadioRead,this,m_radioReadPrio))
	    break;
	if (!TrxWorker::create(m_radioInThread,TrxWorker::TrxRadioIn,this))
	    break;
	if (!TrxWorker::create(m_radioOutThread,TrxWorker::TrxRadioOut,this,m_radioOutPrio))
	    break;
	unsigned int i = 0;
	for (; i < m_arfcnCount; i++)
	    if (!m_arfcn[i]->radioPowerOn(reason))
		break;
	if (i < m_arfcnCount)
	    break;
	unsigned int code = m_radio->getTxTime(m_txIO.timestamp);
	if (code) {
	    Debug(this,DebugNote,"Failed to retrieve radio TX time (0x%x %s), using 0 [%p]",
		code,RadioInterface::errorName(code),this);
	    m_txIO.timestamp = 0;
	}
	code = m_radio->getRxTime(m_rxIO.timestamp);
	if (code) {
	    Debug(this,DebugNote,"Failed to retrieve radio RX time (0x%x %s), using 0 [%p]",
		code,RadioInterface::errorName(code),this);
	    m_rxIO.timestamp = 0;
	}
	changeState(PowerOn);
	return true;
    }
    lck.drop();
    radioPowerOff();
    return setReason(reason,reasonStr);
}

// Power off the radio if not already done
void Transceiver::radioPowerOff()
{
    DBGFUNC_TRXOBJ("Transceiver::radioPowerOff()",this);
    TrxWorker::softCancel(TrxWorker::RadioMask);
    m_stateMutex.lock();
    TrxWorker::cancelThreads(this,0,&m_radioInThread,0,&m_radioOutThread,
	0,&m_radioReadThread);
    // Signal Tx ready: this will stop us waiting for Tx ready
    m_txSync.unlock();
    for (unsigned int i = 0; i < m_arfcnCount; i++)
	m_arfcn[i]->radioPowerOff();
    if (m_state == PowerOn)
	changeState(PowerOff);
    m_rxQueue.clear();
    m_stateMutex.unlock();
}

void Transceiver::changeState(int newState)
{
    if (m_state == newState)
	return;
    Debug(this,DebugNote,"State changed %s -> %s [%p]",
	trxStateName(m_state),trxStateName(newState),this);
    m_state = newState;
}

static inline void clearARFCNs(ARFCN**& ptr, unsigned int& n)
{
    if (!ptr)
	return;
    for (unsigned int i = 0; i < n; i++)
	TelEngine::destruct(ptr[i]);
    delete[] ptr;
    ptr = 0;
    n = 0;
}

// Initialize the ARFCNs list (set or release)
bool Transceiver::resetARFCNs(unsigned int arfcns, int port, const String* rAddr,
    const char* lAddr, unsigned int nFillers)
{
    ARFCN** old = m_arfcn;
    if (m_arfcn) {
	stopARFCNs();
	clearARFCNs(m_arfcn,m_arfcnCount);
    }
    m_arfcnCount = arfcns;
    if (!m_arfcnCount) {
	if (old)
	    arfcnListChanged();
	return true;
    }
    m_arfcn = new ARFCN*[m_arfcnCount];
    for (unsigned int i = 0; i < m_arfcnCount; i++) {
	if (rAddr)
	    m_arfcn[i] = new ARFCNSocket(i);
	else
	    m_arfcn[i] = new ARFCN(i);
	m_arfcn[i]->setTransceiver(*this,"ARFCN[" + String(i) + "]");
	m_arfcn[i]->debugChain(this);
	GSMTxBurst* filler = 0;
	if (m_arfcn[i]->arfcn() == 0) {
	    filler = GSMTxBurst::buildFiller();
	    if (filler) {
		filler->buildTxData(m_signalProcessing);
		if (!filler->txData().length())
		    TelEngine::destruct(filler);
	    }
	}
	m_arfcn[i]->m_fillerTable.init(nFillers,filler);
    }
    if (rAddr)
	for (unsigned int i = 0; i < m_arfcnCount; i++) {
	    ARFCNSocket* a = static_cast<ARFCNSocket*>(m_arfcn[i]);
	    int p = port + 2 * i + 1;
	    if (a->initUDP(p + 100 + 1,*rAddr,p + 1,lAddr))
		continue;
	    clearARFCNs(m_arfcn,m_arfcnCount);
	    if (old)
		arfcnListChanged();
	    return false;
	}
    arfcnListChanged();
    return true;
}

// Stop all ARFCNs
void Transceiver::stopARFCNs()
{
    for (unsigned int i = 0; i < m_arfcnCount; i++)
	m_arfcn[i]->stop();
}

// Sync upper layer GSM clock (update time)
bool Transceiver::syncGSMTime(const char* msg)
{
    String tmp(msg ? msg : "IND CLOCK ");
    Lock lck(m_clockUpdMutex);
    if (!msg) {
	m_lastClockUpd = m_txTime + m_clockUpdOffset;
	tmp << m_lastClockUpd.fn();
	// Next update in 1 second
	m_nextClockUpdTime = m_txTime + GSM_SLOTS_SEC;
#ifdef XDEBUG
	String sent;
	String crt;
	String next;
	Debug(this,DebugInfo,"Sending radio time sync %s at %s next %s [%p]",
	    m_lastClockUpd.c_str(sent),m_txTime.c_str(crt),m_nextClockUpdTime.c_str(next),this);
#endif
    }
    else
	Debug(this,DebugAll,"Sending '%s' on clock interface [%p]",msg,this);
    if (m_clockIface.m_socket.valid()) {
	if (m_clockIface.writeSocket(tmp.c_str(),tmp.length() + 1,*this) > 0) {
	    if (!msg) {
		GSMTime t = m_lastClockUpd;
		lck.drop();
		syncGSMTimeSent(t);
	    }
	    return true;
	}
    }
    else if (msg && !m_exiting)
	Debug(this,DebugFail,"Clock interface is invalid [%p]",this);
    if (msg && !m_exiting)
	return false;
    lck.drop();
    fatalError();
    return false;
}

#define TRX_SET_ERROR_BREAK(e) { code = e; break; }

// Handle (NO)HANDOVER commands. Return status code
int Transceiver::handleCmdHandover(bool on, unsigned int arfcn, String& cmd,
    String* rspParam)
{
    int code = CmdEFailure;
    while (true) {
	if (m_state != PowerOn)
	    TRX_SET_ERROR_BREAK(CmdEInvalidState);
	if (arfcn >= m_arfcnCount)
	    TRX_SET_ERROR_BREAK(CmdEInvalidARFCN);
	int slot = cmd.toInteger(-1);
	if (slot < 0 || slot > 7)
	    TRX_SET_ERROR_BREAK(CmdEInvalidParam);
	m_arfcn[arfcn]->setBurstAccess((unsigned int)slot,on);
	if (rspParam)
	    *rspParam << slot;
	return 0;
    }
    if (rspParam)
	*rspParam = cmd;
    return code;
}

// Handle SETSLOT command. Return status code
int Transceiver::handleCmdSetSlot(unsigned int arfcn, String& cmd, String* rspParam)
{
    int code = CmdEFailure;
    while (true) {
	if (m_state < Idle)
	    TRX_SET_ERROR_BREAK(CmdEInvalidState);
	if (arfcn >= m_arfcnCount)
	    TRX_SET_ERROR_BREAK(CmdEInvalidARFCN);
	int slot = -1;
	int chanType = -1;
	int pos = cmd.find(' ');
	if (pos > 0) {
	    slot = cmd.substr(0,pos).toInteger(-1);
	    chanType = cmd.substr(pos + 1).toInteger(-1);
	}
	if (slot < 0 || slot > 7)
	    TRX_SET_ERROR_BREAK(CmdEInvalidParam);
	setChanType(arfcn,(unsigned int)slot,chanType);
	if (rspParam)
	    *rspParam << slot << " " << chanType;
	return 0;
    }
    if (rspParam)
	*rspParam = cmd;
    return code;
}

int Transceiver::handleCmdSetPower(unsigned int arfcn, String& cmd, String* rspParam)
{
    int code = CmdEFailure;
    while (true) {
	if (!m_radio || m_state != PowerOn)
	    TRX_SET_ERROR_BREAK(CmdEInvalidState);
	// Power can be set for ARFCN 0 only
	if (arfcn)
	    TRX_SET_ERROR_BREAK(CmdEInvalidARFCN);
	if (!cmd)
	    TRX_SET_ERROR_BREAK(CmdEInvalidParam);
	int p = cmd.toInteger();
	if (!radioSetPower(p + m_txAttnOffset))
	    break;
	m_txPower = p;
	if (rspParam)
	    *rspParam << p;
	return 0;
    }
    if (rspParam)
	*rspParam = cmd;
    return code;
}

int Transceiver::handleCmdAdjustPower(unsigned int arfcn, String& cmd, String* rspParam)
{
    int code = CmdEFailure;
    while (true) {
	if (!m_radio || m_state != PowerOn)
	    TRX_SET_ERROR_BREAK(CmdEInvalidState);
	// Power can be set for ARFCN 0 only
	if (arfcn)
	    TRX_SET_ERROR_BREAK(CmdEInvalidARFCN);
	if (!cmd)
	    TRX_SET_ERROR_BREAK(CmdEInvalidParam);
	int p = cmd.toInteger();
	m_txPower += p;
	if (rspParam)
	    *rspParam << m_txPower;
	return 0;
    }
    if (rspParam)
	*rspParam = cmd;
    return code;
}

int Transceiver::handleCmdSetTxAttenuation(unsigned int arfcn, String& cmd, String* rspParam)
{
    int code = CmdEFailure;
    while (true) {
	if (!m_radio || m_state != PowerOn)
	    TRX_SET_ERROR_BREAK(CmdEInvalidState);
	// Power can be set for ARFCN 0 only
	if (arfcn)
	    TRX_SET_ERROR_BREAK(CmdEInvalidARFCN);
	if (!cmd)
	    TRX_SET_ERROR_BREAK(CmdEInvalidParam);
	int p = cmd.toInteger();
	if (!radioSetPower(m_txPower + p))
	    break;
	m_txAttnOffset = p;
	if (rspParam)
	    *rspParam << p;
	return 0;
    }
    if (rspParam)
	*rspParam = cmd;
    return code;
}

// Handle RXTUNE/TXTUNE commands
int Transceiver::handleCmdTune(bool rx, unsigned int arfcn, String& cmd, String* rspParam)
{
    int code = CmdEFailure;
    while (true) {
	if (!m_radio)
	    TRX_SET_ERROR_BREAK(CmdEInvalidState);
	if (arfcn)
	    TRX_SET_ERROR_BREAK(CmdEInvalidARFCN);
	if (!cmd)
	    TRX_SET_ERROR_BREAK(CmdEInvalidParam);
	int freq = cmd.toInteger();
	// Frequency is given in KHz, radio expects it in Hz
	freq *= 1000;
	// Adjust frequency (we are using multiple ARFCNs)
	freq += 600000;
	unsigned int code = rx ? m_radio->setRxFreq(freq) : m_radio->setTxFreq(freq);
	if (!radioCodeOk(code))
	    break;
	double& f = rx ? m_rxFreq : m_txFreq;
	f = freq;
	if (rspParam)
	    *rspParam << freq;
	return 0;
    }
    if (rspParam)
	*rspParam = cmd;
    return code;
}

// Handle SETTSC commands. Return status code
int Transceiver::handleCmdSetTsc(unsigned int arfcn, String& cmd, String* rspParam)
{
    int code = CmdEFailure;
    while (true) {
	if (arfcn)
	    TRX_SET_ERROR_BREAK(CmdEInvalidARFCN);
	int tsc = cmd.toInteger(-1);
	if (tsc < 0 || tsc > 7)
	    TRX_SET_ERROR_BREAK(CmdEInvalidParam);
	if (m_tsc != (unsigned int)tsc) {
	    Debug(this,DebugInfo,"%sTSC changed %u -> %d [%p]",
		prefix(),m_tsc,tsc,this);
	    m_tsc = (unsigned int)tsc;
	}
	if (rspParam)
	    *rspParam << m_tsc;
	return 0;
    }
    if (rspParam)
	*rspParam = cmd;
    return code;
}

// Handle SETRXGAIN command. Return status code
int Transceiver::handleCmdSetGain(bool rx, String& cmd, String* rspParam)
{
    int code = CmdEFailure;
    while (true) {
	if (!m_radio)
	    TRX_SET_ERROR_BREAK(CmdEInvalidState);
	if (!cmd)
	    TRX_SET_ERROR_BREAK(CmdEInvalidParam);
	int gain = cmd.toInteger();
	int newGain = 0;
	unsigned int code = m_radio->setGain(!rx,gain,0,&newGain);
	if (!radioCodeOk(code))
	    break;
	if (rspParam)
	    *rspParam << newGain;
	return 0;
    }
    if (rspParam)
	*rspParam = cmd;
    return code;
}

// Handle SETRXGAIN command. Return status code
int Transceiver::handleCmdFreqCorr(String& cmd, String* rspParam)
{
    int code = CmdEFailure;
    while (true) {
	if (!m_radio)
	    TRX_SET_ERROR_BREAK(CmdEInvalidState);
	if (!cmd)
	    TRX_SET_ERROR_BREAK(CmdEInvalidParam);
	int newVal = 0;
	unsigned int code = m_radio->setFreqOffset(cmd.toInteger(),&newVal);
	if (!radioCodeOk(code))
	    break;
	*rspParam = newVal;
	return 0;
    }
    if (rspParam)
	*rspParam = cmd;
    return code;
}

// Send loopback data
void Transceiver::sendLoopback(ComplexVector& buf, const GSMTime& time)
{
    Lock lck(m_loopbackMutex);
    if (m_loopbackNextSend > Time::now()) {
	Thread::idle();
	return;
    }
    RadioRxData* r = m_radioRxStore.get();
    r->m_data.resize(buf.length());
    m_loopbackNextSend = Time::now() + m_loopbackSleep;
    GSMTime t = time;
    if (m_loopbackData) {
	ComplexVector* v = m_loopbackData;
	unsigned int cp = buf.length();
	if (v->length() == 1251) {
	    t.assign((*v)[1250].real(),(*v)[1250].imag());
	    cp = 1250;
	}
	r->m_data.copy(v->data(),cp <= r->m_data.length() ? cp : r->m_data.length());
	// TODO add Loopback time
	lck.drop();
    }
    else {
	lck.drop();
	r->m_data.copy(buf.data(),buf.length());
    }
    r->m_time = t;
    recvRadioData(r);
}

// Adjust data to send (from test or dumper)
void Transceiver::adjustSendData(ComplexVector& buf, const GSMTime& time)
{
    if (s_dumper) {
	Complex fill;
	switch (time.tn() % 4) {
	    case 0:
	    case 2:
		break;
	    case 1:
		fill.set(2047,2047);
		break;
	    case 3:
		fill.set(-2047,-2047);
		break;
	}
	buf.fill(fill);
	return;
    }
    if (!m_testIncrease)
	return;
    for (unsigned int i = 0; i < buf.length(); i++) {
	buf[i].set(m_testValue,m_testValue);
	m_testValue += m_testIncrease;
	if (m_testValue > m_testMax)
	    m_testValue = m_testMin;
    }
}

bool loadFileData(const String& fileName, ComplexVector& data)
{
    File f;
    if (!f.openPath(fileName)) {
	Debug(DebugNote,"Failed to open file '%s'!",fileName.c_str());
	return false;
    }
    DataBlock buf(0,(unsigned int)f.length());
    int read = f.readData(buf.data(),buf.length());
    if (f.length() != read) {
	Debug(DebugInfo,"Failed to read all the data from file %s",fileName.c_str());
	return false;
    }
    String s((char*)buf.data(),buf.length());
    return data.parse(s,&SigProcUtils::callbackParse);
}

// Handle CUSTOM command. Return status code
int Transceiver::handleCmdCustom(String& cmd, String* rspParam, String* reason)
{
    DDebug(this,DebugAll,"Handle custom cmd %s",cmd.c_str());
    if (cmd.startSkip("dumprx ",false)) {
	if (cmd == YSTRING("complex")) {
	    m_dumpOneRx= true;
	    return CmdEOk;
	}
	unsigned int arfcn = cmd.toInteger(0);
	if (arfcn >= m_arfcnCount)
	    return CmdEUnkCmd;
	ARFCNSocket* ar = YOBJECT(ARFCNSocket,m_arfcn[arfcn]);
	if (!ar)
	    return CmdEUnkCmd;
	ar->setPrintSocket();
	return CmdEOk;
    }
    if (cmd.startSkip("freqout ",false)) { 
	// Sends to output only the frequency shift vectors
	// "mbts radio freqout 0,1,2,3" summes all ARFCN's frequency shift vectors
	// and send them to output.
	ObjList* split = cmd.split(',');
	if (!split)
	    return CmdEUnkCmd;
	m_sendArfcnFS = 0;
	for (ObjList* o = split->skipNull(); o; o = o->skipNext()) {
	    String* s = static_cast<String*>(o->get());
	    unsigned int shift = (unsigned int)s->toInteger(0,0,0);
	    if (shift < 8)
		m_sendArfcnFS |= (1 << shift);
	}
	TelEngine::destruct(split);
	return CmdEOk;
    }
    if (cmd.startSkip("freqout")) {
	m_sendArfcnFS = 0;
	return CmdEOk;
    }
    if (cmd.startSkip("testtxburst ",false)) {
	int increment = -1;
	if (cmd == YSTRING("10"))
	    increment = 2;
	if (cmd == YSTRING("11"))
	    increment = 1;
	DataBlock d(0,GSM_BURST_TXPACKET);
	uint8_t* testData = (uint8_t*)d.data();
	for (int i = GSM_BURST_TXHEADER; i < GSM_BURST_LENGTH && increment > 0; i += increment)
	    testData[i] = 1;
	GSMTxBurstStore store(1,"TrxTestBurst");
	Lock myLock(m_testMutex);
	TelEngine::destruct(m_txTestBurst);
	m_txTestBurst = GSMTxBurst::parse(d,store);
	if (!m_txTestBurst)
	    return CmdEFailure;
	return CmdEOk;
    }
    if (cmd.startSkip("testtxburst")) {
	Lock myLock(m_testMutex);
	TelEngine::destruct(m_txTestBurst);
	return CmdEOk;
    }
    if (cmd == YSTRING("dumponetx")) {
	m_dumpOneTx = true;
	return CmdEOk;
    }
    if (cmd.startSkip(YSTRING("complexrx "),false)) {
	ComplexVector* cv = new ComplexVector();
	if (!loadFileData(cmd,*cv))
	    return CmdEFailure;
	// TODO See how to set burst type
	setLoopbackData(cv);
	return CmdEOk;
    }
    if (cmd == YSTRING("complexrx")) {
	setLoopbackData();
	return CmdEOk;
    }
    if (cmd.startSkip("predefined ",false)) {
	int min = 0, max = 0, inc = 0;
	int ret = ::sscanf(cmd.c_str(),"%d %d %d",&min,&max,&inc);
	if (ret != 3 || min > max)
	    return CmdEUnkCmd;
	if (!m_radio)
	    return CmdEInvalidState;
	setTestData(min,max,inc);
	return CmdEOk;
    }
    if (cmd.startSkip("internal-loopback ",false)) {
	if (!m_radio)
	    return CmdEInvalidState;
	setLoopback(true,cmd.toInteger());
	return CmdEOk;
    }
    if (cmd.startSkip("internal-loopback",false)) {
	if (!m_radio)
	    return CmdEInvalidState;
	setLoopback(false);
	return CmdEOk;
    }
    if (cmd.startSkip("dump-freq-shift ",false)) {
	dumpFreqShift(cmd.toInteger());
	return CmdEOk;
    }
    if (cmd.startSkip("noise",false)) {
	String dump;
	for (unsigned int i = 0;i < m_arfcnCount;i++) {
	    dump << "ARFCN: ";
	    dump <<  i;
	    dump << ": average noise level: ";
	    SigProcUtils::appendFloat(dump,m_arfcn[i]->getAverageNoiseLevel(),"");
	    dump << " ; ";
	}
	*rspParam << dump;
	return CmdEOk;
    }
    if (cmd.startSkip("slots-delay")) {
	if (rspParam) {
	    String dump;
	    for (unsigned int i = 0;i < m_arfcnCount;i++) 
		m_arfcn[i]->dumpSlotsDelay(*rspParam);
	}
	return CmdEOk;
    }
    if (cmd.startSkip("file-dump ",false)) {
	if (FileDataDumper::start(s_dumper,cmd))
	    return CmdEOk;
	return CmdEFailure;
    }
    if (cmd == YSTRING("file-dump")) {
	FileDataDumper::stop(s_dumper);
	return CmdEOk;
    }
    if (cmd == YSTRING("shutdown")) {
	Debug(this,DebugInfo,"Received shutdown command");
	m_shutdown = true;
	fatalError();
	return CmdEOk;
    }
    if (cmd.startSkip("show-mbts-traffic ",false)) {
	ObjList* split = cmd.split(' ',false);
	ObjList* o = split->skipNull();
	if (!o) {
	    TelEngine::destruct(split);
	    return CmdEFailure;
	}
	int arfcn = (static_cast<String*>(o->get()))->toInteger(-1);
	if (arfcn < 0 || arfcn > 4) {
	    Debug(this,DebugNote,"Invalid ARFCN index %d.",arfcn);
	    TelEngine::destruct(split);
	    return CmdEFailure;
	}
	o = o->skipNext();
	if (!o) {
	    Debug(this,DebugNote,"Unable to obtain direction.");
	    TelEngine::destruct(split);
	    return CmdEFailure;
	}
	String* direct = static_cast<String*>(o->get());
	bool direction = *direct == YSTRING("out");
	o = o->skipNext();
	if (!o) {
	    TelEngine::destruct(split);
	    return CmdEFailure;
	}
	m_arfcn[arfcn]->showTraffic(direction,*o);
	TelEngine::destruct(split);
	return CmdEOk;
    }
    if (cmd.startSkip("show-mbts-traffic",false)) {
	Output("mbts radio show-mbts-traffic arfcn[0,1,2-3] in/out enable[true/false] timeslot[0,1,5-7] devider[0 to 51] devider pos[1,11,21,31,41]");
	return CmdEOk;
    }
    if (cmd.startSkip("traffic-shape ",false)) {
	ObjList* split = cmd.split(' ',false);
	ObjList* o = split->skipNull();
	int arfcn = (static_cast<String*>(o->get()))->toInteger(-1);
	if (arfcn < 0 || arfcn > 3) {
	    Debug(this,DebugNote,"Invalid ARFCN index %d",arfcn);
	    return CmdEFailure;
	}
	o = o->skipNext();
	m_arfcn[arfcn]->m_shaper.parse(o);
	TelEngine::destruct(split);
	return CmdEOk;
    }
    if (cmd.startSkip("traffic-shape")) {
	Output("mbts radio traffic-shape arfcn[0,1,2-3] timeslot[0,1,5-7] value");
	return CmdEOk;
    }
    if (cmd.startSkip(YSTRING("toa-shift ")),false) {
	m_toaShift = cmd.toInteger();
	return CmdEOk;
    }
    return CmdEUnkCmd;
}

int Transceiver::handleCmdManageTx(unsigned int arfcn, String& cmd, String* rspParam)
{
    NamedList params("");
    ObjList* p = cmd.split(' ');
    for (ObjList* o = p->skipNull();o;o = o->skipNext()) {
	String* nvs = static_cast<String*>(o->get());
	ObjList* nvo = nvs->split('=');
	if (nvo->count() != 2) {
	    TelEngine::destruct(nvo);
	    continue;
	}
	params.addParam(*static_cast<String*>((*nvo)[0]),*static_cast<String*>((*nvo)[1]));
	TelEngine::destruct(nvo);
    }
    TelEngine::destruct(p);
    String command = params.getParam(YSTRING("cmd"));
    if (command == YSTRING("dump")) {
	m_arfcn[arfcn]->dumpBursts();
	return 0;
    }
    DDebug(this,DebugStub,"Received ManageTx command %s",command.c_str());
    return 0; 
}


//
// TransceiverQMF
//
TransceiverQMF::TransceiverQMF(const char* name)
    : Transceiver(name),
    m_halfBandFltCoeffLen(11),
    m_tscSamples(26),
#ifdef TRANSCEIVER_DUMP_DEMOD_PERF
    m_checkDemodPerf(true)
#else
    m_checkDemodPerf(false)
#endif
{
    initNormalBurstTSC();
    initAccessBurstSync();
}

static void adjustParam(TransceiverObj* obj, NamedList& p, const String& param,
    unsigned int value)
{
    String* s = p.getParam(param);
    int val = s ? s->toInteger(-1) : -1;
    if (val == (int)value)
	return;
    if (!s) {
	p.setParam(param,String(value));
	return;
    }
    if (obj) {
	int level = (val < (int)value) ? DebugAll : DebugConf;
	Debug(obj,level,"%sAdjusting parameter %s='%s' -> %u [%p]",
	    obj->prefix(),param.c_str(),s->c_str(),value,obj);
    }
    *s = value;
}

// Initialize the transceiver. This method should be called after construction
bool TransceiverQMF::init(RadioInterface* radio, const NamedList& params)
{
    if (m_state != Invalid)
	return Transceiver::init(radio,params);
    // First init: adjust ARFCNs and oversampling
    NamedList p(params);
    adjustParam(this,p,YSTRING("oversampling"),8);
    adjustParam(this,p,YSTRING("arfcns"),4);
    p.addParam("conf_arfcns",params.getValue(YSTRING("arfcns"),"1"));
    return Transceiver::init(radio,p);
}

// Re-Initialize the transceiver
void TransceiverQMF::reInit(const NamedList& params)
{
    Transceiver::reInit(params);
    m_tscSamples = getUInt(params,YSTRING("chan_estimator_tsc_samples"),26,2,26);
}

// Process a received radio burst
bool TransceiverQMF::processRadioBurst(unsigned int arfcn, ArfcnSlot& slot, GSMRxBurst& b)
{
    ARFCN* a = this->arfcn(arfcn);
    if (!a)
	return false;

    const GSMTime& t = b.time();
    unsigned int len = b.m_data.length();
    int bType = ARFCN::burstType(slot,t.fn());
    if (bType != ARFCN::BurstNormal && bType != ARFCN::BurstAccess) {
	if (statistics()) {
	    String tmp;
	    int level = -1;
	    if (slot.warnRecvDrop) {
		slot.warnRecvDrop = false;
		level = (bType != ARFCN::BurstUnknown) ? DebugInfo : DebugNote;
		tmp = "unhandled";
		tmp << " burst type (" << bType << "," << ARFCN::burstType(bType) << ")";
		tmp << " slot_type " << a->chanType(slot.type);
	    }
	    a->dropRxBurst(ARFCN::RxDropSlotUnk,t,len,level,0,tmp);
	}
	return false;
    }
    if (len < ARFCN_RXBURST_LEN)
	return false;

    // Use the center values of the input data to calculate the power level.
    float power = SignalProcessing::computePower(b.m_data.data(),
	b.m_data.length(),5,0.2,(148 - 4) / 2);

    // Use the last 4 samples of the input data (guard period) to calculate the noise level
    float noise = SignalProcessing::computePower(b.m_data.data(),
	b.m_data.length(),2,0.25,b.m_data.length() - 2)
	    + SignalProcessing::computePower(b.m_data.data(),b.m_data.length(),2,0.25,0);
    a->addAverageNoise(SignalProcessing::power2db(noise));

    // Calculate Signal to Noise ratio
    float SNR = power / noise;
    b.m_powerLevel = SignalProcessing::power2db(power);

#ifdef TRANSCEIVER_DUMP_ARFCN_PROCESS_IN
    a->dumpRecvBurst("processRadioBurst()",t,b.m_data.data(),len);
#else
    XDebug(a,DebugAll,
	"%sprocessRadioBurst burst_type=%s FN=%u TN=%u T2=%u T3=%u level=%f noise=%f "
	"SNR=%f power_db=%g low_SNR=%s lowPower=%s [%p]",
	a->prefix(),ARFCN::burstType(bType),t.fn(),slot.slot,t.fn() % 26,t.fn() % 51,
	power,noise,SNR,b.m_powerLevel,String::boolText(SNR < m_snrThreshold),
	String::boolText(b.m_powerLevel < m_burstMinPower),a);
#endif

    if (SNR < m_snrThreshold) {
	// Discard Burst low SNR
	a->dropRxBurst(ARFCN::RxDropLowSNR,t,len);
	return false;
    }

    if (b.m_powerLevel < m_burstMinPower) {
	a->dropRxBurst(ARFCN::RxDropLowPower,t,len);
	return false;
    }

    SignalProcessing::applyMinusPIOverTwoFreqShift(b.m_data);

    FloatVector* trainingSeq = 0;
    int start = 0, heLen = 0, center = 0;
    if (bType == ARFCN::BurstNormal) {
	trainingSeq  = &m_nbTSC[m_tsc];
	start = b.m_data.length() / 2 - GSM_NB_TSC_LEN / 2;
	heLen = GSM_NB_TSC_LEN;
	center = GSM_NB_TSC_LEN / 2 - 1;
    }
    else {
	trainingSeq = &m_abSync;
	start = 8;
	center = 24; // HardCodded Ask David
	heLen = GSM_AB_SYNC_LEN + m_maxPropDelay;
    }

    dumpRxData("correlate-in1",arfcn,"",b.m_data.data() + start,heLen);
    dumpRxData("correlate-in2",arfcn,"",trainingSeq->data(), trainingSeq->length());

    ComplexVector he(heLen);
    SignalProcessing::correlate(he,b.m_data,start,heLen,*trainingSeq);

    dumpRxData("correlate-out",arfcn,"",he.data(), he.length());

    // Find the peak power in the channel estimate.
    float max = 0, rms = 0;
    int maxIndex = -1;
    // GSM standards specififys that we should calculate from center +/- 2
    // We use center +/- 5 to be sure
    unsigned int centerMin = center - 5;
    unsigned int centerMax = center + 5;
    for (unsigned i = centerMin; i <= centerMax; i++) {
	float pwr = he[i].mulConj();
	rms += pwr;
	if (pwr < max) 
	    continue;
	max = pwr;
	maxIndex = i;
    }
    rms /= (centerMax - centerMin + 1);
    if (rms < 1e-12) {
	a->dropRxBurst(ARFCN::RxDropNullRMS,t,len,DebugNote);
	return false;
    }
    float peakOverMean = max / rms;
    XDebug(a,DebugAll,"%sSlot %d. chan max %f @ %d, rms %f, peak/mean %f [%p]",
	a->prefix(),slot.slot,max,maxIndex,rms,peakOverMean,a);
    if (peakOverMean < m_peakOverMeanThreshold) {
	a->dropRxBurst(ARFCN::RxDropLowPeakMin,t,len,DebugInfo);
	return false;
    }

    if (b.m_powerLevel > m_upPowerThreshold) {
	slot.powerErrors++;
	if (slot.lastPowerWarn < t) {
	    Debug(a,DebugInfo,"%sSlot %d. Receiver clipping %g dB (FN=%u) count=%u [%p]",
		a->prefix(),slot.slot,b.m_powerLevel,t.fn(),slot.powerErrors,a);
	    slot.lastPowerWarn = t + GSM_SLOTS_SEC;
	    slot.powerErrors = 0;
	}
    }

    // TOA error.  Negative for early, positive for late.
    int toaError = maxIndex - center;
    DDebug(a,DebugAll,"%sSlot %d. TOA error %d [%p]",a->prefix(),slot.slot,toaError,a);
    // indexing relationships
    // maxIndex - actual t0 of the received signal channel estimate
    // center - expected t0 of the received signal channel estimate
    // toaError - the offset of the channel estimate from its expected position, negative for early
    // TOA error should not exceed +/-2 for a normal burst.
    // It indicates a failure of the closed loop timing control.
    if (bType == ARFCN::BurstNormal && (toaError > 2 || toaError < -2)) {
	slot.toaErrors++;
	if (slot.lastTOAWarn < t) {
	    Debug(a,DebugAll,
		"%sSlot %d. Excessive TOA error=%d peak/mean=%g count=%u [%p]",
		a->prefix(),slot.slot,toaError,peakOverMean,slot.toaErrors,a);
	    slot.toaErrors = 0;
	    slot.lastTOAWarn = t + GSM_SLOTS_SEC;
	}
    }
    else if (slot.toaErrors && slot.lastTOAWarn < t) {
	Debug(a,DebugAll,"%sSlot %d. Excessive TOA errors %u [%p]",
	    a->prefix(),slot.slot,slot.toaErrors,a);
	slot.toaErrors = 0;
    }
    // Shift the received signal to compensate for TOA error.
    // This shifts the signal so that the max power image is aligned to the expected position.
    // The alignment error is +/- 1/2 symbol period;
    // fractional alignment is corrected by correlation with channel estimate
    if (toaError) {
	if (toaError < 0) {
	    b.m_data.copySlice(0,-toaError,b.m_data.length() + toaError);
	    b.m_data.reset(0,-toaError);
	}
	else {
	    unsigned int n = b.m_data.length() - toaError;
	    b.m_data.copySlice(toaError,0,n);
	    b.m_data.reset(n);
	}
    }
    b.m_timingError = toaError;

    // Trim and center the channel estimate.
    // This shifts the channel estimate to put the max power peak in the center, +/- 1/2 symbol period.
    // The alignment error of heTrim is exactly the opposiate of the alignment error of xf2.
    if (maxIndex < 0) {
	a->dropRxBurst(ARFCN::RxDropNegativeMaxIndex,t,len,DebugStub);
	return false;
    }
    // GSM standards specififys 10
    // To be sure we go 11
    // ASK David for details
    for (unsigned i = 0; i < 11; i++)
	he[i] = he[maxIndex - 5 + i];

    dumpRxData("demod-he",arfcn,"",he.data(), he.length());

    float sumTSq = 0;
    float sumP = 0;
    int m = 0;
    for(int i = 5; i < 11; i++, m++) {
	float p = he[i].mulConj();
	sumTSq += m * m * p;
	sumP += p;
    }
    float delaySpread = ::sqrt(sumTSq / sumP);
    a->addSlotDelay(slot.slot,delaySpread);

    FloatVector v(b.m_data.length());
    Equalizer::equalize(v,b.m_data,he,11);

    dumpRxData("demod-u",arfcn,"",v.data(), v.length());

    // Calculate the middle without guard period
    float powerSum = 0;
    for (unsigned int i = 72; i <= 76; i++)
	powerSum += v[i] * v[i];
    powerSum = ::sqrtf(powerSum * 0.2);
    for (unsigned int i = 8; i < ARFCN_RXBURST_LEN; i++) {
	float vi = v[i - BOARD_SHIFT] / powerSum;
	if (vi >= 1)
	    b.m_bitEstimate[i] = 255;
	else if (vi <= -1)
	    b.m_bitEstimate[i] = 0;
	else
	    b.m_bitEstimate[i] = (uint8_t)::round((vi + 1.0F) * 0.5 * 255);
    }

#if defined(TRANSCEIVER_DUMP_RX_DEBUG) || defined(TRANSCEIVER_DUMP_RX_INPUT_OUTPUT)
    if (debugAt(DebugAll)) {
#ifdef TRANSCEIVER_DUMP_RX_INPUT_OUTPUT
	RxInData::dump(t);
#endif
	String s;
	for (int i = 8; i < ARFCN_RXBURST_LEN; i++)
	    s << b.m_bitEstimate[i] << ",";
	Debug(a,DebugAll,"%sRX burst estimate: %s [%p]",a->prefix(),s.c_str(),a);
    }
#endif
    if (m_checkDemodPerf)
	checkDemodPerf(a,b,bType);

    a->m_rxTraffic.show(&b);
    return true;
}

// Starting radio power on notification
void TransceiverQMF::radioPowerOnStarting()
{
    Transceiver::radioPowerOnStarting();
    String tmp;
    const QMFBlockDesc* d = s_qmfBlock4;
    for (unsigned int i = 0; i < 15; i++, d++) {
	QmfBlock& b = m_qmf[i];
	b.arfcn = d->arfcn;
	if (b.freqShiftValue != d->freqShiftValue) {
	    b.freqShiftValue = d->freqShiftValue;
	    b.freqShift.clear();
	}
#if 0
	String buf;
	if (!tmp)
	    tmp << "\r\nIndex Name ARFCN FreqShift";
	String s;
	if (b.arfcn != 0xffffffff)
	    s = b.arfcn;
	buf.printf("%-5d %-4s %-5s %.15g",i,d->name,s.safe(),b.freqShiftValue);
	tmp << "\r\n" << buf;
#endif
    }
    if (tmp)
	Debug(this,DebugAll,"%sQMF nodes: [%p]%s",prefix(),this,encloseDashes(tmp));
    // Generate the half band filter coefficients
    if (m_halfBandFltCoeffLen && m_halfBandFltCoeff.length() != m_halfBandFltCoeffLen) {
	m_halfBandFltCoeff.assign(m_halfBandFltCoeffLen);
	float lq_1 = m_halfBandFltCoeff.length() - 1;
	float n0 = lq_1 / 2;
	float* d = m_halfBandFltCoeff.data();
	for (unsigned int i = 0; i < m_halfBandFltCoeff.length(); i++) {
	    float offset = (float)i - n0;
	    float omega = 0.54 - 0.46 * ::cosf(2 * PI * i / lq_1);
	    if (offset) {
		float func = PI / 2 * offset;
		*d++ = omega * (::sinf(func) / func);
	    }
	    else
		*d++ = omega;
	}
#ifdef TRANSCEIVER_DUMP_RX_DEBUG
	m_halfBandFltCoeff.output(20,SigProcUtils::appendFloat,"-----",",",
	    "Transceiver half band filter:\r\n");
#endif
    }
}

// Set channel (slot) type
void TransceiverQMF::setChanType(unsigned int arfcn, unsigned int slot, int chanType)
{
    Transceiver::setChanType(arfcn,slot,chanType);
    if (arfcn >= m_arfcnCount)
	return;
    for (unsigned int i = 0; i < 15; i++) {
	if (m_qmf[i].arfcn != arfcn)
	    continue;
	// Setup have chans availability flags in ARFCN node and parents
	for (unsigned int n = i; m_arfcn[arfcn]->chans(); n = (n - 1) / 2) {
	    m_qmf[n].chans = true;
	    if (!n)
		break;
	}
	break;
    }
}

// ARFCN list changed notification
void TransceiverQMF::arfcnListChanged()
{
    Transceiver::arfcnListChanged();
    for (unsigned int i = 0; i < 15; i++)
	m_qmf[i].chans = false;
}

void TransceiverQMF::dumpFreqShift(unsigned int index) const
{
    if (index > 14) {
	Debug(this,DebugNote,"Invalid qmf index %u",index);
	return;
    }
    String info;
    info << "QMF[" << index << "] frequency shifting vector:\r\n";
    m_qmf[index].freqShift.output(20,SigProcUtils::appendComplex,"-----",",",info);
}

// Process received radio data
void TransceiverQMF::processRadioData(RadioRxData* d)
{
    if (!d)
	return;
    GSMTime time = d->m_time;
    XDebug(this,DebugAll,"Processing radio input TN=%u FN=%u len=%u [%p]",
	time.tn(),time.fn(),d->m_data.length(),this);
    m_qmf[0].data.exchange(d->m_data);
    m_radioRxStore.store(d);
#ifdef TRANSCEIVER_DUMP_RX_INPUT_OUTPUT
    RxInData::add(m_qmf[0].data,time);
#endif
    qmf(time);
}

// Run the QMF algorithm
void TransceiverQMF::qmf(const GSMTime& time, unsigned int index)
{
    if (index > 14) {
	Debug(this,DebugFail,"qmf: invalid index %u [%p]",index,this);
	fatalError();
	return;
    }
    QmfBlock& crt = m_qmf[index];
    if (!(crt.chans && crt.data.data() && crt.data.length()))
	return;

    if (index == 0) {
	if (s_dumper) {
	    s_dumper->addData(crt.data);
	    return;
	}
	crt.power = 0;
	for (unsigned int i = 0;i < crt.data.length();i++)
	    crt.power += crt.data[i].mulConj();
	crt.power /= crt.data.length();
    }

    ARFCN* a = 0;
    int indexLo = -1;
    int indexHi = -1;
    if (index > 6) {
	a = (crt.arfcn < m_arfcnCount) ? m_arfcn[crt.arfcn] : 0;
	if (!a) {
	    Debug(this,DebugFail,"No ARFCN in QMF block at index %d [%p]",index,this);
	    fatalError();
	    return;
	}
    }
    else {
	indexLo = 2 * (int)index + 1;
	indexHi = indexLo + 1;
	if (!m_qmf[indexLo].chans)
	    indexLo = -1;
	if (!m_qmf[indexHi].chans)
	    indexHi = -1;
	if (indexLo < 0 && indexHi < 0)
	    return;
    }

    crt.power = SignalProcessing::power2db(crt.power);

#ifdef XDEBUG
    String tmp;
    tmp << "QMF[" << index << "] ";
    Debug(this,DebugAll,"%s len=%u low=%d high=%d powerDb=%f [%p]",
	  tmp.c_str(),crt.data.length(),indexLo,indexHi,crt.power,this);
#endif
#ifdef TRANSCEIVER_DUMP_QMF_IN
    String tmp1;
    tmp1 << "QMF[" << index << "] starting";
    dumpRecvBurst(tmp1,time,crt.data.data(),crt.data.length());
#endif
    dumpRxData("qmf[",index,"].x",crt.data.data(),crt.data.length());
    // Even if the power is too low compute noise from every 9th timeslot
    if ((crt.power < m_burstMinPower) && (time.timeslot() % 9))
	return;
    // Forward data to ARFCNs
    if (a) {
	RadioRxData* r = a->m_radioRxStore.get();
	r->m_time = time;
	r->m_data.exchange(crt.data);
	XDebug(this,DebugAll,"Forwarding radio data ARFCN=%u len=%u TN=%u FN=%u [%p]",
	    a->arfcn(),r->m_data.length(),time.tn(),time.fn(),this);
	a->recvRadioData(r);
	return;
    }
    // Frequency shift
    qmfApplyFreqShift(crt);
#ifdef TRANSCEIVER_DUMP_QMF_FREQSHIFTED
    String tmp2;
    tmp2 << "QMF[" << index << "] applied frequency shifting";
    dumpRecvBurst(tmp2,time,crt.data.data(),crt.data.length());
#endif
    dumpRxData("qmf[",index,"].xp",crt.data.data(),crt.data.length());
    // Build the half band filter, generate low/high band data and call QMF again
    qmfBuildHalfBandFilter(crt);
#ifdef TRANSCEIVER_DUMP_QMF_HALFBANDFILTER
    String tmp3;
    tmp3 << "QMF[" << index << "] built half band filter";
    dumpRecvBurst(tmp3,time,crt.halfBandFilter.data(),crt.halfBandFilter.length());
#endif
    dumpRxData("qmf[",index,"].w",crt.halfBandFilter.data(),crt.halfBandFilter.length());

    if (indexLo >= 0 && !thShouldExit(this)) {
	qmfBuildOutputLowBand(crt,m_qmf[indexLo].data,&m_qmf[indexLo].power);
	qmf(time,indexLo);
    }
    if (indexHi >= 0 && !thShouldExit(this)) {
	qmfBuildOutputHighBand(crt,m_qmf[indexHi].data,&m_qmf[indexHi].power);
	qmf(time,indexHi);
    }
}

// Build the half band filter
// x: the input data, kept in b.data
// w: half band filter data, kept in b.halfBandFilter
// h: half band filter coefficients, kept in m_halfBandFltCoeff
// w[i] = SUM[j=0..n0](x[i + 2 * j] * h[2 * j])
void TransceiverQMF::qmfBuildHalfBandFilter(QmfBlock& b)
{
    if (b.halfBandFilter.length() < b.data.length())
	b.halfBandFilter.resize(b.data.length() + 10);

    unsigned int n0 = (m_halfBandFltCoeff.length() + 1) / 2;
    unsigned int n0_2 = m_halfBandFltCoeff.length();
    Complex* x = b.data.data();
    Complex* w = b.halfBandFilter.data();
    float* h = m_halfBandFltCoeff.data();

    // From w vector only half of the values are used to determine the high and low band filters
    // So we skip calculating the data.

    // The input data should be padded with m_halfBandFltCoeff length.

    // substract represents the amount of data that should be at the start of x.
    int substract = n0 - m_halfBandFltCoeff.length() % 2;

    for (unsigned int i = 0;i < n0; i += 2, w += 2) {
	// If i + 2 * j < n0 the input data will contain 0;
	// So we have i + 2 * j >= n0 -> 2j >= n0 - i
	(*w).set(0,0);
	for (unsigned int j = (n0 - i);j <  n0_2;j += 2)
	    Complex::sumMulF(*w,x[i + j - substract],h[j]);
    }

    unsigned int end = b.data.length() - n0;
    end += b.data.length() % 2;

    for (unsigned int i = n0;i < end; i += 2, w += 2) {
	(*w).set(0,0);
	Complex* wx = &x[i - substract];
	float* wh = h;
	for (unsigned int j = 0;j < n0_2;j += 2, wx += 2,wh += 2)
	    Complex::sumMulF(*w,(*wx),(*wh));
    }

    for (unsigned int i = end;i < b.data.length(); i += 2, w += 2) {
	unsigned int lastJ = (b.data.length() - i) + n0;
	if (i + lastJ - substract >= b.data.length())
	    lastJ--;
	(*w).set(0,0);
	for (unsigned int j = 0;j < lastJ;j += 2)
	    Complex::sumMulF(*w,x[i + j - substract],h[j]);
    }
}

// Build the QMF low band output
// x: the input data, kept in b.data
// w: half band filter data, kept in b.halfBandFilter
// y[i] = (x[2 * i + 2 * n0] + w[2 * i]) / 2
void TransceiverQMF::qmfBuildOutputLowBand(QmfBlock& b, ComplexVector& y, float* power)
{
    y.resize(b.data.length() / 2);
    Complex* yData = y.data();
    Complex* x = b.data.data();
    Complex* w = b.halfBandFilter.data();
    *power = 0;

    for (unsigned int i = 0;i < y.length();i ++,x += 2,w += 2) {
	Complex::sum(yData[i],*x,*w);
	yData[i] *= 0.5f;
	*power += yData[i].mulConj();
    }
    *power /= y.length();
}

// Build the QMF high band output
// Behave as the input vector is prepended with n0 zero samples
// x: the input data, kept in b.data
// w: half band filter data, kept in b.halfBandFilter
// y[i] = x[2 * i + 2 * n0] - w[2 * i]
void TransceiverQMF::qmfBuildOutputHighBand(QmfBlock& b, ComplexVector& y, float* power)
{
    y.resize(b.data.length() / 2);
    Complex* yData = y.data();
    Complex* x = b.data.data();
    Complex* w = b.halfBandFilter.data();
    *power = 0;

    for (unsigned int i = 0;i < y.length();i ++,x += 2,w += 2) {
	Complex::diff(yData[i],*x,*w);
	yData[i] *= 0.5f;
	*power += yData[i].mulConj();
    }
    *power /= y.length();
}

void TransceiverQMF::initNormalBurstTSC(unsigned int len)
{
    if (!len || len > GSM_NB_TSC_LEN)
	len = 16;
    const int8_t* table = GSMUtils::nbTscTable();
    static const float s_normalScv = 1.0F / 16.0F;
    for (unsigned int i = 0;i < 8;i++, table += GSM_NB_TSC_LEN) {
	m_nbTSC[i].resize(len);
	const int8_t* p = table;
	p += 5; // David needs to explain why
	float* f = m_nbTSC[i].data();
	for (unsigned int n = 0;n < len;n++,f++, p++)
	    *f = *p ? s_normalScv : - s_normalScv;
    }
}

void TransceiverQMF::initAccessBurstSync()
{
    m_abSync.resize(GSM_AB_SYNC_LEN);

    const int8_t* p = GSMUtils::abSyncTable();
    static const float s_accessScv = 1.0F / 41.0F;
    for (unsigned int i = 0; i < GSM_AB_SYNC_LEN;i++, p++)
	m_abSync[i] = *p ? s_accessScv : -s_accessScv;
}

// Check demodulator performance
void TransceiverQMF::checkDemodPerf(const ARFCN* a, const GSMRxBurst& b, int bType)
{
    const GSMTime& t = b.time();
    int level = DebugTest;
    // Check valid bits
    uint8_t burst[148];
    unsigned int count = 0;
    for (int i = 8; i < ARFCN_RXBURST_LEN; i++) {
	burst[i - 8] = (b.m_bitEstimate[i] < 128 ? 0 : 1);
	if (b.m_bitEstimate[i] > 32 && b.m_bitEstimate[i] < 196)
	    count++;
    }
    String demodPerf = (count ? "poor" : "good");
    if (count) {
	level = DebugNote;
	demodPerf << " (uncertain values=" << count << ")";
    }
    // Check training sequence
    const int8_t* ts = GSMUtils::nbTscTable();
    ts += (GSM_NB_TSC_LEN * m_tsc);
    uint8_t* bp = burst + 61;
    String tscDump;
    for (unsigned int i = 0; i < GSM_NB_TSC_LEN; i++) {
	if (ts[i] == bp[i])
	    continue;
	String t1, t2;
	tscDump = "TSC:   " + dumpBits(t1,(void*)ts,GSM_NB_TSC_LEN) +
	    "\r\nburst: " + dumpBits(t2,bp,GSM_NB_TSC_LEN);
	level = DebugWarn;
	break;
    }
    if (!a->debugAt(level))
	return;
    String burstInfo;
    burstInfo.printf("(FN=%u TN=%u T2=%u T3=%u burst=%s)",
	t.fn(),t.tn(),t.fn() % 26,t.fn() % 51,ARFCN::burstType(bType));
    Debug(a,level,
	"%sProcessed RX burst %s: DemodPerformance=%s TSC=%s power=%g [%p]%s",
	a->prefix(),burstInfo.c_str(),demodPerf.c_str(),(tscDump ? "invalid" : "ok"),
	b.m_powerLevel,a,encloseDashes(tscDump,true));
}

//
// TrafficShower
//
TrafficShower::TrafficShower(bool in,unsigned int arfcn)
    : m_show(false), m_in(in), m_arfcn(arfcn), m_modulus(51)
{
    ::memset(*m_table,0,8 * 102 * sizeof(bool));
}


static void setBool(bool* table, unsigned int tableLen, const String& cond)
{
    ::memset(table,0,tableLen * sizeof(bool));
    ObjList* commaSplit = cond.split(',',false);
    for (ObjList* o = commaSplit->skipNull();o;o = o->skipNext()) {
	String* s = static_cast<String*>(o->get());
	if (s->find('-') == -1) {
	    int i = s->toInteger(-1);
	    if (i < 0 || (unsigned int)i >= tableLen) {
		Debug(DebugNote,"Invalid index %d in condition %s",i,cond.c_str());
		continue;
	    }
	    table[i] = true;
	    continue;
	}
	ObjList* min = s->split('-',false);
	if (min->count() != 2) {
	    Debug(DebugNote,"Invalid range count %d for cmd %s",min->count(),cond.c_str());
	    TelEngine::destruct(min);
	    continue;
	}
	int start = (static_cast<String*>((*min)[0]))->toInteger(-1);
	int end   = (static_cast<String*>((*min)[1]))->toInteger(-1);
	TelEngine::destruct(min);
	if (start < 0 || (unsigned int)start >= tableLen) {
	    Debug(DebugNote,"Invalid range interval start %d for cmd %s",start,cond.c_str());
	    TelEngine::destruct(min);
	    continue;
	}
	if (end < start || (unsigned int)end >= tableLen) {
	    Debug(DebugNote,"Invalid range interval start %d end %d tableLen %d for cmd %s",start,end,tableLen,cond.c_str());
	    TelEngine::destruct(min);
	    continue;
	}
	for (int i = start;i <= end;i++)
	    table[i] = true;
    }
    TelEngine::destruct(commaSplit);
}

static void setInt(int* table, unsigned int tableLen, const String& cond, int value)
{
    ObjList* commaSplit = cond.split(',',false);
    for (ObjList* o = commaSplit->skipNull();o;o = o->skipNext()) {
	String* s = static_cast<String*>(o->get());
	if (s->find('-') == -1) {
	    int i = s->toInteger(-1);
	    if (i < 0 || (unsigned int)i >= tableLen) {
		Debug(DebugNote,"Invalid index %d in condition %s",i,cond.c_str());
		continue;
	    }
	    table[i] = value;
	    continue;
	}
	ObjList* min = s->split('-',false);
	if (min->count() != 2) {
	    Debug(DebugNote,"Invalid range count %d for cmd %s",min->count(),cond.c_str());
	    TelEngine::destruct(min);
	    continue;
	}
	int start = (static_cast<String*>((*min)[0]))->toInteger(-1);
	int end   = (static_cast<String*>((*min)[1]))->toInteger(-1);
	TelEngine::destruct(min);
	if (start < 0 || (unsigned int)start >= tableLen) {
	    Debug(DebugNote,"Invalid range interval start %d for cmd %s",start,cond.c_str());
	    TelEngine::destruct(min);
	    continue;
	}
	if (end < start || (unsigned int)end >= tableLen) {
	    Debug(DebugNote,"Invalid range interval start %d end %d tableLen %d for cmd %s",start,end,tableLen,cond.c_str());
	    TelEngine::destruct(min);
	    continue;
	}
	for (int i = start;i <= end;i++)
	    table[i] = value;
    }
    TelEngine::destruct(commaSplit);
}

// Parse the list of arguments
// must be: on/off timeslot/s devider devider range
void TrafficShower::parse(const ObjList& args)
{
    ObjList* o = args.skipNull();
    if (!o)
	return;
    String* onoff = static_cast<String*>(o->get());
    m_show = onoff->toBoolean();
    if (!m_show)
	return;
    o = o->skipNext();
    if (!o) { // Show all
	::memset(*m_table,1,8 * 102 * sizeof(bool));
	return;
    }
    bool ts[8];
    setBool(ts,8,*(static_cast<String*>(o->get())));
    o = o->skipNext();
    if (!o) {
	for (unsigned int i = 0;i < 8;i++) {
	    if (!ts[i])
		continue;
	    ::memset(m_table[i],102,102 * sizeof(bool));
	}
	return;
    }
    m_modulus = (static_cast<String*>(o->get()))->toInteger(-1);
    if (m_modulus < 0 || m_modulus > 102) {
	Debug(DebugNote,"Invalid Modulus %d switching to 51",m_modulus);
	m_modulus = 51;
    }
    o = o->skipNext();

    String* r = 0;
    if (o)
	r = static_cast<String*>(o->get());
    for (unsigned int i = 0;i < 8;i++) {
	if (!ts[i])
	    continue;
	if (r)
	    setBool(m_table[i],102,*r);
	else
	    ::memset(m_table[i],102,102 * sizeof(bool));
    }
}

void TrafficShower::show(GSMBurst* burst)
{
    if (!m_show || !burst)
	return;
    int mod = burst->time().fn() % m_modulus;
    if (!m_table[burst->time().tn()][mod])
	return;
    Debug(DebugNote,"%s burst, ARFCN %d, Time (%d:%d), Modulus %d",
	  m_in ? "Received" : "Sending",m_arfcn,burst->time().fn(),
	  burst->time().tn(),mod);
}

//
// TrafficShaper
//
TrafficShaper::TrafficShaper(ARFCN* arfcn, unsigned int index)
    : m_arfcn(arfcn)
{
    for (unsigned int i = 0;i < 8;i++)
	m_table[i] = index;
    //::memset(m_table,index,8 * sizeof(int));
}

GSMTxBurst* TrafficShaper::getShaped(const GSMTime& t)
{
    if (!m_arfcn)
	return 0;
    int sh = m_table[t.tn()];
    if (sh == (int)m_arfcn->arfcn())
	return 0;
    if (sh >= 0 && sh < 4)
	return GSMTxBurst::getCopy(m_arfcn->transceiver()->arfcn(sh)->getFiller(t));
    // Negative values special case
    // -1 filler burst
    if (sh == -1) {
	GSMTxBurst* d = GSMTxBurst::buildFiller();
	if (!d)
	    return 0;
	d->buildTxData(m_arfcn->transceiver()->signalProcessing());
	return d;
    }
    // SH == -2 void air
    if (sh == -2)
	return GSMTxBurst::getVoidAirBurst(m_arfcn->transceiver()->signalProcessing());
    if (sh == -3)
	return m_arfcn->transceiver()->getTxTestBurstCopy();
    if (sh == -4) {
	uint8_t tmp[154];
	::memset(tmp,0,154);
	GSMTxBurst* d =  GSMTxBurst::parse(tmp,154,m_arfcn->m_txBurstStore);
	if (!d)
	    return 0;
	d->buildTxData(m_arfcn->transceiver()->signalProcessing());
	return d;
    }
    return 0;
}

void TrafficShaper::parse(ObjList* args)
{
    if (!m_arfcn || !args)
	return;
    ObjList* o = args->skipNull();
    if (!o)
	return;
    unsigned int shape = m_arfcn->arfcn();
    if (o->count() >= 2)
	shape = (static_cast<String*>((*o)[1]))->toInteger();
    if (shape == m_arfcn->arfcn())
	return;
    String* ts = static_cast<String*>(o->get());
    setInt(m_table,8,*ts,shape);
}

//
// TxFillerTable
//
// Initialize the filler table
void TxFillerTable::init(unsigned int len, GSMTxBurst* filler)
{
    clear();
    m_filler = filler;
    m_fillers.assign(len >= FILLER_FRAMES_MIN ? len : FILLER_FRAMES_MIN);
    for (unsigned int i = 0; i < m_fillers.length(); i++) {
	GSMTxBurstPtrVector& v = m_fillers[i];
	v.assign(8);
	v.fill(m_filler);
    }
}

void TxFillerTable::clear()
{
    for (unsigned int i = 0; i < m_fillers.length(); i++) {
	GSMTxBurstPtrVector& v = m_fillers[i];
	for (unsigned int j = 0; j < v.length(); j++)
	    if (v[j] != m_filler)
		TelEngine::destruct(v[j]);
    }
    m_fillers.clear();
    TelEngine::destruct(m_filler);
}

GSMTxBurst** TxFillerTable::fillerHolder(const GSMTime& time)
{
    unsigned int modulus = m_owner ? m_owner->getFillerModulus(time.tn()) : 102;
    if (modulus > m_fillers.length())
	modulus = m_fillers.length();
    return &(m_fillers[time.fn() % modulus][time.tn()]);
}

void TxFillerTable::store(GSMTxBurst* burst)
{
    if (m_owner)
	m_owner->m_txBurstStore.store(burst);
    else
	TelEngine::destruct(burst);
}


//
// ARFCN
//
ARFCN::ARFCN(unsigned int index)
    : m_txBurstStore(200,String("ARFCNTx:") + String(index)),
    m_mutex(false,"ARFCN"),
    m_rxQueue(24,"ARFCNRx"),
    m_radioRxStore(50,String("ARFCNRx:") + String(index)),
    m_radioInThread(0),
    m_chans(0),
    m_rxBurstsStart(0),
    m_rxBursts(0),
    m_averegeNoiseLevel(0),
    m_rxTraffic(true,index),
    m_txTraffic(false,index),
    m_arfcn(index),
    m_fillerTable(this),
    m_txMutex(false,"ARFCNTx"),
    m_shaper(this,index)
{
    for (uint8_t i = 0; i < 8; i++) {
	m_slots[i].slot = i;
	m_slots[i].type = ChanNone;
	m_slots[i].burstType = BurstNone;
    }
    ::memset(m_rxDroppedBursts,0,sizeof(m_rxDroppedBursts));
}

ARFCN::~ARFCN()
{
    stop();
}

// Dump chan type
void ARFCN::dumpChanType(String& buf, const char* sep)
{
    for (int i = 0; i < 8; i++) {
	const char* s = chanType(m_slots[i].type);
	buf.append(s ? s : String(m_slots[i].type).c_str(),sep);
    }
}

// Initialize a timeslot and related data
void ARFCN::initTimeslot(unsigned int slot, int t)
{
    if (slot > 7 || m_slots[slot].type == t)
	return;
    ArfcnSlot& s = m_slots[slot];
    s.type = t;
    Debug(this,DebugAll,"%sSlot %u type set to %d '%s' [%p]",
	prefix(),slot,t,chanType(t),this);
    setSlotBurstType(s,getChanBurstType(t));
    s.warnRecvDrop = true;
    switch (t) {
	case ChanNone:
	case ChanI:
	case ChanII:
	case ChanIII:
	case ChanFill:
	case ChanIGPRS:
	    s.modulus = 26;
	    break;
	case ChanIV:
	case ChanV:
	case ChanVI:
	    s.modulus = 51;
	    break;
	case ChanVII:
	    s.modulus = 102;
	    break;
	default:
	    s.modulus = 26;
	    Debug(this,DebugStub,"ARFCN::initTimeslot() unhandled chan type %u",t);
    }
    uint8_t n = 0;
    for (int i = 0; i < 8; i++)
	if (m_slots[i].type != ChanNone)
	    n++;
    m_chans = n;
}

// Start the ARFCN, stop it if already started
bool ARFCN::start()
{
    DBGFUNC_TRXOBJ("ARFCN::start()",this);
    stop();
    return true;
}

// Stop the ARFCN
void ARFCN::stop()
{
    DBGFUNC_TRXOBJ("ARFCN::stop()",this);
    radioPowerOff();
}

// Initialize/start radio power on related data
bool ARFCN::radioPowerOn(String* reason)
{
    m_rxQueue.clear();
    Lock lck(m_mutex);
    if (!TrxWorker::create(m_radioInThread,TrxWorker::ARFCNRx,this))
	return false;
    m_rxBursts = 0;
    ::memset(m_rxDroppedBursts,0,sizeof(m_rxDroppedBursts));
    return true;
}

// Stop radio power on related data
void ARFCN::radioPowerOff()
{
    m_mutex.lock();
    TrxWorker::cancelThreads(this,0,&m_radioInThread);
    int level = DebugInfo;
    String buf;
    if (m_rxBursts) {
	String tmp;
	uint64_t n = dumpDroppedBursts(&tmp);
	uint64_t delta = Time::now() - m_rxBurstsStart;
	String extra;
	uint64_t sec = delta / 1000000;
	double avg = sec ? (double)m_rxBursts / sec : 0;
	extra.printf(512," ellapsed=" FMT64U "sec avg=%.2f bursts/sec",
	    sec,avg);
	String passed;
	uint64_t pass = m_rxBursts - n;
	passed.printf(" passed=" FMT64U " (%.2f%%)",pass,(100 * ((double)pass / m_rxBursts)));
	String dropped;
	dropped.printf(" dropped=" FMT64U " (%.2f%%)",n,(100 * ((double)n / m_rxBursts)));
	buf << " RX: bursts=" << m_rxBursts << passed << dropped;
	if (n) {
	    level = DebugNote;
	    if (tmp)
		buf << " (" << tmp << ")";
	}
	buf << extra;
	m_rxBursts = 0;
    }
    if (buf)
	Debug(transceiver(),level,"%sRadio power off.%s [%p]",prefix(),buf.c_str(),this);
    m_mutex.unlock();
    m_rxQueue.clear();
}

// Forward a burst to upper layer
bool ARFCN::recvBurst(GSMRxBurst*& burst)
{
    if (!burst)
	return true;
    Debug(this,DebugStub,"ARFCN::recvBurst() not implemented [%p]",this);
    return false;
}

void ARFCN::addBurst(GSMTxBurst* burst)
{
    if (!burst)
	return;
    GSMTime txTime;
    transceiver()->getTxTime(txTime);
    Lock myLock(m_txMutex);

    String t;
    String tmp;
    m_txStats.burstLastInTime = burst->time();
    m_txStats.burstsDwIn++;
    m_txStats.burstsDwInSlot[burst->time().tn()]++;
    if (burst->filler() || txTime > burst->time()) {
	if (!burst->filler()) {
	    if (txTimeDebugOk(txTime))
		Debug(this,DebugNote,"%sReceived delayed burst %s at %s [%p]",
		    prefix(),burst->time().c_str(t),txTime.c_str(tmp),this);
	    m_txStats.burstsExpiredOnRecv++;
	}
	m_expired.insert(burst);
	return;
    }
    GSMTime tmpTime = txTime + 204 * 8;
    if (burst->time() > tmpTime) {
	if (txTimeDebugOk(txTime))
	    Debug(this,DebugNote,
		"%sReceived burst %s too far in future at %s by %d timeslots [%p]",
		prefix(),burst->time().c_str(t),txTime.c_str(tmp),
		burst->time().diff(txTime),this);
	m_txStats.burstsFutureOnRecv++;
	m_expired.insert(burst);
	return;
    }
    ObjList* last = &m_txQueue;
    for (ObjList* o = m_txQueue.skipNull();o;o = o->skipNext()) {
	GSMTxBurst* b = static_cast<GSMTxBurst*>(o->get());
	if (burst->time() > b->time()) {
	    XDebug(this,DebugAll,"%sInsert burst %s before %s [%p]",
		prefix(),burst->time().c_str(t),b->time().c_str(tmp),this);
	    o->insert(burst);
	    return;
	}
	if (burst->time() == b->time()) {
	    // Don't use tx silence debug status: this is a bad thing (should never happen)
	    Debug(this,DebugNote,"%sDuplicate burst received at %s [%p]",
		prefix(),burst->time().c_str(t),this);
	    m_txBurstStore.store(burst);
	    m_txStats.burstsDupOnRecv++;
	    return;
	}
	last = o;
    }
    last->append(burst);
}

static inline GSMTxBurst* checkType(ARFCN* arfcn, GSMTxBurst* burst, const GSMTime& time,
    bool isFiller)
{
    if (!burst)
	return 0;
    if (!(arfcn->arfcn() == 0 && time.tn() == 0 && burst->type() != 0))
	return burst;
    unsigned int t3 = time.fn() % 51;
    const char* chan = "unknown channel";
    switch (t3) {
	case 0:
	case 10:
	case 20:
	case 30:
	case 40:
	    if (burst->type() == 2)
		return burst;
	    chan = "FCCH";
	    break;
	case 1:
	case 11:
	case 21:
	case 31:
	case 41:
	    if (burst->type() == 1)
		return burst;
	    chan = "SCH";
	    break;
	default:
	    if (burst->type() == 0)
		return burst;
    }
    String t, tb;
    Debug(arfcn,DebugNote,
	"%sSending burst type %d on %s T3=%u at %s burst %s filler=%s [%p]",
	arfcn->prefix(),burst->type(),chan,t3,time.c_str(t),burst->time().c_str(tb),
	String::boolText(isFiller),arfcn);
    return burst;
}

// Obtain the burst that has to be sent at the specified time
GSMTxBurst* ARFCN::getBurst(const GSMTime& time, bool& owner)
{
    String t;
    GSMTxBurst* burst = m_shaper.getShaped(time);
    owner = (burst != 0);
    Lock lck(m_txMutex);
    // Move filler/expired bursts to filler table
    ObjList* found = m_expired.skipNull();
    for (; found; found = found->skipNull())
	m_fillerTable.set(static_cast<GSMTxBurst*>(found->remove(false)));
    // Check if we have a burst to send
    for (found = m_txQueue.skipNull(); found; found = found->skipNext()) {
	GSMTxBurst* b = static_cast<GSMTxBurst*>(found->get());
	if (b->time() <= time)
	    break;
    }
    // found: burst time equals requested time or is in the past
    if (found) {
	unsigned int expired = 0;
	GSMTxBurst* b = static_cast<GSMTxBurst*>(found->remove(false));
	// Same time: don't use as filler if chan type is GPRS
	// NOTE:
	//  See why we are doing it only if time matches
	//  Why don't we do the same for expired, non filler, bursts?
	//  We should do the same for all GPRS chans assuming the upper
	//   layer is using fixed allocation
	if (b->time() == time) {
	    if (!burst)
		burst = b;
	    if (m_slots[time.tn()].type != ChanIGPRS)
		m_fillerTable.set(b);
	    else if (burst == b)
		owner = true;
	    else
		m_txBurstStore.store(b);
	}
	else {
	    m_fillerTable.set(b);
	    expired++;
	}
	for (found = found->skipNull(); found; found = found->skipNull()) {
	    m_fillerTable.set(static_cast<GSMTxBurst*>(found->remove(false)));
	    expired++;
	}
	if (expired) {
	    m_txStats.burstsExpiredOnSend += expired;
	    if (txTimeDebugOk(time))
		Debug(this,DebugNote,"%s%u burst(s) expired at %s [%p]",
		    prefix(),expired,time.c_str(t),this);
	}
    }
    lck.drop();
    if (burst)
	return checkType(this,burst,time,false);
    bool sync = (arfcn() == 0) && shouldBeSync(time);
    if (!sync)
	return checkType(this,m_fillerTable.get(time),time,true);
    m_txStats.burstsMissedSyncOnSend++;
    if (debugAt(DebugNote) && txTimeDebugOk(time)) {
        uint32_t fn = time.fn();
	Debug(this,DebugNote,"%sMissing SYNC burst at %s T2=%d T3=%d [%p]",
	    prefix(),time.c_str(t),(fn % 26),(fn % 51),this);
    }
    return 0;
}

// Process received radio data
void ARFCN::recvRadioData(RadioRxData* d)
{
    if (!d)
	return;
    if (!transceiver() || transceiver()->statistics()) {
	if (!m_rxBursts)
	    m_rxBurstsStart = Time::now();
	m_rxBursts++;
    }
    if (d->m_data.length() < ARFCN_RXBURST_LEN) {
	dropRxBurst(RxDropInvalidLength,d->m_time,d->m_data.length(),DebugFail,d);
	return;
    }
    XDebug(this,DebugInfo,"%sEnqueueing Rx burst (%p) TN=%u FN=%u len=%u [%p]",
	prefix(),d,d->m_time.tn(),d->m_time.fn(),d->m_data.length(),this);
    if (m_rxQueue.add(d,transceiver()))
	return;
    if (thShouldExit(transceiver()))
	TelEngine::destruct(d);
    else
	dropRxBurst(RxDropQueueFull,d->m_time,d->m_data.length(),DebugFail,d);
}

// Worker terminated notification
void ARFCN::workerTerminated(Thread* th)
{
    if (!th)
	return;
    Lock lck(TrxWorker::s_mutex);
    if (m_radioInThread == th)
	m_radioInThread = 0;
}

// Run radio input data process loop
void ARFCN::runRadioDataProcess()
{
    if (!transceiver())
	return;
    transceiver()->waitPowerOn();
    GenObject* gen = 0;
    GSMRxBurst* burst = 0;
    while (m_rxQueue.waitPop(gen,transceiver())) {
	if (!gen)
	    continue;
	RadioRxData* d = static_cast<RadioRxData*>(gen);
	gen = 0;
	XDebug(this,DebugAll,"%sDequeued Rx burst (%p) TN=%u FN=%u len=%u [%p]",
	    prefix(),d,d->m_time.tn(),d->m_time.fn(),d->m_data.length(),this);
	if (d->m_time.tn() > 7) {
	    dropRxBurst(RxDropInvalidTimeslot,d->m_time,d->m_data.length(),DebugFail,d);
	    continue;
	}
	if (!burst)
	    burst = new GSMRxBurst;
	burst->time(d->m_time);
	burst->m_data.exchange(d->m_data);
	m_radioRxStore.store(d);
	ArfcnSlot& slot = m_slots[burst->time().tn()];
	if (!transceiver()->processRadioBurst(arfcn(),slot,*burst) ||
	    recvBurst(burst))
	    continue;
	transceiver()->fatalError();
	break;
    }
    TelEngine::destruct(gen);
    TelEngine::destruct(burst);
}

void ARFCN::dumpBursts()
{
    Lock myLock(m_txMutex);
    for (ObjList* o = m_txQueue.skipNull();o;o = o->skipNext()) {
	GSMTxBurst* b = static_cast<GSMTxBurst*>(o->get());
	Debug(this,DebugAll,"Queued burst arfcn %u fn %u tn %u [%p]",
	    m_arfcn,b->time().fn(),b->time().tn(),b);
    }
}

// Retrieve the channel type dictionary
const TokenDict* ARFCN::chanTypeName()
{
    return s_gsmSlotType;
}

// Retrieve the burst type dictionary
const TokenDict* ARFCN::burstTypeName()
{
    return s_gsmBurstType;
}

// Set timeslot access burst type
void ARFCN::setSlotBurstType(ArfcnSlot& slot, int val)
{
    if (slot.burstType == val)
	return;
    if (transceiver() && transceiver()->state() == Transceiver::PowerOn)
	Debug(this,DebugAll,"%sSlot %u burst type changed %s -> %s [%p]",
	    prefix(),slot.slot,burstType(slot.burstType),burstType(val),this);
    slot.burstType = val;
}

int ARFCN::getChanBurstType(int chanType)
{
    switch (chanType) {
	case ChanI:
	case ChanIII:
	case ChanIGPRS:
	    return BurstNormal;
	case ChanIV:
	case ChanVI:
	    return BurstAccess;
	case ChanII:
	case ChanV:
	case ChanVII:
	case ChanLoopback:
	    return BurstCheck;
	case ChanFill:
	    return BurstIdle;
	case ChanNone:
	    return BurstNone;
    }
    Debug(DebugStub,"ARFCN::getChanBurstType() unhandled chan type %u",chanType);
    return BurstUnknown;
}

int ARFCN::burstTypeCheckFN(int chanType, unsigned int fn)
{
    switch (chanType) {
	case ChanII:
	    return (fn % 2) ? BurstIdle : BurstNormal;
	case ChanV:
	    fn = fn % 51;
	    if ((14 <= fn && fn <= 36) || fn == 4 || fn == 5 || fn == 45 || fn == 46)
		return BurstAccess;
	    return BurstNormal;
	case ChanVII:
	    fn = fn % 51;
	    return (fn < 12 || fn > 14) ? BurstNormal : BurstIdle;
	case ChanLoopback:
	    fn = fn % 51;
	    return (fn < 48) ? BurstNormal : BurstIdle;
    }
    Debug(DebugStub,"ARFCN::burstTypeCheckFN() unhandled chan type %u",chanType);
    return BurstNone;
}

void ARFCN::dropRxBurst(int dropReason, const GSMTime& t, unsigned int len, int level,
    RadioRxData* d, const char* reason)
{
    bool fatal = (level == DebugFail);
    if (!fatal && transceiver() && !transceiver()->statistics()) {
	m_radioRxStore.store(d);
	return;
    }
    bool debug = fatal;
#ifdef DEBUG
    if (!debug && level >= 0) {
	switch (dropReason) {
	    case RxDropLowSNR:
	    case RxDropLowPower:
	    case RxDropLowPeakMin:
#ifdef XDEBUG
		debug = true;
#endif
		break;
	    case RxDropSlotUnk:
	    case RxDropNullRMS:
	    case RxDropNegativeMaxIndex:
	    case RxDropQueueFull:
	    case RxDropInvalidTimeslot:
	    case RxDropInvalidLength:
		debug = true;
		break;
	    case RxDropCount:
		break;
	}
    }
#endif
    if (debug && level >= 0) {
	if (!reason && dropReason != RxDropSlotUnk)
	    reason = lookup(dropReason,s_rxDropBurstReason);
	if (reason)
	    Debug(this,level,"%sDropping Rx burst TN=%u FN=%u len=%u: %s [%p]",
		prefix(),t.tn(),t.fn(),len,reason,this);
    }
    if (dropReason < RxDropCount)
	m_rxDroppedBursts[dropReason]++;
    m_radioRxStore.store(d);
    if (fatal && transceiver())
	transceiver()->fatalError();
}

uint64_t ARFCN::dumpDroppedBursts(String* buf, const char* sep, bool force,
    bool valueOnly)
{
    uint64_t n = 0;
    for (unsigned int i = 0; i < RxDropCount; i++) {
	n += m_rxDroppedBursts[i];
	if (!(buf && (force || m_rxDroppedBursts[i])))
	    continue;
	String val(m_rxDroppedBursts[i]);
	if (valueOnly)
	    buf->append(val,sep);
	else
	    buf->append(lookup(i,s_rxDropBurstReason),sep) << "=" << val;
    }
    return n;
}

void ARFCN::addAverageNoise(float current)
{
    m_averegeNoiseLevel = 0.9 * m_averegeNoiseLevel + 0.1 * current;
}

void ARFCN::dumpSlotsDelay(String& dest)
{
    dest << "ARFCN[" << m_arfcn << "][";
    for (unsigned int i = 0;i < 8;i++) {
	dest << "s:";
	dest << i;
	dest << ";ds:";
	SigProcUtils::appendFloat(dest,m_slots[i].delay,"");
	if (i != 7)
	    dest << ";";
    }
    dest << "];";
}

//
// ARFCNSocket
//
ARFCNSocket::ARFCNSocket(unsigned int index)
    : ARFCN(index), m_data("data",false,5),
    m_dataReadThread(0)
{
}

ARFCNSocket::~ARFCNSocket()
{
    stop();
}

// Start the ARFCN, stop it if already started
// Start the processing threads
bool ARFCNSocket::start()
{
    DBGFUNC_TRXOBJ("ARFCNSocket::start()",this);
    stop();
    if (!ARFCN::start())
	return false;
    Lock lck(m_mutex);
    bool ok = m_data.initSocket(*this);
    lck.drop();
    if (!ok)
	stop();
    return ok;
}

// Stop the ARFCN.
// Stop the processing threads
void ARFCNSocket::stop()
{
    DBGFUNC_TRXOBJ("ARFCNSocket::stop()",this);
    radioPowerOff();
    ARFCN::stop();
}

// Initialize/start radio power on related data
bool ARFCNSocket::radioPowerOn(String* reason)
{
    if (!ARFCN::radioPowerOn(reason))
	return false;
    Lock lck(m_mutex);
    if (!TrxWorker::create(m_dataReadThread,TrxWorker::ARFCNTx,this))
	return false;
    return true;
}

// Stop radio power on related data
void ARFCNSocket::radioPowerOff()
{
    ARFCN::radioPowerOff();
    m_mutex.lock();
    TrxWorker::cancelThreads(this,0,&m_dataReadThread);
    m_data.terminate();
    m_mutex.unlock();
}

// Forward a burst to upper layer
bool ARFCNSocket::recvBurst(GSMRxBurst*& burst)
{
    if (!burst)
	return true;
    m_lastUplinkBurstOutTime = burst->time();
    burst->fillEstimatesBuffer();
    return m_data.writeSocket(burst->m_bitEstimate,ARFCN_RXBURST_LEN,*this) >= 0;
}

// Read socket loop
void ARFCNSocket::runReadDataSocket()
{
    if (!transceiver())
	return;
    transceiver()->waitPowerOn();
    FloatVector tmpV;
    ComplexVector tmpW;
    while (!thShouldExit(transceiver())) {
	int r = m_data.readSocket(*this);
	if (r < 0)
	    return;
	if (!r)
	    continue;
	DataBlock tmp;
	GSMTxBurst* burst = GSMTxBurst::parse(m_data.m_readBuffer.data(0),r,m_txBurstStore,&tmp);
	if (!burst)
	    continue;
	// Transform (modulate + freq shift)
	burst->buildTxData(transceiver()->signalProcessing(),&tmpV,&tmpW,tmp);
	tmp.clear(false);
	m_txTraffic.show(burst);
	addBurst(burst);
    }
}

// Worker terminated notification
void ARFCNSocket::workerTerminated(Thread* th)
{
    if (!th)
	return;
    Lock lck(TrxWorker::s_mutex);
    if (m_dataReadThread == th)
	m_dataReadThread = 0;
    else {
	lck.drop();
	ARFCN::workerTerminated(th);
    }
}

// Initialize UDP socket
bool ARFCNSocket::initUDP(int rPort, const char* rAddr, int lPort,
    const char* lAddr)
{
    Lock lck(m_mutex);
    return m_data.setAddr(true,lAddr,lPort,*this) &&
	m_data.setAddr(false,rAddr,rPort,*this);
}

/* vi: set ts=8 sw=4 sts=4 noet: */
