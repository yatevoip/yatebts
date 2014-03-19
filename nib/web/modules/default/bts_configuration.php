<?php
/**
 * bts_configuration.php
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

require_once("ybts/ybts_menu.php");
require_once("ybts/lib_ybts.php");

function bts_configuration()
{
	global $section, $subsection;

	$res = test_default_config();
	if (!$res[0]) {//permission errors
		errormess($res[1], "no");
		return;
	}

?>
<table class="page" cellspacing="0" cellpadding="0">
<tr>
    <td class="menu" colspan="2"><?php ybts_menu();?></td>
<tr>
    <td class="content_form"><?php create_form_ybts_section($section, $subsection); ?></td>
    <td class="content_info"><?php description_ybts_section($subsection); ?></td>
</tr>
<tr><td class="page_space" colspan="2"> &nbsp;</td></tr>
</table>
<?php
}

function bts_configuration_database()
{
	global $section, $subsection;
?>
<table class="page" cellspacing="0" cellpadding="0">
<tr>
    <td class="menu" colspan="2"><?php ybts_menu();?>
<tr> 
    <td class="content_form"><?php 
	$res = validate_fields_ybts();
	if (!$res[0])
		create_form_ybts_section($section, $subsection, $res["fields"], $res["error"], $res["error_fields"]);
	else {
		  //if no errors encounted on validate data fields then write the data to ybts.conf
		$res1 = write_params_conf($res["fields"][$section][$subsection]);
		if (!$res1[0])
			errormess("Errors encountered while writting ybts.conf file: ".$res1[1]);
		else
			message($res1[1], "no");
		create_form_ybts_section($section, $subsection, $res["fields"]);
       }


?></td>
    <td class="content_info"><?php description_ybts_section($subsection); ?></td>
</tr>
<tr><td class="page_space" colspan="2"> &nbsp;</td></tr>
</table>
<?php
}
?>
