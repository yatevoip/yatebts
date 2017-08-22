<?php
/**
 * subscribers.php
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

function subscribers()
{
	list_subscribers();
}

function list_subscribers()
{
	global $yate_conf_dir;

	$res = test_default_config();
	if (!$res[0]) {//permission errors
		errormess($res[1], "no");
		return;
	}

	nib_note("Subscribers are accepted based on two criteria: regular expression that matches the IMSI or they be inserted individually.");

	$have_subscribers = true;
	$res = get_subscribers();

	if (!$res[0]) {
		$regexp = get_regexp();
		if ($regexp[0] && strlen($regexp[1])) {
			$regexp = $regexp[1];
			$have_subscribers = false;
		} else {
			if (!getparam("overwrite_file")) {
				errormess($res[1]. ". Press <a href=\"main.php?module=subscribers&method=list_subscribers&overwrite_file=yes\">here</a> to overwrite subscribers.conf.", "no");
				return;
			} else { 
				$new_file = $yate_conf_dir."subscribers.conf.backup".date("Ymdhm");
				rename($yate_conf_dir."subscribers.conf", $new_file);
				message("Your old old file was moved to $new_file", "no");
			}
			$subscribers = array();
		}
	} elseif (!count($res[1])) {
		$regexp = get_regexp();
		if ($regexp[0] && strlen($regexp[1])) {
			$regexp = $regexp[1];
			$have_subscribers = false;
		} else
			$subscribers = array();
	} else {
		//$_SESSION["have_subscribers"] = true;
		$subscribers = $res[1];
	}

	if ($have_subscribers) {
		$all_subscribers = array();$i=0;
		foreach($subscribers as $imsi=>$subscr) {
			$all_subscribers[$i] = $subscr;
			$all_subscribers[$i]["imsi"] = $imsi;
			$i++;
		}

		$formats = array("IMSI"=>"imsi","msisdn","short_number","ki","op","IMSI Type"=>"imsi_type","function_display_bit_field:active"=>"active", "function_write_subcriber_on_sim:"=>"imsi,ki");
		table($all_subscribers, $formats, "subscriber", "imsi", array("&method=edit_subscriber"=>"Edit","&method=delete_subscriber"=>"Delete"), array("&method=add_subscriber"=>"Add subscriber", "&method=edit_regexp"=>"Accept by REGEXP", "&method=export_subscribers_in_csv"=>"Export subscribers", "&method=import_subscribers"=>"Import subscribers"));
	} else {
		start_form();
		addHidden(null, array("method"=>"edit_regexp", "regexp"=>$regexp));
		$fields = array("regexp"=>array("value"=>$regexp, "display"=>"fixed"));
		editObject(null,$fields,"Regular expression based on which subscriber IMSI are accepted for registration",array("Modify", "Set subscribers"),null,true);
		end_form();
	}
}

function online_subscribers()
{
	$command = "nib list registered";
	$marker_end = "null";
	$res = get_socket_response($command, $marker_end);

	if (isset($res[0]) && $res[0]===false) {
		if (isset($res[2])) {
			nib_note($res[1]);
			return;
		}
		errormess($res[1],"no");
		$online_subscribers = array();
	} else
		$online_subscribers = parse_socket_response($res);

	$formats = array("IMSI","MSISDN");
	table($online_subscribers, $formats, "online subscriber", "imsi");
}

function rejected_imsis()
{
	$command = "nib list rejected";
	$marker_end = "null";
	$res = get_socket_response($command, $marker_end);

	if (isset($res[0]) && $res[0]===false) {
		if (isset($res[2])) {
                       nib_note($res[1]);
                       return;
                }
		errormess($res[1],"no");
		$rejected_subscribers = array();
	} else
		$rejected_subscribers = parse_socket_response($res);

	$formats = array("IMSI","No attempts register");
	table($rejected_subscribers, $formats, "rejected IMSIs", "imsi");
}

function edit_regexp($error=null,$error_fields=array())
{
	global $yate_conf_dir;

	if (getparam("Set_subscribers")=="Set subscribers")
		return edit_subscriber();

	$regexp = getparam("regexp");

	$ybts_file = new ConfFile($yate_conf_dir."ybts.conf");

	$warning_data = array();
	if (isset($ybts_file->structure["security"]["auth.call"])) 
		$warning_data[] = $ybts_file->structure["security"]["auth.call"];
	if (isset($ybts_file->structure["security"]["auth.sms"]))
		$warning_data[] =  $ybts_file->structure["security"]["auth.sms"];
	if (isset($ybts_file->structure["security"]["auth.ussd"]))
		$warning_data[] = $ybts_file->structure["security"]["auth.ussd"];

	if (in_array("yes", $warning_data))
		warning_note("You can't set mobile terminated authentication for calls, SMS, USSD when regular expression is used. Authentication requests will be ignored in this scenario.");

	nib_note("If a regular expression is used, 2G/3G authentication cannot be used. For 2G/3G authentication, please set subscribers individually.");
	$fields = array(
		"regexp" => array("value"=> $regexp, "required"=>true, "comment"=>"Ex: ^001")
	);
	error_handle($error,$fields,$error_fields);
        start_form();
        addHidden("write_file");
        editObject(NULL,$fields,"Regular expression based on which subscriber IMSI are accepted for registration","Save");
        end_form();
}

function edit_regexp_write_file()
{
	$expressions = array();
	$res = get_regexp(true);
	if (!$res[0]) 
		errormess($res[1], "no");
	else {
		if (is_array($res[1]) && isset($res[1]["regexp"]))
			$expressions = array($res[1]["regexp"]);
		$cc = array();
		if (isset($res[1]["country_code"]))
			$cc["country_code"] = $res[1]["country_code"];
		if (isset($res[1]["smsc"]))
			$cc["smsc"] = $res[1]["smsc"];
	}
	$regexp = getparam("regexp");

	if (!$regexp)
		return edit_regexp("Please set the regular expression!",array("regexp"));

	if (in_array($regexp, $expressions)) {
		notice("Finished setting regular expression.", "subscribers");
		return;
	}	

	//$write_regexp[count($expressions)] = $regexp;
	$res = set_regexp($regexp, $cc);
	if (!$res[0])
		return edit_regexp($res[1]);

	$res = restart_yate();

	if ($res[0] && isset($res[1])) //yate is not running
		notice("Finished setting regular expression. " .$res[1], "subscribers");
	elseif (!$res[0]) //errors on socket connection
		notice("Finished setting regular expression. For changes to take effect please restart yate or reload just nib.js from telnet with command: \"javascript reload nib\".Please note that after this you will lose existing registrations.", "subscribers");
	else //yate was restarted
		notice("Finished setting regular expression", "subscribers");
}

function country_code_and_smsc()
{
	global $yate_conf_dir;

	$filename = $yate_conf_dir."subscribers.conf";
	$res = get_cc_smsc();
	$country_code = "";
	$smsc = "";
	$gw_sos = "";
	if (is_array($res[1])) {
		$country_code = $res[1]["country_code"];
		$smsc = $res[1]["smsc"];
		$gw_sos = $res[1]["gw_sos"];
	}

	if (!is_file($filename) || !strlen($country_code)) 
		edit_country_code_and_smsc();
	else {
		$fields = array(
			"country_code"=>array("value"=>$country_code, "display"=>"fixed"),
			"smsc" => array("value"=>$smsc, "display"=>"fixed", "column_name"=>"SMSC"),
			"gw_sos" => array("value"=>$gw_sos, "display"=>"fixed", "column_name"=>"Gateway SOS")
		);
		start_form();
		addHidden(null,array("method"=>"edit_country_code_and_smsc", "country_code"=>$country_code, "smsc"=>$smsc, "gw_sos"=>$gw_sos));
		editObject(null,$fields,"Country code and SMSC for the majority of your subscribers.",array("Modify"),null,true);
		end_form();
	}
}

function edit_country_code_and_smsc($error=null,$error_fields=array())
{
	global $method;

	$method = "edit_country_code_and_smsc";

	$country_code = getparam("country_code");
	$smsc = getparam("smsc");
	$gw_sos = getparam("gw_sos");
	$fields = array(
		"country_code"=>array("value"=>$country_code, "compulsory"=>true, "comment"=>" Your Country code (where YateBTS is installed). Ex: 1 for US, 44 for UK"),
		"smsc"=>array("column_name"=>"SMSC", "value"=>$smsc, "compulsory"=>true, "comment"=>"A short message service center (SMSC) used to store, forward, convert and deliver SMS messages."),
		"gw_sos"=>array("column_name"=>"Gateway SOS", "value"=>$gw_sos, "comment"=>"Resource for the emergency calls gateway.<br/>
If not set any emergency calls will be delivered to the outbound gateway<br/>
It is also possible to specify a short or international number (possibly MSISDN)<br/>
Ex: gw_sos=sip/sip:sos@emergency.gw<br/>
Ex: gw_sos=111<br/>
Ex: gw_sos=+10744341111")
	);

	error_handle($error,$fields,$error_fields);
	start_form();
	addHidden("write_file");
	editObject(NULL,$fields,"Set Country Code and SMSC","Save");
	end_form();
}

function edit_country_code_and_smsc_write_file()
{
	$cc_file = array();
	$res = get_cc_smsc();

	if (!$res[0])
		errormess($res[1], "no");
	else
		$cc_file = $res[1];

	$cc_param = getparam("country_code");
	$smsc_param = getparam("smsc");
	$gw_sos_param = getparam("gw_sos");

	if ($gw_sos_param) {
		warning_note("In order to route emergency calls you also need to set RACH.AC to '0'(or another value as stated in GSM 04.08 10.5.2.29) in BTS Configuration>GSM>GSM advanced. <font class='error'>DON'T MODIFY \"Gateway SOS\" UNLESS YOU ARE A REAL OPERATOR</font>. You might catch real emergency calls than won't ever be answered.");
	}

	if (!$cc_param)
		return edit_country_code_and_smsc("Please set the country code!", array("country_code"));
	if (!ctype_digit($cc_param))
		return edit_country_code_and_smsc("Country Code invalid!", array("country_code"));
	if (!$smsc_param)
		return edit_country_code_and_smsc("Please set SMSC!", array("smsc"));
	if (is_array($cc_file) && in_array($cc_param, $cc_file) && in_array($smsc_param, $cc_file) && in_array($gw_sos_param, $cc_file)) {
		notice("Finished setting Country Code and SMSC.", "country_code_and_smsc");
		return;
	}

	$res = set_cc_smsc($cc_param,$smsc_param,$gw_sos_param);
	if (!$res[0])
		return edit_country_code_and_smsc($res[1]);
	notice("Finished writting Country Code and SMSC into subscribers.conf.", "country_code_and_smsc");
}

function get_cc_smsc()
{
	global $yate_conf_dir;

	$filename = $yate_conf_dir."subscribers.conf";

	if (!is_file($filename))
		return array(true,"");

	$subs_file = new ConfFile($yate_conf_dir."subscribers.conf");
	
	$content = array();
	foreach ($subs_file->sections as $name => $value) {
		if ($name == "general") {
			$content["country_code"] = isset($value["country_code"]) ? $value["country_code"] : "";
			$content["smsc"] = isset($value["smsc"]) ? $value["smsc"] : "";
			$content["gw_sos"] = isset($value["gw_sos"]) ? $value["gw_sos"] : "";
		}
	}

	if (!count($content))
		return array(true, "");

	return array(true, $content);
}

function set_cc_smsc($country_code, $smsc, $gw_sos)
{
	global $yate_conf_dir, $global_comment, $country_code_comment, $regexp_comment, $subscriber_comment, $subscriber_example, $gw_sos_comment;

	//if file subscribers doesn't exist; create the file with adecvate comments
	if (!is_file($yate_conf_dir."subscribers.conf" )) {
		$subs_file = new ConfFile($yate_conf_dir."subscribers.conf", false);
		$subs_file->initial_comment = $global_comment;
		$content[] = rtrim($country_code_comment);
		$content["country_code"] = $country_code."\n";
		$content["smsc"] = $smsc."\n";
		$content[] = rtrim($gw_sos_comment);
		$content["gw_sos"] = $gw_sos."\n";
		$content[] = rtrim($regexp_comment);
		$content[] = $subscriber_comment;
		$content[] = $subscriber_example;	
		$subs_file->structure["general"] = $content;
	} else {
		$subs = new ConfFile($yate_conf_dir."subscribers.conf");
		$old_data = array();
		$have_regex = false;
		foreach ($subs->sections as $name => $value) {
			if ($name == "general" && isset($value["regexp"])) {
				$have_regex = true;
				$regex = $value["regexp"];
			}
			if ($name == "general")
				continue;

			$old_data[$name] = $value;
		}
		if (!$have_regex) {
			$subs_file = new ConfFile($yate_conf_dir."subscribers.conf", false);
			$subs_file->initial_comment = $global_comment;
			$content[] = rtrim($country_code_comment);
			$content["country_code"] = $country_code."\n";
			$content["smsc"] = $smsc."\n";
			$content[] = rtrim($gw_sos_comment);
			$content["gw_sos"] = $gw_sos."\n";
			$content[] = $regexp_comment;
			$content[] = $subscriber_comment;
			$subs_file->structure["general"] = $content;

			foreach ($old_data as $name => $value)
				$subs_file->structure[$name] = $value;
		} else {
			$subs_file = new ConfFile($yate_conf_dir."subscribers.conf", false);
			$subs_file->initial_comment = $global_comment;
			$content[] = rtrim($country_code_comment);
			$content["country_code"] = $country_code."\n";
			$content["smsc"] = $smsc."\n";
			$content[] = rtrim($gw_sos_comment);
			$content["gw_sos"] = $gw_sos."\n";
			$content[] = rtrim($regexp_comment);
			$content["regexp"] = $regex."\n";
			$content[] = $subscriber_comment;
			$content[] = $subscriber_example;
			$subs_file->structure["general"] = $content;
		}
	}
	$subs_file->openForWrite();
	if (!$subs_file->status())
		return array(false, $subs_file->getError());

	$subs_file->save();

	return array(true);
}

function edit_subscriber($error=null,$error_fields=array())
{
	global $method;
	$method = "edit_subscriber";

	$imsi = getparam("imsi") ? getparam("imsi") : getparam("imsi_val");
	
	if ($imsi) {
		$subscriber = get_subscriber($imsi);
		if (!$subscriber[0]) {
			if (!$error)
				// if there is not error, print message that we didn't find subscriber
				// otherwise if was probably an error when adding one so this message is not relevant
				errormess($subscriber[1], "no");
			$subscriber = array();
		}if (isset($subscriber[1]))
			$subscriber = $subscriber[1];
		else
			$subscriber = array();
	} else
		$subscriber = array();

	$imsi_type = array("selected"=> "2G", "2G", "3G");

	if (get_param($subscriber,"imsi_type"))
		$imsi_type["selected"] = get_param($subscriber,"imsi_type");
	$active = (get_param($subscriber,"active") == "on") ? 't' : 'f';
	$op = get_param($subscriber,"op") ? get_param($subscriber,"op") : "00000000000000000000000000000000";

	$fields = array(
		"imsi"   => array("value"=>$imsi, "required"=>true, "comment"=>"SIM card id", "column_name"=>"IMSI"),
		"msisdn" => array("value"=>get_param($subscriber,"msisdn"), "comment"=>"DID associated to this IMSI. When outside call is made, this number will be used as caller number.", "column_name"=>"MSISDN"),
		"short_number" => array("value" => get_param($subscriber,"short_number"),"comment"=>"Short number that can be used to call this subscriber."),
		"active" => array("value"=>$active, "display"=>"checkbox"),
		"imsi_type" => array($imsi_type, "display"=>"select", "column_name"=>"IMSI Type", "required"=>true, "comment"=> "Type of SIM associated to the IMSI", "javascript" => 'onclick="show_hide_op()"'),
		"ki" => array("value"=>get_param($subscriber,"ki"), "comment"=>"Card secret. You can use * to disable authentication for this subscriber.", "required"=>true),
		"op" => array("value"=>$op, "triggered_by"=>"imsi_type", "comment"=>"Operator secret. Empty for 2G IMSIs.<br/>00000000000000000000000000000000 for 3G IMSIs.")
	);
	
	if ($imsi && count($subscriber) && !in_array("imsi",$error_fields))
		$fields["imsi"]["display"] = "fixed";
	if ($imsi && $imsi_type["selected"] == "3G")
		unset($fields["op"]["triggered_by"]);
	if (!count($subscriber))
		$imsi = NULL;

	error_handle($error,$fields,$error_fields);
	start_form();
	addHidden("write_file", array("imsi_val"=>$imsi));
	editObject(NULL,$fields,"Set subscriber","Save");
	end_form();
}

function edit_subscriber_write_file()
{
	$res = get_subscribers(true);

	if (!$res[0]) {
		//errormess($res[1], "no");
		$subscribers = array();
	} else {
		$subscribers = $res[1];
		if (isset($res[1]["general"])) {
			$general = $res[1]["general"];
			unset($subscribers["general"]);
		}
	}

	$imsi = (getparam("imsi")) ? getparam("imsi") : getparam("imsi_val");
	if (!$imsi)
		return edit_subscriber("Please set 'imsi'",array("imsi"));
	if (strlen($imsi)!=14 && strlen($imsi)!=15)
		return edit_subscriber("Invalid IMSI $imsi. IMSI length must be 14 or 15 digits long.",array("imsi"));
	if (getparam("ki")!="*" && !preg_match('/^[0-9a-fA-F]{32}$/i', getparam("ki")))
		return edit_subscriber("Invalid KI:".getparam("ki").". KI needs to be 128 bits, in hex format.");
	

	$subscriber = array("imsi"=>$imsi);

	$fields = array("msisdn"=>false, "short_number"=>false, "active"=>false, "ki"=>true, "op"=>false, "imsi_type"=>true);
	foreach ($fields as $name=>$required) {
		$val = getparam($name);
		if ($required && !$val)
			return edit_subscriber("Field $name is required");
		$subscriber[$name] = $val;
	}
//	$subscriber["active"] = ($subscriber["active"]=="on") ? 1 : 0;
	if ($subscriber["imsi_type"]=="2G")
		$subscriber["op"] = "";
	if (getparam("imsi_type")=="3G" && (getparam("op")==NULL || getparam("op")==""))
		return edit_subscriber("OP can't be empty!", array("op"));

	if (getparam("imsi_val") && isset($subscribers[$imsi])) {
		$modified = false;
		//check if there are changes
		$subs_file = $subscribers[$imsi];
		foreach ($fields as $name=>$required) {
			$val = getparam($name);
		//	if ($name == "active")
		//		$val = (getparam($name) == "on") ? 1 : 0;

			if ($subs_file[$name] != $val) 
				$modified = true;
			if ($modified)
				break;
		}

		if (!$modified) {
			notice("Finished setting subscriber with IMSI $imsi.", "subscribers");
			return;
		}
	}

	if (!getparam("imsi_val") && isset($subscribers[$imsi]))
		return edit_subscriber("Subscriber with IMSI $imsi is already set.",array("imsi"));
	$subscribers[$imsi] = $subscriber;

	$res = set_subscribers($subscribers, $general);
	if (!$res[0])
		return edit_subscriber($res[1]);

	$res = restart_yate();

	if ($res[0] && isset($res[1])) //yate is not running
		notice("Finished setting subscriber with IMSI $imsi. " .$res[1], "subscribers");
	elseif (!$res[0]) //errors on socket connection
		notice("Finished setting subscriber with IMSI $imsi. For changes to take effect please restart yate or reload just nib.js from telnet with command: \"javascript reload nib\".Please note that after this you will lose existing registrations.", "subscribers");
	else //yate was restarted
		notice("Finished setting subscriber with IMSI $imsi.", "subscribers");
}

function delete_subscriber()
{
	ack_delete("subscriber", getparam("imsi"), NULL, "imsi", getparam("imsi"));
}

function delete_subscriber_database()
{
	$imsi = getparam("imsi");
	$res = get_subscribers(true);
	if (!$res[0])
		return errormess($res[1], "no");
	$subscribers = $res[1];
	$cc = array();
	if (isset($res[1]["general"])) {
		$cc = $res[1]["general"];
		unset($subscribers["general"]);
	}

	if (!isset($subscribers[$imsi]))
		return errormess("IMSI $imsi is not in subcribers list.","no");

	unset($subscribers[$imsi]);
	$res = set_subscribers($subscribers, $cc);
	if (!$res[0]) 
		return errormess($res[1],"no");

	notice("Finished removing subscriber with IMSI $imsi.", "subscribers");
}

function set_subscribers($subscribers, $general = array())
{
	global $yate_conf_dir, $global_comment, $country_code_comment, $regexp_comment, $subscriber_comment;

	$subs_file = new ConfFile($yate_conf_dir."subscribers.conf", false);

	$subs_file->initial_comment = $global_comment;

	$cc[0] = $country_code_comment;

	if (isset($general["country_code"])) {
		$cc[0] = rtrim($country_code_comment);
		$cc["country_code"] = $general["country_code"]."\n";
	}
	
	if (isset($general["smsc"]))
		$cc["smsc"] = $general["smsc"]."\n";

	$cc[1] = $regexp_comment;
	$cc[2] = $subscriber_comment;

	$subs_file->structure["general"] = $cc;

	foreach ($subscribers as $imsi => $data_imsi) {
		if (isset($data_imsi["imsi"]))
			unset($data_imsi["imsi"]);
		$subs_file->structure[$imsi] = $data_imsi;
	}

	$subs_file->openForWrite();
	if (!$subs_file->status())
		return array(false, $subs_file->getError());

	$subs_file->save();

	return array(true);
}

function set_regexp($regexp, $general = array())
{
	global $yate_conf_dir, $global_comment, $country_code_comment, $regexp_comment, $subscriber_comment, $subscriber_example;

	$subs_file = new ConfFile($yate_conf_dir."subscribers.conf", false);

	$subs_file->initial_comment = $global_comment;

	$cc[0] = $country_code_comment;
	if (count($general)) {
		$cc[0] = rtrim($country_code_comment);
		$cc["country_code"] = $general["country_code"]."\n";
		if (isset($general["smsc"]))
			$cc["smsc"] = $general["smsc"]."\n"; 
	}
	$cc[1] = rtrim($regexp_comment);
	$cc["regexp"] = $regexp."\n";
        $cc[2] = $subscriber_comment;
	$cc[3] = $subscriber_example;

        $subs_file->structure["general"] = $cc;

	$subs_file->openForWrite();
	if (!$subs_file->status())
                return array(false, $subs_file->getError());

	$subs_file->save();

	return array(true);
}

function get_subscribers($keep_country_code = false)
{
	global $yate_conf_dir, $global_comment, $regexp_comment, $subscriber_comment;

        $subs_file = new ConfFile($yate_conf_dir."subscribers.conf");

	$subscribers = array();
	foreach ($subs_file->sections as $name => $value) {
		if (!$keep_country_code && $name == "general") 
			continue;
		$subscribers[$name] = $value;
	}

	return array(true, $subscribers);
}

function get_regexp($keep_country_code = false)
{
	global $yate_conf_dir;

	$filename = $yate_conf_dir."subscribers.conf";

	$subs_file = new ConfFile($filename);

	if (!is_file($filename))
		// if subscribers.conf file doesn't exist don't return error, just empty string
		return array(true,"");


	$content = array();
	foreach ($subs_file->sections as $name => $value) {
		if ($name !== "general")
			continue;
     
		if ($keep_country_code) {
			if (isset($value["country_code"])) 
				$content["country_code"] = $value["country_code"]; 
			if (isset($value["smsc"]))
				$content["smsc"] = $value["smsc"];
			if (isset($value["regexp"]))
				$content["regexp"] = $value["regexp"];
		} else {
	     		if (isset($value["regexp"]))
				$content = $value["regexp"];
		}
	}
	if (!count($content))
		 return array(true, "");
	
	return array(true, $content);
}

function get_subscriber($imsi)
{
	$subscribers = get_subscribers();
	if (!$subscribers[0])
		return $subscribers;
	else
		$subscribers = $subscribers[1];

	if (!isset($subscribers[$imsi]))
		return array(false, "Could not find subcriber with imsi $imsi");
	return array(true, $subscribers[$imsi]);
}

function detect_pysim_installed()
{
	global $pysim_path;

	if (!have_pysim_prog())
		return array(false, "Please install pySIM and create file config.php to set the location for pySIM after instalation. E.g. \$pysim_path = \"/usr/bin\";");

	$pysim_prog_path = have_pysim_prog();
	$pysim_real_path = str_replace(array("/pySim-prog.py","\n"), "", $pysim_prog_path);

	if (!isset($pysim_path)) 
                $pysim_path = $pysim_real_path;

        if (!is_file($pysim_path.'/'.'pySim-prog.py')) 
                return array(false, "The path for pySIM set in configuration file is not correct. Please set in file config.php the right location for pySIM. This path was detected: \$pysim_path = \"$pysim_real_path\";");
	
	return array(true);
}

function manage_sims()
{
	global $pysim_csv;

	$pysim_installed = detect_pysim_installed();

	if (!$pysim_installed[0]) {
		errornote($pysim_installed[1]);
		return;
	}
	
	$all_sim_written = read_csv();

	$formats = array("IMSI"=>"imsi", "ICCID"=>"iccid", "operator_name", "mobile_country_code", "mobile_network_code", "ki", "opc", "function_display_add_into_subscribers:"=>"imsi,ki");
?>
	<div>
		<a class="write_sim" href="main.php?module=subscribers&method=write_sim_form"><img title="SIM Programmer" src="images/sim_programmer.png" alt="SIM Programmer" /></a>
	</div>
<?php
	table($all_sim_written, $formats, "written SIM", "sim");
	if (file_exists($pysim_csv)) {
		?><div class="download_file"><a href="download.php" class="content">Download csv file with all written SIMs</a></div><?php
	}
}

function display_add_into_subscribers($imsi, $ki)
{
	global $module, $main;

	$subscribers = get_subscriber($imsi);

	if (!$subscribers[0]) {
		$link = $main."?module=$module&method=write_imsi_in_subscribers";
		$link .= "&imsi=".$imsi. "&ki=".$ki;	
		print "<a class=\"content\" href=\"$link\">Add in subscribers</a>";
	}else  
		print "";
}

function write_sim_form($error=null,$error_fields=array(), $generate_random_imsi = true, $insert_subscribers = true, $add_existing_subscriber=false, $params = array())
{
	global $yate_conf_dir, $sim_type, $method;

	$method = "write_sim_form";
	$pysim_installed = detect_pysim_installed();

        if (!$pysim_installed[0]) {
                errornote($pysim_installed[1]);
                return;
        } 

	$filename = $yate_conf_dir."ybts.conf";

	$file = new ConfFile($filename);

	$res = get_cc_smsc();

	$cc = $smsc = "";
	if ($res[0] && is_array($res[1])) {
		$cc = $res[1]["country_code"];
		$smsc = $res[1]["smsc"];
	}

	$mcc = "001";
	$mnc = "01";
	$advanced_mcc = $advanced_mnc = $advanced_op = $advanced_smsc = false;
	if (isset($file->structure["gsm"]["Identity.MCC"])) {
		$mcc = $file->structure["gsm"]["Identity.MCC"];
		$advanced_mcc = true;
	}
	if (isset($file->structure["gsm"]["Identity.MNC"])) {
		$mnc = $file->structure["gsm"]["Identity.MNC"];
		$advanced_mnc = true;
	}
	if (isset($file->structure["gsm"]["Identity.ShortName"])) {
		$op = $file->structure["gsm"]["Identity.ShortName"];
		$advanced_op = true;
	}
	$params["smsc"] = get_smsc();
	
	if (!empty($params["smsc"]))
		$advanced_smsc = true;


	if (!$add_existing_subscriber) {
		nib_note("There are two methods of writing the SIM cards, depending on the state of the \"Generate random IMSI\" field. If the field is selected, the SIM credentials are randomly generated. Otherwise, the data must be inserted manually. Please check that your SIM Card Reader is inserted into the USB port of your device. Before saving data, please insert a SIM card into the SIM Card Reader.");
	} else {
		if (test_existing_imsi_in_csv($params["imsi"]))
			nib_note("This IMSI: ".$params["imsi"]." is already written on another SIM card.");
	}

	$type_card = get_card_types();
	$type_card["selected"] = $sim_type; 

	if (!$cc) 
		$fields["country_code"] = array("required" => true, "value"=>$cc, "comment" => "Your Country code (where YateBTS is installed). Ex: 1 for US, 44 for UK");

	if (!$add_existing_subscriber) {
		$fields["generate_random_imsi"] =  array("comment" => "Checked - if you want the parameter for the card to be generated randomly or uncheck - to insert your card values manually", "column_name"=>"Generate random IMSI", "javascript" => 'onclick="show_hide_cols()"', "display"=>"checkbox", "value"=>$generate_random_imsi);
		//show/hide fields when generate_random_imsi is unselected/selected
		$fields["imsi"] = array("required"=>true,"column_name"=>"IMSI", "comment" => "Insert IMSI to be written to the card. Ex.:001011641603116", "triggered_by"=>"generate_random_imsi");
		$fields["iccid"] = array("required"=>true,"column_name"=>"ICCID", "comment" => "Insert ICCID(Integrated Circuit Card Identifier) to be written to the card. Ex.: 8940001017992212557", "triggered_by"=>"generate_random_imsi");
		$fields["ki"] = array("required"=>true,"column_name"=>"Ki", "comment" => "Insert Ki to be written to the card. Ex.: 3b07f45b11d2003247e9ae6f13de7573", "triggered_by"=>"generate_random_imsi");
		$fields["opc"] = array("required"=>true,"column_name"=>"OPC", "comment" => "Insert OPC to be written to the card. Ex.: 6cb49bb6f99e97c3913924e7a1f32650", "triggered_by"=>"generate_random_imsi");
	
		if ($params["smsc"] == "") {
			$fields["smsc"] = array("required"=>true, "column_name"=>"SMSC", "comment"=>"Short message server center.", "advanced"=>true, "javascript"=>"onClick=advanced('sim')", "triggered_by"=>"generate_random_imsi");
		}
	} else {
		$fields["imsi"] = array("column_name"=>"IMSI", "value"=> $params["imsi"],"display"=>"fixed");
		$fields["iccid"] = array("required"=>true,"column_name"=>"ICCID", "value"=> $params["iccid"], "comment" => "Insert ICCID(Integrated Circuit Card Identifier) to be written to the card. Ex.: 8940001017992212557.");
		$fields["ki"] = array("display"=>"fixed", "column_name"=>"Ki", "value" => $params["ki"]);
		$fields["opc"] = array("required"=>true,"column_name"=>"OPC", "value"=> $params["opc"], "comment" => "Insert OPC to be written to the card. Ex.: 6cb49bb6f99e97c3913924e7a1f32650.");
		if ($params["smsc"] == "") {
			$fields["smsc"] = array("required"=>true, "column_name"=>"SMSC", "comment"=>"Short message server center.");
		}
	}
	if (!$add_existing_subscriber) {
		$fields["insert_subscribers"] = array("comment" => "Uncheck if you don't want SIM credentials to be written in subscribers.js.", "display"=>"checkbox", "value" => $insert_subscribers); 
	}
	//advanced fields if they are set in ybts.conf file
	$fields["operator_name"] = $advanced_op ? array("required" => true,"advanced"=> true, "value" =>$op, "comment" => "Set Operator name on SIM.") : array("required" => true, "comment" => "Set Operator name on SIM.");
	if ($cc)
		$fields["country_code"] = array("required" => true, "value"=>$cc, "comment" => "Your Country code (where YateBTS is installed). Ex: 1 for US, 44 for UK", "advanced"=> true);
	$fields["card_type"] = array($type_card,"advanced"=> true, "required"=>true, "display"=>"select", "column_name"=> "Card Type", "comment" =>" Select the card type for writing SIM credentials. The SIM cards that you received are \"GrcardSim\". For other card types, see the list of cards supported by PySim. It is not guaranteed that your card will be written, even if it is in that list."); 
	$fields["mobile_country_code"] = $advanced_mcc ? array("required" => true,"advanced"=> true, "value" => $mcc, "comment" => "Set Mobile Country Code.", "javascript"=>"onClick=advanced('sim')") :
							array("required" => true,"value" => $mcc, "comment" => "Set Mobile Country Code.");
	$fields["mobile_network_code"] = $advanced_mnc ? array("required" => true,"advanced"=> true, "value" => $mnc, "comment" => "Set Mobile Network Code.", "javascript"=>"onClick=advanced('sim')") :
							array("required" => true,"value" => $mnc, "comment" => "Set Mobile Network Code.");
	
	if (strlen($params["smsc"])>0){
		$fields["smsc_adv"] = array("required"=>true, "column_name"=>"SMSC", "comment"=>"Short message server center.", "value"=>$params["smsc"], "advanced"=>true, "javascript"=>"onClick=advanced('sim')");
	}
	if ($generate_random_imsi != "on") {
		unset($fields["imsi"]["triggered_by"]);
		unset($fields["iccid"]["triggered_by"]);
		unset($fields["ki"]["triggered_by"]);
		unset($fields["opc"]["triggered_by"]);
		unset($fields["smsc"]["triggered_by"]);
	}
	
	error_handle($error,$fields,$error_fields);
	start_form(NULL,"post",false,"outbound");
	if ($add_existing_subscriber) 
		addHidden("to_pysim", array("generate_random_imsi"=>$generate_random_imsi, "add_existing_subscriber"=>$add_existing_subscriber, "imsi"=>$params["imsi"], "ki"=>$params["ki"]));
	else
		addHidden("to_pysim", array("generate_random_imsi"=>$generate_random_imsi));
	editObject(NULL,$fields,"Set SIM data for writting","Save");
	end_form();
}

function get_card_types()
{
	$type_card = array(
		array('card_type_id'=>'fakemagicsim', 'card_type'=>'FakeMagicSim'),
		array('card_type_id'=>'supersim', 'card_type'=>'SuperSim', ),
		array('card_type_id'=>'magicsim', 'card_type'=>'MagicSim'),
		array('card_type_id'=>'grcardsim','card_type'=>'GrcardSim'),
		array('card_type_id'=>'sysmosim-gr1', 'card_type'=>'Sysmocom SysmoSIM-GR1'),
		array('card_type_id'=>'sysmoSIM-GR2', 'card_type'=>'Sysmocom SysmoSIM-GR2' ), 
		array('card_type_id'=>'sysmoUSIM-GR1', 'card_type'=>'Sysmocom SysmoUSIM-GR1'),
		array('card_type'=>'auto','card_type_id'=>'auto')//autodetection is implemented in PySim/cards.py only for classes: FakeMagicSim, SuperSim, MagicSim the other types of card will fail (at this time 2014-04-16)
	);
	return $type_card;
}

function write_sim_form_to_pysim()
{
	global $yate_conf_dir, $method;

	$error = "";
	$params = array("operator_name","country_code","mobile_country_code","mobile_network_code", "card_type");
	foreach ($params as $key => $param) {
		if (!getparam($param)) {
			$error .= "This parameter '". ucfirst(str_replace("_"," ",$param)). "' cannot be empty!<br/>\n";
			$error_fields[] = $param;
		} elseif ($param != "operator_name" && $param != "card_type" && !ctype_digit(getparam($param))) {
			$error .= "Invalid integer value for parameter '". ucfirst(str_replace("_"," ",$param)). "': ". getparam($param). ".<br/> \n";
			$error_fields[] = $param;
		} elseif ($param == "mobile_country_code" && (strlen(getparam($param))>4 || getparam($param) <= 0 || getparam($param) >= 999)) {
			$error .= "Mobile Country Code value must be between 0 and 999. <br/>\n";
			$error_fields[] = $param;
		} elseif ($param == "mobile_network_code" && (strlen(getparam($param))>4 || getparam($param) <= 0 || getparam($param) >= 999)) {
			$error .= "Mobile Network Code value must be between 0 and 999. <br/>\n";
			$error_fields[] = $param;	
		} else		
			$data[$param] = getparam($param);
	}

	$change_command = false;	

	if (getparam("generate_random_imsi") != "on" || getparam("add_existing_subscriber"))
	       	$change_command = true;

	if ($change_command) {
		//validation on fields

		$data["smsc"] = getparam("smsc")!=NULL ? getparam("smsc") : getparam("smsc_adv");
		if ($data["smsc"] == "" && !ctype_digit($data["smsc"])) {
		         $error .= "SMSC must be digits only!<br/>\n";
			 $error_fields[] = $param;
		}

		if (getparam("add_existing_subscriber")){ 
			$params = array("iccid", "opc");
			$data["imsi"] = getparam("imsi");
			$data["ki"] = getparam("ki");
		} else	
			$params = array("imsi", "iccid", "ki", "opc");
		foreach ($params as $key => $param) {
			if (!getparam($param)) {
				$error .= "This parameter '".strtoupper($param). "' cannot be empty!<br/>\n";
				$error_fields[] = $param;
			} elseif ($param == "imsi") {
				$imsi = getparam($param);
			        if (!ctype_digit($imsi) || ( strlen($imsi) != 15 && strlen($imsi) != 14))
					$error .= "IMSI: $imsi is not valid, must contain 14 or 15 digits. <br/>\n";
				if (test_existing_imsi_in_csv($imsi))
					$error .= "This IMSI: $imsi is already written on another SIM card.<br/>\n";
				$error_fields[] = $param;
			} elseif (($param == "opc" || $param == "ki") && !preg_match('/^[0-9a-fA-F]{32}$/i', getparam($param))) {
				$name = $param == "opc" ? "OPC" : "Ki";
				 $error .= $name . ": ".getparam($param)." needs to be 128 bits, in hex format.<br/>\n";
				 $error_fields[] = $param;
			} elseif ($param == "iccid" && !ctype_digit(getparam($param)) && strlen(getparam($param)) != 19) {
				$error .= "ICCID: ". getparam($param) ." must contain 19 digits!<br/>\n";
				$error_fields[] = $param;
			}	
			$data[$param] = getparam($param);
		}
	}
	if (!strlen($error))
		$output = execute_pysim($data, $change_command);
	else {
		if (getparam("add_existing_subscriber"))
			return write_sim_form($error, $error_fields, getparam("generate_random_imsi"),getparam("insert_subscribers"),true, $data);
		else
			return write_sim_form($error, $error_fields, getparam("generate_random_imsi"),getparam("insert_subscribers"));
	}

	if ($output)
		print "<pre>".$output."</pre>";

	//for successfully written SIM cards, write tha last one into subscribers file 
	if (substr(trim($output),-6,6) == "Done !" && getparam("insert_subscribers") == "on") {
		$all_sim_written = read_csv();
		write_generated_imsi_to_file($all_sim_written[count($all_sim_written)-1]);
	}

	if (getparam("add_existing_subscriber"))
		list_subscribers();
	else
		manage_sims();
}

function write_imsi_in_subscribers()
{
	$imsi = getparam("imsi");
	$ki = getparam("ki");

	if (!$imsi && !$ki)
		return;

	$subscribers = array("imsi"=> $imsi , "ki"=> $ki);
	write_generated_imsi_to_file($subscribers);

	manage_sims();
}

function write_generated_imsi_to_file($subscribers)
{
	$res = get_subscriber($subscribers["imsi"]);
	if ($res[0])
		return;

	unset($subscribers["iccid"], $subscribers["operator_name"],$subscribers["country_code"],$subscribers["mobile_country_code"],$subscribers["mobile_network_code"]);
	$res = get_subscribers(true);
	if ($res[0])
		$new = $res[1];	

	$cc = array();
	if (isset($new["general"])) {
		$cc = $new["general"];
		unset($new["general"]);
	}

	$new[$subscribers["imsi"]] = array(/*"imsi"=>$subscribers["imsi"],*/"msisdn"=> "","short_number"=>"","active"=>"off","ki"=>$subscribers["ki"],"op"=>"","imsi_type"=>"2G");
	$res = set_subscribers($new, $cc);

	if (!$res[0])
		return errormess($res[1],"no");
}

