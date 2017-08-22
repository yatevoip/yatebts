<?php
/**
 * call_logs.php
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

function call_logs()
{
	global $limit, $yate_conf_dir, $yate_cdr;

	$conf = new ConfFile($yate_conf_dir."cdrfile.conf");
	if ($conf->getError()) {
		nib_note($conf->getError());
		return;
	}

	if (!is_file($yate_cdr)) {
	       
		if (!isset($conf->structure["general"]["file"]) ) 
			form_enable_call_logs();
		elseif (isset($conf->structure["general"]["file"]) && $conf->structure["general"]["file"] != $yate_cdr) 
			nib_note("Detected differences between the CDR file settings. The file set in configuration file: '".$yate_conf_dir. "cdrfile.conf' in parameter 'file=". $conf->structure["general"]["file"]."' is different from the one set in 'defaults.php' in parameter: '\$yate_cdr= ".$yate_cdr."'. Please modify the value in defaults.php in parameter: '\$yate_cdr = ".$conf->structure["general"]["file"]."' or modify in '".$yate_conf_dir. "cdrfile.conf' in [general] section the value of parameter 'file=".$yate_cdr."'" );
		else 
			nib_note("There aren't any call logs.");

	} else {
	
		$ext = explode(".",$yate_cdr);
		if ($ext[1] != "csv") {
			nib_note("Call logs must be in CSV format. Please modify the filename in 'defaults.php'.");
			return;
		}

		if (!is_readable($yate_cdr)) {
			nib_note("Don't have permission to read the call logs file: '".$yate_cdr."'. Please run this command as root: 'chmod -R a+r ".$yate_cdr."'");
			return;
		}

		$show_next = getparam("next");
		if ($show_next && isset($_SESSION["changed_limit"]) && $limit == $_SESSION["changed_limit"])
			$call_logs = get_file_last_lines($yate_cdr, $limit, 512, $_SESSION["offset"], $_SESSION["leftover"], $_SESSION["lines"]);
		else {
			$call_logs = get_file_last_lines($yate_cdr, $limit);
			$_SESSION["changed_limit"] = $limit;
		}

		$_SESSION["offset"] = $call_logs[1];
		$_SESSION["leftover"] = $call_logs[2];
		$_SESSION["lines"] = $call_logs[3];	

		$call_logs = $call_logs[0];

		if (empty($call_logs[0]))
			return table(array(), array(), "call log", "call_logs");

		$calls = $calls_format = array();
		foreach($call_logs as $k => $call_log)
			$calls[$k] = str_getcsv($call_log,",");

		$formats = array("time","billid","chan","address","caller","called","billtime","ringtime","duration","direction","status","reason");

		foreach ($calls as $k => $call) {
			if (count($call) != count($formats))
				continue;
			for ($i=0; $i<count($formats);$i++) { 
				$value = isset($call[$i])? str_replace(array('"="',"'='",'"'), '',$call[$i]) : "";
				if ($formats[$i] == "time") 
					$value = date('Y-m-d h:i:s', $call[$i]);

				$calls_format[$k][$formats[$i]] = $value;
			}
		}

		items_on_page();
		if (count($calls_format) < $limit)
			print "<div class='next_lines'><a class='content' href='main.php?module=call_logs'>Return</a></div>";
		else
			print "<div class='next_lines'><a class='content' href='main.php?module=call_logs&method=call_logs&next=1&limit=$limit'>View next call logs</a></div>";
		table($calls_format, $formats, "call log", "call_logs");
	}
}

function form_enable_call_logs()
{
	global $yate_cdr;

	$fields = array("enable_writing_of_call_logs" => array("display"=>"checkbox", "comment"=>"If enabled the CDRs will be written in '$yate_cdr' file."));

	start_form();
	addHidden("enabled");
	editObject(NULL,$fields, "Enable writing call logs in file", "Save");
	end_form();
}

function call_logs_enabled()
{
	global $yate_conf_dir, $yate_cdr;

	if (getparam("enable_writing_of_call_logs") == "on") {
		$conf = new ConfFile($yate_conf_dir."cdrfile.conf");
		if ($conf->getError()) {
			nib_note($conf->getError());
			return call_logs();
		}
		$conf->openForWrite();
		if (!$conf->status()) {
			nib_note($conf->getError());
			return call_logs();
		}

		$conf->structure["general"] = array("file"=>$yate_cdr, "tabs"=>"false");
		$conf->save();

		$res = restart_yate();
		if ($res[0] && isset($res[1])) //yate is not running
			notice("Finished setting file for writting CDRs. " .$res[1], "call_logs");
		elseif (!$res[0]) //errors on socket connection
			notice("Finished setting file for writting CDRs. For changes to take effect please restart yate or reload just nib.js from telnet with command: \"javascript reload nib\".", "call_logs");
		else //yate was restarted
			notice("Finished setting file for writting CDRs.", "call_logs");
	}  else 
		return call_logs();
}
?>
