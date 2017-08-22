<?php
/**
 * lib_proj.php
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

require_once "ansql/socketconn.php";

function have_pysim_package()
{
	return shell_exec("rpm -qa 'pysim*'");
}

function have_nib_package()
{
	return shell_exec("rpm -qa 'yate-bts-nib*'");
}

function have_pysim_prog()
{
	return shell_exec("which pySim-prog.py");
}

function include_formats($formats,$form_identifier)
{
	$formats = explode(',',$formats);
	?>
	<input type="checkbox" name="<?php print $form_identifier;?>alaw" <?php if (in_array("alaw",$formats)) print "CHECKED";?>>alaw
	<input type="checkbox" name="<?php print $form_identifier;?>mulaw" <?php if (in_array("mulaw",$formats)) print "CHECKED";?>>mulaw
	<input type="checkbox" name="<?php print $form_identifier;?>gsm" <?php if (in_array("gsm",$formats)) print "CHECKED";?>>gsm
	<br/>
	<input type="checkbox" name="<?php print $form_identifier;?>g729"<?php if (in_array("g729",$formats)) print "CHECKED";?>>g729
	<input type="checkbox" name="<?php print $form_identifier;?>g723"<?php if (in_array("g723",$formats)) print "CHECKED";?>>g723
	&nbsp;&nbsp;&nbsp;<input type="checkbox" name="<?php print $form_identifier;?>ilbc"<?php if (in_array("ilbc",$formats)) print "CHECKED";?>>ilbc
	<?php
}

function get_formats($form_identifier = '')
{
	$codecs = array(
		"alaw" => getparam($form_identifier."alaw"), 
		"mulaw"=> getparam($form_identifier."mulaw"), 
		"gsm"  => getparam($form_identifier."gsm"), 
		"ilbc" => getparam($form_identifier."ilbc"), 
		"g729" => getparam($form_identifier."g729"), 
		"g723" => getparam($form_identifier."g723")
	);
	$formats = "";
	foreach ($codecs as $codec=>$value) {
		if ($value=="on") {
			if ($formats!="") 
				$formats .= ',';
			$formats .= $codec;
		}
	}
	return $formats;
}

function search_box($fields, $submit="Search", $title=NULL)
{
	if (!isset($fields[0])) {
		errormess("Wrong format of params in search_box()","no");
		return;
	}
	if (!isset($fields[1])) {
		$fields[1] = array();
		for ($i=0; $i<count($fields[0]); $i++) {
			$fld = strtolower($fields[0][$i]);
			if ($fld=="&nbsp;")
				continue;
			$fields[1][] = "<input type=\"text\" name=\"s_$fld\" value=\"".getparam("s_".$fld)."\" />";
		}
	}
	if ($submit=="Search")
		$fields[count($fields)-1][] = "<input type=\"submit\" value=\"Search\" />";
	start_form();
	addHidden();
	formTable($fields,$title);
	end_form();
}

function get_imsi_msisdn_cond()
{
	$conds = array();
	if (getparam("s_imsi"))
		$conds["imsi"] = getparam("s_imsi");
	if (getparam("s_msisdn"))
		$conds["msisdn"] = getparam("s_msisdn");
	if (getparam("s_status"))
		$conds["status"] = getparam("s_status");

	return $conds;
}

function valid_address($addr)
{
	$expl_addr = explode(".",$addr);
	if (count($expl_addr)!=4)
		return false;
	for ($i=0; $i<4; $i++)
		if ($expl_addr[$i]<0 || $expl_addr[$i]>255)
			return false;
	return true;
}

function positive_int($val, $max_val=NULL)
{
	if (!is_numeric($val) || $val<0 || floor($val)!=$val || substr($val,0,1)=="+")
		return false;
	if (strpos($val,"e") || strpos($val,"E"))
		return false;
	if ($max_val && $val>$max_val)
		return false;
	return true;
}

function valid_ip_port($val)
{
	$val = explode(":",$val);
	if (count($val)==1)
		return valid_address($val[0]);
	elseif (count($val)!=2)
		return false;
	$ip = $val[0];
	$port = $val[1];

	if (!valid_address($ip))
		return false;
	if (!positive_int($port))
		return false;
	return true;
}

function module_loaded($level, $module)
{
	global $do_not_load;

	if (!isset($_SESSION["current_context"])) {
		if (is_file("modules/$level/$module.php") && (!isset($do_not_load) || !in_array($module,$do_not_load)))
			return true;
	} else {
		$context = strtolower($_SESSION["current_context"]);
		if (is_file("modules/$level/$context/$module.php") && (!isset($do_not_load) || !in_array($module,$do_not_load)))
			return true;
	}
	return false;
}

function  lib_loaded($lib)
{
	if (is_file("lib/$lib.php"))
		return true;
	return false;
}

function shell_command($comand)
{
	return shell_exec("/usr/libexec/openbts/ctl-apache $comand 2>&1");
}

function interval_from_seconds($unixtime)
{
	$minutes = floor($unixtime/60);
	$seconds = $unixtime - $minutes*60;
	$seconds = zero_pad($seconds);

	$hours = floor($minutes/60);
	$minutes = $minutes-$hours*60;
	$minutes = zero_pad($minutes);

	$days = floor($hours/24);
	$hours = $hours-$days*24;
	$hours = zero_pad($hours);

	if ($days>0)
		return "$days days $hours:$minutes:$seconds";
	return "$hours:$minutes:$seconds";
}

function zero_pad($val,$length_to_pad=1)
{
	if (strlen($val)==$length_to_pad)
		return "0$val";
	return $val;
}

function get_suggested_imsis($offset=0)
{
	global $lim_recommended;

	if (!isset($lim_recommended))
		$lim_recommended = 5;

	$recommended = request_api(array("limit"=>$lim_recommended,"offset"=>$offset),"get_suggested_imsis","imsi");
	return $recommended;
}

function retrieve_connections()
{
	global $api_servers;

	$servers = Model::selection("server");
	if (!$servers)
		return false;

	$total = count($servers);
	for ($i=0; $i<$total; $i++) {
		$type = strtolower($servers[$i]->type);
		if (!isset($api_servers[$type]))
			$api_servers[$type] = array();
		$server = $servers[$i]->ip;
		$api_servers[$type][$servers[$i]->server_id] = array("server"=>$server, "secret"=>$servers[$i]->header_password, "name"=>$servers[$i]->server);
	}
	if (!count($api_servers))
		print "No server connections defined. Please check MMI internal database.";
	return true;
}

function set_server_conn_from_multiple()
{
	$conn = getparam("vlr_conn");
	$server = Model::selection("server",array("server"=>$conn));
	if (count($server))
		$_SESSION["server_id"] = $server[0]->server_id;
}

function network_settings()
{
	global $method;
	$method = "network_settings";

	$configurations = request_api(array(),"get_configuration","configuration");
	$fields = array("configuration"=>"key","value"=>"value"/*,"function_display_bit_field:default"=>"default"*/);
	table($configurations, $fields, "network setting", "setting_id", array("&method=edit_setting"=>"Edit", "&method=delete_setting"=>"Delete"), array("&method=add_setting"=>"Add"));
}

