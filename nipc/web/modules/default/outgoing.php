<?php
/**
 * outgoing.php
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

global  $sip_fields, $iax_fields;
//var_dump($_REQUEST);
$sip_fields = array(
	"username"=>array("compulsory"=>true, "Username is normally used to authenticate to the other server. It is the user part of the SIP address of your server when talking to the gateway you are currently defining.", "autocomplete"=>"off"), 
	"password"=>array("comment"=>"Insert only when you wish to change"/*,"display"=>"password", "autocomplete"=>"off"*/),
	"server"=>array("compulsory"=>true, "comment"=>"Ex:10.5.5.5:5060, 10.5.5.5:5061 It is IP address of the gateway : port number used for sip on that machine. If transport is TLS then 5061 is the default port, otherwise 5060 is the default."),
	"enabled"=>array("comment"=>"Check this field to mark that you wish to register to this server", "display"=>"checkbox"),
	"description"=>array("advanced"=>true, "comment"=>"Caller name to set on outgoing calls on this account if none specified when routing.", "column_name"=>"CallerName"),
	"ip_transport"=>array("display"=>"select","advanced"=>true, "column_name"=>"Transport", "comment"=>"Protocol used to register to gateway and sending calls. Default is UDP. If you use TLS keep in mind you might need to change the port value in 'Server' to 5061, as this is the default for TLS."),
//	"rtp_localip"=>array("comment"=>"IP address to bind the RTP to. This overwrittes setting from yrtpchan.conf, if set.", "advanced"=>true, "column_name"=>"RTP local IP"),
	"authname"=>array("advanced"=>true, "comment"=>"Authentication ID is an ID used strictly for authentication purpose when the phone attempts to contact the SIP server. This may or may not be the same as the above field username. Set only if it's different."), 
	"outbound"=>array("advanced"=>true, "comment"=>"An Outbound proxy is mostly used in presence of a firewall/NAT to handle the signaling and media traffic across the firewall. Generally, if you have an outbound proxy and you are not using STUN or other firewall/NAT traversal mechanisms, you can use it. However, if you are using STUN or other firewall/NAT traversal tools, do not use an outbound proxy at the same time."),
	"domain"=>array("advanced"=>true, "comment"=>"Domain in which the server is in."),
	"localaddress"=>array("advanced"=>true, "comment"=>"Insert when you wish to force a certain address to be considered as the default address. Set it to 'yes' to detect NAT and re-register with public IP when NAT is detected. Set it to 'no' or ipaddress (e.g. 1.2.3.4 or 1.2.3.4:5060) to disable NAT detection."),
	"interval"=>array("advanced"=>true, "comment"=>"Represents the interval in which the registration will expires. Default value is 600 seconds."),
	"formats"=>array("advanced"=>true,"display"=>"include_formats", "comment"=>"Codecs to be used. If none of the formats is checked then server will try to negociate formats automatically"), 
	"caller" => array(array("Use username", "Keep msisdn", "Custom", "selected"=>"Keep msisdn"), "advanced"=>true, "display"=>"select", "comment"=>"Caller parameter to be set when routing calls using this outbound connection.<br/>Use username - use same value as the account's username<br/>Keep msisdn - keep the msisdn of the user making the call<br/>Custom - insert custom caller to be used when routing call to this gateway"),
//	"rtp_forward"=> array("advanced"=>true,"display"=>"checkbox", "comment"=>"Check this box so that the rtp won't pass  through yate(when possible)."),

//	"callerid"=>array("advanced"=>true, "comment"=>"Use this to set the caller number when call is routed to this gateway. If none set then the System's CallerID will be used."),
//	"callername"=>array("advanced"=>true, "comment"=>"Use this to set the callername when call is routed to this gateway. If none set then the System's Callername will be used."),
	"ip_transport_remoteip"=>array("advanced"=>true,"comment"=>"IP address to connect to register the account. Defaults to outbound or registrar address."),
	"ip_transport_localip"=> array("advanced"=>true,"comment"=>"On UDP, this parameter is used in conjuction ip_transport_localport to identify the transport to use.On TCP/TLS, this is the local IP to use when connecting."),
	"ip_transport_localport" => array("advanced"=>true,"comment"=>"The local port used to identify the transport to use. It is used only for UDP."),
	"keepalive" => array("advanced"=>true,"comment"=> "Optional interval for NAT keep alive. Defaults to 0 if NAT detection is disabled"),
	"match_port" => array("advanced"=>true,"comment"=>"Match the UDP port for inbound calls from a Registrar.", "display"=>"checkbox", "value" => true),
	"match_user" => array("advanced"=>true,"comment"=>"Match the URI user for inbound calls from a Registrar.", "display"=>"checkbox", "value" => true)
);

