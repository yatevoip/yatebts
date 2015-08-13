/*
* Copyright 2011 Range Networks, Inc.
* All Rights Reserved.
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

#include <list>
//#include "RList.h"
#include "LLC.h"
//#include "MSInfo.h"
#include "GPRSL3Messages.h"
#include "Ggsn.h"
#include "Sgsn.h"
#include "SgsnConn.h"
#include "Utils.h"
#include "Globals.h"
//#include "MAC.h"
#include "miniggsn.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iomanip>

using namespace Utils;
using namespace Connection;
#define CASENAME(x) case x: return #x;
#define SRB3 3

//using namespace SIP;

namespace SGSN {
typedef std::list<SgsnInfo*> SgsnInfoList_t;
static SgsnInfoList_t sSgsnInfoList;
typedef std::list<GmmInfo*> GmmInfoList_t;
static GmmInfoList_t sGmmInfoList;
static Mutex sSgsnListMutex;	// One lock sufficient for all lists maintained by SGSN.
static void dumpGmmInfo();
#if RN_UMTS
//static void sendAuthenticationRequest(SgsnInfo *si, string IMSI);
#endif

//static void killOtherTlli(SgsnInfo *si,uint32_t newTlli);
static SgsnInfo *sgsnGetSgsnInfoByHandle(uint32_t mshandle, bool create);
static int getNMO();

unsigned int sgsnDebug()
{
	if (gConfig.getBool("SGSN.Debug"))
	    return GPRS::GPRSDebug;
	return 0;
}

bool enableMultislot()
{
	return gConfig.getNum("GPRS.Multislot.Max.Downlink") > 1 ||
		gConfig.getNum("GPRS.Multislot.Max.Uplink") > 1;
}

const char *GmmCause::name(unsigned mt, bool ornull)
{
	switch (mt) {
		CASENAME(IMSI_unknown_in_HLR)
		CASENAME(Illegal_MS)
		CASENAME(IMEI_not_accepted)
		CASENAME(Illegal_ME)
		CASENAME(GPRS_services_not_allowed)
		CASENAME(GPRS_services_and_non_GPRS_services_not_allowed)
		CASENAME(MS_identity_cannot_be_derived_by_the_network)
		CASENAME(Implicitly_detached)
		CASENAME(PLMN_not_allowed)
		CASENAME(Location_Area_not_allowed)
		CASENAME(Roaming_not_allowed_in_this_location_area)
		CASENAME(GPRS_services_not_allowed_in_this_PLMN)
		CASENAME(No_Suitable_Cells_In_Location_Area)
		CASENAME(MSC_temporarily_not_reachable)
		CASENAME(Network_failure)
		CASENAME(MAC_failure)
		CASENAME(Synch_failure)
		CASENAME(Congestion)
		CASENAME(GSM_authentication_unacceptable)
		CASENAME(Not_authorized_for_this_CSG)
		CASENAME(No_PDP_context_activated)
		// 0x30 to 0x3f - retry upon entry into a new cell?
		CASENAME(Semantically_incorrect_message)
		CASENAME(Invalid_mandatory_information)
		CASENAME(Message_type_nonexistent_or_not_implemented)
		CASENAME(Message_type_not_compatible_with_the_protocol_state)
		CASENAME(Information_element_nonexistent_or_not_implemented)
		CASENAME(Conditional_IE_error)
		CASENAME(Message_not_compatible_with_the_protocol_state)
		CASENAME(Protocol_error_unspecified)
		default:
			return ornull ? 0 : "unrecognized GmmCause type";
	}
}

SgsnInfo::SgsnInfo(uint32_t wMsHandle) :
	//mState(GmmState::GmmNotOurTlli),
	mGmmp(0),
	mLlcEngine(0),
	mMsHandle(wMsHandle),
	mConnId(GprsConnNone),
	mT3310FinishAttach(15000),	// 15 seconds
	mT3370ImsiRequest(6000)		// 6 seconds
	// mSuspended(0),
{
	//memset(mOldMcc,0,sizeof(mOldMcc));
	//memset(mOldMnc,0,sizeof(mOldMnc));
	time(&mLastUseTime);
#if RN_UMTS == 0
	mLlcEngine = new LlcEngine(this);
#endif
	sSgsnInfoList.push_back(this);
	SGSNLOGF(INFO,GPRS_OK|GPRS_MSG,"SGSN","Created SgsnInfo:" << this);
}

SgsnInfo::~SgsnInfo()
{
	if (mLlcEngine) {delete mLlcEngine;}
}

void SgsnInfo::clearConn(int state, int btsPrim, unsigned char info)
{
	if (state >= 0)
		return;
	int id = getConnId();
	mConnId = state;
	if (gGprsMap.unmap(this) && (id >= 0)) {
		setConnId(state);
		gSigConn.send((BtsPrimitive)btsPrim,info,id);
	}
}

void SgsnInfo::sirm()
{
	clearConn(GprsConnNone,SigConnLost);
	std::ostringstream ss;
	sgsnInfoDump(this,ss);
	SGSNLOGF(INFO,GPRS_OK|GPRS_MSG,"SGSN","Removing SgsnInfo:"<<ss);
	sSgsnInfoList.remove(this);
	GmmInfo *gmm = getGmm();
	if (gmm && (gmm->getSI() == this)) {
		gmm->msi = 0;
		if (gmm->isRegistered())
			gmm->setGmmState(GmmState::GmmRegistrationPending);
	}
	delete this;
}

// This is for use by the Command Line Interface
// Return true on success.
bool cliSgsnInfoDelete(SgsnInfo *si)
{
	ScopedLock lock(sSgsnListMutex);
	GmmInfo *gmm = si->getGmm();
	if (gmm && gmm->getSI() == si) {
		// You cannot delete this si by itself.  Must delete the GmmInfo instead.
		return false;
	}
	si->sirm();
	return true;
}

// This is the generalized printer to identify an SgsnInfo.
// The alternate sgsnInfoDump is used only for gmmDump and prints
// only that info that is not duplicated in the Gmm.
std::ostream& operator<<(std::ostream& os, const SgsnInfo*si)
{
	MSUEAdapter *ms = si->getMS();
	if (ms)
		os << ms->msid();
#if RN_UMTS
	os << LOGHEX2("(URNTI", si->mMsHandle) << ")";
#else
	os << LOGHEX2("(TLLI", si->mMsHandle) << ")";
#endif
	if (si->getGmm()) { os << LOGVAR2("imsi",si->getGmm()->mImsi.hexstr()); }
	os << " ConnID=" << si->getConnId() << " (" << si->mConnId << ")";
	os << " [" << hex << (void*) si << "]";
	return os;
}

// Reset this connection, for example, because it is doing a GmmDetach or a new GmmAttach.
void SgsnInfo::sgsnReset(uint8_t wType)
{
	clearConn(GprsConnNone,SigGprsDetach,wType);
	freePdpAll(true);
	if (mLlcEngine) { mLlcEngine->getLlcGmm()->reset(); }
}

// The operator is allowed to choose the P-TMSI allocation strategy, subject to the constraints
// that they should not collide in the same routing area, must not be all 1s, and we dont allow all 0s either.
// Note that the MS sends RA [Routing Area] along with TMSI.
// The MS creates a local TLLI from the P-TMSI by setting the top two bits,
// so the P-TMSI is really limited to 30 bits.
// For UMTS, the URNTI consists of a 20-bit (really 16-bit, because it must fit in that) UE id
// plus a 12 bit SRNC id.
static uint32_t gPTmsiNext = 0;
static uint32_t allocatePTmsi()
{
	if (gPTmsiNext == 0) {
		// Add in the time to the starting TMSI so if the BTS is restarted there is a better chance
		// of not using the same tmsis over again.
		time_t now;
		time(&now);
		gPTmsiNext = ((now&0xff)<<12) + 1;
	}
	if (gPTmsiNext == 0 || gPTmsiNext >= (1<<30)) { gPTmsiNext = 1; }
	return gPTmsiNext++;
	//return Tlli::makeLocalTlli(gPTmsiNext++);
}

MSUEAdapter *SgsnInfo::getMS() const
{
	// The MSInfo struct disappears after a period of time, so look it up.
	//return GPRS::gL2MAC.macFindMSByTLLI(mMsHandle,0);
	return SgsnAdapter::findMs(mMsHandle);
}

GmmInfo::GmmInfo(ByteVector &imsi, uint32_t ptmsi):
	mImsi(imsi), mState(GmmState::GmmDeregistered), msi(0)
{
	memset(mPdps,0,sizeof(mPdps));
	mPTmsi = ptmsi ? ptmsi : allocatePTmsi();
	mGprsMultislotClass = -1;		// -1 means invalid.
	mAttachTime = 0;
	// Must set activityTime to prevent immediate removal from list by another phone simultaneously connection.
	setActivity();
	ScopedLock lock(sSgsnListMutex);
	sGmmInfoList.push_back(this);
}

GmmInfo::~GmmInfo()
{
	freePdpAll(true);
}

// Assumes sSgsnListMutex is locked on entry.
static void GmmRemove(GmmInfo *gmm)
{
	std::ostringstream ss;
	gmmInfoDump(gmm,ss,0);
	SGSNLOGF(INFO,GPRS_OK|GPRS_MSG,"SGSN","Removing gmm:"<<ss);
	SgsnInfo *si;
	RN_FOR_ALL(SgsnInfoList_t,sSgsnInfoList,si) {
		// The second test here should be redundant.
		if (si->getGmm() == gmm || gmm->getSI() == si) {
			si->sirm();	// yes this is suboptimal, but list is short
		}
	}
#if 0
	for (SgsnInfoList_t::iterator itr = sSgsnInfoList.begin(); itr != sSgsnInfoList.end(); ) {
		SgsnInfo *si = *itr;
		if (si->getGmm() == gmm) {
			itr = sSgsnInfoList.erase(itr);
			delete si;
		} else {
			itr++;
		}
	}
#endif
	sGmmInfoList.remove(gmm);
	delete gmm;
}


// This is for use by the Command Line Interface
void cliGmmDelete(GmmInfo *gmm)
{
	ScopedLock lock(sSgsnListMutex);
	GmmRemove(gmm);
}

PdpContext *GmmInfo::getPdp(unsigned nsapi)
{
	//return mSndcp[nsapi] ? mSndcp[nsapi]->mPdp : 0;
	assert(nsapi < sNumPdps);
	setActivity();
	return mPdps[nsapi];
}

// True if the pdpcontext is not in state PDP-INACTIVE
bool GmmInfo::isNSapiActive(unsigned nsapi)
{
	assert(nsapi < sNumPdps);
	return !(mPdps[nsapi] == 0 || mPdps[nsapi]->isPdpInactive());
}

// This status is sent back to the MS in messages to indicate what the Network thinks
// what PDPContexts are currently in use.
PdpContextStatus GmmInfo::getPdpContextStatus()
{
	PdpContextStatus result(true);
	for (int i = 0; i <= 7; i++) {
		if (isNSapiActive(i)) { result.mStatus[0] |= (1<<i); }
		if (isNSapiActive(i+8)) { result.mStatus[1] |= (1<<i); }
	}
	return result;
}

void GmmInfo::connectPdp(PdpContext *pdp, mg_con_t *mgp)
{
	// Order may be important here.
	// We dont want to hook the mgp up until the stack is all connected and prepared
	// to receive packets, because they could come blasting in any time,
	// even before any outgoing packets are sent.
	assert(pdp->mNSapi >= 0 && pdp->mNSapi < (int)sNumPdps);
	mPdps[pdp->mNSapi] = pdp;
	// getSI() should never NULL.  The mLlcEngine is null in umts.
	SgsnInfo *si = getSI();
	assert(si);
	if (si->mLlcEngine) { si->mLlcEngine->allocSndcp(si,pdp->mNSapi,pdp->mLlcSapi); }
	if (mgp)
		mg_con_open(mgp,pdp);
}

// Return TRUE if the pdp was allocated.
bool GmmInfo::freePdp(unsigned nsapi)
{
	assert(nsapi < sNumPdps);
	PdpContext *pdp = mPdps[nsapi];
	mPdps[nsapi] = 0;
	if (pdp) delete pdp;	// This disconnects the mgp also.
	// getSI() should never be NULL.  The mLlcEngine is null in umts.
#if SNDCP_IN_PDP
	// sndcp is in the PdpContext and deleted automatically.
	// Do we want to reset the LLC Sapi?  Doubt it because it is shared.
#else
	LlcEngine *llc = getSI() ? getSI()->mLlcEngine : NULL;
	if (llc) { llc->freeSndcp(nsapi); }
#endif
	return !!pdp;
}

void SgsnInfo::deactivateRabs(unsigned nsapiMask)
{
#if RN_UMTS
//	MSUEAdapter *ms = getMS();
//	if (ms) {
//		ms->msDeactivateRabs(nsapiMask);
//	} else {
//		SGSNERROR("ggsn: DeactivatePdpContextRequest: MS not found "<<this);
//	}
#endif
}

// Return a mask of RABs that were freed.
unsigned GmmInfo::freePdpAll(bool freeRabsToo)
{
	unsigned rabMask = 0;
	for (unsigned nsapi = 0; nsapi < sNumPdps; nsapi++) {
		if (freePdp(nsapi)) { rabMask |= 1<<nsapi; }
	}
	if (freeRabsToo && rabMask) {
		// It would be a serious internal error for getSI() to fail, but check anyway.
		if (getSI()) { getSI()->deactivateRabs(rabMask); }
	}
	if (rabMask) addShellRequest("PdpDeactivateAll",this);
	return rabMask;
}

void SgsnInfo::sgsnSend2PdpLowSide(int nsapi, ByteVector &packet)
{
	PdpContext *pdp = getPdp(nsapi);
	if (!pdp)
		return;
	int id = getConnId();
	if (id >= 0) {
		if (pdp->mWaitUpstream)
			return;
		unsigned char buf[4];
		buf[0] = nsapi;
		buf[1] = buf[2] = buf [3] = 0;
		gMediaConn.send(id,buf,4,packet.begin(),packet.size());
	}
	else
		pdp->pdpWriteLowSide(packet);
}

// The rbid is not used by GPRS, and is just 0.
void SgsnInfo::sgsnSend2MsHighSide(ByteVector &pdu,const char *descr, int rbid, unsigned int sapi)
{
		MSUEAdapter *ms = getMS();
#if RN_UMTS
		// TODO: It would be safer not to call getMS, but just send the dlpdu through
		// an InterthreadQueue and let the UMTS or GPRS L2 handle that part in its own thread.
		// In that case we have to add oldTlli to the message also.
//		if (!ms) {
//			SGSNWARN("no corresponding MS for URNTI " << mMsHandle);
//			return;
//		}
		// For UMTS we pass the rbid which is an intrinsic part of this channel.
		// TODO: Update UMTS to use DownlinkPdu too.
//		ms->msWriteHighSide(pdu,rbid,descr);
#else
		GmmInfo *gmm = getGmm();
		uint32_t tlli, aliasTlli = 0;
		if (gmm && gmm->isRegistered()) {
			tlli = gmm->getTlli();	// The TLLI based on the assigned P-TMSI.
		} else {
			// We send the message using the TLLI of the SgsnInfo,
			// which is the one the MS used to talk to us.
			tlli = mMsHandle;
			// If we know the P-TMSI that will be used for the local TLLI
			// for this MS after the attach procedure, notify L2.
			if (gmm) { aliasTlli = gmm->getTlli(); }
			if (aliasTlli == tlli) { aliasTlli = 0; }	// Be tidy; but dont think this can happen.
		}
		if (!ms) {
			LOG(WARNING) << "no corresponding MS for" << this;
			return;
		}
		SGSNLOGF(DEBUG,GPRS_MSG,"SGSN","Creating '" << descr << "' PDU with TLLI=" << LOGVALHEX(tlli) << "," <<LOGVALHEX(aliasTlli) << " for " << ms->msid()); 
		GprsSgsnDownlinkPdu *dlpdu = new GprsSgsnDownlinkPdu(pdu,tlli,aliasTlli,descr,sapi);
		//ms->msWriteHighSide(dlpdu);
		// This is thread safe:
		// Go ahead and enqueue it even if there is no MS
		SgsnAdapter::saWriteHighSide(dlpdu);
#endif
}

void SgsnInfo::sgsnWriteHighSideMsg(L3GprsDlMsg &msg)
{
#if RN_UMTS
		// bypass llc
//		ByteVector bv(1000);
//		bv.setAppendP(0,0);
//		msg.gWrite(bv);
//		SGSNLOG("Sending "<<msg.str() <<this);
//		sgsnSend2MsHighSide(bv,msg.mtname(),SRB3);	// TODO: Is SRB3 correct?
#else
		LlcDlFrame lframe(1000);
		lframe.setAppendP(0,0);
		msg.gWrite(lframe);
		SGSNLOGF(INFO,GPRS_OK|GPRS_MSG,"SGSN","Sending "<<msg.str() <<this<<" frame(first20)="<<lframe.head(MIN(20,lframe.size())));
		mLlcEngine->getLlcGmm()->lleWriteHighSide(lframe,msg.isSenseCmd(),msg.mtname());
#endif
}

// Incoming packets on a PdpContext come here.
void SgsnInfo::sgsnWriteHighSide(ByteVector &sdu,int nsapi)
{
#if RN_UMTS
		// The PDCP is a complete no-op.
//		sgsnSend2MsHighSide(sdu,"userdata",nsapi);
#else
		mLlcEngine->llcWriteHighSide(sdu,nsapi);
#endif
}

// TLLI 03.03 2.6, Specified in binary:
// starts with 11 - local tlli
// starts with 10 - foreign tlli
// starts with 01111 - random tlli
// starts with 01110 - auxiliary tlli.
// TLLI may not be all 1s, and if it starts with one of the above, cant be all 0s either.
//struct Tlli {
//	enum Type { Unused, LocalTlli, ForeignTlli, RandomTlli, AuxTlli, UnknownTlli };
//	static Type tlli2Type(uint32_t tlli) {
//		unsigned toptwo = tlli >> (32-2);	// It is unsigned, dont have to mask.
//		if (toptwo == 0x3) return LocalTlli;
//		if (toptwo == 0x2) return ForeignTlli;
//		unsigned topfive = tlli >> (32-5);	// It is unsigned, dont have to mask.
//		if (topfive == 0x0f) return RandomTlli;
//		if (topfive == 0x0e) return AuxTlli;
//		return UnknownTlli;
//	}
//	//static uint32_t tlli2ptmsi(uint32_t tlli) { return tlli & ~sLocalTlliMask; }
//	// Make a local TLLI
//	//static uint32_t makeLocalTlli(uint32_t tmsi) { return tmsi | sLocalTlliMask; }
//};

// Return Network Mode of Operation 1,2,3
static int getNMO()
{
	return gConfig.getNum("GPRS.NMO");
}

static void sendAttachAccept(SgsnInfo *si)
{
	si->mT3310FinishAttach.reset();
	GmmInfo *gmm = si->getGmm();
	assert(gmm);
	//L3GmmMsgAttachAccept aa(si->attachResult(),gmm->getPTmsi(),si->mAttachMobileId);
	uint32_t ptmsi = gmm->getPTmsi();
	L3GmmMsgAttachAccept aa(si->attachResult(),ptmsi);
	// We are finished with the attach procedure now.
	// Note that we are using the si (and TLLI) that the message was sent on.
	// If the BTS and the MS disagreed on the attach state at the start of this procedure,
	// we reset the MS registration to match what the MS thinks to make sure we will
	// use the old TLLI in the si, not the new one based on the PTMSI.
	si->sgsnWriteHighSideMsg(aa);
}

static void sendAttachReject(SgsnInfo *si, unsigned cause)
{
	L3GmmMsgAttachReject ar(cause);
	si->sgsnWriteHighSideMsg(ar);
}

static void handleAttachStep(SgsnInfo *si)
{
	GmmInfo *gmm = si->getGmm();
	if (!gmm) {	// This cannot happen.
		SGSNLOGF(ERR,GPRS_ERR,"SGSN","No imsi found for MS during Attach procedure"<<si);
		return;
	}
#if RN_UMTS
	assert(0);
		// Must do the Security Proecedure first, message flow like this:
		//      L3 AttachRequest
		// MS ---------------------------------> Network
		//      RRC SecurityModeCommand
		// MS <--------------------------------- Network
		//      RRC SecurityModeComplete
		// MS ---------------------------------> Network
		//     L3 AttachAccept
		// MS <--------------------------------- Network
		// (pat) Update: Havind added the authentication for NMO I in here,
		// so the above procedure is now moved to 

		// If we are in NMO 2, authentication was allegedly already done by
		// the Mobility Management protocol layer, in which case there is
		// a Kc sitting in the TMSI table.
		// We need to pass it a nul-terminated IMSI string.
//		string IMSI = gmm->mImsi.hexstr();
		//int len = gmm->mImsi.size();
		//char imsi[len+2];
		//memcpy(imsi,gmm->mImsi.hexstr().c_str(),len);
		//imsi[len] = 0;
//		LOG(INFO) << "Looking up Kc for imsi " << IMSI;
//		string Kcs = gTMSITable.getKc(IMSI.c_str());
//		if (Kcs.length() <= 1) {
//			SGSNERROR("No Kc found for MS in TMSI table during Attach procedure"<<si);
			// need to do authentication, send authentication request
                        //sendAuthenticationRequest(si);
//		}
//		sendAuthenticationRequest(si,IMSI);
#else
		// We must use the TLLI that the MS used, not the PTMSI.
		// To do that, reset the registered status.
		switch (si->mtAttachInfo.mMsgType) {
		case L3GmmMsg::AttachRequest:
			gmm->setGmmState(GmmState::GmmDeregistered);
			sendAttachAccept(si);
			break;
		case L3GmmMsg::RoutingAreaUpdateRequest:
		{
			L3GmmMsgRAUpdateAccept raa(si->mtAttachInfo.mRAUpdateType,si->getPdpContextStatus(),
					gmm->getPTmsi(),0);
			si->sgsnWriteHighSideMsg(raa);
			break;
		}
		default:
			SGSNLOGF(INFO,GPRS_ERR,"SGSN","Unmatched attach response "<<si);
			break;
		}
#endif
}

#if RN_UMTS
// Called from UMTS when it receives the SecurityModeComplete or SecurityModeFailure msg.
//void MSUEAdapter::sgsnHandleSecurityModeComplete(bool success)
//{
//	SgsnInfo *si = sgsnGetSgsnInfo();
//	// The si would only be null if the UE sent us a spurious SecurityModeComplete command.
//	if (si == NULL) {
//		SGSNERROR("Received spurious SecurityMode completion command for UE:"<<msid());
//		return;
//	}
//	if (! si->mT3310FinishAttach.active()) {
//		SGSNERROR("Received security response after T3310 expiration for UE:"<<si);
//		return;
//	}
//	if (success) {
//		sendAttachAccept(si);	// happiness
//	} else {
//		SGSNERROR("Integrity Protection failed for UE:"<<si);
//		// Oops!  We could send an attach reject, but why bother?
//		// The UE already knows it failed, no recovery is possible,
//		// and it will timeout shortly anyway.
//	}
//}
#endif

static void adjustConnectionId(SgsnInfo *si)
{
	SGSNLOGF(INFO,GPRS_OK|GPRS_MSG,"SGSN","adjusting connection" << si);
	int id = si->getConnId();
	if (id == GprsConnNone) { // GmmInfo conn ID is none
		id = si->mConnId;
		if (id == GprsConnNone) // also none in SigInfo
			return;
		si->setConnId(id);
	}
	SgsnInfo* si2 = gGprsMap.find(id);
	if (si == si2) {
		si->setConnId(id);
		return;
	}
	if (si2) {
		// we already have a different SgsnInfo mapped on the id of the GmmInfo
		// we want to replace it in the mapping
		// first make sure to unmap the current SgsnInfo connection Id
		// and release its own connection id
		SGSNLOGF(INFO,GPRS_OK|GPRS_MSG,"SGSN","replacing TLLI" << LOGHEX2("old",si2->mMsHandle) 
			<< LOGHEX2("with new",si->mMsHandle) << " in connection " << id);
		if (gGprsMap.unmap(si) && si->mConnId >= 0)
			gSigConn.send(SigConnLost,0,si->mConnId);
		// remove the ownership of the GmmInfo connection id from the old SgsnInfo
		si2->mConnId = GprsConnNone;
	}
	si->setConnId(id);
	gGprsMap.remap(si,id);
}

static unsigned int sendConnAttachReq(SgsnInfo *si, bool authenticated = false)
{
	int id = si->getConnId();
	if (id < 0) {
		id = si->mConnId;
		si->mConnId = si->getConnId();
		if (id >= 0) {
			// This can happen if we just re-identified a locally handled IMSI
			gGprsMap.unmap(si);
			gSigConn.send(SigConnLost,0,id);
		}
		return GmmCause::Congestion;
	}
	bool update = (si->mtAttachInfo.mMsgType == L3GmmMsg::RoutingAreaUpdateRequest);
	GmmInfo *gmm = si->getGmm();
	std::ostringstream buf;
	buf << "tlli=" << hex << std::setw(8) << std::setfill('0') << si->mMsHandle;
	if (gmm) {
		buf << " imsi=" << gmm->mImsi.hexstr();
		if (!gmm->mImei.empty())
			buf << " imei=" << gmm->mImei;
	}
	else {
		uint32_t ptmsi = si->mtAttachInfo.mAttachReqPTmsi;
		if (update && !ptmsi) {
			if ((si->mMsHandle & 0x80000000)) {
				ptmsi = (si->mMsHandle | 0x40000000);
				// remember this ptmsi, we don't want GmmInfo to alloc one
				// and SGSN will return IMSI
				si->mtAttachInfo.mAttachReqPTmsi = ptmsi;
			}
			else
				return GmmCause::Implicitly_detached;
		}
		buf << " ptmsi=" << hex << std::setw(8) << std::setfill('0') << ptmsi;
	}
	if (si->mAUTS.size()) {
		buf << " rand=" << si->mRAND.hexstr();
		buf << " auts=" << si->mAUTS.hexstr();
		si->mAUTS.clear();
	}
	buf << " authenticated=" << (authenticated ? "true" : "false");
	if (update)
		buf << " pdps=" << hex << std::setw(4) << std::setfill('0') << si->mtAttachInfo.mPdpContextStatus.toInt();
	return (gSigConn.send(SigGprsAttachReq,0,id,buf.str()) ? 0 : GmmCause::Congestion);
}

static void handleAuthenticationResponse(SgsnInfo *si, L3GmmMsgAuthenticationResponse &armsg) 
{
	GmmInfo *gmm = si->getGmm();
	if (!gmm) {
		SGSNLOGF(ERR,GPRS_ERR,"SGSN","No imsi found for MS during Attach procedure"<<si);
		return;
	}

	if (armsg.mSRES != si->mSRES) {
		SGSNLOGF(INFO,GPRS_OK|GPRS_MSG,"SGSN","Authentication error on" << si << LOGVAR(armsg.mSRES) << LOGVAR(si->mSRES));
		si->clearConn(GprsConnNone,SigConnLost);
		return;
	}
	if (armsg.mMobileId.isImeiSv())
		gmm->mImei = armsg.mMobileId.getAsBcd();

	switch (si->getConnId()) {
		case GprsConnNone:
			sendImplicitlyDetached(si);
			return;
		case GprsConnLocal:
			handleAttachStep(si);
			return;
	}
	sendConnAttachReq(si,true);
}

static void handleAuthenticationFailure(SgsnInfo *si, L3GmmMsgAuthenticationFailure &afmsg) 
{
	GmmInfo *gmm = si->getGmm();
	if (!gmm) {
		SGSNLOGF(ERR,GPRS_ERR,"SGSN","No imsi found for MS during Attach procedure"<<si);
		return;
	}

	if ((si->getConnId() < 0) || !afmsg.mAUTS.size() || !si->mRAND.size()) {
		sendImplicitlyDetached(si);
		return;
	}
	si->mAUTS = afmsg.mAUTS;
	sendConnAttachReq(si,false);
}

static void handleIdentityResponse(SgsnInfo *si, L3GmmMsgIdentityResponse &irmsg)
{
	if (! si->mT3310FinishAttach.active()) {
		// Well that is interesting.  We got a spurious identity response.
		SGSNLOGF(ERR,GPRS_ERR,"SGSN","unexpected message:"<<irmsg.str());
		return;
	} else {
		// The MS sent an attach request.  Try to send the response using the new IMSI.
		if (! irmsg.mMobileId.isImsi()) {
			SGSNLOGF(ERR,GPRS_ERR,"SGSN","Identity Response message does not include imsi:"<<irmsg.str());
			return;
		}
		ByteVector passbyreftmp = irmsg.mMobileId.getImsi();		// c++ foo bar
		findGmmByImsi(passbyreftmp,si);	// Always succeeds - creates if necessary, sets si->mGmmp.
		adjustConnectionId(si);

		// Use the imsi as the mobileId in the AttachAccept.
		//si->mAttachMobileId = irmsg.mMobileId;
		if (sendConnAttachReq(si))
			handleAttachStep(si);
		//si->mT3310FinishAttach.reset();
		//GmmInfo *gmm = findGmmByImsi(passbyreftmp,si);	// Always succeeds - creates if necessary.
		// TODO: Why do we send the mobileid?  It seems to Work this way, just wondering, because
		// the message is delivered to the MS based on the L2 connection as defined by si.
		//L3GmmMsgAttachAccept aa(si->attachResult(),gmm->getPTmsi(),irmsg.mMobileId);
		//si->sgsnWriteHighSideMsg(aa);
	}
}

void AttachInfo::stashMsgInfo(GMMAttach &msgIEs,
	bool isAttach, int msgType)	// true: attach request; false: RAUpdate
{
	mMsgType = msgType;
	// Save the MCC and MNC from which the MS drifted in on for reporting.
	// We only save them the first time we see them, because I am afraid
	// after that they will revert to our own MCC and MNC.
	if (! mPrevRaId.valid()) { mPrevRaId = msgIEs.mOldRaId; }

	//if (mOldMcc[0] == 0 && mOldMcc[1] == 0) {
	//	for (int i = 0; i < 3; i++) { mOldMcc[i] = DEHEXIFY(msgIEs.mOldRaId.mMCC[i]); }
	//}
	//if (mOldMnc[0] == 0 && mOldMnc[1] == 0) {
	//	for (int i = 0; i < 3; i++) { mOldMnc[i] = DEHEXIFY(msgIEs.mOldRaId.mMNC[i]); }
	//}

	// If a PTMSI was specified in the AttachRequest we need to remember it.
	if (isAttach && msgIEs.mMobileId.isTmsi()) {
		mAttachReqPTmsi = msgIEs.mMobileId.getTmsi();
	}

	if (msgIEs.mMsRadioAccessCapability.size()) {
		mMsRadioAccessCap = msgIEs.mMsRadioAccessCapability;
	}
	//mAttachMobileId = msgIEs.mMobileId;
	//if (msgIEs.mPdpContextStatus.valid())
		mPdpContextStatus = msgIEs.mPdpContextStatus;
}

void AttachInfo::copyFrom(AttachInfo &other)
{
	if (! mPrevRaId.valid()) { mPrevRaId = other.mPrevRaId; }
	if (! mAttachReqPTmsi) { mAttachReqPTmsi = other.mAttachReqPTmsi; }
	if (! mAttachReqType) { mAttachReqType = other.mAttachReqType; }
	mRAUpdateType = other.mRAUpdateType;
	mPdpContextStatus = other.mPdpContextStatus;
	if (other.mMsRadioAccessCap.size()) {
		mMsRadioAccessCap = other.mMsRadioAccessCap;
	}
	mMsgType = other.mMsgType;
}

void AttachInfo::reset()
{
	mAttachReqType = (AttachType) 0;
	mAttachReqPTmsi = 0;
	mRAUpdateType = RAUpdated;
	mMsgType = -1;
}

void sendImplicitlyDetached(SgsnInfo *si, unsigned type, unsigned cause)
{
	GmmInfo* gmm = si->getGmm();
	if (gmm && gmm->isRegistered() && (gmm->getSI() == si))
		gmm->setGmmState(GmmState::GmmRegistrationPending);
	L3GmmMsgGmmStatus statusMsg(GmmCause::Implicitly_detached);
	si->sgsnWriteHighSideMsg(statusMsg);
	// The above didn't do it, so try sending one of these too:
	// Detach type 1 means re-attach required.
	//L3GmmMsgDetachRequest dtr(1,GmmCause::Implicitly_detached);
	// 7-2012: Tried taking out the cause to stop the Multitech modem
	// sending 'invalid mandatory information'.
	// The only reason obvious to send that is in 24.008 8.5 is an unexpected IE,
	// so maybe it is the cause.  But it did not help.
	L3GmmMsgDetachRequest dtr(type,cause);
	si->sgsnWriteHighSideMsg(dtr);
}

static void sendIdentityRequest(SgsnInfo *si)
{
	// 3GPP 24.008 11.2.2 When T3370 expires we can send another Identity Request.
	// However we are also going to use it inverted, and send Identity Requests
	// no closer together than T3370.
	// If this expires, the MS will try again.
	if (si->mT3370ImsiRequest.active() && !si->mT3370ImsiRequest.expired())
		return;
	// Send off a request for the imsi.
	L3GmmMsgIdentityRequest irmsg;
	si->mT3370ImsiRequest.set();
	// We only use the timer in this case, so we only set it in this case, instead
	// of at the top of this function.
	si->mT3310FinishAttach.set();
	si->sgsnWriteHighSideMsg(irmsg);
}

// Complete the attach request locally
static void handleAttachRequestLocally(SgsnInfo *si, GmmInfo *gmm)
{
	// 7-1-2012: Working on multitech modem failure to reattach bug.
	// I tried taking this out to send an extra identity request,
	// but then the modem did not respond to that identity request,
	// just like before it did not respond to the second attach request.
	// Even after deleting all but that single SgsnInfo, and modifying the msid
	// to print both tllis, and looking at pat.log the message is definitely
	// sent on the correct TLLi.
	// But if you tell the modem to detach and then try attach again,
	// then the modem uses a new TLLI and sends an IMSI, so it thinks
	// it was attached, but it is sending an attach request anyway, with a PTMSI.
	// But the first attach used a PTMSI, and it succeeded.
	// Things to try:  send protocol incompabible blah blah.
	// Try converting to local tlli (0xc...); I tried that before but maybe
	// the tlli change procedure was wrong back then.
	// Send a detach, although I think the modem ignores this.
	if (!gmm) {
		// We need need to ask for an imsi.
		// There is a slight problem that we only have 6 seconds to register the MS,
		// which may not be enough time to do the IdentityResponse Challenge.
		// Therefore we save the IMSI associated with the TLLI that we got from the Identity response
		// challenge in the SgsnInfo, and when the MS tries again with the same TLLI,
		// we can skip the IdentityRequest phase.
		sendIdentityRequest(si);
		return;
	}
#if 0
	// We dont care if the MS already had a P-TMSI.
	// If it is doing an attach, go ahead and assign a new one.
	if (!si->mAllocatedTmsiTlli) {
		si->mAllocatedTmsiTlli = Sgsn::allocateTlli();
	}
	// We cant set the tlli in the MS until it has received the new tlli,
	// because we have to use the previous tlli to talk to it.
#endif
	// This was for testing:
	//L3GmmMsgIdentityRequest irmsg;
	//si->sgsnWriteHighSideMsg(irmsg);

	// We are assigning this ptmsi to the MS.
	handleAttachStep(si);
	//si->mT3310FinishAttach.reset();
	//L3GmmMsgAttachAccept aa(si->attachResult(),gmm->getPTmsi(),armsg.mMobileId);
	//si->sgsnWriteHighSideMsg(aa);
}

// The ms may send a P-TMSI or IMSI in the mobile id.
static void handleAttachRequest(SgsnInfo *si, L3GmmMsgAttachRequest &armsg)
{
	switch ((AttachType) (unsigned) armsg.mAttachType) {
	case AttachTypeGprsWhileImsiAttached:
		SGSNLOGF(INFO,GPRS_MSG,"SGSN","NOTICE attach type "<<(int)armsg.mAttachType <<si);
		// Fall through
	case AttachTypeGprs:
		si->mtAttachInfo.mAttachReqType = AttachTypeGprs;
		break;
	case AttachTypeCombined:
		if (getNMO() != 1) {
			// The MS should not have done this.
			LOG(ERR)<<"Combined Attach attempt incompatible with NMO 1 "<<si;
		} else {
			SGSNLOGF(INFO,GPRS_OK,"SGSN","NOTICE attach type "<<(int)armsg.mAttachType <<si);
		}
		si->mtAttachInfo.mAttachReqType = AttachTypeCombined;
		break;
	}
	//uint32_t newptmsi;

	// Save info from the message:
	si->mtAttachInfo.stashMsgInfo(armsg,true,armsg.MTI());

	// Re-init the state machine.
	// If the MS does a re-attach, we may have an existing SgsnInfo from earlier, so we must reset it now:
	// si->sgsnReset(); // 6-3-2012: changed to just freePdpAll.
	si->freePdpAll(true);

	GmmInfo *gmm = si->getGmm();
	if (gmm && armsg.mMobileId.isTmsi())
		gmm->setPTmsi(armsg.mMobileId.getTmsi());
	else if (!gmm && armsg.mMobileId.isImsi()) {
		// The MS included the IMSI in the attach request
		ByteVector imsi = armsg.mMobileId.getImsi();
		gmm = findGmmByImsi(imsi,si);	// Create the gmm and associate with si.
	}

	SgsnInfo *si2 = 0;
	if (si->mConnId >= 0) {
		si2 = gGprsMap.find(si->mConnId);
		if (si2 != si) {
			SGSNLOGF(INFO,GPRS_CHECK_FAIL,"SGSN","Reset connection of unmapped" << si);
			si->mConnId = GprsConnNone;
			if ((si->getConnId() >= 0) && !si2)
				si->setConnId(GprsConnNone);	// Clear connection in GmmInfo
		}
	}
	switch (si->getConnId()) {
		case GprsConnNone:
		{
			// Allocate a connection if this is the first time
			if (si2 && (si2->mConnId >= 0)) {
				SGSNLOGF(INFO,GPRS_CHECK_FAIL,"SGSN","replacing TLLI" << LOGHEX2("old",si2->mMsHandle) << LOGHEX2("with new",si->mMsHandle) << " in connection " << si2->mConnId);
				gGprsMap.remap(si,si2->mConnId);
				si->setConnId(si2->mConnId);
			}
			else
				si->setConnId(gGprsMap.map(si));
			GmmCause::Cause cause = GmmCause::Congestion;
			if (si->getConnId() >= 0) {
				// Make sure current TLLI is the primary in MS info
				MSUEAdapter *ms = si->getMS();
				if (ms)
					ms->msChangeTlli(si->mMsHandle);
				if (gmm)
					gmm->msi = si;
				SGSNLOGF(INFO,GPRS_CHECK_OK,"SGSN","Connection allocated to" << si);
				if (!(cause = (GmmCause::Cause)sendConnAttachReq(si)))
					return;
			}
			sendAttachReject(si,cause);
			si->sirm();
			return;
		}
		case GprsConnLocal:
			break;
		default:
			// Already sent upstream a SigGprsAttachReq and we have an ID
			sendConnAttachReq(si);
			return;
	}

	handleAttachRequestLocally(si,gmm);
}


static void handleAttachComplete(SgsnInfo *si, L3GmmUlMsg &msg)
{
	// The ms is acknowledging receipt of the new tlli.
	GmmInfo *gmm = si->getGmm();
	if (!gmm) {
		// The attach complete does not match this ms state.
		// Happens, for example, when you first turn on the bts and the ms
		// is still trying to complete a previous attach.  Ignore it.
		// The MS will timeout and try to attach again.
		SGSNLOGF(INFO,GPRS_CHECK_FAIL|GPRS_ERR,"SGSN","Ignoring spurious " << msg.mtname() << " " << si);
		// Dont send a reject because we did not reject anything.
		return;
	}
	//SGSNLOG("attach complete gmm="<<((uint32_t)gmm));
	gmm->setGmmState(GmmState::GmmRegisteredNormal);
	gmm->setAttachTime();
	gmm->msi = si;
#if RN_UMTS
#else
	// Start using the tlli associated with this imsi/ptmsi when we talk to the ms.
	SgsnInfo* othersi = si->changeTlli(true);
#endif
	if (othersi->getConnId() >= 0) {
		adjustConnectionId(othersi);
		MSUEAdapter *ms = othersi->getMS();
		if (ms)
			ms->msChangeTlli(othersi->mMsHandle);
		gSigConn.send(SigGprsAttachOk,1,othersi->getConnId());
	}
	if (msg.MTI() == L3GmmMsg::AttachComplete)
		addShellRequest("GprsAttach",gmm);
	si->mtAttachInfo.reset();

#if 0 // nope, we are going to pass the TLLI down with each message and let GPRS deal with it.
	//if (! Sgsn::isUmts()) {
	//	// Update the TLLI in all the known MS structures.
	//	// Only the SGSN knows that the MSInfo with these various TLLIs
	//	// are in fact the same MS.  But GPRS needs to know because
	//	// the MS will continue to use the old TLLIs, and it will botch
	//	// up if, for example, it is in the middle of a procedure on one TLLI
	//	// and the MS is using another TLLI, which is easy to happen given the
	//	// extremely long lag times in message flight.
	//	// The BSSG spec assumes there only two TLLIs, but I have seen
	//	// the Blackberry use three simultaneously.
	//	SgsnInfo *sip;
	//	uint32_t newTlli = gmm->getTlli();
	//	RN_FOR_ALL(SgsnInfoList_t,sSgsnInfoList,sip) {
	//		if (sip->getGmm == gmm) {
	//			UEAdapter *ms = sip->getMS();
	//			// or should we set the ptmsi??
	//			if (ms) ms->changeTlli(newTlli);
	//		}
	//	}
	//}
#endif
}

static void handleDetachRequest(SgsnInfo *si, L3GmmMsgDetachRequest &drmsg)
{
	L3GmmMsgDetachAccept detachAccept(0);
	GmmInfo *gmm = si->getGmm();
	if (!gmm) {
		// Hmm, but fall through, because it is certainly detached.
	} else {
		gmm->setGmmState(GmmState::GmmDeregistered);
	}
	si->sgsnWriteHighSideMsg(detachAccept);
	si->sgsnReset(drmsg.mDetachType);
	if (gmm) addShellRequest("GprsDetach",gmm);
}

static void sendRAUpdateReject(SgsnInfo *si, unsigned cause)
{
	L3GmmMsgRAUpdateReject raur(cause);
	si->sgsnWriteHighSideMsg(raur);
}

// TODO:  Need to follow 4.7.13 of 24.008
static void handleServiceRequest(SgsnInfo *si, L3GmmMsgServiceRequest &srmsg)
{
        GmmInfo *gmm = si->getGmm();
	// TODO:  Should we check the PTmsi and the PDP context status??? 
        if (!gmm) {
	        L3GmmMsgServiceReject sr(GmmCause::Implicitly_detached);
        	si->sgsnWriteHighSideMsg(sr);
                return;
        } else {
                gmm->setActivity();
                L3GmmMsgServiceAccept sa(si->getPdpContextStatus());
                si->sgsnWriteHighSideMsg(sa);
        }
} 

// 24.008 4.7.5, and I quote:
// "The routing area updating procedure is always initiated by the MS.
// 	It is only invoked in state GMM-REGISTERED."
// The MS may send an mMobileId containing a P-TMSI, and it sends TmsiStatus
// telling if it has a valid TMSI.
static void handleRAUpdateReqLocally(SgsnInfo *si, GmmInfo *gmm)
{
	if (!gmm) {
		sendRAUpdateReject(si,GmmCause::Implicitly_detached);
		return;
	}
	L3GmmMsgRAUpdateAccept raa(si->mtAttachInfo.mRAUpdateType,si->getPdpContextStatus(),
					gmm->getPTmsi(),0);
	si->sgsnWriteHighSideMsg(raa);
}

static void handleRAUpdateRequest(SgsnInfo *si, L3GmmMsgRAUpdateRequest &raumsg)
{
	SGSNLOGF(INFO,GPRS_CHECK_FAIL,"SGSN","Received RA Update Req on " << si);
	switch (raumsg.mUpdateType) {
	    case RAUpdated:
	    case PeriodicUpdating:
		    raumsg.mUpdateType = RAUpdated;
		    break;
		case CombinedRALAUpdated:
		case CombinedRALAWithImsiAttach:
			LOG(WARNING) << "Combined RA/LA update requested is not supported," 
				" overwriting it to RA update" << si;
			raumsg.mUpdateType = RAUpdated;
			break;
	}
	si->mtAttachInfo.mRAUpdateType = (RAUpdateType)(unsigned)raumsg.mUpdateType;
	si->mtAttachInfo.stashMsgInfo(raumsg,false,raumsg.MTI());

	GmmInfo *gmm = si->getGmm();
	if (gmm)
		// make sure to use during RAUpdate procudere the TLLI with which the MS came
		gmm->setGmmState(GmmState::GmmRegistrationPending);

        SgsnInfo *si2 = 0;
	if (si->mConnId >= 0) {
		si2 = gGprsMap.find(si->mConnId);
		if (si2 != si) {
			SGSNLOGF(INFO,GPRS_CHECK_FAIL,"SGSN","Reset connection of unmapped" << si);
			si->mConnId = GprsConnNone;
			if ((si->getConnId() >= 0) && !si2)
				si->setConnId(GprsConnNone);	// Clear connection in GmmInfo
		}
	}
	switch (si->getConnId()) {
		case GprsConnNone:
		{
			    // Allocate a connection if this is the first time
			if (si2 && (si2->mConnId >= 0)) {
				SGSNLOGF(INFO,GPRS_CHECK_FAIL,"SGSN","replacing TLLI" << LOGHEX2("old",si2->mMsHandle) << LOGHEX2("with new",si->mMsHandle) << " in connection " << si2->mConnId);
				gGprsMap.remap(si,si2->mConnId);
				si->setConnId(si2->mConnId);
			}
			else
				si->setConnId(gGprsMap.map(si));
			GmmCause::Cause cause = GmmCause::Congestion;
			if (si->getConnId() >= 0) {
				// Make sure current TLLI is the primary in MS info
				MSUEAdapter *ms = si->getMS();
				if (ms)
					ms->msChangeTlli(si->mMsHandle);
				if (gmm)
					gmm->msi = si;
				SGSNLOGF(INFO,GPRS_CHECK_OK,"SGSN","Connection allocated to" << si);
				if (!(cause = (GmmCause::Cause)sendConnAttachReq(si)))
					return;
			}
			sendRAUpdateReject(si,cause);
			si->sirm();
			return;
		}
		case GprsConnLocal:
			break;
		default:
		{
			// we might have a SgsnInfo with ConnId=None and GmmInfo with valid connID from a previous attach request
			// make sure to reset the mapping
			adjustConnectionId(si);
			GmmCause::Cause cause = GmmCause::Congestion;
			if (!(cause = (GmmCause::Cause)sendConnAttachReq(si)))
				return;
			sendRAUpdateReject(si,cause);
			si->sirm();
			return;
		}
	}
	// otherwise - handle locally
	handleRAUpdateReqLocally(si,gmm);
}

static inline void handleRAUpdateComplete(SgsnInfo *si, L3GmmMsgRAUpdateComplete &racmsg)
{
	handleAttachComplete(si,racmsg);
}

// This message may arrive on a DCCH channel via the GSM RR stack, rather than a GPRS message,
// and as such, could be running in a separate thread.
// We queue the message for processing.
// The suspension may be user initiated or by the MS doing some RR messages,
// most often, Location Area Update.  The spec says we are supposed to freeze
// the LLC state and continue after resume.  But in the permanent case any incoming packets
// will be hopelessly stale after resumption, so we just toss them.  Note that web sites chatter
// incessantly with keepalives even when they look quiescent to the user, and we dont want
// all that crap to back up in the downlink queue.
// In the temporary case, which is only a second or two, we will attempt to preserve the packets
// to prevent a temporary loss of service.  I have observed that the MS first stops responding
// to the BSS for about a second before sending the RACH to initiate the RR procedure,
// so there is no warning at all.  However, we MUST cancel the TBFs.  If we dont, and after
// finishing the RR procedure the MS gets back to GPRS before the previous TBFs timeout,
// it assumes they are new TBFs, which creates havoc, because the acknacks do not correspond
// to the previous TBF.  This generates the "STUCK" condition, up to a 10 second loss of service,
// and I even saw the Blackberry detach and reattach to recover.
// In either case the MS signals resumption by sending us anything on the uplink.
// WARNING: This runs in a different thread.
bool Sgsn::handleGprsSuspensionRequest(uint32_t wTlli,
	const ByteVector &wraid)	// The Routing Area id.
{
	SGSNLOGF(INFO,GPRS_MSG,"SGSN","Received GPRS SuspensionRequest for"<<LOGHEX2("tlli",wTlli));
	MSUEAdapter* ms = SgsnAdapter::findMs(wTlli);
	if (!ms) {
		GLOG(NOTICE) << "Received GPRS suspension request for unknown TLLI=" << LOGVALHEX(wTlli);
		return false;
	}
	GmmState::state tlliState = ms->suspend();
	return tlliState == GmmState::GmmRegisteredSuspended;
	// TODO:
	// if sgsn not enabled, return false.
	// save the channel?
	// Send the resumption ie in the RR channel release afterward.
}

// WARNING: This runs in a different thread.
void Sgsn::notifyGsmActivity(const char *imsi)
{
}

// WARNING: This runs in a different thread
void Sgsn::sendPaging(const char* imsi, uint32_t tmsi, GSM::ChannelType chanType)
{
	if (!imsi)
		return;
	ByteVector imsiBv;
	if (!imsiBv.fromBcd(imsi)) {
		GPRSLOG(NOTICE,GPRS_MSG) << "Failed to unhexify received IMSI=" << imsi;
		return;
	}
	GmmInfo* gmm = findGmmByImsi(imsiBv,0);
	if (!(gmm && gmm->getGmmState() == GmmState::GmmRegisteredNormal))
 		return;
	MSUEAdapter* ms = SgsnAdapter::findMs(gmm->getTlli());
	if (!ms)
		return;
	ms->page(imsiBv,tmsi,chanType);
}

// Utility function to reject a PDP context activation
static void sendPdpContextReject(SgsnInfo *si,SmCause::Cause cause,unsigned ti)
{
	L3SmMsgActivatePdpContextReject pdpRej(ti,cause);
	si->sgsnWriteHighSideMsg(pdpRej);
}

// Utility function to extract prefixed string
static std::string getPrefixed(const char* tag, const char* buf)
{
	std::string str;
	if (tag && buf) {
		const char* start = ::strstr(buf,tag);
		if (start) {
			start += ::strlen(tag);
			const char* end = ::strchr(start,' ');
			if (end)
				str.assign(start,end - start);
			else
				str.assign(start);
		}
	}
	return str;
}

// Utility function to convert a string to integer
static long toInteger(const char* str, long defVal = -1)
{
	if (!str || !*str)
		return defVal;
	char* eptr = 0;
	long val = ::strtol(str,&eptr,0);
	return (!eptr || *eptr) ? defVal : val;
}

inline static long toInteger(const std::string& str, long defVal = -1)
{
	return toInteger(str.c_str(),defVal);
}

// Utility function to convert a string to boolean
static bool toBoolean(const char* str, bool defVal = false)
{
	if (!str || !*str)
		return defVal;
	if (!::strcmp(str,"true"))
		return true;
	if (!::strcmp(str,"false"))
		return false;
	return defVal;
}

inline static bool toBoolean(const std::string& str, bool defVal = false)
{
	return toBoolean(str.c_str(),defVal);
}

// Utility function to pick IMSI and P-TMSI and attach a new GmmInfo if needed
void setMsi(SgsnInfo* si, const char* text)
{
	GmmInfo *gmm = si->getGmm();
	if (!gmm) {
		ByteVector imsi;
		imsi.fromBcd(getPrefixed("imsi=",text));
		if (imsi.size()) {
			gmm = findGmmByImsi(imsi,si,si->mtAttachInfo.mAttachReqPTmsi);
			adjustConnectionId(si);
			gmm->setGmmState(GmmState::GmmRegistrationPending);
		}
	}
	if (gmm) {
		std::string str = getPrefixed("ptmsi=",text);
		if (!str.empty()) {
			long int ptmsi = ::strtol(str.c_str(),0,16);
			if ((ptmsi > 0) && (ptmsi != gmm->getPTmsi())) {
				gmm->setPTmsi(ptmsi);
				adjustConnectionId(si);
				findSgsnInfoByHandle(ptmsi,true)->setGmm(gmm);
			}
		}
	}
}

// Application needs IMSI in attach request
void SgsnConn::identityReq(SgsnInfo* si)
{
	sendIdentityRequest(si);
}

// Application requests authentication
// Provides RAND, SRES, possibly AUTN, Kc or Ik+Ck
void SgsnConn::authRequest(SgsnInfo* si, const char* text)
{
	setMsi(si,text);
	if (si->getConnId() < 0) {
		// This can happen if we just re-identified a detached or locally handled IMSI
		int id = si->mConnId;
		si->mConnId = si->getConnId();
		if (id >= 0) {
			gGprsMap.unmap(si);
			gSigConn.send(SigConnLost,0,id);
		}
		if (si->getConnId() == GprsConnLocal)
			handleAttachRequestLocally(si,si->getGmm());
		return;
	}
	si->mRAND.fromHexa(getPrefixed("rand=",text));
	if (!si->mRAND.size()) {
		SGSNLOGF(ERR,GPRS_ERR,"SGSN","Missing RAND for" << si);
		return;
	}
	si->mSRES.fromHexa(getPrefixed("sres=",text));
	si->mKc.fromHexa(getPrefixed("kc=",text));
	si->mCk.fromHexa(getPrefixed("ck=",text));
	si->mIk.fromHexa(getPrefixed("ik=",text));
	GmmInfo *gmm = si->getGmm();
	L3GmmMsgAuthentication amsg(si->mRAND,(!gmm || gmm->mImei.empty()));
	amsg.mAutn.fromHexa(getPrefixed("autn=",text));
	si->sgsnWriteHighSideMsg(amsg);
}

// Application requests everything else to continue locally
// Connection is already cleared
void SgsnConn::attachLBO(SgsnInfo* si)
{
	si->setConnId(GprsConnLocal);
	handleAttachRequestLocally(si,si->getGmm());
}

// Application rejected the attach
// Connection is already cleared
void SgsnConn::attachRej(SgsnInfo* si, unsigned char cause)
{
	si->setConnId(GprsConnNone);
	GmmInfo *gmm = si->getGmm();
	if (gmm)
		gmm->setGmmState(GmmState::GmmDeregistered);
	switch (si->mtAttachInfo.mMsgType) {
		case L3GmmMsg::AttachRequest:
			sendAttachReject(si,cause);
			break;
		case L3GmmMsg::RoutingAreaUpdateRequest:
			sendRAUpdateReject(si,cause);
			break;
		default:
			SGSNLOGF(WARNING,GPRS_ERR,"SGSN","Received unmatched Attach/RAUpdate response " <<si);
			break;
	}
}

// Application accepted to handle the attach
void SgsnConn::attachAccept(SgsnInfo* si, const char* text)
{
	setMsi(si,text);
	// update PDP Context Status
	std::string str = getPrefixed("pdps=",text);
	if (!str.empty()) {
		long int pdps = ::strtol(str.c_str(),0,16);
		if ((pdps >= 0) && (pdps < 0xffff))
			si->mtAttachInfo.mPdpContextStatus.fromInt(pdps);
		GmmInfo* gmm = si->getGmm();
		if (gmm) {
			std::string llcsapis = getPrefixed("llcsapis=",text);
			std::string tids = getPrefixed("tids=",text);
			if (pdps && !(llcsapis.length() && tids.length())) {
				SGSNLOGF(WARNING,GPRS_ERR,"SGSN","Missing LLC SAPIs/TIDs information for PDP contexts, " <<
					"removing all active ones");
				pdps = 0;
			}
			for (uint16_t i = 0; i < 16; i++) {
				bool active = !!((pdps >> i) & 0x01);
				uint8_t llcsapi = 0xff;
				uint8_t tid = 0xff;
				if (active) {
					unsigned int pos = llcsapis.find(',');
					if (pos == std::string::npos) {
						SGSNLOGF(WARNING,GPRS_ERR,"SGSN","Missing LLC SAPI information for NSAPI " <<
							i << ", deactivating context");
						active = false;
					}
					else {
						llcsapi = ::strtoul(llcsapis.substr(0,pos).c_str(),0,10);
						llcsapis = llcsapis.substr(pos + 1);
					}
					pos = tids.find(',');
					if (pos == std::string::npos) {
						SGSNLOGF(WARNING,GPRS_ERR,"SGSN","Missing TID information for NSAPI " <<
							i << ", deactivating context");
						active = false;
					}
					else {
						tid = ::strtoul(tids.substr(0,pos).c_str(),0,10);
						tids = tids.substr(pos + 1);
					}
				}
				if (!active) {
					if (!gmm->isNSapiActive(i))
						continue;
					SGSNLOGF(INFO,GPRS_CHECK_OK,"SGSN","Deactivating PDP context for NSAPI=" << i);
					gmm->freePdp(i);
					continue;
				}
				// active
				PdpContext *pdp = gmm->getPdp(i);
				L3GprsDlMsg::MsgSense sense = (tid & 0x80) ? L3GprsDlMsg::senseReply : L3GprsDlMsg::senseCmd;
				tid &= 0x7f;
				if (pdp && !(pdp->mLlcSapi == llcsapi && pdp->mSense == sense && pdp->mTransactionId == tid)) {
					SGSNLOGF(INFO,GPRS_CHECK_FAIL,"SGSN","PDP Context parameters don't match, replacing " <<
						" PDP context for NSAPI=" << i);
					gmm->freePdp(i);
					pdp = 0;
				}
				if (!pdp) {
					pdp = new PdpContext(gmm,0,i,llcsapi);
					gmm->connectPdp(pdp,0);
				}
				pdp->mTransactionId = tid;
				pdp->setSense(sense);
				pdp->mWaitUpstream = false;
			}
		}
	}
	handleAttachStep(si);
}

// Application requests network initiated GPRS detach
// Connection is already cleared
void SgsnConn::detach(SgsnInfo* si, unsigned char cause, const char* text)
{
	switch (si->mtAttachInfo.mMsgType) {
		case L3GmmMsg::AttachRequest:
		case L3GmmMsg::RoutingAreaUpdateRequest:
			attachRej(si,cause);
			return;
		default:
			break;
	}
	si->setConnId(GprsConnNone);
	GmmInfo *gmm = si->getGmm();
	if (gmm)
	    SGSNLOGF(WARNING,GPRS_ERR,"SGSN"," gmm state = " << GmmState::GmmState2Name(gmm->getGmmState()));
	if (gmm)
		gmm->setGmmState(GmmState::GmmDeregistered);
	sendImplicitlyDetached(si,toInteger(getPrefixed("type=",text),1),
		toInteger(getPrefixed("error=",text),cause));
	si->sgsnReset();
}

// PDP activation response or Network requested PDP activation
void SgsnConn::pdpActivate(SgsnInfo* si, bool reply, const char* text)
{
	if (reply) {
		int nsapi = toInteger(getPrefixed("nsapi=",text),-1);
		if (nsapi < 5 || nsapi > 15) {
			SGSNLOGF(WARNING,GPRS_ERR,"SGSN","Invalid PDP Activate Reply NS SAPI:" << nsapi);
			return;
		}
		PdpContext *pdp = si->getPdp(nsapi);
		if (!pdp) {
			SGSNLOGF(WARNING,GPRS_ERR,"SGSN","PDP Activate Reply for missing NS SAPI:" << nsapi);
			return;
		}
		int err = toInteger(getPrefixed("error=",text),-1);
		if ((err < 0) && toBoolean(getPrefixed("error=",text)))
			err = SmCause::Protocol_error_unspecified;
		if (err >= 0 && err <= 127) {
			unsigned ti = pdp->mTransactionId;
			si->freePdp(nsapi);
			sendPdpContextReject(si,(SmCauseType)err,ti);
			return;
		}
		pdp->mWaitUpstream = false;
		L3SmMsgActivatePdpContextAccept pdpa(pdp->mTransactionId);
		pdpa.mLlcSapi = pdp->mLlcSapi;
		pdpa.mQoS.fromHexa(getPrefixed("qos=",text));
		if (pdpa.mQoS.size() < 3) {
			SmQoS resultQoS(12);
			resultQoS.defaultPS(pdp->mRabStatus.mRateDownlink,pdp->mRabStatus.mRateUplink);
			pdpa.mQoS = resultQoS;
		}
		pdpa.mRadioPriority = toInteger(getPrefixed("priority=",text),2);
		pdpa.mPco.fromHexa(getPrefixed("pco=",text));
		pdpa.mPdpAddress.fromHexa(getPrefixed("pdpaddr=",text));
		si->sgsnWriteHighSideMsg(pdpa);
	}
	else {
		// TODO
		SGSNLOGF(INFO,GPRS_ERR,"SGSN","Network requested PDP activation not implemented");
	}
}

// PDP modification response or Network requested PDP modification
void SgsnConn::pdpModify(SgsnInfo* si, bool reply, const char* text)
{
	// TODO
	SGSNLOGF(INFO,GPRS_ERR,"SGSN","PDP modification not implemented");
}

// Network requested PDP deactivation
void SgsnConn::pdpDeactivate(SgsnInfo* si, const char* text)
{
	int nsapi = toInteger(getPrefixed("nsapi=",text),-1);
	if (nsapi < 5 || nsapi > 15) {
		SGSNLOGF(WARNING,GPRS_ERR,"SGSN","Invalid PDP Deactivate NS SAPI:" << nsapi);
		return;
	}
	PdpContext *pdp = si->getPdp(nsapi);
	if (!pdp)
		return;
	pdp->mWaitUpstream = true;
	int err = toInteger(getPrefixed("error=",text),-1);
	if (err < 0)
		err = SmCause::Protocol_error_unspecified;
	unsigned ti = pdp->mTransactionId;
	L3GprsDlMsg::MsgSense sense = pdp->mSense;
	si->freePdp(nsapi);
	L3SmMsgDeactivatePdpContextRequest deact(ti,(SmCauseType)err,
		toBoolean(getPrefixed("teardown=",text)),sense);
	deact.mPco.fromHexa(getPrefixed("pco=",text));
	si->sgsnWriteHighSideMsg(deact);
}

// Data was received for the user
void SgsnConn::userData(SgsnInfo* si, const unsigned char* data, size_t len)
{
	if (len <= 4)
		return;
	uint8_t nsapi = data[0] & 0x7f;
	PdpContext *pdp = si->getPdp(nsapi);
	if (!pdp)
		return;
	ByteVectorTemp sdu((unsigned char*)data + 4,len - 4);
	si->sgsnWriteHighSide(sdu,nsapi);
}

static void handleActivatePdpContextRequest(SgsnInfo *si, L3SmMsgActivatePdpContextRequest &pdpr)
{
	// Validate request first
	if (!LlcEngine::isValidDataSapi(pdpr.mLlcSapi)) {
		SGSNLOGF(WARNING,GPRS_ERR,"SGSN","Invalid PDP Activate LLC SAPI:" << pdpr.mLlcSapi);
		sendPdpContextReject(si,SmCause::Invalid_mandatory_information,pdpr.mTransactionId);
		return;
	}
	if ((pdpr.mNSapi < 5) || (pdpr.mNSapi > 15)) {
		SGSNLOGF(WARNING,GPRS_ERR,"SGSN","Invalid PDP Activate NS SAPI:" << pdpr.mNSapi);
		sendPdpContextReject(si,SmCause::Invalid_mandatory_information,pdpr.mTransactionId);
		return;
	}
	if (!si->isRegistered()) {
		sendPdpContextReject(si,SmCause::Message_not_compatible_with_the_protocol_state,pdpr.mTransactionId);
		sendImplicitlyDetached(si);
		si->sgsnReset();
		return;
	}
	// Message is valid and the MS is GMM registered
	GmmInfo *gmm = si->getGmm();
	PdpContext *pdp = gmm->getPdp(pdpr.mNSapi);
	if (pdp) {
		if (pdp->mTransactionId != pdpr.mTransactionId) {
			SGSNLOGF(WARNING,GPRS_ERR,"SGSN","Already used NS SAPI:" << pdpr.mNSapi);
			sendPdpContextReject(si,SmCause::NSAPI_already_used,pdpr.mTransactionId);
			return;
		}
		SGSNLOGF(INFO,GPRS_ERR,"SGSN","Duplicate PDP activate for NS SAPI:" << pdpr.mNSapi);
	}
	else {
		pdp = new PdpContext(gmm,0,pdpr.mNSapi,pdpr.mLlcSapi);
		gmm->connectPdp(pdp,0);
	}
	pdp->update(pdpr);

	std::ostringstream buf;
	buf << "nsapi=" << pdpr.mNSapi;
	buf << " llcsapi=" << pdpr.mLlcSapi;
	buf << " tid=" << ((pdp->mSense == L3GprsDlMsg::senseReply ? 0x80 : 0x00) | (uint8_t)pdp->mTransactionId);
	buf << " qos=" << pdpr.mQoS.hexstr();
	buf << " pdpaddr=" << pdpr.mPdpAddress.hexstr();
	buf << " type=" << pdpr.mRequestType;
	if (pdpr.mApName.size())
	    buf << " apn=" << pdpr.mApName.hexstr();
	if (pdpr.mPco.size())
	    buf << " pco=" << pdpr.mPco.hexstr();
	gSigConn.send(SigPdpActivate,0,si->getConnId(),buf.str());
}

static void handleDeactivatePdpContextRequest(SgsnInfo *si, L3SmMsgDeactivatePdpContextRequest &deact)
{
	std::ostringstream buf;
	unsigned int nsapi = 0;
	unsigned int nsapiMask = 0;
	if (deact.mTearDownIndicator)
		nsapiMask = si->freePdpAll(false);
	else {
		for (; nsapi <= 15; nsapi++) {
			PdpContext *pdp = si->getPdp(nsapi);
			if (pdp && (pdp->mTransactionId == deact.mTransactionId)) {
				si->freePdp(nsapi);
				nsapiMask = 1 << nsapi;
				break;
			}
		}
		if (!nsapiMask) {
			nsapi = 0;
			SGSNLOGF(WARNING,GPRS_ERR,"SGSN","No PDP context to deactivate for TI=" << deact.mTransactionId);
		}
	}
	// Accept PDP context deactivation even if not yet cleared upstream
	L3SmMsgDeactivatePdpContextAccept deactaccept(deact.mTransactionId);
	si->sgsnWriteHighSideMsg(deactaccept);
	if (nsapiMask)
		si->deactivateRabs(nsapiMask);
	if (deact.mTearDownIndicator)
		buf << "teardown=true";
	else
		buf << "nsapi=" << nsapi;
	buf << " mask=0x" << hex << std::setw(2) << std::setfill('0') << nsapiMask;
	gSigConn.send(SigPdpDeactivate,0,si->getConnId(),buf.str());
}

static void handleL3GmmMsg(SgsnInfo *si,ByteVector &frame1)
{
	L3GmmFrame frame(frame1);
	// Standard L3 header is 2 bytes:
	unsigned mt = frame.getMsgType();	// message type
	//SGSNLOG("CRACKING GMM MSG TYPE "<<mt);
	MSUEAdapter *ms = si->getMS();
	if (ms == NULL) {
		// This is a serious internal error.
		SGSNLOGF(ERR,GPRS_ERR,"SGSN","L3 message "<<L3GmmMsg::name(mt)
			<<" for non-existent MS Info struct" <<LOGHEX2("tlli",si->mMsHandle));
		return;
	}
	switch (mt) {
	case L3GmmMsg::AttachRequest: {
		L3GmmMsgAttachRequest armsg;
		armsg.gmmParse(frame);
		SGSNLOGF(INFO,GPRS_OK,"SGSN","Received "<<armsg.str()<<si << "\n received buffer " << frame1.hexstr());
		handleAttachRequest(si,armsg);
		dumpGmmInfo();
		break;
	}
	case L3GmmMsg::AttachComplete: {
		L3GmmMsgAttachComplete acmsg;
		//acmsg.gmmParse(frame);	// not needed, nothing in it.
		SGSNLOGF(INFO,GPRS_OK,"SGSN","Received "<<acmsg.str()<<si);
		handleAttachComplete(si,acmsg);
		dumpGmmInfo();
		break;
	}
	case L3GmmMsg::IdentityResponse: {
		L3GmmMsgIdentityResponse irmsg;
		irmsg.gmmParse(frame);
		SGSNLOGF(INFO,GPRS_OK,"SGSN","Received "<<irmsg.str()<<si);
		handleIdentityResponse(si,irmsg);
		break;
	}
	case L3GmmMsg::DetachRequest: {
		L3GmmMsgDetachRequest drmsg;
		drmsg.gmmParse(frame);
		SGSNLOGF(INFO,GPRS_OK,"SGSN","Received "<<drmsg.str()<<si);
		handleDetachRequest(si,drmsg);
		break;
	}
	case L3GmmMsg::DetachAccept:
		SGSNLOGF(INFO,GPRS_OK,"SGSN","Received DetachAccept");
		//TODO...
		break;
	case L3GmmMsg::RoutingAreaUpdateRequest: {
		L3GmmMsgRAUpdateRequest raumsg;
		raumsg.gmmParse(frame);
		SGSNLOGF(INFO,GPRS_OK,"SGSN","Received "<<raumsg.str()<<si);
		handleRAUpdateRequest(si,raumsg);
		break;
	}
	case L3GmmMsg::RoutingAreaUpdateComplete: {
		L3GmmMsgRAUpdateComplete racmsg;
		//racmsg.gmmParse(frame);  not needed
		SGSNLOGF(INFO,GPRS_OK,"SGSN","Received RAUpdateComplete "<<si);
		handleRAUpdateComplete(si,racmsg);
		break;
	}
	case L3GmmMsg::GMMStatus: {
		L3GmmMsgGmmStatus stmsg;
		stmsg.gmmParse(frame);
		SGSNLOGF(INFO,GPRS_OK,"SGSN","Received GMMStatus: "<<stmsg.mCause<<"=" <<GmmCause::name(stmsg.mCause)<<si);
		break;
	}
	case L3GmmMsg::AuthenticationAndCipheringResp: {
		L3GmmMsgAuthenticationResponse armsg;
		armsg.gmmParse(frame);
		SGSNLOGF(INFO,GPRS_OK,"SGSN","Received AuthenticationAndCipheringResp message "<<armsg.str()<<si);
		handleAuthenticationResponse(si,armsg);
		break;
	}
	case L3GmmMsg::AuthenticationAndCipheringFailure: {
		L3GmmMsgAuthenticationFailure afmsg;
		afmsg.gmmParse(frame);
		SGSNLOGF(INFO,GPRS_OK,"SGSN","Received AuthenticationAndCipheringFailure message "<<afmsg.str()<<si);
		handleAuthenticationFailure(si,afmsg);
		break;
	}
	case L3GmmMsg::ServiceRequest: {
		L3GmmMsgServiceRequest srmsg;
		srmsg.gmmParse(frame);
		SGSNLOGF(INFO,GPRS_OK,"SGSN","Received ServiceRequest message" << si);
		handleServiceRequest(si,srmsg);
		break;
	}

		// Downlink direction messages:
		//RoutingAreaUpdateAccept = 0x09,
		//AttachAccept = 0x02,
		//AttachReject = 0x04,
		//RoutingAreaUpdateReject = 0x0b,

		// Other: TODO?
		//ServiceAccept = 0x0d,
		//ServiceReject = 0x0e,
		//PTMSIReallocationCommand = 0x10,
		//PTMSIReallocationComplete = 0x11,
		//AuthenticationAndCipheringRej = 0x14,
		//AuthenticationAndCipheringFailure = 0x1c,
		//GMMInformation = 0x21,
	default:
		//SGSNWARN("Ignoring GPRS GMM message type "<<mt <<L3GmmMsg::name(mt));
		return;
	}
}

static bool handleL3SmMsg(SgsnInfo *si,ByteVector &frame1)
{
	L3SmFrame frame(frame1);
	unsigned mt = frame.getMsgType();
	switch (si->getConnId()) {
		case GprsConnNone:
			SGSNLOGF(ERR,GPRS_ERR,"SGSN","L3 message " << L3SmMsg::name(mt) << " in invalid state" << si);
			sendImplicitlyDetached(si);
			return true;
		case GprsConnLocal:
			return false;
	}
	if (gGprsMap.find(si->getConnId()) != si) {
		SGSNLOGF(ERR,GPRS_ERR,"SGSN","SM for not mapped" << si);
		if (!si->isRegistered()) {
			sendImplicitlyDetached(si);
			si->sgsnReset();
			return true;
		}
		adjustConnectionId(si);
	}
	switch (mt) {
		case L3SmMsg::ActivatePDPContextRequest:
			{
				L3SmMsgActivatePdpContextRequest pdpr(frame);
				SGSNLOGF(INFO,GPRS_OK,"SGSN","Received " << pdpr.str() << si);
				handleActivatePdpContextRequest(si,pdpr);
			}
			break;
		case L3SmMsg::SMStatus:
			{
				L3SmMsgSmStatus stmsg(frame);
				SGSNLOGF(INFO,GPRS_OK,"SGSN","Received SmStatus: " << stmsg.str() << si);
			}
			break;
		case L3SmMsg::DeactivatePDPContextRequest:
			{
				L3SmMsgDeactivatePdpContextRequest deact(frame);
				SGSNLOGF(INFO,GPRS_OK,"SGSN","Received DeactivatePdpContextRequest: " << deact.str());
				handleDeactivatePdpContextRequest(si,deact);
			}
			break;
		case L3SmMsg::DeactivatePDPContextAccept:
			{
				L3SmMsgDeactivatePdpContextAccept deact(frame);
				SGSNLOGF(INFO,GPRS_OK,"SGSN","Received DeactivatePdpContextAccept: " << deact.str());
			}
			break;
		// TODO: handle more messages
		default:
			SGSNLOGF(WARNING,GPRS_ERR,"SGSN","Ignoring GPRS SM message type " << mt << " " << L3SmMsg::name(mt));
			break;
	}
	return true;
}

// This is the old UMTS-centric entry point
//void Sgsn::sgsnWriteLowSide(ByteVector &payload,SgsnInfo *si, unsigned rbid)
//{
//	// No Pdcp, so just send it off.
//	si->sgsnSend2PdpLowSide(rbid, payload);
//}

// The handle is the URNTI and the rbid specfies the rab.
// In gprs, the handle is the TLLI and all the rab info is encoded into the
// payload with LLC headers so rbid is not used, which was a pretty dopey design.
void MSUEAdapter::sgsnWriteLowSide(ByteVector &payload, uint32_t handle, unsigned rbid)
{
	SgsnInfo *si = sgsnGetSgsnInfoByHandle(handle,true);	// Create if necessary.
#if RN_UMTS
	// No Pdcp, so just send it off.
//	si->sgsnSend2PdpLowSide(rbid, payload);
#else
	si->mLlcEngine->llcWriteLowSide(payload,si);
#endif
}

#if RN_UMTS
//void MSUEAdapter::sgsnHandleL3Msg(uint32_t handle, ByteVector &msgFrame)
//{
//	SgsnInfo *si = sgsnGetSgsnInfoByHandle(handle,true);	// Create if necessary.
//	handleL3Msg(si,msgFrame);
//}
#endif

void handleL3Msg(SgsnInfo *si, ByteVector &bv)
{
	unsigned pd = 0;
	try {
		L3GprsFrame frame(bv);
		if (frame.size() == 0) { // David saw this happen.
			//SGSNWARN("completely empty L3 uplink message "<<si);
			return;
		}
		pd = frame.getNibble(0,0);	// protocol descriminator
		switch ((GSM::L3PD) pd) {
		case GSM::L3GPRSMobilityManagementPD: {	// Couldnt we shorten this?
			handleL3GmmMsg(si,frame);
			break;
		}
		case GSM::L3GPRSSessionManagementPD: {	// Couldnt we shorten this?
			if (!handleL3SmMsg(si,frame))
				Ggsn::handleL3SmMsg(si,frame);
			break;
		}
		// TODO: Send GSM messages somewhere
		default:
			SGSNLOGF(WARNING,GPRS_ERR,"SGSN","unsupported L3 Message PD:"<<pd);
		}
	} catch(SgsnError) {
		return;	// Handled already
	} catch(ByteVectorError) {	// oops!
		SGSNLOGF(ERR,GPRS_ERR,"SGSN","internal error assembling SGSN message, pd="<<pd);	// not much to go on.
	}
}

// Forces the SgsnInfo to exist.
// For GPRS the handle is a TLLI.
// From GSM03.03 sec 2.6 Structure of TLLI; and reproduced at class MSInfo comments.
// The top bits of the TLLI encode where it came from.
// A local TLLI has top 2 bits 11, and low 30 bits are the P-TMSI.
// For UMTS, the handle is the invariant URNTI.
SgsnInfo *findSgsnInfoByHandle(uint32_t handle, bool create)
{
	SgsnInfo *si, *result = NULL;
	// We can delete unused SgsnInfo as soon as the attach procedure is over,
	// which is 15s, but let them hang around a bit longer so the user can see them.
	int idletime = gConfig.getNum("SGSN.Timer.MS.Idle");
	time_t now; time(&now);

	ScopedLock lock(sSgsnListMutex);
	RN_FOR_ALL(SgsnInfoList_t,sSgsnInfoList,si) {
		if (si->mMsHandle == handle) {result=si; continue;}
#if RN_UMTS
#else
#if NEW_TLLI_ASSIGN_PROCEDURE
		if (si->mAltTlli == handle) {result=si;continue;}
#endif
#endif
		// Kill off old ones, except ones that are the primary one for a gmm.
		GmmInfo *gmm = si->getGmm();
		if (gmm==NULL || gmm->getSI() != si) {
			if (now - si->mLastUseTime > idletime) { si->sirm(); }
		}
	}
	if (result) {
		time(&result->mLastUseTime);
		return result;
	}
	if (!create) { return NULL; }

	// Make a new one.
	SgsnInfo *sinew = new SgsnInfo(handle);
	return sinew;
}

// Now we create the SgsnInfo for the assigned ptmsi as soon as the ptmsi is created,
// even if the MS has not used it yet.
//GmmInfo *SgsnInfo::findGmm()
//{
//	if (mGmmp) { return mGmmp; }	// Hooked up previously.
//	return NULL;
// Old comment:
// For GPRS, the MS contacts with some random tlli, then we create a GmmInfo and a PTMSI,
// and send the PTMSI to the MS, but the GmmInfo is not yet hooked to any SgsnInfos.
// The MS will then call us again using a TLLI derived from the PTMSI,
// and we hook up that SgsnInfo to the GmmInfo right here.
//	if (! Sgsn::isUmts()) {
//		uint32_t tlli = mMsHandle;
//		// Only a local TLLI can be converted to a P-TMSI to look up the Gmm context.
//		if (Tlli::tlli2Type(tlli) == Tlli::LocalTlli) {
//			uint32_t ptmsi = Tlli::tlli2ptmsi(tlli);
//			GmmInfo *gmm;
//			RN_FOR_ALL(GmmInfoList_t,sGmmInfoList,gmm) {
//				if (gmm->mPTmsi == ptmsi) {
//					SGSNLOG("Hooking up"<<LOGHEX2("tlli",tlli)<<" to"<<LOGHEX2("ptmsi",ptmsi));
//					this->setGmm(gmm);
//					gmm->msi = this;
//					return gmm;
//				}
//			}
//		}
//	} else {
//		// In UMTS the Gmm context is indexed by URNTI.
//		// If this doesnt work right, we will need to look up the Gmm context
//		// from the ptmsi in the L3 messages.
//	}
//	return NULL;
//}

// Works, but not currently used:
void MSUEAdapter::sgsnFreePdpAll(uint32_t mshandle)
{
	SgsnInfo *si = sgsnGetSgsnInfoByHandle(mshandle,false);
	if (si) si->freePdpAll(true);
}

// Forces it to exist if it did not already.
static SgsnInfo *sgsnGetSgsnInfoByHandle(uint32_t mshandle, bool create)
{
	// We cant cache this thing for GPRS because it changes
	// during the TLLI assignment procedure.
	// We could cache it for UMTS, but that assumes the lifetime of the SgsnInfo
	// is greater than the UE, both of which are controlled by user parameters,
	// so to be safe, we are just going to look it up every time.
	// TODO: go back to caching it in UMTS only.
	//if (! mSgsnInfo) {
		//uint32_t mshandle = msGetHandle();
		//mSgsnInfo = findSgsnInfoByHandle(mshandle,create);
	//}
	//return mSgsnInfo;
	return findSgsnInfoByHandle(mshandle,create);
}

#if RN_UMTS
//SgsnInfo *MSUEAdapter::sgsnGetSgsnInfo()
//{
//	uint32_t mshandle = msGetHandle();
//	return findSgsnInfoByHandle(mshandle,false);
//}
#else
void MSUEAdapter::sgsnSendKeepAlive()
{
	// TODO
}
#endif

#if RN_UMTS
	// not applicable
#else
static void parseCaps(GmmInfo *gmm)
{
	if (/*gmm->mGprsMultislotClass == -1 &&*/ gmm->mgAttachInfo.mMsRadioAccessCap.size()) {
		MsRaCapability caps(gmm->mgAttachInfo.mMsRadioAccessCap);
		gmm->mGprsMultislotClass = caps.mCList[0].getCap(AccessCapabilities::GPRSMultislotClass);
		gmm->mGprsGeranFeaturePackI = caps.mCList[0].getCap(AccessCapabilities::GERANFeaturePackage1);
	}
}