function execute_pysim($params, $command_manually=false)
{
	global $pysim_path, $pysim_csv;

	if (!isset($_SESSION["card_num"]))
	       	$_SESSION["card_num"] = 0;
	$random_string = generateToken(5);
	/**
	 * usage: Run pySIM with some minimal set params:
	 * ./pySim-prog.py -n 26C3 -c 49 -x 262 -y 42 -z <random_string_of_choice> -j <card_num>
	 *
	 *   With <random_string_of_choice> and <card_num>, the soft will generate
	 *   'predictable' IMSI and ICCID, so make sure you choose them so as not to
	 *   conflict with anyone. (for eg. your name as <random_string_of_choice> and
	 *   0 1 2 ... for <card num>).
	 */

	$pysim_installed = detect_pysim_installed();

        if (!$pysim_installed[0]) {
                errornote($pysim_installed[1]);
                return;
        }

	$params_sim_data = "-e -n ".$params["operator_name"]." -c ".$params["country_code"]." -x ".$params["mobile_country_code"]." -y ".$params["mobile_network_code"]." -z $random_string  -j ". $_SESSION["card_num"];

	$command = 'stdbuf -o0 ' . $pysim_path.'/'.'pySim-prog.py -p 0 -t '. $params["card_type"]." ".$params_sim_data. " --write-csv ".$pysim_csv ;

	/**
	 * Or if command manually is set then run pySIM with every parameter specified manually:
	 * E.g.:  ./pySim-prog.py -n 26C3 -c 49 -x 262 -y 42 -i <IMSI> -s <ICCID>
	 */ 
	if ($command_manually)
		$command = 'stdbuf -o0 ' . $pysim_path.'/'.'pySim-prog.py -e -p 0 -t '. $params["card_type"]. " -n ".$params["operator_name"]."  --write-csv ".$pysim_csv." -i ". $params["imsi"]." -s ".$params["iccid"]. " -o ". $params["opc"]." -k ". $params["ki"]." -c ".$params["country_code"]." -x ".$params["mobile_country_code"]." -y ".$params["mobile_network_code"];
	
	if (isset($params["smsc"]))
		$command .= " -m ".$params["smsc"];
	
	$descriptorspec = array(
		0 => array("pipe","r"),// stdin is a pipe that the child will read from
		1 => array("pipe","w"),//stdout
		2 => array("pipe","w") //stderr
	) ;

	// define current working directory where files would be stored
	// open process and pass it an argument
	$process = proc_open($command, $descriptorspec, $pipes);
	
	if (is_resource($process)) {
		// $pipes now looks like this:
		// 0 => writeable handle connected to child stdin
		// 1 => readable handle connected to child stdout
		// Any error output will be appended to $yate_conf_dir."pysim-error.log"
		$output = false;
		do {
			$read = array($pipes[1]);$write=array();$except=array();
			if (!stream_select($read, $write ,$except,3)) {
			//if card was not inserted in the Reader or the time expired
		                fclose($pipes[1]);
		                proc_terminate($process);
				proc_close($process);
		                print "Card was not found in SIM card reader... Terminating program...";
		                return;
			}
			$return_message = fread($pipes[1], 1024);
			$output .= $return_message; 
		} while(!empty($return_message));
	}

	if ($err = stream_get_contents($pipes[2])) {
		$split_errs = explode("\n", rtrim($err));
		$output .= "<div style=\"display:none;\" id=\"pysim_err\">";
		$i = 1;
		foreach ($split_errs as $key => $split_err) {
			if ($i == count($split_errs))
				break;
			$output .= $split_err."\n";
			$i++;
		}

		$output .= "\n</div>";
		$output .= $split_errs[count($split_errs)-1];
		if (preg_match("/Exception AttributeError: \"'PcscSimLink' object has no attribute '_con/", $output))
			$output .= "\nPlease connect you SIM card reader to your device.\n"; 
		$output .= "\n<br/><div id=\"err_pysim\">For full pySim traceback <div id=\"err_link_pysim\" class=\"error_link\" onclick=\"show_hide_pysim_traceback('pysim_err')\" style=\"cursor:pointer;\">click here</div></div>";
	} 

	proc_close($process);
	
	$_SESSION["card_num"] += 1;
	return $output;
}