$iax_fields = array(
	"username"=>array("compulsory"=>true, "autocomplete"=>"off"), 
	"password"=>array("comment"=>"Insert only when you wish to change"/*,"display"=>"password", "autocomplete"=>"off"*/),
	"server"=>array("compulsory"=>true, "comment"=>"Ex:10.5.5.5:4569 It is IP address of the gateway : port number used for IAX on that machine."),
	"port" => array("comment"=> "Registrar port, used if not specified in 'server' parameter. If is not set the default port is 4569."),			
	"enabled"=>array("comment"=>"Check this field to mark that you wish to register to this server", "display"=>"checkbox"),

	"interval"=>array("advanced"=>true, "comment"=>"Represents the interval in which the registration will expires. Default value is 600 seconds."), 
//	"formats"=>array("advanced"=>true,"display"=>"include_formats", "comment"=>"Codecs to be used. If none of the formats is checked then server will try to negociate formats automatically"), 
//	"callerid"=>array("advanced"=>true, "comment"=>"Use this to set the caller number when call is routed to this gateway. If none set then the System's CallerID will be used."),
	//	"callername"=>array("advanced"=>true, "comment"=>"Use this to set the callername when call is routed to this gateway. If none set then the System's Callername will be used."),
	"connection_id" => array("advanced"=>true, "comment"=>"The name of the iax listener to use for registration."),
	"ip_transport_localip" => array("advanced"=>true, "comment"=>"This parameter is used in conjuction ip_transport_localport to identify the listener to use for registration and outgoing calls."),
	"ip_transport_localport" => array("advanced"=>true, "comment"=>"Local port. This parameter is used to identify the listener"),
	"trunking" => array("advanced"=>true,"comment"=>"Enable trunking for outgoing calls sent on this line", "display"=>"checkbox"),
	"trunk_timestamps" => array("advanced"=>true,"comment"=>"Configure how trunked audio data is sent, enable it for trunked data with timestamps and disable it to send trunked data without timestamps", "display"=>"checkbox"),
	"trunk_sendinterval" => array("advanced"=>true,"comment"=>"Interval, in milliseconds, to send trunked audio data. Minimum allowed value is 5. Defaults to 20."),
	"trunk_efficient_use" => array("advanced"=>true,"comment"=>"Use the trunk efficiently: don't send trunking data when there is only 1 call using it", "display"=>"checkbox"),
	"trunk_maxlen" => array("advanced"=>true,"comment"=>"Maximum value for trunked data frames. Minimum allowed 20. Default value is 1400."),
	"trunk_nominits_sync_use_ts" => array("advanced"=>true, "comment"=>"Configure how to re-build timestamps when processing incoming trunked audio without miniframe timestamps. When enabled the transaction will use trunk timestamp and last received full voice frame time and timestamp to build miniframe timestamps. When disabled the transaction will use the time difference between current time and last received full voice frame to build the miniframe timestamps", "display"=>"checkbox"),
	"trunk_nominits_ts_diff_restart" => array("advanced"=>true, "comment"=>"The difference (in milliseconds) between current timestamp and first timestamp of incoming trunked audio data without miniframe timestamps at which to restart timestamps build data. Minimum allowed value is 1000. Default value is 5000.")
);

function outgoing($notice=false)
{
	$res = test_default_config();
	if (!$res[0]) {//permission errors
		errormess($res[1], "no");
		return;
	}

	$account = get_outgoing();

	if (!$account[0]) {
		message("Outbound connection not set: ".$account[1],"no");
		return edit_outgoing(false);
	} elseif (!isset($account[1]))
		// if outgoing is not configured
		return edit_outgoing(false);
	if ($notice)
		notice($notice, "no");
	display_outgoing($account[1]);
}

function display_outgoing($account)
{
	global $sip_fields, $iax_fields;

	$protocol = get_param($account,"protocol");

	if (!$protocol)
		return edit_outgoing();

	$fields = ${$protocol."_fields"};

	foreach ($fields as $name=>$def) {
		$fields[$name]["display"] = "fixed";
		$fields[$name]["value"] = get_param($account,$name);

		if (isset($def["comment"]))
			unset($fields[$name]["comment"]);
		if (isset($def["advanced"]) && $def["advanced"])
			unset($fields[$name]["advanced"]);
		if (!strlen($fields[$name]["value"]))
			unset($fields[$name]);
	}
	$fields = array_merge(array("protocol" => array("value"=>strtoupper($protocol),"display"=>"fixed")),$fields);
	if (isset($fields["password"]))
		$fields["password"]["value"] = "****";

	if ($protocol == "sip") {
		$switch = "Switch to IAX";
		$prot = "IAX";
	} else {
		$switch = "Switch to SIP";
		$prot = "SIP";
	}
	start_form(NULL,"post",false,"outbound");
	addHidden(null, array("method"=>"edit_outgoing", "switch_protocol"=> $prot));
	editObject(null,$fields,"Outbound connection",array("Modify",$switch),NULL,true);
	end_form();
}

