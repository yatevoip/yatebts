/**@file SMSCB Control (L3), GSM 03.41. */
/*
* Copyright 2010 Kestrel Signal Processing, Inc.
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

#include "ControlCommon.h"
#include <GSMLogicalChannel.h>
#include <GSMConfig.h>
#include <GSMSMSCBL3Messages.h>
#include <stdio.h>
#include <string>
#include <list>

namespace { // anonymous

class CbMessage
{
public:
	unsigned mId;
	unsigned mCode;
	unsigned mGs;
	unsigned mDcs;
	std::string mData;
	unsigned mPages;
	unsigned mUpdate;

	CbMessage(unsigned wId, unsigned wCode, unsigned wGs, unsigned wDcs, const std::string wData, unsigned wUpdate)
		: mId(wId), mCode(wCode), mGs(wGs), mDcs(wDcs), mData(wData), mUpdate(wUpdate)
		{ mPages = (mData.size() + 81) / 82; mData.resize(mPages * 82,'\x2b'); }

	void send(GSM::CBCHLogicalChannel* CBCH)
		{
			for (unsigned page = 0; page < mPages; page++) {
				// Format the page into an L3 message.
				GSM::L3SMSCBMessage message(
					GSM::L3SMSCBSerialNumber(mGs,mCode,mUpdate),
					GSM::L3SMSCBMessageIdentifier(mId),
					GSM::L3SMSCBDataCodingScheme(mDcs),
					GSM::L3SMSCBPageParameter(page+1,mPages),
					GSM::L3SMSCBContent(mData.data() + (82 * page)));
				// Send it.
				LOG(DEBUG) << "sending L3 message page " << page+1 << ": " << message;
				CBCH->send(message);
			}
		}
};

class CbQueue : public std::list<CbMessage>
{
private:
	Mutex mMutex;

public:
	CbMessage fetch()
		{
			mMutex.lock();
			if (empty()) {
				mMutex.unlock();
				std::exception e;
				throw e;
			}
			CbMessage msg = front();
			pop_front();
			push_back(msg);
			mMutex.unlock();
			return msg;
		}

	void clear()
		{
			ScopedLock lock(mMutex);
			std::list<CbMessage>::clear();
		}

	void add(CbMessage msg)
		{
			ScopedLock lock(mMutex);
			push_back(msg);
		}

	unsigned remove(unsigned id, unsigned code)
		{
			ScopedLock lock(mMutex);
			if (empty())
				return 0;
			for (std::list<CbMessage>::iterator it = begin(); it != end(); ++it) {
				if ((it->mId == id) && (it->mCode == code)) {
					unsigned upd = (it->mUpdate + 1) % 16;
					erase(it);
					return upd;
				}
			}
			return 0;
		}

	void remove(unsigned id)
		{
			ScopedLock lock(mMutex);
			if (empty())
				return;
			for (std::list<CbMessage>::iterator it = begin(); it != end(); ) {
				if (it->mId == id)
					it = erase(it);
				else
					++it;
			}
		}
};

static CbQueue gCbMessages;


}; // anonymous namespace


void* Control::SMSCBSender(void*)
{
	gCbMessages.clear();

	LOG(NOTICE) << "SMSCB service starting";

	// Get a channel.
	GSM::CBCHLogicalChannel* CBCH = gBTS.getCBCH();

	while (CBCH) {
		// Get the next message ready to send.
		try {
			CbMessage msg = gCbMessages.fetch();
			msg.send(CBCH);
		}
		catch (...) {
			usleep(250000);
		}
	}
	// keep the compiler from whining
	return NULL;
}


bool Control::SMSCBwrite(unsigned id, unsigned code, unsigned gs, unsigned dcs, const std::string data)
{
	if ((id >= 0xffff) || (code > 1023) || (gs > 3) || (dcs > 255) || !gBTS.getCBCH())
		return false;
	if (data.empty() || (data.size() & 1))
		return false;
	std::string buf;
	for (size_t i = 0; i < data.size(); i += 2) {
		unsigned c = 0;
		if (sscanf(data.data() + i,"%2x",&c) != 1)
			return false;
		buf.append(1,(char)c);
	}
	unsigned upd = gCbMessages.remove(id,code);
	LOG(INFO) << "adding CB message id=" << id << " upd=" << upd << " code=" << code
		<< " gs=" << gs << " dcs=" << dcs << " len=" << buf.size();
	gCbMessages.add(CbMessage(id,code,gs,dcs,buf,upd));
	return true;
}


void Control::SMSCBkill(unsigned id, unsigned code)
{
	if (id >= 0xffff) {
		LOG(INFO) << "removing all CB messages!";
		gCbMessages.clear();
	}
	else if (code >= 1024) {
		LOG(INFO) << "removing CB messages with id=" << id;
		gCbMessages.remove(id);
	}
	else {
		LOG(INFO) << "removing CB message id=" << id << " code=" << code;
		gCbMessages.remove(id,code);
	}
}


// vim: ts=4 sw=4
