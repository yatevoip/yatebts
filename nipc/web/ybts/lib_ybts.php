<?php
/**
 * lib_ybts.php
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


require_once("ansql/lib_files.php");
require_once("check_validity_fields_ybts.php");
require_once("ybts_fields.php");
require_once("ybts_menu.php");

/* Display form fields with their values. */
function create_form_ybts_section($section, $subsection, $fields_sect_err=false, $error=null, $error_fields=array())
{
	global $yate_conf_dir;

	$structure = get_fields_structure_from_menu();
	$filename = $yate_conf_dir. "ybts.conf";	

	//read config parameters from conf file (ybts.conf) 
	if (file_exists($filename)) {
		$ybts = new ConfFile($filename, true, true, "\n\n");
		$fields = create_fields_from_conffile($ybts->structure); 
	}

	//if the params are not set in ybts get the default values to be displayed
	if (!isset($fields))  
		$fields = get_default_fields_ybts();
	
	if ($fields_sect_err) {//get fields value from getparam
		foreach($structure as $m_section => $params) {
	        	foreach ($params as $key => $m_subsection) {
				foreach($fields[$m_section][$m_subsection] as $param_name => $data) {
					$paramname = str_replace(".", "_", $param_name);
					if ($data["display"] == "select") 
						$fields[$m_section][$m_subsection][$param_name][0]["selected"] = getparam($paramname);
	                                elseif ($data["display"] == "checkbox")
						$fields[$m_section][$m_subsection][$param_name]["value"] = getparam($paramname)=="on"? "1" : "0";
	                                else
		                                $fields[$m_section][$m_subsection][$param_name]["value"] = getparam($paramname);

				}
			}
		}
	}

	print "<div id=\"err_$subsection\">";
	error_handle($error, $fields[$section][$subsection],$error_fields);
	print "</div>";
	start_form();
	foreach($structure as $m_section => $data) {
		foreach($data as $key => $m_subsection) {
			$style = $m_subsection == $subsection ? "" : "style='display:none;'";
			
			print "<div id=\"$m_subsection\" $style>";
			if (!isset($fields[$m_section][$m_subsection])) {
				print "Could not retrieve ybts parameters for $m_subsection.";
				print "</div>";
				continue;
			}
			editObject(null,$fields[$m_section][$m_subsection],"Set parameters values for section [$m_subsection] to be written in ybts.conf file.");
			print "</div>";
		}
	}
	addHidden("database");
	end_form();
}

/* Returns true if found differences between the fields set in FORM and the ones from file (if file is not set from default fields) */
function get_status_fields($structure) 
{
	global $yate_conf_dir;

	$filename = $yate_conf_dir. "ybts.conf";
	$ybts_fields_modified = false;
	
	if (file_exists($filename)) {
	         $ybts = new ConfFile($filename, true, true, "\n\n");
	         $fields = create_fields_from_conffile($ybts->structure, true);
	} else 
		return true;

	if (isset($fields["not_in_file"])) {
		unset($fields["not_in_file"]);
		return true;
	}

	foreach($structure as $m_section => $data) {
		foreach($data as $key => $m_subsection) {
			foreach($fields[$m_section][$m_subsection] as $param_name => $data) {
				$paramname = str_replace(".", "_", $param_name);
				$val_req = getparam($paramname);
				if ($data["display"] == 'checkbox') {
					$value = $data["value"];
					if ($data["value"] != "on" && $data["value"] != "off")
						$value = $data["value"] == "1" ? "on" : "off";
					$val_req = getparam($paramname) ? getparam($paramname) : "off";
				} elseif ($data["display"] == 'select')
					$value = isset($data[0]["selected"]) ? $data[0]["selected"] : "";	
				else
					$value = isset($data["value"]) ? $data["value"] : "";

				if ($value != $val_req) {
					$ybts_fields_modified = true;
					break 3;
				}
			}
		}
	}
	return $ybts_fields_modified;
}

