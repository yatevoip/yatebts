/**
 * subscribers.js
 * Configure subscribers individually or set regular expression to accept imsis
 *
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

/*
// Note! Subscribers are accepted by either matching the IMSI against a configured regular expression 
// or by individually configuring each acceptable IMSI

var subscribers = {
"001990010001014":
{
    "msisdn":"074434", // oficial phone number
    "active":1,
    "ki":"00112233445566778899aabbccddeeff",
    "op":"00000000000000000000000000000000",
    "imsi_type":"3G",
    "short_number":"" //short number to be called by other yatebts-nib users
},
"001990010001015":
{
    "msisdn":"074434", // oficial phone number
    "active":1,
    "ki":"00112233445566778899aabbccddeeff",
    "op":"",
    "imsi_type":"2G",
    "short_number":"" //short number to be called by other yatebts-nib users
}
};
*/

// Note! If a regular expression is used, 2G/3G authentication cannot be used. 
// To use 2G/3G authentication, please set subscribers individually.
var regexp = /^001/;