// $length - the length of the random string
// returns random string with mixed numbers, upperletters and lowerletters
function generateToken($length)
{
	//0-9 numbers,10-35 upperletters, 36-61 lowerletters
	$str = "";
	for ($i=0; $i<$length; $i++) {
		$c = mt_rand(0,61);
		if ($c >= 36) {
			$str .= chr($c+61);
			continue;
		}
		if ($c >= 10) {
			$str .= chr($c+55);
			continue;
		}
		$str .= chr($c+48);
	}
	return $str;
}

function read_csv()
{
	global $yate_conf_dir;

	$sim_data = array();
	$filename = $yate_conf_dir.'sim_data.csv';
	if (!file_exists($filename))
		return $sim_data;

	$formats = array("operator_name", "iccid", "mobile_country_code", "mobile_network_code", "imsi", "smsp", "ki", "opc");
	$csv = new CsvFile($filename,$formats, array(), false);
	if ($csv->getError()) {
		nib_note($csv->getError());
		return $sim_data;
	}

	$sim_data = $csv->file_content;
	return $sim_data;
}

function test_existing_imsi_in_csv($imsi)
{
	$sim_data = read_csv();

	$sim_imsis = array();

	for ($i=0; $i<count($sim_data); $i++){ 
		if (isset($sim_data[$i]["imsi"]))
			$sim_imsis[] = $sim_data[$i]["imsi"];
	}

	if (in_array($imsi, $sim_imsis))
		return true;

	return false;
}

