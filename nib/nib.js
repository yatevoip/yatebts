/**
 * nib.js
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * Copyright (C) 2014 Null Team
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
 * Network-In-a-Box demo
 * To use it put in javascript.conf:
 *
 * [scripts]
 * nib=nib.js
 */

#require "libchatbot.js"

// Helper that returns a left or right aligned fixed length string
function strFix(str,len,pad)
{
    if (str === null)
	str = "";
    if (pad == "")
	pad = " ";
    if (len < 0) {
	// right aligned
	len = -len;
	if (str.length >= len)
	    return str.substr(str.length - len);
    	while (str.length < len)
	    str = pad + str;
    }
    else {
	// left aligned
	if (str.length >= len)
	    return str.substr(0,len);
	while (str.length < len)
	    str += pad;
    }
    return str;
}

function updateSubscribersInfo()
{
    var msisdn, imsi;
    var upd_subscribers = readConfiguration(true);

    if (upd_subscribers.length==0 && regexp!=undefined) {
	// we moved from accepting individual subscribers to accepting them by regexp
	// remove all registered users that don't match new regexp
	
	for (msisdn in regUsers) {
		imsi = regUsers[msisdn];
		if (!imsi.match(regexp))
			delete regUsers[msisdn];
	}
	if (subscribers!=undefined)
	    delete subscribers;
	return; 
    }

    if (subscribers==undefined)
	subscribers = {};

    upd_subscribers = upd_subscribers["subscribers"];
    for (imsi in upd_subscribers)
	updateSubscriber(upd_subscribers[imsi], imsi);

    for (imsi in subscribers) {
	if (subscribers[imsi]["updated"]!=true) {
	    msisdn = alreadyRegistered(imsi);
	    if (msisdn) {
		delete regUsers[msisdn];
	    }
	    delete subscribers[imsi];
	    
	}

	// clear updated field after checking it
	delete subscribers[imsi]["updated"];
    }
}

function updateSubscriber(fields, imsi)
{
    var fields_to_update = ["msisdn", "active", "ki", "op", "imsi_type", "short_number"];

    if (subscribers[imsi]!=undefined) {
	if (subscribers[imsi]["active"]==true && fields["active"]!=true) {
	    // subscriber was deactivated. make sure to delete it from regUsers
	    for (var msisdn in regUsers)
		if (regUsers[msisdn]==imsi)
		    delete regUsers[msisdn];
	} else if (subscribers[imsi]["msisdn"]!=fields["msisdn"]) {
	    // subscriber msisdn was changed. Try to keep registration
	    for (var msisdn in regUsers)
		if (regUsers[msisdn]==imsi) {
		    regUsers[fields["msisdn"]] = regUsers[subscribers[imsi]["msisdn"]];
		    delete regUsers[subscribers[imsi]["msisdn"]];
		}
	}

    }

    if (subscribers[imsi]=="")
	subscribers[imsi] = {};

    for (var field_name of fields_to_update)
	subscribers[imsi][field_name] = fields[field_name];
    
     subscribers[imsi]["updated"] = true;
}

function readConfiguration(return_subscribers)
{
    var configuration = getConfigurationObject("subscribers");
    country_code = configuration["general"]["country_code"];
    if (country_code.length==0)
	Engine.alarm(alarm_conf,"Please configure country code. See subscribers.conf or use the NIB web interface");

    var reg = configuration["general"]["regexp"];
    regexp = new RegExp(reg);
    var upd_subscribers = configuration;

    delete upd_subscribers["general"];

    if (upd_subscribers.length==0 && regexp==undefined)
	Engine.alarm(alarm_conf,"Please configure subscribers or regular expresion to accept registration requests. See subscribers.conf or use the NIB web interface");

    var imsi, active, count=0;
    for (var imsi in upd_subscribers) {
	active = upd_subscribers[imsi].active;
	active = active.toLowerCase();
	if (active=="1" || active=="on" || active=="enable" || active=="yes")
	    upd_subscribers[imsi].active = true;
	else
	    upd_subscribers[imsi].active = false;
	count++;
    }

    if (return_subscribers) {
	var ret = {"subscribers":upd_subscribers, "length":count};
	return ret;
    }

    if (count>0)
    	subscribers = upd_subscribers;
}

