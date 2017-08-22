/**
 * custom_sms.js
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * Copyright (C) 2015 Null Team
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

/*
 * Custom SMS 
 * To use it put in javascript.conf:
 *
 * [scripts]
 * custom_sms=custom_sms.js
 */

function onControl(msg)
{
	if (!msg.called && (!msg.text || !msg.rpdu)) {
		msg.retValue("Missing IMSI or Message. The SMS will not be sent.");
		msg["operation-status"] = false;
		return true;
	}

	var m = new Message("msg.execute");
	m.caller = "12345";
	m.called = msg.called;
	m["sms.caller"] = "12345";
	if (msg.text)
		m.text = msg.text;
	else if (msg.rpdu)	
		m.rpdu = msg.rpdu;

	m.callto = "ybts/IMSI"+msg.called;
	msg["operation-status"] = m.dispatch();
	return true;
}

Engine.debugName("custom_sms");
Message.install(onControl,"chan.control",80,"component","custom_sms");
