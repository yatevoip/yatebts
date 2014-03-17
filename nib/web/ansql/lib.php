<?php
/**
 * lib.php
 * This file is part of the FreeSentral Project http://freesentral.com
 *
 * FreeSentral - is a Web Graphical User Interface for easy configuration of the Yate PBX software
 * Copyright (C) 2008-2014 Null Team
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



global $module, $method, $action, $vm_base, $limit, $db_true, $db_false, $limit, $page, $system_standard_timezone;

function include_classes($path='')
{
	$classes_dirs = array("classes/", "ansql/default_classes");

	for ($i=0; $i<count($classes_dirs); $i++) {
		if (!is_dir($path.$classes_dirs[$i]))
			continue;
		$handle = opendir($path.$classes_dirs[$i]);

		while (false !== ($file = readdir($handle))) {
			if (substr($file,-4) != '.php')
				continue;
			else {
				if ($classes_dirs[$i] == "ansql/default_classes") {
					$file_name = substr($file,0,strlen($file)-4);
					global ${"custom_$file_name"};
					if (isset(${"custom_$file_name"}) && ${"custom_$file_name"})
						continue;
				}
				require_once($path.$classes_dirs[$i]."/$file");
			}
		}
	}
}
if (!function_exists("stripos")) {
	// PHP 4 does not define stripos
	function stripos($haystack,$needle,$offset=0)
	{
		return strpos(strtolower($haystack),strtolower($needle),$offset);
	}
}

escape_page_params();

if (!isset($system_standard_timezone))
	$system_standard_timezone = "GMT".substr(date("O"),0,3);

function get_default_function()
{
	global $module, $method, $action;

	if(!$method)
		$method = $module;
	if(substr($method,0,4) == "add_")
		$method = str_replace("add_","edit_",$method);
	if($action)
		$call = $method.'_'.$action;
	else
		$call = $method;
	return $call;
}

function testpath($path)
{
	if (ereg("[^A-Za-z0-9_]",$path)) 
	{
		// Client tried to hack around the path naming rules - ALERT!
print "by here ereg matched $path : ".ereg("[^A-Za-z0-9_]",$path);
		forbidden();
    }
}

function forbidden()
{
	header("403 Forbidden");
	session_unset();
	print '<html><body style="color:red">Forbidden</body></html>';
	exit();
}

function start_form($action = NULL, $method = "post", $allow_upload = false, $form_name = NULL, $class=null)
{
	global $module;

	if(!$method)
		$method = "post";
	$form = (!$module) ? "current_form" : $module;
	if(!$form_name)
		$form_name = $form;
	if(!$action) {
		if(isset($_SESSION["main"]))
			$action = $_SESSION["main"];
		else
			$action = "index.php";
	}

	?><form action="<?php print $action;?>" name="<?php print $form_name;?>" id="<?php print $form_name;?>" class="<?php print $class;?>" method="<?php print $method;?>" <?php if($allow_upload) print 'enctype="multipart/form-data"';?>><?php
}

function end_form()
{
	?></form><?php
}

function note($note)
{
	print 'Note!! '.$note.'<br/>';
}

function errornote($text)
{
	print "<br/><font color=\"red\" style=\"font-weight:bold;\" > Error!!</font> <font style=\"font-weight:bold;\">$text</font><br/>";
}

function message($text, $path=NULL)
{
	global $module,$method;

	print '<div class="notice">'."\n";
	print "$text\n";
	if($path != 'no')
		print '<a class="information" href="main.php?module='.$module.'&method='.$path.'">Go back to application</a>';
	print '</div>';
}

function errormess($text, $path=NULL)
{
	global $module;

	if(!$path)
		$path = '';
	print '<div class="notice">'."\n";
	print "<font class=\"error\"> Error!!</font>"."\n";
	print "<font style=\"font-weight:bold;\">$text</font>"."\n";
	if($path != 'no')
		print '<a class="information" href="main.php?module=' .$module.  '&method=' . $path . '">Go back to application</a>'."\n";
	print '</div>';
}

function notice($message, $next=NULL, $no_error = true)
{
	global $module;

	if(!$next)
		$next = $module;

	if($no_error)
		print '<div class="notice">'.$message.'</div>';
	else
		print '<div class="notice"><font class="error">Error!! </font>'.$message.'</div>';

	if($next != "no")
		$next();
}

function plainmessage($text)
{
	print "<br/><font style=\"font-weight:bold;\">$text</font><br/><br/>";
}

function notify($res)
{
	global $path;

	if($res[0])
		message($res[1],$path);
	else
		errormess($res[1],$path);
}

function escape_page_params()
{
	foreach ($_POST as $param=>$value)
		$_POST[$param] =  escape_page_param($value);
	foreach ($_GET as $param=>$value)
		$_GET[$param] = escape_page_param($value);
	foreach ($_REQUEST as $param=>$value)
		$_REQUEST[$param] = escape_page_param($value);
}

function escape_page_param($value)
{
	if (!is_array($value))
		return htmlentities($value);
	else  {
		foreach ($value as $index=>$val)
			$value[$index] = htmlentities($val);
		return $value;
	}
}

function getparam($param,$escape = true)
{
	$ret = NULL;
	if (isset($_POST[$param]))
		$ret = $_POST[$param];
	else if (isset($_GET[$param]))
		$ret = $_GET[$param];
	else
		return NULL;
	if(is_array($ret)) {
		foreach($ret as $index=>$value) {
			if (substr($ret[$index],0,6) == "__sql_")
				$ret[$index] = NULL; 
			if ($ret[$index] == "__empty")
				$ret[$index] = NULL;
			if ($ret[$index] == "__non_empty" || $ret[$index] == "__not_empty")
				$ret[$index] = NULL;
			if (substr($ret[$index],0,6) == "__LIKE")
				$ret[$index] = NULL;
			if (substr($ret[$index],0,10) == "__NOT LIKE")
				$ret = NULL;
		}
		return $ret;
	}
	if (substr($ret,0,6) == "__sql_")
		$ret = NULL; 
	if ($ret == "__empty")
		$ret = NULL;
	if ($ret == "__non_empty" || $ret == "__not_empty")
		$ret = NULL;
	if (substr($ret,0,6) == "__LIKE")
		$ret = NULL;
	if (substr($ret,0,10) == "__NOT LIKE")
		$ret = NULL;
	return $ret;
}

function killspaces($value)
{
	return str_replace(' ','_',$value);
}

function Numerify($num, $very_big = false)
{
	if ($num == '0') 
		$num = '0';
	if($very_big) {
		for($i=0; $i<strlen($num); $i++) {
			if(!is_numeric($num[$i]))
				return "NULL";
		}
	}else
		if (!is_numeric($num) && strlen($num)) 
			$num = "NULL";
	return $num;
}

// Build a full date string from parts, return false on failure, true on empty
function dateCheck($year,$month,$day,$hour,$end)
{
	if ("$year$month$day" == "") {
		if ($hour == "")
	    	return true;
		if (($hour<0) || ($hour>23))
	    	return false;
		$hour=sprintf(" %02u:%02u:%02u",$hour,$end,$end);
		return gmdate("Y-m-d") . $hour;
    }
	if (!($year && $month && $day))
		return false;
	if ($hour == "")
		$hour=$end ? 23 : 0;
	if (!(is_numeric($year) && is_numeric($month) && is_numeric($day) && is_numeric($hour)))
		return false;
	if (($year<2000) || ($month<1) || ($month>12) || ($day<1) || ($day>31) || ($hour<0) || ($hour>23))
		return false;
	return sprintf("%04u-%02u-%02u %02u:%02u:%02u",$year,$month,$day,$hour,$end,$end);
}

function items_on_page($nrs = array(20,50,100))
{
	global $module, $method, $action, $limit;

	$link = $_SESSION["main"] ? $_SESSION["main"] : "main.php";
	$link .= "?";
	foreach($_REQUEST as $param=>$value)
	{
		if($param=="page" || $param=="PHPSESSID" || ($param=="action" && $action) || ($param=="method" && $method) || ($param=="module" && $module) || $param == "limit" || !strlen($value))
			continue;
		if (substr($link,-1)!="?")
			$link .= "&";
		$link .= "$param=".urlencode($value);
	}
	if ($module)
		$link .= "&module=$module";
	if ($method)
		$link .= "&method=$method";
	if ($action) {
		$call = get_default_function();
		if (function_exists($call))
			$link .= "&action=$action";
	}

	print "<div class=\"items_on_page\">";
	for($i=0; $i<count($nrs); $i++)
	{
		$option = $link."&limit=".$nrs[$i];
		if ($i>0)
			print '|';
		print '&nbsp;<a class="pagelink';
		if ($nrs[$i]==$limit)
			print " selected_pagelink";
		print '" href="'.$option.'">'.$nrs[$i].'</a>&nbsp;';
	}
	print "</div>";
}

function pages($total = NULL, $params = array())
{
	global $limit, $page, $module, $method, $action;
	if(!$limit)
		$limit = 20;

	$link = $_SESSION["main"] ? $_SESSION["main"] : "main.php";
	$link .= "?";
	$slink = $link;
	$found_total = false;
	$page = 0;
	foreach($_REQUEST as $param=>$value)
	{
		if(($param=="action" && $action) || ($param=="module" && $module) || ($param=="method" && $method) || $param=="PHPSESSID" || !strlen($value))
			continue;
		if($param=="page")
		{
			$page = $value;
			continue;
		}
		if($link != $slink)
			$link .= "&";
		$link .= "$param=".urlencode($value);
		if($param == "total")
		{
			$total = $value;
			$found_total = true;
		}
	}

	if(!$total)
		$total = 0;
	if($total < $limit)
		return;

	if(!$found_total)
		$link .= "&total=$total";
	if($module)
		$link .= "&module=$module";
	if($method)
		$link .= "&method=$method";
	if ($action) {
		/* if action param is set, check that $method_$action function exists before adding it to link */
		$function_to_call = get_default_function();
		if (function_exists($function_to_call))
			$link .= "&action=$action";
	}

	$pages = floor($total/$limit);
	print '<center>';
	print '<div class="pages">';
	if($page != 0)
	{
		/* jump to first page */
		print '<a class="pagelink" href="'.$link.'&page=0">|<</a>&nbsp;&nbsp;';

		/* jump back 5 pages */
		$prev5 = $page - 5*$limit;
		if ($prev5>0)
			print '<a class="pagelink" href="'.$link.'&page='.$prev5.'"><<</a>&nbsp;&nbsp;';

		/* jump to previous page */
		/*$prev_page = $page - $limit;
		print '<a class="pagelink" href="'.$link.'&page='.$prev_page.'"><</a>&nbsp;&nbsp;';*/

		$diff = floor(($total - ($page + $limit * 2))/$limit) * $limit;
		$sp = $page - $limit * 2;
		if($diff < 0){
			$sp = $sp - abs($diff);
		}

		while($sp<0)
			$sp += $limit;
		while($sp<$page)
		{
			$pg_nr = $sp/$limit + 1;
			print '<a class="pagelink" href="'.$link.'&page='.$sp.'">'.$pg_nr.'</a>&nbsp;&nbsp;';
			$sp += $limit;
		}
	}
	$pg_nr = $page/$limit + 1;
	print '<font class="pagelink selected_pagelink" href="#">'.$pg_nr.'</font>&nbsp;&nbsp;';
	if(($page+$limit)<=$total)
	{
		if($pg_nr>=3)
			$stop_at = $pg_nr + 2;
		else
			$stop_at = $pg_nr + 5 - (floor($page/$limit)+1);

		$next_page = $page + $limit;
		while($next_page<$total && $pg_nr<$stop_at)
		{
			$pg_nr++;
			print '<a class="pagelink" href="'.$link.'&page='.$next_page.'">'.$pg_nr.'</a>&nbsp;&nbsp;';
			$next_page += $limit;
		}

		/* jump to next page */
		/*$next_page = $page + $limit;
		if($next_page<$total)
			print '<a class="pagelink" href="'.$link.'&page='.$next_page.'">></a>&nbsp;&nbsp;';*/

		$next5 = $page + $limit*5;
		$last_page = floor($total/$limit) * $limit;

		/* jump 5 pages */
		if ($next5<$last_page)
			print '<a class="pagelink" href="'.$link.'&page='.$next5.'">>></a>&nbsp;&nbsp;';

		/* jump to last page */
		print '<a class="pagelink" href="'.$link.'&page='.$last_page.'">>|</a>&nbsp;&nbsp;';
	}
	print '</div>';
	print '</center>';
}