function get_params_subscriber_from_pysim_csv($imsi)
{
	if (!test_existing_imsi_in_csv($imsi))
		return array();

	$params = array();
	$sim_data = read_csv();

	for ($i=0; $i<count($sim_data); $i++)
		if ($sim_data[$i]["imsi"]== $imsi)
			return $sim_data[$i];
}

function write_subcriber_on_sim($imsi, $ki)
{
	return '<a href="main.php?module=subscribers&method=write_subscriber_form&imsi='.$imsi.'&ki='.$ki.'"><img src="images/sim_programmer.png" /></a>';
}

function write_subscriber_form()
{
	$iccid_required = get_iccid_required_params();
	$params = array("imsi" => getparam("imsi"), "ki"=>getparam("ki"));
	$sim_data = get_params_subscriber_from_pysim_csv($params["imsi"]);
	if (count($sim_data)) 
		$params = $sim_data;
	$params["smsc"] = get_smsc();

	if (!isset($params["iccid"]))
		$params["iccid"] = get_iccid_random($iccid_required["cc"], $iccid_required["mcc"], $iccid_required["mnc"]);
	if (!isset($params["opc"]))
		$params["opc"] = get_opc_random();

	write_sim_form($error=null,$error_fields=array(), "on", "off", true, $params);
}

