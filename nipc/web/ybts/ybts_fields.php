<?php 
/**
 * ybts_fields.php
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

require_once("create_radio_band_select_array.php");

/*
 * This returns the $fields array that consists of the SECTION and SUBSECTIONS for "BTS configuration" module (TAB).
 * The subsections are the actual sections (their name) in ybts.conf.
 * Each of this sections contains an array with each parameter name that can be configured.
 * And each parameter name has the following caracteristics:
 * - "display" : select, text, checkbox etc ( the specific type for FORM parameter)
 * -  array with selected value and all the values allowed
 * - "value" the default value of an input text or checkbox
 * - "comment" the text explaining the field 
 * - "validity" is an array with the name of the callback function that tests the field validity and arguments of the function
 *
 * There are 4 types of validity functions:
 * - a default function that will be applied when condition ($fields[$section][$subsection]["display"] === "select") is true, that will test if a value is in the array with the selected values
 * - a generic function called "check_field_validity" the will test if a field_value in in a specific interval OR if a value respects a regex
 * - a specific function name that will have the last parameter the value that is the parameter name field that restricts the value of the field checked
 * - a specific function name that has no parameters. This function will implement the field specific restrictions.
 */
function get_default_fields_ybts()
{
	global $private_version;

	$radio_c0 = prepare_gsm_field_radio_c0();
	$fields = array();
	$fields["GSM"] = array(
		"gsm" => array(
		"Radio.Band"=>	array( 
				array(array("Radio.Band_id"=>"850", "Radio.Band"=>"GSM850"), array("Radio.Band_id"=>"900", "Radio.Band"=>"EGSM900"),array("Radio.Band_id"=> "1800", "Radio.Band"=>"DCS1800"),array("Radio.Band_id"=>"1900", "Radio.Band"=>"PCS1900")),
				"display" => "select",
				"comment" => "The GSM operating band.
Valid values are 850 (GSM850), 900 (PGSM900), 1800 (DCS1800) and 1900 (PCS1900).
For non-multiband units, this value is dictated by the hardware and should not be changed."
			),
			"Radio.C0" => array( 
				$radio_c0,
				"display" => "select",
				"comment" => "The C0 ARFCN. Also the base ARFCN for a multi-ARFCN configuration.Valid values depend on the selected Radio.Band.
  GSM850: 128..251.
  EGSM900: 0..124 or 975..1023.
  DSC1800: 512..885.
  PCS1900: 512..810.
THERE IS NO DEFAULT, YOU MUST SET A VALUE HERE!",
				"validity" => array("check_radio_band", "Radio_Band")
			),
			"Identity.MCC" => array( 
				"display" => "text",
				"value" => "001",
				"comment" => "Mobile country code, must be three digits.
The value 001 is reserved for for test networks.
Defaults to 001 (Test Network)",
				"validity" => array("check_field_validity", false, false, "^[0-9]{3}$")
			),
			"Identity.MNC" => array( 
				"display" => "text",
				"value" => "01",
				"comment" => "Mobile network code, two or three digits.
The value 01 is usual for test networks with MCC 001.
Defaults to 01 (Test Network, only in association with Identity.MCC=001)",
				"validity" => array("check_field_validity", false, false, "^[0-9]{2,3}$")
			),
			"Identity.LAC" => array( 
				"display" => "text",
				"value" => "1000",
				"comment" => "Location area code, 16 bits, values 0xFFxx are reserved.
For multi-BTS networks, assign a unique LAC to each BTS unit.
Interval allowed: 0..65280.
Defaults to 1000 (arbitrary)",
				"validity" => array("check_field_validity", 0, 65280)
			),
			"Identity.CI" => array(
				"display" => "text",
				"value" => "10",
				"comment" => "Cell ID, 16 bits, should be unique.
Interval allowed: 0..65535.
Defaults to 10 (arbitrary)",
				"validity" => array("check_field_validity", 0, 65535)
			),

			"Identity.BSIC.BCC"=> array(
				array("selected"=>2,0,1,2,3,4,5,6,7),
				"display" => "select",
				"comment" => "GSM basestation color code; lower 3 bits of the BSIC.
BCC values in a multi-BTS network should be assigned so that BTS units with overlapping coverage do not share a BCC.
This value will also select the training sequence used for all slots on this unit.
Interval allowed: 0..7.
Defaults to 2 (arbitrary).",
			),
			"Identity.BSIC.NCC" => array(
				array("selected"=>0,0,1,2,3,4,5,6,7),
				"display"=> "select",
				"comment" => "GSM network color code; upper 3 bits of the BSIC.
Assigned by your national regulator, must be distinct from NCCs of other GSM operators in your area.
Interval allowed: 0..7. Defaults to 0."
		 	 ),
		        "Identity.ShortName" =>	array( 
				"display" => "text",
				"value" => "YateBTS",
				"regex" => "^[a-zA-Z0-9]+$",
				"comment" => "Network short name, displayed on some phones.
Optional but must be defined if you also want the network to send time-of-day.
Defaults to YateBTS.",
				"validity" => array("check_field_validity", false, false, "^[a-zA-Z0-9]+$")
			),
			"Radio.PowerManager.MaxAttenDB" => array( 
				array("selected" => 10, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80),
				"display" => "select",
				"value" => "10",
				"comment" => "Maximum transmitter attenuation level, in dB wrt full scale on the D/A output.
This sets the minimum power output level in the output power control loop.
Interval allowed: 0..80. Defaults to 10 (-10dB)"
			),
			"Radio.PowerManager.MinAttenDB" => array( 
				array("selected" => 0, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80),
				"display" => "select",
				"comment" => "Minimum transmitter attenuation level, in dB wrt full scale on the D/A output.
This sets the maximum power output level in the output power control loop.
Interval allowed: 0..80, must be less or equal to Radio.PowerManager.MaxAttenDB. 
Defaults to  0(dB) (maximum).",
				"validity" => array("check_radio_powermanager", "Radio_PowerManager_MaxAttenDB")
			)
		),
		"gsm_advanced" => array(
			"Channels.NumC1s" => array( 
				array("selected"=>7, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100),	
				"display" => "select",
				"comment" => "Number of Combination-I timeslots to configure.
The C-I slot carries a single full-rate TCH, used for speech calling.
Interval allowed: 0..100. Default value: 7(all timeslots of a single ARFCN)"
			),	
			"Channels.NumC7s" => array(
				array("selected"=>"0", 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100),
				"display" => "select",
				"comment" => "Number of Combination-VII timeslots to configure.
The C-VII slot carries 8 SDCCHs, useful to handle high registration loads, SMS or USSD.
If C0T0 is C-IV, you must have at least one C-VII also.
Interval allowed: 0..100.
Defaults to 0(timeslots)."
			),	
			"Channels.C1sFirst" => array( 
				"display" => "checkbox",
				"value" => "0",
				"comment" => "Allocate C-I slots first, starting at C0T1.
Defaults to no (allocate C-VII slots first)."
			),
			"Channels.SDCCHReserve" => array( 
				array("selected"=>"0", 0,1,2,3,4,5,6,7,8,9,10),
				"display" => "select",
				"comment" => "Number of SDCCHs to reserve for non-LUR operations.
This can be used to force LUR transactions into a lower priority.
Interval allowed: 0..10.
Defaults to 0(SDCCHs)",
				"validity" => array("check_field_validity",0,10)
			),	
			"CCCH.AGCH.QMax" => array( 
				array("selected"=>5, 3,4,5,6,7,8),
				"display" => "select",
				"comment" => "Maximum number of access grants to be queued for transmission on AGCH before declaring congestion.
Interval allowed: 3..8.
Defaults to 5",
				"validity" => array("check_field_validity",3,8)
			),	
			 "CCCH.CCCH-CONF" => array( 
				 array("selected"=> "1",array("CCCH.CCCH-CONF_id"=> "1", "CCCH.CCCH-CONF"=> "C-V beacon"), array("CCCH.CCCH-CONF_id"=> "2", "CCCH.CCCH-CONF"=> "C-IV beacon")),
				"display" => "select",
				"value" => "1",
				"comment" => "CCCH configuration type.
Values allowed: 1 (C-V beacon) or 2 (C-IV beacon).
Defaults to 1."
			),
			"CellOptions.RADIO-LINK-TIMEOUT" => array( 
				array("selected"=>15, 10,11,12,13,14,15,16,17,18,19,20),
				"display" => "select",
				"comment" => "Seconds before declaring a physical link dead.
Interval allowed: 10..20.
Defaults to 15",
				"validity" => array("check_field_validity",10,20)
			),	
			"CellSelection.CELL-RESELECT-HYSTERESIS" => array( 
				array("selected"=>"3", array("CellSelection.CELL-RESELECT-HYSTERESIS_id"=>"0","CellSelection.CELL-RESELECT-HYSTERESIS"=>"0dB"),array("CellSelection.CELL-RESELECT-HYSTERESIS_id"=>"1","CellSelection.CELL-RESELECT-HYSTERESIS"=> "2dB"), array("CellSelection.CELL-RESELECT-HYSTERESIS_id"=>"2","CellSelection.CELL-RESELECT-HYSTERESIS"=> "4dB"),array("CellSelection.CELL-RESELECT-HYSTERESIS_id"=>"3","CellSelection.CELL-RESELECT-HYSTERESIS"=> "6dB"),array("CellSelection.CELL-RESELECT-HYSTERESIS_id"=>"4","CellSelection.CELL-RESELECT-HYSTERESIS"=> "8dB"),array("CellSelection.CELL-RESELECT-HYSTERESIS_id"=>"5","CellSelection.CELL-RESELECT-HYSTERESIS"=> "10dB"),array("CellSelection.CELL-RESELECT-HYSTERESIS_id"=>"6","CellSelection.CELL-RESELECT-HYSTERESIS"=>"12dB"),array("CellSelection.CELL-RESELECT-HYSTERESIS_id"=>"7","CellSelection.CELL-RESELECT-HYSTERESIS"=>"14dB" )),
				"display"=>"select",
				"comment" => "Cell Reselection Hysteresis.
See GSM 04.08 10.5.2.4, Table 10.5.23 for encoding, Hysteresis is 2N dB.
Interval allowed: 0..7 (corresponds to 0..14 dB).
Defaults to 3"
			 ),	
			"CellSelection.MS-TXPWR-MAX-CCH" => array( 
				array("selected"=>0, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31),
				"display" => "select",
				"comment" => "Cell selection parameters.
See GSM 04.08 10.5.2.4.,Table 10.5.23 for encoding, Hysteresis is 2N dB.
Interval allowed: 0..7 (corresponds to 0..14 dB).
Defaults to 0."
			),
			"CellSelection.NCCsPermitted" => array( 
				"display" => "text",
				"value" => "-1",
				"comment" => "An 8-bit mask of allowed NCCs.
Unless you are coordinating with another carrier use -1 which allows to select your own NCC.
Valid values: 0..255 or -1. 
Defaults to -1 (allow your own NCC only)",
				"validity" => array("check_field_validity",-1,255)
			),	
			"CellSelection.NECI" => array( 
				array("selected" =>"1", array("CellSelection.NECI_id"=>"0", "CellSelection.NECI"=>"New establishment causes are not supported"),array("CellSelection.NECI_id"=>"1","CellSelection.NECI"=>"New establishment causes are supported")),
				"display" => "select",
				"comment" => "New Establishment Causes support.
See GSM 04.08 10.5.2.4, Table 10.5.23 and 04.08 9.1.8, Table 9.9.
This must be set to 1 if you want to support Very Early Assignment
Check VEA description for details from Control tab.
Valid values: 0..1
Defaults to 1",
				"validity" => array("validate_neci_vea")
			),	
			"CellSelection.RXLEV-ACCESS-MIN" => array( 
				array("selected"=>0, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63),
				"display" => "select",
				"comment" => "Cell selection parameters.
See GSM 04.08 10.5.2.4 for details.
Interval allowed: 0..63.
Defaults to 0."
			),
			"Cipher.CCHBER" => array( 
				array("selected"=>"0.0", "0.0", "0.1","0.2","0.3","0.4","0.5","0.6","0.7","0.8","0.9","1.0"),
				"display" => "select",
				"comment" => "Probability of a bit getting toggled in a control channel burst for cracking protection.
Interval allowed: 0.0 - 1.0.
Defauls to 0"
			),	
			"Cipher.Encrypt" => array( 
				"display" => "checkbox",
				"value" => "0",
				"comment" => "Encrypt traffic between phone and BTS.
Defaults to no."
			),
			"Cipher.RandomNeighbor" => array( 
				array("selected"=>"0.0", "0.0", "0.1","0.2","0.3","0.4","0.5","0.6","0.7","0.8","0.9","1.0"),
				"display" => "select",
				"comment" => "Probability of a random neighbor being added to SI5 for cracking protection.
Interval allowed: 0.0 - 1.0.
Defauls to 0."
			),
		 	"Cipher.ScrambleFiller" => array( 
				"display" => "checkbox",
				"value" => "0",
				"comment" => "Scramble filler in layer 2 for cracking protection. 
Defaults to no."
			),	
			"Control.GPRSMaxIgnore" => array( 
				array("selected"=>5, 3,4,5,6,7,8),
				"display" => "select",
				"comment" => "Ignore GPRS messages on GSM control channels.
Value is number of consecutive messages to ignore.
Interval allowed: 3..8.
Defaults to 5"
			),
			"Handover.InitialHoldoff" => array(
				array("selected"=>5000, 2000,3000,4000,5000,6000,7000),
				"display" => "select",
				"comment" => "Handover determination holdoff time after channel establishment, allows the MS RSSI value to stabilize. 2000 to 7000 ms, default 5000."
			),
			"Handover.RepeatHoldoff" => array(
				array("selected"=>3000, 750,1000,2000,3000,4000,5000,6000,7000),
				"display" => "select",
				"comment" => "Handover determination holdoff time after a previous attempt. 750 to 7000 ms, default 3000."
			),
			"Handover.LocalRSSIMin" => array(
				array("selected"=>-80, -100,-99,-98,-97,-96,-95,-94,-93,-92,-91,-90,-89,-88,-87,-86,-85,-84,-83,-82,-81,-80,-79,-78,-77,-76,-75,-74,-73,-72,-71,-70,-69,-68,-67,-66,-65,-64,-63,-62,-61,-60,-59,-58,-57,-56,-55,-54,-53,-52,-51,-50),
				"display" => "select",
				"comment" => "Do not handover if downlink RSSI is above this level (in dBm), regardless of power difference.
Allowed values -100:-50."
			),	
			"Handover.ThresholdDelta" => array(
			       array("selected" => 10, 5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20),	
				"display" => "select",
				"comment" => "A neighbor downlink signal must be this much stronger (in dB) than this downlink signal for handover to occur.
Allowed values 5:20.",
			),
			"MS.Power.Damping" =>  array(
			       array("selected" => 50, 25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75),	
				"display" => "select",
				"comment" => "Damping value for MS power control loop.
Interval allowed: 25..75. Defaults to 50."
			),
			"MS.Power.Max" => array( 
				array("selected" => 33, 5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100),
				"display" => "select",
				"comment" => "Maximum commanded MS power level in dBm.
Interval allowed: 5..100
Defaults to 33"
			),
			"MS.Power.Min" => array( 
				array("selected" => 5, 5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100),
				"display" => "select",
				"comment" => "Minimum commanded MS power level in dBm.
Interval allowed: 5..100
Defaults to 5"
			),
			"MS.TA.Damping" => array( 
				array("selected" => 50, 25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75),
				"display" => "select",
				"comment" => "Damping value for timing advance control loop. 
Interval allowed: 25..75
Defaults to 50"
			),
			"MS.TA.Max" => array( 
				array("selected" =>62, 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62),
				"display" => "select",
				"comment" => "Maximum allowed timing advance in symbol periods.
One symbol period of round-trip delay is about 0.55 km of distance.
RACH bursts with delays greater than this are ignored.
Can be used to limit service range.
Interval allowed: 1..62.
Defaults to 62 (maximum range)"
			),
			"MaxSpeechLatency" => array( 
				array("selected" => 2, 1,2,3,4,5),
				"display" => "select",
				"comment" => "Maximum allowed speech buffering latency.
It is expressed in 20ms frames.
Frames will be dropped if the jitter is larger than this delay.
Interval allowed: 1..5.
Defaults to 2."
			),
			"Neighbors.NumToSend" => array(
			       array("selected" => 8, 1,2,3,4,5,6,7,8,9,10),
				"display" => "select",
				"comment" => "Maximum number of neighbors to send to handset in a neighbor list in BCCH.
Interval allowed:1..10.
Defaults to 8."
			),
			"Ny1" => array( 
				array("selected" => 5, 1,2,3,4,5,6,7,8,9,10),
				"display" => "select",
				"comment" => "Maximum number of repeats of the Physical Information Message during handover procedure, GSM 04.08 11.1.3.
Interval allowed: 1..10
Defaults to 5"
			),
			"RACH.AC" => array(
				"value" => "0x0400",
				"display" => "text",
				"comment" => "Integer: Access Class flags.
This is the raw parameter sent on BCCH, see GSM 04.08 10.5.2.29 for encoding.
DO NOT ALTER THIS UNLESS YOU ARE A REAL OPERATOR!
Interval allowed: 0..65535 (16 bits, 0x0000..0xffff).
Defaults to 0x0400 (No emergency services access).",
				"validity" => array("check_rach_ac")
			),
			"RACH.MaxRetrans" => array( 
				array("selected" => "1", array("RACH.MaxRetrans_id"=>"0","RACH.MaxRetrans"=> "1 retransmission"),array("RACH.MaxRetrans_id"=>"1","RACH.MaxRetrans"=>"2 retransmission"),array("RACH.MaxRetrans_id"=>"2","RACH.MaxRetrans"=> "4 retransmission"),array("RACH.MaxRetrans_id"=>"3","RACH.MaxRetrans"=> "7 retransmission")),
				"display" => "select",
				"comment" => "Maximum RACH retransmission attempts.
This is the raw parameter sent on BCCH, see GSM 04.08 10.5.2.29 for encoding
Interval allowed: 0..3 (encodes to 1..7 retransmissions)
Defaults to 1 (2 retransmissions)"
			),	
			"RACH.TxInteger" => array( 
				array("selected" =>"14", array("RACH.TxInteger_id"=>"0","RACH.TxInteger"=> "3 slots"), array("RACH.TxInteger_id"=>"1","RACH.TxInteger"=> "4 slots"), array("RACH.TxInteger_id"=>"2","RACH.TxInteger"=>"5 slots"), array("RACH.TxInteger_id"=>"2","RACH.TxInteger"=>"5 slots"), array("RACH.TxInteger_id"=>"6","RACH.TxInteger"=>"9 slots"), array("RACH.TxInteger_id"=>"7","RACH.TxInteger"=> "10 slots"), array("RACH.TxInteger_id"=>"8","RACH.TxInteger"=>"11 slots"), array("RACH.TxInteger_id"=>"9","RACH.TxInteger"=>"12 slots"), array("RACH.TxInteger_id"=>"10","RACH.TxInteger"=>"14 slots"), array("RACH.TxInteger_id"=>"11","RACH.TxInteger"=>"16 slots"), array("RACH.TxInteger_id"=>"12","RACH.TxInteger"=>"20 slots"), array("RACH.TxInteger_id"=>"13","RACH.TxInteger"=>"25 slots"), array("RACH.TxInteger_id"=>"14","RACH.TxInteger"=>"32 slots"),array("RACH.TxInteger_id"=>"15","RACH.TxInteger"=>"50 slots")),
				"display" => "select",
				"comment" => "Parameter to spread RACH busts over time. 
This is the raw parameter sent on BCCH, see GSM 04.08 10.5.2.29 for encoding.
Interval allowed: 0..15 (encodes to 3..50 slots)
Defaults to 14 (32 slots)" 
			),
			"Radio.ARFCNs" => array( 
				array("selected" =>1, 1,2,3,4,5,6,7,8,9,10),
				"display" => "select",
				"comment" => "The number of ARFCNs to use.
The ARFCN set will be C0, C0+2, C0+4, ...Interval allowed: 1..10
Defaults to 1 (single ARFCN unit)"	
			),
			"Radio.MaxExpectedDelaySpread" => array( 
				array("selected" => 2, 1,2,3,4),
				"display" => "select",
				"comment" => "Expected worst-case delay spread
Expressed in symbol periods, roughly 3.7 us or 1.1 km per unit.
This parameter is dependent on the terrain type in the installation area.
Typical values: 1 -> open terrain, small coverage, 4 -> large coverage area.
This parameter has a large effect on computational requirements of the software radio.
Interval allowed: 1..4.
Defaults to 2."
			),
			"Radio.NeedBSIC" => array( 
				"display" => "checkbox",
				"value" => "0",
				"comment" => "Does the Radio type require the full BSIC.
Defaults to no."
			),
			"Radio.PowerManager.NumSamples" => array( 
				array("selected"=>10, 5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20), 
				"display" => "select",
				"comment" => "Number of samples averaged by the output power control loop
Interval allowed: 5..20
Defaults to 10"
			),
			"Radio.PowerManager.Period" => array( 
				"display" => "text",
				"value" => "6000",
				"comment" => "Power manager control loop master period, in milliseconds
Interval allowed: 4500..7500
Defaults to 6000",
				"validity" => array("check_field_validity",4500,7500)
			),
			 "Radio.PowerManager.SamplePeriod" => array( 
				"display" => "text",
				"value" => "2000",
				"comment" => "Sample period for the output power control loopm in milliseconds
Interval allowed: 1500..2500
Defaults to 2000",
				"validity" => array("check_field_validity",1500,2500)
			),
			"Radio.PowerManager.TargetT3122" => array( 
				"display" => "text",
				"value" => "5000",
				"comment" => "Target value for T3122, the random access hold-off timer, for the power control loop.
Interval allowed: 3750..6250.
Defaults to 5000",
				"validity" => array("check_field_validity",3750,6250)
			),
			"Radio.RSSITarget" => array( 
				array("selected"=>-50, -75,-74,-73,-72,-71,-70,-69,-68,-67,-66,-65,-64,-63,-62,-61,-60,-59,-58,-57,-56,-55,-54,-53,-52,-51,-50,-49,-48,-47,-46,-45,-44,-43,-42,-41,-40,-39,-38,-37,-36,-35,-34,-33,-32,-31,-30,-29,-28,-27,-26,-25),
				"display" => "select",
				"comment" => "Target uplink RSSI for MS power control loop
Expressed in dB wrt to receiver's A/D convertor full scale
Should be 6-10 dB above the noise floor (see the 'mbts noise' command)
Interval allowed: -75..-25
Defaults to -50",
				"validity" => array("check_radio_rssitarget")
			),	
			"Radio.RxGain" => array( 
				array("selected"=>"Factory calibrated", "Factory calibrated",0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75),
				"display" => "select",
				"comment" => "Receiver gain setting in dB
Ideal value is dictated by the hardware; 47 dB for RAD1, less for USRPs
Interval allowed: 0..75
Defaults to empty value. Radios provide their own calibrated default. To see current value run 'mbts trxfactory' after connecting to rmanager interface." 
			),
			"ShowCountry" => array( 
				"display" => "checkbox",
				"value" => "0",
				"comment" => "Tell the phone to show the country name based on MCC
Defaults to no"
			),
			"Timer.T3103" => array( 
				array("selected"=>5000, 2500,2600,2700,2800,2900,3000,3100,3200,3300,3400,3500,3600,3700,3800,3900,4000,4100,4200,4300,4400,4500,4600,4700,4800,4900,5000,5100,5200,5300,5400,5500,5600,5700,5800,5900,6000,6100,6200,6300,6400,6500,6600,6700,6800,6900,7000,7100,7200,7300,7400,7500),
				"display" => "select",
				"comment" => "Handover timeout in milliseconds, GSM 04.08 11.1.2
This is the timeout for a handset to sieze a channel during handover.
Allowed: 2500:7500(100)
Defaults to 5000"
			),	
			"Timer.T3105" => array( 
				array("selected"=>50, 25,30,35,40,45,50,55,60,65,70,75),
				"display" => "select",
				"comment" => "Milliseconds for handset to respond to physical information.GSM 04.08 11.1.2.
Interval allowed 25:75(5).
Defaults to 50"
			),
			"Timer.T3113" => array( 
				array("selected"=>10000, 5000,5500,6000,6500,7000,7500,8000,8500,9000,9500,10000,10500,11000,11500,12000,12500,13000,13500,14000,14500,15000 ),
				"display" => "select",
				"comment" => "Paging timer T3113 in milliseconds. 
This is the timeout for a handset to respond to a paging request.
This should usually be the same as SIP.Timer.B in your VoIP network.
Allowed interval 5000:15000(500).
Defaults to 10000"
			),
			"Timer.T3122Max" => array( 
				"display" => "text",
				"value" => 255000,
				"comment" => "Maximum allowed value for T3122, the RACH holdoff timer, in millseconds.
Allowed interval 127500:382500(1000).
It is not required to be a factor of 1000.
Defaults to 255000.",
				"validity" => array("check_field_validity",127500,382500)
			),
			"Timer.T3122Min" => array( 
				array("selected"=>2000, 1000,1100,1200,1300,1400,1500,1600,1700,1800,1900,2000,2100,2200,2300,2400,2500,2600,2700,2800,2900,3000),
				"display" => "select",
				"comment" => "Minimum allowed value for T3122, the RACH holdoff timer, in milliseconds.
Allowed interval 1000:3000(100).
Defaults to 2000"
			),
			"Timer.T3212" => array( 
				array("selected"=>24, 0,6,12,18,24,30,36,42,48,54,60,66,72,78,84,90,96,102,108,114,120,126,132,138,144,150,156,162,168,174,180,186,192,198,204,210,216,222,228,234,240,246,252,258,264,270,276,282,288,294,300,306,312,318,324,330,336,342,348,354,360,366,372,378,384,390,396,402,408,414,420,426,432,438,444,450,456,462,468,474,480,486,492,498,504,510,516,522,528,534,540,546,552,558,564,570,576,582,588,594,600,606,612,618,624,630,636,642,648,654,660,666,672,678,684,690,696,702,708,714,720,726,732,738,744,750,756,762,768,774,780,786,792,798,804,810,816,822,828,834,840,846,852,858,864,870,876,882,888,894,900,906,912,918,924,930,936,942,948,954,960,966,972,978,984,990,996,1002,1008,1014,1020,1026,1032,1038,1044,1050,1056,1062,1068,1074,1080,1086,1092,1098,1104,1110,1116,1122,1128,1134,1140,1146,1152,1158,1164,1170,1176,1182,1188,1194,1200,1206,1212,1218,1224,1230,1236,1242,1248,1254,1260,1266,1272,1278,1284,1290,1296,1302,1308,1314,1320,1326,1332,1338,1344,1350,1356,1362,1368,1374,1380,1386,1392,1398,1404,1410,1416,1422,1428,1434,1440,1446,1452,1458,1464,1470,1476,1482,1488,1494,1500,1506,1512,1518,1524,1530),
				"display" => "select",
				"comment" => "Registration timer T3212 period in minutes. Should be a factor of 6.
Set to 0 to disable periodic registration.
Should be smaller than SIP registration period.
Allowed interval 0:1530(6)
Defaults to 24."
			)

		)
	);
	$fields["GPRS"] = array(
		"gprs" =>array(
			"Enable" => array(
				"display" => "checkbox",
				"value" => "1",
				"comment" => "Advertise GPRS in C0T0 beacon and start service on demand.
See also Channels.* in [gprs_advanced] for how many channels are reserved.
Defauts to yes."
			),
			"RAC" => array(
				"display" => "text",
				"value" => "0",
				"comment" => "GPRS Routing Area Code, advertised in the C0T0 beacon.
Interval allowed: 0..255
Defaults to 0 (arbitrary)",
				"validity" => array("check_field_validity",0,255)
			),
			"RA_COLOUR" => array(
				array("selected"=>0, 0,1,2,3,4,5,6,7),
				"display" => "select",
				"comment" => "GPRS Routing Area Color as advertised in the C0T0 beacon
Interval allowed: 0..7
Defaults to 0 (arbitrary)"
			)
		),
		"gprs_advanced" => array(
			"Debug" => array(
				"param" => "Debug",
				"display" => "checkbox",
                                "value" => "0",
                                "comment" => "Toggle GPRS debugging. Defaults to no."
	                 ),
			 "MS.Power.Alpha" => array(
				 array("selected" => "10",array("MS.Power.Alpha_id"=>"1","MS.Power.Alpha"=>"0.1"),array("MS.Power.Alpha_id"=>"2","MS.Power.Alpha"=>"0.2"),array("MS.Power.Alpha_id"=>"3","MS.Power.Alpha"=>"0.3"),array("MS.Power.Alpha_id"=>"4","MS.Power.Alpha"=>"0.4"),array("MS.Power.Alpha_id"=>"5","MS.Power.Alpha"=>"0.5"),array("MS.Power.Alpha_id"=>"6","MS.Power.Alpha"=>"0.6"),array("MS.Power.Alpha_id"=>"7","MS.Power.Alpha"=>"0.7"),array("MS.Power.Alpha_id"=>"8","MS.Power.Alpha"=>"0.8"),array("MS.Power.Alpha_id"=>"9","MS.Power.Alpha"=>"0.9"),array("MS.Power.Alpha_id"=>"10","MS.Power.Alpha"=>"1.0")),
				"display" => "select",
				"comment" => "MS power control parameter, unitless, in steps of 0.1, so a parameter of 5 is an alpha value of 0.5.
Determines sensitivity of handset to variations in downlink RSSI.
Valid range is 0...10 for alpha values of 0.0 ... 1.0. See GSM 05.08 10.2.1."
			),
			"MS.Power.Gamma" => array(
				array("selected"=>"31", array("MS.Power.Gamma_id" => "0", "MS.Power.Gamma"=>"0 dB"),array("MS.Power.Gamma_id" =>"1", "MS.Power.Gamma"=>"2 dB"),array("MS.Power.Gamma_id" =>"2", "MS.Power.Gamma"=>"4 dB"),array("MS.Power.Gamma_id" =>"3", "MS.Power.Gamma"=>"6 dB"),array("MS.Power.Gamma_id" =>"4", "MS.Power.Gamma"=>"8 dB"),array("MS.Power.Gamma_id" =>"5", "MS.Power.Gamma"=>"10 dB"),array("MS.Power.Gamma_id" =>"6", "MS.Power.Gamma"=>"12 dB"),array("MS.Power.Gamma_id" =>"7", "MS.Power.Gamma"=>"14 dB"),array("MS.Power.Gamma_id" =>"8", "MS.Power.Gamma"=>"16 dB"),array("MS.Power.Gamma_id" => "9", "MS.Power.Gamma"=>"18 dB"),array("MS.Power.Gamma_id" =>"10", "MS.Power.Gamma"=>"20 dB"),array("MS.Power.Gamma_id" =>"11", "MS.Power.Gamma"=>"22 dB"),array("MS.Power.Gamma_id" =>"12", "MS.Power.Gamma"=>"24 dB"),array("MS.Power.Gamma_id" =>"13", "MS.Power.Gamma"=>"26 dB"),array("MS.Power.Gamma_id" =>"14", "MS.Power.Gamma"=>"28 dB"),array("MS.Power.Gamma_id" =>"15", "MS.Power.Gamma"=>"30 dB"),array("MS.Power.Gamma_id" =>"16", "MS.Power.Gamma"=>"32 dB"),array("MS.Power.Gamma_id" =>"17", "MS.Power.Gamma"=>"34 dB"),array("MS.Power.Gamma_id" =>"18", "MS.Power.Gamma"=>"36 dB"),array("MS.Power.Gamma_id" =>"19", "MS.Power.Gamma"=>"38 dB"),array("MS.Power.Gamma_id"=>"20", "MS.Power.Gamma"=>"40 dB"),array("MS.Power.Gamma_id"=>"21", "MS.Power.Gamma"=>"42 dB"),array("MS.Power.Gamma_id" =>"22", "MS.Power.Gamma"=>"44 dB"),array("MS.Power.Gamma_id" =>"23", "MS.Power.Gamma"=>"46 dB"),array("MS.Power.Gamma_id" =>"24", "MS.Power.Gamma"=>"48 dB"),array("MS.Power.Gamma_id" =>"25", "MS.Power.Gamma"=>"50 dB"),array("MS.Power.Gamma_id" => "26", "MS.Power.Gamma"=>"52 dB"),array("MS.Power.Gamma_id" =>"27", "MS.Power.Gamma"=>"54 dB"),array("MS.Power.Gamma_id" =>"28", "MS.Power.Gamma"=>"56 dB"),array("MS.Power.Gamma_id" =>"29", "MS.Power.Gamma"=>"58 dB"),array("MS.Power.Gamma_id" =>"30", "MS.Power.Gamma"=>"60 dB"),array("MS.Power.Gamma_id" =>"31", "MS.Power.Gamma"=>"62 dB")),
				"display" => "select",
				"comment" => "MS power control parameter, in 2 dB steps.
Determines baseline of handset uplink power relative to downlink RSSI
The optimum value will tend to be lower for BTS units with higher power output
Valid range is 0...31 for gamma values of 0...62 dB. See GSM 05.08 10.2.1."
	                 ),
			 "MS.Power.T_AVG_T" => array(
				 array("selected"=>15, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25),
				"display" => "select",
				"comment" => "MS power control parameter; see GSM 05.08 10.2.1.
Valid range is 0...25."
			),
			"MS.Power.T_AVG_W" => array(
				array("selected"=>15, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25),
				"display" => "select",
				"comment" => "MS power control parameter; see GSM 05.08 10.2.1.
Valid range is 0...25
Defaults to 15."
	                 ),
			 "Multislot.Max.Downlink" => array(
				 array("selected"=>3, 0,1,2,3,4,5,6,7,8,9,10),
				"display" => "select",
				"comment" => "Maximum number of channels used for a single MS in downlink
Valid range is 0...10
Defaults to 3."			
			),
			"Multislot.Max.Uplink" => array(
				array("selected"=>2, 0,1,2,3,4,5,6,7,8,9,10),
				"display" => "select",
				"comment" => "Maximum number of channels used for a single MS in uplink
Valid range is 0...10. Defaults to 2."
	                 ),
			 "Codecs.Downlink" => array(
				 "display" => "text",
				"value" => "14",
				"comment" => "List of allowed GPRS downlink codecs 1..4 for CS-1..CS-4.
Currently, only 1 and 4 are supported e.g. 14."
				
			),
			"Codecs.Uplink" => array(
				"display" => "text",
                                "value" => "14",
				"comment" => "List of allowed GPRS uplink codecs 1..4 for CS-1..CS-4.
Currently, only 1 and 4 are supported e.g. 14."
	                 ),
			 "Uplink.KeepAlive" => array(
				 array("selected"=>300, 200,300,400,500,600,700,800,900,1000,1100,1200,1300,1400,1500,1600,1700,1800,1900,2000,2100,2200,2300,2400,2500,2600,2700,2800,2900,3000,3100,3200,3300,3400,3500,3600,3700,3800,3900,4000,4100,4200,4300,4400,4500,4600,4700,4800,4900,5000),
				"display" => "select",
				"value" => "300",
				"comment" => "How often to send keep-alive messages for persistent TBFs in milliseconds; must be long enough to avoid simultaneous in-flight duplicates, and short enough that MS gets one every 5 seconds.
Allowed interval 200:5000(100)
Defauts to 300"
			),
			"Uplink.Persist" => array(
				array("selected"=>4000,0,100,200,300,400,500,600,700,800,900,1000,1100,1200,1300,1400,1500,1600,1700,1800,1900,2000,2100,2200,2300,2400,2500,2600,2700,2800,2900,3000,3100,3200,3300,3400,3500,3600,3700,3800,3900,4000,4100,4200,4300,4400,4500,4600,4700,4800,4900,5000,5100,5200,5300,5400,5500,5600,5700,5800,5900,6000),
				"display" => "select",
				"comment" => "After completion, uplink TBFs are held open for this time in milliseconds
If non-zero, must be greater than GPRS.Uplink.KeepAlive
This is broadcast in the beacon and it cannot be changed once BTS is started
Allowed interval 0:6000(100)
Defauts to 4000.",
				"validity" => array("check_uplink_persistent", "Uplink_KeepAlive")
	                 ),
			 "TBF.Downlink.Poll1" => array(
				 array("selected"=>10, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15),
				"display" => "select",
				"comment" => "When the first poll is sent for a downlink tbf, measured in blocks sent.
Allowed values 5:15
Defauts to 10"
			),
			"TBF.EST" => array(
				"display" => "checkbox",
                                "value" => "1",
				"comment" => "Allow MS to request another uplink assignment at end up of uplink TBF. See GSM 4.60 9.2.3.4
Defauts to yes."
	                 ),
			"TBF.Expire" => array(
				"display" => "text",
				"value" => "30000",
				"comment" => "How long to try before giving up on a TBF
Allowed interval 20000:40000
Defauts to 30000",
				"validity" => array("check_field_validity",20000,40000)
			),
			"TBF.KeepExpiredCount" => array(
				array("selected"=>20, 15,16,17,18,19,20,21,22,23,24,25),
				"display" => "select",
				"comment" => "How many expired TBF structs to retain; they can be viewed with gprs list tbf -x
Allowed interval 15:25"
	                 ),
			 "TBF.Retry" => array(
				 array("selected"=>"1", array("TBF.Retry_id"=>"0", "TBF.Retry"=>"Do Not Retry"),array("TBF.Retry_id"=>"1", "TBF.Retry"=>"Codec 1"),array("TBF.Retry_id"=>"2", "TBF.Retry"=>"Codec 2"),array("TBF.Retry_id"=>"3","TBF.Retry"=>"Codec 3"),array("TBF.Retry_id"=>"4","TBF.Retry"=>"Codec 4")),
				"display" => "select",
				"comment" => "If 0, no tbf retry, otherwise if a tbf fails it will be retried with this codec, numbered 1..4."
			),
			"advanceblocks"=> array(
				array("selected"=>10,5,6,7,8,9,10,11,12,13,14,15),
				"display" => "select",
				"comment" => "Number of advance blocks to use in the CCCH reservation.
Allowed interval 5:15
Defaults to 10."
	                 ),
			"CellOptions.T3168Code" => array(
				array("selected"=>"5", array("CellOptions.T3168Code_id"=>"0","CellOptions.T3168Code"=>"500ms"),array("CellOptions.T3168Code_id"=>"1","CellOptions.T3168Code"=>"1000ms"),array("CellOptions.T3168Code_id"=>"2","CellOptions.T3168Code"=>"1500ms"),array("CellOptions.T3168Code_id"=>"3","CellOptions.T3168Code"=>"2000ms"),array("CellOptions.T3168Code_id"=>"4","CellOptions.T3168Code"=>"2500ms"),array("CellOptions.T3168Code_id"=>"5","CellOptions.T3168Code"=>"3000ms"),array("CellOptions.T3168Code_id"=>"6", "CellOptions.T3168Code"=>"3500ms"),array("CellOptions.T3168Code_id"=>"7","CellOptions.T3168Code"=>"4000ms")),
				"display" => "select",
				"comment" => "Timer 3168 in the MS controls the wait time after sending a Packet Resource Request to initiate a TBF before giving up or reattempting a Packet Access Procedure, which may imply sending a new RACH.
This code is broadcast to the MS in the C0T0 beacon in the GPRS Cell Options IE. See GSM 04.60 12.24.
Range 0..7 to represent 0.5sec to 4sec in 0.5sec steps."
			),
			"CellOptions.T3192Code" => array(
				array("selected"=>"0", array("CellOptions.T3192Code_id"=>"0", "CellOptions.T3192Code"=>"500ms"), array("CellOptions.T3192Code_id"=>"1", "CellOptions.T3192Code"=>"1000ms"),array("CellOptions.T3192Code_id"=>"2", "CellOptions.T3192Code"=>"1500ms"),array("CellOptions.T3192Code_id"=>"3", "CellOptions.T3192Code"=>"0ms"),array("CellOptions.T3192Code_id"=>"4","CellOptions.T3192Code"=>"80ms"),array("CellOptions.T3192Code_id"=>"5", "CellOptions.T3192Code"=>"120ms"), array("CellOptions.T3192Code_id"=>"6", "CellOptions.T3192Code"=>"160ms"), array("CellOptions.T3192Code_id"=>"7","CellOptions.T3192Code"=>"200ms")),
				"display" => "select",
				"comment" => "Timer 3192 in the MS specifies the time MS continues to listen on PDCH after all downlink TBFs are finished, and is used to reduce unnecessary RACH traffic.
This code is broadcast to the MS in the C0T0 beacon in the GPRS Cell Options IE.
The value must be one of the codes described in GSM 04.60 12.24.
Value 0 implies 500msec
2 implies 1500msec
3 imples 0msec."			
			),
			"ChannelCodingControl.RSSI" => array(
				array("selected"=>-40,-65,-64,-63,-62,-61,-60,-59,-58,-57,-56,-55,-54,-53,-52,-51,-50,-49,-48,-47,-46,-45,-44,-43,-42,-41,-40,-39,-38,-37,-36,-35,-34,-33,-32,-31,-30,-29,-28,-27,-26,-25,-24,-23,-22,-21,-20,-19,-18,-17,-16,-15),
				"display" => "select",
				"comment" => "If the initial unlink signal strength is less than this amount in DB GPRS uses a lower bandwidth but more robust encoding CS-1.
This value should normally be GSM.Radio.RSSITarget + 10 dB.
Interval allowed -65:-15
Defaults to -40.",
				"validity" => array("check_channelcodingcontrol_rssi")
	                 ),
			"Channels.Congestion.Threshold" => array(
				array("selected"=>200, 100,105,110,115,120,125,130,135,140,145,150,155,160,165,170,175,180,185,190,195,200,205,210,215,220,225,230,235,240,245,250,255,260,265,270,275,280,285,290,295,300),
				"display" => "select",
				"comment" => "The GPRS channel is considered congested if the desired bandwidth exceeds available bandwidth by this amount, specified in percent
Interval allowed 100:300(5)
Defaults to 200."
			),
			"Channels.Congestion.Timer" => array(
				array("selected"=>60,30,35,40,45,50,55,60,65,70,75,80,85,90),
				"display" => "select",
				"comment" => "How long in seconds GPRS congestion exceeds the Congestion
Threshold before we attempt to allocate another channel for GPRS
Interval allowed 30:90(5)
Defaults to 60."
	                 ),
			"Channels.Min.C0" => array(
				array("selected"=>3,0,1,2,3,4,5,6,7),
				"display" => "select",
				"comment" => "Minimum number of channels allocated for GPRS service on ARFCN C0.
Interval allowed 0:7.
Defaults to 3."
			),
			"Channels.Min.CN" => array(
				array("selected"=>0, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100),
				"display" => "select",
				"comment" => "Minimum number of channels allocated for GPRS service on ARFCNs other than C0
Interval allowed 0:100
Defaults to 0."
	                 ),
			 "Channels.Max" => array(
				 array("selected"=>4, 0,1,2,3,4,5,6,7,8,9,10),
				"display" => "select",
				"comment" => "Maximum number of channels allocated for GPRS service.
Interval allowed 0:10.
Defaults to 4."
	                 ),
			 "Counters.Assign" => array(
				 array("selected"=>10, 5,6,7,8,9,10,11,12,13,14,15),
				"display" => "select",
				"comment" => "Maximum number of assign messages sent.
Interval allowed 5:15
Defaults to 10"
			),
			"Counters.N3101" => array(
				array("selected"=>20, 8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32),
				"display" => "select",
				"comment" => "Counts unused USF responses to detect nonresponsive MS. Should be > 8. See GSM04.60 sec 13.
Interval allowed 8:32
Defaults to 20"
			),
			"Counters.N3103" => array(
				array("selected"=>8, 4,5,6,7,8,9,10,11,12),
				"display" => "select",
				"comment" => "Counts ACK/NACK attempts to detect nonresponsive MS. See GSM04.60 sec 13
Interval allowed 4:12
Defaults to 8."
			),
			"Counters.N3105" => array(
				array("selected"=>12, 6,7,8,9,10,11,12,13,14,15,16,17,18),
				"display" => "select",
				"comment" => "Counts unused RRBP responses to detect nonresponsive MS. See GSM04.60 sec 13
Interval allowed 6:18
Defaults to 12"
			),
			"Counters.Reassign" => array(
				array("selected"=>6, 3,4,5,6,7,8,9),
				"display" => "select",
				"comment" => "Maximum number of reassign messages sent
Interval allowed 3:9
Defaults to 6"
			),
			"Counters.TbfRelease" => array(
				array("selected"=>5, 3,4,5,6,7,8),
				"display" => "select",
				"comment" => "Maximum number of TBF release messages sent
Interval allowed 3:8
Defaults to 5"
	                 ),
			"Downlink.KeepAlive" => array(
				array("selected"=>300, 200,300,400,500,600,700,800,900,1000,1100,1200,1300,1400,1500,1600,1700,1800,1900,2000,2100,2200,2300,2400,2500,2600,2700,2800,2900,3000,3100,3200,3300,3400,3500,3600,3700,3800,3900,4000,4100,4200,4300,4400,4500,4600,4700,4800,4900,5000),
				"display" => "select",
				"comment" => "How often to send keep-alive messages for persistent TBFs in milliseconds
must be long enough to avoid simultaneous in-flight duplicates, and short enough that MS gets one every 5 seconds.
GSM 5.08 10.2.2 indicates MS must get a block every 360ms.
Interval allowed 200:5000(100)
Defaults to 300"
			),
			"Downlink.Persist" => array(
				"display" => "text",
                                "value" => "0",
				"comment" => "After completion, downlink TBFs are held open for this time in milliseconds
If non-zero, must be greater than GPRS.Downlink.KeepAlive.",
				"validity" => array("check_downlink_persistent", "Downlink_KeepAlive")
	                 ),
			"LocalTLLI.Enable" => array(
				"display" => "checkbox",
				"value" => "1",
				"comment" => "Enable recognition of local TLLI."
			),
			"MS.KeepExpiredCount" => array(
				array("selected"=>20, 10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30),
				"display" => "select",
				"comment" => "How many expired MS structs to retain; they can be viewed with gprs list ms -x.
Interval allowed 10:30
Defaults to 20"
	                 ),
			 "NC.NetworkControlOrder" => array(
				 array("selected"=>2, 0,1,2,3),
				"display" => "select",
				"comment" => "Controls measurement reports and cell reselection mode (MS autonomous or under network control); should not be changed
Interval allowed 0:3
Defaults to 2."
			),
			"NMO" => array(
				array("selected"=>"2",array("NMO_id"=>"1", "NMO"=>"Mode I"),array("NMO_id"=>"2","NMO"=>"Mode II"),array("NMO_id"=>"3","NMO"=>"Mode III")),
				"display" => "select",
				"comment" => "Network Mode of Operation. See GSM 03.60 Section 6.3.3.1 and 24.008 4.7.1.6.
Allowed values are 1, 2, 3 for modes I, II, III.
Mode II (2) is recommended.
Mode I implies combined routing updating procedures."
			),
			"PRIORITY-ACCESS-THR" => array(
				array("selected"=>"6",array( "PRIORITY-ACCESS-THR_id"=>"0","PRIORITY-ACCESS-THR"=>"Packet access not allowed in the cell"),array("PRIORITY-ACCESS-THR_id"=>"3","PRIORITY-ACCESS-THR" =>"Packet access allowed for priority level 1"),array("PRIORITY-ACCESS-THR_id"=>"4","PRIORITY-ACCESS-THR" =>"Packet access allowed for priority level 1 to 2"),array("PRIORITY-ACCESS-THR_id"=>"5","PRIORITY-ACCESS-THR" =>"Packet access allowed for priority level 1 to 3"),array("PRIORITY-ACCESS-THR_id"=>"6","PRIORITY-ACCESS-THR" =>"Packet access allowed for priority level 1 to 4")),
				"display" => "select",
				"comment" => "Code contols GPRS packet access priorities allowed. See GSM04.08 table  10.5.76.
0=Packet access not allowed in the cell,
3=Packet access allowed for priority level 1,
4=Packet access allowed for priority level 1 to 2,
5=Packet access allowed for priority level 1 to 3,
6=Packet access allowed for priority level 1 to 4"
			
	                 ),
			 "RRBP.Min" => array(
				 array("selected"=>0,0,1,2,3),
				"display" => "select",
				"comment" => "Minimum value for RRBP reservations, range 0..3. Should normally be 0. A non-zero value gives the MS more time to respond to the RRBP request."
			),
			"Reassign.Enable" => array(
				"display" => "checkbox",
                                "value" => "1",
                                "comment" => "Enable TBF Reassignment."
	                 ),
			"SendIdleFrames" => array(
				"display" => "checkbox",
				"value" => "0",
				"comment" => "Should be 0 for current transceiver or 1 for deprecated version of transceiver."
			),
			"SGSN.port" => array(
				"display" => "text",
                                "value" => "1920",
				"comment" => "Port number of the SGSN required for GPRS service.
This must match the port specified in the SGSN config file, currently osmo_sgsn.cfg."
	                 ),
			"Timers.Channels.Idle" => array(
				array("selected"=>6000, 3000,3100,3200,3300,3400,3500,3600,3700,3800,3900,4000,4100,4200,4300,4400,4500,4600,4700,4800,4900,5000,5100,5200,5300,5400,5500,5600,5700,5800,5900,6000,6100,6200,6300,6400,6500,6600,6700,6800,6900,7000,7100,7200,7300,7400,7500,7600,7700,7800,7900,8000,8100,8200,8300,8400,8500,8600,8700,8800,8900,9000),
				"display" => "select",
				"comment" => "How long in milliseconds a GPRS channel is idle before being returned to the pool of channels.
Also depends on Channels.Min.
Currently the channel cannot be returned to the pool while there is any GPRS activity on any channel.
Interval allowed 3000:6000(100)
Defaults to 6000."
			),
			"Timers.MS.Idle" => array(
				array("selected"=>600, 300,310,320,330,340,350,360,370,380,390,400,410,420,430,440,450,460,470,480,490,500,510,520,530,540,550,560,570,580,590,600,610,620,630,640,650,660,670,680,690,700,710,720,730,740,750,760,770,780,790,800,810,820,830,840,850,860,870,880,890,900),
				"display" => "select",
				"comment" => "How long in seconds an MS is idle before the BTS forgets about it
Interval allowed 300:900(10)
Defaults to 600."
	                 ),
			"Timers.MS.NonResponsive" => array(
				array("selected"=>6000, 3000,3100,3200,3300,3400,3500,3600,3700,3800,3900,4000,4100,4200,4300,4400,4500,4600,4700,4800,4900,5000,5100,5200,5300,5400,5500,5600,5700,5800,5900,6000,6100,6200,6300,6400,6500,6600,6700,6800,6900,7000,7100,7200,7300,7400,7500,7600,7700,7800,7900,8000,8100,8200,8300,8400,8500,8600,8700,8800,8900,9000),
				"display" => "select",
				"comment" => "How long in milliseconds a TBF is non-responsive before the BTS kills it
Interval allowed 3000:9000(100)
Defaults to 6000."
			),
			"Timers.T3169" => array(
				array("selected"=>5000,2500,2600,2700,2800,2900,3000,3100,3200,3300,3400,3500,3600,3700,3800,3900,4000,4100,4200,4300,4400,4500,4600,4700,4800,4900,5000,5100,5200,5300,5400,5500,5600,5700,5800,5900,6000,6100,6200,6300,6400,6500,6600,6700,6800,6900,7000,7100,7200,7300,7400,7500),
				"display" => "select",
				"comment" => "Nonresponsive downlink TBF resource release timer, in milliseconds. See GSM04.60 Section 13
Interval allowed 2500:7500(100)
Defaults to 5000"
			),
			"Timers.T3191" => array(
				array("selected"=>5000,2500,2600,2700,2800,2900,3000,3100,3200,3300,3400,3500,3600,3700,3800,3900,4000,4100,4200,4300,4400,4500,4600,4700,4800,4900,5000,5100,5200,5300,5400,5500,5600,5700,5800,5900,6000,6100,6200,6300,6400,6500,6600,6700,6800,6900,7000,7100,7200,7300,7400,7500),
				"display" => "select",
				"comment" => "Nonresponsive downlink TBF resource release timer, in milliseconds. See GSM04.60 Section 13
Interval allowed 2500:7500(100)
Defaults to 5000"
				 
			 ),
			 "Timers.T3193" => array(
				 array("selected"=>0,0,100,200,300,400,500,600,700,800,900,1000,1100,1200,1300,1400,1500,1600,1700,1800,1900,2000,2100,2200,2300,2400,2500,2600,2700,2800,2900,3000,3100,3200,3300,3400,3500,3600,3700,3800,3900,4000,4100,4200,4300,4400,4500,4600,4700,4800,4900,5000),
				 "display" => "select",
				 "comment" => "Timer T3193 (in milliseconds) in the base station corresponds to T3192 in the MS, which is set by GPRS.CellOptions.T3192Code
The T3193 value should be slightly longer than that specified by the T3192Code
If 0, the BTS will fill in a default value based on T3192Code
Interval allowed 0:5000(100)
Defaults to 0"
			),
			"Timers.T3195" => array(
				array("selected"=>5000,2500,2600,2700,2800,2900,3000,3100,3200,3300,3400,3500,3600,3700,3800,3900,4000,4100,4200,4300,4400,4500,4600,4700,4800,4900,5000,5100,5200,5300,5400,5500,5600,5700,5800,5900,6000,6100,6200,6300,6400,6500,6600,6700,6800,6900,7000,7100,7200,7300,7400,7500),
				"display" => "select",
				"comment" => "Nonresponsive downlink TBF resource release timer, in milliseconds. See GSM04.60 Section 13
Interval allowed 2500:7500(100)
Defaults to 5000"
			)
		),
		"sgsn" => array(
			"Debug"=>array(	
				"display" => "checkbox",
				"value" =>"0",
				"comment" => "Add layer-3 messages to the GGSN.Logfile, if any."
			),
			"Timer.ImplicitDetach" => array( 
				"display" => "text",
				"value" => "3480",
				"comment" => "GPRS attached MS is implicitly detached in seconds
Should be at least 240 seconds greater than SGSN.Timer.RAUpdate. 3GPP 24.008 11.2.2.
Interval allowed 2000:4000(10).
Defaults to 3480.",
				"validity" => array("check_timer_implicitdetach", "Timer_RAUpdate")
			),
			"Timer.MS.Idle" => array(
				array("selected"=>600, 300,310,320,330,340,350,360,370,380,390,400,410,420,430,440,450,460,470,480,490,500,510,520,530,540,550,560,570,580,590,600,610,620,630,640,650,660,670,680,690,700,710,720,730,740,750,760,770,780,790,800,810,820,830,840,850,860,870,880,890,900),
				"display" => "select",
				"comment" => "How long an MS is idle before the SGSN forgets TLLI specific information. Interval allowed 300:900(10). Defaults to 600."
			),
			"Timer.RAUpdate" => array(
				"display" => "text",
				"value" => "3240",
				"comment" => "How often the MS reports into the SGSN when it is idle, in seconds.
Also known as T3312, 3GPP 24.008 4.7.2.2.
Setting to 0 or >12000 deactivates entirely, i.e., sets the timer to effective infinity.
Note: to prevent GPRS Routing Area Updates you must set both this and GSM.Timer.T3212 to 0.
Interval allowed 0:11160(2)
Defaults to 3240.",
				"validity" => array("check_timer_raupdate")
			),
			"Timer.Ready" =>array(
				array("selected"=>44, 30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90),
				"display" => "select",
				"comment" => "Also known as T3314, 3GPP 24.008 4.7.2.1
Inactivity period required before MS may perform another routing area or cell update, in seconds. Interval allowed 30:90. Defaults to 44."
			)
		),
		"ggsn" => array(
			"DNS" => array(	
				"display" => "text",
				"comment" => "The list of DNS servers to be used by downstream clients.
By default, DNS servers are taken from the host system.
To override, specify a space-separated list of the DNS servers, in IP dotted notation, eg: 1.2.3.4 5.6.7.8.
To use the host system DNS servers again, execute 'unconfig GGSN.DNS'",
				"validity" => array("check_valid_dns")
			),
			"Firewall.Enable" => array( 
				array( "selected"=> "1", array("Firewall.Enable_id"=>"0","Firewall.Enable"=>"no firewall"),array( "Firewall.Enable_id"=>"1","Firewall.Enable"=> "block MS attempted access to OpenBTS or other MS"),array ( "Firewall.Enable_id"=>"2","Firewall.Enable"=> "block all private IP addresses")),
				"display" => "select",
				"comment" => " 0=no firewall; 1=block MS attempted access to OpenBTS or other MS; 2=block all private IP addresses
Defaults to 1."
			),
			"IP.MaxPacketSize" =>array( 
				"display" => "text",
				"value" =>"1520",
				"comment" => "Maximum size of an IP packet.Should normally be 1520
Allowed value 1492:9000",
				"validity" => array("check_field_validity",1492,9000)
			),
			"IP.ReuseTimeout" => array(
				array("selected"=>180, 120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240),
				"display" => "select",
				"comment" => "How long IP addresses are reserved after a session ends. Allowed value 120:240."
			),		
			"IP.TossDuplicatePackets" => array(
				"display" => "checkbox",
				"value" => "0",
				"comment" => "Toss duplicate TCP/IP packets to prevent unnecessary traffic on the radio."
			),
			"Logfile.Name" => array(
				"display" => "text",
				"value" => "ggsn.log",
				"comment" => "If specified, internet traffic is logged to this file e.g. ggsn.log"
			),
			"MS.IP.Base" =>	array(
				"display" => "text",
				"value" => "192.168.99.1",
				"comment" => "Base IP address assigned to MS.",
				"validity" => array("check_valid_ipaddress")
			),
			"MS.IP.MaxCount" => array(
				"param"=>"MS.IP.MaxCount",
				"display" => "text",
				"value" => "254",
				"comment" => "Number of IP addresses to use for MS. Allowed values 1:254
Defaults to 254",
				"validity" => array("check_field_validity",1,254)
			),
			"MS.IP.Route" => array(
				"display" => "text",
				"value" => "",
				"comment" => "A route address to be used for downstream clients.
By default, OpenBTS manufactures this value from the GGSN.MS.IP.Base assuming a 24 bit mask
To override, specify a route address in the form xxx.xxx.xxx.xxx/yy.
The address must encompass all MS IP addresses. To use the auto-generated value again, execute 'unconfig GGSN.MS.IP.Route'"
			),
			"ShellScript" => array(
				"display" => "text",
				"value" => "",
				"comment" => "A shell script to be invoked when MS devices attach or create IP connections.
By default, this feature is disabled.
To enable, specify an absolute path to the script you wish to execute e.g. /usr/bin/ms-attach.sh.
To disable again, execute 'unconfig GGSN.ShellScript'"
			),
			"TunName" => array(
				"display" => "text",
				"value" => "sgsntun",
				"comment" => "Tunnel device name for GGSN."
			)
		)
	);


	$fields["Transceiver"] = array(
		"transceiver" => array(
			"Path" => array(
				array("selected"=>"./transceiver", "./transceiver", "./transceiver-bladerf", "./transceiver-rad1", "./transceiver-usrp1", "./transceiver-uhd"),
				"display" => "select",
				"comment" => "Path to the transceiver relative to where MBTS is started.
Should be one of: ./transceiver-bladerf ./transceiver-rad1 ./transceiver-usrp1 ./transceiver-uhd.
Defaults to ./transceiver"
			),
			"Args" => array(
				"display" => "text",
				"value" => "",
				"comment"=> "Extra arguments to be passed by MBTS to the transceiver executable.
Defaults to no arguments."
			),
			"MinimumRxRSSI" => array(
				"display" => "text",
				"value" => "-63",
				"comment" => "Minimum signal strength (in dB) of acceptable bursts.
Bursts received at the physical layer below this threshold are ignored.
Do not adjust without proper calibration.
Interval allowed: -90..90
Defaults to -63",
				"validity" => array("check_field_validity", -90, 90)
			),
			"RadioFrequencyOffset" => array(
				array("selected"=>"Factory calibrated", "Factory calibrated",64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192),
				"display" => "select",
				"comment" => "Master clock frequency adjustment. Fine-tuning adjustment for the transceiver master clock, roughly 170 Hz/step.
Set at the factory, do not adjust without proper calibration.
Interval allowed: 64..192
Defaults to empty value. Radios provide their own calibrated default. To see current value run 'mbts trxfactory' after connecting to rmanager interface."
			),
			"TxAttenOffset" => array(
				array("selected"=>"Factory calibrated", "Factory calibrated",0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100),
				"display" => "select",
				"comment" => "Transmitter attenuation in dB.
Hardware-specific gain adjustment for transmitter, matched to the amplifier.
Do not adjust without proper calibration
Interval allowed: 0..100
Defaults to empty value. Radios provide their own calibrated default. To see current value run 'mbts trxfactory' after connecting to rmanager interface."
			),
			"Timeout.Clock" => array(
				array("selected"=>10, 5,6,7,8,9,10,11,12,13,14,15),
				"display" => "select",
				"comment" => "Transceiver read timeout in seconds.
How long to wait during a read operation from the transceiver before giving up
Interval allowed: 5..15
Defaults to 10"
			)
		)
	);

	$fields["Control"] = array(
		"control" => array(
			"VEA" => array(
				"display" => "checkbox",
				"value" => "1",
				"comment" => "Use very early assignment for speech call establishment.
See GSM 04.08 Section 7.3.2 for a detailed explanation of assignment types.
See GSM 04.08 Sections 9.1.8 and 10.5.2.4 for an explanation of the NECI bit.
Some handsets exhibit bugs when VEA is used that can affect performance.
If VEA is selected, [gsm_advanced] CellSelection.NECI should be set to 1.
Defaults to yes.",
				"validity" => array("validate_neci_vea")
			),
			"LUR.AttachDetach" => array(
				"display" => "checkbox",
				"value" => "1",
				"comment" => "Use the attach/detach procedure. This will make initial LUR more prompt.
It will also cause an un-registration if the handset powers off and really heavy LUR loads in areas with spotty coverage.
Defaults to yes."
			),
			"SACCHTimeout.BumpDown" => array(
				array("selected"=>1, 1,2,3),
				"display" => "select",
				"comment"=> "RSSI decrease amount. Decrease the RSSI by this amount to induce more power in the MS each time we fail to receive a response from it.
Interval allowed: 1..3
Defaults to 1"
			)
		)
	);

	$fields["Tapping"] = array(
		"tapping" => array(
			"GSM" =>array(
				"display" => "checkbox",
				"value" => "0",
				"comment" => "Capture GSM signaling at L1/L2 interface via GSMTAP
Do not leave tapping enabled after finishing troubleshooting.
Defaults to no."
			),
			"GPRS" => array(
				"display" => "checkbox",
				"value" => "0",
				"comment" => "Capture GPRS signaling and traffic at L1/L2 interface via GSMTAP.
Do not leave tapping enabled after finishing troubleshooting
Defaults to no"
			),
			"TargetIP" => array(
				"display" => "text",
				"value" => "127.0.0.1",
				"comment" => "Target IP address for GSMTAP packets.
The IP address of receiving Wireshark, if you use it for real time traces.",
				"validity" => array("check_valid_ipaddress")
			)
		)
	);

	$fields["Test"] = array(
		"test" => array(
			"SimulatedFER.Downlink"=>array(	
				array("selected"=>0, 0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100),
				"display" => "select",
				"comment" => "Probability (0-100) of dropping any downlink frame to test robustness."
			),
			 "SimulatedFER.Uplink"=> array( 
				 array("selected"=>0, 0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100),
				"display" => "select",
				"comment" => " Probability (0-100) of dropping any uplink frame to test robustness."
			),
			"UplinkFuzzingRate" => array(
				array("selected"=>0, 0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100),
				"display" => "select",
				"comment" => "Probability (0-100) of flipping a bit in any uplink frame to test robustness."
			)
		)
	);

	$fields["YBTS"] = array(
		"ybts" => array(
			"mode" => array(
				array("selected"=> "nib", "nib","roaming"),
				"display" => "select",
				"comment" => "BTS mode of operation. This setting will specify which Javascript script 
to load for the operation. Possible values are:
    - nib: loads script necessary for Network In a Box mode of operation
    - roaming: loads script necessary for the voice roaming mode of operation"
			),
			"heartbeat_ping"=> array(
				"display" => "text",
				"value" => "30000",
				"comment" => "Interval, in milliseconds, to send heartbeat (PING) to MBTS application.
Ping is sent only if idle, e.g. it is not sent if there are other signalling packets to be sent.
This parameter is applied on reload.
Allowed interval: 1000 .. 120000
Defaults to 30000",
				"validity"=> array("check_field_validity",1000,120000)
			),
			"heartbeat_timeout"=> array(
                                "display" => "text",
				"value" => "60000",
				"comment" => "Interval, in milliseconds, to wait for heartbeat (PING) or any other signalling packets from mbts application
MBTS will be restarted if this timer ellapses
This parameter is applied on reload
Minimum allowed value is heartbeat_ping plus 3000, it will be automatically adjusted
Maximum allowed value is 180000
Defaults to 60000.",
				"validity"=> array("check_field_validity", 3000,180000)
			),
			"handshake_start" => array(
				"display" => "text",
				"value" => "60000",
				"comment" => "Interval, in milliseconds, to wait for handshake from the MBTS application after it's started
MBTS will be restart if this timer ellapses
This parameter is applied on reload
Allowed interval: 10000 .. 300000
Defaults to 60000.",
				"validity"=> array("check_field_validity",10000,300000)
			),
			"max_restart" => array(
				"display" => "text",
				"value" => "10",
				"comment" => "Maximum number of restart attempts
When start counter reaches this value the entire Yate application will exit!
The start index is reset each time the peer notifies radio up after handshake.
This parameter is applied on reload.
Defaults to 10, minimum allowed value is 3",
				"validity"=> array("check_field_validity",3,10)
			),
			"peer_cmd" => array(
				"display" => "text",
				"value" => "\${modulepath}/server/bts/mbts",
				"comment" => "Command used to run the MBTS application.
Defaults to expansion of \${modulepath}/server/bts/mbts"
			),
			"peer_arg" => array(
				"display" => "text",
				"value" => "",
				"comment" => "Optional command line parameter for the MBTS application.
Defaults to no parameter."
			),
			"peer_dir" => array(
				"display" => "text",
				"value" => "\${modulepath}/server/bts",
				"comment" => "Directory to change after forking the MBTS application.
Defaults to expansion of \${modulepath}/server/bts"
			),
			"print_msg" => array(
				"display" => "text",
				"value" => "yes",
				"comment" => "Print sent/received messages to output if debug is at level 9.
Allowed values are boolean values or string.
If a non boolean value is specified message buffer will be shown.
This parameter is applied on reload.
Defaults to yes if empty or missing."
			),
			"print_msg_data" => array(
				"display" => "text",
				"value" => "verbose",
				"comment" => "Print sent/received messages data.
This parameter is ignored if message print is disabled.
Allowed values are boolean values or string.
If a non boolean value is specified (empty string also) message data will be printed in a friendly way (e.g. XML data will be printed with children on separate lines).
This parameter is applied on reload.
Defaults to verbose."
			),
			"imei_request" => array(
				"display" => "checkbox",
				"value" => "1",
				"comment" => "Ask for IMEI when updating location.
This parameter is applied on reload.
Defaults to yes"
			),
			"tmsi_expire" => array(
				"display" => "text",
				"value" => "864000",
				"comment" => "Interval, in seconds, to keep TMSIs.
This interval should be enforced in the applications, not ybts.cpp
This parameter is applied on reload.
Allowed interval: 7200..2592000.",
				"validity"=> array("check_field_validity",7200,2592000)
			),
			"t305" => array(
				"display" => "text",
				"value" => "30000",
				"comment" => "Disconnect (hangup) timeout interval in milliseconds.
After sending Disconnect the call will wait for Release or Release Complete.
Release will be sent on timer expiry.
This parameter is applied on reload.
Interval allowed: 20000..60000
Defaults to 30000",
				"validity"=> array("check_field_validity",20000,60000)
			),
			"t308"  => array(
				"display" => "text",
				"value" => "5000",
				"comment" => "Release timeout interval in milliseconds (wait for Release Complete).
Release will be sent on first expiry, call reference will be released on second expiry.
This parameter is applied on reload.
Interval allowed: 4000..20000
Defaults to 5000",
				"validity"=> array("check_field_validity",4000,20000)
			),   
		        "t313" => array(
				"display" => "text",
				"value" => "5000",
				"comment" => "Connect (answer) timeout interval in milliseconds (wait for Connect Ack)
Call reference will be released on expiry
This parameter is applied on reload
Interval allowed: 4000..20000
Defaults to 5000",
				"validity"=> array("check_field_validity",4000,20000)
			),
			"sms.timeout" => array(
				"display" => "text",
				"value" => "300000",
				"comment" => "Interval, in milliseconds, to wait for a MT SMS to complete
This interval includes paging if needed.
This parameter can be overridden in the msg.execute message by a 'timeout' parameter.
This parameter is applied on reload.
Interval allowed: 5000..600000
Defaults to 300000",
				"validity"=> array("check_field_validity",5000, 600000)
			),
			"ussd.session_timeout" => array(
				"display" => "text",
				"value" => "600000",
				"comment" => "Timeout, in milliseconds, of USSD sessions.
This parameter configures the overall USSD session duration.
When timed out a session will be terminated.
This parameter can be overridden in the msg.execute message by a 'timeout' parameter.
This parameter is applied on reload.
Minimum allowed value is 30000.
Defaults to 600000 (10 minutes)",
				"validity"=> array("check_field_validity",30000, 600000)
			),
			"export_xml_as" => array(
				array("selected"=> "object", "object","string","both"),
				"display" => "select",
				"comment" => "Specify in which way the XML will be passed along into Yate messages.
Allowed values are:
- string: pass the XML as a string
- object: pass the XML as an object 
- both: pass the XML as a string and object
This parameter is applied on reload
Defaults to object."
			)
		),
		"security" => array(
			"t3260" => array(
				"display" => "text",
				"value" => "720000",
				"comment" => "Interval (in milliseconds) to wait for a response to authentication requests.
Authentication will be aborted on timer expiry.
This parameter is applied on reload.
Interval allowed: 5000..3600000
Defaults to 720000",
				"validity" => array("check_t3260", "t3260")
			),
			"auth.call" => array(
				"display" => "checkbox",
				"value" => "0",
				"comment" => "Authenticate MS on MT calls. This parameter is applied on reload. Defaults to 'no'."
			),
			"auth.sms" => array(
				"display" => "checkbox",
				"value" => "0",
				"comment" => "Authenticate MS on MT SMS. This parameter is applied on reload. Defaults to 'no'."
			),
			"auth.ussd" => array(
				"display" => "checkbox",
				"value" => "0",
				"comment" => "Authenticate MS on MT USSD. This parameter is applied on reload. Defaults to 'no'."
			)

		),
		"roaming" => array(
			"expires" => array(
				"display" => "text",
				"value" => "3600",
				"comment" => "Expire time for registrations."
			),
			"reg_sip" => array(
				"display" => "text",
				"comment" => "String: ip:port where SIP requests are sent.
It is REQUIRED to set reg_sip or nodes_sip.
Example: reg_sip=192.168.1.245:5058",
				"validity" => array("valid_reg_sip","reg_sip")
			),
			"nodes_sip" => array(
				"display" => "text",
				"comment" => "json object: node => ip:port of each YateUCN server
node, which is computed based on the TMSI received from the handset.
This ensures that registrations are always sent to the same YateUCN server.".
//It is REQUIRED to set reg_sip or nodes_sip.
"Example: nodes_sip={\"123\":\"192.168.1.245:5058\",\"101\":\"192.168.1.176:5059\"}",
				"validity" => array("valid_nodes_sip", "nodes_sip")
			),
			"nnsf_bits" => array(
				"display" => "text",
				"value" => "8",
				"comment" => "Int: Number of bits to use for
the Non Access Stratum (NAS) Node Selection Function (NNSF)."
			),
			"my_sip" => array(
				"display" => "text",
				"comment" => "string: ip:port for the local SIP listener.
Unless otherwise configured, this is the IP of the machine
where YateBTS is installed.
Example: my_sip=198.168.1.168"
			),
			"gstn_location" => array(
				"display" => "text",
				"comment" => "String: unique number that identies the cell in the national database
Associated to each base station by the network operator."
			),
			"text_sms" => array(
				"display" => "checkbox",
				"comment" => "If possible decode and send the SMS as text/plain SIP MESSAGE body.
By default the binary type application/vnd.3gpp.sms is used."
			)
		),

		"handover" => array(
			"enable" => array(
				"display" => "checkbox",
				"value" => "1",
				"comment" => "Globally enable handover functions.
Default is enabled."
			),
			"neighbors" => array(
				"display" => "text",
				"comment" => "Comma separated list of neighbor SIP addresses.
Each neighbor will be periodically queried for target handover availability.
Example: neighbors=10.0.0.1, 10.0.0.2, 10.0.0.3:5065.
Default is empty."
			),
			"reason" => array(
				"display" => "text",
				"value" => "GSM;text=\"Handover\"",
				"comment" => 'Text to place in the Reason SIP header.
An empty or boolean false value disables the Reason header.
Default: GSM;text="Handover".'
			)
		),
	);

/*	$fields["Logging"] = array(
		"logging" => array(
			"Level"=> array(
				array("selected"=> "NOTICE", "EMERG", "ALERT", "CRIT", "ERR", "WARNING", "NOTICE", "INFO","DEBUG"),
				"display" => "select",
				"comment" => "Default logging level of MBTS.
Valid values are: EMERG, ALERT, CRIT, ERR, WARNING, NOTICE, INFO, DEBUG.
Defaults to NOTICE."
			)
		)
	);
 */

	if (isset($private_version) && $private_version===true) {
		// add dataroam mode
		$fields["YBTS"]["ybts"]["mode"][0][] = "dataroam";
		$fields["YBTS"]["ybts"]["mode"]["comment"] .= "\n- dataroam: voice and data roaming modes. Available only in the private YateBTS version";

		$fields["YBTS"]["gprs_roaming"] = array(
			"local_breakout" => array(
				"display" => "text",
				"comment" => "boolean or regexp: Activate local IP address termination.
If boolean enables or disables LBO for all subscribers.
If regexp activates LBO only for matching IMSIs.
Defaults to no",
			),
			"gprs_nnsf_bits" => array(
				"display" => "text",
				"column_name" => "NNSF bits",
				"comment" => "Number of bits to use for SGSN
Non Access Stratum (NAS) Node Selection Function (NNSF).
Defaults to 0 (disabled)."
			), 
			"nnsf_dns" => array(
				"display" => "text",
				"column_name" => "NNSF DNS",
				"comment" => "boolean or string: Use DNS for SGSN node mapping.
Defaults to no if nnsf_bits is zero or an explicit node mapping exists.
If a string is provided it will replace default domain mnc<NNN>.mcc<NNN>.gprs"
			),
			"network_map" => array(
				"column_name" => "Explicitly map network nodes to IP addresses",
				"display" => "textarea",
				"comment" => "Example:
20=10.0.0.1
23=10.2.74.9"
			)
		    );
	}

	foreach($fields as $section => $subsections) {
		foreach ($subsections as $subs => $data1) {
			foreach ($data1 as $paramname => $data) {
				if (isset($data["comment"])) {
					$fields[$section][$subs][$paramname]["comment"] = str_replace(array("\t\t\t\t","\n"),array("","<br/>"), $data["comment"]);
				}
		}
		}
	}


	return $fields;
}

?>