function navbuttons($params=array(),$class = "llink")
{
	global $module, $method, $page;

	$step = '';
	$link="main.php?module=$module&method=$method&";
	foreach($params as $key => $value)
	{
		if ($key=="page" || $key=="tot")
			continue;
		$link="$link$key=$value&";
		if ($key == "step")
			$step = $value;
	}
	$total = $params["tot"];
	
	if(!$step || $step == '')
		$step = 10;
?>
	<center>
	<table border="0" cellspacing="0" cellpadding="0">
		<tr>
			<td class="navbuttons">
				<?php $vl=$page-$step;
				if ($vl>=0)
				{ ?>
					<font size="-1"><a class="<?php print $class;?>" href="<?php print ("$link"."page"."=$vl");?>">Previous</a>&nbsp;&nbsp;</font>
				<?php } ?>
			</td>
			<td class="navbuttons">
				<font size="-3">
				<?php
				$r=$page/$step+1;
 				print ("$r");
				?>
				</font>
			</td>
			<td class="navbuttons">
			    <?php
			    $vl=$page+$step;
			    if ($vl<$total) { ?>
				&nbsp;&nbsp;<font size="-1"><a class="<?php print $class;?>" href="<?php print ("$link"."page"."=$vl");?>">Next</a> </font><?php
				} ?>
			</td>
		</tr>
	</table>
	</center>
<?php
}

function check_valid_mail($mail)
{
	if(!$mail)
		return true;

	$pattern = '^([_a-z0-9-]+)(\.[_a-z0-9-]+)*@([a-z0-9-]+)(\.[a-z0-9-]+)*(\.[a-z]{2,4})$' ;
	return eregi($pattern, $mail);
}

function addHidden($action=NULL, $additional = array(), $empty_page_params=false)
{
	global $method,$module;

	if (($method || $empty_page_params) && !isset($additional["method"]))
		print "<input type=\"hidden\" name=\"method\" id=\"method\" value=\"$method\" />\n";

	if (is_array($module) && !isset($additional["module"]))
		print "<input type=\"hidden\" name=\"module\" id=\"module\" value=\"$module[0]\" />\n";
	elseif (($module || $empty_page_params) && !isset($additional["module"]))
		print "<input type=\"hidden\" name=\"module\" id=\"module\" value=\"$module\" />\n";

	print "<input type=\"hidden\" name=\"action\" id=\"action\" value=\"$action\" />\n";

	if(count($additional))
		foreach($additional as $key=>$value) 
			print '<input type="hidden" id="' . $key . '" name="' . $key . '" value="' . $value . '">';

	if (isset($_SESSION["previous_page"])) {
		foreach ($_SESSION["previous_page"] as $param=>$value)
			if (!isset($additional[$param]) && $param!="module" && $param!="method" && $param!="action")
				print '<input type="hidden" name="' . $param . '" value="' . $value . '">';
	}
}

/**
 * Creates a form for editing an object
 * @param $object Object that will be edited or NULL if fields don't belong to an object
 * @param $fields Array of type field_name=>field_formats
 * Ex: $fields =  array(
		"username"=>array("display"=>"fixed", "compulsory"=>true), 
		// if index 0 in the array is not set then this field will correspond to variable username of @ref $object
		// the field will be marked with a *(compulsory)
		"description"=>array("display"=>"textarea", "comment"=>"short description"), 
		// "comment" is used for inserting a comment under the html element 
		"password"=>array("display"=>"password", "compulsory"=>"yes"), 
		"birthday"=>array("date", "display"=>"include_date"), 
		// will call function include_date
		"category"=>array($categories, "display"=>"select") 
		// $categories is an array like 
		// $categories = array(array("category_id"=>"4", "category"=>"Nature"), array("category_id"=>"5", "category"=>"Movies")); when select category 'Nature' $_POST["category"] will be 4
		// or $categories = array("Nature", "Movies");
		"sex"=>array($sex, "display"=>"radio") 
		// $sex = array("male","female","don't want to answer");
	); 
 * instead of "compulsory", "requited" can be also used
 * possible values for "display" are "textarea", "password", "fileselect", "text", "select", "radio", "radios", "checkbox", "fixed"
 * If not specified display is "text"
 * If the field corresponds to a bool field in the object given display is ignored and display is set to "checkbox"
 * @param $title Text representing the title of the form
 * @param $submit Text representing the value of the submit button or Array of values that will appear as more submit buttons
 * @param $compulsory_notice Bool true for using default notice, Text representing a notice that will be printed under the form if other notice is desired or NULL or false for no notice
 * @param $no_reset When set to true the reset button won't be displayed, Default value is false
 * @param $css Name of the css to be used when generating the elements. Default value is 'edit'
 * @param $form_identifier Text. Used to make the current fields unique(Used when this function is called more than once inside the same form with fields that can have the same name when being displayed)
 * @param $td_width Array or by default NULL. If Array("left"=>$value_left, "right"=>$value_right), force the widths to the ones provided. $value_left could be 20px or 20%.
 * @param $hide_advanced Bool default false. When true advanced fields will be always hidden when displaying form
 */
function editObject($object, $fields, $title, $submit="Submit", $compulsory_notice=NULL, $no_reset=false, $css=NULL, $form_identifier='', $td_width=NULL, $hide_advanced=false)
{
	if(!$css)
		$css = "edit";

	print '<table class="'.$css.'" cellspacing="0" cellpadding="0">';
	if($title) {
		print '<tr class="'.$css.'">';
		print '<th class="'.$css.'" colspan="2">'.$title.'</th>';
		print '</tr>';
	}

	$show_advanced = false;
	$have_advanced = false;
	//find if there are any fields marked as advanced that have a value(if so then all advanced fields should be displayed)
	foreach($fields as $field_name=>$field_format)
	{
		if(!isset($field_format["advanced"]))
			continue;
		if($field_format["advanced"] != true)
			continue;
		$have_advanced = true;
		if($object)
			$value = (!is_array($field_name) && isset($object->{$field_name})) ? $object->{$field_name} : NULL;
		else
			$value = NULL;
		if(isset($field_format["value"]))
			$value = $field_format["value"];
		if (!$object)
			break;
		$variable = $object->variable($field_name);
		if((!$variable && $value && !$hide_advanced))
		{
			$show_advanced = true;
			break;
		}
		if(!$variable)
			continue;
		if (($value && $variable->_type != "bool" && !$hide_advanced) || ($variable->_type == "bool" && $value == "t" && !$hide_advanced))
		{
			$show_advanced = true;
			break;
		}
	}
	foreach($fields as $field_name=>$field_format)
		display_pair($field_name, $field_format, $object, $form_identifier, $css, $show_advanced, $td_width);

	if($have_advanced && !$compulsory_notice)
	{
		print '<tr class="'.$css.'">';
		print '<td class="'.$css.' left_td advanced">&nbsp;</th>';
		print '<td class="'.$css.' left_right advanced"><img id="'.$form_identifier.'xadvanced"';
		if(!$show_advanced)
			print " src=\"images/advanced.jpg\" title=\"Show advanced fields\"";
		else
			print " src=\"images/basic.jpg\" title=\"Hide advanced fields\"";
		print ' onClick="advanced(\''.$form_identifier.'\');"/></th></tr>';
	}
	if($compulsory_notice && $compulsory_notice !== true)
	{
		if($have_advanced) {
		print '<tr class="'.$css.'">';
		print '<td class="'.$css.' left_td" colspan="2">';
		print '<img class="advanced" id="'.$form_identifier.'advanced" ';
		if(!$show_advanced)
			print "src=\"images/advanced.jpg\" title=\"Show advanced fields\"";
		else
			print "src=\"images/basic.jpg\" title=\"Hide advanced fields\"";
		print ' onClick="advanced(\''.$form_identifier.'\');"/>'.$compulsory_notice.'</td>';
		print '</tr>';
		}
	}elseif($compulsory_notice === true){
		print '<tr class="'.$css.'">';
		print '<td class="'.$css.' left_td" colspan="2">';
		if($have_advanced) {
		print '<img id="'.$form_identifier.'xadvanced"';
		if(!$show_advanced)
			print " class=\"advanced\" src=\"images/advanced.jpg\" title=\"Show advanced fields\"";
		else
			print " class=\"advanced\" src=\"images/basic.jpg\" title=\"Hide advanced fields\"";
		print ' onClick="advanced(\''.$form_identifier.'\');"/>';
		}
		print 'Fields marked with <font class="compulsory">*</font> are required.</td>';
		print '</tr>';
	}
	if($submit != "no" && $submit != "no_submit")
	{
		print '<tr class="'.$css.'">';
		print '<td class="'.$css.' trailer" colspan="2">';
		if(is_array($submit))
		{
			for($i=0; $i<count($submit); $i++)
			{
				print '&nbsp;&nbsp;';
				print '<input class="'.$css.'" type="submit" name="'.$submit[$i].'" value="'.$submit[$i].'"/>';
			}
		}else
			print '<input class="'.$css.'" type="submit" name="'.$submit.'" value="'.$submit.'"/>';
		if(!$no_reset) {
			print '&nbsp;&nbsp;<input class="'.$css.'" type="reset" value="Reset"/>';
			$cancel_but = cancel_button($css);
			if ($cancel_but)
				print "&nbsp;&nbsp;$cancel_but";
		}
		print '</td>';
		print '</tr>';
	}
	print '</table>';
}

function cancel_button($css="", $name="Cancel")
{
	$res = null;
	if (isset($_SESSION["previous_page"])) {
		$link = $_SESSION["main"]."?";
		foreach ($_SESSION["previous_page"] as $param=>$value)
			$link.= "$param=".urlencode($value)."&";
		$res = '<input class="'.$css.'" type="button" value="'.$name.'" onClick="location.href=\''.$link.'\'"/>';
	}
	return $res;
}

function cancel_params()
{
	$link = "";
	foreach ($_SESSION["previous_page"] as $param=>$value)
		$link.= "$param=".urlencode($value)."&";
	return $link;
}