function edit_outgoing($read_account=true, $error=NULL, $error_fields=array())
{
	global $method, $sip_fields, $iax_fields;

	$prot = getparam("switch_protocol");
	if (getparam("Switch_to_$prot"))
		return delete_and_switch_outgoing();

	$method = "edit_outgoing";

	$account = array();
	if ($read_account) {
		$account = get_outgoing();
		if (!$account[0]) {
			errormess($account[1],"no");
		} else
			$account = $account[1];
	}

	$ip_transport = array("UDP","TLS","TCP");
	$ip_transport["selected"] = (get_param($account,"ip_transport")) ? get_param($account,"ip_transport") : "UDP";
	$sip_fields["ip_transport"][0] = $ip_transport;

	$trigger_custom_field = false;

	start_form(NULL,"post",false,"outbound");
	if (!count($account)) {
		$protocol = getparam("regprotocol");
		$sip_fields["password"]["compulsory"] = true;
		$iax_fields["password"]["compulsory"] = true;
		unset($sip_fields["password"]["comment"]);
		unset($iax_fields["password"]["comment"]);

		$protocols = array("sip", "iax");
		$protocols["selected"] = $protocol;

		if($protocol)
		{
			$fields = $protocol."_fields";
			foreach(${$fields} as $fieldname=>$fieldformat)
			{
				$form_fieldname = "reg_".$protocol . $fieldname;
				$form_value  = getparam($form_fieldname);
				if($form_value) {
					if ($fieldformat!="select" && $fieldformat!="checkbox")
						${$fields}[$fieldname]["value"] = $form_value;
					elseif ($fieldformat=="checkbox")
						${$fields}[$fieldname]["value"] = ($form_value=="on") ? 1 : 0;
					else
						${$fields}[$fieldname][0]["selected"] = $form_value;
				}
			}
			if ($protocol=="sip") {
				${$fields}["formats"]["value"] = get_formats("reg_sipformats");
				${$fields}["caller"]["javascript"] = "onchange='custom_value_dropdown(\"\",\"reg_sipcaller\");'";
				${$fields}["caller"][0]["selected"] = getparam("reg_sipcaller");
			}
			/*$gateway->formats = get_formats("reg_".$protocol."formats");*/
			error_handle($error,${$fields},$error_fields);
		}

		addHidden("write_to_file",array("method"=>"eadd_outgoing"));
		//select protocol for gateway with registration
		?><div id="div_Yes"><?php
		editObject(NULL,
			array("protocol"=>array($protocols,"display"=>"select","javascript"=>'onChange="form_for_gateway(\'reg\');"')),
			"Select protocol outbound connection",
			"no",null,null,null,"reg");
		?></div><?php

		// display all the divs with fields for gateway with registration depending on the protocol
		for($i=0; $i<count($protocols); $i++)
		{
			if(!isset($protocols[$i]) || !isset(${$protocols[$i]."_fields"})) 
				continue;

			?><div id="div_reg_<?php print $protocols[$i]?>" style="display:<?php if ($protocol == $protocols[$i]) print "block;"; else print "none;";?>"><?php
			editObject(NULL,
				${$protocols[$i]."_fields"}, 
				"Define ".strtoupper($protocols[$i])." gateway", 
				"Save",true,null,null,"reg_".$protocols[$i]);
			?></div><?php
		}
		$custom_caller = getparam("custom_reg_sipcaller");
		if ($custom_caller) {
			?>
			<script> custom_value_dropdown("<?php print $custom_caller; ?>","reg_sipcaller"); </script>
			<?php
		}
	} else {
		$protocol = get_param($account,"protocol");
		$fields["protocol"] = array("value"=>strtoupper($protocol), "display"=>"fixed");

		$fields = array_merge($fields,${$protocol."_fields"});
		foreach($fields as $fieldname=>$fieldformat) {
			$file_value = get_param($account,$fieldname);
			if ($file_value) {
				if (!isset($fieldformat["display"]) ||
				    ($fieldformat["display"]!="select" && $fieldformat["display"]!="checkbox"))
					$fields[$fieldname]["value"] = $file_value;
				elseif ($fieldformat["display"]=="checkbox")
					$fields[$fieldname]["value"] = ($file_value=="yes") ? "1" : "0";
				else
					$fields[$fieldname][0]["selected"] = $file_value;
			}
		}
		if ($protocol=="sip") {
			$value = (isset($account["out:caller"])) ? $account["out:caller"] : "";
			if ($value) {
				if ($value!=$account["username"]) {
					$fields["caller"][0]["selected"] = "Custom";
					$trigger_custom_field = $value;
				} else
					$fields["caller"][0]["selected"] = "Use username";
			} else
				$fields["caller"][0]["selected"] = "Keep msisdn";

			$fields["caller"]["javascript"] = "onchange='custom_value_dropdown(\"$value\",\"caller\");'";
		}

		error_handle($error,$fields,$error_fields);
		addHidden("write_to_file",array("protocol"=>$protocol));
		editObject(NULL,$fields, "Edit outbound ".strtoupper($protocol)." gateway", "Save");
		if ($trigger_custom_field) {
			?>
			<script> custom_value_dropdown("<?php print $trigger_custom_field; ?>","caller"); </script>
			<?php
		}
	}
	end_form();
}