function getConfigurationObject(file)
{
    if (file.substr(-4)!=".conf")
	file = Engine.configFile(file);

    var conf = new ConfigFile(file,true);
    var sections = conf.sections();
    var section, section_name, prop_name, keys;
    var configuration = {};

    for (section_name in sections) {
	section = conf.getSection(section_name);
	configuration[section_name] = {};
	keys = section.keys();
	for (prop_name of keys)
	    configuration[section_name][prop_name] = section.getValue(prop_name);
    }
    return configuration;
}
 
function randomint(modulus)
{
    if (randomint.count==undefined) {
	var d = new Date();
	randomint.count = d.getSeconds()*1000 + d.getMilliseconds();
    }
    randomint.count++;
    // Knuth's integer hash.
    var hash =(randomint.count * 2654435761) % 4294967296;
    return hash % modulus;
}

function rand32()
{
    var tmp = Math.random();
    return strFix(tmp.toString(16),-8,'0');
}
 
function rand128()
{
    return rand32() + rand32() + rand32() + rand32();
}
 
function numberAvailable(val)
{
    if (regUsers["+"+val]==undefined)
	return true;
    return false;
}
 
function newNumber(imsi)
{
    if (country_code.length==0) {
	Engine.alarm(alarm_conf,"Please configure country code. See subscribers.conf or use the NIB web interface.");
	country_code = "";
    }

    if (imsi=="") 
    	var val = country_code + goodNumber();
    else
	// create number based on IMSI. Try to always generate same number for same IMSI
	var val = country_code + imsi.substr(-7);

    while (!numberAvailable(val))
	val = country_code + goodNumber();

    return val;
}

function goodNumber()
{
    var An = 2 + randomint(8);
    var A = An.toString();
    var Bn = randomint(10);
    var B = Bn.toString();
    var Cn = randomint(10);
    var C = Cn.toString();
    var Dn = randomint(10);
    var D = Dn.toString();
    var En = randomint(10);
    var E = En.toString();
 
    switch (randomint(25)) {
	// 4 digits in a row - There are 10,000 of each.
	case 0: return A+B+C+D+D+D+D;
	case 1: return A+B+C+C+C+C+D;
	case 2: return A+B+B+B+B+C+D;
	case 3: return A+A+A+A+B+C+D;
	// ABCCBA palidromes - There are about 10,000 of each.
	case 4: return A+B+C+C+B+A+D;
	case 5: return A+B+C+D+D+C+B;
	// ABCABC repeats - There are about 10,000 of each.
	case 6: return A+B+C+A+B+C+D;
	case 7: return A+B+C+D+B+C+D;
	case 8: return A+B+C+D+A+B+C;
	// AABBCC repeats - There are about 10,000 of each.
	case 9: return A+A+B+B+C+C+D;
	case 10: return A+B+B+C+C+D+D;
	// AAABBB repeats - About 1,000 of each.
	case 11: return A+A+A+B+B+B+C;
	case 12: return A+A+A+B+C+C+C;
	case 13: return A+B+B+B+C+C+C;
	// 4-digit straights - There are about 1,000 of each.
	case 14: return "2345"+B+C+D;
	case 15: return "3456"+B+C+D;
	case 16: return "4567"+B+C+D;
	case 17: return "5678"+B+C+D;
	case 18: return "6789"+B+C+D;
	case 19: return A+B+C+"1234";
	case 20: return A+B+C+"2345";
	case 21: return A+B+C+"3456";
	case 22: return A+B+C+"4567";
	case 23: return A+B+C+"5678";
	case 24: return A+B+C+"6789";
    }
}

function getRegUserMsisdn(imsi)
{
    var msisdn_key;

    for (msisdn_key in regUsers)
	if (regUsers[msisdn_key] == imsi)
	    return msisdn_key;
    return false;
}

function alreadyRegistered(imsi)
{
    return getRegUserMsisdn(imsi);
}

function getSubscriberMsisdn(imsi)
{
    if (subscribers[imsi]!=undefined) {
	// what happens if user didn't set msisdn ??
	if (subscribers[imsi].active)
	    return subscribers[imsi].msisdn;
	else
	    return false;
    }
    return false;
}

