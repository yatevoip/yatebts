<?php
/**
 * debug_all.php
 * This file is part of the FreeSentral Project http://freesentral.com
 *
 * FreeSentral - is a Web Graphical User Interface for easy configuration of the Yate PBX software
 * Copyright (C) 2008-2014 Null Team
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

require_once "set_debug.php";
require_once "lib.php";

if (is_file("../defaults.php"))
	require_once "../defaults.php";
if (is_file("../config.php"))
	require_once "../config.php";

?>
<html>
<head>
	<title> Debug Page </title>
	<link type="text/css" rel="stylesheet" href="../css/main.css" />
</head>
<body>
<?php

$array = array(
	1 => "E_ERROR",
	2 => "E_WARNING",
	4 => "E_PARSE",
	8 => "E_NOTICE",
	16 => "E_CORE_ERROR",
	32 => "E_CORE_WARNING",
	64 => "E_COMPILE_ERROR",
	128 =>	"E_COMPILE_WARNING",
	256 =>  "E_USER_ERROR",
	512 =>	"E_USER_WARNING",
	1024 =>	"E_USER_NOTICE",
	2048 =>	"E_STRICT",
	4096 =>	"E_RECOVERABLE_ERROR",
	8191 => "E_ALL",
	6183 => "E_ALL ^ E_NOTICE",
	10239 => "E_ALL | E_STRICT",
	0 => "disable"
);

$action = getparam("action");
if($action) 
	$call = "debug_".$action;
else
	$call = "debug";
$call();


function debug()
{
	global $array;

	$error = ini_get("error_reporting");
	$errors = $array;
	if(isset($array[$error]))
		$errors["selected"] = $array[$error];
	else{
		$errors[$error] = $error;
		$errors["selected"] = $errors[$error];
	}

	$display_errors = (ini_get("display_errors")) ? 't' : 'f';

	$debug_queries = (isset($_SESSION["debug_all"])) ? "t" : "f";
	$log_errors = (ini_get("log_errors")) ? "t" : "f";
	$error_log = ($a = ini_get("error_log")) ? $a : '';
	$arr = array(
		"pass_debug"=>array("compulsory"=>true, "column_name"=>"Password"),
		"debug_queries"=>array("value"=>$debug_queries, "display"=>"checkbox"),
		"error_reporting"=>array($errors, "display"=>"select"), 
		"display_errors"=>array("value"=>$display_errors, "display"=>"checkbox"),
		"log_errors"=>array("value"=>$log_errors, "display"=>"checkbox"),
		"error_log"=>array("value"=>$error_log/*, "display"=>"checkbox"*/)
	);

	if (is_auth("debug"))
		unset($arr["pass_debug"]);

?>	<br/><br/>
	<form action="debug_all.php" method="post"><?php
	addHidden("database");
	editObject(NULL,$arr, "Setting debug levels", "Save");
?></form><?php
}


function debug_database()
{
	global $array, $pass_debug_page;

	if (!check_auth("debug")) {
		print "Error! Please insert debug password to see this page.";
		debug();
		return;
	}

	if(getparam("debug_queries") == "on")
		$_SESSION["debug_all"] = "on";
	else
		unset($_SESSION["debug_all"]);

	$err = getparam("error_reporting");
	$error_reporting = NULL;
	foreach($array as $key=>$value) {
	//	print $key. ' '. $value.'<br/>';
		if($value == $err) {
			$error_reporting = $key;
			break;
		}
	}
	if(!$error_reporting)
		$error_reporting = $err;
	ini_set("error_reporting",$error_reporting);
	$_SESSION["error_reporting"] = $error_reporting;
//	ini_set('error_reporting', E_ALL);

	$display_errors = (getparam("display_errors") == "on") ? true : false;

	ini_set("display_errors",$display_errors);
	$_SESSION["display_errors"] = $display_errors;

	$log_errors = (getparam("log_errors") == "on") ? true : false;
	ini_set("log_errors",$log_errors);
	$_SESSION["log_errors"] = $log_errors;

	$error_log = getparam("error_log");
	ini_set("error_log", $error_log);
	$_SESSION["error_log"] = $error_log;

	print 'Settings were saved';
	debug();
}

?>
</body>
</html>