function eadd_outgoing_write_to_file()
{
	$protocol = getparam("regprotocol");
	return edit_outgoing_write_to_file("reg_".$protocol, "reg");
}

function edit_outgoing_write_to_file($prefix='',$prefix_protocol='')
{
	$protocol = getparam($prefix_protocol."protocol");

	$read_account = ($prefix=='') ? true : false;

	if (!$protocol || !in_array($protocol,array("sip","iax")))
		return edit_outgoing($read_account, "Please select 'Protocol'", array($prefix_protocol."protocol"));

	$params = array("protocol"=>$protocol);

	$compulsory = array("username", "server", "password");
	for($i=0; $i<count($compulsory); $i++) {
		$val = getparam($prefix.$compulsory[$i]);
		if (!$val)
			return edit_outgoing($read_account, "Field '".$compulsory[$i]."' is required.");
		$params[$compulsory[$i]] = getparam($prefix.$compulsory[$i]);
	}

	$sip = array('authname','outbound', 'domain', 'localaddress', 'description', 'interval', 'rtp_localip', 'ip_transport', 'ip_transport_remoteip', 'ip_transport_localip', 'ip_transport_localport', 'keepalive');
	$iax = array('description', 'interval', 'connection_id', 'ip_transport_localip', 'ip_transport_localport', 'trunk_sendinterval', 'trunk_maxlen', 'trunk_nominits_ts_diff_restart', 'port');
	
	for($i=0; $i<count(${$protocol}); $i++) {
		$value = getparam($prefix.${$protocol}[$i]);
		if ($value)
			$params[${$protocol}[$i]] = $value;
	}

	$params["enabled"] = (getparam($prefix."enabled")=="on") ? "yes" : "no";

	if ($params["protocol"] == 'sip') {
		$params["match_port"] = (getparam($prefix."match_port")=="on") ? "yes" : "no"; 
		$params["match_user"] = (getparam($prefix."match_user")=="on") ? "yes" : "no";
		$params["formats"] = get_formats($prefix."formats");
		$caller = getparam("caller");
		if ($caller=="Use username")
			$params["out:caller"] = $params["username"];
		elseif ($caller=="Custom")
			$params["out:caller"] = getparam("custom_caller");
		else
			$params["out:caller"] = "";
	} else {
		$params["trunking"] = (getparam($prefix."trunking")=="on") ? "yes" : "no";
		$params["trunk_timestamps"] = (getparam($prefix."trunk_timestamps")=="on") ? "yes" : "no";
		$params["trunk_efficient_use"] = (getparam($prefix."trunk_efficient_use")=="on") ? "yes" : "no";
		$params["trunk_nominits_sync_use_ts"] = (getparam($prefix."trunk_nominits_sync_use_ts")=="on") ? "yes" : "no";

	}
	$validate_results = validate_account($params);

	if(!$validate_results[0])
		return edit_outgoing($read_account, $validate_results[1], $validate_results[2]);


	$outgoing_file = get_outgoing();
	if (isset($outgoing_file[0]) && $outgoing_file[0]) {
		$params_modified = verify_modification_params($params, $outgoing_file[1]);

		if (!$params_modified) {
			outgoing();
			return;
		}
	}

	if (isset($params["out:caller"]) && $params["out:caller"]=="")
		unset($params["out:caller"]);

	$res = set_outgoing($params);
	if (!$res[0])
		return edit_outgoing($read_account, $res[1]);
	
	$res = restart_yate();

	if ($res[0] && isset($res[1])) //yate is not running
		outgoing("Finished setting outbound connection. " . $res[1]);
	elseif (!$res[0]) //errors on socket connection
		outgoing("Finished setting outbound connection. For changes to take effect please restart yate or reload just accfile module from telnet with command: \"reload accfile\".");
	else //yate was restarted
		outgoing("Finished setting outbound connection.");
}