function getSubscriberIMSI(msisdn)
{
    var imsi_key, nr, short_number;

    for (imsi_key in subscribers) {
	nr = subscribers[imsi_key].msisdn;
	if (nr.length>0) {
	    // strip + from start of number
	    if (nr.substr(0,1)=="+")
		nr = nr.substr(1);

	    //if (nr==msisdn || nr.substr(0,msisdn.length)==msisdn || (msisdn.substr(0,1)=="+" && msisdn.substr(1,nr.length)==nr))
	    if (msisdn.substr(-nr.length)==nr)
		return imsi_key;
	}

	short_number = subscribers[imsi_key].short_number;
	if (short_number.length>0) 
	    if (short_number==msisdn)
		return imsi_key;
    }

    return false;
}

function getRouteToMsisdn(called)
{
    if (called.substr(0,1)=="+")
	return "ybts/"+called;

    return "ybts/+"+called;
}

function getRouteToIMSI(imsi)
{
    return "ybts/IMSI"+imsi;
}

function updateCaller(msg)
{
    var caller = msg.caller;

    // if caller is in IMSI format then find it's MSISDN
    if (caller.match(/IMSI/)) {
	imsi = caller.substr(4);
	if (subscribers) {
	    var caller_msisdn = getSubscriberMsisdn(imsi);
	    if (caller_msisdn!=false)
		msg.caller = caller_msisdn;
	    else {
		// this should not happen
		// if I have subscribers and caller is in IMSI format then he should be in our subscribers list
		msg.error = "forbidden";
		return false;
	    }
	} else {
	    // user should be registered so get msisdn from registered users
	    caller_msisdn = getRegUserMsisdn(imsi);
	    if (caller_msisdn==false) {
		// user is not registered. What now?
		msg.error = "forbidden";
		return false;
	    }
	}
	if (caller_msisdn.substr(0,1)=="+")
		caller_msisdn = caller_msisdn.substr(1);
	msg.caller = caller_msisdn;
    }
    return true;
}

function isShortNumber(called)
{
    if (!subscribers)
	// subscribers is not defined so we don't have short numbers
	return called;

    for (var imsi_key in subscribers) {
	if (subscribers[imsi_key].short_number==called) {
	    var msisdn = subscribers[imsi_key].msisdn;
	    if (msisdn.length>0)
		return msisdn;
	    msisdn = getRegUserMsisdn(imsi_key);
	    if (msisdn!=false)
		return msisdn;
	    // this is the short number for an offline user
	    // bad luck
	}
    }

    return called;
}

function routeOutside(msg,called)
{
    if (called.substr(0,1)!="+")
	msg.called = "+"+msg.called;

    msg.line = "outbound";
    msg.retValue("line/"+msg.called);

    return true;
}

function routeToRegUser(msg,called)
{
    for (var number in regUsers) {
	imsi = regUsers[number];
	if (number.substr(0,1)=="+")
	    number = number.substr(1);

	if (called.substr(-number.length)==number || number.substr(-called.length)==called) {
	    msg.retValue(getRouteToIMSI(imsi));
	    return true;
	}
    }
    return false;
}

// Run expiration and retries
function onInterval()
{
    var when = Date.now() / 1000;
    if (onInterval.nextIdle >= 0 && when >= onInterval.nextIdle) {
	onInterval.nextIdle = -1;
	var m = new Message("idle.execute");
	m.module = "nib_cache";
	if (!m.enqueue())
	    onInterval.nextIdle = when + 5;
    }
}

// Execute idle loop actions
function onIdleAction()
{
    // check if we have any SMSs to send
    if (pendingSMSs.length>0) {
	// check if sms from first position is ready to be delivered
	if (pendingSMSs[0].next_try<=Date.now()) {
	    // retrieve sms from first position and remove it
	    var sms = pendingSMSs.shift();
	    res = moLocalDelivery(sms);
	    if (res==false) {
		sms.tries = sms.tries - 1;
		if (sms.tries>=0) {
		    // if number of attempts to deliver wasn't excedeed push it on last position
		    // add sms at the end of pending SMSs
		    sms.next_try = (Date.now() / 1000) + 5; // current time + 5 seconds 
		    pendingSMSs.push(sms);
		    Engine.debug(Engine.DebugInfo,"Could not deliver sms from imsi "+sms.imsi+" to number "+sms.dest+".");
		} else
		    Engine.debug(Engine.DebugInfo,"Droped sms from imsi "+sms.imsi+" to number "+sms.dest+". Exceeded attempts.");
	    } else
		Engine.debug(Engine.DebugInfo,"Delivered sms from imsi "+sms.imsi+" to number "+sms.dest);
	}
    }

    // Reschedule after 5s
    onInterval.nextIdle = (Date.now() / 1000) + 5;
}

