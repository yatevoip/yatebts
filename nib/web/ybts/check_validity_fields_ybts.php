<?php 
/**
 * check_validity_fields_ybts.php
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

/* test the given param to not be empty string or null */
function valid_param($param)
{
	return ($param !== NULL && $param !== "");
}

/* test the given param to be a valid number */
function is_valid_number($field_value)
{
	//the allowed values are ^-?[1-9][0-9]*|0$
	if (!is_numeric($field_value))
		return false;
	if (strcmp($field_value,(string)(int)$field_value)!== 0 )
		return false;
	return true;
}

/*
 * validate a field value to be in a given array 
 * [Used for SELECT FIELDS]
 */
function check_valid_values_for_select_field($field_name, $field_value, $select_array)
{
	foreach ($select_array as $key => $values) {
		if ($key === "selected" || $key === "SELECTED")
			continue;
		if (is_array($values))
			$selectable_data[] = $values[$field_name."_id"];
		else 
			$selectable_data[] = $values;	
	}

	if (!in_array($field_value, $selectable_data)) 
		return array(false, "The field $field_name is not valid, is different from the allowed values.");

	return array(true);
}

/* 
 * Test the validity of [gsm] Radio.C0 param to be the right value for the chosen Radio.Band
 */ 
function check_radio_band($field_name, $field_value, $restricted_value)
{
	$permitted_values = array(
		"850" => array("128","251"),
		"900" => array("0","124","975","1023"), 
		"1800" => array("512","885"),
		"1900" => array("512","810")
	);

	foreach ($permitted_values as $radio_band_val => $radio_c0_val) {
		if ($restricted_value == $radio_band_val) {
			 $field_value = explode("-",$field_value);
			 $field_value = $field_value[1];
			 $int_value = (int)$field_value;
			 $min = $radio_c0_val[0];
			 $max = $radio_c0_val[1];

			 if (!isset($radio_c0_val[2])) {

			 	if ($int_value<$min || $int_value>$max)
					return array(false, "Field $field_name selected, is not in the right range for the Radio.Band chosen.");
				return array(true);

			 } elseif (isset($radio_c0_val[2])) {
				$min2 = $radio_c0_val[2];
				$max2 = $radio_c0_val[3];
				if (($int_value>=$min && $int_value<=$max) || ($int_value>=$min2 && $int_value<=$max2))
					return array(true);
				return array(false, "Field $field_name selected, is not in the right range for the Radio.Band chosen.");
			 }

		}
	}

	return array(false, "The given value for $field_name is not a permitted one.");
}

/* 
 * Test if Radio.PowerManager.MinAttenDB is less or equal to Radio.PowerManager.MaxAttenDB.
 *
 * $field_value = getparam("Radio.PowerManager.MinAttenD");
 * $restricted_value = getparam("Radio.PowerManager.MaxAttenDB");
 */
function check_radio_powermanager($field_name, $field_value, $restricted_value)
{
	if (!is_valid_number($field_value))
		return array(false, "Field $field_name is not a valid number: $field_value.");

	if ((int)$field_value > $restricted_value)
		return array(false, "Radio.PowerManager.MinAttenDB, must be less or equal to Radio.PowerManager.MaxAttenDB");
	
	return array(true);
}

/*
 * Generic test field function
 * Test field to be in an interval [min,max] or
 * compare the field against a given regex
 */
function check_field_validity($field_name, $field_value, $min=false, $max=false, $regex=false)
{
	if ($min !== false && $max !== false)  {
		if (!is_valid_number($field_value))		
			return array(false, "Field $field_name is not a valid number: $field_value.");
		if ((int)$field_value<$min || (int)$field_value>$max) 
			return array(false, "Field $field_name is not valid. It has to be smaller then $max and greater then $min.");
	} 

	if ($regex) {
		if (!filter_var($field_value, FILTER_VALIDATE_REGEXP, array("options"=>array("regexp"=>"/".$regex."/"))))
			return array(false, "Field $field_name is not valid.");
	}

	return array(true);
}

