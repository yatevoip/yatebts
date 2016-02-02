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


// -------------------------------------------------------------------
// !FUNCTIONS IN THIS AREA COULD/SHOULD BE MOVED TO A LIBRARY

/*
 * Read registered subscribers and last allocated tmsi. Should be called when script is started.
 * Function calls other functions based on tmsi_storage configuration to read users from various mediums: conf file/db
 */
function readUEs()
{
    if (tmsi_storage==undefined || tmsi_storage=="conf")
	readUEsFromConf();
}

/*
 * Read registered subscribers from tmsidata.conf configuration file and last allocated tmsi
 */
function readUEsFromConf()
{
    conf = new ConfigFile(Engine.configFile("tmsidata"),true);

    tmsisection = conf.getSection("tmsi",true);
    last_tmsi = tmsisection.getValue("last");

    ues = conf.getSection("ues",true);
    var subscriber_info, imsi;
    registered_subscribers = {};

    keys = ues.keys();
    var count_ues = 0;
    for (imsi of keys) {
	// Ex   registered user:226030182676743=000000bd,354695033561290,40121212121,1401097352,ybts/TMSI000000bd
	// Ex unregistered user:226030182676743=000000bd,354695033561290,40121212121,1401097352,
	// imsi=tmsi,imei,msisdn,expires,location
	subscriber_info = ues.getValue(imsi);
	subscriber_info = subscriber_info.split(",");
	var tmsi = subscriber_info[0];
	var imei = subscriber_info[1];
	var msisdn = subscriber_info[2];
	var expires = subscriber_info[3];
	expires = parseInt(expires);
	var loc = subscriber_info[4];
	registered_subscribers[imsi] = {"tmsi":tmsi,"imei":imei,"msisdn":msisdn,"expires":expires,"location":loc};
	count_ues = count_ues+1;
    }

    Engine.debug(Engine.DebugInfo, "Finished reading saved registered subscribers. Found "+count_ues+" registered_subscribers.");
}

/*
 * Check if modification were actually made and save modifications to registered subscribers storage
 * Function calls other functions based on tmsi_storage configuration to write registered_subscribers to various mediums: conf file/db
 * @param imsi String
 * @param subscriber Object/undefined. If undefined them subscriber must be deleted
 */
function saveUE(imsi,subscriber)
{
    if (subscriber!=undefined) {
	var regsubscriber = registered_subscribers[imsi];
	if (subscriberEqual(subscriber,regsubscriber)==true) {
	    Engine.debug(Engine.DebugInfo, "No change when updating subscriber info for IMSI "+imsi);
	    return;
	}
	registered_subscribers[imsi] = subscriber;
    } else
	delete registered_subscribers[imsi];

    if (tmsi_storage==undefined || tmsi_storage=="conf")
	saveUEinConf(imsi,subscriber)
}

function subscriberEqual(obj1, obj2)
{
    var keys = ["tmsi", "msisdn", "imei", "expires", "location"];
    for (var key of keys) {
	if (obj1[key]!=obj2[key])
	    return false;
    }
    return true;
}

/*
 * Save modifications to registered_subscribers in tmsidata.conf configuration file 
 * @param imsi String
 * @param subscriber Object/undefined. If undefined them subscriber must be deleted
 */
function saveUEinConf(imsi,subscriber)
{
    if (subscriber!=undefined) {
	var fields = subscriber["tmsi"]+","+subscriber["imei"]+","+subscriber["msisdn"]+","+subscriber["expires"]+","+subscriber["location"];
	conf.setValue(ues,imsi,fields);
    } else
	conf.clearKey(ues,imsi);

    if (conf.save()==false)
	Engine.alarm(4, "Could not save tmsi in tmsidata.conf");
}

function saveTMSI(tmsi)
{
    if (tmsi_storage==undefined || tmsi_storage=="conf")
	saveTMSIinConf(tmsi);
}

function saveTMSIinConf(tmsi)
{
    conf.setValue(tmsisection,"last",tmsi);
    if (conf.save()==false)
	Engine.alarm(4, "Could not save last tmsi in tmsidata.conf");
}

/*
 * Update subscriber after registration in registered_subscribers in memory and on storage medium
 * @param msg Object where to take subscriber parameters from
 * @param loc String location where calls/smss should be routed for this subscriber
 */