/* Put the values that were set in ybts.conf file for each section from default $fields. */
function create_fields_from_conffile($fields_from_file,$exists_in_file = false)
{
	$structure = get_fields_structure_from_menu();

	$fields = get_default_fields_ybts();

	foreach($structure as $section => $data) {
		foreach($data as $key => $subsection) {
			if ($exists_in_file) {
				if (!isset($fields_from_file[$subsection])) {
					$fields["not_in_file"] = true;
					break 2;
				}

				foreach($fields[$section][$subsection] as $paramname => $data_fields) {
					$allow_empty_params = array("Args", "DNS", "ShellScript", "MS.IP.Route", "Logfile.Name", "peer_arg");
					if (!in_array($paramname, $allow_empty_params) && !isset($fields_from_file[$subsection][$paramname])) {
						$fields["not_in_file"] = true;	
						break 3;
					}
				}
			}
		}
	}

	$network_map = "";
	foreach($structure as $section => $data) {
		foreach($data as $key => $subsection) {
			if (isset($fields_from_file[$subsection])) {
				foreach($fields_from_file[$subsection] as $param => $data) {
					if (is_numeric($param))  //keep the original comments from $fields
						continue;
					if (!isset($fields[$section][$subsection]))
						continue;
					if ($subsection=="gprs_roaming" && substr($param,0,2)=="__") {
						if (strlen($network_map))
							$network_map .= "\n";
						$network_map .= substr($param,2)."=$data";
						continue;
					}
					if ($subsection=="gprs_roaming" && $param=="nnsf_bits")
						$param = "gprs_nnsf_bits";
					if (!isset($fields[$section][$subsection][$param])) 
						continue;

					if ($fields[$section][$subsection][$param]["display"] == "select") 
						$fields[$section][$subsection][$param][0]["selected"] = $data;
					elseif ($fields[$section][$subsection][$param]["display"] == "checkbox") 
						$fields[$section][$subsection][$param]["value"] = $data == "yes" ? "on" : "off";
					else 
						$fields[$section][$subsection][$param]["value"] = $data; 
				}
			}
		}
	}
	if (strlen($network_map)) 
		$fields["YBTS"]["gprs_roaming"]["network_map"]["value"] = $network_map;

	if (isset($fields['GSM']['gsm']['Radio.Band'][0]["selected"])) {
		$particle = $fields['GSM']['gsm']['Radio.Band'][0]["selected"];
		$fields['GSM']['gsm']["Radio.C0"][0]["selected"] = "$particle-".$fields['GSM']['gsm']["Radio.C0"][0]["selected"];
	}
	return $fields;
}

/*
 * For each param from form apply the specific validity function callback to validate parameter from configuration file.
 * See call_function_from_validity_field for more explanations.
 */ 
