<?php

/**
 * Copyright (C) 2012-2013 Null Team
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

$content_sent = false;

function send_content()
{
	global $content_sent;
	if ($content_sent)
		return;
	$content_sent = true;
	header("Content-type: application/json");
}

function send_error($errcode,$errtext)
{
	global $content_sent;
	if (!$content_sent)
		header("HTTP/1.1 $errcode $errtext");
}

function check_request($method,$secret = "")
{
	global $allow_http, $exception_to_https;

	if (!isset($allow_http))
		$allow_http = false;
	
	$errcode = 0;
	$errtext = "";
	if ((!isset($_SERVER["HTTPS"]) || $_SERVER["HTTPS"]=="off") && !$allow_http) {
		$errcode = 403;
		$errtext = "Forbidden";
	}
	else if (($secret!="") && (!isset($_SERVER["HTTP_X_AUTHENTICATION"]) || $_SERVER["HTTP_X_AUTHENTICATION"]!=$secret)) {
		$errcode = 494;
		$errtext = "Security Agreement Required";
	}
	else if ($_SERVER["REQUEST_METHOD"] != $method) {
		$errcode = 405;
		$errtext = "Method Not Allowed";
	}
	else if ($_SERVER["CONTENT_TYPE"] != "application/json") {
		$errcode = 415;
		$errtext = "Unsupported Media Type";
	}

	if ($errcode != 0) {
		header("HTTP/1.1 $errcode $errtext");
?>
<html>
    <head>
    <title><?php print($errtext); ?></title>
    </head>
    <body>
    <h1><?php print($errtext); ?></h1>
    Expecting <b><?php print($method); ?></b> with type <b>application/json</b>
    </body>
</html>
<?php
		exit;
	}
}

function valid_param($param)
{
	return ($param !== null) && ($param != "");
}

function arr_to_db($arr, $class_name, $edit_function="edit", $obj_id=NULL)
{
	$obj = new $class_name;

	$vars = Model::getVariables($class_name);
	if (!$vars || !$obj) {
		//send_error(501, "Not implemented");
		return array(false, "code"=>"2"); // "Not yet implemented"
	}

	$params = array();
	foreach($vars as $name=>$var) {
		if (!isset($arr[$name]))
			continue;
		$value = get_param($arr, $name);
		if ($var->isRequired() && !strlen($value)) {
			//send_error(406, "Not Acceptable");
			return array(false, "code"=>"10");
		}
		$params[$name] = $value;
	}
	if ($obj_id != "__no_id") {
		$id = ($obj_id) ? $obj_id : $obj->getIdName();
		// make sure we have $params[$id] in case class was not correctly defined
		if (!isset($params[$id]) || !strlen($params[$id])) {
			//send_error(406, "Not Acceptable");
			return array(false, "code"=>"10");
		}
	
		$db_obj = Model::selection($class_name, array($id=>$params[$id]));
		if (count($db_obj)) {
			// this object already exists and we should modify it
			$obj = $db_obj[0];
			if ($edit_function == "edit") {
				if ($obj_id)
					$res = $obj->edit($params,array($obj_id=>$params[$obj_id]));
				else
					$res = $obj->edit($params);
			} elseif ($edit_function == "fieldupdate") {
				$params_to_edit = array();
				foreach($arr as $name=>$value) {
					if ($name == $obj_id)
						continue;
					$obj->{$name} = $value;
					$params_to_edit[] = $name;
				}
				$res = $obj->fieldUpdate(array($obj_id=>$params[$obj_id]),$params_to_edit);
			} else {
				//send_error(500, "Internal Server Error");
				return array(false, "code"=>"98"); // Internal Server Error:: called wrong function
			}
		} else {
			$res = $obj->add($params);
		}
	} else {
		$res = $obj->add($params);
	}
	if (!$res[0]) {
		//send_error(406, "Not Acceptable");
		return array(false, "code"=>$res[1]);
	}
	return array(true, "code"=>"0", "obj"=>$obj);
}

function check_response($out)
{
	$arr = array();
	foreach ($out as $name=>$val) {
		if (is_array($val))
			$arr[$name] = check_response($val);
		else {
			if ($name == "error_code" && $val=="")
				$val = "0";
			$arr[$name] = $val;
		}
	}
	return $arr;
}

function translate_error_to_code($res, $exception_true=false)
{
//	if (is_array($res) && count($res)==1 && $res[0]===true)
//		return $res;

	if (!is_array($res) || (count($res)<2 && (($res[0]===false) || !isset($res[0]))) || !isset($res[0]))
		return array(false, "code"=>"99"); // Internal Server Error: wrong result format from called function

	if ($res[0] && $exception_true) {
		if (!isset($res["code"]))
			return array(true);
		return array(true, "code"=>$res["code"]);  // this code will define a user/contact not success of request
	}
	elseif ($res[0]/* && !$exception_true*/) {
		return $res; // normal success case
	} elseif (!$res[0] && $exception_true) {
		if (!isset($res["code"])) // succes and default contact already existed
			return array(true);
		return array(true, "code"=>$res["code"]);  // this code will define a user/contact not success of request
	}
	$new_res = array(false);
	if (isset($res[1])) {
		if (substr($res[1], -12)==" is required" || substr($res[1], -9)==" not set.")
			$new_res["code"] = "10";    // Missing or empty parameters
		else
			$new_res["code"] = "97";	
	} elseif (isset($res["code"]))
		$new_res["code"] = $res["code"];
	else
		$new_res["code"] = "99";

	// even if error, if more fields set in result (with non numeric key) copy them to response
	if (count($res)>2 || (count($res)>1 && !isset($res[1]))) {
		foreach ($res as $key=>$val) {
			if (!is_numeric($key))
				$new_res[$key] = $val;
		}
	}

	return $new_res;
}