function get_iccid_required_params()
{
	global $yate_conf_dir;

	$res = get_cc_smsc();
	if ($res[0] && is_array($res[1])) 
		$cc = $res[1]["country_code"];
		
	$filename = $yate_conf_dir."ybts.conf";
	$file = new ConfFile($filename);
	$mcc = "001";
	$mnc = "01";

	if (isset($file->structure["gsm"]["Identity.MCC"]))
		$mcc = $file->structure["gsm"]["Identity.MCC"];
	if (isset($file->structure["gsm"]["Identity.MNC"]))
		$mnc = $file->structure["gsm"]["Identity.MNC"];

	$params = array("cc"=>$cc, "mcc"=>$mcc, "mnc"=>$mnc);

	return $params;
}

function get_smsc()
{
	$smsc = "";
	
	$res = get_cc_smsc();
	if (is_array($res[1])) 
		$smsc = $res[1]["smsc"];

	return $smsc;
}

function import_subscribers($error=null,$error_fields=array())
{
	$fields = array(
		"insert_file_location" => array("display"=>"file", "file_example"=>"import_example.csv"),
		"note!" => array("value"=>"File type must be .csv.", "display"=>"fixed")
	);

	error_handle($error,$fields,$error_fields);
	start_form(NULL,"post",true);
	addHidden("from_csv");
	editObject(NULL,$fields,"Import subscribers from .csv file", "Upload");
	end_form();
}

