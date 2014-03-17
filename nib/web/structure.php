<?php
if (is_file("defaults.php"))
        require_once("defaults.php");

if (is_file("config.php"))
	require_once("config.php");

require_once("lib/lib_proj.php");

$struct = array();
$struct["default_subscribers"] = array("list_subscribers", "online_subscribers", "rejected_IMSIs");

// methods listed here won't be saved in saved_pages -> needed to know where to return for Cancel button or Return button
// besides method listed here, all methods starting with add_,edit_,delete_ are not saved
$exceptions_to_save = array();

?>
