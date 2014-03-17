#! /usr/bin/php -q
<?php
/**
 * force_update.php
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


/*
 * To use make symlink to it from main project directory (it can't be run from this directory)
 *
 * >> ln -s ansql/force_update.php force_update.php
 * >> ./force_update.php
 */

require_once ('ansql/lib.php');
require_once ('ansql/framework.php');

include_classes();
require_once ('ansql/set_debug.php');


$stdout = (php_sapi_name() == 'cli') ? 'php://stdout' : 'web';
if (isset($logs_in)) {
	if (is_array($logs_in)) {
		if (!in_array($stdout,$logs_in))
			$logs_in[] = $stdout;
	} elseif ($logs_in!=$stdout)
		$logs_in = array($logs_in, $stdout);
} else 
	$logs_in = $stdout;

$res = Model::updateAll();
if ($res)
	Debug::Output("Succesfully performed sql structure update.");
// Error is printed from framework
//else
//	Debug::Output("Errors update sql structure. Please see above messages.");

?>
