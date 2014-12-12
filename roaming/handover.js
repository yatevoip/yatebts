/**
 * handover.js
 * This file is part of the Yate-BTS Project http://www.yatebts.com
 *
 * SIP based handover functions for YateBTS
 * This file must be included by roaming.js
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
 * Route handover calls
 * @param msg Object. Message to be handled
 */
function routeHandover(msg)
{
    if (msg.module != "ybts" || msg.called == "")
	return false;
    if (msg.called.startsWith("HANDOVER")) {
	var ref = msg.called.substr(8);
	if (ref == "")
	    return false;
	var info = ho_inbound[ref];
	if (!info)
	    return false;
	delete ho_inbound[ref];

	if (msg.imsi != "")
	    current_calls[msg.id] = msg.imsi;
	msg.retValue("sip/" + info.uri);
	// Force the new call to be a reINVITE using received dialog info
	msg["osip_Call-ID"] = info.callid;
	msg.osip_From = info.from;
	msg.osip_To = info.to;
	if (info.local_cseq != "")
	    msg.osip_CSeq = (parseInt(info.local_cseq) + 1) + " INVITE";
	msg.remote_cseq = info.remote_cseq;
	msg.osip_Reason = ho_reason;
	msg["osip_P-Access-Network-Info"] = accnet_header;
	if (msg.phy_info)
	    msg["osip_P-PHY-Info"] = "YateBTS; " + msg.phy_info;

	if (info.contact) {
	    // Send a BYE towards old YateBTS to release call resources
	    var m = new Message("xsip.generate");
	    m.method = "BYE";
	    m.uri = info.contact;
	    m["sip_Call-ID"] = info.callid;
	    msg.sip_To = info.from;
	    m.sip_From = info.to;
	    if (info.remote_cseq != "")
		m.sip_CSeq = (parseInt(info.remote_cseq) + 1) + " BYE";
	    m.sip_Reason = ho_reason;
	    m["sip_P-Access-Network-Info"] = accnet_header;
	    if (msg.phy_info)
		m["sip_P-PHY-Info"] = "YateBTS; " + msg.phy_info;
	    m.enqueue();
	}

	return true;
    }
    return false;
}

function hangupHandover(msg)
{
    delete ho_outbound[msg.id];
}

/*
 * Various periodic cleanups
 */
function handoverClearInterval(now)
{
    if (now === undefined)
	now = Date.now() / 1000;
    // Neighbors holdoff clear
    for (var n of neighbors) {
	if (n.holdoff && (now > n.holdoff)) {
	    n.holdoff = false;
	    if (debug)
		Engine.debug(Engine.DebugNote,"No longer holding off cell " + n.cellid);
	}
    }
}
/*
 * Load cell and neighbors information from configuration file ybts.conf
 */