int MSUEAdapter::sgsnGetMultislotClass(uint32_t mshandle)
{
	SgsnInfo *si = sgsnGetSgsnInfoByHandle(mshandle,false);
	if (!si) { return -1; }
	GmmInfo *gmm = si->getGmm();	// Must be non-null or we would not be here.
	if (!gmm) { return -1; }		// But dont crash if I'm mistaken.
	parseCaps(gmm);
	return gmm->mGprsMultislotClass;
}

bool MSUEAdapter::sgsnGetGeranFeaturePackI(uint32_t mshandle)
{
	SgsnInfo *si = sgsnGetSgsnInfoByHandle(mshandle,false);
	if (!si) { return -1; }
	GmmInfo *gmm = si->getGmm();	// Must be non-null or we would not be here.
	if (!gmm) { return -1; }		// But dont crash if I'm mistaken.
	parseCaps(gmm);
	return gmm->mGprsGeranFeaturePackI;
}
#endif

GmmState::state MSUEAdapter::sgsnGetRegistrationState(uint32_t mshandle, GmmInfo** outGmm)
{
	SgsnInfo *si = sgsnGetSgsnInfoByHandle(mshandle,false);
	if (!si) { return GmmState::GmmDeregistered; }
	GmmInfo *gmm = si->getGmm();	// Must be non-null or we would not be here.
	if (!gmm) { return GmmState::GmmDeregistered; }
	if (outGmm) *outGmm = gmm;
	return gmm->getGmmState();
}