/* validate an IP address*/
function check_valid_ipaddress($field_name, $field_value)
{
	if (!filter_var($field_value, FILTER_VALIDATE_IP)) 
		  return array(false, "Field $field_name is not valid. $field_value is not a valid IP address!");
	
	return array(true);
}

/* Validate a space-separated list of the DNS servers, in IP dotted notation, eg: 1.2.3.4 5.6.7.8. */
function check_valid_dns($field_name, $field_value)
{
	//this field can be empty
	if (!$field_value)
		return array(true);

	//validate DNS that are separed by space
	$dns_addresses = explode(" ", $field_value);
	$total = count($dns_addresses);
	for ($i=0; $i<$total; $i++)
		$res[] = check_valid_ipaddress($field_name, $dns_addresses[$i]);

	for ($i=0; $i<count($res); $i++) {
		if (!$res[$i][0])
			return array(false, $res[$i][1]);
	}

	return array(true);
}

/*
 * VEA ([control]) field value depends on CellSelection.NECI ([gsm_advanced]) 
 * this fields are in different section 
 * if one of the interest params was written in config file 
 * validate the other in form
 * This has to be valid:
 * If VEA is selected, [gsm_advanced] CellSelection.NECI should be set to 1.
 */
function validate_neci_vea($field_name, $field_value)
{
	global $yate_conf_dir;

	$filename = $yate_conf_dir."ybts.conf";

	$file = new ConfFile($filename);
	if ($field_name == "VEA" && ( $field_value == "off" || $field_value == "no" || $field_value == "0")) {
		//read the ybts.conf section [gsm_advanced] and check value for "CellSelection.NECI" param . If value set is not 1 then give error
		if (isset($file->structure["gsm_advanced"]["CellSelection.NECI"]) && $file->structure["gsm_advanced"]["CellSelection.NECI"] =="1")
			return array(true, "Field $field_name doesn't have a recommended value. It has to be checked because CellSelection.NECI param from GSM tab in GSM Advanced subsection was set to 1.");
	} 

	if ($field_name == "CellSelection.NECI" && $field_value != "1") {
		//read the ybts.conf section [ggsn] and check value for "VEA" param. If not yes, give error
		if (isset($file->structure["control"]["VEA"]) && $file->structure["control"]["VEA"] == "yes")
			return array(true, "Field $field_name doesn't have a recommended value. It has to be selected the 'New establisment causes are supported'  because VEA parameter from Control Tab was selected.");
	}

	return array(true);
}

/*
 * Timer.ImplicitDetach : should be at least 240 seconds greater than SGSN.Timer.RAUpdate. 
 * Interval allowed 2000:4000(10).
 *
 * $field_value => is the value of Timer.ImplicitDetach from form
 * $restricted_value => is the value of Timer.RAUpdate from form
 */
function check_timer_implicitdetach($field_name, $field_value, $restricted_value)
{
	for ($i=2000; $i<4000; $i++) {
		$interval_allowed[] = $i;
		$i = $i+9;
	}

	if (!is_valid_number($field_value))
                return array(false, "Field $field_name is not a valid number: $field_value.");

	//first test if $field_value is in the allowed interval
	if (!in_array($field_value, $interval_allowed))
		return array(false, "Field $field_name is not valid. The value must be in interval [2000,4000] and should be a factor of 10.");

	if ($field_value-$restricted_value < 240)
		return array(true, "Field $field_name doesn't have a recommended value. It should be at least 240 seconds greater than Timer.RAUpdate.");

	return array(true);
}

/*
 * [gprs_advanced] param: Timer.RAUpdate
 * Setting to 0 or >12000 deactivates entirely, i.e., sets the timer to effective infinity.
 * Note: to prevent GPRS Routing Area Updates you must set both this and GSM.Timer.T3212 to 0.
 * Interval allowed 0:11160(2). Defaults to 3240.
 */