function validate_fields_ybts($section, $subsection)
{
        // this array contains the name of the params that can be empty in configuration file (ybts.conf)
	$allow_empty_params = array("Args", "DNS", "ShellScript", "MS.IP.Route", "Logfile.Name", "peer_arg", "RadioFrequencyOffset", "TxAttenOffset", "Radio.RxGain", "my_sip", "reg_sip", "nodes_sip", "gstn_location", "neighbors", "gprs_nnsf_bits", "nnsf_dns", "network_map", "local_breakout" );
	
	$fields = get_default_fields_ybts();
	$new_fields = array();
	$error_field = array();
	$warning_field  = array();
	
	if (!isset($fields[$section][$subsection]))
		return array(true);

	foreach ($fields[$section][$subsection] as $param_name => $data) {
		$paramname = str_replace(".", "_", $param_name);
		$new_fields[$section][$subsection][$param_name] = $data;

		$field_param = getparam($paramname);
		if (isset($data["display"]) && $data["display"] == "checkbox" && $field_param == NULL) 
			$field_param = "off";
		$field_param = htmlspecialchars_decode($field_param);

		if (!valid_param($field_param)) {
			if (!in_array($param_name, $allow_empty_params))
				$error_field[] = array("Field $param_name can't be empty.", $param_name);
		}

		$res = array(true);
		if ($data["display"] == "select" && !isset($data["validity"])) 
			$res = check_valid_values_for_select_field($param_name, $field_param, $data[0]);
		elseif (isset($data["validity"])) 
			$res = call_function_from_validity_field($data["validity"], $param_name, $field_param);

		if (!$res[0])
			$error_field[] = array($res[1], $param_name);
		elseif ($res[0] && isset($res[1]))
			$warning_field[] = array($res[1], $param_name);

		// set to "" parameters that are not set or are set to "Factory calibrated"
		// they will be written commented in ybts.conf
		if (!valid_param($field_param) || $field_param=="Factory calibrated")
			$field_param = "";

		if ($data["display"] == 'select')
			$new_fields[$section][$subsection][$param_name][0]["selected"] = $field_param;
		else
			$new_fields[$section][$subsection][$param_name]["value"] = $field_param;
	}

	// validate roaming mode params if roaming mode is activated
	if (getparam("mode")=="roaming" && $subsection=="roaming") {
		$res = validate_roaming_params();
		if (!$res[0])
			$error_field[] = array($res[1], $param_name);
		elseif ($res[0] && isset($res[1]))
			$warning_field[] = array($res[1], $param_name);
	}

	if (getparam("mode")=="dataroam" && $subsection=="gprs_roaming") {
		$res = validate_dataroam_params();
		if (!$res[0])
			$error_field[] = array($res[1], $param_name);
		elseif ($res[0] && isset($res[1]))
			$warning_field[] = array($res[1], $param_name);
	}
	if (getparam("mode")=="dataroam" && $subsection=="roaming") {
		$res = validate_piece_roaming();
		if (!$res[0])
			$error_field[] = array($res[1], $param_name);
		elseif ($res[0] && isset($res[1]))
			$warning_field[] = array($res[1], $param_name);
	}

	$warning = "";
	$warning_fields = array();
	foreach ($warning_field as $key => $err) {
		$warning .=  $err[0]."<br/>";
		$warning_fields[] =  $err[1];
	}

	//if no error found return the new fields from form data
	if (!count($error_field))
		return array(true,"fields"=> $new_fields, "warning"=>$warning, "warning_fields"=>$warning_fields);

	$error = "";
	$error_fields = array();

	foreach ($error_field as $key => $err) {
		$error .=  $err[0]."<br/>";
		$error_fields[] =  $err[1];
	}

	return array(false,"fields"=>$new_fields,"error"=>$error,"error_fields"=>$error_fields,"warning"=>$warning, "warning_fields"=>$warning_fields);
}

function validate_roaming_params()
{
	$required = array("nnsf_bits", "gstn_location");

	foreach ($required as $field_name)
		if (!getparam($field_name))
			return array(false, "Field $field_name is required in roaming mode.", array($field_name));

	$reg_sip = getparam("reg_sip");
	$nodes_sip = getparam("nodes_sip");
	if (!$reg_sip && !$nodes_sip)
		return array(false, "You need to set 'Reg sip' or 'Nodes sip' in roaming mode.", array("reg_sip", "nodes_sip"));

	$nnsf_bits = getparam("nnsf_bits");
	$expires = getparam("expires");
	if (valid_param($expires) && !is_valid_number($expires))
		return array(false, "Field 'Expires' field should be numeric.", array("expires"));
	if (valid_param($nnsf_bits) && (!is_valid_number($nnsf_bits) || $nnsf_bits<0))
		return array(false, "Field 'NNSF bits' should be a positive int.", array("nnsf_bits"));

	return array(true);
}

function validate_piece_roaming()
{
	// verify fields from roaming as well, if set
	$nnsf_bits = getparam("nnsf_bits");
	$expires = getparam("expires");
	if (valid_param($expires) && !is_valid_number($expires))
		return array(false, "Field 'Expires' field should be numeric.", array("expires"));
	if (valid_param($nnsf_bits) && (!is_valid_number($nnsf_bits) || $nnsf_bits<0))
		return array(false, "Field 'NNSF bits' should be a positive int.", array("nnsf_bits"));


	return array(true);
}