GmmState::state MSUEAdapter::suspend()
{
	GmmInfo* gmm = 0;
	GmmState::state state = sgsnGetRegistrationState(msGetHandle(),&gmm);
	if (state == GmmState::GmmRegisteredNormal && gmm)
		gmm->setGmmState((state = GmmState::GmmRegisteredSuspended));
	return state;
}

GmmState::state MSUEAdapter::resume()
{
	GmmInfo* gmm = 0;
  	GmmState::state state = sgsnGetRegistrationState(msGetHandle(),&gmm);
	if (!(state == SGSN::GmmState::GmmRegisteredSuspended && gmm))
		return state;
	GLOG(INFO) << "Resuming GPRS for " << msid();
	state = GmmState::GmmRegisteredNormal;
	gmm->setGmmState(state);
	return state;
}

#if RN_UMTS
//void MSUEAdapter::sgsnHandleRabSetupResponse(unsigned rabId, bool success)
//{
//	SgsnInfo *si = sgsnGetSgsnInfo();
//	if (si == NULL) {
//		// Dont think this can happen, but be safe.
//		SGSNERROR("Received spurious RabSetupResponse for UE:"<<msid());
//		return;
//	}
//	if (success) {
//		PdpContext *pdp = si->getPdp(rabId);
//		if (pdp==NULL) return; // FIXME: Not sure what to do here
//		if (pdp->mUmtsStatePending) {
//			pdp->update(pdp->mPendingPdpr);
//			pdp->mUmtsStatePending = false;
//		}
//		sendPdpContextAccept(si,pdp);
//	} else {
//		// We do NOT want to send a RAB teardown message - we got here because
//		// the RAB setup did not work in the first place.  Just free it.
//		si->freePdp(rabId);
//	}
//}
#endif

