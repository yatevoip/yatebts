/**
 * roaming.js
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
 * Roaming over SIP interface to OpenVoLTE
 * To use it put in javascript.conf:
 *
 * [scripts]
 * roaming=roaming.js
 */

#require "roamingconf.js"

/**
 * Handle "auth" message
 * @param msg Object message to be handled
 */
function onAuth(msg)
{
    var imsi = msg.imsi;

    var sr = buildRegister(imsi,expires,msg.imei);
    if (!addAuthParams(sr,msg,imsi))
	return false;

    sr.wait = true;
    if (sr.dispatch(true)) {
	if (authSuccess(sr,imsi,msg))
	    return true;
	return reqResponse(sr,imsi,msg,true);
    } else {
	Engine.debug(Engine.debugWarn, "Could not do xsip.generate for method REGISTER in onAuth.");
    }

    return false;
}

/**
 * Handle "user.register" message
 * @param msg Object message to be handled
 */
function onRegister(msg)
{
    var imsi = "" + msg.username;
    if (!imsi)
	return false;

    Engine.debug(Engine.DebugInfo, "Preparing to send REGISTER");

    var sr = buildRegister(imsi,expires,msg.imei);
    if (!addAuthParams(sr,msg,imsi))
	return false;

    sr.wait = true;
    if (sr.dispatch(true)) {
	if (authSuccess(sr,imsi,msg)) {
	    // check for expires in Contact header 
	    var contact = sr["sip_contact"];
	    var res = contact.match(/expires *= *"?([^ ;]+)"?/);
	    if (res)
		expires = res[1];
	    else 
		// otherwise check for it in the Expires header
		expires = sr["sip_expires"];

	    expires = parseInt(expires);

	    if (isNaN(expires)) {
		var warning = "Missing Expires header or parameter in Contact header.";
	    } else { 
		if (expires>=t3212) {
		    var half = expires/2;
		    if (half<=t3212) {
			var errmess = "Configuration issue: Timer.T3212 should be smaller than half the Expire time received from server. Expires="+expires+"(seconds), Timer.T3212="+t3212+"(seconds)";
			Engine.debug(Engine.debugWarn, errmess);
		    }
		    return true;
		}
		else
		    var warning = "Configuration issue: Timer.T3212 is higher than Expires received from server. Expires="+expires+", Timer.T3212="+t3212;
	    }
	    Engine.debug(Engine.debugWarn, warning);

	    // deregister from VLR
	    var sr = buildRegister(imsi,0,msg.imei,warning);
	    sr.enqueue();

	    msg.error = "location-area-not-allowed";
	    return false;	    
	}
	return reqResponse(sr,imsi,msg);
    } else {
	Engine.debug(Engine.debugWarn, "Could not do xsip.generate for method REGISTER in onRegister.");
    }

    return false;
}

/**
 * Build a SIP REGISTER message 
 * @param imsi String
 * @param exp Integer Expire time for SIP registration. Min 0, Max 3600 
 * @param imei String representing IMEI of device where request comes from
 * @param warning String - Optional. If set adds Warning header
 */
function buildRegister(imsi,exp,imei,warning)
{
    var sr = new Message("xsip.generate");
    sr.method = "REGISTER";
    sr.user = "IMSI" + imsi;
    sr.uri = "sip:" + reg_sip;
    sr.sip = "sip:" + sr.user + "@" + my_sip;
    var uri = "<" + sr.sip + ">";
    var uri_reg = "<sip:" + sr.user + "@" + reg_sip + ">";
    sr.sip_From = uri_reg;
    sr.sip_To = uri_reg;
    sr.sip_Contact = uri + "; expires=" + exp;
    if (imei!="") {
	imei = imei.substr(0,8) + "-" + imei.substr(8,6) + "-" + imei.substr(-1);
	sr.sip_Contact = sr.sip_Contact + "; +sip.instance=\"<urn:gsma:imei:"+ imei +">\"";
    }
    sr.sip_Expires = exp;
    sr["sip_P-Access-Network-Info"] = "3GPP-GERAN; cgi-3gpp="+mcc+mnc+hex_lac+hex_ci+"; gstn-location=\""+gstn_location+"\"";
    if (warning)
	sr["sip_Warning"] = warning;

    return sr;
}

