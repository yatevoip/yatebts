<?php
/**
 * subscribers.php
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

function subscribers()
{
	list_subscribers();
}

function list_subscribers()
{
	global $yate_conf_dir;

	$res = test_default_config();
	if (!$res[0]) {//permission errors
		errormess($res[1], "no");
		return;
	}

	nib_note("Subscribers are accepted based on two criteria: regular expression that matches the IMSI or they be inserted individually.");

	$have_subscribers = true;
	$res = get_subscribers();

	if (!$res[0]) {
		$regexp = get_regexp();
		if ($regexp[0] && strlen($regexp[1])) {
			$regexp = $regexp[1];
			$have_subscribers = false;
		} else {
			if (!getparam("overwrite_file")) {
				errormess($res[1]. ". Press <a href=\"main.php?module=subscribers&method=list_subscribers&overwrite_file=yes\">here</a> to overwrite subscribers.js.", "no");
				return;
			} else { 
				$new_file = $yate_conf_dir."subscribers.js.backup".date("Ymdhm");
				rename($yate_conf_dir."subscribers.js", $new_file);
				message("Your old old file was moved to $new_file", "no");
			}
			$subscribers = array();
		}
	} elseif (!count($res[1])) {
		$regexp = get_regexp();
		if ($regexp[0] && strlen($regexp[1])) {
			$regexp = $regexp[1];
			$have_subscribers = false;
		} else
			$subscribers = array();
	} else {
		//$_SESSION["have_subscribers"] = true;
		$subscribers = $res[1];
	}

	if ($have_subscribers) {
		$all_subscribers = array();
		foreach($subscribers as $imsi=>$subscr)
			$all_subscribers[] = $subscr;

		$formats = array("IMSI"=>"imsi","msisdn","short_number","ki","op","IMSI Type"=>"imsi_type","function_display_bit_field:active"=>"active");
		table($all_subscribers, $formats, "subscriber", "imsi", array("&method=edit_subscriber"=>"Edit","&method=delete_subscriber"=>"Delete"), array("&method=add_subscriber"=>"Add subscriber", "&method=edit_regexp"=>"Accept by REGEXP"));
	} else {
		start_form();
		addHidden(null, array("method"=>"edit_regexp", "regexp"=>$regexp));
		$fields = array("regexp"=>array("value"=>$regexp, "display"=>"fixed"));
		editObject(null,$fields,"Regular expression based on which subscriber IMSI are accepted for registration",array("Modify", "Set subscribers"),null,true);
		end_form();
	}
}

function online_subscribers()
{
	$command = "nib list registered";
	$marker_end = "null";
	$res = get_socket_response($command, $marker_end);

	if (isset($res[0]) && $res[0]===false) {
		if (isset($res[2])) {
			nib_note($res[1]);
			return;
		}
		errormess($res[1],"no");
		$online_subscribers = array();
	} else
		$online_subscribers = generate_table_format_from_socket($res);

	$formats = array("IMSI","MSISDN");
	table($online_subscribers, $formats, "online subscriber", "imsi");
}

function rejected_imsis()
{
	$command = "nib list rejected";
	$marker_end = "null";
	$res = get_socket_response($command, $marker_end);

	if (isset($res[0]) && $res[0]===false) {
		if (isset($res[2])) {
                       nib_note($res[1]);
                       return;
                }
		errormess($res[1],"no");
		$rejected_subscribers = array();
	} else
		$rejected_subscribers = generate_table_format_from_socket($res);

	$formats = array("IMSI","No attempts register");
	table($rejected_subscribers, $formats, "rejected IMSIs", "imsi");
}

function edit_regexp($error=null,$error_fields=array())
{
	if (getparam("Set_subscribers")=="Set subscribers")
		return edit_subscriber();

	$regexp = getparam("regexp");

	nib_note("If a regular expression is used, 2G/3G authentication cannot be used. For 2G/3G authentication, please set subscribers individually.");
	$fields = array(
		"regexp" => array("value"=> $regexp, "required"=>true, "comment"=>"Ex: /^310030/")
	);
	error_handle($error,$fields,$error_fields);
        start_form();
        addHidden("write_file");
        editObject(NULL,$fields,"Regular expression based on which subscriber IMSI are accepted for registration","Save");
        end_form();
}

function edit_regexp_write_file()
{
	$expressions = array();
	$res = get_regexp();
	if (!$res[0]) 
		errormess($res[1], "no");
	else
		$expressions = array($res[1]);
	
	$regexp = getparam("regexp");

	if (!$regexp)
		return edit_regexp("Please set the regular expression!",array("regexp"));

	if (in_array($regexp, $expressions)) {
		notice("Finished setting regular expression.", "subscribers");
		return;
	}	

	//$write_regexp[count($expressions)] = $regexp;
	$res = set_regexp($regexp);
	if (!$res[0])
		return edit_regexp($res[1]);
	notice("Finished setting regular expression. For changes to take effect please restart yate or reload just nib.js from telnet with command: \"javascript reload nib\". Please note that after this you will lose existing registrations.", "subscribers");
}

function edit_subscriber($error=null,$error_fields=array())
{
	global $method;
	$method = "edit_subscriber";

	$imsi = getparam("imsi") ? getparam("imsi") : getparam("imsi_val");
	
	if ($imsi) {
		$subscriber = get_subscriber($imsi);
		if (!$subscriber[0]) {
			if (!$error)
				// if there is not error, print message that we didn't find subscriber
				// otherwise if was probably an error when adding one so this message is not relevant
				errormess($subscriber[1], "no");
			$subscriber = array();
		}if (isset($subscriber[1]))
			$subscriber = $subscriber[1];
		else
			$subscriber = array();
	} else
		$subscriber = array();

	$imsi_type = array("selected"=> "2G", "2G", "3G");

	if (get_param($subscriber,"imsi_type"))
		$imsi_type["selected"] = get_param($subscriber,"imsi_type");
	$active = (get_param($subscriber,"active") == 1) ? 't' : 'f';

	$fields = array(
		"imsi"   => array("value"=>$imsi, "required"=>true, "comment"=>"SIM card id", "column_name"=>"IMSI"),
		"msisdn" => array("value"=>get_param($subscriber,"msisdn"), "comment"=>"DID associated to this IMSI. When outside call is made, this number will be used as caller number.", "column_name"=>"MSISDN"),
		"short_number" => array("value" => get_param($subscriber,"short_number"),"comment"=>"Short number that can be used to call this subscriber."),
		"active" => array("value"=>$active, "display"=>"checkbox"),
		"imsi_type" => array($imsi_type, "display"=>"select", "column_name"=>"IMSI Type", "required"=>true, "comment"=> "Type of SIM associated to the IMSI"),
		"ki" => array("value"=>get_param($subscriber,"ki"), "comment"=>"Card secret", "required"=>true),
		"op" => array("value"=>get_param($subscriber,"op"), "comment"=>"Operator secret. Empty for 2G IMSIs.<br/>00000000000000000000000000000000 for 3G IMSIs.")
	);
	
	if ($imsi && count($subscriber) && !in_array("imsi",$error_fields))
		$fields["imsi"]["display"] = "fixed";
	if (!count($subscriber))
		$imsi = NULL;

	error_handle($error,$fields,$error_fields);
	start_form();
	addHidden("write_file", array("imsi_val"=>$imsi));
	editObject(NULL,$fields,"Set subscriber","Save");
	end_form();
}

function edit_subscriber_write_file()
{
	$res = get_subscribers();
	if (!$res[0]) {
		//errormess($res[1], "no");
		$subscribers = array();
	} else
		$subscribers = $res[1];

	$imsi = (getparam("imsi")) ? getparam("imsi") : getparam("imsi_val");
	if (!$imsi)
		return edit_subscriber("Please set 'imsi'",array("imsi"));
	if (strlen($imsi)!=14 && strlen($imsi)!=15)
		return edit_subscriber("Invalid IMSI $imsi. IMSI length must be 14 or 15 digits long.",array("imsi"));
	$subscriber = array("imsi"=>$imsi);

	$fields = array("msisdn"=>false, "short_number"=>false, "active"=>false, "ki"=>true, "op"=>false, "imsi_type"=>true);
	foreach ($fields as $name=>$required) {
		$val = getparam($name);
		if ($required && !$val)
			return edit_subscriber("Field $name is required");
		$subscriber[$name] = $val;
	}
	$subscriber["active"] = ($subscriber["active"]=="on") ? 1 : 0;
	if ($subscriber["imsi_type"]=="2G")
		$subscriber["op"] = "";

	if (getparam("imsi_val") && isset($subscribers[$imsi])) {
		$modified = false;
		//check if there are changes
		$subs_file = $subscribers[$imsi];
		foreach ($fields as $name=>$required) {
			$val = getparam($name);
			if ($name == "active")
				$val = (getparam($name) == "on") ? 1 : 0;

			if ($subs_file[$name] != $val) 
				$modified = true;
			if ($modified)
				break;
		}

		if (!$modified) {
			notice("Finished setting subscriber with IMSI $imsi.", "subscribers");
			return;
		}
	}

	if (!getparam("imsi_val") && isset($subscribers[$imsi]))
		return edit_subscriber("Subscriber with IMSI $imsi is already set.",array("imsi"));
	$subscribers[$imsi] = $subscriber;

	$res = set_subscribers($subscribers);
	if (!$res[0])
		return edit_subscriber($res[1]);
	notice("Finished setting subscriber with IMSI $imsi. For changes to take effect please restart yate or reload just nib.js from telnet with command: \"javascript reload nib\".  Please note that after this you will lose existing registrations.", "subscribers");
}

function delete_subscriber()
{
	ack_delete("subscriber", getparam("imsi"), NULL, "imsi", getparam("imsi"));
}

function delete_subscriber_database()
{
	$imsi = getparam("imsi");
	$res = get_subscribers();
	if (!$res[0])
		return errormess($res[1], "no");
	$subscribers = $res[1];

	if (!isset($subscribers[$imsi]))
		return errormess("IMSI $imsi is not in subcribers list.","no");

	unset($subscribers[$imsi]);
	$res = set_subscribers($subscribers);
	if (!$res[0]) 
		return errormess($res[1],"no");

	notice("Finished removing subscriber with IMSI $imsi.", "subscribers");
}

function set_subscribers($subscribers)
{
	global $yate_conf_dir, $subscribers_prefix, $subscribers_suffix;

	$js_file = new JsObjFile($yate_conf_dir."subscribers.js", $subscribers_prefix, $subscribers_suffix, $subscribers);
	$js_file->safeSave();

	if (!$js_file->status())
		return array(false, $js_file->getError());
	return array(true);
}

function set_regexp($regexp)
{
	global $yate_conf_dir, $regexp_prefix, $regexp_suffix;
	
	$filename = $yate_conf_dir."subscribers.js";
	$file = new GenericFile($filename);
	$file->openForWrite();
	if (!$file->status())
		return array(false, $file->getError());
	$fh = $file->getHandler("w");

	fwrite($fh, $regexp_prefix.$regexp.$regexp_suffix);
	$file->close();

	return array(true);
}

function get_subscribers()
{
	global $yate_conf_dir, $subscribers_prefix, $subscribers_suffix;

	$filename = $yate_conf_dir."subscribers.js";
	if (!is_file($filename))
		// if subscribers.js file doesn't exist don't return error, just empty subscribers array
		return array(true, array());

	$js_file = new JsObjFile($yate_conf_dir."subscribers.js", $subscribers_prefix, $subscribers_suffix);
	$js_file->read();
	if (!$js_file->status())
		return array(false, $js_file->getError());
	$subscribers = $js_file->getObject();
	if (!$subscribers)
		$subscribers = array();
	return array(true, $subscribers);
}

function get_regexp()
{
	global $yate_conf_dir, $regexp_prefix, $regexp_suffix;

	$filename = $yate_conf_dir."subscribers.js";
	if (!is_file($filename))
		// if subscribers.js file doesn't exist don't return error, just empty string
		return array(true,"");

	$file = new GenericFile($filename);
	$file->openForRead();
	if (!$file->status())
		return array(false, $file->getError());

	$fh = $file->getHandler();
	clearstatcache();
	$content = fread($fh, filesize($filename));
	if (substr($content,0,strlen($regexp_prefix))!=$regexp_prefix) {
		// file doesn't match, we probably have subscribers
		return array(true, "");
	}
	$content = substr($content,strlen($regexp_prefix));
	$content = substr($content,0,strlen($content)-strlen($regexp_suffix));

	return array(true, $content);
}

function get_subscriber($imsi)
{
	$subscribers = get_subscribers();
	if (!$subscribers[0])
		return $subscribers;
	else
		$subscribers = $subscribers[1];

	if (!isset($subscribers[$imsi]))
		return array(false, "Could not find subcriber with imsi $imsi");
	return array(true, $subscribers[$imsi]);
}

function detect_pysim_installed()
{
	global $pysim_path;

	if (!have_pysim_prog())
		return array(false, "Please install pySIM and create file config.php to set the location for pySIM after instalation. E.g. \$pysim_path = \"/usr/bin\";");

	$pysim_prog_path = have_pysim_prog();
	$pysim_real_path = str_replace(array("/pySim-prog.py","\n"), "", $pysim_prog_path);

	if (!isset($pysim_path)) 
                $pysim_path = $pysim_real_path;

        if (!is_file($pysim_path.'/'.'pySim-prog.py')) 
                return array(false, "The path for pySIM set in configuration file is not correct. Please set in file config.php the right location for pySIM. This path was detected: \$pysim_path = \"$pysim_real_path\";");
	
	return array(true);
}

function manage_sims()
{
	$pysim_installed = detect_pysim_installed();

	if (!$pysim_installed[0]) {
		errornote($pysim_installed[1]);
		return;
	}
	
	$all_sim_written = read_csv();

	$formats = array("IMSI"=>"imsi", "ICCID"=>"iccid", "operator_name", "mobile_country_code", "mobile_network_code", "ki", "opc", "function_display_add_into_subscribers:"=>"imsi,ki");
?>
	<div>
		<a class="write_sim" href="main.php?module=subscribers&method=write_sim_form"><img title="SIM Programmer" src="images/sim_programmer.png" alt="SIM Programmer" /></a>
	</div>
<?php
        table($all_sim_written, $formats, "written SIM", "sim");
}

function display_add_into_subscribers($imsi, $ki)
{
	global $module, $main;

	$subscribers = get_subscriber($imsi);

	if (!$subscribers[0]) {
		$link = $main."?module=$module&method=write_imsi_in_subscribers";
		$link .= "&imsi=".$imsi. "&ki=".$ki;	
		print "<a class=\"content\" href=\"$link\">Add in subscribers</a>";
	}else  
		print "";
}


function write_sim_form($error=null,$error_fields=array(), $generate_random_imsi = "on",$insert_subscribers = "on")
{
	global $yate_conf_dir;

	$pysim_installed = detect_pysim_installed();

        if (!$pysim_installed[0]) {
                errornote($pysim_installed[1]);
                return;
        } 

	$filename = $yate_conf_dir."ybts.conf";

	$file = new ConfFile($filename);

	$cc = get_country_code();
	$mcc = "001";
	$mnc = "01";
	$advanced_mcc = $advanced_mnc = $advanced_op = false;
	if (isset($file->structure["gsm"]["Identity.MCC"])) {
		$mcc = $file->structure["gsm"]["Identity.MCC"];
		$advanced_mcc = true;
	}
	if (isset($file->structure["gsm"]["Identity.MNC"])) {
		$mnc = $file->structure["gsm"]["Identity.MNC"];
		$advanced_mnc = true;
	}
	if (isset($file->structure["gsm"]["Identity.ShortName"])) {
		$op = $file->structure["gsm"]["Identity.ShortName"];
		$advanced_op = true;
	}
	 

	nib_note("There are two methods of writing the SIM cards, depending on the state of the \"Generate random IMSI\" field. If the field is selected, the SIM credentials are randomly generated. Otherwise, the data must be inserted manually. Please check that your SIM Card Reader is inserted into the USB port of your device. Before saving data, please insert a SIM card into the SIM Card Reader.");
	$type_card = array(
		array('card_type_id'=>'fakemagicsim', 'card_type'=>'FakeMagicSim'),
		array('card_type_id'=>'supersim', 'card_type'=>'SuperSim', ),
		array('card_type_id'=>'magicsim', 'card_type'=>'MagicSim'),
		array('card_type_id'=>'grcardsim','card_type'=>'GrcardSim'),
		array('card_type_id'=>'sysmosim-gr1', 'card_type'=>'Sysmocom SysmoSIM-GR1'),
		array('card_type_id'=>'sysmoSIM-GR2', 'card_type'=>'Sysmocom SysmoSIM-GR2' ), 
		array('card_type_id'=>'sysmoUSIM-GR1', 'card_type'=>'Sysmocom SysmoUSIM-GR1'),
		array('card_type'=>'auto','card_type_id'=>'auto')//autodetection is implemented in PySim/cards.py only for classes: FakeMagicSim, SuperSim, MagicSim the other types of card will fail (at this time 2014-04-16)
	);
	$type_card["selected"] = "grcardsim"; //type of card that was successfully written 

	if (!$cc) 
		$fields["country_code"] = array("required" => true, "value"=>$cc, "comment" => "Your Country code (where YateBTS is installed). Ex: 1 for US, 44 for UK");

	$fields["generate_random_imsi"] =  array("comment" => "Checked - if you want the parameter for the card to be generated randomly or uncheck - to insert your card values manually", "column_name"=>"Generate random IMSI", "javascript" => 'onclick="show_hide_cols()"', "display"=>"checkbox", "value"=>$generate_random_imsi);
	//show/hide fields when generate_random_imsi is unselected/selected
	$fields["imsi"] = array("required"=>true,"column_name"=>"IMSI", "comment" => "Insert IMSI to be written to the card. Ex.:001011641603116", "triggered_by"=>"generate_random_imsi");
	$fields["iccid"] = array("required"=>true,"column_name"=>"ICCID", "comment" => "Insert ICCID(Integrated Circuit Card Identifier) to be written to the card. Ex.: 8940001017992212557", "triggered_by"=>"generate_random_imsi");
	$fields["ki"] = array("required"=>true,"column_name"=>"Ki", "comment" => "Insert Ki to be written to the card. Ex.: 3b07f45b11d2003247e9ae6f13de7573", "triggered_by"=>"generate_random_imsi");
	$fields["opc"] = array("required"=>true,"column_name"=>"OPC", "comment" => "Insert OPC to be written to the card. Ex.: 6cb49bb6f99e97c3913924e7a1f32650", "triggered_by"=>"generate_random_imsi");
	
	$fields["insert_subscribers"] = array("comment" => "Uncheck if you don't want SIM credentials to be written in subscribers.js.", "display"=>"checkbox", "value" => $insert_subscribers); 
	//advanced fields if they are set in ybts.conf file
	$fields["operator_name"] = $advanced_op ? array("required" => true,"advanced"=> true, "value" =>$op, "comment" => "Set Operator name on SIM.") : array("required" => true, "comment" => "Set Operator name on SIM.");
	if ($cc)
		$fields["country_code"] = array("required" => true, "comment" => "Your Country code (where YateBTS is installed). Ex: 1 for US, 44 for UK", "advanced"=> true);
	$fields["card_type"] = array($type_card,"advanced"=> true, "required"=>true, "display"=>"select", "column_name"=> "Card Type", "comment" =>" Select the card type for writing SIM credentials. The SIM cards that you received are \"GrcardSim\". For other card types, see the list of cards supported by PySim. It is not guaranteed that your card will be written, even if it is in that list."); 
	$fields["mobile_country_code"] = $advanced_mcc ? array("required" => true,"advanced"=> true, "value" => $mcc, "comment" => "Set Mobile Country Code.", "javascript"=>" onClick=advanced('sim')") :
							array("required" => true,"value" => $mcc, "comment" => "Set Mobile Country Code.");
	$fields["mobile_network_code"] = $advanced_mnc ? array("required" => true,"advanced"=> true, "value" => $mnc, "comment" => "Set Mobile Network Code.", "javascript"=>" onClick=advanced('sim')") :
							array("required" => true,"value" => $mnc, "comment" => "Set Mobile Network Code.");

	if (!$generate_random_imsi) {
		unset($fields["imsi"]["triggered_by"]);
		unset($fields["iccid"]["triggered_by"]);
		unset($fields["ki"]["triggered_by"]);
		unset($fields["opc"]["triggered_by"]);
	}

	error_handle($error,$fields,$error_fields);
	start_form(NULL,"post",false,"outbound");
	addHidden("to_pysim");
	editObject(NULL,$fields,"Set SIM data for writting","Save");
	end_form();
}

function write_sim_form_to_pysim()
{
	global $yate_conf_dir;

	$error = "";
	$params = array("operator_name","country_code","mobile_country_code","mobile_network_code", "card_type");
	foreach ($params as $key => $param) {
		if (!getparam($param)) {
			$error .= "This parameter '". ucfirst(str_replace("_"," ",$param)). "' cannot be empty!<br/>\n";
			$error_fields[] = $param;
		} elseif ($param != "operator_name" && $param != "card_type" && !ctype_digit(getparam($param))) {
			$error .= "Invalid integer value for parameter '". ucfirst(str_replace("_"," ",$param)). "': ". getparam($param). ".<br/> \n";
			$error_fields[] = $param;
		} elseif ($param == "mobile_country_code" && (int)getparam($param) <= 0 || (int)getparam($param) >= 999) {

			$error .= "Mobile Country Code value must be between 0 and 999. <br/>\n";
			$error_fields[] = $param;
		} elseif ($param == "mobile_network_code" && (int)getparam($param) <= 0 ||  (int)getparam($param) >= 999) {
			$error .= "Mobile Network Code value must be between 0 and 999. <br/>\n";
			$error_fields[] = $param;	
		} else		
			$data[$param] = getparam($param);
	}

	$change_command = false;	
	if (getparam("generate_random_imsi") != "on")
	       	$change_command = true;

	if ($change_command) {
		//validation on fields
		$params = array("imsi", "iccid", "ki", "opc");
		foreach ($params as $key => $param) {
			if (!getparam($param)) {
				$error .= "This parameter '".strtoupper($param). "' cannot be empty!<br/>\n";
				$error_fields[] = $param;
			} elseif ($param == "imsi") {
				$imsi = getparam($param);
			        if (!ctype_digit($imsi) || ( strlen($imsi) != 15 && strlen($imsi) != 14))
					$error .= "IMSI: $imsi is not valid, must contain 14 or 15 digits. <br/>\n";
				if (test_existing_imsi_in_csv($imsi))
					$error .= "This IMSI: $imsi is already written on another SIM card.<br/>\n";
				$error_fields[] = $param;
			} elseif (($param == "opc" || $param == "ki") && !preg_match('/^[0-9a-fA-F]{32}$/i', getparam($param))) {
				$name = $param == "opc" ? "OPC" : "Ki";
				 $error .= $name . ": ".getparam($param)." needs to be 128 bits, in hex format.<br/>\n";
				 $error_fields[] = $param;
			} elseif ($param == "iccid" && !ctype_digit(getparam($param)) && strlen(getparam($param)) != 19) {
				$error .= "ICCID: ". getparam($param) ." must contain 19 digits!<br/>\n";
				$error_fields[] = $param;
			}  
			$data[$param] = getparam($param);

		}
	}
	if (!strlen($error))
		$output = execute_pysim($data, $change_command);
	else
		return write_sim_form($error, $error_fields, getparam("generate_random_imsi"),getparam("insert_subscribers"));

	if ($output)
		print "<pre>".$output."</pre>";

	//for successfully written SIM cards, write tha last one into subscribers file 
	if (substr(trim($output),-6,6) == "Done !" && getparam("insert_subscribers") == "on") {
		$all_sim_written = read_csv();
		write_generated_imsi_to_file($all_sim_written[count($all_sim_written)-1]);
	}

	manage_sims();
}

function write_imsi_in_subscribers()
{
	$imsi = getparam("imsi");
	$ki = getparam("ki");

	if (!$imsi && !$ki)
		return;

	$subscribers = array("imsi"=> $imsi , "ki"=> $ki);
	write_generated_imsi_to_file($subscribers);

	manage_sims();
}

function write_generated_imsi_to_file($subscribers)
{
	$res = get_subscriber($subscribers["imsi"]);
	if ($res[0])
		return;

	unset($subscribers["iccid"], $subscribers["operator_name"],$subscribers["country_code"],$subscribers["mobile_country_code"],$subscribers["mobile_network_code"]);
	$res = get_subscribers();
	if ($res[0])
		$new = $res[1];	
	
	$new[$subscribers["imsi"]] = $subscribers; 
	$new[$subscribers["imsi"]] = array("imsi"=>$subscribers["imsi"],"msisdn"=> "","short_number"=>"","active"=>"0","ki"=>$subscribers["ki"],"op"=>"","imsi_type"=>"2G");

	$res = set_subscribers($new);

	if (!$res[0])
		return errormess($res[1],"no");
}

function execute_pysim($params, $command_manually=false)
{
	global $pysim_path, $pysim_csv;

	if (!isset($_SESSION["card_num"]))
	       	$_SESSION["card_num"] = 0;
	$random_string = generateToken(5);
	/**
	 * usage: Run pySIM with some minimal set params:
	 * ./pySim-prog.py -n 26C3 -c 49 -x 262 -y 42 -z <random_string_of_choice> -j <card_num>
	 *
	 *   With <random_string_of_choice> and <card_num>, the soft will generate
	 *   'predictable' IMSI and ICCID, so make sure you choose them so as not to
	 *   conflict with anyone. (for eg. your name as <random_string_of_choice> and
	 *   0 1 2 ... for <card num>).
	 */

	$pysim_installed = detect_pysim_installed();

        if (!$pysim_installed[0]) {
                errornote($pysim_installed[1]);
                return;
        }

	$params_sim_data = "-e -n ".$params["operator_name"]." -c ".$params["country_code"]." -x ".$params["mobile_country_code"]." -y ".$params["mobile_network_code"]." -z $random_string  -j ". $_SESSION["card_num"];

	$command = 'stdbuf -o0 ' . $pysim_path.'/'.'pySim-prog.py -p 0 -t '. $params["card_type"]." ".$params_sim_data. " --write-csv ".$pysim_csv ;

	/**
	 * Or if command manually is set then run pySIM with every parameter specified manually:
	 * E.g.:  ./pySim-prog.py -n 26C3 -c 49 -x 262 -y 42 -i <IMSI> -s <ICCID>
	 */ 
	if ($command_manually)
		$command = 'stdbuf -o0 ' . $pysim_path.'/'.'pySim-prog.py -e -p 0 -t '. $params["card_type"]. " -n ".$params["operator_name"]."  --write-csv ".$pysim_csv." -i ". $params["imsi"]. " -s ".$params["iccid"]. " -o ". $params["opc"]. " -k ". $params["ki"];

	$descriptorspec = array(
		0 => array("pipe","r"),// stdin is a pipe that the child will read from
		1 => array("pipe","w"),//stdout
		2 => array("pipe","w") //stderr
	) ;

	// define current working directory where files would be stored
	// open process and pass it an argument
	$process = proc_open($command, $descriptorspec, $pipes);
	
	if (is_resource($process)) {
		// $pipes now looks like this:
		// 0 => writeable handle connected to child stdin
		// 1 => readable handle connected to child stdout
		// Any error output will be appended to $yate_conf_dir."pysim-error.log"
		$output = false;
		do {
			$read = array($pipes[1]);$write=array();$except=array();
			if (!stream_select($read, $write ,$except,3)) {
			//if card was not inserted in the Reader or the time expired
		                fclose($pipes[1]);
		                proc_terminate($process);
				proc_close($process);
		                print "Card was not found in SIM card reader... Terminating program...";
		                return;
			}
			$return_message = fread($pipes[1], 1024);
			$output .= $return_message; 
		} while(!empty($return_message));
	}

	if ($err = stream_get_contents($pipes[2])) {
		$split_errs = explode("\n", rtrim($err));
		$output .= "<div style=\"display:none;\" id=\"pysim_err\">";
		$i = 1;
		foreach ($split_errs as $key => $split_err) {
			if ($i == count($split_errs))
				break;
			$output .= $split_err."\n";
			$i++;
		}

		$output .= "\n</div>";
		$output .= $split_errs[count($split_errs)-1];
		if (preg_match("/Exception AttributeError: \"'PcscSimLink' object has no attribute '_con/", $output))
			$output .= "\nPlease connect you SIM card reader to your device.\n"; 
		$output .= "\n<br/><div id=\"err_pysim\">For full pySim traceback <div id=\"err_link_pysim\" class=\"error_link\" onclick=\"show_hide_pysim_traceback('pysim_err')\" style=\"cursor:pointer;\">click here</div></div>";
	} 

	proc_close($process);
	
	$_SESSION["card_num"] += 1;
	return $output;
}