function validate_dataroam_params()
{
	$nnsf_bits = getparam("gprs_nnsf_bits");
	if (valid_param($nnsf_bits) && (!is_valid_number($nnsf_bits) || $nnsf_bits<0))
		return array(false, "NNSF bits must be a positive int, if set.", array("gprs_nnsf_bits"));

	$map = getparam("network_map");
	if (!strlen($map))
		return array(true);
	$map = explode("\n", $map);
	foreach ($map as $map_entry) {
		$entry = explode("=",$map_entry);
		if (count($entry)!=2)
			return array(false, "Invalid format for 'Network map'.", array("network_map"));
		if (Numerify($entry[0])=="NULL")
			return array(false, "Invalid value '".$entry[0]."'. Should be numeric.", array("network_map"));
		if (filter_var(trim($entry[1]),FILTER_VALIDATE_IP)===false)
			return array(false, "Invalid value '".$entry[1]."'. Should be a valid IP address.", array("network_map"));
	}

	return array(true);
}

/* Makes the callback for the function set in "validity" field from array $fields (from ybts_fields.php).  
 * Note: There are 4 types of callbacks:(call_function_from_validity_field will make the callback for the last 3 cases)
 * - a generic function that test if a value is in an array (check_valid_values_for_select_field($param_name, $field_param, $select_array)) this is not set in "validity" field
 * this function is triggered by $fields[$section][$subsection]["display"] == "select"
 * - a generic function that test if a value is in a specific interval OR in a given regex(check_field_validity($field_name, $field_value, $min=false, $max=false, $regex=false))
 * this is triggered from $fields[$section][$subsection]["validity"] field with the specific parameters.
 * Ex: "validity" => array("check_field_validity", false, false, "^[0-9]{2,3}$") OR 
 *     "validity" => array("check_field_validity", 0, 65535) 
 * - a specified function name with 3 parameter: field_name, field_value, $restricted_value is the getparam("field_value")
 * this is triggered from $fields[$section][$subsection]["validity"] that contains the name of the function and the specific parameters. Ex: "validity" => array("check_uplink_persistent", "Uplink_KeepAlive")
 * - a specified function name with 2 parameters: field_name, field_value
 * this is triggered from $fields[$section][$subsection]["validity"] that contains only the name of the function. Ex: "validity" => array("check_timer_raupdate") 
 */
function call_function_from_validity_field($validity, $param_name, $field_param)
{
	if (function_exists("call_user_func_array")) { //this function exists in PHP 5.3.0
		$total = count($validity);
		$args = array($param_name,$field_param);
		
		for ($i=1; $i<$total; $i++) 
			$args[] = $total==2 ? getparam($validity[$i]) : $validity[$i];

		//call a function ($validity[0] = the function name; and his args as an array)
		$res = call_user_func_array($validity[0],$args);
	} else {
		$func_name = $validity[0];
		if (count($validity)>= 3) {
			$param1 = $validity[1];
			$param2 = $validity[2];
			$param3 = isset($validity[3])? $validity[3] : false;
			//call to specified validity function
			$res = $func_name($param_name, $field_param,$param1,$param2,$param3);
		} else  {
			if (!isset($validity[1])) 
				$res = $func_name($param_name, $field_param);
			else
				$res = $func_name($param_name, $field_param, getparam($validity[1]));
		}
	}
	return $res;
}


function get_package_marker($structure, $start_marker)
{
	// retrieve existing commencts for mode param in ybts section
	$old_comments = array();
	foreach ($structure["ybts"] as $ind=>$ind_value) {
		if (is_numeric($ind))
			$old_comments[] = $ind_value;
		elseif ($ind!="mode")
			$old_comments = array();
		else
			break;
	}

	// see if we have package_marker between them
	foreach ($old_comments as $ind=>$ind_value)
		if (substr($ind_value,0,strlen($start_marker))==$start_marker)
			return $ind_value;

	return null;
}

/*
 * This function will write the new values of the parameters for each SECTION 
 * the comments are written one time when the section is created (they are not overwritten every time the section is updated)
 */ 