function loadNeighbors()
{
    my_cell = null;
    var neigh = neighbors;
    neighbors = [];
    var conf = new ConfigFile(Engine.configFile("ybts"),true);
    if (!parseBool(conf.getValue("handover","enable"),true))
	return;
    ho_holdoff = parseInt(conf.getValue("handover","holdoff"),10);
    if (ho_holdoff <= 0)
	ho_holdoff = 10;
    else if (ho_holdoff < 5)
	ho_holdoff = 5;
    else if (ho_holdoff > 60)
	ho_holdoff = 60;
    ho_reason = conf.getValue("handover","reason",'GSM;text="Handover"');
    if (ho_reason == "" || !parseBool(ho_reason,true))
	ho_reason = undefined;
    var gsm = conf.getSection("gsm");
    if (!gsm)
	return;

    var band = parseInt(gsm.getValue("Radio.Band"));
    var arfcn = parseInt(gsm.getValue("Radio.C0"));
    if (isNaN(band) || isNaN(arfcn))
	return;

    var ncc = parseInt(gsm.getValue("Identity.BSIC.NCC",0));
    var bcc = parseInt(gsm.getValue("Identity.BSIC.BCC",2));
    if (isNaN(ncc) || ncc < 0 || ncc > 7 || isNaN(bcc) || bcc < 0 || bcc > 7)
	return;

    var mcc = gsm.getValue("Identity.MCC","001");
    var mnc = gsm.getValue("Identity.MNC","01");
    if (mcc.length != 3 || mnc.length < 2 || mnc.length > 3)
	return;
    var lac = parseInt(gsm.getValue("Identity.LAC",1000));
    var ci = parseInt(gsm.getValue("Identity.CI",10));
    if (isNaN(lac) || lac <= 0 || lac >= 65534 || isNaN(ci) || ci < 0 || ci > 65535)
	return;

    my_cell = {};
    my_cell.band = band;
    my_cell.arfcn = arfcn;
    my_cell.bsic = ncc * 8 + bcc;
    my_cell.cellid = mcc + "" + mnc + lac.toString(16,4) + ci.toString(16,4);
    my_cell.info = "Type: GERAN\r\n"
	+ "Band: " + band + "\r\n"
	+ "ARFCN: " + arfcn + "\r\n"
	+ "BSIC: " + my_cell.bsic + "\r\n"
	+ "CellID: " + my_cell.cellid + "\r\n";

    neighbors = neigh;
    for (var k of neighbors)
	k.unused = true;
    neigh = conf.getValue("handover","neighbors");
    if (neigh != "") {
	neigh = neigh.split(',');
	for (var k of neigh) {
	    k = k.trim();
	    if (k == "")
		continue;
	    var n = findSipNeighbor(k);
	    if (n)
		delete n.unused;
	    else
		neighbors.push({sip: k, active: false});
	}
    }
    neigh = [];
    var chg = false;
    for (var k in neighbors) {
	if (neighbors[k].unused)
	    chg = true;
	else
	    neigh.push(neighbors[k]);
    }
    if (chg) {
	neighbors = neigh;
	ping_idx = 0;
	updateNeighbors();
    }
}

/*
 * Helper function used in sorting neighbors by ARFCN
 * @param n1 First neighbor
 * @param n2 Second neighbor
 * @return Zero if equal, > 0 if n1 > n2, < 0 if n1 < n2
 */
function compareNeighbors(n1,n2)
{
    n1 = parseInt(n1.arfcn);
    n2 = parseInt(n2.arfcn);
    if (n1 == n2)
	return 0;
    // ARFCN 0 must be at end of list
    if (!n1)
	return 1;
    if (!n2)
	return -1;
    return n1 - n2;
}

/*
 * Build a sorted by ARFCN list of usable neighbors and push it to ybts
 */
function updateNeighbors()
{
    if (!bts_available)
	return;
    var tmp = "0";
    if (my_cell) {
	var lst = [];
	for (var n of neighbors) {
	    if (n.active === false)
		continue;
	    if (n.band != "" && n.band != my_cell.band)
		continue;
	    if (n.arfcn != "" || n.bsic != "" || n.cellid != "")
		lst.push(n);
	}
	if (lst.length > 1)
	    lst.sort(compareNeighbors);
	tmp = my_cell.band;
	for (var n of lst)
	    tmp += " " + n.arfcn + ":" + n.bsic + ":" + n.cellid;
    }
    if (debug)
	Engine.debug(Engine.DebugInfo,"Neighbors: " + tmp);
    var m = new Message("chan.control");
    m.component = "ybts";
    m.operation = "neighbors";
    m.list = tmp;
    m.enqueue();
}

/*
 * Pings and updates a single peer, return true if changed
 * @param Peer Neighbor object to ping
 * @return True if neighbor state changed
 */