const char *GmmState::GmmState2Name(GmmState::state state)
{
	switch (state) {
	CASENAME(GmmDeregistered)
	CASENAME(GmmRegistrationPending)
	CASENAME(GmmRegisteredNormal)
	CASENAME(GmmRegisteredSuspended)
	}
	return "";
}

// The alternate sgsnInfoPrint is used only for gmmDump and prints
void sgsnInfoDump(SgsnInfo *si,std::ostream&os)
{
	//if (si == gmm->getSI()) {continue;}		// Already printed the main one.
	uint32_t handle = si->mMsHandle;
	os << "SgsnInfo"<<LOGHEX(handle)
		<<" T3370:active="<<si->mT3370ImsiRequest.active()
		<<" remaining=" << si->mT3370ImsiRequest.remaining();
		MSUEAdapter *ms = si->getMS();
		if (ms) { os << ms->msid(); }
		else { os << " MS=not_active"; }
		AttachInfo *ati = &si->mtAttachInfo;
		if (ati->mPrevRaId.valid()) { os << " prev:"; ati->mPrevRaId.text(os); }
	if (!si->getGmm()) { os << " no gmm"; }
	os << endl;
}

void gmmInfoDump(GmmInfo *gmm,std::ostream&os,int options)
{
	os << " GMM Context:";
	os << LOGVAR2("imsi",gmm->mImsi.hexstr());
	os << LOGHEX2("ptmsi",gmm->mPTmsi);
	os << LOGHEX2("tlli",gmm->getTlli());
	if (!gmm->mImei.empty())
	    os << LOGVAR2("imei",gmm->mImei);
	os << LOGVAR2("state",GmmState::GmmState2Name(gmm->getGmmState()));
	time_t now; time(&now);
	os << LOGVAR2("age",(gmm->mAttachTime ? now - gmm->mAttachTime : 0));
	os << LOGVAR2("idle",now - gmm->mActivityTime);
	SgsnInfo *si = gmm->getSI();
	if (!(options & printNoMsId)) {
		if (si) {	// Can this be null?  No, but dont crash.
			// The mPrevRaId is generally invalid in the SgsnInfo for the GMM,
			// because it is the one we assigned, and the routing info is in the SgsnInfo
			// the MS initially called in on.
			//os << LOGVAR2("oldMCC",si->mOldMcc);
			//os << LOGVAR2("oldMNC",si->mOldMnc);
			// The GPRS ms struct will disappear shortly after the MS stops communicating with us.
			MSUEAdapter *ms = si->getMS();
			if (ms) { os << ms->msid(); }
			else { os << " MS=not_active"; }
		}
	}

	if (gmm->mConnId >= 0)
		os << " ConnId=" << gmm->mConnId;
	else {
		os << " IPs=";
		int ipcnt = 0;
		for (unsigned nsapi = 0; nsapi < GmmInfo::sNumPdps; nsapi++) {
			if (gmm->isNSapiActive(nsapi)) {
				// FIXME: Darn it, we need to lock the pdp contexts for this too.
				// Go ahead and do it anyway, because collision is very low probability.
				PdpContext *pdp = gmm->getPdp(nsapi);
				mg_con_t *mgp;	// Temp variable reduces probability of race; the mgp itself is immortal.
				if (pdp && (mgp=pdp->mgp)) {
					char buf[30];
					if (ipcnt++) {os <<",";}
					os << ip_ntoa(mgp->mg_ip,buf);
				}
			}
		}
		if (ipcnt == 0) { os <<"none"; }
	}
	os << endl;

	if (options & printDebug) {
		// Print out all the SgsnInfos associated with this GmmInfo.
		RN_FOR_ALL(SgsnInfoList_t,sSgsnInfoList,si) {
			if (si->getGmm() != gmm) {continue;}
			os <<"\t";	// this sgsn is associated with the GmmInfo just above it.
			sgsnInfoDump(si,os);
		}
	}

	// Now the caps:
	if ((options & printCaps) && gmm->mgAttachInfo.mMsRadioAccessCap.size()) {
		MsRaCapability caps(gmm->mgAttachInfo.mMsRadioAccessCap);
		caps.text2(os,true);
	}
	//os << endl;	// This is extra.  There is one at the end of the caps.
}