function check_timer_raupdate($field_name, $field_value)
{
	global $yate_conf_dir;

	$filename = $yate_conf_dir."ybts.conf";

        $file = new ConfFile($filename);

	for ($i=0; $i<11160; $i++) {
		$interval_allowed[] = $i;
		$i = $i+1;
	}
	if (!is_valid_number($field_value))
                return array(false, "Field $field_name is not a valid number: $field_value.");

	if (!in_array($field_value, $interval_allowed) && $field_value<12000)
	        return array(false, "Field $field_name is not valid. The value must be in interval [0,11160] and should be a factor of 2 or greater than 12000.");

	if (isset($file->structure["gsm"]["Timer.T3212"]) && $file->structure["gsm"]["Timer.T3212"] == "0")
		return array(true, "To prevent GPRS Routing Area Updates it is recommended you set $field_name also to 0, setting field Timer.T3212 from GSM Tab set to 0 is not enough.");

	return array(true);
}

/*
 * [gprs_advanced] param:  Uplink.Persist
 * If non-zero, must be greater than GPRS.Uplink.KeepAlive.
 * This is broadcast in the beacon and it cannot be changed once BTS is started.
 * Allowed interval 0:6000(100).
 */
function check_uplink_persistent($field_name, $field_value, $restricted_value)
{
	for ($i=0; $i<6000; $i++) {
		$interval_allowed[] = $i;
		$i = $i+99;
	}

	if (!is_valid_number($field_value))
                return array(false, "Field $field_name is not a valid number: $field_value.");

	if (!in_array($field_value, $interval_allowed))
		 return array(false, "Field $field_name is not valid. The value must be in interval [0,6000] and should be a factor of 100.");

	if ($field_value != 0 || $field_value != "0") {
		if ((int)$field_value < (int)$restricted_value)
			return array(true, "Field $field_name doesn't have a recommended value. This value must be greater then Uplink.KeepAlive value.");
	}
	
	return array(true);
}

/*
 * [gprs_advanced] param: Downlink.Persist
 * If non-zero, must be greater than GPRS.Downlink.KeepAlive.
 */
function check_downlink_persistent($field_name, $field_value, $restricted_value)
{
	if (!is_valid_number($field_value))
                return array(false, "Field $field_name is not a valid number: $field_value.");

	$field_value = (int) $field_value;
	$restricted_value = (int)$restricted_value;
	
	if ($field_value < 0 || $field_value > 10000)
		return array(false, "Field $field_name is not valid. It has to be smaller than 10000.");
	if ($field_value != 0) {
                if ($field_value < $restricted_value)
			return array(true, "Field $field_name doesn't have a recommended value. This value must be greater then Downlink.KeepAlive value.");
	}

	return array(true);
}

/*
 * Test for [gprs_advanced] param: ChannelCodingControl.RSS.
 * This value should normally be GSM.Radio.RSSITarget + 10 dB.
 * Interval allowed -65:-15.
 */
function check_channelcodingcontrol_rssi($field_name, $field_value)
{
	global $yate_conf_dir;

	$filename = $yate_conf_dir."ybts.conf";

	if (!is_valid_number($field_value))
                return array(false, "Field $field_name is not a valid number: $field_value.");
	$field_value = (int) $field_value;

	for ($i=-65; $i<-15; $i++) 
		$interval_allowed[] = $i;

	if (!in_array($field_value, $interval_allowed))
		return array(false, "Field $field_name is not valid. The value must be in interval [-65,-15].");

	$file = new ConfFile($filename);
	if (isset($file->structure["gsm_advanced"]["Radio.RSSITarget"])) {
		$radio_RSSITarget = (int)$file->structure["gsm_advanced"]["Radio.RSSITarget"];
		if ($radio_RSSITarget+10 >= $field_value)
			return array(true, "Field $field_name doesn't have a recommended value. This value should normally be Radio.RSSITarget + 10 dB, value added from tab GSM, from GSM Advanced that has the value: $radio_RSSITarget.");
	}
	
	return array(true);
}

/*
 * validate field Radio.RSSITarget to be ChannelCodingControl.RSS-10 dB.
 */ 