function registerSubscriber(msg,loc)
{
    var imsi = msg.imsi;
    if (imsi=="") {
	// this should not happen. If it does => BUG
	Engine.debug(Engine.DebugWarn, "ERROR: got registerSubscriber with msg without imsi. tmsi='"+msg.tmsi+"'");
	return;
    }

    var imei = msg.imei;
    if (imei=="" && registered_subscribers[imsi]!=undefined)
	imei = registered_subscribers[imsi]["imei"];

    var expire_subscriber = Date.now()/1000 + imsi_cleanup;
    var subscriber = {"tmsi":msg.tmsi, "msisdn":msg.msisdn, "imei":imei, "expires":expire_subscriber};
    if (loc==undefined)
	subscriber["location"] = "ybts/TMSI"+msg.tmsi;
    else
	subscriber["location"] = loc;

    saveUE(imsi,subscriber);
}

function unregisterSubscriber(imsi)
{
    if (registered_subscribers[imsi]=="")
	return;

    var tmsi = registered_subscribers[imsi]["tmsi"];
    var msisdn = registered_subscribers[imsi]["msisdn"];
    var imei = registered_subscribers[imsi]["imei"];
    var expires = registered_subscribers[imsi]["expires"];
    var subscriber = {"tmsi":tmsi, "msisdn":msisdn, "imei":imei, "expires":expires};

    saveUE(imsi,subscriber);
}

/**
 * Make sure both imsi and tmsi are set in routing messages
 * @param msg Object represent the routing message
 * @param imsi String
 */
function addRoutingParams(msg,imsi)
{
    msg.imsi = imsi;
    msg.tmsi = subscribers[imsi]["tmsi"];
}

function initNnsf()
{
    if (nnsf_bits > 0 && nnsf_bits <= 10) {
	nnsf_node &= 0x03ff >> (10 - nnsf_bits);
	nnsf_node_mask = (0xffc000 << (10 - nnsf_bits)) & 0xffc000;
	nnsf_node_shift = nnsf_node << (24 - nnsf_bits);
	nnsf_local_mask = 0xffffff >> nnsf_bits;
    }
    else {
	nnsf_bits = 0;
	nnsf_node = 0;
	nnsf_node_mask = 0;
	nnsf_node_shift = 0;
	nnsf_local_mask = 0xffffff;
    }
}

// Create a candidate new TMSI taking into account node selection function
function newTmsi()
{
    var t = last_tmsi;
    if (t)
	t = 1 * parseInt(t,16);
    else
	t = 0;
    if (nnsf_bits > 0)
	t = ((t & 0xff000000) >> nnsf_bits) | (t & nnsf_local_mask);
    t++;
    if (nnsf_bits > 0)
	t = ((t << nnsf_bits) & 0xff000000) | nnsf_node_shift | (t & nnsf_local_mask);
    if ((t & 0xc0000000) == 0xc0000000)
	t = nnsf_node_shift + 1;
    return last_tmsi = t.toString(16,8);
}

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

// -------------------------------------------------------------------------------

