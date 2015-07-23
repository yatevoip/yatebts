/**
 * SigConnection.cpp
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * Signaling socket connection
 *
 * Yet Another Telephony Engine - Base Transceiver Station
 * Copyright (C) 2013-2014 Null Team Impex SRL
 * Copyright (C) 2014 Legba, Inc
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

#include "SigConnection.h"
#include "ConnectionMap.h"

#include <Logger.h>
#include <Globals.h>
#include <GSML3CommonElements.h>
#include <GSMLogicalChannel.h>
#include <GSML3RRMessages.h>
#include <GSMTransfer.h>
#include <GSMConfig.h>
#include <NeighborTable.h>
#include <SgsnConn.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

using namespace Connection;
using namespace GSM;
using namespace SGSN;

#define PROTO_VER 1
#define HB_MAXTIME 30000
#define HB_TIMEOUT 60000

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

static void processPaging(L3MobileIdentity& ident, uint8_t type)
{
    switch (type) {
	case ChanTypeVoice:
	    gBTS.pager().addID(ident,GSM::TCHFType);
	    break;
	case ChanTypeSMS:
	case ChanTypeSS:
	    gBTS.pager().addID(ident,GSM::SDCCHType);
	    break;
	default:
	    gBTS.pager().removeID(ident);
    }
}

static void processPaging(const char* data, uint8_t type)
{
    std::string ident = getPrefixed("identity=",data);
    if (ident.empty()) {
	LOG(ERR) << "can't extract identity from paging data: " << data;
	return;
    }
    if (!ident.compare(0,4,"TMSI")) {
	char* err = 0;
	unsigned int tmsi = (unsigned int)::strtol(ident.c_str() + 4,&err,16);
	if (err && !*err) {
	    std::string imsi = getPrefixed("imsi=",data);
	    L3MobileIdentity id(tmsi,imsi.c_str());
	    processPaging(id,type);
	}
	else
	    LOG(ERR) << "received invalid Paging TMSI " << (ident.c_str() + 4);
    }
    else if (!ident.compare(0,4,"IMSI")) {
	L3MobileIdentity id(ident.c_str() + 4);
	processPaging(id,type);
    }
    else
	LOG(ERR) << "received unknown Paging identity " << ident;
}

static void processHandover(SigConnection* conn, unsigned char info)
{
    if (!gBTS.hold()) {
	GSM::LogicalChannel* chan = gBTS.getTCH();
	if (chan) {
	    int id = gConnMap.map(chan,chan->SACCH(),info);
	    if (id >= 0) {
		gConnMap.mapMedia(id,dynamic_cast<TCHFACCHLogicalChannel*>(chan));
		LOG(INFO) << "for reference " << (unsigned int)info << " mapped id " << id << " channel " << *chan;
		const L3ChannelDescription desc = chan->channelDescription();
		L3HandoverCommand cmd(
		    L3CellDescription(gTRX.C0(),gBTS.NCC(),gBTS.BCC()),
		    L3ChannelDescription2(desc.typeAndOffset(),desc.TN(),desc.TSC(),desc.ARFCN()),
		    L3HandoverReference(info),
		    L3PowerCommandAndAccessType(),
		    L3SynchronizationIndication(true,true)
		);
		chan->handoverPending(true);
		L3Frame frm(cmd,DATA);
		unsigned int len = frm.length();
		unsigned char buf[len];
		frm.pack(buf);
		conn->send(SigHandoverAck,info,id,buf,len);
		return;
	    }
	    else {
		LOG(ERR) << "for reference " << (unsigned int)info << " failed to map channel " << *chan;
		chan->send(HARDRELEASE);
	    }
	}
	else
	    LOG(NOTICE) << "for reference " << (unsigned int)info << " failed to allocate channel";
    }
    else
	LOG(NOTICE) << "for reference " << (unsigned int)info << " the BTS was holding off";
    // We failed, report it back
    conn->send(SigHandoverReject,info);
}


bool SigConnection::send(BtsPrimitive prim, unsigned char info)
{
    assert((prim & 0x80) != 0);
    if (!valid())
	return false;
    unsigned char buf[2];
    buf[0] = (unsigned char)prim;
    buf[1] = info;
    return send(buf,2);
}

bool SigConnection::send(BtsPrimitive prim, unsigned char info, unsigned int id)
{
    assert((prim & 0x80) == 0);
    if (!valid())
	return false;
    unsigned char buf[4];
    buf[0] = (unsigned char)prim;
    buf[1] = info;
    buf[2] = (unsigned char)(id >> 8);
    buf[3] = (unsigned char)id;
    return send(buf,4);
}

bool SigConnection::send(BtsPrimitive prim, unsigned char info, unsigned int id, const void* data, size_t len)
{
    assert((prim & 0x80) == 0);
    if (!valid())
	return false;
    unsigned char buf[len + 4];
    buf[0] = (unsigned char)prim;
    buf[1] = info;
    buf[2] = (unsigned char)(id >> 8);
    buf[3] = (unsigned char)id;
    ::memcpy(buf + 4,data,len);
    return send(buf,len + 4);
}

bool SigConnection::send(unsigned char sapi, unsigned int id, const GSM::L3Frame* frame)
{
    assert(frame);
    if (!valid())
	return false;
    unsigned int len = frame->length();
    unsigned char buf[len];
    frame->pack(buf);
    return send(SigL3Message,sapi,id,buf,len);
}

bool SigConnection::send(const void* buffer, size_t len)
{
    if (!GenConnection::send(buffer,len))
	return false;
    LOG(DEBUG) << "sent message primitive " << (unsigned int)((const unsigned char*)buffer)[0] << " length " << len;
    mHbSend.future(HB_MAXTIME);
    return true;
}

void SigConnection::process(BtsPrimitive prim, unsigned char info)
{
    switch (prim) {
	case SigHandshake:
	    // TODO
	    break;
	case SigHeartbeat:
	    // TODO
	    break;
	case SigHandoverRequest:
	    processHandover(this,info);
	    break;
	default:
	    LOG(ERR) << "unexpected primitive " << prim;
    }
}

void SigConnection::process(BtsPrimitive prim, unsigned char info, const unsigned char* data, size_t len)
{
    switch (prim) {
	case SigStartPaging:
	    if (len >= 12)
		processPaging((const char*)data,info);
	    else
		LOG(ERR) << "received short Start Paging of length " << len;
	    break;
	case SigStopPaging:
	    if (len >= 12)
		processPaging((const char*)data,0xff);
	    else
		LOG(ERR) << "received short Stop Paging of length " << len;
	    break;
	case SigNeighborsList:
	    if (!gNeighborTable.setNeighbors((const char*)data))
		LOG(ERR) << "received invalid Neighbors List of length " << len;
	    break;
	default:
	    LOG(ERR) << "unexpected primitive " << prim << " with data";
    }
}

void SigConnection::process(BtsPrimitive prim, unsigned char info, unsigned int id)
{
    switch (prim) {
	case SigConnRelease:
	    {
		LogicalChannel* ch = gConnMap.unmap(id);
		if (!ch) {
		    SgsnInfo* sgsn = gGprsMap.unmap(id);
		    if (sgsn)
			SgsnConn::detach(sgsn, info, "");
		    else
			LOG(INFO) << "received SigConnRelease for unmapped id " << id;
		}
		else if (info & 0x80)
		    ch->send(HARDRELEASE);
		else
		    ch->send(L3ChannelRelease(info));
	    }
	    break;
	case SigStartMedia:
	    {
		LogicalChannel* ch = gConnMap.find(id);
		if (ch) {
		    TCHFACCHLogicalChannel* tch = dynamic_cast<TCHFACCHLogicalChannel*>(ch);
		    if (tch) {
			GSM::L3ChannelMode mode((GSM::L3ChannelMode::Mode)info);
			gConnMap.mapMedia(id,tch);
			ch->send(GSM::L3ChannelModeModify(ch->channelDescription(),mode));
		    }
		    else {
			TCHFACCHLogicalChannel* tch = gConnMap.findMedia(id);
			if (!tch) {
			    LOG(NOTICE) << "received Start Media without traffic channel on id " << id;
			    send(SigMediaError,ErrInterworking,id);
			    break;
			}
			tch->open();
			tch->setPhy(*ch);
			GSM::L3ChannelMode mode((GSM::L3ChannelMode::Mode)info);
			gConnMap.mapMedia(id,tch);
			ch->send(GSM::L3AssignmentCommand(tch->channelDescription(),mode));
		    }
		}
		else
		    LOG(ERR) << "received Start Media for unmapped id " << id;
	    }
	    break;
	case SigStopMedia:
	    {
		TCHFACCHLogicalChannel* tch = gConnMap.findMedia(id);
		if (!tch)
		    break;
		gConnMap.mapMedia(id,0);
		if (static_cast<LogicalChannel*>(tch) != gConnMap.find(id))
		    tch->send(GSM::RELEASE);
	    }
	    break;
	case SigAllocMedia:
	    {
		TCHFACCHLogicalChannel* tch = gConnMap.findMedia(id);
		if (tch)
		    break;
		LogicalChannel* ch = gConnMap.find(id);
		if (!ch) {
		    LOG(ERR) << "received Alloc Media for unmapped id " << id;
		    break;
		}
		if (dynamic_cast<TCHFACCHLogicalChannel*>(ch))
		    break;
		tch = gBTS.getTCH();
		if (tch)
		    gConnMap.mapMedia(id,tch);
		else {
		    LOG(WARNING) << "congestion, no TCH available for assignment";
		    send(SigMediaError,ErrCongestion,id);
		}
	    }
	    break;
	case SigEstablishSAPI:
	    {
		LogicalChannel* ch = gConnMap.find(id);
		if (ch) {
		    unsigned char sapi = info;
		    if (sapi & 0x80) {
			ch = ch->SACCH();
			sapi &= 0x7f;
		    }
		    if (!ch) {
			LOG(ERR) << "received Establish SAPI for missing SACCH of id " << id;
			break;
		    }
		    if ((sapi > 4) || !ch->debugGetL2(sapi)) {
			LOG(ERR) << "received Establish for invalid SAPI " << (unsigned int)info;
			break;
		    }
		    if (ch->multiframeMode(sapi))
			send(prim,info,id);
		    else
			ch->send(GSM::ESTABLISH,sapi);
		}
		else
		    LOG(ERR) << "received Establish SAPI for unmapped id " << id;
	    }
	    break;
	case SigGprsAttachOk:
	    {
		SgsnInfo* sgsn = gGprsMap.find(id);
		if (sgsn)
		    SgsnConn::attachAccept(sgsn,"");
		else
		    LOG(ERR) << "received GPRS Attach Complete for unmapped id " << id;
	    }
	    break;
	case SigGprsAttachLBO:
	    {
		SgsnInfo* sgsn = gGprsMap.find(id);
		if (sgsn) {
		    gGprsMap.unmap(sgsn);
		    SgsnConn::attachLBO(sgsn);
		}
		else
		    LOG(ERR) << "received GPRS Local Breakout for unmapped id " << id;
	    }
	    break;
	case SigGprsAttachRej:
	    {
		SgsnInfo* sgsn = gGprsMap.find(id);
		if (sgsn) {
		    gGprsMap.unmap(sgsn);
		    SgsnConn::attachRej(sgsn,info);
		}
		else
		    LOG(INFO) << "received GPRS Attach Reject for unmapped id " << id;
	    }
	    break;
	case SigGprsIdentityReq:
	    {
		SgsnInfo* sgsn = gGprsMap.find(id);
		if (sgsn)
		    SgsnConn::identityReq(sgsn);
		else
		    LOG(ERR) << "received GPRS Identity Request for unmapped id " << id;
	    }
	    break;
	default:
	    LOG(ERR) << "unexpected primitive " << prim << " with id " << id;
    }
}

void SigConnection::process(BtsPrimitive prim, unsigned char info, unsigned int id, const unsigned char* data, size_t len)
{
    switch (prim) {
	case SigConnRelease:
	    {
		LogicalChannel* ch = gConnMap.unmap(id);
		if (!ch) {
		    SgsnInfo* sgsn = gGprsMap.unmap(id);
		    if (sgsn)
			SgsnConn::detach(sgsn, info, (const char*)data);
		    else
			LOG(INFO) << "received SigConnRelease for unmapped id " << id;
		}
		else if (info & 0x80)
		    ch->send(HARDRELEASE);
		else {
		    ByteVector extra;
		    if (!extra.fromHexa((const char*)data))
			LOG(ERR) << "received invalid Channel Release extra bytes on id " << id;
		    ch->send(L3ChannelRelease(info,extra));
		}
	    }
	    break;
	case SigL3Message:
	    {
		LogicalChannel* ch = gConnMap.find(id);
		if (ch) {
		    if (info & 0x80) {
			ch = ch->SACCH();
			info &= 0x7f;
		    }
		    if (!ch) {
			LOG(ERR) << "received L3 frame for missing SACCH of id " << id;
			break;
		    }
		    if ((info < 4) && ch->debugGetL2(info)) {
			L3Frame frame((const char*)data,len);
			ch->send(frame,info);
		    }
		    else
			LOG(ERR) << "received L3 frame for invalid SAPI " << (unsigned int)info;
		}
		else
		    LOG(ERR) << "received L3 frame for unmapped id " << id;
	    }
	    break;
	case SigGprsAttachOk:
	    {
		SgsnInfo* sgsn = gGprsMap.find(id);
		if (sgsn)
		    SgsnConn::attachAccept(sgsn, (const char*)data);
		else
		    LOG(ERR) << "received GPRS Attach Complete for unmapped id " << id;
	    }
	    break;
	case SigGprsAuthRequest:
	    {
		SgsnInfo* sgsn = gGprsMap.find(id);
		if (sgsn) {
		    if (len >= 12)
			SgsnConn::authRequest(sgsn, (const char*)data);
		    else
			LOG(ERR) << "received short GPRS Auth Request of length " << len;
		}
		else
		    LOG(ERR) << "received GPRS Auth Request for unmapped id " << id;
	    }
	    break;
	case SigGprsDetach:
	    {
		SgsnInfo* sgsn = gGprsMap.find(id);
		if (sgsn) {
		    if (len >= 1) {
			gGprsMap.unmap(sgsn);
			SgsnConn::detach(sgsn, info, (const char*)data);
		    }
		    else
			LOG(ERR) << "received short GPRS Detach of length " << len;
		}
		else
		    LOG(INFO) << "received GPRS Detach for unmapped id " << id;
	    }
	    break;
	case SigPdpActivate:
	    {
		SgsnInfo* sgsn = gGprsMap.find(id);
		if (sgsn) {
		    if (len >= 7)
			SgsnConn::pdpActivate(sgsn, !!info, (const char*)data);
		    else
			LOG(ERR) << "received short GPRS PDP Activate of length " << len;
		}
		else
		    LOG(ERR) << "received GPRS PDP Activate for unmapped id " << id;
	    }
	    break;
	case SigPdpModify:
	    {
		SgsnInfo* sgsn = gGprsMap.find(id);
		if (sgsn) {
		    if (len >= 7)
			SgsnConn::pdpModify(sgsn, !!info, (const char*)data);
		    else
			LOG(ERR) << "received short GPRS PDP Modify of length " << len;
		}
		else
		    LOG(ERR) << "received GPRS PDP Modify for unmapped id " << id;
	    }
	    break;
	case SigPdpDeactivate:
	    {
		SgsnInfo* sgsn = gGprsMap.find(id);
		if (sgsn) {
		    if (len >= 7)
			SgsnConn::pdpDeactivate(sgsn, (const char*)data);
		    else
			LOG(ERR) << "received short GPRS PDP Deactivate of length " << len;
		}
		else
		    LOG(INFO) << "received GPRS PDP Deactivate for unmapped id " << id;
	    }
	    break;
	default:
	    LOG(ERR) << "unexpected primitive " << prim << " with id " << id << " and data";
    }
}

void SigConnection::process(const unsigned char* data, size_t len)
{
    if (len < 2) {
	LOG(ERR) << "received short message of length " << len;
	return;
    }
    LOG(DEBUG) << "received message primitive " << (unsigned int)data[0] << " length " << len;
    mHbRecv.future(HB_TIMEOUT);
    // TODO: If this is really slow move everything to a separate thread
    Timeval timer;
    if (data[0] & 0x80) {
	len -= 2;
	if (len)
	    process((BtsPrimitive)data[0],data[1],data + 2,len);
	else
	    process((BtsPrimitive)data[0],data[1]);
    }
    else {
	if (len < 4) {
	    LOG(ERR) << "received short message of length " << len << " for primitive " << (unsigned int)data[0];
	    return;
	}
	unsigned int id = (((unsigned int)data[2]) << 8) | data[3];
	len -= 4;
	if (len)
	    process((BtsPrimitive)data[0],data[1],id,data + 4,len);
	else
	    process((BtsPrimitive)data[0],data[1],id);
    }
    long ms = timer.elapsed();
    if (ms > 500)
	LOG(ERR) << "primitive " << (unsigned int)data[0] << " length " << len << " took " << ms << " ms";
}

void SigConnection::started()
{
    mHbRecv.future(HB_TIMEOUT);
    mHbSend.future(HB_MAXTIME);
    send(SigHandshake);
}

void SigConnection::idle()
{
    if (mHbRecv.passed()) {
	LOG(ALERT) << "heartbeat timed out!";
	clear();
	// TODO abort
    }
    if (mHbSend.passed() && !send(SigHeartbeat))
	mHbSend.future(HB_MAXTIME);
}

/* vi: set ts=8 sw=4 sts=4 noet: */