/**
 * Add @autz parameter in sr message that will translate to a 
 * Authorization header in the corresonsing SIP request
 * @param sr Object Message object where @autz parameter will be set
 * @param msg Object Message object where authentication parameters are found in
 * @param imsi String
 * @param autz String. Name of the parameter in @sr where Authorization info is set.
 * Default value "sip_Authorization"
 */
function addAuthParams(sr,msg,imsi,autz)
{
    if (!autz)
	autz = "sip_Authorization";

    if (msg["auth.response"]!="") {
	sr[autz] = 'Digest ' + tempinfo[imsi]["realm"] + 'uri="' + imsi + '", nonce="' + tempinfo[imsi]["nonce"] + '", response="' + msg["auth.response"] + '", algorithm=AKAv1-MD5';
    } else if (msg["auth.auts"]!="") {
	sr[autz] = 'Digest ' + tempinfo[imsi]["realm"] + 'uri="' + imsi + '", nonce="' + tempinfo[imsi]["nonce"] + '", response="' + msg["auth.response"] + ', auts="' + msg["auth.auts"] + '", algorithm=AKAv1-MD5';
    } else if (msg.error!="") {
	Engine.debug(Engine.DebugWarn, "Authentication failed for imsi: "+imsi+", error: "+msg.error);
	return false;
    }
    return true;
}

/**
 * Handle "chan.disconnected" message
 * @param msg Object message to be handled
 */
function onDisconnected(msg)
{
    var chanid = msg.id;
    if (tempinfo_route[chanid]=="")
	return false;

    var imsi = tempinfo_route[chanid];
    delete tempinfo_route[chanid];

    var auth = msg["sip_www-authenticate"];
    var realm = ""; 
    res = auth.match(/realm *= *"?([^ "]+)"?/); 
    if (res)
	realm = 'realm="' + res[1] + '", ';

    var res = auth.match(/nonce *= *"?([^ "]+)"?/);
    if (res) {
	var nonce = res[1];
	tempinfo[imsi] = { "nonce":nonce,"realm":realm };
	var rand = Engine.atoh(nonce);
	if (rand.length > 32) {
	    var autn = rand.substr(32);
	    autn = autn.substr(0,32); 
	    rand = rand.substr(0,32);
	    msg["auth.rand"] = rand;
	    msg["auth.autn"] = autn;
	} else
	    msg["auth.rand"] = rand;
	msg.error = "noauth";
	return true;
    } else 
	Engine.debug(Engine.DebugWarn,"Invalid header: "+auth);

    return false;
}

/**
 * Check if authentication was successfull in REGISTER response
 * @param sr Object. Message where REGISTER response is received
 * @param imsi String
 * @param msg Object. "user.register"/"auth" message that will be handled
 */
function authSuccess(sr,imsi,msg)
{
    if ((sr.code/100)==2) {
	var uri = sr["sip_p-associated-uri"];
	if (uri != "") {
	    var res = uri.match(/:+?([0-9]+)[@>]/);
	    if (res) {
		msisdn = res[1];
		msg.msisdn = msisdn;
		return true;
	    }
	}
    }
    return false;
}

/**
 * Check request response for authentication parameters
 * @param sr Object. Message representing the SIP response where to check for WWW-Authenticate header
 * @param imsi String
 * @param msg Object. Message object to which authentication parameters are added
 * @param is_auth Bool. Default false. Whether msg is an "auth" message or not. 
 */
function reqResponse(sr,imsi,msg,is_auth)
{
    switch (sr.code) {
	case 401:
	//case 407:
	    var auth = sr["sip_www-authenticate"];

	    var realm = ""; 
	    res = auth.match(/realm *= *"?([^ "]+)"?/); 
	    if (res)
		realm = 'realm="' + res[1] + '", ';

	    var res = auth.match(/nonce *= *"?([^ "]+)"?/);
	    if (res) {
		var nonce = res[1];
		tempinfo[imsi] = { "nonce":nonce,"realm":realm };
		var rand = Engine.atoh(nonce);
		if (rand.length > 32) {
		    var autn = rand.substr(32);
		    autn = autn.substr(0,32); 
		    rand = rand.substr(0,32);
		    msg["auth.rand"] = rand;
		    msg["auth.autn"] = autn;
		} else
		    msg["auth.rand"] = rand;
	    } else 
		Engine.debug(Engine.DebugWarn,"Invalid header: "+auth);

	    msg.error = "noauth";
	    if (is_auth)
		return true;
	    break;
	case 408:
	    msg.reason = "timeout";
	    break;
    }
    return false;
}