function edit_setting($error=NULL,$error_fields=array())
{
	$allowed_configurations = array("MCC", "MNC");
	$allowed_configurations["selected"] = getparam("key");

	$fields = array(
		"configuration" => array($allowed_configurations, "display"=>"select", "compulsory"=>true),
		"value" => array("value"=>getparam("value"), "compulsory"=>true),
		/*"default" => array("value"=>getparam("default"),"display"=>"checkbox")*/
	);

	error_handle($error,$fields,$error_fields);
	start_form();
	addHidden("database",array("setting_id"=>getparam("setting_id")));
	editObject(NULL,$fields,"Set network configuration","Save");
	end_form();
}

function edit_setting_database()
{
	$setting_id = getparam("setting_id");
	$params = array("setting_id"=>getparam("setting_id"));
	$required = array("configuration"=>"key","value"=>"value");
	foreach ($required as $form_name=>$field_name) {
		$val = getparam($form_name);
		if (!$val)
			return edit_setting("Field $form_name is required.",array($form_name));
		$params[$field_name] = $val;
	}
	//$params["default"] = (getparam("default")=="on") ? "1" : "0";

	$res = request_api($params,"set_configuration",null,"edit_setting");
	notice("Finished setting ".getparam("configuration"),"network_settings");
}

function delete_setting()
{
	ack_delete("network configuration",getparam("key"),null,"setting_id",getparam("setting_id"),"&key=".getparam("key"));
}

