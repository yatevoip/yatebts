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
				errormess($res[1]. ". Press <a href=\"main.php?module=subscribers&method=list_subscribers&overwrite_file=yes\">here</a> to overwrite subscribers.js.", "no");
				return;
			} else { 
				$new_file = $yate_conf_dir."subscribers.js.backup".date("Ymdhm");
				rename($yate_conf_dir."subscribers.js", $new_file);
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
		$all_subscribers = array();
		foreach($subscribers as $imsi=>$subscr)
			$all_subscribers[] = $subscr;

		$formats = array("IMSI"=>"imsi","msisdn","short_number","ki","op","IMSI Type"=>"imsi_type","function_display_bit_field:active"=>"active");
		table($all_subscribers, $formats, "subscriber", "imsi", array("&method=edit_subscriber"=>"Edit","&method=delete_subscriber"=>"Delete"), array("&method=add_subscriber"=>"Add subscriber", "&method=edit_regexp"=>"Accept by REGEXP"));
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
		$online_subscribers = generate_table_format_from_socket($res);

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
		$rejected_subscribers = generate_table_format_from_socket($res);

	$formats = array("IMSI","No attempts register");
	table($rejected_subscribers, $formats, "rejected IMSIs", "imsi");
}

function edit_regexp($error=null,$error_fields=array())
{
	if (getparam("Set_subscribers")=="Set subscribers")
		return edit_subscriber();

	$regexp = getparam("regexp");

	nib_note("If a regular expression is used, 2G/3G authentication cannot be used. For 2G/3G authentication, please set subscribers individually.");
	$fields = array(
		"regexp" => array("value"=> $regexp, "required"=>true, "comment"=>"Ex: /^310030/")
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
	$res = get_regexp();
	if (!$res[0]) 
		errormess($res[1], "no");
	else
		$expressions = array($res[1]);
	
	$regexp = getparam("regexp");

	if (!$regexp)
		return edit_regexp("Please set the regular expression!",array("regexp"));

	if (in_array($regexp, $expressions)) {
		notice("Finished setting regular expression.", "subscribers");
		return;
	}	

	//$write_regexp[count($expressions)] = $regexp;
	$res = set_regexp($regexp);
	if (!$res[0])
		return edit_regexp($res[1]);
	notice("Finished setting regular expression. For changes to take effect please restart yate or reload just nib.js from telnet with command: \"javascript reload nib\". Please note that after this you will lose existing registrations.", "subscribers");
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
	$active = (get_param($subscriber,"active") == 1) ? 't' : 'f';

	$fields = array(
		"imsi"   => array("value"=>$imsi, "required"=>true, "comment"=>"SIM card id", "column_name"=>"IMSI"),
		"msisdn" => array("value"=>get_param($subscriber,"msisdn"), "comment"=>"DID associated to this IMSI. When outside call is made, this number will be used as caller number.", "column_name"=>"MSISDN"),
		"short_number" => array("value" => get_param($subscriber,"short_number"),"comment"=>"Short number that can be used to call this subscriber."),
		"active" => array("value"=>$active, "display"=>"checkbox"),
		"imsi_type" => array($imsi_type, "display"=>"select", "column_name"=>"IMSI Type", "required"=>true, "comment"=> "Type of SIM associated to the IMSI"),
		"ki" => array("value"=>get_param($subscriber,"ki"), "comment"=>"Card secret", "required"=>true),
		"op" => array("value"=>get_param($subscriber,"op"), "comment"=>"Operator secret. Empty for 2G IMSIs.<br/>00000000000000000000000000000000 for 3G IMSIs.")
	);
	
	if ($imsi && count($subscriber) && !in_array("imsi",$error_fields))
		$fields["imsi"]["display"] = "fixed";
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
	$res = get_subscribers();
	if (!$res[0]) {
		//errormess($res[1], "no");
		$subscribers = array();
	} else
		$subscribers = $res[1];

	$imsi = (getparam("imsi")) ? getparam("imsi") : getparam("imsi_val");
	if (!$imsi)
		return edit_subscriber("Please set 'imsi'",array("imsi"));
	if (strlen($imsi)!=14 && strlen($imsi)!=15)
		return edit_subscriber("Invalid IMSI $imsi. IMSI length must be 14 or 15 digits long.",array("imsi"));
	$subscriber = array("imsi"=>$imsi);

	$fields = array("msisdn"=>false, "short_number"=>false, "active"=>false, "ki"=>true, "op"=>false, "imsi_type"=>true);
	foreach ($fields as $name=>$required) {
		$val = getparam($name);
		if ($required && !$val)
			return edit_subscriber("Field $name is required");
		$subscriber[$name] = $val;
	}
	$subscriber["active"] = ($subscriber["active"]=="on") ? 1 : 0;
	if ($subscriber["imsi_type"]=="2G")
		$subscriber["op"] = "";

	if (getparam("imsi_val") && isset($subscribers[$imsi])) {
		$modified = false;
		//check if there are changes
		$subs_file = $subscribers[$imsi];
		foreach ($fields as $name=>$required) {
			$val = getparam($name);
			if ($name == "active")
				$val = (getparam($name) == "on") ? 1 : 0;

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

	$res = set_subscribers($subscribers);
	if (!$res[0])
		return edit_subscriber($res[1]);
	notice("Finished setting subscriber with IMSI $imsi. For changes to take effect please restart yate or reload just nib.js from telnet with command: \"javascript reload nib\".  Please note that after this you will lose existing registrations.", "subscribers");
}

function delete_subscriber()
{
	ack_delete("subscriber", getparam("imsi"), NULL, "imsi", getparam("imsi"));
}

function delete_subscriber_database()
{
	$imsi = getparam("imsi");
	$res = get_subscribers();
	if (!$res[0])
		return errormess($res[1], "no");
	$subscribers = $res[1];

	if (!isset($subscribers[$imsi]))
		return errormess("IMSI $imsi is not in subcribers list.","no");

	unset($subscribers[$imsi]);
	$res = set_subscribers($subscribers);
	if (!$res[0]) 
		return errormess($res[1],"no");

	notice("Finished removing subscriber with IMSI $imsi.", "subscribers");
}

function set_subscribers($subscribers)
{
	global $yate_conf_dir, $subscribers_prefix, $subscribers_suffix;

	$js_file = new JsObjFile($yate_conf_dir."subscribers.js", $subscribers_prefix, $subscribers_suffix, $subscribers);
	$js_file->safeSave();

	if (!$js_file->status())
		return array(false, $js_file->getError());
	return array(true);
}

function set_regexp($regexp)
{
	global $yate_conf_dir, $regexp_prefix, $regexp_suffix;
	
	$filename = $yate_conf_dir."subscribers.js";
	$file = new GenericFile($filename);
	$file->openForWrite();
	if (!$file->status())
		return array(false, $file->getError());
	$fh = $file->getHandler("w");

	fwrite($fh, $regexp_prefix.$regexp.$regexp_suffix);
	$file->close();

	return array(true);
}

function get_subscribers()
{
	global $yate_conf_dir, $subscribers_prefix, $subscribers_suffix;

	$filename = $yate_conf_dir."subscribers.js";
	if (!is_file($filename))
		// if subscribers.js file doesn't exist don't return error, just empty subscribers array
		return array(true, array());

	$js_file = new JsObjFile($yate_conf_dir."subscribers.js", $subscribers_prefix, $subscribers_suffix);
	$js_file->read();
	if (!$js_file->status())
		return array(false, $js_file->getError());
	$subscribers = $js_file->getObject();
	if (!$subscribers)
		$subscribers = array();
	return array(true, $subscribers);
}

function get_regexp()
{
	global $yate_conf_dir, $regexp_prefix, $regexp_suffix;

	$filename = $yate_conf_dir."subscribers.js";
	if (!is_file($filename))
		// if subscribers.js file doesn't exist don't return error, just empty string
		return array(true,"");

	$file = new GenericFile($filename);
	$file->openForRead();
	if (!$file->status())
		return array(false, $file->getError());

	$fh = $file->getHandler();
	clearstatcache();
	$content = fread($fh, filesize($filename));
	if (substr($content,0,strlen($regexp_prefix))!=$regexp_prefix) {
		// file doesn't match, we probably have subscribers
		return array(true, "");
	}
	$content = substr($content,strlen($regexp_prefix));
	$content = substr($content,0,strlen($content)-strlen($regexp_suffix));

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

?>