function import_subscribers_from_csv()
{
	global $module, $yate_conf_dir;

	$filename = basename($_FILES["insert_file_location"]["name"]);
	$ext = strtolower(substr($filename,-4));
	if ($ext != ".csv")
		return import_subscribers("File format must be .csv", array("insert_file_location"));

	$real_name = time().".csv";
	$file = "$yate_conf_dir/$real_name";
	if (!move_uploaded_file($_FILES["insert_file_location"]['tmp_name'],$file))
		return import_subscribers("Could not upload file.", array("insert_file_location"));

	$new_subscribers = get_subscribers_from_uploaded_csv($file);

	if (!$new_subscribers[0])
		return import_subscribers($new_subscribers[1], array("insert_file_location"));

	$new_subscribers = $new_subscribers[1];

	//insert subscribers into subscribers.conf

	$subscribers = get_subscribers(true);

	if (!$subscribers[0]) {
		$keep_general = array();
		$regexp = get_regexp(true);
	        if ($regexp[0] && strlen($regexp[1])) {
			$regexp = $regexp[1];
			if (isset($regexp[1]["country_code"]))
				$keep_general["country_code"] = $regexp[1]["country_code"];
			if (isset($regexp[1]["smsc"]))
				$keep_general["smsc"] = $regexp[1]["country_code"];
		}
		$res = set_subscribers($new_subscribers, $keep_general);
		if (!$res[0]) 
			return import_subscribers("Import subscribers failed. Error: ".$res[1]); 
		
		$res = restart_yate();
		if ($res[0] && isset($res[1])) //yate is not running
			notice("Finished importing subscribers. " .$res[1], "list_subscribers");
		elseif (!$res[0]) //errors on socket connection
			notice("Finished importing subscribers. For changes to take effect please restart yate or reload just nib.js from telnet with command: \"javascript reload nib\".Please note that after this you will lose existing registrations.", "list_subscribers");
		else //yate was restarted
			notice("Finished importing subscribers.", "list_subscribers");

	} else {
		$imsi_duplicate = array();
		$subscribers = $subscribers[1];
		if (isset($subscribers["general"])) {
			$keep_general = $subscribers["general"];
			unset($subscribers["general"]);
		}
		foreach ($new_subscribers as $imsi => $data) {
			if ($data["imsi_type"] == "2G")
				$new_subscribers[$imsi]["op"] = "";
			if ($data["imsi_type"] == "3G" &&  $data["op"] == "")
				$new_subscribers[$imsi]["op"] = "00000000000000000000000000000000";
			if (isset($subscribers[$imsi])) {
				//test if the data from imported file is different from the one set in configuration file and keep the duplicated IMSIs
				if ($data["msisdn"] != $subscribers[$imsi]["msisdn"] || $data["short_number"] != $subscribers[$imsi]["short_number"] || $data["active"] != $subscribers[$imsi]["active"] || $data["ki"] != $subscribers[$imsi]["ki"] || $data["imsi_type"] != $subscribers[$imsi]["imsi_type"]) {
					$imsi_duplicate[] = $imsi;
				}
			}
		}
		$merge_subs = array_merge($subscribers, $new_subscribers);
		$_SESSION["new_subs"] =  $merge_subs;
		$_SESSION["imsi_duplicate"] = $imsi_duplicate;
		$_SESSION["keep_general"] = $keep_general;
		if (count($imsi_duplicate))
			overwrite_imsi_form();
		else {
			$res = set_subscribers($merge_subs, $keep_general);
			if (!$res[0]) 
				return import_subscribers("Import subscribers failed. Error: ".$res[1]); 
			$res = restart_yate();
			if ($res[0] && isset($res[1])) //yate is not running
				notice("Finished importing subscribers. " .$res[1], "list_subscribers");
			elseif (!$res[0]) //errors on socket connection
				notice("Finished importing subscribers. For changes to take effect please restart yate or reload just nib.js from telnet with command: \"javascript reload nib\".Please note that after this you will lose existing registrations.", "list_subscribers");
			else //yate was restarted
				notice("Finished importing subscribers.", "list_subscribers");
		}
	}
}