function delete_setting_database()
{
	$res = request_api(array("setting_id"=>getparam("setting_id")),"delete_configuration",null,"network_settings");
	notice("Finished deleteting ".getparam("key"),"network_settings");
}

function set_timezone()
{
	//Turn off all error reporting
	//Returns the old error_reporting level 
	$level = error_reporting(0);

	if (!isset($_SESSION["timezone"])) 
		$_SESSION["timezone"] = date_default_timezone_get();
	
	date_default_timezone_set($_SESSION["timezone"]);

        error_reporting($level);
}

function nib_note($text)
{
	print "<div class='note'>";
	if (is_file("images/note.jpg"))
		print "<img src='images/note.jpg' />";
	else
		print "Note! ";
	print $text;
	print "</div>";
}

function warning_note($text)
{
	print "<div class='note'>";
	print "<font class='error'>Warning!</font>&nbsp;";
	print $text;
	print "</div>";
}

function parse_socket_response($socket_response)
{
	//Ex: $socket_response = string(221) "IMSI MSISDN --------------- --------------- 00101000000000 +4381234 00101001100110 +2500001 00101001100120 +8788783 001900000000002 +6669990 00101000000003 +6789723 00101000000002 +2943456 " 
	$table_array = array();
	$socket_response = trim($socket_response);

	if (!strlen(trim($socket_response)))
		return $table_array;

	$res = explode("--------------- ---------------",$socket_response);

	if (!isset($res[1]) || !strlen($res[1]))
		return $table_array;

	$titles = explode("           ",trim($res[0]));
	$subs = explode("\n",trim($res[1]));
	for ($i=0; $i<count($subs); $i++)
		$data_to_display[] = explode("   ", trim($subs[$i]));

	foreach($data_to_display as $key=>$data)
		$table_array[$key] = array(trim($titles[0]) => trim($data[0]), trim($titles[1]) => trim($data[1]));

	return $table_array;

}

function get_socket_response($command, $marker_end)
{
	global $yate_ip, $default_ip, $default_port;
	//check if Yate is running
	//if yate is not running returns NULL otherwize return process ID 

	if (!shell_exec('pidof yate'))
		return array(false, "Please start YateBTS before performing this action.", false);

	if (!$yate_ip)
		return array(false, "Please set Yate IP in configuration file.");

	$socket = new SocketConn($default_ip, $default_port);
	if (strlen($socket->error))
		return array(false, "There was a problem with the socket connection. Error reported is: ".$socket->error);

	return $socket->command($command, $marker_end);
}

function restart_yate()
{
	$res = get_socket_response("restart now", "\r\n");
	if (!shell_exec('pidof yate'))
		return array(true);

	if (is_array($res)) {
		if (isset($res[2]))
			return array(true);
		return $res;
	}

	if ($res == "Engine restarting - bye!")
		return array(true);

	return array(false, $res);
}

function test_default_config()
{
	global $yate_conf_dir;

	$res = check_permission($yate_conf_dir);
	if (!$res[0])
		return array(false, $res[1]);
	return array(true);
}

function check_permission($dir)
{
	if (!is_dir($dir))
		return array(false, "The directory: ".$dir ." was not found on this server. Please create the directory and run this as root: 'chmod -R a+rw ".$dir."'");
	if (!is_readable($dir) || !is_writable($dir))
		return array(false, "Don't have r/w permission on ". $dir.". Please run this command as root: 'chmod -R a+rw ".$dir."'");

	return array(true);
}

function change_key_name($array, $key_changes)
{
	foreach ($array as $key => $value){ 
		if (is_array($value)) {
			$array[$key] = change_key_name($value,$key_changes);
		} else {
			foreach($key_changes as $old_key => $new_key) {
				$array[$new_key] =  $array[$old_key];  
			}
		}	
	}
	foreach($key_changes as $old_key => $new_key) 
		unset($array[$old_key]);          

	return $array;   
}

function get_opc_random()
{
	$opc = bin2hex(openssl_random_pseudo_bytes(16));	
	return $opc;
}

function get_iccid_random($cc,$mcc,$mnc)
{
	$iccid = "89" . $cc . $mcc . $mnc;
	$number_length = 19 - strlen($iccid);
	$number = random_numbers($number_length);
	$iccid .= $number;

	return $iccid;
}

function random_numbers($digits) 
{
	$min = pow(10, $digits - 1);
	$max = pow(10, $digits) - 1;
	return mt_rand($min, $max);
}

?>
