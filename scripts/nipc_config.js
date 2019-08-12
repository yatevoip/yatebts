/**
 * nipc_config.js
 * This file is part of the YATE Project http://YATE.null.ro
 *
 * Yet Another Telephony Engine - a fully featured software PBX and IVR
 * Copyright (C) 2014-2015 Null Team
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

#require "nipc_validations.js"
#require "generic_validations.js"

// Class and reimplemented methods to configure subscribers.conf 
SubscribersConfig = function()
{
    GenericConfig.apply(this);
    this.name = "subscribers";
    this.file = "subscribers";
    this.custom = "subs-custom";
    this.error = new Object();
    this.skip_empty_params = new Object();
    this.sections = new Array();
    this.params_allowed_empty = ["gw_sos", "op", "opc", "msisdn", "short_number"];
    this.params_required = new Object();
};

SubscribersConfig.prototype = new GenericConfig;
SubscribersConfig.prototype.validations = {
    "general": {
	"country_code": {"callback": checkValidCC},
	"smsc": {"callback": checkValidSMSC},
	"gw_sos": {},
	"regexp": {},
	"sms_text": {}
    }
};

SubscribersConfig.prototype.initConfig = function(params)
{
    if (debug)
	dumpObj("Entered SubscribersConfig.initConfig", params);

    check_duplicate = false;
    if (params.general) {
	return true;
    }

    for (var imsi in params) {
	if (imsi == "general")
	    continue;
	if (!imsi.match(/^[0-9]{14,15}$/)) {
	    this.error.error = 401;
	    this.error.reason = "The IMSI: " + imsi + " is not valid. Must contain 14 or 15 digits.";
	    return false;
	}
	if (imsi["imsi_type"] == "2G" && imsi["op"] != "") {
	    this.error.reason = "For 2G type of subscriber the OP must be empty";
	    this.error.error = 401;
	    return false;
	}
	if (imsi["imsi_type"] == "3G" && imsi["op"] == "") {
	    this.error.reason = "OP can't be empty!";
	    this.error.error = 401;
	    return false;
	}
	if (!params[imsi]["imsi"]) {
	    check_duplicate = new Object();
	    check_duplicate = {"imsi": imsi};
	}
    }

    if (this.current_config.general) 
	delete this.current_config.general;

    for (var data in params) {
	// on edit a subscriber check if it is already written in file the IMSI set in params
	if (check_duplicate) {
	    for (var imsi in check_duplicate) {
		if (this.current_config[data]) {
		    this.error.reason = "This IMSI: " + check_duplicate["imsi"] + " is already set in subscribers.conf.";
		    this.error.error = 401;
		    return false;
		}
	    }
	}
	// combine the new values with the ones already written in file
	this.current_config[data] = params[data];
    }

    //find duplicate in combined data
    var elements = [{element: "msisdn", display: "MSISDN"},
	{element: "short_number", display: "Short number"},
	{element: "iccid", display: "ICCID"}];
	
    for (var i in elements) {
	var res = findDuplicatedImsi(this.current_config,elements[i].element,elements[i].display);
	if (res.duplicated) {
	    this.error.error = 401;
	    this.error.reason = res.reason;
	    return false;
	}
    }
    return true;
};


SubscribersConfig.prototype.genericPrepareConfig = SubscribersConfig.prototype.prepareConfig;

SubscribersConfig.prototype.prepareConfig = function(params)
{
    if (debug)
	Engine.output("Entered SubscribersConfig.prepareConfig for object name " + this.name);

    if (params["subscribers"]) {
	this.overwrite = false;
	var subs_validations = {
	    //the msisdn should not start with 0 dar should have at least 7 digits
	    "msisdn": {"regex": "^[1-9][0-9]{6,}$"},
	    //the short number has to have at least 3 digits
	    "short_number": {"regex": "^[0-9]{3,}$"},
	    "active": {"callback":checkOnOff},
	    "ki": {"callback":checkValidKi},
	    "op": {"callback":checkValidOP},
	    "opc": {"callback":checkOnOff},
	    "imsi_type":{"select":["2G", "3G"]},
	    "iccid": {"callback": checkValidIccid},
	};

	// see if we have any defined subscribers and modify sections and validations so this fields are written
	var subs = params["subscribers"];
	var subscriber;
	for (subscriber in subs) {
	    if (!subscriber)
		break;
	    // add section name to this.sections and this.validations so section is written and verified
	    if (debug)
		Engine.output("Extended sections,validations and params_required in SubscribersConfig to include section '" + subscriber + "'");
	    this.sections.push(subscriber);
	    this.validations[subscriber] = subs_validations;
	
	    this.params_required[subscriber] = ["ki", "imsi_type"];
	    params[subscriber]= new Object();
	    params[subscriber] = subs[subscriber];
	}
	delete params.subscribers;
    }

    if (params["regexp"] || params["country_code"] || params["smsc"] || params["gw_sos"] || params[";regexp"] || params["sms_text"]) {
	this.overwrite = false;
	var general_sections = ["country_code", "regexp", "gw_sos", "smsc", ";regexp", "sms_text"];

	params["general"] =  new Object();
	for (var i in general_sections) {
	    if (!isMissing(params[general_sections[i]])) {
		params["general"][general_sections[i]] = params[general_sections[i]];
		delete params[general_sections[i]];
	    }
	}

	this.sections.push("general");
	var total = Object.keys(params).length;
	if (total != 1 && !params["general"]["regexp"])
	    this.params_required["general"] = ["country_code","smsc","sms_text"];
    }

    this.genericPrepareConfig(params);
};

SubscribersConfig.prototype.genericValidateConfig = SubscribersConfig.prototype.validateConfig;

SubscribersConfig.prototype.validateConfig = function(section_name,param_name,param_value,params)
{
    if (debug)
	Engine.output("Entered SubscribersConfig.validateConfig for object name " + this.name);

    if (!this.genericValidateConfig(section_name,param_name,param_value,params))
	return false;

    return true;
};

// Get subscribers.conf params
API.on_get_nipc_subscribers = function(params,msg)
{
    var subs = new SubscribersConfig;
    var res = API.on_get_generic_file(subs,params,msg);

    if (res.reason)
	return res;

    delete res.general;
    var total = Object.keys(res).length;

    if (params.limit || params.offset) {
	if (isMissing(params.offset))
	    return { error: 402, reason: "Missing 'offset' in request" };
	if (isMissing(params.limit))
	    return { error: 402, reason: "Missing 'limit' in request" };

	if (!total)
	     return { name: "subscribers", object: {}};

	var i = 0;
	var start = parseInt(params.offset);
	var end = parseInt(params.limit);
	for (var imsi in res) {
	    if (i < start || i >= (start+end))
		delete res[imsi];
	    i++;
	}
	return { name: "subscribers", object: res};
    } else if (params.imsi) {
        if (res[params.imsi])
	    var subs = res[params.imsi];
	else
	    var subs = {};
	return { name: "subscribers", object: subs};

    } else {

	return { name: "count", object: total};
    }
};

// Configure subscribers.conf params
API.on_set_nipc_subscribers = function(params,msg,setNode)
{
    // don't delete the 'regexp' param from general section

    var subs = new SubscribersConfig;
    res = API.on_set_generic_file(subs,params,msg,setNode);
    if (res.reason)
	return res;
    return reloadNiPC();
};

// Remove a subscriber section from subscribers.conf
API.on_delete_nipc_subscriber = function(params,msg)
{
    if (!confs)
	return { error: 201, reason: "Devel/configuration error: no config files defined."};
    if (!params.imsi)
	return { error: 402, reason: "Missing IMSI in request." };

    c = prepareConf("subscribers",msg.received,false);
    if (!c.getSection(params.imsi)) {
	var mess = "Missing subscriber with IMSI: " + params.imsi + ". Nothing to delete.";
	return { error: 201, reason: mess};
    }

    var error = new Object();
    c.clearSection(params.imsi);

    if (!saveConf(error,c))
	return error;

    return reloadNiPC();
};

// Get subscribers.conf params from [general] section 
API.on_get_nipc_system = function(params,msg)
{
    var subs = new SubscribersConfig;
    var res = API.on_get_generic_file(subs,msg);

    if (res.reason)
	return res;

    delete res.general["updated"];
    delete res.general["locked"];

    if (!res.general) {
	res.general = {"country_code":"", "smsc":"", "regexp":"", "sms_text":""};
    } 

    return { name: "nipc_system", object: res.general};
};

// Configure subscribers.conf params
API.on_set_nipc_system = function(params,msg,setNode)
{
    // clearing regexp
    if (params[";regexp"]==="") {
	// delete the 'regexp' param from general section
	c = prepareConf("subscribers",msg.received,false);
	if (!c.load("Could not load subscribers."))
	    return { name: "subscribers", object: {}, message: "File subscribers.conf not load."};
	var error = new Object();
	c.clearKey("general", "regexp");

	if (!saveConf(error,c)) {
	    return error;
	}

	return reloadNiPC();
    }


    var subs = new SubscribersConfig;
    res = API.on_set_generic_file(subs,params,msg,setNode);

    if (res.reason)
	return res;

    return reloadNiPC();
};

// Configure accfile.conf params
API.on_set_nipc_outbound = function(params,msg)
{
    var section_name;
    var section;
    var section_params;
    var param_name;

    if (!confs)
	return { error: 201, reason: "Devel/configuration error: no config files defined."};
    if (!params)
	return { error: 402, reason: "Missing params in request" };

    var outbound_params = {"sip": ["enabled","protocol", "username","password","server", "formats", "description", "registrar", "authname", "domain", "outbound","ip_transport","ip_transport_remoteip","ip_transport_remoteport", "ip_transport_localip", "ip_transport_localport", "localaddress", "keepalive", "match_port", "match_user", "interval", "out:caller"],
	"iax":['enabled', 'protocol', 'username', 'password', 'server', 'description', 'interval', 'connection_id', 'ip_transport_localip', 'ip_transport_localport', 'trunking','trunk_timestamps', 'trunk_efficient_use', 'trunk_sendinterval', 'trunk_maxlen', 'trunk_nominits_sync_use_ts', 'trunk_nominits_ts_diff_restart', 'port']};
    var required_params = ["username", "server", "password"];

    var int_params = ["ip_transport_remoteport","ip_transport_localport","keepalive","interval",'trunk_maxlen','port','trunk_sendinterval','trunk_nominits_ts_diff_restart'];
    var ip_params  = ["ip_transport_remoteip","ip_transport_localip","localaddress"];

    var outbound_conf = prepareConf("accfile",msg.received,false,"acct-custom");

    sections = c.sections();

    // keep existing section and values
    var current_config = outbound_conf.sections();
    for (section_name in current_config) {
	if (section_name == "outbound") {
	    outbound_conf.clearSection("outbound");
	    continue;
	}

	section = current_config[section_name];
	section_params = section.keys();
	for (param_name of section_params)
	    section.setValue(param_name, section.getValue(param_name));
    }

    if (params) {

	//verify parameters against defined protocol
	for (var param_name in params) {
	    var protocol = outbound_params[params['protocol']];
	    if (!protocol) {
		var mess = "Protocol: " + params['protocol'] + " not allowed, only sip and iax accepted.";
		return { error: 402, reason: mess };
	    }
	    if (protocol.indexOf(param_name) < 0) {
		var mess = "Invalid parameter: "+param_name+" for protocol :"+params['protocol'];
		return { error: 402, reason: mess };
	    }
	}

	for (var ind in required_params) {
	    if (!params[required_params[ind]]) {
		var mess = "Missing required parameter " + params[required_params[ind]] + ".";
		return { error: 402, reason: mess };
	    }
	}
	//fields validations
	if (params['protocol'] == 'sip') {

	    if ( params['localaddress'] && params['localaddress'] == 'no' && params['keepalive'] != 0) 
		return { error: 402, reason: "Invalid keepalive value if localaddress is not set." };

	    if (params['ip_transport'] == 'UDP' && !params['ip_transport_localip'] && params['ip_transport_localport']) 
		return { error: 402, reason: "Field ip_transport_localip must be set. This parameter is used in conjuction ip_transport_localport to identify the transport to use." };

	    if (params['ip_transport'] == 'UDP' && params['ip_transport_localip'] && !params['ip_transport_localport'])
		return { error: 402, reason: "Field ip_transport_localport must be set. This parameter is used in conjuction ip_transport_localip to identify the transport to use." };

	    if (params['ip_transport'] != 'UDP' && params['ip_transport_localport'])
		return { error: 402, reason: "Invalid ip_transport. The Transport must be on UDP." };

	} else if (params['protocol'] == 'iax') {
	    var trunk_sendinterval = parseInt(params['trunk_sendinterval']);
	    if (params['trunk_sendinterval'] && trunk_sendinterval < 5)
		return { error: 402, reason: "For trunk_sendinterval minimum allowed is 5." };

	    var trunk_maxlen = parseInt(params['trunk_maxlen']);
	    if (params['trunk_maxlen'] && trunk_maxlen < 20)
		return { error: 402, reason: "For trunk_maxlen minimum allowed is 20." };

	    var trunk_nominits_ts_diff_restart = parseInt(params['trunk_nominits_ts_diff_restart']);
	    if (params['trunk_nominits_ts_diff_restart'] && trunk_nominits_ts_diff_restart < 1000)
		return { error: 402, reason: "For trunk_nominits_ts_diff_restart minimum allowed is 1000." };

	    if (params['trunk_nominits_ts_diff_restart'] && params['trunk_nominits_ts_diff_restart'] && params['trunk_nominits_sync_use_ts'] == 'no')
		return { error: 402, reason: "Field Trunk nominits ts diff restart is ignored because trunk_nominits_sync_use_ts is disabled." };
	} else {
	    var mess = "Protocol: " + params['protocol'] + " not allowed, only sip and iax accepted."; 
	    return { error: 402, reason: mess };
	}

	//TBI: get all the existing data before writing the received parameters

	var error = {};
	//write data in file
	for (var param_name in params) {
	    var param_value = params[param_name];
	    if (-1 != int_params.indexOf(param_name)) {
		if (!checkValidInteger(error, param_name, param_value, "outbound"))
		    return error;
	    }
	    if (-1 != ip_params.indexOf(param_name)) {
		if (!checkValidIP(error, param_name, param_value, "outbound"))
		    return error;
	    }

	    outbound_conf.setValue("outbound",param_name,param_value);
	}
    }

    if (!saveConf(error,outbound_conf))
	return error;

    Engine.output("Restart on node config: " + msg.received);
    Engine.restart();
    return {};
};

// Get accfile.conf params
API.on_get_nipc_outbound = function(params,msg)
{
    if (!confs)
	return { error: 201, reason: "Devel/configuration error: no config files defined." };

    c = new ConfigFile(Engine.configFile("accfile"));
    if (!c.load("Could not load accfile."))
	return { name: "outbound", object: {}, message: "File accfile.conf not load." };

    sections = c.sections();

    if (!sections["outbound"])
	return { name: "outbound", object: {} };

    res = {};
    var section = sections["outbound"];
    keys = section.keys();
    for (key of keys) {
	if (!res["outbound"])
	    res["outbound"] = {};
	res["outbound"][key] = section.getValue(key);
    }

    return { name: "outbound", object: res.outbound };
};

// run NiPC telnet command and return two column result as array of objects
function imsis_telnet_command(command, name_in_result, name_in_message, column_names)
{
    var m = new Message("engine.command");
    m.line = command;
    m.cmd_width  = 1000;
    m.cmd_height = 10000;
    if (!m.dispatch()) {
	var message = "Could not retrieve " + name_in_message;
        return { error: 402, reason: message };
    }

    var res = m.retValue();
    res = res.split("\n");
    if (res.length<=2)
	return { name: "count", object: 0 };

    var subscriber;
    var col0;
    var coln;

    var reg_subs = {};
    var current_index;
    for (var i=2; i<res.length; i++) {
	subscriber = res[i];
	subscriber = strSplit(subscriber, " | ");
	col0 = subscriber[0];
	coln = subscriber[subscriber.length - 1];
	if (!col0.length)
		continue;
	if (coln.endsWith("\r\n")) {
		subscriber[subscriber.length - 1] = coln.substr(0,coln.length-1);
	}
	current_index = i-2;
	var reg_subscriber = {};
	for (var j=0; j < subscriber.length; j++) {
	    reg_subscriber[column_names[j]] = subscriber[j];
	}
	reg_subs[current_index] = reg_subscriber;
    }

    var total = Object.keys(reg_subs).length;

    if (params.limit || params.offset) {
	if (isMissing(params.offset))
	    return { error: 402, reason: "Missing 'offset' in request" };
	if (isMissing(params.limit))
	    return { error: 402, reason: "Missing 'limit' in request" };

	if (!total)
	    return { name: name_in_result, object: {} };

	var i = 0;
	var start = parseInt(params.offset);
	var end = parseInt(params.limit);
	for (var subs in reg_subs) {
	    if (i < start || (i > (start+end)))
		delete reg_subs[subs];
	    i++;
	}
	return { name: name_in_result, object: reg_subs };
    } 

    return { name: "count", object: total };
}

// Get online subscribers
API.on_get_online_nipc_subscribers = function(params,msg)
{
    var column_names = ["IMSI", "MSISDN", "REGISTERED", "EXPIRES"];
    return imsis_telnet_command("nipc list registered", "subscribers", "online subscribers", column_names);
};

// Get rejected subscribers
API.on_get_rejected_nipc_subscribers = function(params,msg)
{
    var column_names = ["IMSI", "NO"];
    return imsis_telnet_command("nipc list rejected", "imsis", "rejected IMSIs", column_names);    
};

// Get accepted subscribers
API.on_get_accepted_nipc_subscribers = function(params,msg)
{
    var column_names = ["IMSI", "MSISDN"];
    return imsis_telnet_command("nipc list accepted", "subscribers", "accepted subscribers", column_names);
};

function reloadNiPC()
{
    var m = new Message("engine.command");
    m.line = "nipc reload";
    if (!m.dispatch())
	return { error: 402, reason: "Reload of NiPC failed." };

    return {};
};

/* Find if a given element is duplicated in the current configuration */
function findDuplicatedImsi(current_config,element,field_name)
{	
    var duplicated_imsi = new Object();
    for (var imsi in current_config) {
	if (!duplicated_imsi[current_config[imsi][element]])
	    duplicated_imsi[current_config[imsi][element]] = new Array();
	var keep_duplicated = duplicated_imsi[current_config[imsi][element]];
	keep_duplicated.push(imsi);
    }
    var err = "";
    for (var element in duplicated_imsi) {
	var imsis = duplicated_imsi[element];
	if (imsis.length > 1) {
	    err = "The IMSIs: ";
	    for (var i = 0; i < imsis.length; i++) {
		err += imsis[i] +", ";
	    }
	    err += " have the same "+field_name+": "+ element+".";		
	    return { duplicated: true, reason: err };
	}
    }
    return { duplicated: false };
};
/* vi: set ts=8 sw=4 sts=4 noet: */
