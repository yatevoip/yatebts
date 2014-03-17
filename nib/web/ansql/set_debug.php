<?php
/**
 * set_debug.php
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


session_start();

if(isset($_SESSION["error_reporting"]))
	ini_set("error_reporting",$_SESSION["error_reporting"]);

if(isset($_SESSION["display_errors"]))
	ini_set("display_errors",true);
//if this was set then errors will be hidden unless set from debug_all.php page
//else
//	ini_set("display_errors",false);

if(isset($_SESSION["log_errors"]))
	ini_set("log_errors",$_SESSION["log_errors"]);
//if this was set then errors will be logged unless set from debug_all.php page
//else
//	ini_set("log_errors",false);

if(isset($_SESSION["error_log"]))
	ini_set("error_log", $_SESSION["error_log"]);

?>