void gmmDump(std::ostream &os)
{
	// The sSgsnListMutex exists for this moment: to allow this cli list routine
	// to run from a foreign thread.
	ScopedLock lock(sSgsnListMutex);
	int debug = sgsnDebug();
	GmmInfo *gmm;
	RN_FOR_ALL(GmmInfoList_t,sGmmInfoList,gmm) {
		gmmInfoDump(gmm,os,debug ? printDebug : 0);
		os << endl;	// This is extra.  There is one at the end of the caps.
	}
	// Finally, print out SgsnInfo that are not yet associated with a GmmInfo.
	if (debug) {
		SgsnInfo *si;
		RN_FOR_ALL(SgsnInfoList_t,sSgsnInfoList,si) {
			if (! si->getGmm()) { sgsnInfoDump(si,os); }
		}
	}
}

void dumpGmmInfo()
{
	if (sgsnDebug()) {
		std::ostringstream ss;
		gmmDump(ss);
		SGSNLOGF(INFO,GPRS_MSG,"SGSN",ss.str());
	}
}

void SgsnInfo::setGmm(GmmInfo *gmm)
{
	// Copy pertinent info from the Routing Update or Attach Request message into the GmmInfo.
	gmm->mgAttachInfo.copyFrom(mtAttachInfo);
	this->mGmmp = gmm;
}

