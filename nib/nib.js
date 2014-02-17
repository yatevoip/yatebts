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

#require "subscribers.js"
#require "libchatbot.js"

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
 
function numberAvailable(val)
{
    if (regUsers["+"+val]==undefined)
	return true;
    return false;
}
 
function newNumber()
{
    var val = goodNumber();
    while (!numberAvailable(val)) {
	val = goodNumber();
	//Engine.debug(Engine.DebugInfo,val);
    }
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
	if (subscribers[imsi].active==1)
	    return subscribers[imsi].msisdn;
	else
	    return false;
    }
    return false;
}

function getSubscriberIMSI(msisdn)
{
    var imsi_key, nr, short_number;

//    Engine.debug(Debug.DebugInfo,"getSubscriberIMSI, msisdn="+msisdn);

    for (imsi_key in subscribers) {
	nr = subscribers[imsi_key].msisdn;
	//Engine.debug("nr="+nr);
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

//    Engine.debug(Debug.DebugInfo,"could not get imsi for nr="+msisdn);
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

    for (imsi_key in subscribers) {
	if (subscribers[imsi_key].short_number==called)
	    return subscribers[imsi_key].msisdn;
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
    for (number in regUsers) {
	imsi = regUsers[number];
	if (number.substr(0,1)=="+")
	    number = number.substr(1);

	//Engine.debug("checking registered user with number "+number);

	if (called.substr(-number.length)==number) {
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
		    Engine.debug(Debug.DebugInfo,"Could not deliver sms from imsi "+sms.imsi+" to number "+sms.dest+".");
		} else
		    Engine.debug(Debug.DebugInfo,"Droped sms from imsi "+sms.imsi+" to number "+sms.dest+". Exceeded attempts.");
	    } else
		Engine.debug(Debug.DebugInfo,"Delivered sms from imsi "+sms.imsi+" to number "+sms.dest);
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
    //Engine.debug(Engine.DebugInfo,"Calling eliza with text='"+msg.text+"' from imsi "+imsi_orig);
    var answer = chatWithBot(msg.text,imsi_orig);

    var sms = {"imsi":"nib_smsc", "msisdn":eliza_number,"smsc":nib_smsc_number, "dest":msisdn, "dest_imsi":imsi_orig, "next_try":try_time, "tries": sms_attempts, "msg":answer};
    pendingSMSs.push(sms);

    return true;
}

// MO SMS handling (since we only deliver locally the MT SMSs are generated by NIB)
function onSMS(msg)
{
    imsi_orig = msg.caller;
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
    
    var sms = {"imsi":imsi_orig, "msisdn":msisdn,"smsc": msg.called, "dest":dest, "dest_imsi":dest_imsi, "next_try":Date.now(), "tries": sms_attempts, "msg":msg.text};
    pendingSMSs.push(sms);

    return true;
}

function message(msg, dest_imsi, dest_msisdn)
{
    var try_time = (Date.now()/1000) + 5;
    var sms = {"imsi":"nib_smsc", "msisdn":nib_smsc_number,"smsc":nib_smsc_number, "dest":dest_msisdn, "dest_imsi":dest_imsi, "next_try":try_time, "tries": sms_attempts, "msg":msg};
    pendingSMSs.push(sms);
}

function sendGreetingMessage(imsi, msisdn)
{
    if (msisdn.substr(0,1)=="+")
	msisdn = msisdn.substr(1);
    var msg_text = "Your allocated phone no. is "+msisdn+". Thank you for installing YateBTS. Call David at david("+david_number+")";

    message(msg_text,imsi,msisdn);
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
	    //Engine.debug("caller_imsi='"+caller_imsi+"'and it's not in our subscribers "+subscribers[caller_imsi]);
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

function onRegister(msg)
{
    var imsi = msg.username;
    if (imsi=="") {
	// this should not happen
	Engine.debug(Engine.DebugWarn,"Got user.register with NULL username(imsi).");
	return false;
    }

    if (subscribers != undefined) {
	Engine.debug(Engine.DebugInfo,"Searching imsi in subscribers.");
	msisdn = getSubscriberMsisdn(imsi);
	if (msisdn==null || msisdn=="") {
	    // check if imsi is already registered so we don't allocate a new number
	    msisdn = alreadyRegistered(imsi);
	    if (msisdn==false) {
		Engine.debug(Engine.DebugInfo,"Located imsi without msisdn. Allocated random number");
		msisdn = newNumber();
		//sendGreetingMessage(imsi, msisdn);
	    }
	} else if (msisdn==false) {
	    addRejected(imsi);
	    msg.error = "forbidden";
	    return false;
	}
    } else {
	if (regexp == undefined) {
	    Engine.debug(Engine.DebugWarn,"Please configure accepted subscribers or regular expression to accept by");

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
		    Engine.debug(Engine.DebugInfo,"Allocated random number");
		    msisdn = newNumber();
		    //sendGreetingMessagie(imsi, msisdn);
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

function onCommand(msg)
{
    // To DO: implement commands

    if (!msg.line) {
	//onComplete(msg,msg.partline,msg.partword);
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

Engine.debugName("nib");
Message.install(onUnregister,"user.unregister",80);
Message.install(onRegister,"user.register",80);
Message.install(onRoute,"call.route",80);
Message.install(onIdleAction,"idle.execute",110,"module","nib_cache");
Message.install(onSMS,"msg.execute",80,"callto","nib_smsc");
Message.install(onCommand,"engine.command",120);

Engine.setInterval(onInterval,1000);