function doPeerPing(peer)
{
    if (debug)
	Engine.debug(Engine.DebugAll,"Pinging " + peer.sip);
    var m = new Message("xsip.generate");
    m.method = "OPTIONS";
    m.uri = "sip:" + peer.sip;
    m.sip_Accept = "text/vnd.nullteam.cell-info";
    m.sip_Reason = ho_reason;
    m.xsip_trans_count = 2;
    m.wait = true;
    var act = m.dispatch(true) && (m.code == 200);
    if (peer.active === undefined)
	peer.active = false;
    var chg = act != peer.active;
    peer.active = act;
    if (act && m.xsip_type == "text/vnd.nullteam.cell-info" && m.xsip_body) {
	var body = m.xsip_body;
	while (var r = body.match(/^([[:alnum:]]+): *([[:alnum:]]+)[[:space:]]+(.*)$/)) {
	    var n = r[1];
	    n = n.toLowerCase();
	    switch (n) {
		case "band":
		case "arfcn":
		case "bsic":
		case "cellid":
		    if (peer[n] != r[2]) {
			chg = true;
			peer[n] = r[2];
		    }
		    break;
	    }
	    body = r[3];
	}
    }
    return chg;
}

/*
 * Periodic neighbors check
 */
function checkInterval()
{
    var chg = false;
    for (var idx = ping_idx; neighbors.length; ) {
	var n = neighbors[idx];
	if (!n) {
	    ping_idx = 0;
	    return;
	}
	idx = (idx + 1) % neighbors.length;
	if (ping_start && !idx) {
	    // Just finished first round of checks
	    ping_start = false;
	    chg = true;
	}
	if (n.sip && !n.static) {
	    ping_idx = idx;
	    if (doPeerPing(n)) {
		Engine.debug(Engine.DebugNote,"Peer " + n.sip + " active: " + n.active);
		chg = true;
	    }
	    break;
	}
	if (idx == ping_idx)
	    break;
    }
    if (chg && !ping_start)
	updateNeighbors();
}

/*
 * Enable / disable handover availability and polling
 * @param avail Availability of the local BTS, undefined to detect
 */
function setBtsAvailable(avail)
{
    if (!my_cell)
	avail = false;
    if (avail === undefined) {
	avail = false;
	var tmp = new Message("engine.status");
	tmp.module = "mbts";
	if (tmp.dispatch()) {
	    tmp = tmp.retValue();
	    avail = tmp.indexOf("state=RadioUp") >= 0;
	}
    }
    if (avail != bts_available) {
	Engine.debug(Engine.DebugNote,"BTS Available: " + avail);
	for (var tmp of neighbors) {
	    if (tmp.active)
		tmp.active = false;
	}
    }
    var tmp = setBtsAvailable.timer;
    if (avail) {
	bts_available = true;
	if (tmp === undefined) {
	    ping_start = true;
	    if (ping_interval === undefined)
		ping_interval = 10000;
	    setBtsAvailable.timer = Engine.setInterval(checkInterval,ping_interval);
	    if (debug)
		Engine.debug(Engine.DebugInfo,"Starting neighbors polling");
	}
    }
    else {
	setBtsAvailable.timer = undefined;
	updateNeighbors();
	bts_available = false;
	if (tmp !== undefined) {
	    Engine.clearInterval(tmp);
	    ping_start = false;
	    ping_idx = 0;
	    if (debug)
		Engine.debug(Engine.DebugInfo,"Stopped neighbors polling");
	}
    }
}

/*
 * Request inter-MSC handover information
 * @param chan Channel ID of the call
 * neigh Neighbor object
 * @param msg Message object
 * @return True if handover could be started
 */
function requestMscHandover(chan,neigh,msg)
{
    if (debug)
	Engine.debug(Engine.DebugNote,"Requesting MSC Handover from " + neigh.msc);
    // TODO: Implement inter-MSC handover
    return false;
}

/*
 * Retrieve SIP dialog information
 * @param id Channel ID of call to retrieve
 * @return Multiline string describing SIP dialog
 */
function getSipDialog(id)
{
    var m = new Message("chan.control");
    m.targetid = id;
    m.operation = "query";
    var res = "";
    if (m.dispatch(true)) {
	if (m.sip_uri)
	    res += "URI: " + m.sip_uri + "\r\n";
	if (m.sip_callid)
	    res += "Call-ID: " + m.sip_callid + "\r\n";
	if (m.sip_from)
	    res += "From: " + m.sip_from + "\r\n";
	if (m.sip_to)
	    res += "To: " + m.sip_to + "\r\n";
	if (m.local_cseq)
	    res += "L-CSeq: " + m.local_cseq + "\r\n";
	if (m.remote_cseq)
	    res += "R-CSeq: " + m.remote_cseq + "\r\n";
    }
    return res;
}

