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
    if (prompts_dir!="" && File.exists(prompts_dir+pprompt))
	return prompts_dir+pprompt;

    var dirs = ["/usr/share/yate/sounds/", "/usr/local/share/yate/sounds/", "/var/spool/yatebts/"];
    for (var i=0; i<dirs.length; i++) { 
	if (File.exists(dirs[i]+pprompt)) {
	    prompts_dir = dirs[i];
	    return prompts_dir+pprompt;
	}
    }
    
    //this should not happen
    Engine.debug(Debug.Warn,"Don't have path for prompt "+pprompt);
    return false;
}

function sendToConference()
{
    var m = new Message("chan.masquerade");
    m.message = "call.execute";
    m.id = partycallid;
    m.callto = "conf/333";
    m.lonely = true;
    m.dispatch();
}

function makeCall()
{
    var m = new Message("chan.masquerade");
    m.message = "call.execute";
    m.id = partycallid;
    m.callto = "iax/iax:32843@83.166.206.79/32843";
    //m.callto = "iax/iax:090@192.168.168.1/090";
    m.caller = partycaller;
    m.dispatch();  
}

function startEchoTest()
{
    var m = new Message("chan.masquerade");
    m.message = "call.execute";
    m.id = partycallid;
    m.callto = "external/playrec/echo.sh";
    m.timeout = 0;
    m.dispatch();
}

function playEchoPrompt()
{
    var m = new Message("chan.masquerade");
    m.message = "call.execute";
    m.id = partycallid;
    m.callto = "wave/play/"+getPathPrompt("echo.au");
    m.dispatch();
}

function onChanDtmf(msg)
{
    Engine.debug(Engine.DebugInfo,"Got dtmf "+msg.text+" for "+partycallid+" in state='"+state+"'");
    if (state != "")
	// Ignore dtmfs after state changed
	// Otherwise users can continue changing between states
	return;

    if (timeout_started==true) {
	// reset timeout if it was started
	var m = new Message("chan.control");
	m.targetid = partycallid;
	m.timeout = 0;
	m.dispatch();
    }

    if(msg.text == 1) {
	state = "echoTest";
	playEchoPrompt();
    } else if (msg.text == 2) {
	state = "conf";
	sendToConference()
    } else if (msg.text == 3) {
	state = "call";
	makeCall();
    }
}

function welcomeIVR(msg)
{
    Engine.debug(Engine.DebugInfo,"Got call to welcome IVR.");
    partycallid = msg.id;
    partycaller = "yatebts";

    Message.install(onChanDtmf, "chan.dtmf", 90, "id", msg.id);
    Channel.callTo("wave/play/"+getPathPrompt("welcome.au"));

    if (state == "") {
	// No digit was pressed
	// Wait for 10 seconds to see if digit is pressed
	var m = new Message("chan.control");
	m.targetid = partycallid;
	m.timeout = 10000;
	m.dispatch();
	timeout_started = true;

	Channel.callTo("wave/record/-");
    }

    Engine.debug(Engine.DebugInfo,"Returned to main function in state '"+state+"'");
    if (state == "echoTest") {
	startEchoTest();
	return;
    }
}

state = "";
timeout_started = false;
prompts_dir = "";

Engine.debugName("welcome");
// 32843 -> david
if (message.called=="32843")
    welcomeIVR(message);
