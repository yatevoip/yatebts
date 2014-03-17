<?php
/**
 * actionlog.php
 * This file is part of the FreeSentral Project http://freesentral.com
 *
 * FreeSentral - is a Web Graphical User Interface for easy configuration of the Yate PBX software
 * Copyright (C) 2008-2009 Null Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

require_once("ansql/framework.php");

// Default class used for logging 
class ActionLog extends Model
{
	public static function variables()
	{
		return array(
					"date" => new Variable("timestamp"),
					"log" => new Variable("text"), // log in human readable form meant to be displayed
					"performer_id" => new Variable("text"), // id of the one performing the action (taken from $_SESSION)
					"performer" => new Variable("text"), // name of the one performing the action (taken from $_SESSION)
					"real_performer_id" => new Variable("text"),
					"object" => new Variable("text"),  // name of class that was marked as performer for actions
					"query" => new Variable("text"), //query that was performed
					"ip" => new Variable("text")
				);
	}

	function __construct()
	{
		parent::__construct();
	}

	public static function index()
	{
		return array(
			"date"
		);
	}

	public static function writeLog($log, $query = NULL)
	{
		// it's important that the next line is placed here
		// in case no object was created then self::$_performers won't be setted
		// self::$_performers is set when the first object derived from model is created
		$actionlog = new ActionLog;
		$performers = self::$_performers;
		$object = '';
		$performer_id = '';
		$performer = '';
		foreach($performers as $object_name=>$performing_columns)
		{
			// check that the necessary fields were defined correctly 
			if(!isset($performing_columns["performer"]) || !isset($performing_columns["performer_id"]))
				continue;
			if($object != '')
				$object .= ",";
			$object .= $object_name;
			$perf_id = (isset($_SESSION[$performing_columns["performer_id"]])) ? $_SESSION[$performing_columns["performer_id"]] : '';
			$perf = (isset($_SESSION[$performing_columns["performer"]])) ? $_SESSION[$performing_columns["performer"]] : '';
			if($performer_id != '')
				$performer_id .= ',';
			$performer_id .= $perf_id;
			if($performer != '')
				$performer .= ',';
			$performer .= $perf;
			$real_performer_id = (isset($_SESSION[$performing_columns["real_performer_id"]])) ? $_SESSION[$performing_columns["real_performer_id"]] : "";
		}
		$actionlog->date = "now()";
		$actionlog->log = $log;
		$actionlog->performer_id = $performer_id;
		$actionlog->performer = $performer;
		$actionlog->real_performer_id = $real_performer_id;
		$actionlog->object = $object;
		$actionlog->query = stripslashes($query);
		if (isset($_SERVER['REMOTE_ADDR']))
			$actionlog->ip = $_SERVER['REMOTE_ADDR'];
		// insert  the log entry whitout trying to retrive the id and without going into a loop of inserting log for log
		$actionlog->insert(false,false);
	}
}
?>