// Deliver SMS to registered MS in MT format
function moLocalDelivery(sms)
{
    var m = new Message("msg.execute");
    m.caller = sms.smsc;
    m.called = sms.dest;
    m["sms.caller"] = sms.msisdn;
    if (sms.msisdn.substr(0,1)=="+") {
	m["sms.caller.nature"] = "international";
	m["sms.caller"] = sms.msisdn.substr(1);
    }
    m.text= sms.msg;
    m.callto = getRouteToIMSI(sms.dest_imsi);
    if (m.dispatch())
	return true;
    return false;
}

function onRouteSMS(msg)
{
    Engine.debug(Engine.DebugInfo,"onSMS " + msg.caller + " -> " + msg["sms.called"]);

    if (msg.caller=="" || msg.called=="" || msg["sms.called"]=="")
	return false;

    // usually we should check if it's for us: msg.called = nib_smsc_number

    // NIB acts as smsc
    msg.retValue("nib_smsc");
    return true; 
}

function onElizaSMS(msg, imsi_orig, dest, msisdn)
{
    var answer = chatWithBot(msg.text,imsi_orig);

    var sms = {"imsi":"nib_smsc", "msisdn":eliza_number,"smsc":nib_smsc_number, "dest":msisdn, "dest_imsi":imsi_orig, "next_try":try_time, "tries": sms_attempts, "msg":answer};
    pendingSMSs.push(sms);

    return true;
}

// MO SMS handling (since we only deliver locally the MT SMSs are generated by NIB)
function onSMS(msg)
{
    imsi_orig = msg.username;
    if (imsi_orig.match(/IMSI/))
	imsi_orig = imsi_orig.substr(4);
    // user must be registered
    var msisdn = getRegUserMsisdn(imsi_orig);
    if (msisdn==false) {
	// imsi is not registered
	msg.error = "forbidden";
	return true;
    }

    dest = msg["sms.called"];

    if (dest==eliza_number)
	return onElizaSMS(msg, imsi_orig, dest, msisdn);

    // dest should be one of our users. we only deliver locally

    // check if short number was used
    dest = isShortNumber(dest);
    dest_imsi = getSubscriberIMSI(dest);

    if (dest_imsi==false) {
	msg.error = "no route";
	return true;
    }
    
    var sms = {"imsi":imsi_orig, "msisdn":msisdn,"smsc": msg.called, "dest":dest, "dest_imsi":dest_imsi, "next_try":Date.now(), "tries": sms_attempts, "msg":msg.text };
    pendingSMSs.push(sms);

    return true;
}

function message(msg, dest_imsi, dest_msisdn, wait_time)
{
    if (wait_time==null)
	wait_time = 5;

    var try_time = (Date.now()/1000) + wait_time;
    var sms = {"imsi":"nib_smsc", "msisdn":nib_smsc_number,"smsc":nib_smsc_number, "dest":dest_msisdn, "dest_imsi":dest_imsi, "next_try":try_time, "tries": sms_attempts, "msg":msg};
    pendingSMSs.push(sms);
}

function sendGreetingMessage(imsi, msisdn)
{
    if (msisdn.substr(0,1)=="+")
	msisdn = msisdn.substr(1);
    var msg_text = "Your allocated phone no. is "+msisdn+". Thank you for installing YateBTS. Call David at david("+david_number+")";

    message(msg_text,imsi,msisdn,9);
}

