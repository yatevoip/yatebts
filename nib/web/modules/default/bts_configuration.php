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
    <td class="content_info"><?php description_ybts_section(); ?></td>
</tr>
<tr><td class="page_space" colspan="2"> &nbsp;</td></tr>
</table>
<?php
}

function bts_configuration_database()
{
	global $section, $subsection;

	$structure = get_fields_structure_from_menu();
	$errors_found = false;
	$warnings = "";
	foreach ($structure as $m_section => $data) {
		foreach($data as $key => $m_subsection) {
			$res = validate_fields_ybts($m_section, $m_subsection);
			if (!$res[0]) { 
				$errors_found = true;
				$section = $m_section;
				$subsection = $m_subsection;
				$_SESSION["section"] = $m_section;
				$_SESSION["subsection"] = $m_subsection;
				break;
			} else {
				$fields[] = $res["fields"];  
			}
			if (isset($res["warning"])) {
				$warnings .= $res["warning"];
			}
		}
		if ($errors_found)
		       	break;
	}

?>
<table class="page" cellspacing="0" cellpadding="0">
<tr>
    <td class="menu" colspan="2"><?php ybts_menu();?>
<tr> 
	<td class="content_form"><?php 
	if ($errors_found) {
		if (strlen($warnings))
			message("Warning! ".$warnings, "no");
		create_form_ybts_section($section, $subsection, true, $res["error"], $res["error_fields"]);
	}
	else {
		
		$ybts_fields_modified = get_status_fields($structure);
		if (!$ybts_fields_modified) {
			print "<div id=\"notice_$subsection\">";
                        message("Finish editing sections. Nothing to update in ybts.conf file.", "no");
                        print "</div>";
			create_form_ybts_section($section, $subsection);
	?></td>
    	<td class="content_info"><?php description_ybts_section(); ?></td>
</tr>
<tr><td class="page_space" colspan="2"> &nbsp;</td></tr>
</table>
<?php
			return;
		}

		//if no errors encounted on validate data fields then write the data to ybts.conf
		$res1 = write_params_conf($fields);
		if (!$res1[0]) {
			print "<div id=\"file_err_$subsection\">";
			errormess("Errors encountered while writting ybts.conf file: ".$res1[1]);
			print "</div>";
		} else {
			unset($_SESSION["section"], $_SESSION["subsection"]);
			print "<div id=\"notice_$subsection\">";
			message($res1[1], "no");
			print "</div>";
			$res = set_codecs_ysipchan(getparam("mode"));
			if (!$res[0]) {
				errormess($res[1]);
			}
		}
		if (strlen($warnings))
			message("Warning! ".$warnings,"no");

		create_form_ybts_section($section, $subsection);
 }
?></td>
    <td class="content_info"><?php description_ybts_section(); ?></td>
</tr>
<tr><td class="page_space" colspan="2"> &nbsp;</td></tr>
</table>
<?php
}

function set_codecs_ysipchan($mode)
{
	global $yate_conf_dir;

	$filename = $yate_conf_dir. "ysipchan.conf";
	if (is_file($yate_conf_dir. "ysipchan.conf"))
		$file = new ConfFile($filename,true,true);
	else
		$file = new ConfFile($filename,false,true);

	if ($mode=="nib") {
		$file->structure["codecs"] = array();
		$file->structure["codecs"]["default"] = "enable";
		$mess = "default=enable";
	} else {
		$file->structure["codecs"]["default"] = "disable";
		$file->structure["codecs"]["gsm"] = "enable";
		$mess = "<br/>default=disable<br/>gsm=enable<br/>";
	}

	$file->save();
	if (!$file->status())
		return array(false, "Could not set $mess"." in [codecs] section in ysipchan.conf: ".$file->getError());
	return array(true);
}

?>