#if NEW_TLLI_ASSIGN_PROCEDURE
static void killOtherTlli(SgsnInfo *si,uint32_t newTlli)
{
	SgsnInfo *othersi = findSgsnInfoByHandle(newTlli,false);
	if (othersi && othersi != si) {
		if (othersi->getGmm()) {
			// This 'impossible' situation can happen if an MS that
			// is already attached tries to attach again.
			// Easy to reproduce by pulling the power on an MS.  (Turning off may not be enough.)
			// For example:
			// MS -> attachrequest TLLI=80000001; creates new SgsnInfo 80000001
			// MS <- attachaccept
			// MS -> attachcomplete TLLI=c0000001; SgsnInfo 80000001 changed to c0000001
			// MS gets confused (turn off or pull battery)
			// MS -> attachrequest TLLI=80000001; creates new SgsnInfo 80000001
			// MS <- attachaccept TLLI=80000001;  <--- PROBLEM 1!  See below.
			// MS -> attachcomplete TLLI=c0000001; <--- PROBLEM 2: Both SgsnInfo exist.
			// PROBLEM 1:  We have already issued the TLLI change command so L2
			// is now using the new TLLI.  We will use the old TLLI (80000001)
			// in the attachaccept, which will cause a 'change tlli' procedure in L2
			// to switch back to TLLI 80000001 temporarily.
			// PROBLEM 2: Solved by deleting the original registered SgsnInfo (c0000001 above)
			// and then caller will change the TLLI of the unregistred one (80000001 above.)
			SGSNLOGF(WARNING,GPRS_ERR,"SGSN","Probable repeat attach request: TLLI change procedure"<<LOGVAR(newTlli)
				<<" for SgsnInfo:"<<si
				<<" found existing registered SgsnInfo:"<<othersi);
			// I dont think any recovery is possible; sgsn is screwed up.
		} else {
			// We dont know or care where this old SgsnInfo came from.
			// Destroy it with prejudice and use si, which is the
			// SgsnInfo the MS is using to talk with us right now.
			SGSNLOGF(WARNING,GPRS_ERR,"SGSN","TLLI change procedure"<<LOGVAR(newTlli)
				<<" for SgsnInfo:"<<si
				<<" overwriting existing unregistered SgsnInfo:"<<othersi);
			othersi->sirm();
		}
	}
}
#endif