function get_subscribers_from_uploaded_csv($file)
{
	$new_subscribers = array();
	$formats = array("imsi","msisdn","short_number","active","ki","op","imsi_type");
	$csv = new CsvFile($file,$formats);

	if (!count($csv->file_content) && $csv->getError())
		return array(false, $csv->getError());

	foreach ($csv->file_content as $key => $subs_data) {
		if (isset($subs_data["imsi"])){
			$new_subscribers[$subs_data["imsi"]] = $csv->file_content[$key];
			unset($new_subscribers[$subs_data["imsi"]]["imsi"]);
		}	
	}

	return array(true,$new_subscribers);
}

function overwrite_imsi_form($error=null, $error_fields=array())
{
	global $method;

	$method="overwrite_imsi";
	$imsi_duplicated = array();
	if (isset($_SESSION["imsi_duplicate"]))
		$imsi_duplicate = $_SESSION["imsi_duplicate"];

	$i=0;
	foreach ($imsi_duplicate as $key => $imsi) {
		$fields["imsi".$i] = array("display"=>"fixed", "value"=>$imsi, "column_name"=>"IMSI");
		$i++;
	}
	$fields["note!"] = array("value"=>"The IMSI found in csv file are the same as the ones in subscribers.conf but with different values.", "display"=>"fixed");

	error_handle($error,$fields,$error_fields);
	start_form();
	addHidden("in_file");
	editObject(NULL,$fields, "Overwrite existing subscribers", "Overwrite IMSIs");
	end_form();
}

