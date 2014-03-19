<?php
/**
 * test_ybts_fields_validity.php
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


require_once "../../ybts/check_validity_fields_ybts.php";
require_once "../../ansql/base_classes.php";
require_once "../../ansql/lib_files.php";
require_once "../../defaults.php";
/**
 * Make automatic validity test for checking fields values from BTS Configuration TAB
 * The tests will be written into an array that will contain the folowing data:
 * test description 
 * test function to apply
 * test input data to give it as parameters to the function
 * test output data => the output expected to be received from the function
 */ 

$tests = array(
	array(
		"test_description" => "1. Test radio band. The function check_radio_band will be called. The input data will be wrong, meaning that the Rado.Band chosen will have a different Radio.C0. The output expected will be an error.",
		"input"=> array("check_radio_band","Radio.C0","512","850"),
		"output" =>array(false,"Field Radio.C0 selected, is not in the right range for the Radio.Band chosen.")
	),
	array(
		"test_description" => "2. Check radio powermanager to see if the min value is smaller than the maximum. Expected error output.",
		"input"=> array("check_radio_powermanager","Radio.PowerManager.MinAttenDB", "10", "2"),
		"output" =>array(false,"Radio.PowerManager.MinAttenDB, must be less or equal to Radio.PowerManager.MaxAttenDB")
	),
	array(
		"test_description" => "3. Check validity for a number to be in a specific range. Expecting error output.",
		"input"=> array("check_field_validity", "test name1","333",0,255),
		"output" =>array(false, "Field test name1 is not valid. It has to be smaller then 255 and greater then 0.")
	),
	array(
		"test_description" => "4. Check validity of a regex. Expecting error output.",
		"input"=> array("check_field_validity", "test name2","333",false,false,"^1{0,1}2{0,1}3{0,1}4{0,1}$"),
		"output" => array(false, "Field test name2 is not valid.")
	),
	array(
		"test_description" => "5. Check validity of an ipaddress. Expecting error output.",
		"input"=> array("check_valid_ipaddress", "test name3", "123456.23.3"),
		"output" => array(false, "Field test name3 is not valid. 123456.23.3 is not a valid IP address!")
	),
	array(
		"test_description" => "6. Check validity of DNS.  Expecting error output.",
		"input"=> array("check_valid_dns", "test name4", "23456.34 1.2.3.4"),
		"output" => array(false, "Field test name4 is not valid. 23456.34 is not a valid IP address!")
	),
	array(
		"test_description" => "7.  Check validity of VEA - NECI. This test depends on what is written in [gprs_advanced] in CellSelection.NECI (set in ybts.conf). So it could be error output or 'Test failed.'",
		"input"=> array("validate_neci_vea", "VEA", 0),
		"output" => array(false, "Field VEA is not valid. It has to be checked because CellSelection.NECI param from GSM tab in GSM Advanced subsection was set to 1.")
	),
	array(
		"test_description" => "9. Check Timer.ImplicitDetach invalid value. Expecting error output.",
		"input"=> array("check_timer_implicitdetach", 200,240),
		"output" => array(false, "Field 200 is not valid. The value must be in interval [2000,4000] and should be a factor of 10.")
	),
	array(
		"test_description" => "10. Check Timer.ImplicitDetach to be > 240+Timer.RAUpdate. expecting error.",
		"input"=> array("check_timer_implicitdetach","Timer.ImplicitDetach", "2000", 2040),
		"output" => array(false, "Field Timer.ImplicitDetach is not valid. It should be at least 240 seconds greater than Timer.RAUpdate.")
	),
	array(
		"test_description" => "11. Check Uplink.Persist to be greater than Uplink.KeepAlive. Expecting error.",
		"input"=> array("check_uplink_persistent","Uplink.Persist", "5100", 6800),
		"output" => array(false, "Field Uplink.Persist is not valid. This value must be greater then Uplink.KeepAlive value.")
	),
	array( 
		"test_description" => "12. Check Downlink.Persist if non-zero, must be greater than Downlink.KeepAlive.Expecting error.",
		"input"=> array("check_downlink_persistent", "3000",3400),
		"output" => array(false, "Field 3000 is not valid. This value must be greater then Downlink.KeepAlive value.")
	),
	array(
		"test_description" => "13. Check ChannelCodingControl.RSS. This value should normally be GSM.Radio.RSSITarget + 10 dB. Expecting error if in ybts.conf we have already set the Radio.RSSITarget -65. If not the 'output' will be: Test failed.",
		"input"=> array("check_channelcodingcontrol_rssi","ChannelCodingControl.RSS", "-63"),
		"output"=> array(false,"Field ChannelCodingControl.RSS is not valid. This value should normally be Radio.RSSITarget + 10 dB, value added from tab GSM, from GSM Advanced that has the value: -66.")
	)
);


foreach ($tests as $key => $test)
{
	$funct_name = $test["input"][0];
	$total = count($test["input"]);

	for ($i=0; $i<$total; $i++)  {
		${"param".$i} = $test["input"][$i];
		if ($i>0)
			$args[$i-1] = $test["input"][$i];
	}

	if (function_exists("call_user_func_array")){ 
		$res = call_user_func_array($funct_name,$args);
	} else {
		if (isset($param5))
			$res = $funct_name($param1, $param2, $param3, $param4, $param5);
		elseif (isset($param4))
			$res = $funct_name($param1, $param2, $param3, $param4);
		elseif (isset($param3))
			$res = $funct_name($param1, $param2, $param3);
		else
			$res = $funct_name($param1, $param2);
	 }
	print $test["test_description"]."\n";
	if (!$res[0]) {
		if ($res[1] == $test["output"][1])
			print "Error was found and is the one expected.<br/>\n";
		else
			print "Unexpected error found: ". $res[1]."<br/>\n";
	} else 
		print "Test failed.\n";
}

?>