function onRoute(msg)
{
    var route_type = msg.route_type;
    if (route_type=="msg")
	return onRouteSMS(msg);
    if (route_type!="" && route_type!="call")
	return false;

    Engine.debug(Engine.DebugInfo,"onRoute " + msg.caller + " -> " + msg.called);

    var called = msg.called;
    var caller = msg.caller;
    var caller_imsi = msg.username;
    if (caller.substr(0,1)=="+") {
	msg.caller = caller.substr(1);
	msg.callernumtype = "international";
    }

    // rewrite caller param to msisdn in case it's in IMSI format
    if (!updateCaller(msg))
	return true;

    if (called.substr(-david_number.length)==david_number) {
	// this should have been handled by welcome.js
	return false;
    }

    if (called=="333" || called=="+333") {
	// call to conference number
	msg.retValue("conf/333");
	return true;
    }

    called = isShortNumber(called);

    if (routeToRegUser(msg,called)==true)
	return true;

    if (subscribers) {
	if (getSubscriberIMSI(called)!=false) {
	    // called is in our users but it's not registered
	    msg.error = "offline";
	    return true;
	}

	// call seems to be for outside
	// must make sure caller is in our subscribers

	if (caller_imsi=="" || subscribers[caller_imsi]==undefined) {
	    // caller is not in our subscribers
	    msg.error = "forbidden";
	    return true;
	}

	return routeOutside(msg);
    }

    // registration is accepted based on IMSI regexp
    // check that caller is registered and the routeOutside

    if (getRegUserMsisdn(caller_imsi)!=false)
	return routeOutside(msg);

    // caller is not registered to us and is trying to call outside NIB
    msg.error = "forbidden";
    return true;
}

function addRejected(imsi)
{
    if (seenIMSIs[imsi]==undefined)
	seenIMSIs[imsi] = 1;
    else
	seenIMSIs[imsi] = seenIMSIs[imsi]+1;
}

function authResync(msg,imsi,is_auth)
{
    var ki   = subscribers[imsi].ki;
    var op   = subscribers[imsi].op;
    var sqn  = subscribers[imsi].sqn;
    var rand = msg.rand;
    var auts = msg.auts;

    var m = new Message("gsm.auth");
    m.protocol = "milenage";
    m.ki = ki;
    m.op = op;
    m.rand = rand;
    m.auts = auts;
    m.dispatch(true);
    var ns = m.sqn;
    if (ns != "") {
	Engine.debug(Engine.DebugInfo,"Re-sync " + imsi + " by SQN " + sqn + " -> " + ns);
	// Since the synchronized sequence was already used increment it
	sqn = 0xffffffffffff & (0x20 + parseInt(ns,16));
	sqn = strFix(sqn.toString(16),-12,'0');
	subscribers[imsi].sqn = sqn;

	// continue with Authentication now
	return startAuth(msg,imsi,is_auth);
    }
    else {
	Engine.debug(Engine.DebugWarn,"Re-sync " + imsi + " failed, SQN " + sqn);
	return false;
    }
}

function startAuth(msg,imsi,is_auth)
{
    var ki = subscribers[imsi].ki;
    var op = subscribers[imsi].op;
    var imsi_type = subscribers[imsi].imsi_type;
    var sqn = subscribers[imsi].sqn;

    if (sqn == "")
	sqn = "000000000000";

    if (ki=="" || ki==null) {
	Engine.alarm(alarm_conf, "Please configure ki and imsi_type in subscribers.conf. You can edit the file directly or use the NIB interface. If you don't wish to authenticate SIMs, please configure regexp instead of individual subscribers, but in this case MT authentication can't be used -- see [security] section in ybts.conf.");
	msg.error = "unacceptable";
	return false;
    }

    if (imsi_type=="3G") {
	rand = rand128();
	var m = new Message("gsm.auth");
	m.protocol = "milenage";
	m.ki = ki;
	m.op = op;
	m.rand = rand;
	m.sqn = sqn;
	if (!m.dispatch(true)) {
	    msg.error = "failure";
	    return false;
	}
	// Increment the sequence without changing index
	sqn = 0xffffffffffff & (0x20 + parseInt(sqn,16));
	sqn = strFix(sqn.toString(16),-12,'0');

	// remember xres and sqn
	subscribers[imsi]["xres"] = m.xres;
	subscribers[imsi]["sqn"] = sqn;
	// Populate message with auth params
	msg["auth.rand"] = rand;
	msg["auth.autn"] = m.autn;
	msg.error = "noauth";
	if (is_auth)
	    return true;
	return false;
    } 
    else {
	rand = rand128();
	var m = new Message("gsm.auth");
	m.protocol = "comp128";
	m.ki = ki;
	m.op = op;
	m.rand = rand;
	if (!m.dispatch(true)) {
	    msg.error = "failure";
	    return false;
	}
	// remember sres
	subscribers[imsi]["sres"] = m.sres;
	// Populate message with auth params
	msg["auth.rand"] = rand;
	msg.error = "noauth";
	if (is_auth)
	    return true;
	return false;
    }
}