function display_pair($field_name, $field_format, $object, $form_identifier, $css, $show_advanced, $td_width)
{
	$q_mark = false;
	if(isset($field_format["advanced"]))
		$have_advanced = true;

	if(isset($field_format["triggered_by"]))
		$needs_trigger = true;

	if($object)
		$value = (!is_array($field_name) && isset($object->{$field_name})) ? $object->{$field_name} : NULL;
	else
		$value = NULL;
	if(isset($field_format["value"]))
		$value = $field_format["value"];

	print '<tr id="tr_'.$form_identifier.$field_name.'"';
//		if($needs_trigger == true)	
//			print 'name="'.$form_identifier.$field_name.'triggered'.$field_format["triggered_by"].'"';

	if(isset($field_format["error"]) && $field_format["error"]===true)
		$css .= " error_field";
	print ' class="'.$css.'"';
	if(isset($field_format["advanced"]))
	{
		if(!$show_advanced)
			print ' style="display:none;"';
		elseif(isset($field_format["triggered_by"])){
			if($needs_trigger)
				print ' style="display:none;"';
			else
				print ' style="display:table-row;"';
		}else
			print ' style="display:table-row;"';
	}elseif(isset($field_format["triggered_by"])){
		if($needs_trigger)
			print ' style="display:none;"';
		else
			print ' style="display:table-row;"';
	}
	print '>';
	// if $var_name is an array we won't use it
	$var_name = (isset($field_format[0])) ? $field_format[0] : $field_name;
	$display = (isset($field_format["display"])) ? $field_format["display"] : "text";

	if($object)
	{
		$variable = (!is_array($var_name)) ? $object->variable($var_name) : NULL;
		if($variable)
			if($variable->_type == "bool" && $display!="text")
				$display = "checkbox";
	}

	if($display == "message") {
		print '<td class="'.$css.' double_column" colspan="2">';
		print $value;
		print '</td>';
		print '</tr>';
		return;
	}

	if($display != "hidden") {
		print '<td class="'.$css.' left_td ';
		if (isset($field_format["custom_css_left"]))
			print $field_format["custom_css_left"];
		print '"';
		if(isset($td_width["left"]))
			print ' style="width:'.$td_width["left"].'"';
		print '>';
		if(!isset($field_format["column_name"]))
			print ucfirst(str_replace("_","&nbsp;",$field_name));
		else
			print ucfirst($field_format["column_name"]);
		if(isset($field_format["required"]))
			$field_format["compulsory"] = $field_format["required"];
		if(isset($field_format["compulsory"]))
			if($field_format["compulsory"] === true || $field_format["compulsory"] == "yes" || $field_format["compulsory"] == "t" || $field_format["compulsory"] == "true")
				print '<font class="compulsory">*</font>';
		print '&nbsp;</td>';
		print '<td class="'.$css.' right_td ';
		if (isset($field_format["custom_css_right"]))
			print $field_format["custom_css_right"];
		print '"';
		if(isset($td_width["right"]))
			print ' style="width:'.$td_width["right"].'"';
		print '>';
	}
	switch($display)
	{
		case "textarea":
			print '<textarea class="'.$css.'" name="'.$form_identifier.$field_name.'" cols="20" rows="5">';
			print $value;
			print '</textarea>';
			break;
		case "select":
		case "mul_select":
		case "select_without_non_selected":
			print '<select class="'.$css.'" name="'.$form_identifier.$field_name.'" id="'.$form_identifier.$field_name.'" ';
			if(isset($field_format["javascript"]))
				print $field_format["javascript"];
			if($display == "mul_select")
				print ' multiple="multiple" size="5"';
			print '>';
			if($display != "mul_select" && $display != "select_without_non_selected")
				print '<option value="">Not selected</option>';
			$options = (is_array($var_name)) ? $var_name : array();
			if(isset($options["selected"]))
				$selected = $options["selected"];
			elseif(isset($options["SELECTED"]))
				$selected = $options["SELECTED"];
			else
				$selected = '';
			foreach ($options as $var=>$opt) {
				if ($var === "selected" || $var === "SELECTED")
					continue;
				$css = (is_array($opt) && isset($opt["css"])) ? 'class="'.$opt["css"].'"' : "";
				if(is_array($opt) && isset($opt[$field_name.'_id'])) {
					$optval = $field_name.'_id';
					$name = $field_name;

					$printed = trim($opt[$name]);
					if (substr($printed,0,4) == "<img") {
						$expl = explode(">",$printed);
						$printed = $expl[1];
						$jquery_title = " title=\"".str_replace("<img","",$expl[0])."\"";
					} else
						$jquery_title = '';

					if ($opt[$optval] === $selected || (is_array($selected) && in_array($opt[$optval],$selected))) {
						print '<option value=\''.$opt[$optval].'\' '.$css.' SELECTED ';
						if($opt[$optval] == "__disabled")
							print ' disabled="disabled"';
						print $jquery_title;
						print '>' . $printed . '</option>';
					} else {
						print '<option value=\''.$opt[$optval].'\' '.$css;
						if($opt[$optval] == "__disabled")
							print ' disabled="disabled"';
						print $jquery_title;
						print '>' . $printed . '</option>';
					}
				}else{
					if ($opt == $selected ||  (is_array($selected) && in_array($opt,$selected)))
						print '<option '.$css.' SELECTED >' . $opt . '</option>';
					else
						print '<option '.$css.'>' . $opt . '</option>';
				}
			}
			print '</select>';
			break;
		case "radios":
		case "radio":
			$options = (is_array($var_name)) ? $var_name : array();
			if(isset($options["selected"]))
				$selected = $options["selected"];
			elseif(isset($options["SELECTED"]))
				$selected = $options["SELECTED"];
			else
				$selected = "";
			foreach ($options as $var=>$opt) {
				if ($var === "selected" || $var === "SELECTED")
					continue;
				if(count($opt) == 2) {
					$optval = $field_name.'_id';
					$name = $field_name;
					$value = $opt[$optval];
					$name = $opt[$name];
				}else{
					$value = $opt;
					$name = $opt;
				}
				print '<input class="'.$css.'" type="radio" name="'.$form_identifier.$field_name.'" id="'.$form_identifier.$field_name.'" value=\''.$value.'\'';
				if ($value == $selected)
					print ' CHECKED ';
				if(isset($field_format["javascript"]))
					print $field_format["javascript"];
				print '>' . $name . '&nbsp;&nbsp;';
			}
			break;
		case "checkbox":
		case "checkbox-readonly":
			print '<input class="'.$css.'" type="checkbox" name="'.$form_identifier.$field_name.'" id="'.$form_identifier.$field_name.'"';
			if($value == "t" || $value == "on" || $value=="1")
				print " CHECKED ";
			if(isset($field_format["javascript"]))
				print $field_format["javascript"];
			if($display=="checkbox-readonly")
				print " disabled=''";
			print '/>';
			break;
		case "text":
		case "password":
		case "file":
		case "hidden":
		case "text-nonedit":
			print '<input class="'.$css.'" type="'.$display.'" name="'.$form_identifier.$field_name.'" id="'.$form_identifier.$field_name.'"';
			if($display != "file" && $display != "password")
				print ' value="'.$value.'"';
			if(isset($field_format["javascript"]))
				print $field_format["javascript"];
			if($display == "text-nonedit")
				print " readonly=''";
			if(isset($field_format["autocomplete"]))
				print " autocomplete=\"".$field_format["autocomplete"]."\"";
			if($display != "hidden" && isset($field_format["comment"])) {
				$q_mark = true;
				print '>&nbsp;&nbsp;<img class="pointer" src="images/question.jpg" onClick="show_hide_comment(\''.$form_identifier.$field_name.'\');"/>';;
			} else
				print '>';
			break;
		case "fixed":
			if(strlen($value))
				print $value;
			else
				print "&nbsp;";
			break;
		default:
			// need to do a callback here
			// it might be that we don't have the name seeted to that the javascript function that display the advanced settings could work 
			if(isset($field_format["advanced"]))
				print '<input type="hidden" name="'.$form_identifier.$field_name.'">';
			$value = $display($value,$form_identifier.$field_name); 
			if($value)
				print $value;
	}
	if($display != "hidden") {
		if(isset($field_format["comment"]))
		{
			$comment = $field_format["comment"];
			if(!$q_mark)
				print '&nbsp;&nbsp;<img class="pointer" src="images/question.jpg" onClick="show_hide_comment(\''.$form_identifier.$field_name.'\');"/>';
			print '<font class="comment" style="display:none;" id="comment_'.$form_identifier.$field_name.'">'.$comment.'</font>';
		}
		print '</td>';
	}
	print '</tr>';
}

function find_field_value($res, $line, $field)
{
	for($j=0; $j<pg_num_fields($res); $j++) {
		if (pg_field_name($res,$j) == $field)
				return pg_fetch_result($res,$line,$j);
	}

	return NULL;
}

function tree($array, $func = "copacClick",$class="copac",$title=NULL)
{
    global $module,$path;
	$i = 0;
	if (!isset($array[$i]) && count($array))
		while(!isset($array[$i]))
			$i++;
	if(count($array)){
		$num = count($array[$i]);
		$verify = array();
		for ($j=0;$j<$num;$j++)
			$verify[$j]="";
	}
	$level = 0;
	if(!$title)
		$title = $module;

	print '<div class="'.$class.'">';
	print '<div class="titlu">' . ucfirst($title). '</div>';
	print '<ul class="copac">';
	for ($i=0;$i<count($array);$i++) {
		$j = 0;
		foreach($array[$i] as $fld=>$val) {
	    	if ($val == "") {
				$j++;
				break;
			}
			if ($val == $verify[$j]) {
				$j++;
				continue;
			}
	    	for (;$level>$j;$level--)
				print "</ul>";
	    	if ($j > $level) {
				print "\n<ul class=\"copac_${level}\" id=\"copac_ul_${i}_${level}\" style=\"display:none\">";
				$level++;
			}
			$tip = ($j+1 < $num) ? "disc" : "square";
			print "\n<li class=\"copac_${j} \" id=\"copac_li_${i}_${j}\" type=\"${tip}\"><a class=\"copac\" href=\"#\" onClick=\"${func}(${i},${j},'${fld}'); return false\">$val</a></li>";
			$verify[$j]=$val;
			for ($k=$j+1;$k<$num;$k++)
				$verify[$k]="";
			$j++;
		}
	}
	for (;$level>0;$level--)
		print "</ul><br/>";
    print "\n</ul></div>";
}

function in_formats($field, $formats)
{
	foreach($formats as $key=>$value) {
		if (substr_count($key, $field))
			return array("key"=>$key, "value"=>$value);
	}

	return false;
}

function query_to_array($res, $formats=array())
{
	$begginings = array('1_', '2_' ,'3_' , '4_', '5_', '6_', '7_', '8_', '9_', '0_');

    $array = array();
    for($i=0; $i<pg_num_rows($res);$i++) {
        $array[$i] = array();
        for($j=0; $j<pg_num_fields($res); $j++) {
			$nm = pg_field_name($res,$j);
			$value = pg_fetch_result($res,$i,$j);
			if(isset($formats[$nm]) || in_formats($nm, $formats)) {
				$arr = in_formats($nm, $formats);

				$val = $arr["value"];
				$nm = $arr["key"];
				if(in_array(substr($nm,0,2), $begginings))
					$nm = substr($nm,2,strlen($nm));
				$save_nm = $nm;

				if(substr($val,0,9) == "function_") {
						$name = substr($val,9,strlen($val));
						$arr = explode(':',$name);

						if(count($arr)>1)
						{
							$nm = $arr[1];
							$name = $arr[0];
						}

						if(str_replace(',','',$save_nm) == $save_nm){
							$value = call_user_func($name,find_field_value($res,$i,$save_nm));
						}else
						{
							$save_nm = explode(',',$save_nm);
							$params = array();
							for($x=0; $x<count($save_nm); $x++)
								$params[trim($save_nm[$x])] = find_field_value($res, $i, trim($save_nm[$x]));
							$value = call_user_func_array($name,$params);
							$save_nm = implode(":",$save_nm);
						}
				}elseif($val)
					$nm = $val;
			}
            $array[$i][$nm] = $value;
        }
    }
    return $array;
}

function tableOfObjects_objectnames($objects, $formats, $object_name, $object_actions=array(), $general_actions=array(),$object_actions_names=array())
{
	tableOfObjects($objects, $formats, $object_name, $object_actions, $general_actions, NULL, false, "content", array(), $object_actions_names);
}

/**
 * Creates table of objects
 * @param $objects Array with the objects to be displayed
 * @param $formats Array with columns to be displayed in the table
 * Ex: array("username", "function_truncdate:registered_on"=>"date", "function_unifynames:name"=>"first_name,last_name")
 * will display a table having the column names : Username | Registered on | Name
 * "function_truncdate:registered_on"=>"date" means that on each variable named date 
 * for all objects function truncdate will be called and the result will be printed in the table under column Registered on
 * "function_unifynames:name"=>"first_name,last_name" means that under the Name column, for all objects 
 * function unifynames will be called with 2 parameters" the content of variables first_name and last_name from each object 
 * @param $object_name Name of the object to be displayed. It's important only when the number of objects to be printer is 0
 * @param $object_actions Array of $method=>$method_name, $method will be added in the link and $method_name will be printed
 * Ex: array("&method=edit_user"=>"Edit")
 * @param $general_actions Array of $method=>$method_name that will be printed at the end of the table
 * Ex: array("&method=add_user"=>"Add user")
 * @param $base Text representing the name of the page the links from @ref $object_name and @ref $general_actions will be sent
 * Ex: $base = "main.php"
 * If not sent, i will try to see if $_SESSION["main"] was set and create the link. If $_SESSION["main"] was not set then  
 * "main.php" is the default value 
 * @param $insert_checkboxes Bool value. If true then in front of each row a checkbox will be created. The name attribute
 * for it will be "check_".value of the id of the object printed at that row
 * Note!! This parameter is taken into account only if the objects have an id defined
 * @param $css Name of the css to use for this table. Default value is 'content'
 * @param $conditional_css Array ("css_name"=>$conditions) $css is the to be applied on certain rows in the table if the object corresponding to that row complies to the array of $conditions
 */
