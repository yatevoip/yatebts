/**
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

function toggle_column(element)
{
	form = document.forms["current_form"];
	for(var z=0; z<form.length;z++) {
		if (form[z].type != 'checkbox')
			continue;
		if (form[z].disabled == true)
			continue;
		form[z].checked = element.checked;
	}
}

function show_all_tabs(count_sections,partids)
{
	if (partids==null)
		partids="";
	var img = document.getElementById("img_show_tabs"+partids);
	if (img.src.substr(-12)=="/sm_show.png")
		img.src = "images/sm_hide.png";
	else
		img.src = "images/sm_show.png";
	var i;
	for (i=1; i<count_sections; i++) {
		section_tab = document.getElementById("tab_"+partids+i);
		section_fill = document.getElementById("fill_"+partids+i);
		if (section_tab==null) {
			alert("Don't have section tab for "+partids+i);
			continue;
		}
		if (section_tab.style.display=="") {
			section_tab.style.display = "none";
			if (section_fill!=null)
				section_fill.style.display = "";
		} else {
			section_tab.style.display = "";
			if (section_fill!=null)
				section_fill.style.display = "none";
		}
	}
}

function show_section(section_name,count_sections,partids,custom_css)
{
	//alert(section_name);

	if (partids==null)
		partids="";
	if (custom_css==null)
		custom_css="";

	var i, section_div, section_tab;
	for (i=0; i<count_sections; i++) {
		section_tab = document.getElementById("tab_"+partids+i);
		section_div = document.getElementById("sect_"+partids+i);
		if (section_tab==null) {
			alert("Don't have section tab for "+i);
			continue;
		}
		if (section_div==null) {
			alert("Don't have section div for "+i);
			continue;
		}
		if (i==section_name) {
			if (i==0)
				section_tab.className = "section_selected basic "+custom_css+"_selected";
			else
				section_tab.className = "section_selected "+custom_css+"_selected";
			section_div.style.display = "";
		} else {
			cls = section_tab.className;
			if (cls.substr(0,16)=="section_selected") {
				if (i==0)
					section_tab.className = "section basic "+custom_css;
				else
					section_tab.className = "section "+custom_css;
				section_div.style.display = "none";
			}
		}
	}
}

function show_hide_comment(id)
{
	var fontvr = document.getElementById("comment_"+id);
	if(fontvr == null)
		return;
	if (fontvr.style.display == "none")
		fontvr.style.display = "block";
	else
		if(fontvr.style.display == "block")
			fontvr.style.display = "none";
}

function show_hide(element)
{
	var div = document.getElementById(element);

	if (div.style.display == "none") {
		if(div.tagName == "TR")
			div.style.display = (ie > 1 && ie<8) ? "block" : "table-row";//"block";//"table-row";
		else
			if(div.tagName == "TD")
				div.style.display = (ie > 1 && ie<8) ? "block" : "table-cell";
			else
				if (div.tagName=="IMG")
					div.style.display="";
				else
					div.style.display = "block";
	}else{
		div.style.display = "none";
	}
}

function submit_form(formid)
{
	document.getElementById(formid).submit();
}