function checkAuth(msg,imsi,is_auth)
{
    var res;

    if (subscribers[imsi]["imsi_type"]=="3G")
	res = subscribers[imsi]["xres"];
    else
	res = subscribers[imsi]["sres"];

    var upper = msg["auth.response"];
    upper = upper.toUpperCase();

    if (res != upper)
	// it's safe to do this directly, ybts module makes sure this can't start a loop
	return startAuth(msg,imsi,is_auth);
    return true;
}

function onAuth(msg)
{
    var imsi = msg.imsi;

    if (msg["auth.response"]=="" && msg["error"]=="" && msg["auth.auts"]=="") {
	return startAuth(msg,imsi,true);
    } 
    else if (msg["auth.response"]!="") {
	if (checkAuth(msg,imsi,true)==false)
	    return false;
	return true;
    }
    else if (msg["auth.auts"]) {
	return authResync(msg,imsi,true);
    }	

    return false; 
}

function onRegister(msg)
{
    var imsi = msg.username;
    var posib_msisdn = msg.msisdn;

    if (imsi=="") {
	// this should not happen
	Engine.debug(Engine.DebugWarn,"Got user.register with NULL username(imsi).");
	return false;
    }

    if (subscribers != undefined) {
	Engine.debug(Engine.DebugInfo,"Searching imsi in subscribers.");
	msisdn = getSubscriberMsisdn(imsi);

	if (subscribers[imsi] != "") {
	    // start authentication procedure if it was not started
	    if (msg["auth.response"]=="" && msg["error"]=="" && msg["auth.auts"]=="") {
		return startAuth(msg,imsi);
	    }
	    else if (msg["auth.response"]!="") {
		if (checkAuth(msg,imsi)==false)
		    return false;
	    } 
	    else if (msg["auth.auts"]) {
		return authResync(msg,imsi);
	    }
	    else
		return false;
	}

	if (msisdn==null || msisdn=="") {
	    // check if imsi is already registered so we don't allocate a new number
	    msisdn = alreadyRegistered(imsi);
	    if (msisdn==false) {

		if (posib_msisdn!="")
		    if (numberAvailable(posib_msisdn))
			msisdn = posib_msisdn;

		if (msisdn==false) {
		    Engine.debug(Engine.DebugInfo,"Located imsi without msisdn. Allocated random number");
		    msisdn = newNumber(imsi);
		    //sendGreetingMessage(imsi, msisdn);
		}
	    }
	} else if (msisdn==false) {
	    addRejected(imsi);
	    msg.error = "forbidden";
	    return false;
	}
    } else {
	if (regexp == undefined) {
	    Engine.alarm(alarm_conf,"Please configure accepted subscribers or regular expression to accept by.");

	    // maybe reject everyone until system is configured ??
	    // addRejected(imsi);
	    msg.error = "forbidden";
	    return false;
	} else {
	    // check that imsi is valid against regexp

	    if (imsi.match(regexp)) {
		// check if imsi is already registered so we don't allocate a new number
		msisdn = alreadyRegistered(imsi);
		if (msisdn==false) {
		    if (posib_msisdn!="")
			if (numberAvailable(posib_msisdn))
			     msisdn = posib_msisdn;

		    if (msisdn==false) {
		        Engine.debug(Engine.DebugInfo,"Allocated random number");
		        msisdn = newNumber(imsi);
		        //sendGreetingMessagie(imsi, msisdn);
		    }
		}
	    } else {
		addRejected(imsi);
		msg.error = "forbidden";
		return false;
	    }
	}
    }

    if (alreadyRegistered(imsi)==false)
	sendGreetingMessage(imsi, msisdn);
    
    if (msisdn.substr(0,1)!="+")
	msisdn = "+"+msisdn;

    // if we do this, in the future it will be copied in call.route messages(after this is implemented in ybts.cpp)
    msg.msisdn = msisdn;

    regUsers[msisdn] = imsi;
    Engine.debug(Engine.DebugInfo,"Registered imsi "+imsi+" with number "+msisdn);
    return true;
}