function updateSubscribersInfo()
{
    var imsi;
    var upd_subscribers = readConfiguration(true);

    if (upd_subscribers.length==0 && regexp!=undefined) {
	// we moved from accepting individual subscribers to accepting them by regexp
	// remove all registered users that don't match new regexp

	for (imsi in registered_subscribers) {
	    if (!imsi.match(regexp))
		unregisterSubscriber(imsi);
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
	    if (alreadyRegistered(imsi)) 
		unregisterSubscriber(imsi);

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
	    // subscriber was deactivated. make sure to clean location from registered_subscribers
	    if (registered_subscribers[imsi]!="" && registered_subscribers[imsi]["location"]!="") 
		unregisterSubscriber(imsi);
	} else if (subscribers[imsi]["msisdn"]!=fields["msisdn"]) {
	    // subscriber msisdn was changed. Try to keep registration
	    if (registered_subscribers[imsi]!="") {
		var subscriber = registered_subscribers[imsi];
		subscriber["msisdn"] = fields["msisdn"];
		saveUE(imsi,subscriber);
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
    gw_sos = configuration["general"]["gw_sos"];

    var reg = configuration["general"]["regexp"];
    regexp = new RegExp(reg);
    var upd_subscribers = configuration;

    if (configuration["general"]["nnsf_bits"]!="")
	nnsf_bits = configuration["general"]["nnsf_bits"];
    else
	nnsf_bits = def_nnsf_bits;

    if (configuration["general"]["nnsf_node"]!="")
	nnsf_node = configuration["general"]["nnsf_node"];
    else
	nnsf_node = def_nnsf_node;
    initNnsf();

    var ybts_conf = new ConfigFile(Engine.configFile("ybts"),true);
    imsi_cleanup = ybts_conf.getValue("ybts","tmsi_expire",864000); // 3600 * 24 * 10
    imsi_cleanup = parseInt(imsi_cleanup);

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
 
// Allocate an unused TMSI
function allocTmsi()
{
    var imsi_key, tmsi, in_use;

    for (;;) {
	tmsi = newTmsi();
	in_use = false;
	for (imsi_key in registered_subscribers) {
	    if (registered_subscribers[imsi_key]==tmsi) {
		in_use = true;
		break;
	    }
	}
	if (in_use)
	    continue;
	saveTMSI(tmsi);
	break;
    }
    return tmsi;
}
 
function numberAvailable(val,imsi)
{
    var number;
    for (var imsi_key in registered_subscribers) {
	number = registered_subscribers[imsi_key]["msisdn"];
	if (number.substr(0,1)=="+")
	    number = number.substr(1);

	if (number==val) {
	    if (imsi!=undefined && imsi==imsi_key)
		// keep numbers already associated
		return true;
	    else
	    	return false;
	}
    }
    return true;
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
    if (registered_subscribers[imsi]!="")
	if (registered_subscribers[imsi]["location"]!="")
	    return registered_subscribers[imsi]["msisdn"];

    return false;
}

function alreadyRegistered(imsi)
{
    return getRegUserMsisdn(imsi);
}

function getSubscriberMsisdn(imsi)
{
    if (subscribers[imsi]!=undefined) {
	if (subscribers[imsi].active)
	    return subscribers[imsi].msisdn;
	else
	    return false;
    }
    return false;
}

function getSubscriberIMSI(msisdn,tmsi)
{
    var imsi_key, nr, short_number, user_tmsi;

    if (msisdn) {
	if (subscribers) {
	    for (imsi_key in subscribers) {
		nr = subscribers[imsi_key]["msisdn"];
		if (nr.length>0) {
		    // strip + from start of number
		    if (nr.substr(0,1)=="+")
			nr = nr.substr(1);

		    //if (nr==msisdn || nr.substr(0,msisdn.length)==msisdn || (msisdn.substr(0,1)=="+" && msisdn.substr(1,nr.length)==nr))
		    if (msisdn.substr(-nr.length)==nr)
			return imsi_key;
	        }

		short_number = subscribers[imsi_key]["short_number"];
		if (short_number.length>0) 
		    if (short_number==msisdn)
			return imsi_key;
	    }
	}

	for (imsi_key in registered_subscribers) {
	    nr = registered_subscribers[imsi_key]["msisdn"];
	    if (nr.length>0) {
		// strip + from start of number
		if (nr.substr(0,1)=="+")
		    nr = nr.substr(1);

		//if (nr==msisdn || nr.substr(0,msisdn.length)==msisdn || (msisdn.substr(0,1)=="+" && msisdn.substr(1,nr.length)==nr))
		if (msisdn.substr(-nr.length)==nr)
		    return imsi_key;
	    }
	}

    }
    else if (tmsi) {
	for (imsi_key in registered_subscribers) {
	    user_tmsi = registered_subscribers[imsi_key].tmsi;
	    if (user_tmsi==tmsi) {
		return imsi_key;
	    }
	}
    }

    return false;
}

function updateCaller(msg,imsi)
{
    var caller = msg.caller;

    // if caller is in IMSI format then find it's MSISDN
    if (caller.match(/IMSI/)) {
	if (registered_subscribers[imsi]!="")
	    msg.caller = registered_subscribers[imsi]["msisdn"];
	else {
	    // user is not registered
	    msg.error = "service-unavailable";
	    return false;
	}
    } else if (caller.match(/TMSI/)) {
	var tmsi = caller.substr(4);
	for (var imsi_key in registered_subscribers) {
	    if (registered_subscribers[imsi_key].tmsi == tmsi) {
		msg.caller = registered_subscribers[imsi_key].msisdn;
		break;
	    }
	}
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
	    // this is the short number for an offline user that doesn't have a msisdn associated in subscribers
	    // bad luck
	    if (registered_subscribers[imsi_key]!="")
		return registered_subscribers[imsi_key].msisdn;
	}
    }

    return called;
}

function routeOutside(msg)
{
    if (msg.callednumtype=="international")
	msg.called = "+"+msg.called;

    msg.line = "outbound";
    msg.retValue("line/"+msg.called);

    return true;
}

function routeSOS(msg)
{
    if ("ybts" != msg.module || "sos" != msg.called)
	return false;
    if (gw_sos.length) {
	if (gw_sos.match(/^\+?[0-9]+$/)) {
	    // Route to a number (registered user)
	    msg.called = gw_sos;
	    return false;
	}
	msg.retValue(gw_sos);
	return true;
    }
    return routeOutside(msg);
}

function routeToRegUser(msg,called)
{
    var msisdn, loc;
    for (var imsi_key in registered_subscribers) {
	msisdn = registered_subscribers[imsi_key]["msisdn"];
	loc = registered_subscribers[imsi_key]["location"];
	if (msisdn.substr(0,1)=="+")
	    msisdn = msisdn.substr(1);
	if (called.substr(-msisdn.length)==msisdn || msisdn.substr(-called.length)==called) {
	    if (loc!="") {
		msg.otmsi = registered_subscribers[imsi_key]["tmsi"];
		msg.oimsi = imsi_key;
		msg.retValue(loc);
		return true;
	    } else {
		msg.error = "offline";
		return true;
	    }
	}
    }
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
    var now = Date.now()/1000;
    var sms;

    // check if we have any SMSs to send
    for (var i=0; i<pendingSMSs.length; i++) {
	if (pendingSMSs[i].next_try<=now) {
	    var sms = pendingSMSs[i];
	    pendingSMSs.splice(i,1);
	}
    }

    if (sms!=undefined) {

	// make sure destination subscriber is registered
	var loc = registered_subscribers[sms.dest_imsi]["location"];
	if (loc=="") {
	    sms.next_try = now + 300; // current time + 5 minutes
	    pendingSMSs.push(sms);
	    Engine.debug(Engine.DebugInfo,"Did not deliver sms from imsi "+sms.imsi+" to number "+sms.dest+" because destination is offline. Waiting 5 minutes before trying again.");

	    // Reschedule after 5s
	    onInterval.nextIdle = now + 5;
	    return;
	}

	res = mtLocalDelivery(sms);
	if (res==false) {
	    sms.tries = sms.tries - 1;
	    if (sms.tries>=0) {
		// if number of attempts to deliver wasn't excedeed push it on last position
		// add sms at the end of pending SMSs
		sms.next_try = now + 5; 
		pendingSMSs.push(sms);
		Engine.debug(Engine.DebugInfo,"Could not deliver sms from imsi "+sms.imsi+" to number "+sms.dest+".");
	    } else
	        Engine.debug(Engine.DebugInfo,"Droped sms from imsi "+sms.imsi+" to number "+sms.dest+". Exceeded attempts.");
	} else
	    Engine.debug(Engine.DebugInfo,"Delivered sms from imsi "+sms.imsi+" to number "+sms.dest);
    }

    if (now%100<5) {

	// see if we should expire TMSIs
	for (var imsi_key in registered_subscribers) {
	    if (now>=registered_subscribers[imsi_key]["expires"]) {
		Engine.debug(Engine.DebugInfo, "Expiring subscriber "+imsi_key+". now="+now+", expire="+registered_subscribers[imsi_key]["expires"]);
		delete registered_subscribers[imsi_key];
		saveUE(imsi_key);
	    }
	}

    }
  
    // Reschedule after 5s
    onInterval.nextIdle = now + 5;
}

// Deliver SMS to registered MS in MT format
function mtLocalDelivery(sms)
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
    m.callto = registered_subscribers[sms.dest_imsi]["location"];
    m.oimsi = sms.dest_imsi;
    m.otmsi = registered_subscribers[sms.dest_imsi]["tmsi"];
    m.maxpdd = "5000";
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

    var sms = {"imsi":"nib_smsc", "msisdn":eliza_number, "smsc":nib_smsc_number, "dest":msisdn, "dest_imsi":imsi_orig, "next_try":Date.now(), "tries":sms_attempts, "msg":answer };

    pendingSMSs.push(sms);
    return true;
}

// MO SMS handling (since we only deliver locally the MT SMSs are generated by NIB)
function onSMS(msg)
{
    Engine.debug(Engine.DebugInfo,"onSMS");

    var imsi = msg.imsi;
    var tmsi = msg.tmsi;

    if (msg.caller=="" || msg.called=="" || (imsi=="" && tmsi=="")) {
	// Protocol error, unspecified
	msg.error = "111";
	return false;
    }

    if (tmsi!="" && imsi=="") {
	imsi = getSubscriberIMSI(null,tmsi);
	msg.imsi = imsi;
    }
    if (imsi=="" || registered_subscribers[imsi]==undefined) {
	// Unidentified subscriber
	msg.error = "28";
	return false;
    }

    // user must be registered
    var msisdn = getRegUserMsisdn(imsi);
    if (msisdn==false) {
	// Unidentified subscriber
	msg.error = "28";
	return false;
    }

    var dest = msg["sms.called"];
    if (dest==eliza_number)
	return onElizaSMS(msg, imsi, dest, msisdn);

    // dest should be one of our users. we only deliver locally

    // check if short number was used
    dest = isShortNumber(dest);
    var dest_imsi = getSubscriberIMSI(dest);
    if (dest_imsi==false) {
	msg.error = "1"; // Unassigned (unallocated) number
	return false;
    }
   
    var next_try = Date.now()/1000; 
    var sms = {"imsi":imsi, "msisdn":msisdn, "smsc":msg.called, "dest":dest, "dest_imsi":dest_imsi, "next_try":next_try, "tries":sms_attempts, "msg":msg.text };

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

    var caller_imsi = msg.imsi;
    var caller_tmsi = msg.tmsi;
    if (caller_tmsi!="" && caller_imsi=="") {
	caller_imsi = getSubscriberIMSI(null,caller_tmsi);
	msg.imsi = caller_imsi;
    } 
    else if (caller_imsi!="" && caller_tmsi=="") {
	if (registered_subscribers[caller_imsi]["tmsi"]=="") {
	    // weird
	    msg.error = "service-unavailable";
	    return false;
	}
	msg.tmsi = registered_subscribers[caller_imsi]["tmsi"];
    }

    // rewrite caller param to msisdn in case it's in IMSI format
    if (!updateCaller(msg))
	return true;

    if (routeSOS(msg))
	return true;

    var called = msg.called;
    var caller = msg.caller;

    if (caller.substr(0,1)=="+") {
	msg.caller = caller.substr(1);
	msg.callernumtype = "international";
    }

    if (called.substr(-david_number.length)==david_number) {
	// this should have been handled by welcome.js
	return false;
    }

    if (called=="333" || called=="+333") {
	// call to conference number
	msg.retValue("conf/333");
	return true;
    }

    var orig_called = called;
    var posib_called_param = ["called", "sip_to", "sip_p-called-party-id", "sip_ruri"];

    for (var called_param of posib_called_param) {
	called = getCalled(msg, called_param);
	if (called=="")
	    continue;

	called = isShortNumber(called);

	if (routeToRegUser(msg,called)==true) {
	    msg.called = called;
	    return true;
	}

	if (subscribers) {
	    if (getSubscriberIMSI(called)!=false) {
		msg.called = called;
		// called is in our users but it's not registered
		msg.error = "offline";
		return true;
	    }
	}
    }
    called = orig_called;

    if (subscribers) {
	// call seems to be for outside
	// must make sure caller is in our subscribers

	if (caller_imsi=="" || subscribers[caller_imsi]=="") {
	    msg.error = "service-unavailable"; // or maybe forbidden
	    return true;
	}

	return routeOutside(msg);
    }

    // registration is accepted based on IMSI regexp
    // check that caller is registered and then routeOutside

    if (registered_subscribers[caller_imsi]!="")
	return routeOutside(msg);

    // caller is not registered to us and is trying to call outside NIB
    msg.error = "service-unavailable"; // or maybe forbidden
    return true;
}

/*
 * Retrieve called number from different parameters/SIP headers
 */
function getCalled(msg,param_name)
{
    var called = msg[param_name];
    if (param_name=="called")
	return called;
    res = called.match(/:?([+0-9]+)[@>]/);
    if (res)
	return res[1];
    return "";
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
    var rand = msg["auth.rand"];
    var auts = msg["auth.auts"];

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
    if (subscribers==undefined) {
	Engine.debug(Engine.DebugWarn, "MT auth is set, but subscribers are not defined. You can't authentify MT calls/SMSs when regexp is defined in subscribers.conf.");
	return true;
    }

    var imsi = msg.imsi;
    var tmsi = msg.tmsi;

    if (tmsi!="" && imsi=="") {
	imsi = getSubscriberIMSI(null,tmsi);
    } 
    if (imsi=="") {
	// this should not happen
	Engine.debug(Engine.DebugWarn,"Got auth with empty imsi and tmsi.");
	return false;
    }
    var ki = subscribers[imsi].ki;
    if (ki=="*") {
	Engine.debug(Engine.DebugWarn, "MT auth is set, but IMSI "+imsi+" doesn't use authentication.");
	return true;
    }

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
    var posib_msisdn;

    // don't look at user.register from other modules besides ybts
    if (msg.driver!="ybts")
	return false;

    var imsi = msg.imsi;
    var tmsi = msg.tmsi;

    if (tmsi!="" && imsi=="") {
	imsi = getSubscriberIMSI(null,tmsi);
	if (imsi==false) {
	    msg.askimsi = true;
	    msg.askimei = true;
	    return false;
	}
    } else if (imsi=="") {
	// this should not happen
	Engine.debug(Engine.DebugWarn,"Got user.register with empty imsi and tmsi.");
	return false;
    } else if (tmsi=="") {
	if (registered_subscribers[imsi]!="")
	    tmsi = registered_subscribers[imsi]["tmsi"];
    }

    if (registered_subscribers[imsi]!="")
	posib_msisdn = registered_subscribers[imsi]["msisdn"];

    msg.imsi = imsi;
    msg.tmsi = tmsi; 

    Engine.debug(Engine.DebugInfo, "Got user.register for imsi='"+imsi+"', tmsi='"+tmsi+"'");

    if (subscribers != undefined) {
	Engine.debug(Engine.DebugInfo,"Searching imsi in subscribers.");

	msisdn = getSubscriberMsisdn(imsi);
	if (subscribers[imsi] != "") {
	    var ki = subscribers[imsi].ki;
	    if (ki!="*") {
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
	}
	if (msisdn==null || msisdn=="") {
	    // check if imsi is already registered so we don't allocate a new number
	    msisdn = alreadyRegistered(imsi);
	    if (msisdn==false) {
		if (posib_msisdn!="") 
		    if (numberAvailable(posib_msisdn,imsi))
			msisdn = posib_msisdn;

		if (msisdn==false) {
		    Engine.debug(Engine.DebugInfo,"Located imsi without msisdn. Allocated random number");
		    msisdn = newNumber(imsi);
		}
	    }
	} else if (msisdn==false) {
	    // IMSI is not allowed
	    addRejected(imsi);
	    msg.error = "location-area-not-allowed";
	    return false;
	}
    } else {
	if (regexp == undefined) {
	    Engine.alarm(alarm_conf,"Please configure accepted subscribers or regular expression to accept by.");
	    // maybe reject everyone until system is configured ??
	    // addRejected(imsi);
	    msg.error = "network-failure";
	    return false;
	} else {
	    // check that imsi is valid against regexp

	    if (imsi.match(regexp)) {
		// check if imsi is already registered so we don't allocate a new number
		msisdn = alreadyRegistered(imsi);
		if (msisdn==false) {
		    if (posib_msisdn!="")
			if (numberAvailable(posib_msisdn,imsi))
			    msisdn = posib_msisdn;

		    if (msisdn==false) {
			Engine.debug(Engine.DebugInfo,"Allocated random number");
			msisdn = newNumber(imsi);
		    }
		}
	    } else {
		addRejected(imsi);
		msg.error = "location-area-not-allowed";
		return false;
	    }
	}
    }

    if (tmsi=="")
	msg.tmsi = allocTmsi();

    if (registered_subscribers[imsi]=="" || (registered_subscribers[imsi]!="" && registered_subscribers[imsi]["msisdn"]!=msisdn))
	sendGreetingMessage(imsi, msisdn);

    msg.msisdn = msisdn;
    registerSubscriber(msg);

    Engine.debug(Engine.DebugInfo,"Registered imsi "+imsi+" with number "+msisdn);
    return true;
}

function onUnregister(msg)
{
    if (msg.driver!="ybts")
	return false;

    var imsi = msg.imsi;
    var tmsi = msg.tmsi;
    if (tmsi!="" && imsi=="")
	imsi = getSubscriberIMSI(null,tmsi);
    if (imsi=="")
	return false;

    unregisterSubscriber(imsi);
    Engine.debug(Engine.DebugInfo,"Finished onUnregister imsi "+imsi);

    return true;
}

// Make sure only NIB mode is enabled
function verifyYBTSMode()
{
    var m = new Message("engine.help");
    m.line = "roaming";
    m.dispatch();

    if (m.dispatch())
	Engine.alarm(alarm_conf, "NIB mode and Roaming mode are enabled at the same time. Please edit javascript.conf and disable one of them. It is recommened to set mode=nib/roaming in [ybts] section in ybts.conf.");
    else
	Engine.debug(Engine.DebugInfo,"Checked that only NIB is enabled.");
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


nibHelp =  "  nib [list|reload]\r\n";

// Provide help for rmanager command line
function onHelp(msg)
{
    if (msg.line) {
	if (msg.line == "nib") {
	    msg.retValue(nibHelp + "Control the NIB mode\r\n");
	    return true;
	}
	return false;
    }
    msg.retValue(msg.retValue() + nibHelp);
    return false;
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
	case "reload":
	    oneCompletion(msg,"nib",part);
	    return;
    }

    switch (partial) {
	case "n":
	case "ni":
	    oneCompletion(msg,"nib",part);
	    break;
    }
}

function onReload(msg)
{
    if (msg.plugin && (msg.plugin != "nib"))
	return false;

    updateSubscribersInfo();
    Engine.output("NIB: Finished updating subscribers and configurations.\r\n");
    return !!msg.plugin;
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
	    for (var imsi_key in registered_subscribers)
		if (registered_subscribers[imsi_key]["location"]!="")
		    tmp += imsi_key+"   "+registered_subscribers[imsi_key]["msisdn"]+"\r\n";
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
	    msg.retValue("Finished updating subscribers and configurations.\r\n");
	    return true;

	case "nib":
	case "nib list":
	    msg.retValue("Incomplete command!\r\n");
	    return true;
    }

    var part = msg.line.split(" ");
    var build_command = part[0] + " " +part[1];
    var part_command = "nib registered";

    if (build_command != part_command)
	return false;

    if (!part[2]) {
	msg.retValue("Incomplete command: "+msg.line+"! Add IMSI to test if is registered.\r\n");
	return true;
    }

    var tmp = part[3]+" not registered.";
    for (var imsi_key in registered_subscribers) {
	if (imsi_key != part[2])
	    continue;
	if (registered_subscribers[imsi_key]["location"]!="")
	    var tmp = imsi_key+" is registered.\r\n";
	msg.retValue(tmp);
	return true;
    }

    msg.retValue(tmp);
    return false;
}

eliza_number = "35492";
david_number = "32843";
nib_smsc_number = "12345";
sms_attempts = 3;
registered_subscribers = {};
pendingSMSs = [];
seenIMSIs = {};  // imsi:count_rejected
ussd_sessions = {};
alarm_conf = 3;
def_nnsf_bits = 8;
def_nnsf_node = 123;

Engine.debugName("nib");
Message.install(onUnregister,"user.unregister",80);
Message.install(onRegister,"user.register",80);
Message.install(onRoute,"call.route",80);
Message.install(onIdleAction,"idle.execute",110,"module","nib_cache");
Message.install(onSMS,"msg.execute",80,"callto","nib_smsc");
Message.install(onCommand,"engine.command",120);
Message.install(onHelp,"engine.help",120);
Message.install(onAuth,"auth",80);
Message.install(onReload,"engine.init",110);

Engine.setInterval(onInterval,1000);
readConfiguration();
readUEs();
verifyYBTSMode();