function write_params_conf($fields)
{
	global $yate_conf_dir;

	$c0 = $fields[0]['GSM']['gsm']['Radio.C0'][0]['selected'];
	$c0 = explode("-",$c0);
	$c0 = $c0[1];
	$fields[0]['GSM']['gsm']['Radio.C0'][0]['selected'] = $c0;

	$structure = get_fields_structure_from_menu(); 

	$filename = $yate_conf_dir. "ybts.conf";
	$ybts = new ConfFile($filename,true,true,"\n\n");
	// old structure
	$structure = $ybts->structure;
	$ybts = new ConfFile($filename,false,true,"\n\n");

	$dont_copy = array();
	$start_marker = "; Installed by yate-bts-";
	$found_marker = false;
	$package_marker = null;

	foreach ($fields as $key => $arr) {
		foreach($arr as $section => $params) {
			$i=0;
			foreach ($params as $subsection => $data1) {
				$dont_copy[$subsection] = array();
				foreach ($data1 as $param => $data) {

					if ($subsection=="gprs_roaming" && $param=="gprs_nnsf_bits")
						$param = "nnsf_bits";

					// if this is the ybts subsection and param is mode
					// make sure we don't delete package markers
					// are not deleted: Installed by ...
					if ($subsection=="ybts" && $param=="mode") 
						$package_marker = get_package_marker($structure,$start_marker);

					// write the comments
					$comment_arr = explode("<br/>", $data["comment"]);
					$total = count($comment_arr);
					for ($j = 0; $j< $total; $j++) {
						if ( $package_marker &&
						    substr($comment_arr[$j],0,strlen($start_marker))==$start_marker )
							$found_marker = true;

						$ybts->structure[$subsection][$i] = ";".$comment_arr[$j];
						$i++;
					}
					// add package_marker to comments if we have one and a marker was not already added
					if ($package_marker && !$found_marker) {
						$ybts->structure[$subsection][$i] = $package_marker;
						$i++;
						$package_marker = null;
					}

					//prepare the data to be written
					if (isset($data[0]["selected"]))
						$value = $data[0]["selected"];
					elseif ($data["display"] == "checkbox")
						$value = $data["value"]=="on"? "yes" : "no";
					else
						$value = $data["value"];

					if (strlen($value)) {
						if ($subsection=="gprs_roaming" && $param=="network_map") {
							$network_map = explode("\n",$value);
							foreach ($network_map as $map_entry) {
								$expl = explode("=", $map_entry);
								$ybts->structure[$subsection][$expl[0]] = array($expl[1]);
							}
						} else
							$ybts->structure[$subsection][$param] = array($value);
					} else {
						$dont_copy[$subsection][] = $param;
						// write field commented
						$ybts->structure[$subsection][$i] = ";$param=\n";
						$i++;
					}
				}
			}
		}
	}

	// add unknown sections at the end of the file and unknown parameters that don't start with __ at the end of each section
	foreach($structure as $subsection_name=>$subsection) {
		if (!isset($ybts->structure[$subsection_name]))
			$ybts->structure[$subsection_name] = $subsection;
		else
			foreach ($subsection as $field_name=>$field_value) {
				if (is_numeric($field_name))
					continue;
				if (in_array($field_name,$dont_copy[$subsection_name]))
					continue;
				if (!isset($ybts->structure[$subsection_name][$field_name]) && substr($field_name,0,2)!="__")
					$ybts->structure[$subsection_name][$field_name] = $field_value;
			}
	}

	$ybts->save();

	if ($ybts->getError())
		return array(false, $ybts->getError());
	else { 
		$res = restart_yate();

		if ($res[0] && isset($res[1])) //yate is not running
			return array(true, "Finished writting sections in ybts.conf file without issues. " .$res[1]);
		elseif (!$res[0]) //errors on socket connection
			return array(true, "Finished writting sections in ybts.conf file without issues. For changes to take effect please restart YateBTS.");
		else //yate was restarted
			return array(true, "Finish writting sections in ybts.conf file without issues.");
	}
}
?>
