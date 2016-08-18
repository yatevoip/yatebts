/**
 * nib_config.js
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

#require "nib_validations.js"

// Class and reimplemented methods to configure subscribers.conf 
SubscribersConfig = function()
{
    GenericConfig.apply(this);
    this.name = "subscribers";
    this.file = "subscribers";
    this.error = new Object();
    this.skip_empty_params = new Object();
    this.sections = new Array();
    this.params_allowed_empty = ["gw_sos", "op", "msisdn", "short_number"];
    this.params_required = new Object();
};

SubscribersConfig.prototype = new GenericConfig;
SubscribersConfig.prototype.validations = {
    "general": {
	"country_code": {"callback": checkValidCC},
	"smsc": {"callback": checkValidSMSC},
	"gw_sos": {},
	"regexp": {}
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
	if (!imsi.match(/[0-9]{14,15}/)) {
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
		    this.error.reason = "This IMSI: " + imsi["imsi"] + " is already set in subscribers.conf.";
		    this.error.error = 401;
		    return false;
		}
	    }
	}
	// combine the new values with the ones already written in file
	this.current_config[data] = params[data];
    }

    //find duplicate in combined data
    var duplicated_imsi = new Object();
    for (var imsi in this.current_config) {
	if (!duplicated_imsi[this.current_config[imsi]["msisdn"]])
	    duplicated_imsi[this.current_config[imsi]["msisdn"]] = new Array();
	var msisdn = duplicated_imsi[this.current_config[imsi]["msisdn"]];
	msisdn.push(imsi);
    }
    for (var msisdn in  duplicated_imsi) {
	var imsis = duplicated_imsi[msisdn];
	if (imsis.length > 1) {
	    this.error.error = 401;
	    this.error.reason = "The IMSIs: ";
	    for (var i = 0; i < imsis.length; i++) {
		this.error.reason += " " +imsis[i] +", ";
	    }
	    this.error.reason += " have the same MSISDN: "+ msisdn;		
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
	    "short_number": {"regex": "^[0-9].*$"},
	    "active": {"callback":ceckOnOff},
	    "ki": {"callback":checkValidKi},
	    "op": {"callback":checkValidOP},
	    "imsi_type":{"select":["2G", "3G"]},
	    "iccid": {"callback": checkValidIccid},
	    "opc": {"callback": checkValidHex}
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

    if (params["regexp"] || params["country_code"] || params["smsc"] || params["gw_sos"]) {
	this.overwrite = false;
	var general_sections = ["country_code", "regexp", "gw_sos", "smsc"];

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
	    this.params_required["general"] = ["country_code","smsc"];
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
API.on_get_nib_subscribers = function(params,msg)
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
	    if (i < start || i > (start+end))
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
API.on_set_nib_subscribers = function(params,msg,setNode)
{
    // delete the 'regexp' param from general section
    c = new ConfigFile(Engine.configFile("subscribers"));
        if (!c.load("Could not load subscribers."))
		        return { name: "subscribers", object: {}, message: "File subscribers.conf not load."};
    var error = new Object();
    c.clearKey("general", "regexp");

    if (!saveConf(error,c)) {
	return error;
    }

    var subs = new SubscribersConfig;
    return  API.on_set_generic_file(subs,params,msg,setNode);
};

// Remove a subscriber section from subscribers.conf
API.on_delete_nib_subscriber = function(params,msg)
{
    if (!confs)
	return { error: 201, reason: "Devel/configuration error: no config files defined."};
    if (!params.imsi)
	return { error: 402, reason: "Missing IMSI in request." };

    c = new ConfigFile(Engine.configFile("subscribers"));
    if (!c.load("Could not load subscribers."))
	return { name: "subscribers", object: {}, message: "File subscribers.conf not load."};

    if (!c.getSection(params.imsi)) {
	var mess = "Missing subscriber with IMSI: " + params.imsi + ". Nothing to delete.";
	return { error: 201, reason: mess};
    }

    var error = new Object();
    c.clearSection(params.imsi);

    if (!saveConf(error,c))
	return error;

    return {};
};

// Get subscribers.conf params from [general] section 
API.on_get_nib_system = function(params,msg)
{
    var subs = new SubscribersConfig;
    var res = API.on_get_generic_file(subs,msg);

    if (res.reason)
	return res;

    delete res.general["updated"];
    delete res.general["locked"];
    return { name: "nib_system", object: res.general};
};

// Configure subscribers.conf params
API.on_set_nib_system = function(params,msg,setNode)
{
    var subs = new SubscribersConfig;
    return API.on_set_generic_file(subs,params,msg,setNode);
};

// Configure accfile.conf params
API.on_set_nib_outbound = function(params,msg)
{
    if (!confs)
	return { error: 201, reason: "Devel/configuration error: no config files defined."};
    if (!params)
	return { error: 402, reason: "Missing params in request" };

    var outbound_params = {"sip": ["enabled","protocol", "username","password","server", "formats", "description", "registrar", "authname", "domain", "outbound","ip_transport","ip_transport_remoteip","ip_transport_remoteport", "ip_transport_localip", "ip_transport_localport", "localaddress", "keepalive", "match_port", "match_user"],
    "iax":['enabled', 'protocol', 'username', 'password', 'server', 'description', 'interval', 'connection_id', 'ip_transport_localip', 'ip_transport_localport', 'trunking','trunk_timestamps', 'trunk_efficient_use', 'trunk_sendinterval', 'trunk_maxlen', 'trunk_nominits_sync_use_ts', 'trunk_nominits_ts_diff_restart', 'port']};
    var required_params = ["username", "server", "password"];

    var outbound_conf = new ConfigFile(Engine.configFile("accfile"));
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

	    if (params['ip_transport'] == 'UDP' && params['ip_transport_localip'] && params['ip_transport_localport']) 
		return { error: 402, reason: "Field ip_transport_localip must be set. This parameter is used in conjuction ip_transport_localport to identify the transport to use." };

	    if (params['ip_transport'] == 'UDP' && params['ip_transport_localip'] && params['ip_transport_localport'])
		return { error: 402, reason: "Field ip_transport_localport must be set. This parameter is used in conjuction ip_transport_localip to identify the transport to use." };

	    if (params['ip_transport'] != 'UDP' && params['ip_transport_localport'])
		return { error: 402, reason: "Invalid ip_transport. The Transport must be on UDP." };

	} else if (params['protocol'] == 'iax') {
	    if (params['trunk_sendinterval'] && params['trunk_sendinterval'] < 5)
		return { error: 402, reason: "For trunk_sendinterval minimum allowed is 5." };

	    if (params['trunk_maxlen'] && params['trunk_maxlen'] < 20)
		return { error: 402, reason: "For trunk_maxlen minimum allowed is 20." };

	    if (params['trunk_nominits_ts_diff_restart'] && params['trunk_nominits_ts_diff_restart'] < 1000)
		return { error: 402, reason: "For trunk_nominits_ts_diff_restart minimum allowed is 1000." };
	    
	    if (params['trunk_nominits_ts_diff_restart'] && params['trunk_nominits_ts_diff_restart'] && params['trunk_nominits_sync_use_ts'] == 'no')
		return { error: 402, reason: "Field Trunk nominits ts diff restart is ignored because trunk_nominits_sync_use_ts is disabled." };
	} else {
	    var mess = "Protocol: " + params['protocol'] + " not allowed, only sip and iax accepted."; 
	    return { error: 402, reason: mess };
	}

	//write data in file
	for (var param_name in params) {
	    var param_value = params[param_name];
	    outbound_conf.setValue("outbound",param_name,param_value);
	}
    }
    
    if (!saveConf(error,outbound_conf))
	return error;

    return {};
};

// Get accfile.conf params
API.on_get_nib_outbound = function(params,msg)
{
    if (!confs)
	return { error: 201, reason: "Devel/configuration error: no config files defined." };

    c = new ConfigFile(Engine.configFile("accfile"));
    if (!c.load("Could not load accfile."))
	return { name: "outbound", object: {}, message: "File accfile.conf not load." };

    sections = c.sections();

    if (!sections["outbound"])
	return { error: 201, reason: "Missing outbound section in accfile.conf." };

    res = {};
    var section = sections["outbound"];
    keys = section.keys();
    for (key of keys) {
	if (!res["outbound"])
	    res["outbound"] = {};
	res["outbound"][key] = section.getValue(key);
    }

    if (res.outbound)
	return { name: "outbound", object: res.outbound };

    return { error: 201, reason: "Missing outbound section in accfile.conf." };
};

// Get online subscribers
API.on_get_online_nib_subscribers = function(params,msg)
{
    if (!registered_subscribers) {
	if (params.limit || params.offset)
	    return { name: "subscribers", object: {} };

	return { name: "count", object: 0 };

    }

    var reg_subs = {};
    for (var imsi_key in registered_subscribers) {
	if (registered_subscribers[imsi_key]["location"] != "") {
	    if (!reg_subs["IMSI"])
		reg_subs["IMSI"] = {};
	    reg_subs["IMSI"] = imsi_key;
	    if (!reg_subs["MSISDN"])
		reg_subs["MSISDN"] = {};
	    reg_subs["MSISDN"] = registered_subscribers[imsi_key]["msisdn"];
	}
    }

    var total = Object.keys(reg_subs).length;

    if (params.limit || params.offset) {
	if (isMissing(params.offset))
	    return { error: 402, reason: "Missing 'offset' in request" };
	if (isMissing(params.limit))
	    return { error: 402, reason: "Missing 'limit' in request" };

	if (!total)
	    return { name: "subscribers", object: {} };

	var i = 0;
	var start = parseInt(params.offset);
	var end = parseInt(params.limit);
	for (var subs in reg_subs) {
	    if (i < start || (i > (start+end)))
		delete reg_subs[subs];
	    i++;
	}
	return { name: "subscribers", object: reg_subs };
    } 

    return { name: "count", object: total };
};

// Get rejected subscribers
API.on_get_rejected_nib_subscribers = function(params,msg)
{
    if (!seenIMSIs) {
	if (params.limit || params.offset)
	    return {name:"subscribers", object:{}};

	return {name:"count", object:0};
    }

    var rejected_subs = "";
    for (var imsi_key in seenIMSIs) {
	if (!rejected_subs["IMSI"])
	    rejected_subs["IMSI"] = {};
	rejected_subs["IMSI"] = imsi_key;
	if (!rejected_subs["NO"])
	    rejected_subs["NO"] = {};
	rejected_subs["NO"] = seenIMSIs[imsi_key];
    }

    var total = Object.keys(rejected_subs).length;

    if (params.limit || params.offset) {
	if (isMissing(params.offset))
	    return { error: 402, reason: "Missing 'offset' from request." };
	if (isMissing(params.limit))
	    return { error: 402, reason: "Missing 'limit' from request." };

	if (!total)
	    return { name: "subscribers", object: {} };

	var i = 0;
	var start = parseInt(params.offset);
	var end = parseInt(params.limit);
	for (var imsi in rejected_subs) {
	    if (i < start || (i > (start+end)))
		delete rejected_subs[imsi];
	    i++;

	}
	return { name: "subscribers", object: rejected_subs };
    }
    return { name: "count", object: total };

};

// Configure cdrfile.conf params
API.on_set_nib_cdrfile = function(params,msg)
{
    if (!confs)
	return { error: 201, reason: "Devel/configuration error: no config files defined."};
    if (!params)
	return { error: 402, reason: "Missing params in request" };
    var cdrfile = new ConfigFile(Engine.configFile("cdrfile"));
    //write data in file
    for (var param_name in params) {
	var param_value = params[param_name];
	cdrfile.setValue("general",param_name,param_value);
    }
    
    if (!saveConf(error,cdrfile))
	return error;

    return {};
};

// Get cdrfile.conf params
API.on_get_nib_cdrfile = function(params,msg)
{
    if (!confs)
	return { error: 201, reason: "Devel/configuration error: no config files defined."};

    c = new ConfigFile(Engine.configFile("cdrfile"));
    if (!c.load("Could not load cdrfile."))
	return { name: "cdrfile", object: {}, message: "File cdrfile.conf not load."};

    sections = c.sections();

    if (!sections["general"])
	return { error: 201, reason: "Missing general section in cdrfile.conf."};

    res = {};
    var section = sections["general"];
    keys = section.keys();
    for (key of keys) {
	if (!res["general"])
	    res["general"] = {};
	res["general"][key] = section.getValue(key);
    }

    if (res.general)
	return { name: "cdrfile", object: res.general};

    return { error: 201, reason: "Missing general section in cdrfile.conf."};
};
/* vi: set ts=8 sw=4 sts=4 noet: */
