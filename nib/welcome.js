/**
 * welcome.js
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
 * Network-In-a-Box demo
 * To use it put in javascript.conf:
 *
 * [general]
 * routing=welcome.js
 */

function getPathPrompt(pprompt)
{
    if (File.exists("/usr/share/yate/sounds/"+pprompt))
	return "/usr/share/yate/sounds/"+pprompt;
    else if (File.exists("/usr/local/share/yate/sounds/"+pprompt))
	return "/usr/local/share/yate/sounds/"+pprompt;
    else if (File.exists("/var/spool/yatebts/"+pprompt))
	return "/var/spool/yatebts/"+pprompt;

    //this should not happen
    Engine.debug(Debug.Warn,"Don't have path for prompt "+pprompt);
    return false;
}

function sendToConference()
{
    m = new Message("chan.masquerade");
    m.message = "call.execute";
    m.id = partycallid;
    m.callto = "conf/333";
    m.lonely = true;
    m.dispatch();
}

function makeCall()
{
    m = new Message("chan.masquerade");
    m.message = "call.execute";
    m.id = partycallid;
    m.callto = "iax/iax:32843@83.166.206.79/32843";
    //m.callto = "iax/iax:090@192.168.168.1/090";
    m.caller = partycaller;
    m.dispatch();  
}

function startEchoTest()
{
    m = new Message("chan.masquerade");
    m.message = "call.execute";
    m.id = partycallid;
    m.callto = "external/playrec/echo.sh";
    m.dispatch();
}

function ChanDtmf(msg)
{
    Engine.debug(Engine.DebugInfo,"Got dtmf "+msg.text+" for "+partycallid);

    if(msg.text == 1) {
        startEchoTest();
    } else if (msg.text == 2) {
	sendToConference()
    } else if (msg.text == 3) {
	makeCall();
    }
}

function welcome_ivr(msg)
{
    Engine.debug(Engine.DebugInfo,"Got call to welcome IVR.");
    partycallid = msg.id;
    partycaller = "yatebts";

    Message.install(ChanDtmf, "chan.dtmf", 90, "id", msg.id);
    Channel.callTo("wave/play/"+getPathPrompt("welcome.au"));
    //Channel.timeout = "10000";
    Channel.callJust("wave/record/-");
}

// 32843 -> david
if (message.called=="32843")
    welcome_ivr(message);
