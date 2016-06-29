/**
 * ybts_config.js
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

//#pragma cache "true"
//#pragma compile "bts_config.jsc"
//#pragma trace "cachegrind.out.bts_config"

#require "lib_str_util.js"
#require "sdr_config.js"

debug = true;

conf_node = "bts";
// define configuration files that can be set/get 
confs = ["ybts", "satsite", "ybladerf"];

YbtsConfig = function()
{
    GenericConfig.apply(this);
    this.name = "bts";
    this.file = "ybts";
    this.error = new Object();
    this.skip_empty_params = new Object();
    this.sections = ["gsm", "gprs", "ggsn", "transceiver", "control", "tapping", "gsm_advanced", "gprs_advanced", "sgsn", "logging", "test", "ybts", "security", "handover", "roaming", "gprs_roaming"];
    this.params_allowed_empty = ["Args", "DNS", "ShellScript", "MS.IP.Route", "Logfile.Name", "peer_arg", "RadioFrequencyOffset", "TxAttenOffset", "Radio.RxGain", "gprs_nnsf_bits", "nnsf_dns", "network_map", "local_breakout", "neighbors", "reg_sip", "nodes_sip", "my_sip", "gstn_location"];
    this.params_required = { "gsm": ["Radio.Band", "Radio.C0","Identity.MCC", "Identity.MNC", "Identity.LAC", "Identity.CI", "Identity.BSIC.NCC", "Identity.BSIC.BCC", "Radio.PowerManager.MaxAttenDB", "Radio.PowerManager.MinAttenDB"]};
    this.factory_calibrated = {"transceiver": ["TxAttenOffset","RadioFrequencyOffset"], "gsm_advanced": ["Radio.RxGain"]};
};

YbtsConfig.prototype = GenericConfig.prototype;

#require "ybts_fields.js"

YbtsConfig.prototype.genericValidateConfig = YbtsConfig.prototype.validateConfig;

YbtsConfig.prototype.validateConfig = function(section_name,param_name,param_value,params)
{
    if (this.factory_calibrated[section_name] != undefined) {
	var fact_calibrated = this.factory_calibrated[section_name];
	if (fact_calibrated.indexOf(param_name) >= 0 && (param_value == "" || param_value == "Factory calibrated")) {
	    if (!this.skip_empty_params[section_name]) 
		this.skip_empty_params[section_name] = new Object();
	    this.skip_empty_params[section_name][param_name] = true;
	}
    }

    if (!this.genericValidateConfig(section_name,param_name,param_value,params))
	return false;

    var mode = params.ybts["mode"];
    if (!mode)
	return true;

    // validate roaming params if dataroam mode is activated
    if (mode == "dataroam" && section_name == "roaming") {
	if (!validatePositiveNumber(this.error,param_name,param_value,section_name))
	    return false;
    }

    // validate gprs_roaming params if dataroam mode is activated
    if (mode == "dataroam" && section_name == "gprs_roaming") {
	if (param_name == "gprs_nnsf_bits" || param_name == "nnsf_bits")
	    if (!validatePositiveNumber(this.error,param_name,param_value,section_name))
		return false;
	if (param_name == "map_network")
	    if (!validateMapNetworkDataroam(this.error,param_name,param_value,section_name))
		return false;
	if (!isNaN(parseInt(param_name))) {
	    param_value = param_name + "=" + param_value + "\n";
	    if (!validateMapNetworkDataroam(this.error,"Network map",param_value,section_name))
	        return false;
	}
    }

    if (mode == "roaming" && section_name == "roaming") {
	if (!validateRoamingParams(this.error,params))
	    return false;
    }

    if (mode == "roaming" && section_name == "gprs_roaming") {
	if (param_name == "gprs_nnsf_bits" || param_name == "nnsf_bits")
	    if (!checkNnsfBits(this.error,param_name,param_value,section_name))
		return false;
	if (!isNaN(parseInt(param_name))) {
	    param_value = param_name + "=" + param_value + "\n";
	    if (!validateMapNetworkDataroam(this.error,"Network map",param_value,section_name))
	        return false;
	}
    }

    return true;
};

// Get ybts.conf parameters
API.on_get_ybts_node = function(params,msg)
{
    var ybts = new YbtsConfig;
    return API.on_get_generic_file(ybts,msg);
};

// Configure ybts.conf related parameters
API.on_set_ybts_node = function(params,msg,setNode)
{
    var ybts = new YbtsConfig;
    var res = API.on_set_generic_file(ybts,params,msg,setNode);

    if (res.reason)
	return res;

    // set codecs in ysipchan.conf
    var ysipchan = new ConfigFile(Engine.configFile("ysipchan"));
    var codecs_section = ysipchan.getSection("codecs",true);

    var mode = params["ybts"]["mode"];
    Engine.output("YBTS mode is "+mode+". Setting codecs in ysipchan.conf");
    if (mode == "nib") {
	codecs_section.setValue("default","enable");
    } else {
	codecs_section.setValue("default","disable");
	codecs_section.setValue("gsm","enable");
    }
    if (!ysipchan.save())
	return { reason: "Could not save codecs setting in ysipchan.conf", error: 501 };

    return res;
};

// use generic get and set functions
API.on_set_bts_node = API.on_set_node;
API.on_get_bts_node = API.on_get_node;

Engine.debugName("bts_config");
Message.trackName("bts_config");

Message.install(onReload,"engine.init",120);
Message.install(onApiRequest,"api.request",90,"type","config_bts");
/* vi: set ts=8 sw=4 sts=4 noet: */