function overwrite_imsi_in_file()
{
	if (isset($_SESSION["new_subs"]))
		$merge_subs = $_SESSION["new_subs"];

	if (isset($_SESSION["keep_general"]))
		$keep_general = $_SESSION["keep_general"];

	if (!count($merge_subs))
		return overwrite_imsi_form("No subscribers found to overwrite.");

	$res = set_subscribers($merge_subs, $keep_general);
	if (!$res[0]) 
	 	return overwrite_imsi_form("The subscribers were not overwritten. Error: ". $res[1]);

	unset($_SESSION["new_subs"],$_SESSION["keep_general"],$_SESSION["imsi_duplicate"]);
	$res = restart_yate();
	if ($res[0] && isset($res[1])) //yate is not running
		notice("Finished overwritting subscribers. " .$res[1], "list_subscribers");
	elseif (!$res[0]) //errors on socket connection
		notice("Finished overwritting subscribers. For changes to take effect please restart yate or reload just nib.js from telnet with command: \"javascript reload nib\".Please note that after this you will lose existing registrations.", "list_subscribers");
	else //yate was restarted
		notice("Finished overwritting subscribers.", "list_subscribers");
}

function export_subscribers_in_csv()
{
	global $yate_conf_dir;

	$subscribers = get_subscribers();
	if (!$subscribers[0]) {
		nib_note("No subscribers to export.");
		return;
	}
	$smsc = get_smsc();
	$formats = array("IMSI", "Msisdn", "Short_number", "Active", "Ki", "OP", "IMSI_Type", "ICCID", "SMSC", "OPC");
	$i=0;
	$arr = array();
	foreach ($subscribers[1] as $imsi => $params) {
		//array_push($params,$imsi);	
		$params_pysim = get_params_subscriber_from_pysim_csv($imsi);
		if (count($params_pysim)) {
			$params["ICCID"] = $params_pysim["iccid"];
			$params["OPC"] = trim($params_pysim["opc"]);
		}
		$params["IMSI"] = $imsi;
		$params["SMSC"] = $smsc; 
		$arr[] = $params;
	}

	foreach ($arr as $key => $val)
		$new[] = change_key_name($val, array("msisdn"=>"Msisdn","short_number"=>"Short_number", "active"=>"Active", "ki"=>"Ki", "op"=>"OP", "imsi_type"=>"IMSI_Type"));

	$filename = "list_subscribers.csv";

	$csv = new CsvFile($yate_conf_dir.$filename, $formats, $new, false, false);

	if (!$csv->status())
		return notice($csv->getError(),"list_subscribers");

	$csv->write();

	notice("Content was exported. <a href=\"download.php?file=$filename\">Download</a>", "list_subscribers");
}

?>
