/**
 * lib_str_util.js
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * String utility functions library for Javascript
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

// Parse a string as boolean value
function parseBool(str,defVal)
{
    switch (str) {
	case false:
	case "false":
	case "no":
	case "off":
	case "disable":
	case "f":
	    return false;
	case true:
	case "true":
	case "yes":
	case "on":
	case "enable":
	case "t":
	    return true;
    }
    if (defVal === undefined)
	return false;
    return defVal;
}

// Helper that returns a left or right aligned fixed length string
function strFix(str,len,pad)
{
    if (str === null)
	str = "";
    if (pad == "")
	pad = " ";
    if (len < 0) {
	// right aligned
	len = -len;
	if (str.length >= len)
	    return str.substr(str.length - len);
	while (str.length < len)
	    str = pad + str;
    }
    else {
	// left aligned
	if (str.length >= len)
	    return str.substr(0,len);
	while (str.length < len)
	    str += pad;
    }
    return str;
}

// Returns 16 bit (or more) hex value, false if not a number
function get16bitHexVal(val)
{
    val = parseInt(val);
    if (isNaN(val))
	return false;
    return val.toString(16,4);
}

// Perform one command line completion
function oneCompletion(msg,str,part)
{
    if (part != "" && str.indexOf(part) != 0)
	return;
    var ret = msg.retValue();
    if (ret != "")
	ret += "\t";
    msg.retValue(ret + str);
}

/* vi: set ts=8 sw=4 sts=4 noet: */