function tableOfObjects($objects, $formats, $object_name, $object_actions=array(), $general_actions=array(), $base = NULL, $insert_checkboxes = false, $css = "content", $conditional_css = array(), $object_actions_names=array(), $select_all = false)
{
	global $db_true, $db_false, $module;

	if(!$db_true)
		$db_true = "yes";
	if(!$db_false)
		$db_false = "no";

	if(!count($objects))
	{
		$plural = get_plural_form($object_name);
		print "<table class=\"$css\"><tr><td style=\"text-align:right;\">";
		plainmessage("There aren't any $plural in the database.");
		print "</td></tr>";
		if(!count($general_actions)){
			print '</table>';
			return;
		}
	}

	if(!$base)
	{
		$main = (isset($_SESSION["main"])) ? $_SESSION["main"] : "main.php";
		$base = "$main?module=$module";
	}

	$ths = "";
	print '<table class="'.$css.'" cellspacing="0" cellpadding="0">';

	if(count($objects))
	{
		$ths .= '<tr class="'.$css.'">';
		$no_columns = 0;

		if($insert_checkboxes)
		{
			$ths .= '<th class="'.$css.' first_th checkbox">';
			if ($select_all)
				$ths .= '<input type="checkbox" name="select_all" id="select_all" onclick="toggle_column(this);">';
			else
				$ths .= '&nbsp;';
			$ths .= '</th>';
			$no_columns++;
		}

		// print the name of the columns + add column for each action on object
		foreach($formats as $column_name => $var_name)
		{
			$exploded = explode(":",$column_name);
			if(count($exploded)>1)
				$name = $exploded[1];
			else{
				$name = $column_name;
				if(substr($column_name, 0, 9) == "function_")
					$name = substr($column_name,9);
				if(is_numeric($column_name))
					$name = $var_name;
			}
			$ucss = ($no_columns == 0) ? "$css first_th" : $css;
			$ths .= '<th class="'.$ucss.'">';
			$ths .= str_replace("_","&nbsp;",ucfirst($name));
			$ths .= '</th>';
			$no_columns++;
		}
		for($i=0; $i<count($object_actions); $i++)
		{
			$ths .= '<th class="'.$css.'">';
			if  (!isset($object_actions_names) || !isset($object_actions_names[$i]))
				$ths .= '&nbsp;';
			else
				$ths .= $object_actions_names[$i];
			$ths .= '</th>';
			$no_columns++;
		}
		$ths .= '</tr>';

		$vars = $objects[0]->extendedVariables();
		$class = get_class($objects[0]);
		$id_name = $objects[0]->getIdName();
	}else
		$no_columns = 2;

	if(count($general_actions) && in_array("__top",$general_actions))
		links_general_actions($general_actions, $no_columns, $css, $base, true);

	print $ths;

	for($i=0; $i<count($objects); $i++)
	{
		$cond_css = '';
		foreach($conditional_css as $css_name=>$conditions)
		{
			$add_css = true;
			foreach($conditions as $column=>$cond_column)
			{
				if($objects[$i]->{$column} != $cond_column)
				{
					$add_css = false;
					break;
				}
			}
			if($add_css)
				$cond_css .= " $css_name ";
		}
		if ($i==(count($objects)-1))
			$cond_css.= " pre_end"; 
		print '<tr class="'.$css.'">';
		if($insert_checkboxes && $id_name)
		{
			print '<td class="'.$css.$cond_css;
			if($i%2 == 0)
				print " evenrow";
			print '">';
			print '<input type="checkbox" name="check_'.$objects[$i]->{$id_name}.'"/>';
			print '</td>';
		}
		foreach($formats as $column_name=>$var_name)
		{
			print '<td class="'.$css.$cond_css;
			if($i%2 == 0)
				print " evenrow";
			print '">';
			$use_vars = explode(",", $var_name);
			array_walk($use_vars, 'trim_value');
			$exploded_col = explode(":", $column_name);
			$column_value = '';

			if(substr($exploded_col[0],0,9) == "function_") 
			{
				$function_name = substr($exploded_col[0],9,strlen($exploded_col[0]));
				if(count($use_vars)) 
				{
					$params = array();
					for($var_nr=0; $var_nr<count($use_vars); $var_nr++)
						if(array_key_exists($use_vars[$var_nr], $vars))
							array_push($params, $objects[$i]->{$use_vars[$var_nr]});
					$column_value = call_user_func_array($function_name,$params);
				}
			}elseif(isset($objects[$i]->{$var_name})){
				$column_value = $objects[$i]->{$var_name};
				$var = $objects[$i]->variable($use_vars[0]);
				if($var->_type == "bool")
				{
					if($column_value == "t")
						$column_value = $db_true;
					else
						$column_value = $db_false;
				}
			}
			if($column_value !== NULL)
				print $column_value;
			else
				print "&nbsp;";
			print '</td>';
		}
		$link = '';
		foreach($vars as $var_name => $var)
			if (strlen($objects[$i]->{$var_name}) < 30)
				$link .= "&$var_name=".htmlentities(urlencode($objects[$i]->{$var_name}));
		$link_no = 0;
		foreach($object_actions as $methd=>$methd_name)
		{
			print '<td class="'.$css.$cond_css;
			if($i%2 == 0)
				print ' evenrow object_action';
			else
				print ' object_action';
			print '">';
		//	if($link_no)
		//		print '&nbsp;&nbsp;';
			print '<a class="'.$css.'" href="'.$base.$methd.$link.'">'.$methd_name.'</a>';
			print '</td>';
			$link_no++;
		}
		print '</tr>';
	}
	if(count($general_actions) && !in_array("__top",$general_actions))
		links_general_actions($general_actions, $no_columns, $css, $base);

	print "</table>";
}

function links_general_actions($general_actions, $no_columns, $css, $base, $on_top=false)
{
	if (!count($general_actions))
		return;
    
	$pos_css = ($on_top) ? "starttable" : "endtable";

		print '<tr class="'.$css.' endtable">';
		if(isset($general_actions["left"]))
		{
			$left_actions = $general_actions["left"];
			$columns_left = floor($no_columns/2);
			$no_columns -= $columns_left;
			print '<td class="'.$css.' allleft '.$pos_css.'" colspan="'.$columns_left.'">';
			$link_no = 0;
			if (is_array($left_actions)) {
				foreach($left_actions as $methd=>$methd_name)
				{
					if($link_no)
						print '&nbsp;&nbsp;';
					print '<a class="'.$css.'" href="'.$base.$methd.'">'.$methd_name.'</a>';
					$link_no++;
				} 
			} else
				print $left_actions;
			print '</td>';
			if(isset($general_actions["right"]))
				$general_actions = $general_actions["right"];
			else
				$general_actions = array();
		}
		
		print '<td class="'.$css.' allright '.$pos_css.'" colspan="'.$no_columns.'">';
		$link_no = 0;
		if(!count($general_actions))
			print "&nbsp;";
		foreach($general_actions as $methd=>$methd_name)
		{
			if ($methd_name == "__top")
				continue;
			if($link_no)
				print '&nbsp;&nbsp;';
			print '<a class="'.$css.'" href="'.$base.$methd.'">'.$methd_name.'</a>';
			$link_no++;
		}
		print '</td>';
		print '</tr>';
}

function get_plural_form($object_name)
{
	if (substr($object_name,0,7)=="__keep_") {
		return substr($object_name,7);
	} elseif(class_exists($object_name)){
		$obj = new $object_name;
		$plural = $obj->getTableName();
	}else{
		if(substr($object_name,-1) == "s")
			$plural = $object_name;
		elseif(substr($object_name, -1) == "y")
			$plural = substr($object_name,0,strlen($object_name)-1)."ies";
		else
			$plural = $object_name."s";
	}
	return $plural;
}

function trim_value(&$value) 
{ 
	$value = trim($value); 
}

function table($array, $formats, $element_name, $id_name, $element_actions =array(), $general_actions=array(), $base = NULL, $insert_checkboxes = false, $css = "content", $conditional_css = array(), $object_actions_names=array(), $table_id=null, $select_all=false)
{
	global $module;

	if(!$css)
		$css = "content";
	if(!count($array))
	{
		$plural = get_plural_form($element_name);
		plainmessage("<table class=\"$css\"><tr><td>There aren't any $plural.</td></tr>");
		if(!count($general_actions)){
			print '</table>';
			return;
		}
	}

	if(!$base)
	{
		$main = (isset($_SESSION["main"])) ? $_SESSION["main"] : "main.php";
		$base = "$main?module=$module";
	}

	$lines = count($array);
	if ($element_actions)
		$act = count($element_actions);
	else
		$act = NULL;

	print '<table class="'.$css.'" cellspacing="0" cellpadding="0"';
	if ($table_id)
		print " id=\"$table_id\"";
	print '>';
	if($lines) 
	{
		print '<tr class="'.$css.'">';
		$no_columns = 0;

		$ths ="";
		if($insert_checkboxes)
		{
			$ths .= '<th class="'.$css.' first_th checkbox">';
			if ($select_all)
				$ths .= '<input type="checkbox" name="select_all" id="select_all" onclick="toggle_column(this);">';
			else
				$ths .= '&nbsp;';
			$ths .= '</th>';
			print $ths;
			$no_columns++;
		}
		// print the name of the columns + add column for each action on object

		foreach($formats as $column_name => $var_name)
		{
			$exploded = explode(":",$column_name);
			if(count($exploded)>1)
				$name = $exploded[1];
			else{
				$name = $column_name;
				if(substr($column_name, 0, 9) == "function_")
					$name = substr($column_name,9);
				if(is_numeric($column_name))
					$name = $var_name;
			}
			print '<th class="'.$css.'">';
			print str_replace("_","&nbsp;",ucfirst($name));
			print '</th>';
			$no_columns++;
		}
		for($i=0; $i<count($element_actions); $i++)
		{
/*			print '<th class="'.$css.'">&nbsp;</th>';
			$no_columns++;*/
			print '<th class="'.$css.'">';
			if  (!isset($object_actions_names) || !isset($object_actions_names[$i]))
				print '&nbsp;';
			else
				print $object_actions_names[$i];
			print '</th>';
			$no_columns++;
		}
		print '</tr>';
	}else
		$no_columns = 2;

	for($i=0; $i<count($array); $i++) 
	{
		$cond_css = '';
		foreach($conditional_css as $css_name=>$conditions)
		{
			$add_css = true;
			foreach($conditions as $column=>$cond_column)
			{
				if($array[$i][$column] != $cond_column)
				{
					$add_css = false;
					break;
				}
			}
			if($add_css)
				$cond_css .= " $css_name ";
		}
		if ($i==(count($array)-1))
			$cond_css.= " pre_end";
		print '<tr class="'.$css.'">';
		if($insert_checkboxes && $id_name)
		{
			print '<td class="'.$css. "$cond_css";
			if($i%2 == 0)
				print " evenrow";
			print '">';
			print '<input type="checkbox" name="check_'.$array[$i][$id_name].'"/>';
			print '</td>';
		}
		foreach($formats as $column_name=>$names_in_array)
		{
			print '<td class="'.$css. "$cond_css";
			if($i%2 == 0)
				print " evenrow";

			print '">';
			$use_vars = explode(",", $names_in_array);
			$exploded_col = explode(":", $column_name);
			$column_value = '';

			if(substr($exploded_col[0],0,9) == "function_") 
			{
				$function_name = substr($exploded_col[0],9,strlen($exploded_col[0]));
				if(count($use_vars)) 
				{
					$params = array();
					for($var_nr=0; $var_nr<count($use_vars); $var_nr++)
						array_push($params, $array[$i][$use_vars[$var_nr]]);
					$column_value = call_user_func_array($function_name,$params);
				}
			}elseif(isset($array[$i][$names_in_array])){
				$column_value = $array[$i][$names_in_array];
			}
			if(strlen($column_value))
				print $column_value;
			else
				print "&nbsp;";
			print '</td>';
		}
		$link = '';
		foreach($array[$i] as $col_name => $col_value)
			if (strlen($col_value)<55)
				$link .= "&$col_name=".htmlentities(urlencode($col_value));
		$link_no = 0;
		foreach($element_actions as $methd=>$methd_name)
		{
			print '<td class="'.$css. "$cond_css";
			if($i%2 == 0)
				print ' evenrow object_action';
			print '">';
			if($link_no)
				print '&nbsp;&nbsp;';
			
			if (substr($methd,0,8)!="__clean_")
				$current_link = $base.$methd.$link;
			else {
				$current_link = substr($methd,8)."?".trim($link,"&");
			}

			if (substr($methd_name,0,11)!="__inactive_")
				print '<a class="'.$css. "$cond_css".'" href="'.$current_link.'">'.$methd_name.'</a>';
			else {
				$methd_name = substr($methd_name,11);
				if (substr($methd_name,0,4)=="<img") {
					$methd_name = str_replace("<img", "<img style=\"opacity:0.4\"", $methd_name);
				}
				print $methd_name;
			}
			print '</td>';
			$link_no++;
		}
		print '</tr>';
	}

	if(count($general_actions))
	{
		print '<tr>';
		if(isset($general_actions["left"]))
		{
			$left_actions = $general_actions["left"];
			$columns_left = floor($no_columns/2);
			$no_columns -= $columns_left;
			print '<td class="'.$css.' allleft endtable" colspan="'.$columns_left.'">';
			$link_no = 0;
			foreach($left_actions as $methd=>$methd_name)
			{
				if($link_no)
					print '&nbsp;&nbsp;';
				print '<a class="'.$css.'" href="'.$base.$methd.'">'.$methd_name.'</a>';
				$link_no++;
			}
			print '</td>';
			if(isset($general_actions["right"]))
				$general_actions = $general_actions["right"];
			else
				$general_actions = array();
		}
		
		print '<td class="'.$css.' allright endtable" colspan="'.$no_columns.'">';
		$link_no = 0;
		if(!count($general_actions))
			print "&nbsp;";
		foreach($general_actions as $methd=>$methd_name)
		{
			if($link_no)
				print '&nbsp;&nbsp;';
			print '<a class="'.$css.'" href="'.$base.$methd.'">'.$methd_name.'</a>';
			$link_no++;
		}
		print '</td>';
		print '</tr>';
	}
	print "</table>";
}

function ack_delete($object, $value=NULL, $message=NULL, $object_id=NULL, $value_id=NULL, $additional=NULL, $next=null)
{
	global $module, $method;

	if(!$object_id)
		$object_id = $object.'_id';
	if(!$value_id)
		$value_id = getparam($object_id);

	print "<br/><br/>Are you sure you want to delete ".str_replace("_","&nbsp;",$object)." $value?";
	if ($message) {
		if(substr($message,0,2)!="__") {
			if(substr($message,0,1) == ",")
				$message = substr($message, 1, strlen($message));
			print " If you delete it you will also delete or set to NULL it's associated objects from $message.";
		} else
			print substr($message,2);
	}

	print "<br/><br/>";

	$link = $_SESSION["main"] .'?';
	if(isset($_SESSION["previous_page"]))
		foreach ($_SESSION["previous_page"] as $param=>$value)
			$link .= "$param=$value&";
	$link .= '&module=' . $module . '&method=' . $method . '&action=database&' . $object_id . '=' . $value_id . $additional;

	

	print '<a class="llink" href="'.$link.'">Yes</a>';

	print '&nbsp;&nbsp;&nbsp;&nbsp;'; 

	if (!$next && isset($_SESSION["previous_page"])) {
		$link = $_SESSION["main"].'?';
		foreach ($_SESSION["previous_page"] as $param=>$value)
			$link .= "$param=$value&";
	} else {
		$link = $_SESSION["main"].'?module='.$module.'&method='.$next;
	}
	print '<a class="llink" href="'.$link.'">No</a>';
}