/*
 * Request handover information from target YateBTS
 * @param chan Channel ID of the call
 * neigh Neighbor object
 * @param msg Message object
 * @return True if handover could be started
 */
function requestBtsHandover(chan,neigh,msg)
{
    if (debug)
	Engine.debug(Engine.DebugNote,"Requesting BTS Handover from " + neigh.sip);
    var m = new Message("xsip.generate");
    m.method = "INFO";
    m.uri = "sip:" + neigh.sip;
    m.sip_Accept = "application/vnd.nullteam.ho-accept";
    m.sip_Reason = ho_reason;
    m.xsip_type = "text/vnd.nullteam.ho-request";
    m.xsip_body = getSipDialog(msg.peerid);
    if (msg.gsm_state)
	m.xsip_body += "Conn: " + msg.gsm_state + "\r\n";
    m.xsip_body += "CellID: " + neigh.cellid + "\r\n";
    m.xsip_trans_count = 3;
    m.wait = true;
    if (!m.dispatch(true) || (m.code != 200)) {
	Engine.debug(Engine.DebugNote,"Holding off handovers to " + neigh.sip + " for " + ho_holdoff + " s");
	neigh.holdoff = (Date.now() / 1000) + ho_holdoff;
	return false;
    }
    if (m.xsip_body != "" && m.xsip_type == "application/vnd.nullteam.ho-accept") {
	var m2 = new Message("call.update");
	m2.targetid = chan;
	m2.operation = "transport";
	m2.l3raw = m.xsip_body;
	m2.hard_release = true;
	m2.enqueue();
    }
    return true;
}

/*
 * Attempt to start handover to target cell
 * @param chan Channel ID of the call
 * @param cell Cell ID of the target
 * @param msg Message object
 * @return True if handover could be started
 */
function attemptHandover(chan,cell,msg)
{
    for (var n of neighbors) {
	if (cell != n.cellid)
	    continue;
	if (n.holdoff)
	    break;
	if (n.sip)
	    return requestBtsHandover(chan,n,msg);
	if (n.msc)
	    return requestMscHandover(chan,n,msg);
	break;
    }
    return false;
}

/*
 * Find a neighbor by SIP address
 * @param addr Neighbor address with optional port
 * @return Neighbor object or null if not found
 */
function findSipNeighbor(addr)
{
    for (var n of neighbors) {
	if (n.sip == addr)
	    return n;
    }
    return null;
}

/*
 * Handle Handover Required
 * @param msg Message object
 * @return True if message was handled
 */
function handoverReq(msg)
{
    if (!msg.answered)
	return true;
    if (ho_outbound[msg.id]) {
	if (debug)
	    Engine.debug(Engine.DebugInfo,"Already in handover to " + ho_outbound[msg.id] + " on " + msg.id);
	return true;
    }
    for (var i = 1; ; i++) {
	var cell = msg.cell[i];
	if (!cell)
	    break;
	ho_outbound[msg.id] = cell;
	if (attemptHandover(msg.id,cell,msg))
	    return true;
    }
    delete ho_outbound[msg.id];
    return true;
}

/*
 * Handle Handover Failure
 * @param msg Message object
 * @return True if message was handled
 */
function handoverFail(msg)
{
    var cell = ho_outbound[msg.id];
    if (!cell)
	return true;
    delete ho_outbound[msg.id];
    Engine.debug(Engine.DebugWarn,"Failed handover to " + cell + ": " + msg.reason);
    for (var n of neighbors) {
	if (n.cellid == cell && n.active !== false && !n.holdoff) {
	    Engine.debug(Engine.DebugNote,"Holding off handovers to " + n.sip + " for " + ho_holdoff + " s");
	    n.holdoff = (Date.now() / 1000) + ho_holdoff;
	    break;
	}
    }
    return true;
}