// Switch to the new tlli.  If now, do it now, otherwise, just allocate the new SgsnInfo,
#if NEW_TLLI_ASSIGN_PROCEDURE
// just set altTlli to the future TLLI we will be using after the attach procedure,
#endif
// which we do ahead of time so we can inform GPRS L2 that the new and old TLLIs are the same MS.
// Return the new si.  In the new tlli procedure, it is the same as the old.
// ------------------
// Note the following sequence, easy to reproduce by pulling the power on an MS:
// MS -> attachrequest TLLI=80000001; creates new SgsnInfo 80000001
// MS <- attachaccept
// MS -> attachcomplete TLLI=c0000001; SgsnInfo 80000001 changed to c0000001
// MS gets confused (turn off or pull battery)
// MS -> attachrequest TLLI=80000001; creates new SgsnInfo 80000001
// MS <- attachaccept TLLI=80000001;  <--- PROBLEM 1!  See below.
// MS -> attachcomplete TLLI=c0000001; <--- PROBLEM 2: Both SgsnInfo exist.
// PROBLEM 1:  We have already issued the TLLI change command so L2
// is now using the new TLLI.  Easy fix is we use the old TLLI (80000001)
// in the attachaccept, which will cause a 'change tlli' procedure in L2
// to switch back to TLLI 80000001 temporarily until we receive attachcomplete.
// PROBLEM 2: The SgsnInfo for the new tlli will already exist.
// ???? Solved by deleting the original registered SgsnInfo (c0000001 above)
// ???? and then caller will change the TLLI of the unregistred one (80000001 above.)
SgsnInfo * SgsnInfo::changeTlli(bool now)
{
	GmmInfo *gmm = getGmm();
	uint32_t newTlli = gmm->getTlli();
#if NEW_TLLI_ASSIGN_PROCEDURE
	SgsnInfo *si = this;
	if (si->mMsHandle != newTlli) {
		killOtherTlli(si,newTlli);
		if (now) {
			si->mAltTlli = si->mMsHandle;
			si->mMsHandle = newTlli;
		} else {
			si->mAltTlli = newTlli;
		}
	}
	return si;
#else
	SgsnInfo *othersi = findSgsnInfoByHandle(newTlli,true);
	// We will use the new tlli for downlink l3 messages, eg, pdp context messages,
	// unless they use some other SI specifically, like AttachAccept
	// must be sent on the SI tha the AttachRequest arrived on.
	if (othersi != this) {
		if (!othersi->getMS()) {
			// Alias new TLLI in the MSInfo so it can be identified later
			MSUEAdapter *ms = getMS();
			if (ms)
				ms->msAliasTlli(newTlli);
		}
		othersi->setGmm(gmm);
		othersi->mConnId = getConnId();
		SGSNLOGF(INFO,GPRS_CHECK_FAIL,"SGSN","Changing TLLI" << (now ? " now" : "") << " to" << othersi);
		if (now) {
			gmm->msi = othersi;
			if (mConnId >= 0) {
				SGSNLOGF(INFO,GPRS_CHECK_OK|GPRS_OK,"SGSN","replacing TLLI" << LOGHEX2("old",mMsHandle) << LOGHEX2("with new",othersi->mMsHandle) << " in connection " << mConnId);
				gGprsMap.remap(othersi,mConnId);
			}
		}
	}
	return othersi;
#endif
}

