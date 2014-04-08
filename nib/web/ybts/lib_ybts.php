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
function create_form_ybts_section($section, $subsection, $fields= array(), $error=null, $error_fields=array())
{
	global $yate_conf_dir;

	$structure = get_fields_structure_from_menu();
        $filename = $yate_conf_dir. "ybts.conf";	
	$default_fields = get_default_fields_ybts();

	//read config parameters from conf file (ybts.conf) 
	if (file_exists($filename) && !isset($_SESSION["ybts_params"])) {
		$ybts = new ConfFile($filename);
		$fields = create_fields_from_conffile($ybts->structure); 
	}

	if (isset($_SESSION["ybts_params"])) {
		foreach($structure as $m_section => $data) {
			foreach($data as $key => $m_subsection) 
				if (isset($_SESSION["ybts_params"][$m_section][$m_subsection]))
					$fields[$m_section][$m_subsection] = $_SESSION["ybts_params"][$m_section][$m_subsection];
			if (!isset($fields[$m_section][$m_subsection])) 
				$fields[$m_section][$m_subsection] = $default_fields[$m_section][$m_subsection];
		}
	}

	//if the params are not set in file get the default values to be displayed
	if (count($fields) == 0) 
		$fields = $default_fields;

	print "<div id=\"err_$subsection\">";	
	error_handle($error, $fields[$section][$subsection],$error_fields);
	print "</div>";
	start_form();
	foreach($structure as $m_section => $data) {
		foreach($data as $key => $m_subsection) {
			$style = $m_subsection == $subsection ? "" : "style='display:none;'";
			
			print "<div id=\"$m_subsection\" $style>";
			if (!isset ($fields[$m_section][$m_subsection])) {
				print "Could not retrieve ybts parameters for $m_section.";
				return;
			}
			editObject(null,$fields[$m_section][$m_subsection],"Set parameters values for section [$m_subsection] to be written in ybts.conf file.");
			print "</div>";
		}
	}
	addHidden("database");
	end_form();
}

function set_fields_in_session()
{
	global $yate_conf_dir, $ybts_fields_modified;
	

	$fields = array();
	$filename = $yate_conf_dir. "ybts.conf";

	//global parameter. If false means that the ybts fields were not changed when submitting the forms.
	$ybts_fields_modified = false;

	$structure = get_fields_structure_from_menu();
	//read config parameters from conf file (ybts.conf) 
	if (file_exists($filename)) {
                   $ybts = new ConfFile($filename);
		   $fields = create_fields_from_conffile($ybts->structure);
	}      
	if (!count($fields))
		$fields = get_default_fields_ybts();	

	$ybts_fields_modified = get_status_fields($fields, $structure);

	//set values from FORM in session
	foreach($structure as $m_section => $data) {
		foreach($data as $key => $m_subsection) {
			foreach($fields[$m_section][$m_subsection] as $param_name => $data) {
				$paramname = str_replace(".", "_", $param_name);
				$_SESSION["ybts_params"][$m_section][$m_subsection][$param_name] = $data;

				$field_param = getparam($paramname);
				if ($data["display"] == 'select')
					$_SESSION["ybts_params"][$m_section][$m_subsection][$param_name][0]["selected"] = $field_param;
				elseif ($data["display"] == "checkbox")
					$_SESSION["ybts_params"][$m_section][$m_subsection][$param_name]["value"] = $field_param == "on" ? "1" : "0";
				else
					$_SESSION["ybts_params"][$m_section][$m_subsection][$param_name]["value"] = $field_param;
			}
		}
	}
}

/* Returns true if found differences between the fields set in FORM and the ones from file (if file is not set from default fields) */
function get_status_fields($fields, $structure) {

	$ybts_fields_modified = false;

	foreach($structure as $m_section => $data) {
		foreach($data as $key => $m_subsection) {
			foreach($fields[$m_section][$m_subsection] as $param_name => $data) {
				$paramname = str_replace(".", "_", $param_name);
				if (isset($_SESSION["ybts_params"][$m_section][$m_subsection][$param_name])) {
					$val_req = getparam($paramname);
					if ($data["display"] == 'select')
						$value = $_SESSION["ybts_params"][$m_section][$m_subsection][$param_name][0]["selected"];
					elseif ($data["display"] == "checkbox") {
						$value = $_SESSION["ybts_params"][$m_section][$m_subsection][$param_name]["value"]=="1" ? "on" : "off";
						$val_req = getparam($paramname) ? getparam($paramname) : "off";
					} else 
						$value = $_SESSION["ybts_params"][$m_section][$m_subsection][$param_name]["value"];	

				} else {//if fields are not set in session then compare values from POST with the existing value
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

				}
				if ($value != $val_req) {
					$ybts_fields_modified = true;
					break 3;
				}
			}
		}
	}
	return $ybts_fields_modified;
}

