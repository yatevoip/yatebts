<?php
/**
 * ybts_menu.php
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


/* builds the structure of the to display the menu and the submenu */
function get_menu_structure()
{
	global $private_version;

	//The key is the MENU alias the sections from $fields 
	//and for each menu is an array that has the submenu data with subsections 
	$structure = array(
		"GSM" => array("GSM","GSM Advanced"),
		"GPRS" => array("GPRS", "GPRS Advanced", "SGSN", "GGSN"),
		"Control" => array(),
		"Transceiver" => array(),
		"Tapping" => array(),
		"Test" => array(),
		"YBTS" => array("YBTS","Security","Roaming","Handover")
	);

	if (isset($private_version) && $private_version==true)
		$structure["YBTS"][] = "GPRS Roaming";

	return $structure;
}

function get_fields_structure_from_menu()
{
	$structure = get_menu_structure();
	$fields_structure = array();
	foreach($structure as $section => $data) {
		if (!$data)
			$data = array(strtolower($section));
		foreach($data as $key => $subsection) {
			$subsection = str_replace(" ", "_",strtolower($subsection));
			if (!$subsection)
				$subsection = strtolower($section);
			$fields_structure[$section][] = $subsection;
		}
	}
	return $fields_structure;
}

/* display the menu and the submenu*/
function ybts_menu()
{
	global $section, $subsection;
	
	$structure = get_menu_structure();


//Create the open/close menu structure
?>
<script type="text/javascript">
	var subsections = new Array();
	var sect_with_subsect = new Array();
<?php
	$i = $j = 0; 
	foreach ($structure as $j_section => $j_subsections) {
			$j_section = str_replace(" ", "_",strtolower($j_section));
		if (count($j_subsections)) {
			echo "sect_with_subsect[\"" . $j . "\"]='" . $j_section . "';";
			$j++;
		} else { 
			echo "subsections[\"" . $i . "\"]='" . $j_section . "';";
			$i++;
		}



		foreach ($j_subsections as $key => $j_subsection) {
	                 if (isset($j_subsection)) {
				 $j_subsection = str_replace(" ", "_",strtolower($j_subsection));
				 echo "subsections[\"" . $i . "\"]='" . $j_subsection . "';";
				 $i++;
			 }  
		}
	}

?>	
</script>
<table class="menu" cellspacing="0" cellpadding="0">
<tr>
<?php
	$i=0;
	$total = count($structure);
	foreach($structure as $menu => $submenu) {
		$subsect = strtolower($menu);
		if ($menu == $section)
			$css = "open";
		else
			$css = "close";
		print "<td class='menu_$css' id='section_$i' onclick=\"show_submenu_tabs($i, $total, '$subsect')\">". $menu."</td>";
		print "<td class='menu_space'>&nbsp;</td>";
		if ($i == $total-1)
			print "<td class='menu_empty'>&nbsp;</td>";
		$i++;
	}
?>
</tr>
</table>
</td>
</tr>
<tr><td class="submenu" id="submenu_line" colspan="2">
<!--Create the submenu structure -->
<table class="submenu" cellspacing="0" cellpadding="0">
<tr> <td>
<?php
	$css = "open";
	$i = 0;
	foreach($structure as $menu => $submenu) {
		if ($menu == $section) 
			$style = "";
		else
			$style = "style='display:none;'";
		if (count($submenu)<1) {
			$i++;
			continue;
		}
		print "<div class='submenu' id='submenu_$i' $style>";
		foreach ($submenu as $key => $name) {
			if (str_replace(" ", "_",strtolower($name)) == $subsection) 
				$css = "open";
			else
				$css = "close";
			$link = str_replace(" ","_",strtolower($name));
			print "<div class='submenu_$css' id=\"tab_$link\" onclick=\"show_submenu_fields('$link')\">".$name."</a></div>";
			print "<div class='submenu_space'>&nbsp;</div>";
		}
		$i++;
		print '</div>';
	}
?>
</td></tr>
</table>
<?php
}

/* Displayes the explanation of each subsection. */
function description_ybts_section()
{
	global $subsection;

	$section_desc = array(
		"gsm" => "Section [gsm] controls basic GSM operation.
		You MUST set and review all parameters here before starting the BTS!",
		"gsm_advanced" => "Section [gsm_advanced] controls more advanced GSM features.
		You normally don't need to make changes in this section",
		"gprs" => "Section [gprs] controls basic GPRS operation.
		You should review all parameters in this section.",
		"gprs_advanced" => "Section [gprs_advanced] controls more advanced GPRS features.
		You normally don't need to make changes in this section.",
		"ggsn" => "Section [ggsn] has internal GGSN function configuration.",
		"sgsn" => "Section [sgsn] has internal SGSN function configuration.",
		"control" => "Section [control] - configuration for various MBTS controls.
		You normally don't need to change anything in this section.",
		"transceiver" => "Section [transceiver] controls how the radio transceiver is started and operates.",
		"tapping" => "Section [tapping] - settings control if radio layer GSM and GPRS packets are tapped to Wireshark.",
		"test" => "Section [test] has special parameters used to simulate errors.",
		"ybts" => "Section [ybts] configures ybts related parameters.",
		"security" => "Section [security] configures security related parameters.",
		"roaming" => "Section [roaming] controls parameters used by roaming.js when connecting YateBTS to a core network.",
		"handover" => "Section [handover] controls handover parameters used by roaming.js.",
		"gprs_roaming" => "Section [gprs_roaming] controls parameters used by dataroam.js when connecting YateBTS to a core data network."
	);

	foreach ($section_desc as $subsect => $desc) {
		$style = (isset($section_desc[$subsection]) && $subsect == $subsection) ? "" : "style='display:none;'";
		print "<div id=\"info_$subsect\" $style>". $desc ."</div>";
	}

}

?>