// If si, forces the GmmInfo for this imsi to exist and associates it with that si.
// If si == NULL, return NULL if gmm not found - used for CLI.
GmmInfo *findGmmByImsi(ByteVector &imsi, SgsnInfo *si, uint32_t ptmsi)
{
	ScopedLock lock(sSgsnListMutex);
	GmmInfo *gmm, *result = NULL;
	// 24.008 11.2.2: Implicit Detach timer default is 4 min greater
	// than T3323, which can be provided in AttachAccept, otherwise
	// defaults to T3312, which defaults to 54 minutes.
	int attachlimit = gConfig.getNum("SGSN.Timer.ImplicitDetach");	// expiration time in seconds.
	time_t now; time(&now);
	RN_FOR_ALL(GmmInfoList_t,sGmmInfoList,gmm) {
		if (now - gmm->mActivityTime > attachlimit) {
			GmmRemove(gmm);
			continue;
		}
		if (gmm->mImsi == imsi) { result = gmm; }
	}
	if (result) {
		if (si) si->setGmm(result);
		return result;
	}
	if (!si) { return NULL; }

	// Not found.  Make a new one in state Registration Pending.
	gmm = new GmmInfo(imsi,ptmsi);
	gmm->setGmmState(GmmState::GmmRegistrationPending);
	gmm->mConnId = si->getConnId();
	si->setGmm(gmm);
	gmm->msi = si;
	SGSNLOGF(INFO,GPRS_OK,"SGSN","Allocated new GMM info for" << si);
#if RN_UMTS
		// For UMTS, the si is indexed by URNTI, which is invariant, so hook up and we are finished.
#else
		// We hook up the GMM context to the SgsnInfo corresponding to the assigned P-TMSI,
		// even if that SgsnInfo does not exist yet,
		// rather than the SgsnInfo corresponding to the current TLLI, which could be anything.
		// The MS will use the SgsnInfo for the P-TMSI to talk to us after a successful attach.
		si->changeTlli(false);
//#if NEW_TLLI_ASSIGN_PROCEDURE
//		// 3GPP 04.64 7.2.1.1 and 8.3: Perform the TLLI reassignment procedure.
//		// Change the TLLI in the SgsnInfo the MS is currently using.
//		// The important point is that the LLC state does not change.
//		uint32_t newTlli = gmm->getTlli();
//		killOtherTlli(si,newTlli);
//		// Do the TLLI reassignment.
//		if (si->mMsHandle != newTlli) { // Any other case is extremely unlikely.
//			// We must continue using the existing MsHandle until we
//			// receive the AttachComplete message from the MS, but mark
//			// that the new tlli is an alias for this TLLI.
//			si->mAltTlli = newTlli;
//		}
//#else
//		SgsnInfo *newsi = findSgsnInfoByTlli(gmm->getTlli(),true);
//		newsi->setGmm(gmm);
//		// NO, not until attachComplete: gmm->msi = newsi;	// Use this one instead.
//#endif
#endif
	return gmm;
}

void RabStatus::text(std::ostream &os) const
{
	os <<"RabStatus(mStatus=";
	switch (mStatus) {
	case RabIdle: os << "idle"; break;
	case RabFailure: os << "failure"; break;
	case RabPending: os << "pending"; break;
	case RabAllocated: os << "allocated"; break;
	case RabDeactPending: os << "deactPending"; break;
	}
	os<<LOGVAR2("mFailCode",SmCause::name(mFailCode));
	os<<LOGVAR(mRateDownlink)<<LOGVAR(mRateUplink);
	os<<")";
}

void MSUEAdapter::sgsnPrint(uint32_t mshandle, int options, std::ostream &os)
{
	ScopedLock lock(sSgsnListMutex);		// Probably not needed.
	SgsnInfo *si = sgsnGetSgsnInfoByHandle(mshandle,false);
	if (!si) { os << " GMM state unknown\n"; return; }
	GmmInfo *gmm = si->getGmm();	// Must be non-null or we would not be here.
	if (!gmm) { os << " GMM state unknown\n"; return; }
	gmmInfoDump(gmm,os,options);
}

}; // namespace
