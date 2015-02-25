<?php
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
if (getparam("called"))
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
		"sms_type" => array("column_name"=>"SMS Type", 'display'=>'select', array("selected"=>"text", "text", "binary"))
	);
	
	start_form("custom_sms.php", "get");
	addHidden(null,array("method"=>"send_message_to_yate"));
	editObject(null,$fields,"Create SMS.",array("Send SMS"),null,true);
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

	//test if called is on line 
	$command = "nib registered ". $called;
	$marker_end = 'null';
	$socket = new SocketConn($default_ip, $default_port);
	if (strlen($socket->error))
		return send_sms($socket->error);
	
	$response = $socket->command($command, "quit");
	
	if (!preg_match("/".$called ." is registered./i", $response)) {
		$socket->close();
		return send_sms(null, "The subscriber ". $params["called"]." is not online, try later to send the SMS.");
	}

	
	$command = "control custom-sms called=".$called;
	$type_sms = getparam("sms_type");

	if ($type_sms == "text") 
		$command .= " text=";
	else
		$command .= " rpdu=";
	$command .= $text;

	$response = $socket->command($command, 'quit');
	$socket->close();

	if (preg_match("/msg.execute returned false/", $response)) 
		$note = "SMS was not sent.";
	elseif (preg_match("/msg.execute returned true/", $response)) 
		$note =  "SMS was sent.";
	
	send_sms(null, $note);
}
?>
