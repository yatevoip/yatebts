<?php
/**
 * debug.php
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


if(is_file("defaults.php"))
	include_once("defaults.php");
if (is_file("config.php"))
	include_once("config.php");

class Debug
{
	public static function output($msg)
	{
		global $logs_in;

		if(!isset($logs_in))
			$logs_in = "web";

		$arr = $logs_in;
		if(!is_array($arr))
			$arr = array($arr);

		for($i=0; $i<count($arr); $i++) {
			if($arr[$i] == "web") {
				print "<br/>\n<br/>\n$msg<br/>\n<br/>\n";
			}else{
				$date = gmdate("[D M d H:i:s Y]");
				if(!is_file($arr[$i]))
					$fh = fopen($arr[$i], "w");
				else
					$fh = fopen($arr[$i], "a");
				fwrite($fh, $date.' '.$msg."\n");
				fclose($fh);
			}
		}
	}
}
?>
