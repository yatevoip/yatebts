<?php
/**
 * custom_sms.php
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * Copyright (C) 2015 Null Team
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

require_once("lib/lib_proj.php");
set_timezone();
require_once("ansql/set_debug.php");
require_once("ansql/lib.php");
require_once("lib/lib_proj.php");

?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en">
<head>
<title><?php print $proj_title; ?></title>
<link type="text/css" rel="stylesheet" href="css/main.css" />
    <meta name="viewport" content="initial-scale=1.0, user-scalable=no" />
    <meta http-equiv="content-type" content="text/html; charset=UTF-8"/>
<script type="text/javascript" src="ansql/javascript.js"></script>
<script type="text/javascript" src="javascript.js"></script>
</head>
<body class="mainbody">
<?php  
if (getparam("method")=="send_message_to_yate")
	send_message_to_yate();
else
	send_sms();
?>
</body>
</html>

<?php
function send_sms($error = null, $note = null)
{
	if (!shell_exec('pidof yate')) 
                errormess("Please start YateBTS before performing this action.", "no");	
	
	if (strlen($error))
		errormess($error,"no");
	
	if ($note)
		nib_note($note);
	
	$fields = array(
		"called"=> array("comment"=>"The IMSI where the SMS will be send.","column_name"=>"IMSI"),
		"text" => array("column_name"=>"Message", "display"=>"textarea", "comment"=>"The message can be text or RPDU."),
		"sms_type" => array("column_name"=>"SMS Type", 'display'=>'select', array("selected"=>"text", "text", "binary"), "comment"=>"The type of the message: text or binary.")
	);
	
	start_form("custom_sms.php", "get");
	addHidden(null,array("method"=>"send_message_to_yate"));
	editObject(null,$fields,"Create SMS.","Send SMS",null,true);
	end_form();
}

function send_message_to_yate()
{
	global $default_ip, $default_port;

	$called = trim(getparam("called"));
	$text = trim(getparam("text"));

	//test required data to send the SMS 
	if (!$called || !$text)
		return send_sms("Insufficient data to send the SMS request!");

	if ($called == "") 
		return send_sms("The IMSI cannot be empty. Please insert IMSI.");

	if ($text == "")
		return send_sms("The Message cannot be empty. Please insert the message.");
	
	if (preg_match("/=/", $text))
		return send_sms("The Message cannot contain '='.");


	$socket = new SocketConn($default_ip, $default_port);
	if (strlen($socket->error))
		return send_sms($socket->error);
	

	$response = $socket->command("debug off", "quit");
	$response = $socket->command("sniffer off", "quit");
	
	//test if called is online 
	$command = "nib registered ". $called;
	$response = $socket->command($command, "quit");
	
	if (!preg_match("/".$called ." is registered./i", $response)) {
		$socket->close();
		return send_sms(null, "The subscriber ". $params["called"]." is not online, try later to send the SMS.");
	}
	$command = "javascript load custom_sms";
	$response = $socket->command($command, 'quit');

	if (preg_match("/Failed to load script from file 'custom_sms.js'/i", $response))
		return send_sms("Failed to load script from file 'custom_sms.js'. Please check logs."); 

	$command = "control custom_sms called=".$called;
	$type_sms = getparam("sms_type");

	if ($type_sms == "text") 
		$command .= " text=";
	else
		$command .= " rpdu=";
	$command .= $text;

	$response = $socket->command($command, 'quit');
	$socket->close();
	
	if (preg_match("/Could not control /i", $response)) 
		return send_sms("Internal error, script custom_sms.js didn't handle chan.control.");

	$note = "Problems encounted while sending custom SMS.";
	if (preg_match("/Control 'custom_sms' ([[:print:]]+)\r/i", $response, $match)) 
		$note = $match[1];
	switch ($note) {
		case "FAILED":
			$note = "SMS was not sent.";
			break;
		case "OK":
			$note =  "SMS was sent.";
			break;
	}
	
	send_sms(null, $note);
}
?>
