/**
 * ybts_fields.js
 * This file is part of the YATE Project http://YATE.null.ro
 *
 * Yet Another Telephony Engine - a fully featured software PBX and IVR
 * Copyright (C) 2016 Null Team
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

//#pragma trace "cachegrind.out.ybts_fields"

#require "generic_validations.js"

/*---------------  YbtsFields validations  ---------------*/
YbtsConfig.prototype.validations =  { 
    "gsm": {
	"Radio.Band": {"select": ["850", "900", "1800", "1900"]},
	"Radio.C0": {"callback": checkRadioBand},
	"Identity.MCC": {"regex": "^[0-9]{3}$"},
	"Identity.MNC": {"regex": "^[0-9]{2,3}$"},
	"Identity.LAC": {"minimum": 0, "maximum": 65280}, 
	"Identity.CI": {"minimum": 0, "maximum": 65535}, 
	"Identity.BSIC.NCC": {"minimum": 0, "maximum": 7},
	"Identity.BSIC.BCC": {"minimum": 0, "maximum": 7},
	"Identity.ShortName": {"regex": "^[a-zA-Z0-9]+$"},	
	"Radio.PowerManager.MaxAttenDB": {"minimum": 0, "maximum": 80}, 
	"Radio.PowerManager.MinAttenDB": {"callback": checkRadioPowerManager}
    },
    "gsm_advanced": { 
	"Channels.NumC1s": {"minimum": 0, "maximum": 100},
	"Channels.NumC7s": {"minimum": 0, "maximum": 100},
	"Channels.C1sFirst": {"callback": checkOnOff},
	"Channels.SDCCHReserve": {"minimum": 0, "maximum": 10},
	"CCCH.AGCH.QMax": {"minimum": 3, "maximum": 8},
	"CCCH.CCCH-CONF": {"minimum": 1, "maximum": 2},
	"CellOptions.RADIO-LINK-TIMEOUT": {"minimum": 10, "maximum": 20},
	"CellSelection.CELL-RESELECT-HYSTERESIS": {"minimum": 0, "maximum": 7},
	"CellSelection.MS-TXPWR-MAX-CCH": {"minimum": 0, "maximum": 31},
	"CellSelection.NCCsPermitted": {"minimum": -1, "maximum": 255},
	"CellSelection.NECI": {"select": ["0", "1"]},
	"CellSelection.RXLEV-ACCESS-MIN": {"minimum": 0, "maximum": 63},
	"Cipher.CCHBER": {"select": ["0.0", "0.1", "0.2", "0.3", "0.4", "0.5", "0.6", "0.7", "0.8", "0.9", "1.0"]},
	"Cipher.Encrypt": {"callback": checkOnOff},
	"Cipher.RandomNeighbor": {"select": ["0.0", "0.1", "0.2", "0.3", "0.4", "0.5", "0.6", "0.7", "0.8", "0.9", "1.0"]},
	"Cipher.ScrambleFiller": {"callback": checkOnOff},
	"Control.GPRSMaxIgnore": {"minimum": 3, "maximum": 8},
	"Handover.InitialHoldoff": {"select": ["2000", "3000", "4000", "5000", "6000", "7000"]},
	"Handover.LocalRSSIMin": {"minimum": -100, "maximum": -50},
	"Handover.ThresholdDelta": {"minimum": 5, "maximum": 20},
	"MS.Power.Damping": {"minimum": 25, "maximum": 75},
	"MS.Power.Max": {"minimum": 5, "maximum": 100},
	"MS.Power.Min": {"minimum": 5, "maximum": 100},
	"MS.TA.Damping": {"minimum": 25, "maximum": 75},
	"MS.TA.Max": {"minimum": 1, "maximum": 62},
	"MaxSpeechLatency": {"minimum": 1, "maximum": 5},
	"Neighbors.NumToSend": {"minimum": 1, "maximum": 10},
	"Ny1": {"minimum": 1, "maximum": 10},
	"RACH.AC": {"callback": checkRachAc},
	"RACH.MaxRetrans": {"minimum": 0, "maximum": 3},
	"RACH.TxInteger": {"minimum": 0, "maximum": 15},
	"Radio.ARFCNs": {"minimum": 1, "maximum": 10},
	"Radio.MaxExpectedDelaySpread": {"minimum": 1, "maximum": 4},
	"Radio.NeedBSIC": {"callback": checkOnOff},
	"Radio.PowerManager.NumSamples": {"minimum": 5, "maximum": 20},
	"Radio.PowerManager.Period": {"minimum": 4500, "maximum": 7500},
	"Radio.PowerManager.SamplePeriod": {"minimum": 1500, "maximum": 2500},
	"Radio.PowerManager.TargetT3122": {"minimum": 3750, "maximum": 6250},
	"Radio.RSSITarget": {"callback": checkRadioRssiTarget},
	"Radio.RxGain": {"select": ["", "Factory calibrated", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "70", "71", "72", "73", "74", "75"]},
	"ShowCountry": {"callback": checkOnOff},
	"Timer.T3103": {"select": ["2500", "2600", "2700", "2800", "2900", "3000", "3100", "3200", "3300", "3400", "3500", "3600", "3700", "3800", "3900", "4000", "4100", "4200", "4300", "4400", "4500", "4600", "4700", "4800", "4900", "5000", "5100", "5200", "5300", "5400", "5500", "5600", "5700", "5800", "5900", "6000", "6100", "6200", "6300", "6400", "6500", "6600", "6700", "6800", "6900", "7000", "7100", "7200", "7300", "7400", "7500"]},
	"Timer.T3105": {"select": ["25", "30", "35", "40", "45", "50", "55", "60", "65", "70", "75"]},
	"Timer.T3113": {"select": ["5000", "5500", "6000", "6500", "7000", "7500", "8000", "8500", "9000", "9500", "10000", "10500", "11000", "11500", "12000", "12500", "13000", "13500", "14000", "14500", "15000"]},
	"Timer.T3122Max": {"minimum": 127500, "maximum": 382500},
	"Timer.T3122Min": {"select": ["1000", "1100", "1200", "1300", "1400", "1500", "1600", "1700", "1800", "1900", "2000", "2100", "2200", "2300", "2400", "2500", "2600", "2700", "2800", "2900", "3000"]},
	"Timer.T3212": {"select": ["0", "6", "12", "18", "24", "30", "36", "42", "48", "54", "60", "66", "72", "78", "84", "90", "96", "102", "108", "114", "120", "126", "132", "138", "144", "150", "156", "162", "168", "174", "180", "186", "192", "198", "204", "210", "216", "222", "228", "234", "240", "246", "252", "258", "264", "270", "276", "282", "288", "294", "300", "306", "312", "318", "324", "330", "336", "342", "348", "354", "360", "366", "372", "378", "384", "390", "396", "402", "408", "414", "420", "426", "432", "438", "444", "450", "456", "462", "468", "474", "480", "486", "492", "498", "504", "510", "516", "522", "528", "534", "540", "546", "552", "558", "564", "570", "576", "582", "588", "594", "600", "606", "612", "618", "624", "630", "636", "642", "648", "654", "660", "666", "672", "678", "684", "690", "696", "702", "708", "714", "720", "726", "732", "738", "744", "750", "756", "762", "768", "774", "780", "786", "792", "798", "804", "810", "816", "822", "828", "834", "840", "846", "852", "858", "864", "870", "876", "882", "888", "894", "900", "906", "912", "918", "924", "930", "936", "942", "948", "954", "960", "966", "972", "978", "984", "990", "996", "1002", "1008", "1014", "1020", "1026", "1032", "1038", "1044", "1050", "1056", "1062", "1068", "1074", "1080", "1086", "1092", "1098", "1104", "1110", "1116", "1122", "1128", "1134", "1140", "1146", "1152", "1158", "1164", "1170", "1176", "1182", "1188", "1194", "1200", "1206", "1212", "1218", "1224", "1230", "1236", "1242", "1248", "1254", "1260", "1266", "1272", "1278", "1284", "1290", "1296", "1302", "1308", "1314", "1320", "1326", "1332", "1338", "1344", "1350", "1356", "1362", "1368", "1374", "1380", "1386", "1392", "1398", "1404", "1410", "1416", "1422", "1428", "1434", "1440", "1446", "1452", "1458", "1464", "1470", "1476", "1482", "1488", "1494", "1500", "1506", "1512", "1518", "1524", "1530"]}
    },
    "gprs": {
	"Enable": {"callback": checkOnOff},
	"RAC": {"minimum": 0, "maximum": 255},
	"RA_COLOUR": {"minimum": 0, "maximum": 7}
    },
    "gprs_advanced": {
	"Debug": {"callback": checkOnOff},
	"MS.Power.Alpha": {"minimum": 0, "maximum": 10},
	"MS.Power.Gamma": {"minimum": 0, "maximum": 31},
	"MS.Power.T_AVG_T": {"minimum": 0, "maximum": 25},
	"MS.Power.T_AVG_W": {"minimum": 0, "maximum": 25},
	"Multislot.Max.Downlink": {"minimum": 0, "maximum": 10},
	"Multislot.Max.Uplink": {"minimum": 0, "maximum": 10},
	"Uplink.KeepAlive": {"select": ["200", "300", "400", "500", "600", "700", "800", "900", "1000", "1100", "1200", "1300", "1400", "1500", "1600", "1700", "1800", "1900", "2000", "2100", "2200", "2300", "2400", "2500", "2600", "2700", "2800", "2900", "3000", "3100", "3200", "3300", "3400", "3500", "3600", "3700", "3800", "3900", "4000", "4100", "4200", "4300", "4400", "4500", "4600", "4700", "4800", "4900", "5000"]},
	"Uplink.Persist": {"callback": checkUplinkPersistent},
	"TBF.Downlink.Poll1": {"minimum": 0, "maximum": 15},
	"TBF.EST": {"callback": checkOnOff},
	"TBF.Expire": {"minimum": 20000, "maximum": 40000},
	"TBF.KeepExpiredCount": {"minimim": 15, "maximum": 25},
	"TBF.Retry": {"minimum": 0, "maximum": 4},
	"advanceblocks": {"minimim": 5, "maximum": 15},
	"CellOptions.T3168Code": {"minimim": 0,"maximum": 7},
	"CellOptions.T3192Code": {"minimim": 0,"maximum": 7},
	"ChannelCodingControl.RSSI": {"callback": checkChannelcodingcontrolRssi},
	"Channels.Congestion.Threshold": {"select": ["100", "105", "110", "115", "120", "125", "130", "135", "140", "145", "150", "155", "160", "165", "170", "175", "180", "185", "190", "195", "200", "205", "210", "215", "220", "225", "230", "235", "240", "245", "250", "255", "260", "265", "270", "275", "280", "285", "290", "295", "300"]},
	"Channels.Congestion.Timer": {"select": ["30", "35", "40", "45", "50", "55", "60", "65", "70", "75", "80", "85", "90"]},
	"Channels.Min.C0": {"minimum": 0, "maximum": 7},
	"Channels.Min.CN": {"minimum": 0,"maximum": 100},
	"Channels.Max": {"minimum": 0, "maximum": 10},
	"Counters.Assign": {"minimum": 5, "maximum": 15},
	"Counters.N3101": {"minimum": 8, "maximum": 32},
	"Counters.N3103": {"minimum": 4, "maximum": 12},
	"Counters.N3105": {"minimum": 6, "maximum": 18},
	"Counters.Reassign": {"minimum": 3, "maximum": 9},
	"Counters.TbfRelease": {"minimum": 3, "maximum": 8},
	"Downlink.KeepAlive": {"select": ["200", "300", "400", "500", "600", "700", "800", "900", "1000", "1100", "1200", "1300", "1400", "1500", "1600", "1700", "1800", "1900", "2000", "2100", "2200", "2300", "2400", "2500", "2600", "2700", "2800", "2900", "3000", "3100", "3200", "3300", "3400", "3500", "3600", "3700", "3800", "3900", "4000", "4100", "4200", "4300", "4400", "4500", "4600", "4700", "4800", "4900", "5000"]},
	"Downlink.Persist": {"callback": checkDownlinkPersistent},
	"LocalTLLI.Enable": {"callback": checkOnOff},
	"MS.KeepExpiredCount": {"minimum": 10, "maximum": 30},
	"NC.NetworkControlOrder": {"minimum": 0, "maximum": 3},
	"NMO": {"minimum": 1, "maximum": 3},
	"PRIORITY-ACCESS-THR": {"select": ["0", "3", "4", "5", "6"]},
	"RRBP.Min": {"minimum": 0, "maximum": 3}, 
	"Reassign.Enable": {"callback": checkOnOff},
	"SendIdleFrames": {"callback": checkOnOff},
	"Timers.Channels.Idle": {"select": ["3000", "3100", "3200", "3300", "3400", "3500", "3600", "3700", "3800", "3900", "4000", "4100", "4200", "4300", "4400", "4500", "4600", "4700", "4800", "4900", "5000", "5100", "5200", "5300", "5400", "5500", "5600", "5700", "5800", "5900", "6000", "6100", "6200", "6300", "6400", "6500", "6600", "6700", "6800", "6900", "7000", "7100", "7200", "7300", "7400", "7500", "7600", "7700", "7800", "7900", "8000", "8100", "8200", "8300", "8400", "8500", "8600", "8700", "8800", "8900", "9000"]},
	"Timers.MS.Idle": {"select": ["300", "310", "320", "330", "340", "350", "360", "370", "380", "390", "400", "410", "420", "430", "440", "450", "460", "470", "480", "490", "500", "510", "520", "530", "540", "550", "560", "570", "580", "590", "600", "610", "620", "630", "640", "650", "660", "670", "680", "690", "700", "710", "720", "730", "740", "750", "760", "770", "780", "790", "800", "810", "820", "830", "840", "850", "860", "870", "880", "890", "900"]},
	"Timers.MS.NonResponsive": {"select": ["3000", "3100", "3200", "3300", "3400", "3500", "3600", "3700", "3800", "3900", "4000", "4100", "4200", "4300", "4400", "4500", "4600", "4700", "4800", "4900", "5000", "5100", "5200", "5300", "5400", "5500", "5600", "5700", "5800", "5900", "6000", "6100", "6200", "6300", "6400", "6500", "6600", "6700", "6800", "6900", "7000", "7100", "7200", "7300", "7400", "7500", "7600", "7700", "7800", "7900", "8000", "8100", "8200", "8300", "8400", "8500", "8600", "8700", "8800", "8900", "9000"]},
	"Timers.T3169": {"select": ["2500", "2600", "2700", "2800", "2900", "3000", "3100", "3200", "3300", "3400", "3500", "3600", "3700", "3800", "3900", "4000", "4100", "4200", "4300", "4400", "4500", "4600", "4700", "4800", "4900", "5000", "5100", "5200", "5300", "5400", "5500", "5600", "5700", "5800", "5900", "6000", "6100", "6200", "6300", "6400", "6500", "6600", "6700", "6800", "6900", "7000", "7100", "7200", "7300", "7400", "7500"]},
	"Timers.T3191": {"select": ["2500", "2600", "2700", "2800", "2900", "3000", "3100", "3200", "3300", "3400", "3500", "3600", "3700", "3800", "3900", "4000", "4100", "4200", "4300", "4400", "4500", "4600", "4700", "4800", "4900", "5000", "5100", "5200", "5300", "5400", "5500", "5600", "5700", "5800", "5900", "6000", "6100", "6200", "6300", "6400", "6500", "6600", "6700", "6800", "6900", "7000", "7100", "7200", "7300", "7400", "7500"]},
	"Timers.T3193": {"select": ["0", "100", "200", "300", "400", "500", "600", "700", "800", "900", "1000", "1100", "1200", "1300", "1400", "1500", "1600", "1700", "1800", "1900", "2000", "2100", "2200", "2300", "2400", "2500", "2600", "2700", "2800", "2900", "3000", "3100", "3200", "3300", "3400", "3500", "3600", "3700", "3800", "3900", "4000", "4100", "4200", "4300", "4400", "4500", "4600", "4700", "4800", "4900", "5000"]},
	"Timers.T3195": {"select": ["2500", "2600", "2700", "2800", "2900", "3000", "3100", "3200", "3300", "3400", "3500", "3600", "3700", "3800", "3900", "4000", "4100", "4200", "4300", "4400", "4500", "4600", "4700", "4800", "4900", "5000", "5100", "5200", "5300", "5400", "5500", "5600", "5700", "5800", "5900", "6000", "6100", "6200", "6300", "6400", "6500", "6600", "6700", "6800", "6900", "7000", "7100", "7200", "7300", "7400", "7500"]}
    },
    "sgsn": {
	"Debug": {"callback": checkOnOff},
	"Timer.ImplicitDetach": {"callback": checkTimerImplicitdetach},
	"Timer.MS.Idle": {"select": ["300", "310", "320", "330", "340", "350", "360", "370", "380", "390", "400", "410", "420", "430", "440", "450", "460", "470", "480", "490", "500", "510", "520", "530", "540", "550", "560", "570", "580", "590", "600", "610", "620", "630", "640", "650", "660", "670", "680", "690", "700", "710", "720", "730", "740", "750", "760", "770", "780", "790", "800", "810", "820", "830", "840", "850", "860", "870", "880", "890", "900"]},
	"Timer.RAUpdate": {"callback": checkTimerRaupdate},
	"Timer.Ready": {"minimum": 30, "maximum": 90}
    },
    "ggsn": {
	"dns": {"callback": checkValidDns},
	"Firewall.Enable": {"minimum": 0, "maximum": 2},
	"IP.MaxPacketSize": {"minimum": 1492, "maximum": 9000},
	"IP.ReuseTimeout": {"minimum": 120, "maximum": 240},
	"IP.TossDuplicatePackets": {"callback": checkOnOff},
	"MS.IP.Base":{"callback": checkValidIP},
	"MS.IP.MaxCount":{"minimum": 1, "maximum": 254},
    },
    "transceiver": {
	"path": {"select": ["./transceiver", "./transceiver-bladerf", "./transceiver-rad1", "./transceiver-usrp1", "./transceiver-uhd"]},
	"MinimumRxRSSI": {"minimum": -90, "maximum": 90},
	"RadioFrequencyOffset": {"select": ["", "Factory calibrated", "64", "65", "66", "67", "68", "69", "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "100", "101", "102", "103", "104", "105", "106", "107", "108", "109", "110", "111", "112", "113", "114", "115", "116", "117", "118", "119", "120", "121", "122", "123", "124", "125", "126", "127", "128", "129", "130", "131", "132", "133", "134", "135", "136", "137", "138", "139", "140", "141", "142", "143", "144", "145", "146", "147", "148", "149", "150", "151", "152", "153", "154", "155", "156", "157", "158", "159", "160", "161", "162", "163", "164", "165", "166", "167", "168", "169", "170", "171", "172", "173", "174", "175", "176", "177", "178", "179", "180", "181", "182", "183", "184", "185", "186", "187", "188", "189", "190", "191", "192"]},
	"TxAttenOffset": {"select": ["", "Factory calibrated", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "100"]},
	"Timeout.Clock": {"minimum": 5, "maximum": 15}
    },
    "control": {
	"VEA": {"callback": checkOnOff},
	"LUR.AttachDetach": {"callback": checkOnOff},
	"SACCHTimeout.BumpDown": {"minimum": 1,"maximum": 3}
    },
    "tapping": {
	"GSM": {"callback": checkOnOff},
	"GPRS": {"callback": checkOnOff},
	"TargetIP": {"callback": checkValidIP},
    },
    "test": {
	"SimulatedFER.Downlink": {"select": ["0", "5", "10", "15", "20", "25", "30", "35", "40", "45", "50", "55", "60", "65", "70", "75", "80", "85", "90", "95", "100"]},
	"SimulatedFER.Uplink": {"select":["0", "5", "10", "15", "20", "25", "30", "35", "40", "45", "50", "55", "60", "65", "70", "75", "80", "85", "90", "95", "100"]},
	"UplinkFuzzingRate": {"select":["0", "5", "10", "15", "20", "25", "30", "35", "40", "45", "50", "55", "60", "65", "70", "75", "80", "85", "90", "95", "100"]},
    },
    "ybts": {
	"mode": {"select": ["nib", "dataroam", "roaming"]},
	"heartbeat_ping": {"minimum": 1000, "maximum": 120000},
	"heartbeat_timeout": {"minimum": 3000, "maximum": 180000},
	"handshake_start": {"minimum": 10000, "maximum": 300000},
	"max_restart": {"minimum": 3, "maximum": 10},
	"imei_request": {"callback": checkOnOff},
	"tmsi_expire": {"minimum": 7200, "maximum": 2592000},
	"t305": {"minimum": 20000, "maximum": 60000},
	"t308": {"minimum": 4000, "maximum": 20000},
	"t313": {"minimum": 4000, "maximum": 20000},
	"sms.timeout": {"minimum": 5000, "maximum": 600000},
	"ussd.session_timeout": {"callback": checkUssdSessTimeout},
	"security": {"callback": checkT3260},
	"auth.call": {"callback": checkOnOff},
	"auth.sms": {"callback": checkOnOff},
	"auth.ussd": {"callback": checkOnOff}
    },
    "roaming": {
	"expires": {"callback": checkValidInteger},
	"nnsf_bits": {"callback": checkValidInteger},
	"reg_sip": {"callback": checkRegSip},
	"nodes_sip": {"callback": checkNodesSip},
	"my_sip": {"callback": checkRegSip}
    },
    "handover": {
	"enable": {"callback": checkOnOff}
    }
};


/*--------- Field specific validation functions ----------*/

// Check validity for Radio.Band and Radio.C0 from GSM section
function checkRadioBand(error,field_name,field_value,section_name,params)
{
    var permitted_values = { "850": [128,251],
	"900": [0, 124, 975, 1023],
	"1800": [512, 885],
	"1900": [512, 810]};

    var restricted_value = params.gsm["Radio.Band"];

    if (!permitted_values[restricted_value]) {
	error.reason = "The given value for '" + field_name + "': '" + restricted_value + "' is not a permitted one. Allowed values: 850,900,1800,1900 in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }

    field_value = parseInt(field_value);
    if (isNaN(field_value)) {
	error.reason = "Field '" + field_name + "' is not a valid number: '" + field_value + "' in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }

    var int_value = parseInt(field_value);
    var radio_c0_val = permitted_values[restricted_value];
    var min = radio_c0_val[0];
    var max = radio_c0_val[1];

    if (radio_c0_val[3] == undefined) {
	if (int_value < min || int_value > max) {
	    error.reason = "Field '"+ field_name +"' is not in the right range for the Radio.Band chosen in section '" + section_name + "'.";
	    error.error = 401;
	    return false;
	}
	return true;
    } else {
	var min2 = radio_c0_val[2];
	var max2 = radio_c0_val[3];
	if ((int_value >= min && int_value <= max) || 
	    (int_value >= min2 && int_value <= max2))
	    return true;
	error.reason = "Field " + field_name + " is not in the right range for the Radio.Band chosen in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }
}

// Test [gsm] parameters validity: 'Radio.PowerManager.MinAttenDB','Radio.PowerManager.MaxAttenDB' 
function checkRadioPowerManager(error,field_name,field_value,section_name,params)
{
    var min = parseInt(field_value);
    if (isNaN(min)) {
	error.reason = "Field 'Radio.PowerManager.MinAttenDB' is not a valid number '" + min + "' in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }

    var max = parseInt(params.gsm['Radio.PowerManager.MaxAttenDB']);
    if (isNaN(max)) {
	error_reason = "Field 'Radio.PowerManager.MinAttenDB' is not a valid number '" + max + "' in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }

    if (min > max) { 
	error.reason = "'Radio.PowerManager.MinAttenDB', must be less or equal to 'Radio.PowerManager.MaxAttenDB' in section '" + section_name + "'";
	error.error = 401;
	return false;
    }

    if (!checkFieldValidity(error,section_name,field_name,min,0,80))
	return false;

    if (!checkFieldValidity(error,section_name,'Radio.PowerManager.MaxAttenDB',max,0,80))
	return false;

    return true;
}

// Validate a space-separated list of the DNS servers, in IP dotted notation, eg: 1.2.3.4 5.6.7.8.
function checkValidDNS(error,field_name,field_value,section_name)
{
    //this field can be empty
    if (isEmpty(field_value))
	return true;

    //validate DNS that are separed by space
    var dns_addresses = field_value.split(" ");
    var total = dns_addresses.length;

    for (var i = 0; i < total; i++) {
	if (!checkValidIP(error,field_name,dns_addresses[i],section_name))
	    return false;
    }
    return true;
}

function validateMapNetworkDataroam(error,param_name,param_value,section_name)
{
    if (!param_value.length)
	return true;

    var map = param_value.split("\n");
    for (var i=0; i < map.length; i++) {
	var map_entry = map[i];
	if (!map_entry.length)
	    continue;
	var entry = map_entry.split("=");
	if (entry.length != 2) {
	    error.reason = "Invalid format for '" + param_name + "' in section '" + section_name + "'.";
	    error.error = 401;
	    return false;
	}

	var no = parseInt(entry[0]);
	if (isNaN(no)) {
	    error.reason = "Invalid value '" + entry[0] + "' for 'Network map', should be numeric in section '" + section_name + "'.";
	    error.error = 401;
	    return false;
	}
	if (!checkValidIP(error,param_name,entry[1],section_name)) {
	    return false;
	}
    }
    return true;
}

function validateRoamingParams(error,params)
{
    var required = ["nnsf_bits", "gstn_location"];

    for (var i = 0; i < required.length; i++) {
	if (!params.roaming[required[i]]) {
	    error.reason = "Field '" + required[i] + "' from 'Roaming' section is required in roaming mode in section 'roaming'.";
	    error.error = 401;
	    return false;
	}
    }

    var reg_sip = params.roaming["reg_sip"];
    var nodes_sip = params.roaming["nodes_sip"];
       if (isEmpty(reg_sip) && isEmpty(nodes_sip)) {
	error.reason = "Either 'Reg sip' or 'Nodes sip' must be set in roaming mode in section 'roaming'.";
	error.error = 401;
	return false;
    }

    var nnsf_bits = params.roaming["nnsf_bits"];
    var expires = params.roaming["expires"];
    if (!validatePositiveNumber(error, 'NNSF bits', nnsf_bits, 'roaming') || 
	!validatePositiveNumber(error, 'Expires', expires, 'roaming'))
	return false;

    return true;
}

/**
 * Timer.ImplicitDetach: it is recommended to be at least 240 seconds greater than SGSN.Timer.RAUpdate. 
 * Interval allowed 2000:4000(10).
 */
function checkTimerImplicitdetach(error,field_name,field_value,section_name)
{
    field_value = parseInt(field_value);
    if (isNaN(field_value)) {
	error.reason = "Field '" + field_name + "' is not a valid number: " + field_value + " in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }

    if (!(field_value >= 2000 && field_value <= 4000) || field_value % 10 != 0) {
	error.reason = "Field '" + field_name + "' is not in the allowed interval: " + field_value + ". The value must be in interval [2000,4000] and should be a factor of 10 in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }

    return true;
}

/**
 * [gprs_advanced] param: Timer.RAUpdate
 * Setting to 0 or >12000 deactivates entirely, i.e., sets the timer to effective infinity.
 * Recommended: to prevent GPRS Routing Area Updates you must set both this and GSM.Timer.T3212 to 0.
 * Interval allowed 0:11160(2). Defaults to 3240.
 */
function checkTimerRaupdate(error,field_name,field_value,section_name)
{
    field_value = parseInt(field_value); 
    if (isNaN(field_value)) {
	error.reason = "Field '" + field_name + "' is not a valid number: " + field_value + " in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }

    if (field_value > 12000)
	return true;

    if (!(field_value >= 0 && field_value <= 11160) || field_value % 2 != 0) { 
	error.reason = "Field '" + field_name + "' is not in the allowed interval: " + field_value + ". The value must be in interval [0,11160] and should be a factor of 2 or greater than 12000 in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }

    return true;
}

/**
 * [gprs_advanced] param:  Uplink.Persist
 * If non-zero, is recommended to be greater than GPRS.Uplink.KeepAlive.
 * This is broadcast in the beacon and it cannot be changed once BTS is started.
 * Allowed interval 0:6000(100).
 */
function checkUplinkPersistent(error,field_name,field_value,section_name)
{
    field_value = parseInt(field_value);
    if (isNaN(field_value)) {
	error.reason = "Field '" + field_name + "' is not a valid number: " + field_value + " in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }

    if (!(field_value >= 0 && field_value <= 6000) || field_value % 100 != 0) {
	error.reason = "Field '" + field_name + "' is not in the allowed interval: " + field_value + ". The value must be in interval [0,6000] and should be a factor of 100 in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }

    return true;
}

/**
 * [gprs_advanced] param: Downlink.Persist
 * If non-zero, it is recommended to be greater than GPRS.Downlink.KeepAlive but not required.
 */
function checkDownlinkPersistent(error,field_name,field_value,section_name)
{
    field_value = parseInt(field_value); 
    if (isNaN(field_value)) {
	error.reason = "Field '" + field_name + "' is not a valid number: '" + field_value + "' in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }

    if (field_value < 0 || field_value > 10000) {
	error.reason = "Field '" + field_name + "' is not valid. It has to be smaller than 10000 in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }

    return true;
}

/**
 * Test for [gprs_advanced] param: ChannelCodingControl.RSS.
 * This value should normally be GSM.Radio.RSSITarget + 10 dB but not required.
 * Interval allowed -65:-15.
 */
function checkChannelcodingcontrolRssi(error,field_name,field_value,section_name)
{
    field_value = parseInt(field_value);
    if (isNaN(field_value)) {
	error.reason = "Field '" + field_name + "' is not a valid number: '" + field_value + "' in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }

    if (!(field_value >= -65 && field_value <= -15)) {
	error.reason = "Field '" + field_name + "' is not in the allowed interval: " + field_value + ". The value must be in interval [-65,-15] in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }

    return true;
}

/**
 * Validate field Radio.RSSITarget.
 * It is recommanded to be ChannelCodingControl.RSS-10 dB, but not required.
 */
function checkRadioRssiTarget(error,field_name,field_value,section_name)
{
    field_value = parseInt(field_value);
    if (isNaN(field_value)) {
	error.reason = "Field '" + field_name + "' is not a valid number: '" + field_value + "' in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }

    if (!(field_value >= -75 && field_value <= -25)) {
	error.reason = "Field '" + field_name + "' is not in the allowed interval: " + field_value + ". The value must be in interval [-75,-25] in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }

    return true;
}

/**
 * Validate t3260
 */
function checkT3260(error,field_name,field_value,section_name)
{
    field_value = parseInt(field_value);
    if (isNaN(field_value)) {
	error.reason = "Field '" + field_name + "' is not valid, interval allowed: 5000..3600000 in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }

    if (field_value < 5000 || field_value > 3600000) {
	error.reason = "Field '" + field_name + "' from '" + section_name + "' is not valid, interval allowed: 5000..3600000 in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }

    return true;
}

/**
 * Validate RACH.AC from [gsm.advanced]
 */ 
function checkRachAc(error,field_name,field_value,section_name)
{
    if (!field_value.length)
	return true;

    if (field_value.substr(0,2) != "0x") {
	return checkFieldValidity(error, section_name, field_name, field_value, 0, 65535);
    } else {
	var str = /^0x[0-9a-zA-Z]{1,4}$/;
	if (!str.test(field_value)) {
	    error.reason =  "Invalid hex value '" + field_value + "' for '" + field_name + "' in section  '" + section_name + "'.";
	    error.error = 401;
	    return false;
	}
    }

    return true;
}


// in section ybts validate Ussd.session_timeout= timeout, in milliseconds, of USSD sessions.
// minimum allowed 30000.
function checkUssdSessTimeout(error,field_name,field_value,section_name)
{
    field_value = parseInt(field_value);
    if (isNaN(field_value)) {
	error.reason = "Field '" + field_name + "' is not a valid number: " + field_value + " in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }

    if (field_value < 30000) {
	error.reason = "Field '" + field_name + "' is smaller than 30000: " + field_value + " in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }
    return true;
}

/**
 * Validate reg_sip from [roaming] section: ip:port or just ip
 */
function checkRegSip(error,field_name,field_value,section_name)
{
    if (!field_value.length)
	return true;

    var expl = field_value.split(":");
    var ip = expl[0];
    if (!checkValidIP(error, field_name, ip, section_name))
       return false;

    if (expl[1] != undefined)
	var port = parseInt(expl[1]);
    else
	var port = null;

    if (port && isNaN(port)) {
	 error.reason = "Field '" + field_name + "' doesn't contain a valid port:  '"+ field_value + "' in section '" + section_name + "'.";
	 error.error = 401;
	 return false;
    }

    return true;
}

function checkNodesSip(error,field_name,field_value,section_name)
{
    if (!field_value.length)
	return true;

    if (!JSON.parse(field_value)) {
	error.reason = "Field '" + field_name + "' doesn't contain a valid JSON object:  '"+ field_value + "' in section '" + section_name + "'.";
	error.error = 401;
	return false;
    }
    return true;
}

function checkNnsfBits(error,field_name,field_value,section_name)
{
   if (!field_value.length)
	return true;

   if (!validatePositiveNumber(error, 'NNSF bits', field_value, section_name))
      return false;

   return true;
}
/* vi: set ts=8 sw=4 sts=4 noet: */
