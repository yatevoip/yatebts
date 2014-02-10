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
var subscribers = {
"00101000000000":
{
    "imsi":"00101000000000",
    "msisdn":"074433", // oficial phone number
    "active":1,
    "ki":"3",
    "op":""
    "short_number":"45454" //short number to be called by other yatebts-nib users
}
};
*/

var regexp = /^001/;