function onUnregister(msg)
{
    var imsi = msg.username;
    if (imsi=="")
	return false;

    msisdn = msg.msisdn;
    if (msisdn=="") {
	msisdn = getRegUserMsisdn(imsi);
	if (msisdn==false)
	    // user doesn't seem to be registered, weird
	    return true;
    }
    if (msisdn.substr(0,1)!="+")
	msisdn = "+"+msisdn;   
    delete regUsers[msisdn];

    Engine.debug(Engine.DebugInfo,"Unregistered imsi "+imsi+" with msisdn "+msisdn);
    return true;
}

// Perform one command line completion
function oneCompletion(msg,str,part)
{
	if (part != "" && str.indexOf(part) != 0)
		return;
	var ret = msg.retValue();
	if (ret != "")
		ret += "\t";
	msg.retValue(ret + str);
}

function onHelp(msg)
{
    var ret = msg.retValue();
    msg.retValue(ret+"  nib {list|reload}\r\n");
}

function onComplete(msg, line, partial, part)
{
    switch (line) {
	case "nib":
	    oneCompletion(msg,"list",part);
	    oneCompletion(msg,"reload",part);
	    return;
	case "nib list":
	    oneCompletion(msg,"registered",part);
	    oneCompletion(msg,"sms",part);
	    oneCompletion(msg,"rejected",part);
	    return;
    }

    switch (partial) {
	case "n":
	case "ni":
	    oneCompletion(msg,"nib",part);
	    break;
    }
}

function onCommand(msg)
{
    if (!msg.line) {
	onComplete(msg,msg.partline,msg.partial,msg.partword);
	return false;
    }
    switch (msg.line) {
	case "nib list registered":
	    var tmp = "IMSI            MSISDN \r\n";
	    tmp += "--------------- ---------------\r\n";
	    for (var msisdn_key in regUsers)
		tmp += regUsers[msisdn_key]+"   "+msisdn_key+"\r\n";
	    msg.retValue(tmp);
	    return true;

	case "nib list sms":
	    var tmp = "FROM_IMSI        FROM_MSISDN        TO_IMSI        TO_MSISDN\r\n";
	    tmp += "--------------- --------------- --------------- ---------------\r\n";
	    for (var i=0; i<pendingSMSs.length; i++)
		tmp += pendingSMSs[i].imsi+"   "+pendingSMSs[i].msisdn+"   "+pendingSMSs[i].dest_imsi+"   "+pendingSMSs[i].dest+"\r\n";
	    msg.retValue(tmp);
	    return true;

	case "nib list rejected":
	    var tmp = "IMSI            No attempts register \r\n";
	    tmp += "--------------- ---------------\r\n";
	    for (var imsi_key in seenIMSIs)
		tmp += imsi_key+"    "+seenIMSIs[imsi_key]+"\r\n";
	    msg.retValue(tmp);
	    return true;

	case "nib reload":
	    updateSubscribersInfo();
	    msg.retValue("Finished updating subscribers.\r\n");
	    return true;
    }

    return false;
}

var eliza_number = "35492";
var david_number = "32843";
var nib_smsc_number = "12345";
var sms_attempts = 3;
var regUsers = {};
var pendingSMSs = [];
var seenIMSIs = {};  // imsi:count_rejected
var ussd_sessions = {};
var alarm_conf = 3;

Engine.debugName("nib");
Message.install(onUnregister,"user.unregister",80);
Message.install(onRegister,"user.register",80);
Message.install(onRoute,"call.route",80);
Message.install(onIdleAction,"idle.execute",110,"module","nib_cache");
Message.install(onSMS,"msg.execute",80,"callto","nib_smsc");
Message.install(onCommand,"engine.command",120);
Message.install(onHelp,"engine.help",120);
Message.install(onAuth,"auth",80);

Engine.setInterval(onInterval,1000);
readConfiguration();
