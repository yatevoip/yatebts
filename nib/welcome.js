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
    for (var dir of dirs)
	if (File.exists(dir+pprompt)) {
	    prompts_dir = dir;
	    return prompts_dir+pprompt;
	}
    
    //this should not happen
    Engine.debug(Debug.Warn,"Don't have path for prompt "+pprompt);
    return false;
}

function onChanDtmf(msg)
{
    if(msg.text == 1) {
	state = "echoTest";
	Channel.callTo("wave/play/"+getPathPrompt("echo.au"));
    }

    else if (msg.text == 2) 
	Channel.callJust("conf/333",{"lonely":true});

    else if (msg.text == 3)
	Channel.callJust("iax/iax:32843@83.166.206.79/32843",{"caller":"yatebts"});
	//Channel.callJust("iax/iax:090@192.168.1.1/090",{"caller":"yatebts"});
}

function welcomeIVR(msg)
{
    Engine.debug(Engine.DebugInfo,"Got call to welcome IVR.");

    Message.install(onChanDtmf, "chan.dtmf", 90, "id", msg.id);
    Channel.callTo("wave/play/"+getPathPrompt("welcome.au"));

    if (state == "")
	// No digit was pressed
	// Wait aprox 10 seconds to see if digit is pressed
	Channel.callTo("wave/record/-",{"maxlen":180000});

    Engine.debug(Engine.DebugInfo,"Returned to main function in state '"+state+"'");
    if (state == "echoTest")
	Channel.callJust("external/playrec/echo.sh");
}

state = "";
prompts_dir = "";

Engine.debugName("welcome");
// 32843 -> david
if (message.called=="32843")
    welcomeIVR(message);