// $length - the length of the random string
// returns random string with mixed numbers, upperletters and lowerletters
function generateToken($length)
{
	//0-9 numbers,10-35 upperletters, 36-61 lowerletters
	$str = "";
	for ($i=0; $i<$length; $i++) {
		$c = mt_rand(0,61);
		if ($c >= 36) {
			$str .= chr($c+61);
			continue;
		}
		if ($c >= 10) {
			$str .= chr($c+55);
			continue;
		}
		$str .= chr($c+48);
	}
	return $str;
}

function read_csv()
{
	global $yate_conf_dir;

	$sim_data = array();
	$filename = $yate_conf_dir.'sim_data.csv';
	if (!file_exists($filename))
		return $sim_data;

	$handle = fopen($filename, "r");
	$content = fread($handle,filesize($filename));
	$content = eregi_replace('"','',$content); // in case they choose to mark text fields with "
	$content = eregi_replace("'",'',$content);  // in case they choose to mark text fields with '
	$content = explode("\n",$content);


	for ($i=0; $i<count($content); $i++) {
		$row = explode(',',$content[$i]);
		if (!is_array($row) || count($row) < 8)
			continue;
		// the order in csv file: name,iccid,mcc,mnc,imsi,smsp,ki,opc
		$sim_data[$i]["operator_name"] = $row[0];
		$sim_data[$i]["iccid"] = $row[1];
		$sim_data[$i]["mobile_country_code"] = $row[2];
		$sim_data[$i]["mobile_network_code"] = $row[3];
		$sim_data[$i]["imsi"] = $row[4];
		// $sim_data[$i]["smsp"] = $row[5];
		$sim_data[$i]["ki"] = $row[6];
		$sim_data[$i]["opc"] = $row[7];
	}
	return $sim_data;
}

function test_existing_imsi_in_csv($imsi)
{
	$sim_data = read_csv();

	for ($i=0; $i<count($sim_data); $i++) 
		$sim_imsis[] = $sim_data[$i]["imsi"];

	if (in_array($imsi, $sim_imsis))
		return true;

	return false;
}

function get_country_code()
{
	return null;
}
?>