function month_year($value=null,$key=null)
{
	$date = params_date($value);
	return set_month($date["month"],$key).set_year($date["year"],$key);
}

function month_day_year_hour_end($val,$key = NULL)
{
	return month_day_year_hour($val,$key,true,true,false);
}

function day_month_year_hour_end($val,$key = NULL)
{
	return day_month_year_hour($val,$key,true,true,false);
}

function month_day_year($val,$key=null,$etiquettes=false)
{
	$date = params_date($val);
	$res = set_month($date["month"], $key, $etiquettes);
	$res.= set_day($date["day"], $key, $etiquettes);
	$res.= set_year($date["year"], $key, $etiquettes);
	return $res;	
}

function month_day_year_hour($date, $key='', $default_today=true, $etiquettes=true, $begin_hour=true)
{
	$date = params_date($date, $default_today, $begin_hour);
	$res = set_month($date["month"], $key, $etiquettes);
	$res.= set_day($date["day"], $key, $etiquettes);
	$res.= set_year($date["year"], $key, $etiquettes);
	$res.= set_hour($date["hour"], $key, $etiquettes);
	return $res;
}

function day_month_year_hour($date, $key='', $default_today=true, $etiquettes=true, $begin_hour=true)
{
	$date = params_date($date, $default_today, $begin_hour);
	$res = set_day($date["day"], $key, $etiquettes);
	$res.= set_month($date["month"], $key, $etiquettes);
	$res.= set_year($date["year"], $key, $etiquettes);
	$res.= set_hour($date["hour"], $key, $etiquettes);
	return $res;
}

function params_date($date, $default_today=false, $begin_hour=null)
{
	$month = $day = $year = $hour = "";
	if ((!$date || $date == '') && $default_today) {
		/*$today = explode("-",date('F-j-Y-H-i',time()));
		$month = $today[0]; 
		$day = $today[1]; 
		$year = $today[2];*/
		$today = date("Y-m-d H:i:s");
		$today = check_timezone($today);
		$expl = explode(" ",$today);
		$date = explode("-",$expl[0]);
		$time = explode(":",$expl[1]);
		$month = $date[1];
		$day = $date[2];
		$year = $date[0];
		if ($begin_hour === true)
			$hour = "0";
		elseif ($begin_hour === false)
			$hour = "23";
		else
			$hour = $today[3];
	} elseif($date && $date!="") {
		$today = explode(" ",$date);
		$hour = explode(":",$today[1]);
		$hour = $hour[0];
		$today = explode("-",$today[0]); 
		$year = $today[0];
		$month = $today[1]; 
		$day = $today[2]; 
	}
	return array("day"=>$day, "month"=>$month, "year"=>$year, "hour"=>$hour);
}

function set_hour($hour, $key='', $etiquettes=false, $js='')
{
	$res = "";
	if ($etiquettes)
		$res .= "&nbsp;Hour:&nbsp;";
	$res.= '<select name="'.$key.'hour" '.$js.'>'."\n";
	for ($i=0; $i<24; $i++) {
		$selected = ($hour == $i) ? "SELECTED" : "";
		$res.= "<option $selected>".$i."</option>"."\n";
	}
	$res.= "</select>";
	return $res;
}

function set_year($year, $key='', $etiquettes=false, $js='')
{
	$res = "";
	if ($etiquettes)
		$res .= "&nbsp;Year:&nbsp;";
	$res.= '<input type="text" name="'.$key.'year" value="'.$year.'" size="4" '.$js.'/>'."\n";
	return $res;
}

function set_month($month, $key='', $etiquettes=false, $default_today=false, $js='')
{
	$months = array("January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December");;
	$res = "";
	if ($etiquettes)
		$res .= "Month:&nbsp;";
	$res .= '<select name="'.$key.'month" '.$js.'>'."\n";
	if (!$default_today)
		$res.= "<option value=\"\">-</option>";
	for ($i=0; $i<count($months); $i++) {
		$index = $i+1;
		$d_index = (strlen($index)==1) ? "0".$index : $index;
		$selected = ($month == $months[$i] || $month == $index || $month == $d_index) ? "SELECTED" : "";
		$res .= "<option $selected>".$months[$i]."</option>"."\n";
	}
	$res.= "</select>"."\n";
	return $res;
}

function set_day($day, $key='', $etiquettes=false, $default_today=false, $js="")
{
	$res = "";
	if ($etiquettes)
		$res .= "Day:&nbsp;";
	$res.= '<select name="'.$key.'day" '.$js.'>'."\n";
	if (!$default_today)
		$res.= "<option value=\"\">-</option>";
	for ($i=1; $i<32; $i++) {
		$selected = ($day == $i || $day == "0".$i) ? "SELECTED" : "";
		$res .= "<option $selected>".$i."</option>"."\n";
	}
	$res.= "</select>"."\n";
	return $res;
}

function seconds_trunc($time, $floor=false)
{
	global $date_format;

	$date = explode(" ",$time);
	if(count($date)>1) {
		// not just "time" but "date and time"
		if($date_format == "d-m-y") {
			$new_date = european_date($date[0]);
			$time = $new_date." ".$date[1];
		}
	}

	$date = explode('.',$time);
	$date = $date[0];
	$date = explode(":",$date);
	$sec = count($date) - 1;

	if (!$floor)
		$date[$sec] ++;

	if(strlen($date[$sec]) == 1)
		$date[$sec] = '0'.$date[$sec];
	if($date[$sec] == 60) {
		$date[$sec-1]++;
		$date[$sec] = 0;
	}
	$date = implode(":",$date);
	return $date;
}

function seconds_trunc_floor($time)
{
	return seconds_trunc($time,true);
}

function select_date($timestamp)
{
	global $date_format;

	$timestamp = check_timezone($timestamp);
	$timestamp = explode(" ",$timestamp);
	$date =  $timestamp[0];
	if (isset($date_format) && $date_format == "d-m-y")
		$date = european_date($date);
	return $date;
}

function european_date($date)
{
	$date = explode("-",$date);
	return $date[2]."-".$date[1]."-".$date[0];
}

function select_time($timestamp)
{
	$timestamp = check_timezone($timestamp);
	$date = seconds_trunc($timestamp);
	$date = explode(' ',$date);
	return $date[1];
}

function trunc_date($timestamp)
{
	global $date_format;

	$timestamp = check_timezone($timestamp);
	$date = explode(" ",$timestamp);
	if(count($date)>1) {
		// not just "time" but "date and time"
		if($date_format == "d-m-y") {
			$new_date = european_date($date[0]);
			$timestamp = $new_date." ".$date[1];
		}
	}
	$timestamp = explode('.',$timestamp);
	return $timestamp[0];
}

//	formatfor $timestamp must be Y-m-d H:i:s.miliseconds
function check_timezone($timestamp, $reverse_apply=false)
{
	global $system_standard_timezone;

	// check if $_SESSION["timezone"] is set and if is different from system timezone
	if (!isset($_SESSION["timezone"]))
		return $timestamp;

	$timezone = str_replace("GMT","",$_SESSION["timezone"]);
	if ($timezone == "")
		$timezone = 0;
/*	$system_tz = str_replace("0","",date("P"));
	$system_tz = str_replace(":","",$system_tz);*/
	$system_tz = str_replace("GMT","",$system_standard_timezone);
	if ($system_tz == "")
		$system_tz = 0;

	$diff = $timezone - $system_tz;
	if ($reverse_apply)
		$diff = -$diff;

	$arr = explode(".",$timestamp);
	$milisecs = (isset($arr[1])) ? $arr[1] : null;
	$date = $arr[0];
	$date = explode(" ",$date);
	$time = explode(":",$date[1]);
	$date = explode("-",$date[0]);
	$hours = $time[0] + $diff;	
	$new_timestamp = gmdate("Y-m-d H:i:s", gmmktime($hours, $time[1], $time[2], $date[1], $date[2], $date[0]));
	return $new_timestamp;
}

function get_timezone_diff($reverse_apply=false)
{
	global $system_standard_timezone;

	// check if $_SESSION["timezone"] is set and if is different from system timezone
	if (!isset($_SESSION["timezone"]))
		return "0";

	$timezone = str_replace("GMT","",$_SESSION["timezone"]);
	if ($timezone == "")
		$timezone = 0;
/*	$system_tz = str_replace("0","",date("P"));
	$system_tz = str_replace(":","",$system_tz);*/
	$system_tz = str_replace("GMT","",$system_standard_timezone);
	if ($system_tz == "")
		$system_tz = 0;

	$diff = $timezone - $system_tz;
	if ($reverse_apply)
		$diff = -$diff;

	return $diff;
}

function add_interval($timestamp, $unit, $nr)
{
	$timestamp = explode(" " ,$timestamp);
	$date = explode("-",$timestamp[0]);
	$time = explode(".",$timestamp[1]);
	$time = explode(":",$time[0]);
	$year = $date[0];
	$month = $date[1];
	$day = $date[2];
	$hours = $time[0];
	$minutes = $time[1];
	$seconds = $time[2];

	${$unit} += $nr;
	$date = date('Y-m-d H:i:s',gmmktime($hours,$minutes,$seconds,$month,$day,$year));

	return $date;
}

function get_time($key='',$hour2='00',$min='00',$sec="00")
{
	$day = getparam($key."day");
	if(!$day)
		return null;
	$month = getparam($key."month");
	$month = getmonthnumber($month);
	if(strlen($month))
		$month = '0'.$month;
	$year = getparam($key."year");
	$hour = getparam($key."hour");
	if(!$hour)
		$hour = $hour2;

	if(!checkdate($month,$day,$year)) {
		errormess("This date does not exit : day=".$day.' month='.$month.' year='.$year,'no');
		return;
	}
	$date = gmmktime($hour,$min,0,$month,$day,$year);
	return $date;
}

function get_date($key='',$_hour='00',$min='00',$sec='00')
{
	$day = getparam($key."day");
	$month = getparam($key."month");
	if(!$day || !$month)
		return null;
	$month = getmonthnumber($month);
	$year = getparam($key."year");
	$hour = getparam($key."hour");
	if (strlen(!$hour))
		$hour = $_hour;
	if (strlen($hour)==1)
		$hour = "0".$hour;

	if (!checkdate($month,$day,$year)) {
		errormess("This date does not exit : day=".$day.' month='.$month.' year='.$year,'no');
		return;
	}
	if(strlen($month) == 1)
		$month = '0'.$month;
	if(strlen($day) == 1)
		$day = '0'.$day;
	if(strlen($_hour))
		$date = "$year-$month-$day $hour:$min:$sec";
	else
		$date = "$year-$month-$day";
	return $date;
}