/*
 * Answer SIP INFO
 * @param msg Message object
 * @return True if message was handled
 */
function onSipInfo(msg)
{
    if (msg.sip_accept == "application/vnd.nullteam.ho-accept") {
	var info = {};
	var info_gsm = false;
	if ((msg.xsip_type == "text/vnd.nullteam.ho-request") && (msg.xsip_body != "")) {
	    var body = msg.xsip_body;
	    while (var r = body.match(/^([[:alnum:]_-]+): *([[:print:]]+)[^[:print:]]+(.*)$/)) {
		var n = r[1];
		n = n.toLowerCase();
		switch (n) {
		    case "uri":
			info.uri = r[2];
			break;
		    case "call-id":
			info.callid = r[2];
			break;
		    case "from":
			info.from = r[2];
			break;
		    case "to":
			info.to = r[2];
			break;
		    case "l-cseq":
			info.local_cseq = r[2];
			break;
		    case "r-cseq":
			info.remote_cseq = r[2];
			break;
		    case "conn":
			info_gsm = r[2];
			break;
		}
		body = r[3];
	    }
	    if (info.from != "") {
		var r = info.from.match(/:([^[:space:]:;@]+@)/);
		if (r)
		    info.contact = "sip:" + r[1] + msg.address;
	    }
	}
	var r = (ho_reference = (ho_reference + 1) % 256);
	var m = new Message("chan.control");
	m.component = "ybts";
	m.operation = "handover";
	m.reference = r;
	if (info_gsm)
	    m.gsm_state = info_gsm;
	msg.osip_Reason = ho_reason;
	if (m.dispatch(true) && m.command) {
	    msg.xsip_type = msg.sip_accept;
	    msg.xsip_body = m.command;
	    msg.xsip_body_encoding = "hex";
	    ho_inbound[r] = info;
	}
	else
	    msg.reason = "noconn";
	return true;
    }
    return false;
}

/*
 * Answer SIP OPTIONS
 * @param msg Message object
 * @return True if message was handled
 */
function onSipOptions(msg)
{
    switch (msg.sip_accept) {
	case "text/vnd.nullteam.cell-info":
	    msg.osip_Reason = ho_reason;
	    if (bts_available && my_cell) {
		msg.xsip_type = msg.sip_accept;
		msg.xsip_body = my_cell.info;
		break;
	    }
	    msg.reason = "noconn";
	    return false;
	case "application/sdp":
	case "":
	    break;
	default:
	    msg.reason = "nomedia";
	    return false;
    }
    return true;
}

/*
 * Handle channel handover
 * @param msg Message object
 * @return True if message was handled
 */
function onChanUpdate(msg)
{
    switch (msg.operation) {
	case "ho-required":
	    return handoverReq(msg);
	case "ho-failure":
	    return handoverFail(msg);
    }
    return false;
}

/*
 * Track the ybts module status
 * @param msg Message object
 * @return False
 */
function onModuleUpdate(msg)
{
    setBtsAvailable(msg.operational);
    return false;
}

// BTS availability
bts_available = undefined;
// neighbors list
neighbors = [];
// current neighbors pinging index
ping_idx = 0;
// first pinging cycle indicator
ping_start = false;
// local cell information
my_cell = null;
// current handover reference number
ho_reference = 0;
// pending outbound handover information
ho_outbound = {};
// pending inbound handover
ho_inbound = {};
// holdoff after handover failute
ho_holdoff = 10;
// handover reason to set in messages
ho_reason = 'GSM;text="Handover"';

loadNeighbors();
Message.install(onChanUpdate,"call.update",90);
Message.install(onSipOptions,"sip.options",90);
Message.install(onSipInfo,"sip.info",90);
Message.install(onModuleUpdate,"module.update",90,"module","ybts");
setBtsAvailable();

/* vi: set ts=8 sw=4 sts=4 noet: */