/* Put the value that was set in ybts.conf file for each section. */
function create_fields_from_conffile($fields_from_file)
{
	$structure = get_fields_structure_from_menu();

	$fields = get_default_fields_ybts();
	foreach($structure as $section => $data) {
                foreach($data as $key => $subsection) {
			if (isset($fields_from_file[$subsection])) {
				foreach($fields_from_file[$subsection] as $param => $data) {
					if (is_numeric($param))  //keep the original comments from $fields
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
	return $fields;
}

/*
 * For each param from form apply the specific validity function callback to validate parameter from configuration file.
 * See call_function_from_validity_field for more explanations.
 */ 
function validate_fields_ybts($section, $subsection)
{

        // this array contains the name of the params that can be empty in configuration file (ybts.conf)
	$allow_empty_params = array("Args", "DNS", "ShellScript", "MS.IP.Route", "Logfile.Name", "peer_arg");
	
	$fields = get_default_fields_ybts();
	$new_fields = array();
	$error_field = array();
	
	foreach ($fields[$section][$subsection] as $param_name => $data) {
		$paramname = str_replace(".", "_", $param_name);
		$new_fields[$section][$subsection][$param_name] = $data;

		$field_param = getparam($paramname);
		if (isset($data["display"]) && $data["display"] == "checkbox" && $field_param == NULL) 
			$field_param = "off";

		if (!valid_param($field_param)) {
			if (!in_array($param_name, $allow_empty_params))
				$error_field[] = array("This field $param_name can't be empty.", $param_name);
		}

		$res = array(true);
		if ($data["display"] == "select" && !isset($data["validity"])) 
			$res = check_valid_values_for_select_field($param_name, $field_param, $data[0]);
		elseif (isset($data["validity"])) 
			$res = call_function_from_validity_field($data["validity"], $param_name, $field_param);
		
		if (!$res[0])
			$error_field[] = array($res[1], $param_name);

		//skip parameters that are not set (and they are allowed to be empty)
		if (!valid_param($field_param)) {
			if (in_array($param_name, $allow_empty_params)){
				unset($new_fields[$section][$subsection][$param_name]);
				continue;
		       }
		}

		if ($data["display"] == 'select')
			$new_fields[$section][$subsection][$param_name][0]["selected"] = $field_param;
		else
			$new_fields[$section][$subsection][$param_name]["value"] = $field_param;
	}

	//if no error found return the new fields from form data
	if (!count($error_field)) 
		return array(true,"fields"=> $new_fields);

	$error = "";
	$error_fields = array();

	foreach ($error_field as $key => $err) {
		$error .=  $err[0]."<br/>";
		$error_fields[] =  $err[1];
	}
	return array(false, "fields"=>$new_fields,"error"=> $error,"error_fields"=> $error_fields);
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

/*
 * This function will write the new values of the parameters for each SECTION 
 * the comments are written one time when the section is created (they are not overwritten every time the section is updated)
 */ 
function write_params_conf($fields)
{
	global $yate_conf_dir;

	$structure = get_fields_structure_from_menu(); 

	$filename = $yate_conf_dir. "ybts.conf";

	if (file_exists($filename))
		$ybts = new ConfFile($filename);
	else
		$ybts = new ConfFile($filename, false);
	
	foreach ($fields as $key => $arr) {
		foreach($arr as $section => $params) {
			foreach ($params as $subsection => $data1) {
				$i=0;
				 foreach ($data1 as $param => $data) {
					//write comments just the first time when section is written in file
					if (!isset($ybts->structure[$subsection][$i])) 
						$ybts->structure[$subsection][$i] = ";".$data["comment"];

					//prepare the data to be written
					if (isset($data[0]["selected"]))
						$value = array($data[0]["selected"]);
					elseif ($data["display"] == "checkbox")
						$value = array($data["value"]=="on"? "yes" : "no");
					else
						$value = array($data["value"]);

					$ybts->structure[$subsection][$param] = $value;
					$i++;
				 }
			}

		}
	 }
	$ybts->save();

	if ($ybts->getError())
		return array(false, $ybts->getError());
	else 
		return array(true, "Finish writting sections in ybts.conf file without issues. For changes to take effect please restart YateBTS.");
}
?>