function check_radio_rssitarget($field_name, $field_value)
{
	global $yate_conf_dir;


	$filename = $yate_conf_dir."ybts.conf";

	if (!is_valid_number($field_value))
                return array(false, "Field $field_name is not a valid number: $field_value.");
	$field_value = (int) $field_value;

	for ($i=-75; $i<-25; $i++)
		$interval_allowed[] = $i;

	if (!in_array($field_value, $interval_allowed))
		return array(false, "Field $field_name is not valid. The value must be in interval [-75,-25].");

	$file = new ConfFile($filename);
	if (isset($file->structure["gprs_advanced"]["ChannelCodingControl.RSSI"])) {
		$chancontrol_rss = (int)$file->structure["gprs_advanced"]["ChannelCodingControl.RSSI"];
		if ($chancontrol_rss -10 < $field_value)
			return array(true, "Field $field_name doesn't have a recommended value. This value should be the value of ChannelCodingControl.RSSI -10dB, parameter already added from submenu GPRS Advanced with the value: $chancontrol_rss.");
	}

	return array(true);
}

/*
 * validate t3260
 */
function check_t3260($field_name, $field_value)
{
	if (!is_valid_number($field_value))
		return array(false, "Field $field_name is not valid. Interval allowed: 5000..3600000.");
	$field_value = (int) $field_value;

	if ($field_value<5000 || $field_value>3600000)
		return array(false, "Field $field_name is not valid. Interval allowed: 5000..3600000.");

	return array(true);
}

/*
 * Validate reg_sip from [roaming] section in ybts.conf: ip:port or just ip
 */
function valid_reg_sip($field_name, $field_value)
{
	if (!strlen($field_value))
		return array(true);

	$expl = explode(":",$field_value);
	$ip = $expl[0];
	if (!filter_var($ip, FILTER_VALIDATE_IP))
		return array(false, "Field $field_name '$field_value' doesn't contain a valid IP address.");

	$port = (isset($expl[1])) ? $expl[1] : null;
	if ($port && !filter_var($port,FILTER_VALIDATE_INT))
		return array(false, "Field $field_name '$field_value' doesn't contain a valid port.");

	return array(true);
}

/*
 * Validate nodes_sip from [roaming] section in ybts.conf
 */
function valid_nodes_sip($field_name, $field_value)
{
	if (!strlen($field_value))
		return array(true);

	$field_value = htmlspecialchars_decode($field_value);
	$decoded = json_decode($field_value);

	if ($decoded===null)
		return array(false, "Field $field_name '$field_value' isn't a valid JSON object.");

	return array(true);
}

function check_rach_ac($field_name, $field_value)
{
	global $yate_conf_dir;

	if (!strlen($field_value))
		return array(true);

	if (substr($field_value,0,2)!="0x") {
		$res = check_field_validity($field_name, $field_value, 0, 65535);
		if (!$res[0])
			return $res;
		$emergency_disabled = $field_value & 0x0400;
	} else {
		$hex = substr($field_value,2);
		if (!ctype_xdigit($hex))
			return array(false, "Invalid hex value '$field_value' for $field_name.");
		if (strcasecmp($hex,"ffff")>0)
			return array(false, "Value '$field_value' for $field_name exceeds limit 0xffff.");
		$hex = base_convert($hex,16,10);
		$emergency_disabled = $hex & 0x0400;
	}
	if (!$emergency_disabled) {
		if (is_file($yate_conf_dir."/subscribers.conf")) {
			$conf = new ConfFile($yate_conf_dir."/subscribers.conf");
			if (!isset($conf->sections["general"]["gw_sos"]) || !strlen($conf->sections["general"]["gw_sos"]))
				return array(false, "You enabled emergency calls in RACH.CA. <font class='error'>DON'T MODIFY THIS UNLESS YOU ARE A REAL OPERATOR</font>. You might catch real emergency calls than won't ever be answered. If you really want to enable them, first set 'Gateway SOS' in Subscribers>Country code and SMSC.");
		}

		warning_note("You enabled emergency calls in RACH.CA. <font class='error'>DON'T MODIFY THIS UNLESS YOU ARE A REAL OPERATOR</font>. You might catch real emergency calls than won't ever be answered.");
	}

	return array(true);
}

?>
