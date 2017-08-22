/**
 * javascript.js
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


function getInternetExplorerVersion()
// Returns the version of Internet Explorer or a -1
// (indicating the use of another browser).
{
  var rv = -1; // Return value assumes failure.
  if (navigator.appName == 'Microsoft Internet Explorer')
  {
    var ua = navigator.userAgent;
    var re  = new RegExp("MSIE ([0-9]{1,}[\.0-9]{0,})");
    if (re.exec(ua) != null)
      rv = parseFloat( RegExp.$1 );
  }
  return rv;
}

function form_for_gateway(gwtype)
{
	//var sprot = document["forms"]["outbound"][gwtype+"protocol"];
	var sprot = document.getElementById(gwtype+"protocol");
	var sprotocol = sprot.options[sprot.selectedIndex].value || sprot.options[sprot.selectedIndex].text;
	var protocols = new Array("sip", "h323", "iax", "pstn", "BRI", "PRI");
	var i;
	var currentdiv;
	var othergw;

	if(gwtype == "reg")
		othergw = "noreg";
	else
		othergw = "reg";

	for(var i=0; i<protocols.length; i++) 
	{
		currentdiv = document.getElementById("div_"+gwtype+"_"+protocols[i]);
		if(currentdiv == null)
			continue;
		if(currentdiv.style.display == "block")
			currentdiv.style.display = "none";
	}
	for(var i=0; i<protocols.length; i++) 
	{
		currentdiv = document.getElementById("div_"+othergw+"_"+protocols[i]);
		if(currentdiv == null)
			continue;
		if(currentdiv.style.display == "block")
			currentdiv.style.display = "none";
	}
	currentdiv = document.getElementById("div_"+gwtype+"_"+sprotocol);
	if(currentdiv == null)
		return false;
	if(currentdiv.style.display == "none")
		currentdiv.style.display = "block";
}

function advanced(identifier)
{
	var elems = document.outbound.elements;
	var elem_name;
	var elem;

	var ie = getInternetExplorerVersion();

	var not_advanced = ["imsi","iccid","opc","ki","smsc"]; 
	for(var i=0;i<elems.length;i++)
	{
		elem_name = elems[i].name;
		if(in_array(elem_name, not_advanced))
			continue;
		if(identifier.length < elem_name.length && elem_name.substr(0,identifier.length) != identifier)
			continue;
		var elem = document.getElementById("tr_"+elem_name); 

		if(elem == null)
			continue;
		if(elem.style.display == null || elem.style.display == "")
			continue;
		if(elem.style.display == "none")
			elem.style.display = (ie > 1 && ie < 8) ? "block" : "table-row";
		else
			// specify the display property (the elements that are not advanced will have display="")
			if(elem.style.display == "block" || elem.style.display == "table-row")
				elem.style.display = "none";
	}

	var img = document.getElementById(identifier+"xadvanced");
	var imgsrc= img.src;
	var imgarray = imgsrc.split("/");
	if(imgarray[imgarray.length - 1] == "advanced.jpg"){
		imgarray[imgarray.length - 1] = "basic.jpg";
		img.title = "Hide advanced fields";
	}else{
		imgarray[imgarray.length - 1] = "advanced.jpg";
		img.title = "Show advanced fields";
	}

	img.src = imgarray.join("/");
}

function show_submenu_tabs(id, total, name)
{
	for(var i=0; i<total; i++) {
		document.getElementById("section_"+i).className = 'menu_close';
		if (document.getElementById("submenu_"+i))
			document.getElementById("submenu_"+i).style.display = 'none';
	}

	document.getElementById("section_"+id).className = 'menu_open';
	if (document.getElementById("submenu_"+id))
		document.getElementById("submenu_"+id).style.display = 'block';

	show_submenu_fields(name);

	if (in_array(name, sect_with_subsect)) {
		if (document.getElementById("submenu_line"))
			document.getElementById("submenu_line").className = 'submenu';
	} else {
		document.getElementById("submenu_line").className = 'submenu_no_line';
	}

}

function show_submenu_fields(name)
{
	//  subsections variable is now built dynamically depending on the detected section in ybts_fields
        //	var subsections = ["gsm", "gsm_advanced","gprs","gprs_advanced","sgsn","ggsn", "control", "transceiver", "tapping", "test","ybts","security"];
	var forms_ids = ["","info_","err_","file_err_","notice_"];
	
	for (var i=0; i<subsections.length; i++) {
		for (var j=0; j<forms_ids.length; j++) {
			if (document.getElementById("tab_"+subsections[i]) && document.getElementById("tab_"+subsections[i]).className == 'submenu_open')
				document.getElementById("tab_"+subsections[i]).className = 'submenu_close';
	  		if (document.getElementById(forms_ids[j]+subsections[i]) && document.getElementById(forms_ids[j]+subsections[i]).style.display != "none")
				document.getElementById(forms_ids[j]+subsections[i]).style.display = "none";

			if (subsections[i] == name) {
				if (document.getElementById("tab_"+subsections[i]))
					document.getElementById("tab_"+subsections[i]).className = 'submenu_open';
				if (document.getElementById(forms_ids[j]+subsections[i]) && forms_ids[j]!= "notice_")
					document.getElementById(forms_ids[j]+subsections[i]).style.display = "block";
			}
		}
	}

}

function show_hide_cols()
{
	var cols = ["tr_imsi", "tr_iccid", "tr_ki", "tr_opc","tr_smsc"];
	 for (var i=0;i<cols.length;i++) {
		 if (!document.getElementById(cols[i]))
			 continue;
		show_hide(cols[i]);
	 }
}

function show_hide_op()
{
	if (document.getElementById("imsi_type").value == "3G")
		show_hide("tr_op");
	return;
}

function show_hide(element)
{
	var ie = getInternetExplorerVersion();
	var div = document.getElementById(element);

	if (div.style.display == "none") {
		if(div.tagName == "TR")
			div.style.display = (ie > 1 && ie<8) ? "block" : "table-row";//"block";//"table-row";
		else
			if(div.tagName == "TD")
				div.style.display = (ie > 1 && ie<8) ? "block" : "table-cell";
			else
				div.style.display = "block";
	}else{
		div.style.display = "none";
	}
}

function show_hide_pysim_traceback(pysim_err)
{
	show_hide(pysim_err);
	if (document.getElementById(pysim_err).style.display == "none") {
		document.getElementById("err_pysim").innerHTML = "For full PySim traceback <div id=\"err_link_pysim\" class=\"error_link\" onclick=\"show_hide_pysim_traceback('pysim_err')\" style=\"cursor:pointer;\">click here</div></div>";
	} else {
		document.getElementById("err_pysim").innerHTML = "To hide full traceback <div id=\"err_link_pysim\" class=\"error_link\" onclick=\"show_hide_pysim_traceback('pysim_err')\" style=\"cursor:pointer;\">click here</div>";
	}
}

function in_array(needle, haystack) 
{
    var length = haystack.length;
    for (var i = 0; i < length; i++) {
        if (haystack[i] == needle) 
		return true;
    }
    return false;
}
/*
function advanced(identifier)
{
	var form = document.getElementById(identifier);
	var elems = form.elements;
	var elem_name;
	var elem;

	var ie = getInternetExplorerVersion();

	for(var i=0;i<elems.length;i++)
	{
		elem_name = elems[i].name;
		if(identifier.length > elem_name.length && elem_name.substr(0,identifier.length) != identifier)
			continue;
		var elem = document.getElementById("tr_"+elem_name); 
		if(elem == null)
			continue;
		if(elem.style.display == null || elem.style.display == "")
			continue;
		if(elem.style.display == "none")
			elem.style.display = (ie > 1 && ie < 8) ? "block" : "table-row";
		else
			// specify the display property (the elements that are not advanced will have display="")
			if(elem.style.display == "block" || elem.style.display == "table-row")
				elem.style.display = "none";
	}

	var img = document.getElementById(identifier+"xadvanced");
	var imgsrc= img.src;
	var imgarray = imgsrc.split("/");
	if(imgarray[imgarray.length - 1] == "advanced.jpg"){
		imgarray[imgarray.length - 1] = "basic.jpg";
		img.title = "Hide advanced fields";
	}else{
		imgarray[imgarray.length - 1] = "advanced.jpg";
		img.title = "Show advanced fields";
	}

	img.src = imgarray.join("/");
}*/