/*
 * Handle "user.unregister" message
 * @param msg Object. Message to be handled
 */
function onUnregister(msg)
{
    var imsi = msg.username;
    if (imsi=="")
	return false;

    var sr = buildRegister(imsi,0,msg.imei);
    sr.enqueue();
    return true;
}

/**
 * Handle routing request for SMSs
 * @param msg Object. Message to be handled
 */
function onRouteSMS(msg)
{
    if (msg.username!="") {
	// MO SMS
	msg.retValue("smsc_yatebts");
    } 
    else {
	// MT SMS
	var caller = msg.caller;
	if (caller.substr(0,1)=="+") {
	    caller = caller.substr(1);
	    msg["sms.caller.nature"] = "international";
	}

	msg["sms.caller"] = caller;
	msg.rpdu = Engine.atoh(msg.xsip_body);
	msg.retValue("ybts/"+msg.called);
    }

    return true;
}

/**
 * Handle sending of MO SMS
 * @param msg Object. Message to be handled
 */
function onMoSMS(msg)
{
    Engine.debug(Engine.DebugInfo,"onMoSMS");

    if (msg.caller=="" || msg.called=="")
	return false;

    var imsi = msg["username"];
    var dest = msg["sms.called"];

    if (msg["sms.called.nature"]=="international" && dest.substr(0,1)!="+")
	dest = "+"+dest;

    if (msg.callednumtype=="international" && msg.called.substr(0,1)!="+")
	msg.called = "+"+msg.called;

    var m = new Message("xsip.generate");
    m.method = "MESSAGE";
    m.uri = "sip:" + msg.called  + "@" + reg_sip;
    m.user = msg.caller; 
    m.sip_To = "<sip:" + msg.called + "@" + reg_sip + ">";
    m["sip_P-Called-Party-ID"] = "<tel:" + dest + ">";
    m.xsip_type = "application/vnd.3gpp.sms";
    m.xsip_body_encoding = "hex";
    m.xsip_body = msg.rpdu;
    m["sip_P-Access-Network-Info"] = "3GPP-GERAN; cgi-3gpp="+mcc+mnc+hex_lac+hex_ci+"; gstn-location=\""+gstn_location+"\"";
    m.wait = true;
    if (!addAuthParams(m,msg,imsi))
	return false;

    if (m.dispatch(true)) {
	if ((m.code/100)==2) {
	    if (m.xsip_body!="")
                msg.irpdu = Engine.atoh(m.xsip_body);
	    return true;
	}
	return reqResponse(m,imsi,msg);
    } else
	Engine.debug(Engine.debugWarn, "Could not do xsip.generate for method MESSAGE");

    msg.error = "service unavailable";
    return true;
}

/** 
 * Handle call.route message
 * @param msg Object. Message to be handled
 */
function onRoute(msg)
{
    if (msg.route_type=="msg")
	return onRouteSMS(msg);
    if (msg.route_type!="call" && msg.route_type!="")
	return false;

    var imsi = msg.username;
    if (imsi) {
	// call from inside that must be routed to VLR/MSC if we are online	
	if (msg.callednumtype=="international")
	    msg.called = "+"+msg.called;
	
	tempinfo_route[msg.id] = imsi;
	msg["osip_P-Access-Network-Info"] = "3GPP-GERAN; cgi-3gpp="+mcc+mnc+hex_lac+hex_ci+"; gstn-location=\""+gstn_location+"\"";
	msg.retValue("sip/sip:"+msg.called+"@"+reg_sip);
    } 
    else {
    	// call is from openvolte to user registered in this bts
	var caller = msg.caller;
	if (caller.substr(0,1)=="+") {
	    msg.caller = caller.substr(1);
	    msg.callernumtype = "international";
	}

    	msg.retValue("ybts/"+msg.called);
    }

    return true;
}