function verify_modification_params($edited_params, $file_params)
{
	//$edited_params -> params from form
	//$file_params -> params from file 
	$modified = false;
	foreach ($edited_params as $name => $value) {
		if (!in_array($value,$file_params)) {
			$modified = true;
			break;
		}
		if (isset($file_params[$name]) && $value != $file_params[$name]) {
			$modified = true;
			break;
		}
	}
	return $modified;
}

function validate_account($params)
{
	if ($params['protocol'] == 'sip') {
		if ( !empty($params['localaddress']) && $params['localaddress'] == 'no' && $params['keepalive'] != 0)
			return array(false, "Invalid keepalive value if localaddress is not set.", array('keepalive', 'localaddress'));
		if ($params['ip_transport'] == 'UDP' && empty($params['ip_transport_localip']) && !empty($params['ip_transport_localport']))
			return array(false, "Field Ip transport localip must be set. This parameter is used in conjuction Ip transport localport to identify the transport to use.", array('ip_transport_localip'));
		if ($params['ip_transport'] == 'UDP' && !empty($params['ip_transport_localip']) && empty($params['ip_transport_localport']))
			return array(false, "Field Ip transport localport must be set. This parameter is used in conjuction Ip transport localip to identify the transport to use.", array('ip_transport_localport'));
		if ($params['ip_transport'] != 'UDP' && !empty($params['ip_transport_localport']))
			return array(false, "Invalid Transport. The Transport must be on UDP.", array('ip_transport'));
	} else {
		if (!empty($params['trunk_sendinterval']) && $params['trunk_sendinterval'] < 5)
			return array(false, "For Trunk sendinterval minimum allowed is 5", array('trunk_sendinterval'));
		if (!empty($params['trunk_maxlen']) && $params['trunk_maxlen'] < 20)
			return array(false, "For Trunk maxlen minimum allowed is 20", array('trunk_maxlen'));
		if (!empty($params['trunk_nominits_ts_diff_restart']) && $params['trunk_nominits_ts_diff_restart'] < 1000)
			return array(false, "For Trunk nominits ts diff restart minimum allowed is 1000", array('trunk_nominits_ts_diff_restart'));
		if (!empty($params['trunk_nominits_ts_diff_restart']) &&  $params['trunk_nominits_ts_diff_restart'] && $params['trunk_nominits_sync_use_ts'] == 'no')
			return array(false, "This parameter Trunk nominits ts diff restart is ignored because Trunk nominits sync use ts is disabled.", array('trunk_nominits_ts_diff_restart', 'trunk_nominits_sync_use_ts'));
	}

	return array(true);
}

function delete_and_switch_outgoing()
{
	global $method;
	$switch_protocol = getparam("switch_protocol");
	$method= "delete_and_switch_outgoing";
	ack_delete("the existing outbound connection to switch to $switch_protocol",null,null,"regprotocol","",strtolower($switch_protocol));
}

function delete_and_switch_outgoing_database()
{
	global $method;

	$method = "outgoing";
	$res = set_outgoing(array());
	if (!$res[0])
		errormess("Could not delete outbound connection: ".$res[1], "no");
	else
		message("Outbound connection was deleted.","no");
	edit_outgoing(false);
}

function get_outgoing()
{
	global $yate_conf_dir;

	$accfile_name = $yate_conf_dir."accfile.conf";
	if (!is_file($accfile_name))
	    return array(true);

	$accfile = new ConfFile($accfile_name);

	if (!$accfile->status())
		return array(false, $accfile->getError());

	$res = $accfile->getSection("outbound");
	if (!$accfile->status())
		return array(false, $accfile->getError());
	return array(true,$res);
}

function set_outgoing($account_info)
{
	global $yate_conf_dir;

	$accfile_name = $yate_conf_dir."accfile.conf";
	$accfile = new ConfFile($accfile_name,false);

	if (!$accfile->status())
		return array(false, $accfile->getError());

	if(count($account_info))
		$accfile->structure["outbound"] = $account_info;
	else
		$accfile->structure = array();
	$accfile->safeSave();
	if (!$accfile->status())
		return array(false, $accfile->getError());

	return array(true);
}

?>