function clean_session()
{
	global $direct_request;
	
	// taken from comments on php site for session_unset
/*	session_unset();
	session_destroy();
	session_write_close();
	setcookie(session_name(),'',0,'/');
session_regenerate_id(true);*/


	// Unset all of the session variables.
	$_SESSION = array();

	if (isset($direct_request))
		return;

	// If it's desired to kill the session, also delete the session cookie.
	// Note: This will destroy the session, and not just the session data!
	if (ini_get("session.use_cookies")) {
		$params = session_get_cookie_params();
		setcookie(session_name(), '', time() - 42000,
			$params["path"], $params["domain"],
			$params["secure"], $params["httponly"]
		);
	}

	if (isset($_SESSION))
		// Finally, destroy the session.
		session_destroy();
}


function set_session()
{
	global $sesslife_json;

	if (isset($sesslife_json))
		session_set_cookie_params($sesslife_json);
	if (!session_id()) {
		$res = session_start();
		if (!$res)
			return array(false,"code"=>"105");
	}
	return array(true);
}

function retrieve_json()
{
	global $json;

	$json = file_get_contents('php://input');
        $inp = json_decode($json,true);
        if ($inp === null)
		return array(false,"code"=>"6");
	return array(true,"json"=>$inp);
}

function write_response($out)
{
	global $error_codes, $add_newline, $enable_logs, $logs_file;
	global $start_time, $med_time;
	global $func_name, $class_name, $request_id;
	global $json, $used_backup, $current_input;

	if (isset($error_codes[$out["code"]]) && !isset($out["message"]))
		$out["message"] = $error_codes[$out["code"]];
	$out = check_response($out);
	send_content();
	$o_json = json_encode($out);
	print $o_json;
	if (isset($add_newline) && in_array($func_name,$add_newline))
		print "\n";

	$end_time = microtime(true);
	if ($enable_logs) {
		$day = date("Y-m-d");
		$file = str_replace(".txt","$day.txt",$logs_file);
		$fh = fopen($file, "a");
		if (!$fh) {
			print ("Could not open logs file $file");
			exit;
		}
		$aprox_duration = round($end_time - $start_time,4);
		$id = (isset($_SESSION["user_id"])) ? ", user_id=".$_SESSION["user_id"] : "";
		$captcha = isset($_SESSION["req_captcha"])? ", captcha_sess=".$_SESSION["req_captcha"] : "";
		fwrite($fh, "------ ".date("Y-m-d H:i:s").", ip=".$_SERVER['REMOTE_ADDR'].", func=$func_name, class=$class_name"."$id, request_id='$request_id', aprox_duration=$aprox_duration, med_time=$med_time"."$captcha \n");
		fwrite($fh, "Json: $json"."\n");
		fwrite($fh, "Input: ".str_replace("\n"," ",print_r($current_input,true))."\n");
		fwrite($fh, "Output: ".str_replace("\n"," ",print_r($out,true))."\n");
		fwrite($fh, "JsonOut: $o_json"."\n");
		if (isset($used_backup))
			fwrite($fh,$used_backup."\n");
		fclose($fh);
	}
}

function process_request($inp, $func="arr_to_db", $class_name=NULL, $exit_after_func_ok=false)
{
	global $json, $enable_r_logs, $request_id, $r_logs_file;

	$request_id = rand();

	if ($enable_r_logs) {
		$day = date("Y-m-d");
		$file = str_replace(".txt","$day.txt",$r_logs_file);

		$fh = fopen($file, "a");
		if (!$fh)
			// "Internal Server Error: Could not open requests logs file."
			return array(false, "code"=>"101"); 
		fwrite($fh, "------ ".date("Y-m-d H:i:s").", ip=".$_SERVER['REMOTE_ADDR'].", func=$func, class=$class_name".", request_id='$request_id'\n");
		fwrite($fh, "Json: $json"."\n");
		fclose($fh);
	}

	$return_error = get_param($inp,"returnerror");
	if (missing_param($return_error)) {
		if ($class_name)
			$res = $func($inp, $class_name);
		else
			$res = $func($inp);
	} else {
		if (substr($return_error,0,7) == "timeout") {
			$time = str_replace("timeout:","",$return_error);
			// max_execution_time = 30 in php.ini so requester should get timeout
			sleep($time);
			//print "This should not be seen if you set timeout parameter when making request.";
			//exit();
			if ($class_name)
				$res = $func($inp, $class_name);
			else
				$res = $func($inp);
		} else
			$res = array(false, "code"=>$return_error);
	}
	if (isset($res["obj"]))
		unset($res["obj"]);
	if (!isset($res[0]))
		return array(false, "code"=>"99");  // Internal server error: wrong result format from called function

	$res = translate_error_to_code($res);
	if ($res[0] && $exit_after_func_ok)
		// for get_captcha type of request: don't return anything by default
		exit;
	if (!$res[0]) {
		unset($res[0]);
		return $res;
	} else
		unset($res[0]);

	if (!isset($res[1])) {
		$ret = array("code"=>"0");
		if (count($res))
			$ret = array_merge($ret,$res);
		return $ret;
     	} else {
		if (!isset($res[1]["code"]))
			$res[1]["code"] = 0;
		return $res[1];
	}
}

// in case backup database connection is used, call this function to record this in the logs
function log_used_backup_conn($connection_index)
{
	global $used_backup, $server_id;

	$used_backup = "Used backup connection '$connection_index' for server_id=$server_id";
}
?>