/**
 * Add param osip_Authorization to call.execute message that will add 
 * Authorization header to INVITE request if authentication params are set
 * @param msg Object. Message where parameter is added
 */
function onExecute(msg)
{
    if (msg.username!="") {
	// add auth params if any
    	addAuthParams(msg,msg,msg.username,"osip_Authorization");
	msg["osip_P-Access-Network-Info"] = "3GPP-GERAN; cgi-3gpp="+mcc+mnc+hex_lac+hex_ci+"; gstn-location=\""+gstn_location+"\"";
    }

    return false;	
}

/*
 * Read only necessary configuration from [gsm],[gsm_advanced] sections in ybts.conf
 */
function readYBTSConf()
{
    var conf = new ConfigFile(Engine.configFile("ybts"),true);
    var gsm_section = conf.getSection("gsm");

    mcc = gsm_section.getValue("Identity.MCC");
    mnc = gsm_section.getValue("Identity.MNC");

    var lac = gsm_section.getValue("Identity.LAC");
    var ci = gsm_section.getValue("Identity.CI");

    if (mcc=="" || mnc=="" || lac=="" || ci=="")
	Engine.alarm(alarm_conf, "Please configure Identity.MCC, Identity.MNC, Identity.LAC, Identity.CI in ybts.conf. All this parameters are required in roaming mode.");

    hex_lac = get16bitHexVal(lac);
    hex_ci = get16bitHexVal(ci);
    if (hex_lac==false || hex_ci==false)
	Engine.alarm(alarm_conf, "Wrong configuration for Identity.LAC="+lac+" or Identity.CI="+ci+". Can't hexify value.");

    var gsm_advanced = conf.getSection("gsm_advanced");
    if (gsm_advanced)
	t3212 = gsm_advanced.getValue("Timer.T3212");
   
    if (t3212 == undefined)
	t3212 = 1440; // defaults to 24 minutes
    else {
	t3212 = parseInt(t3212);
	if (isNaN(t3212))
	    Engine.alarm(alarm_conf, "Wrong configuration for Timer.T3212. Value is not numeric: '"+gsm_advanced.getValue("Timer.T3212")+"'");
	else
	    t3212 = t3212 * 60;
    }

    if (t3212 == 0)
	Engine.alarm(alarm_conf, "Incompatible configuration: Timer.T3212=0. When sending requests to SIP/IMS server Timer.T3212 is in 6..60 range.");
}

/*
 * Returns 16 bit(or more) hex value
 * @param String String representation of int value that should be returned as hex
 * @return String representing the hex value of @val
 */ 
function get16bitHexVal(val)
{
    val = parseInt(val);
    if (isNaN(val))
	return false;

    val = val.toString(16);
    val = val.toString();
    while (val.length<4)
	val = "0"+val;

    return val;
}

/*
 * Read and check configuration from subscribersconf.js and ybts.conf
 */ 
function checkConfiguration()
{
    if (expires=="" || expires==undefined)
	expires = 3600;

    if (reg_sip=="" || reg_sip==undefined || reg_sip==null)
	Engine.alarm(alarm_conf,"Please configure reg_sip parameter in subscribersconf.js located in the configurations directory.");
    if (my_sip=="" || my_sip==undefined || my_sip==null)
	Engine.alarm(alarm_conf,"Please configure my_sip parameter in subscribersconf.js located in the configurations directory.");
    if (gstn_location=="" || gstn_location==undefined || gstn_location==null)
	Engine.alarm(alarm_conf,"Please configure gstn_location parameter in subscribersconf.js located in the configurations directory.");

    readYBTSConf();
}

// hold temporary info: nonce and realm for authenticating various requests
var tempinfo = {};
// hold temporary chanid-imsi association for authenticating INVITEs
var tempinfo_route = {};
// alarm for configuration issues
var alarm_conf = 3;

Engine.debugName("roaming");
checkConfiguration();
Message.install(onRegister,"user.register",80);
Message.install(onUnregister,"user.unregister",80);
Message.install(onRoute,"call.route",80);
Message.install(onAuth,"auth",80);
Message.install(onMoSMS,"msg.execute",80,"callto","smsc_yatebts");
Message.install(onDisconnected,"chan.disconnected",40);
Message.install(onExecute,"call.execute",80);
