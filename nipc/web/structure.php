<?php
/**
 * structure.php
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

if (is_file("defaults.php"))
        require_once("defaults.php");

if (is_file("config.php"))
	require_once("config.php");

require_once("lib/lib_proj.php");

$struct = array();
$struct["default_subscribers"] = array("list_subscribers", "country_code_and_smsc", "online_subscribers", "rejected_IMSIs", "manage_SIMs");

// methods listed here won't be saved in saved_pages -> needed to know where to return for Cancel button or Return button
// besides method listed here, all methods starting with add_,edit_,delete_ are not saved
$exceptions_to_save = array();

?>