function insert_letters($link = "main.php", $force_method = NULL)
{
	global $module,$method;

	if (is_array($module))
		$module = $module[0];
	if ($force_method)
		$method = $force_method;

	$letter = getparam("letter");
	
	if (!$letter)
		$letter = 'A';
	
	$letters = array("A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "X", "Y", "W", "Z");

	for($i=0; $i<count($letters); $i++) {
		print '&nbsp;';
		if ($letter == $letters[$i])
			print $letter;
		else{
			print '<a class="llink" href="'.$link;
			if(!strpbrk($link,"?"))
				print '?';
			else
				print "&";
			print 'module='.$module.'&method='.$method.'&letter='.$letters[$i].'">'.$letters[$i].'</a>';
		}
	}
	print '<br/><hr>';
	return $letter;
}

function insert_numbers($link = "main.php")
{
	global $module,$method;

	$nr = getparam("nr");
	if(!$nr)
		$nr = 0;

	if (is_array($module))
		$module = $module[0];

	for($i=0; $i<10; $i++) {
		print '&nbsp;';
		if ($nr == $i)
			print $nr;
		else
			print '<a class="llink" href="'.$link.'?module='.$module.'&method='.$method.'&nr='.$i.'">'.$i.'</a>';
	}
	print '<br/><hr>';
	return $nr;
}

function interval_to_minutes($interval)
{
	if(!$interval)
		return NULL;
	$interval2 = explode(':',$interval);

	return round($interval2[0]*60+$interval2[1]+$interval2[2]/60,2);
}

function minutes_to_interval($minutes = NULL)
{
	//minutes should be interger: ingoring seconds
	$minutes = floor($minutes);

	if(!$minutes)
		return '00:00:00';

	$hours = floor($minutes / 60);
	$mins = $minutes - $hours*60;

	//don't care if $hours > 24
	if (strlen($hours) == 1)
		$hours = '0'.$hours;
	if (strlen($mins) == 1)
		$mins = '0'.$mins;
	return "$hours:$mins:00";
}

function gen_random($lim = 8)
{
	$nr = '';
	for ($digit = 0; $digit < $lim; $digit++) {
		$r = rand(0,1);
		$c = ($r==0)? rand(65,90) : rand(97,122);
		$nr .= chr($c);
	}
	return $nr;
}

function getmonthnumber($month)
{
    switch($month){
		case "January":
		case "Ianuarie":
			return '1';
		case "February":
		case "Februarie":
			return '2';
		case "March":
		case "Martie":
			return '3';
		case "April":
		case "Aprilie":
			return '4';
		case "May":
		case "Mai":
			return '5';
		case "June":
		case "Iunie":
			return '6';
		case "July":
		case "Iulie":
			return '7';
		case "August":
			return '8';
		case "September":
		case "Septembrie":
			return '9';
		case "October":
		case "Octombrie":
			return '10';
		case "November":
		case "Noiembrie":
			return '11';
		case "December":
		case "Decembrie":
			return '12';
	}
	return false;
}

function get_month($nr)
{
	switch($nr) {
		case "13":
		case "1":
		case "01":
			return "January";
		case "2":
		case "02":
		case "14":
			return "February";
		case "3":
		case "03":
		case "15":
			return "March";
		case "4":
		case "04":
			return "April";
		case "5":
		case "05":
			return "May";
		case "6":
		case "06":
			return "June";
		case "7":
		case "07":
			return "July";
		case "8":
		case "08":
			return "August";
		case "9":
		case "09":
			return "September";
		case "10":
			return "October";
		case "11":
			return "November";
		case "12":
			return "December";
		default:
			return "Invalid month: $nr";
	}
}

function make_number($value)
{
	$value = str_replace("$",'',$value);
	$value = str_replace(' ','',$value);
	$value = str_replace('%','',$value);
	$value = str_replace('&','',$value);
	return $value;
}

function make_picture($name)
{
	$name = strtolower($name);
	$name = str_replace(" ","_",$name);
	$name .= '.jpg';
	return $name;
}

function next_page($max)
{
	global $module, $method, $limit;
	$offset = getparam("offset");
	if(!$offset)
		$offset = 0;

	$minus = $offset - $limit;
	$plus = $offset + $limit;
	$page = number_format($offset / $limit) + 1;
	print '<br/><center>';
	if($minus >= 0)
		print '<a class="llink" href="main.php?module='.$module.'&method='.$method.'&offset='.$minus.'&max='.$max.'"><<</a>&nbsp;&nbsp;&nbsp;';
	print $page;
	if($plus < $max)
		print '&nbsp;&nbsp;&nbsp<a class="llink" href="main.php?module='.$module.'&method='.$method.'&offset='.$plus.'&max='.$max.'">>></a>&nbsp;&nbsp;&nbsp;';
	print '</center>';
}

function unsetparam($param)
{
	if(isset($_POST[$param]))
		unset($_POST[$param]);
	if(isset($_GET[$param]))
		unset($_GET[$param]);
	if(isset($_REQUEST[$param]))
		unset($_REQUEST[$param]);
}

function bytestostring($size, $precision = 0) 
{
	$sizes = array('YB', 'ZB', 'EB', 'PB', 'TB', 'GB', 'MB', 'kB', 'B');
	$total = count($sizes);

	while($total-- && $size > 1024) 
		$size /= 1024;

	return round($size, $precision).$sizes[$total];
}

function form_params($fields)
{
	$params = array();
	for($i=0; $i<count($fields); $i++)
		$params[$fields[$i]] = getparam($fields[$i]);
	return $params;
}

function field_value($field, $array)
{
	if(isset($array[$field]))
		return $array[$field];
	return NULL;
}

function explanations($logo, $title, $explanations, $style="explanation")
{
	global $method;

	if(is_array($explanations))
		if(isset($explanations[$method]))
			$text = $explanations[$method];
		else
			$text = $explanations["default"];
	else
		$text = $explanations;

	print '<div class="'.$style.'">';
	print '<table class="fillall" cellspacing="0" cellpadding="0" >';
	print '<tr>';
	print '<td class="logo_wizard" style="padding:5px;">';
	if ($logo && $logo != "")
		print '<img src="'.$logo.'">';
	print '</td>';
	print '<td class="title_wizard" style="padding:5px;" >';
	print '<div class="title_wizard">'.$title.'</div>';
	print '</td>';	
	print '</tr>';
	print '<tr>';
	print '<td class="step_description" style="font-size:13px;padding:5px;" colspan="2">';
	print $text;
	print '</td>';
	print '</tr>';
	print '</table>';
	print '</div>';
}

function build_dropdown($arr, $name, $show_not_selected = true, $disabled = "", $css="", $javascript="", $just_options=false, $hidden=false)
{
	if(!$just_options) {
		$hidden = ($hidden) ? " style=\"display:none;\"" : "";
		$res = '<select class="dropdown '.$css.'" name="'.$name.'" id="'.$name.'" '.$disabled.$hidden.' ';
		if(substr($name,-2) == "[]")
			$res .= " multiple=\"multiple\" size=\"3\"";
		$res .= $javascript.'>'."\n";
	} else
		$res = '';
	if($show_not_selected)
		$res .= '<option value=""> - </option>'."\n";
	$selected = (isset($arr["selected"]))? $arr["selected"] : "";
	unset($arr["selected"]);
	for($i=0; $i<count($arr); $i++) {
		if(is_array($arr[$i])) {
			$value = $arr[$i]["field_id"];
			$value_name = $arr[$i]["field_name"];
			$css = (isset($arr[$i]["css"])) ? 'class="'.$arr[$i]["css"].'"' : "";
			$res .= "<option value=\"$value\" $css";
			if(is_array($selected)) {
				if(in_array($value,$selected))
					$res .= " SELECTED";
			}else{
				if($selected == $value)
					$res .= " SELECTED";
			}
			if($value === "__disabled")
				$res .= " disabled=\"disabled\"";
			$res .= ">$value_name</option>\n";
		}
	}
	if(!$just_options)
		$res .= "</select>\n";
	return $res;
}

function format_for_dropdown($vals)
{
	$arr = array();
	for($i=0; $i<count($vals); $i++)
		array_push($arr, array("field_id"=>$vals[$i], "field_name"=>$vals[$i]));
	return $arr;
}

function formTable($rows, $th=null, $title = null, $submit = null, $width=null, $id=null, $css_first_column='', $color_indexes=array(), $dif_css="")
{
	if(is_array($th))
		$cols = count($th);
	elseif(isset($rows[0]))
		$cols = count($rows[0]);
	else
		$cols = count($rows);
	if (substr($width,-2) != "px" && substr($width,-1) != "%")
		$width .= "px";
	$width = ($width) ? "style=\"width:".$width.";\"" : "";
	$id = ($id) ? " id=\"$id\"" : "";
	print '<table class="formtable '.$dif_css.'" cellspacing="0" cellpadding="0" '.$width.' '.$id.'>'."\n";
	if($title) {
		print "<tr>\n";
		print '<th class="title_formtable" colspan="'.$cols.'">'.$title.'</td>'."\n";
		print "</tr>\n";
	}
	if(is_array($th)) {
		print "<tr>\n";
		for($i=0; $i<count($th); $i++) {
			if(is_array($th[$i])) {
				$style = "style=\"width:".$th[$i]["width"].";\"";
				$info = $th[$i][0];
			}else{
				$style = "";
				$info = $th[$i];
			}
			print "<th class=\"formtable\" $style>".$info."</th>\n";
		}
		print "</tr>\n";
	}
	if(isset($rows[0])) {
		for($i=0; $i<count($rows); $i++) {
			$row = $rows[$i];
			$custom_css = (in_array($i,$color_indexes)) ? "custom_border" : "";
			print "<tr>\n";
			if(is_array($row)) {
				for($j=0; $j<count($row); $j++) {
					$css = ($i%2 == 0) ? "formtable evenrow_ftable $dif_css" : "formtable $dif_css";
					if (!$th && $i==0 && $j==0)
						$css .= "firsttd";
			//		if($j == 0)
			//			$css .= " $css_first_column"."";
					if($i%2 == 0)
						print "<td class=\"$css $custom_css\">". $row[$j] ."</td>\n";
					else
						print "<td class=\"$css $custom_css\">". $row[$j] ."</td>\n";
				}
			}else{
				print '<td class="white_row" colspan="'.count($th).'">'.$row.'</td>';
			}
			print "</tr>\n";
		}
	}else{
		$i = 0;
		foreach($rows as $key=>$format) {
			print "<tr>\n";
			$css = ($i%2 === 0) ? "formtable evenrow $dif_css" : "formtable oddrow $dif_css";
			display_pair($key, $format, null, null, $css, null, null);
			print "</tr>\n";
			$i++;
		}
	}
	if($submit) {
		print "<tr>\n";
		print "<td class=\"submit_formtable\" colspan=$cols>";
		print $submit;
		print "</td>";
		print "</tr>\n";
	}
	print "</table>\n";
}

function set_default_object($fields, $selected, $method, $message)
{
	start_form();
	if ($method)
		addHidden("database", array("method"=>$method));
	if (!$selected) {
		subtitle($message);
		print '<table class="error" cellspacing="0" cellpadding="0"><tr><td>'."\n";
	}
	formTable($fields, null, null, null, null, null, '', array(), "blue_letters");
	if (!$selected)
		print '</td></tr></table>'."\n";
	else
		hr();
	end_form();
	br();
}

function set_form_fields(&$fields, $error_fields, $field_prefix='')
{
	if (!$error_fields)
		$error_fields = array();

	foreach ($fields as $name=>$def)
	{
		if (!isset($def["display"]))
			$def["display"] = "text";
		if ($def["display"] == "hidden" || $def["display"]=="message" || $def["display"]=="fixed")
			continue;
		if (in_array($name, $error_fields))
			$fields[$name]["error"] = true;
		if (substr($name,-2) == "[]" && $def["display"] == "mul_select")
			$val = getparam($field_prefix.substr($name,0,strlen($name)-2));
		else
			$val = getparam($field_prefix.$name);
		if ($val) {
			if (isset($fields[$name][0]) && is_array($fields[$name][0]))
				$fields[$name][0]["selected"] = $val;
			elseif ($def["display"] == "checkbox")
				$fields[$name]["value"] = ($val == "on") ? "t" : "f";
			else
				$fields[$name]["value"] = $val;
		}
	}
}

function set_error_fields($error, &$error_fields)
{
	// fields between '' are considered names
	$field = '';
	$start = false;
	$error = strtolower($error);
	for	($i=0; $i<strlen($error); $i++) {
		if ($error[$i] != "'") {
			if($start)
				$field .= $error[$i];
		} else {
			if ($start) {
				if(!in_array($field, $error_fields))
					$error_fields[] = $field;
				$field = '';
				$start = false;
			} else
				$start = true;
		}
	}
}

function error_handle($error, &$fields, &$error_fields, $field_prefix='')
{
	if ($error) {
		errormess($error,"no");
		set_error_fields($error, $error_fields);
		set_form_fields($fields, $error_fields, $field_prefix);
	}
	if ($error===false) {
		set_error_fields($error, $error_fields);
		set_form_fields($fields, $error_fields, $field_prefix);
	}
}

function return_button($method=null, $_module=null, $align="right", $name="Return")
{
	global $module;

	if ($method) {
		if (!$_module)
			$_module = $module;
		$link = $_SESSION["main"].'?module='.$_module.'&method='.$method;
	} elseif (isset($_SESSION["previous_page"])) {
		$link = $_SESSION["main"]."?";
		foreach ($_SESSION["previous_page"] as $param=>$value)
			$link.= "$param=".urlencode($value)."&";
	} else
		$link = $_SESSION["main"]. "?"."module=".$module;
	print '<div style="float:'.$align.'"><a class="llink" href="'.$link.'">'.$name.'</a></div>';
	br();
}

function hr()
{
	print "<hr class=\"bluehr\" />";
}

function exception_to_save()
{
	global $method, $module, $exceptions_to_save;

	if (isset($exceptions_to_save) && isset($exceptions_to_save[$_SESSION["level"]]) && isset($exceptions_to_save[$_SESSION["level"]]["$module"]) && (in_array($method, $exceptions_to_save[$_SESSION["level"]]["$module"]) || (in_array("__all", $exceptions_to_save[$_SESSION["level"]]["$module"]))))
		return true;

	if (!isset($_SESSION["level"]) && isset($exceptions_to_save["__default"][$method]))
		return true;

	return false;
}

function save_page_info()
{
	global $method, $module, $exceptions_to_save;

	// don't save info for edit/add/delete pages
	if (substr($method,0,4) == "edit" || substr($method,0,4) == "add_" || substr($method,0,6) == "delete" || exception_to_save() || substr($method,0,6)=="import" || substr($method,0,6)=="export") {
		return;
	}

	$_SESSION["previous_page"] = array();

	$param_exceptions = array("PHPSESSID", "old_submit");
	foreach ($_REQUEST as $param=>$value) {
		if (in_array($param, $param_exceptions) || substr($param,0,5)=="__utm")
			continue;
		if ($param == "module")
			$value = $module;
		elseif ($param == "method")
			$value = $method;
		$_SESSION["previous_page"][$param] = $value;
	}
}

function make_routing_test($fields)
{
	global $module;

	if ($_SESSION["level"] != "admin")
		return;

	if (!$fields) {
		$fields = array("called"=>"", "caller"=>"");
	}

	$fields["th_custom"] = array("display"=>"message", "value"=>"<div style=\"width:100px; float:left; text-align:center;\">Param</div>   <div style=\"width:100px; float:left; position:relative; text-align:center;\">Value</div>", "triggered_by"=>"1");
	$fields["custom_1"] = array("display"=>"message", "value"=>link_for_custom_field(1));
	for ($i=1; $i<=20; $i++) {
		$fields["custom$i"] = array("display"=>"message", "triggered_by"=>"$i", "value"=>routing_test_custom_field($i));
	}

	start_form();
	addHidden("yate");
	editObject(null, $fields, "Make routing test", "Send");
	end_form();
}

function link_for_custom_field($index)
{
	$ret = "<a class=\"llink\" id=\"custom_field_index$index\" onClick=\"";
	$ret.= ($index == 1) ? "show_hide('tr_th_custom');" : "";
	$ret.= "show_hide('tr_custom$index'); show_hide('custom_field_index$index');";
	if ($index == 1)
		$ret.= "show_hide('tr_custom_$index');";
	$ret.= "\" > Add parameter</a>";
	return $ret;
}

function routing_test_custom_field($index)
{
	$param_name = getparam("param_name$index");
	$param_value = getparam("param_value$index");
	$ret = "<input type=\"text\" name=\"param_name$index\" value=\"$param_name\" style=\"width:100px;\"/> = \n";
	$ret.= "<input type=\"text\" name=\"param_value$index\" value=\"$param_value\" style=\"width:100px;\" />";
	$index++;
	$ret.= link_for_custom_field($index);
	return $ret;
}

function make_routing_test_yate($fields=null)
{
	if (!$fields) {
		$fields = array();
		$fields["called"] = getparam("called");
		$fields["caller"] = getparam("caller");
	}

	for ($i=1; $i<=20; $i++) {
		if (!getparam("param_name$i"))
			continue;
		$fields[getparam("param_name$i")] = getparam("param_value$i");
	}
	return $fields;
}

function send_routing_test_yate($fields)
{
	$command = "routing_test ";
	foreach ($fields as $name=>$value) {
		if ($command != "routing_test ")
			$command .= ";";
		$name = str_replace(";","",$name);
		$value = str_replace(";","",$value);
		$command .= "$name=$value";
	}

	$socket = new SocketConn;
	if($socket->error == "") {
		return array(true,$socket->command($command/*"uptime"*/));
	}else{
		errormess("Can't make test: ".print_r($socket->error,true), "no");
		return array(false);
	}
}


function br($count = null)
{
	if(!$count)
		$count = 1;

	for($i=0; $i<$count; $i++) {
		print "<br/>"."\n";
	}
}

function nbsp($count = null, $print=true)
{
	if(!$count)
		$count = 1;

	$res = '';
	for($i=0; $i<$count; $i++) {
		$res .= "&nbsp;";
	}
	if ($print)
		print $res;
	else
		return $res;
}

function send_mail($emailaddress, $fromaddress, $emailsubject, $body, $attachments=false, $names=null, $notice=true, $link_with_notice = false)
{
	global $path;
	$now = time();
	# Is the OS Windows or Mac or Linux
	if (strtoupper(substr(PHP_OS,0,3)=='WIN')) {
	    $eol="\r\n";
	} elseif (strtoupper(substr(PHP_OS,0,3)=='MAC')) {
	    $eol="\r";
	} else {
	    $eol="\n";
	}
	$mime_boundary=md5(time());

	if(!$names)
		$names = '';//$fromaddress;

	# Common Headers
	$headers = 'From: '.$names.'<'.$fromaddress.'>'.$eol;
	$headers .= 'Reply-To: <'.$fromaddress.'>'.$eol;
	$headers .= 'Return-Path: <'.$fromaddress.'>'.$eol;    // these two to set reply address
	$headers .= "Message-ID: <".$now." TheSystem@".$_SERVER['SERVER_NAME'].">".$eol;
	$headers .= "X-Mailer: PHP v".phpversion().$eol;          // These two to help avoiker spam-filters
	# Boundry for marking the split & Multitype Headers
	$headers .= 'MIME-Version: 1.0'.$eol;
	$headers .= "Content-Type: multipart/mixed; boundary=\"".$mime_boundary."\"".$eol;
	$msg = "";
 
	#this is the body of the message
	$msg .= "--".$mime_boundary.$eol;
	$msg .= "Content-Type: text/html; charset=ISO-8859-1".$eol;
	$msg .= "Content-Transfer-Encoding: QUOTED-PRINTABLE".$eol.$eol;
	//$msg .= strip_tags(str_replace("<br>", "\n", $body)).$eol.$eol;
	//$msg .= strip_tags(str_replace("\n", "<br/>", $body)).$eol.$eol;
	$msg .= $body.$eol.$eol;

	if($attachments !== false  && count($attachments) > 0) {
	# Boundry for marking the split & Multitype Headers
		$headers .= 'MIME-Version: 1.0'.$eol;
		$headers .= "Content-Type: multipart/mixed; boundary=\"".$mime_boundary."\"".$eol;

		$msg = "";
	
		#this is the body of the message
		$msg .= "--".$mime_boundary.$eol;
		$msg .= "Content-Type: text/html; charset=ISO-8859-1".$eol;

		//$msg .= strip_tags(str_replace("<br>", "\n", $body)).$eol.$eol;
		//$msg .= strip_tags(str_replace("\n", "<br/>", $body)).$eol.$eol;
		$msg .= $body.$eol.$eol;
	
		#support for multiple attachments (i won't be using this for now)
		if ($attachments !== false  && count($attachments) > 0)
		{
	
			for($i=0; $i < count($attachments); $i++)
			{
				if (is_file($attachments[$i]["file"]))
				{  
				# File for Attachment
					$file_name = substr($attachments[$i]["file"], (strrpos($attachments[$i]["file"], "/")+1));
		
					$handle=fopen($attachments[$i]["file"], 'rb');
					$f_contents=fread($handle, filesize($attachments[$i]["file"]));
					$f_contents=chunk_split(base64_encode($f_contents));    //Encode The Data For Transition using base64_encode();
					fclose($handle);
		
					# Attachment
					$msg .= "--".$mime_boundary.$eol;
					$msg .= "Content-Type: ".$attachments[$i]["content_type"]."; name=\"".$file_name."\"".$eol;
					$msg .= "Content-Transfer-Encoding: base64".$eol;
					$msg .= "Content-Disposition: attachment; filename=\"".$file_name."\"".$eol.$eol; // !! This line needs TWO end of lines !! IMPORTANT !!
					$msg .= $f_contents.$eol.$eol;
				}
			}
		}
	}
	$msg .= "--".$mime_boundary.'--'.$eol.$eol;
	
	# SEND THE EMAIL
	ini_set('sendmail_from',$fromaddress);  // the INI lines are to force the From Address to be used !
	if (mail($emailaddress, $emailsubject, $msg, $headers)){
		if($notice && !$link_with_notice)
			message("E-mail message was sent succesfully to ".$emailaddress,'no');
		elseif($notice && $link_with_notice)
			message("E-mail message was sent succesfully to ".$emailaddress);
		return true;
	}else{
		if($notice && !$link_with_notice)
			errormess("Could not send e-mail message to ".$emailaddress,'no');
		elseif($notice && $link_with_notice)
			errormess("Could not send e-mail message to ".$emailaddress);
		return false;
	}

	ini_restore('sendmail_from');
}

// function taken from user comments from php.net site
function get_mp3_len($file) 
{
	$rate1 = array(0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, "bad");
	$rate2 = array(0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, "bad");
	$rate3 = array(0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, "bad");
	$rate4 = array(0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, "bad");
	$rate5 = array(0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, "bad");

	$bitrate = array(
			'1'  => $rate5,
			'2'  => $rate5,
			'3'  => $rate4,
			'9'  => $rate5,
			'10' => $rate5,
			'11' => $rate4,
			'13' => $rate3,
			'14' => $rate2,
			'15' => $rate1
		);

	$sample = array(
			'0'  => 11025,
			'1'  => 12000,
			'2'  => 8000,
			'8'  => 22050,
			'9'  => 24000,
			'10' => 16000,
			'12' => 44100,
			'13' => 48000,
			'14' => 32000
		);

	$fd = fopen($file, 'rb');
	$header = fgets($fd, 5);
	fclose($fd);

	$bits = "";
	while (strlen($header) > 0) {
		//var_dump($header);
		$bits .= str_pad(base_convert(ord($header{0}), 10, 2), 8, '0', STR_PAD_LEFT);
		$header = substr($header, 1);
	}

	$bits = substr($bits, 11); // lets strip the frame sync bits first.

	$version = substr($bits, 0, 2); // this gives us the version
	$layer = base_convert(substr($bits, 2, 2), 2, 10); // this gives us the layer
	$verlay = base_convert(substr($bits, 0, 4), 2, 10); // this gives us both

	$rateidx = base_convert(substr($bits, 5, 4), 2, 10); // this gives us the bitrate index
	$sampidx = base_convert($version.substr($bits, 9, 2), 2, 10); // this gives the sample index
	$padding = substr($bits, 11, 1); // are we padding frames?

	$rate = $bitrate[$verlay][$rateidx];
	$samp = $sample[$sampidx];

	$framelen = 0;
	$framesize = 384; // Size of the frame in samples
	if ($layer == 3) { // layer 1?
		$framelen = (12 * ($rate * 1000) / $samp + $padding) * 4;
	} else { // Layer 2 and 3
		$framelen = 144 * ($rate * 1000) / $samp + $padding;
		$framesize = 1152;
	}

	$headerlen = 4 + ($bits{4} == 0 ? '2' : '0');

	return (filesize($file) - $headerlen) / $framelen / ($samp / $framesize);
}

function arr_to_csv($file_name, $arr, $formats=null, $func="write_in_file")
{
	global $upload_path;

	$format = ".csv";

	$date = date("YMd_H_i_s");
	$file = $upload_path."/$file_name$format";
	if (is_file($file)) {
		unlink($file);
	}
	$fh = fopen($file, "w") or die("Can't open file for writting.");

	$sep = ",";
	$func($fh, $formats, $arr, $sep);
	fclose($fh);
	chmod($file, 0777);

	//print "Content was exported. <a href=\"$upload_path/$file_name$date$format\">Download</a>";
	print "Content was exported. <a href=\"download.php?file=$file_name$format\">Download</a>";
}

function mysql_in_file($fh, $formats, $array, $sep)
{
	$col_nr = 0;
	
	if (!$formats && mysql_num_rows($array))
		for($i=0; $i<mysql_num_fields($array); $i++)
			$formats[mysql_field_name($array,$i)] = mysql_field_name($array,$i);
	if ($formats!="no") {
		foreach($formats as $column_name => $var_name)
		{
			$exploded = explode(":",$column_name);
			if(count($exploded)>1)
				$name = $exploded[1];
			else{
				$name = $column_name;
				if(substr($column_name, 0, 9) == "function_")
					$name = substr($column_name,9);
				if(is_numeric($column_name))
					$name = $var_name;
			}
			$val = str_replace("_"," ",ucfirst($name));
			$val = str_replace("&nbsp;"," ",$val);
			$val = ($col_nr) ? $sep."\"$val\"" : "\"$val\"";
			fwrite($fh, $val);
			$col_nr++;
		} 
		fwrite($fh,"\n");
	}
	//for($i=0; $i<mysql_num_rows($array); $i++) 
	while($row = mysql_fetch_assoc($array))
	{
		$col_nr = 0;

		foreach($formats as $column_name=>$names_in_array)
		{
			$use_vars = explode(",", $names_in_array);
			$exploded_col = explode(":", $column_name);
			$column_value = '';

			if(substr($exploded_col[0],0,9) == "function_") 
			{
				$function_name = substr($exploded_col[0],9,strlen($exploded_col[0]));
				if(count($use_vars)) 
				{
					$params = array();
					for($var_nr=0; $var_nr<count($use_vars); $var_nr++)
						array_push($params, $row[$use_vars[$var_nr]]);
					$column_value = call_user_func_array($function_name,$params);
				}
			}elseif(isset($row[$names_in_array])){
				$column_value = $row[$names_in_array];
			}
			if(!strlen($column_value))
				$column_value = "";
			$column_value = "\"=\"\"".$column_value."\"\"\"";
			if ($col_nr)
				$column_value =  $sep.$column_value;
			fwrite($fh,$column_value);
			$col_nr++;
		}

		fwrite($fh,"\n");
	}
}

function write_in_file($fh, $formats, $array, $sep, $key_val_arr=true, $col_header=true)
{
	$col_nr = 0;
	
	if (!$formats && count($array))
		foreach($array[0] as $name=>$val)
			$formats[$name] = $name;
	if ($col_header && $formats!="no") {
		foreach($formats as $column_name => $var_name)
		{
			$exploded = explode(":",$column_name);
			if(count($exploded)>1)
				$name = $exploded[1];
			else{
				$name = $column_name;
				if(substr($column_name, 0, 9) == "function_")
					$name = substr($column_name,9);
				if(is_numeric($column_name))
					$name = $var_name;
			}
			$val = str_replace("_"," ",ucfirst($name));
			$val = str_replace("&nbsp;"," ",$val);
			$val = ($col_nr) ? $sep."\"$val\"" : "\"$val\"";
			fwrite($fh, $val);
			$col_nr++;
		} 
		fwrite($fh,"\n");
	}
	for($i=0; $i<count($array); $i++) 
	{
		$col_nr = 0;
		if ($key_val_arr) {
			foreach($formats as $column_name=>$names_in_array)
			{
				$use_vars = explode(",", $names_in_array);
				$exploded_col = explode(":", $column_name);
				$column_value = '';

				if(substr($exploded_col[0],0,9) == "function_") 
				{
					$function_name = substr($exploded_col[0],9,strlen($exploded_col[0]));
					if(count($use_vars)) 
					{
						$params = array();
						for($var_nr=0; $var_nr<count($use_vars); $var_nr++)
							array_push($params, $array[$i][$use_vars[$var_nr]]);
						$column_value = call_user_func_array($function_name,$params);
					}
				}elseif(isset($array[$i][$names_in_array])){
					$column_value = $array[$i][$names_in_array];
				}
				if(!strlen($column_value))
					$column_value = "";
				$column_value = "\"=\"\"".$column_value."\"\"\"";
				if ($col_nr)
					$column_value =  $sep.$column_value;
				fwrite($fh,$column_value);
				$col_nr++;
			}
		} else {
			for ($j=0; $j<count($array[$i]);$j++) {
				$column_value = "\"".$array[$i][$j]."\"";
				if ($col_nr)
					$column_value =  $sep.$column_value;
				fwrite($fh,$column_value);
				$col_nr++;
			}
		}
		fwrite($fh,"\n");
	}
}

// function used to check on_page authentication
// used in debug_all.php
function is_auth($identifier)
{
	global ${"pass_".$identifier."_page"};

	if (isset($_SESSION["pass_$identifier"]) || !isset(${"pass_".$identifier."_page"}) || !strlen(${"pass_".$identifier."_page"}))
		return true;
	return false;
}

// function used to check on_page authentication/authenticate
// used in debug_all.php
function check_auth($identifier)
{
	global ${"pass_".$identifier."_page"};

	if (strlen(${"pass_".$identifier."_page"}) && !isset($_SESSION["pass_$identifier"])) {
		$pass = (isset($_REQUEST["pass_$identifier"])) ? $_REQUEST["pass_$identifier"] : '';
		if ($pass != ${"pass_".$identifier."_page"}) 
			return false;
		$_SESSION["pass_$identifier"] = true;
	}
	return true;
}

function generateNumericToken($length)
{
	$str = "";
	for ($i=0; $i<$length; $i++)
	{
		$c = mt_rand(0,9);
		$str .= $c;
	}
	return $str;
}

function display_bit_field($val)
{
	if ($val=="1")
		return "yes";
	return "no";
}

function display_field($field_name,$field_format,$form_identifier='',$css=null)
{
	$q_mark = false;
	if(isset($field_format["value"]))
		$value = $field_format["value"];

	$display = (isset($field_format["display"])) ? $field_format["display"] : "text";
	$var_name = (isset($field_format[0])) ? $field_format[0] : $field_name;

	$res = "";
	switch($display)
	{
		case "select":
		case "mul_select":
		case "select_without_non_selected":
			$res .= '<select class="'.$css.'" name="'.$form_identifier.$field_name.'" id="'.$form_identifier.$field_name.'" ';
			if(isset($field_format["javascript"]))
				$res .= $field_format["javascript"];
			if($display == "mul_select")
				$res .= ' multiple="multiple" size="5"';
			$res .=  '>';
			if($display != "mul_select" && $display != "select_without_non_selected")
				$res .=  '<option value="">Not selected</option>';
			$options = (is_array($var_name)) ? $var_name : array();
			if(isset($options["selected"]))
				$selected = $options["selected"];
			elseif(isset($options["SELECTED"]))
				$selected = $options["SELECTED"];
			else
				$selected = '';
			foreach ($options as $var=>$opt) {
				if ($var === "selected" || $var === "SELECTED")
					continue;
				$css = (is_array($opt) && isset($opt["css"])) ? 'class="'.$opt["css"].'"' : "";
				if(is_array($opt) && isset($opt[$field_name.'_id'])) {
					$optval = $field_name.'_id';
					$name = $field_name;

					$printed = trim($opt[$name]);
					if (substr($printed,0,4) == "<img") {
						$expl = explode(">",$printed);
						$printed = $expl[1];
						$jquery_title = " title=\"".str_replace("<img","",$expl[0])."\"";
					} else
						$jquery_title = '';

					if ($opt[$optval] === $selected || (is_array($selected) && in_array($opt[$optval],$selected))) {
						$res .= '<option value=\''.$opt[$optval].'\' '.$css.' SELECTED ';
						if($opt[$optval] == "__disabled")
							print ' disabled="disabled"';
						$res .= $jquery_title;
						$res .= '>' . $printed . '</option>';
					} else {
						$res .= '<option value=\''.$opt[$optval].'\' '.$css;
						if($opt[$optval] == "__disabled")
							print ' disabled="disabled"';
						$res .= $jquery_title;
						$res .= '>' . $printed . '</option>';
					}
				}else{
					if ($opt == $selected ||  (is_array($selected) && in_array($opt[$optval],$selected)))
						$res .= '<option '.$css.' SELECTED >' . $opt . '</option>';
					else
						$res .= '<option '.$css.'>' . $opt . '</option>';
				}
			}
			$res .= '</select>';
			break;
		case "checkbox":
		case "checkbox-readonly":
			$res .= '<input class="'.$css.'" type="checkbox" name="'.$form_identifier.$field_name.'" id="'.$form_identifier.$field_name.'"';
			if($value == "t" || $value == "on" || $value=="1")
				$res .= " CHECKED ";
			if(isset($field_format["javascript"]))
				$res .= $field_format["javascript"];
			if ($display=="checkbox-readonly")
				$res .= " readonly=''";
			$res .= '/>';
			break;
		case "text":
		case "password":
		case "file":
		case "hidden":
		case "text-nonedit":
			$res .= '<input class="'.$css.'" type="'.$display.'" name="'.$form_identifier.$field_name.'" id="'.$form_identifier.$field_name.'"';
			if($display != "file" && $display != "password")
				$res .= ' value="'.$value.'"';
			if(isset($field_format["javascript"]))
				$res .= $field_format["javascript"];
			if($display == "text-nonedit")
				$res .= " readonly=''";
			if(isset($field_format["autocomplete"]))
				$res .= " autocomplete=\"".$field_format["autocomplete"]."\"";
			if($display != "hidden" && isset($field_format["comment"])) {
				$q_mark = true;
				$res .= '>&nbsp;&nbsp;<img class="pointer" src="images/question.jpg" onClick="show_hide_comment(\''.$form_identifier.$field_name.'\');"/>';;
			} else
				$res .= '>';
			break;
		case "fixed":
			if(strlen($value))
				$res .= $value;
			else
				$res .= "&nbsp;";
			break;
	}
	if(isset($field_format["comment"]))
	{
		$comment = $field_format["comment"];
		if(!$q_mark)
			$res .= '&nbsp;&nbsp;<img class="pointer" src="images/question.jpg" onClick="show_hide_comment(\''.$form_identifier.$field_name.'\');"/>';
		$res .= '<font class="comment" style="display:none;" id="comment_'.$form_identifier.$field_name.'">'.$comment.'</font>';
	}
	return $res;
}

function stdClassToArray($d) {
	if (is_object($d)) {
		// Gets the properties of the given object
		// with get_object_vars function
		$d = get_object_vars($d);
	}

	if (is_array($d)) {
		/*
		* Return array converted to object
		* Using __FUNCTION__ (Magic constant)
		* for recursive call
		*/
		return array_map(__FUNCTION__, $d);
	}
	else {
		// Return array
		return $d;
	}
}

function get_param($array,$name)
{
	return isset($array[$name]) ? $array[$name] : null;
}

function missing_param($param)
{
	return ($param === null) || ($param == "");
}

function generic_tabbed_settings($options,$config,$section_to_open=array(),$show_advanced=false, $form_name=null,$form_submit=true,$specif_ind="",$custom_css=null)
{
	global $module, $tabs_callback;

	if (!$form_name)
		$form_name = $module;

        print '<table class="sections '.$custom_css.'" cellspacing="0" cellpadding="0">'."\n";
        print '<tr>'."\n";
	$total_options = count($options);
	$disp_show = "";
	$disp_hide = ' style="display:none;"';

	$initial_show_advanced = $show_advanced;
	if (!isset($section_to_open["-"]))
		$show_advanced = true;

	$first_open = NULL;
	foreach($section_to_open as $sect_name=>$def) {
		$first_open = $sect_name;
		$first_subsec = (!strlen($def)) ? "-" : $def;
		break;
	}

        for ($i=0; $i<$total_options; $i++) {
		$name = $options[$i];
		$js_name = $specif_ind.$i;

		$css = ($first_open!=$name && !(!$i && $first_open=="-")) ? "section" : "section_selected";
		if (!$i)
			$css .= " basic";
		$css .= " $custom_css";
		if ($first_open==$name || ($first_open=="-" && !$i))
			$css .= "_selected";
		$td_css = (!$i) ? "section basic $custom_css" : "$custom_css";

		print '<td class="'.$td_css.'" id="td_'.$js_name.'">';
		if ($i) {
			print '<div class="fill_section" id="fill_'.$js_name.'" ';
			if ($show_advanced)
				print $disp_hide;
			print '>&nbsp;</div>';
		}


		print '<div class="'.$css.'" onClick="show_section(\''.$i.'\','.$total_options.',\''.$specif_ind.'\',\''.$custom_css.'\');" id="tab_'.$js_name.'" ';
		if ($i && !$show_advanced)
			print $disp_hide;
		print '>'.ucwords($name);

		if (!$i && $initial_show_advanced!==NULL) {
			$img = ($show_advanced) ? "sm_hide.png" : "sm_show.png";
			print "&nbsp;&nbsp;<img src=\"images/$img\" class=\"pointer\" id=\"img_show_tabs$specif_ind\" onclick=\"show_all_tabs($total_options,'$specif_ind');\" width=\"11px\" height=\"11px\"/>";
		}

		print '</div></td>'."\n";
                print '<td class="space_section">&nbsp;</td>'."\n";
        }
	print '<td class="fillspace_section">&nbsp;</td>'."\n";
        print '</tr>'."\n";
        print '<tr>'."\n";
	$colspan = $total_options*2+1;
        print '<td class="section_content" colspan="'.$colspan.'">'."\n";

	for ($i=0; $i<$total_options; $i++) {
		$name = $options[$i];
		$js_name = $specif_ind.$i;
		$current_fields = $config[$options[$i]];
		$disp = ($first_open!=$options[$i] && !(!$i && $first_open=="-")) ? " style='display:none;'" : '';

		print "<div class='section_fields' id='sect_$js_name'$disp>";
		if (isset($current_fields["subsections"])) {
			$sub_sections = $current_fields["subsections"];
			$names = array();
			foreach ($sub_sections as $name=>$fields)
				$names[] = $name;
			generic_tabbed_settings($names, $sub_sections, array("$first_subsec"=>""), false, "", false, $js_name."_", "subsec");
		} else {
			if (isset($tabs_callback))
				$tabs_callback($name, $current_fields);
			editObject(NULL,$current_fields,NULL,"no");
		}
		print "</div>";
	}

	if ($form_submit===true) {
		print '<div class="section_submit">';
		//print '<input type="button" value="Save" onClick="submit_form(\''.$form_name.'\')" />&nbsp;';
		//print '<input type="reset" value="Reset" onClick="reset_form(\''.$form_name.'\')"/>&nbsp;';
		print '<input type="submit" value="Save" />&nbsp;';
		print '<input type="reset" value="Reset" />&nbsp;';
		print cancel_button('','Cancel');
		print '&nbsp;</div>';
	} elseif (is_array($form_submit)) {
		print '<div class="section_submit">';
		foreach($form_submit as $name=>$func)
			if ($func!="no_cancel")
				print '<input type="button" value="'.$name.'" onClick="'.$func.'(\''.$form_name.'\')" />&nbsp;';
		if (!in_array("no_cancel",$form_submit))
			print cancel_button('','Cancel');
		print '&nbsp;</div>';
	}
	print '</td>';
	print '</tr>';
	print '</table>';
}

function load_page($page)
{
	if (headers_sent())
		print "<meta http-equiv=\"REFRESH\" content=\"0;url=$page\">";
	else
		header("Location: $page");
}

/* vi: set ts=8 sw=4 sts=4 noet: */
?>
