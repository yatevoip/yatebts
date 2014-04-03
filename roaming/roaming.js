#require "roamingconf.js"

var success_codes = [200,202,204];

/**
 * Handle "auth" message
 * @param msg Object message to be handled
 */
function onAuth(msg)
{
    var imsi = msg.imsi;

    var sr = buildRegister(imsi,expires);
    if (!addAuthParams(sr,msg,imsi))
	return false;

    sr.wait = true;
    if (sr.dispatch(true)) {
	if (authSuccess(sr,imsi,msg))
	    return true;
	return reqResponse(sr,imsi,msg,true);
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

    var sr = buildRegister(imsi,expires);
    if (!addAuthParams(sr,msg,imsi))
	return false;

    sr.wait = true;
    if (sr.dispatch(true)) {
	if (authSuccess(sr,imsi,msg))
	    return true;
	return reqResponse(sr,imsi,msg);
    }

    return false;
}

/**
 * Build a SIP REGISTER message 
 * @param imsi String
 * @param exp Integer Expire time for SIP registration. Min 0, Max 3600 
 */
function buildRegister(imsi,exp)
{
    var sr = new Message("xsip.generate");
    sr.method = "REGISTER";
    sr.user = "IMSI" + imsi;
    sr.uri = "sip:" + reg_sip;
    sr.sip = "sip:" + sr.user + "@" + my_sip;
    var uri = "<" + sr.sip + ">";
    sr.sip_From = uri;
    sr.sip_To = uri;
    sr.sip_Contact = uri + ";expires=" + exp;
    sr.sip_Expires = exp;

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
    if (success_codes.indexOf(sr.code)!=-1) {
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

    var sr = buildRegister(imsi,0);
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
    m.wait = true;
    if (!addAuthParams(m,msg,imsi))
	return false;

    if (m.dispatch(true)) {
	if (success_codes.indexOf(m.code)!=-1) {
	    if (m.xsip_body!="")
                msg.irpdu = Engine.atoh(m.xsip_body);
	    return true;
	}
	return reqResponse(m,imsi,msg);
    }

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
    if (msg.username!="")
	// add auth params if any
    	addAuthParams(msg,msg,msg.username,"osip_Authorization");

    return false;	
}

// hold temporary info: nonce and realm for authenticating various requests
var tempinfo = {};
// hold temporary chanid-imsi association for authenticating INVITEs
var tempinfo_route = {};

Message.install(onRegister,"user.register",80);
Message.install(onUnregister,"user.unregister",80);
Message.install(onRoute,"call.route",80);
Message.install(onAuth,"auth",80);
Message.install(onMoSMS,"msg.execute",80,"callto","smsc_yatebts");
Message.install(onDisconnected,"chan.disconnected",40);
Message.install(onExecute,"call.execute",80);
