function onControl(msg)
{
	Engine.debug(Engine.DebugInfo,"enter in onControl. Parameters received: called-> " + msg.called + " sms text -> " + msg.text + " sms rpdu -> "+ msg.rpdu );

	if (!msg.called && (!msg.text || !msg.rpdu)) {
		Engine.debug(Engine.DebugWarn, "ERROR: got chan.control with no caller or message. The SMS will not be send.");
		return false;
	}

	Engine.debug(Engine.DebugInfo,"CREATE msg.execute with data from message to send the SMS to: "+ msg.called+ " and text "+ msg.text + " or RPDU: "+ msg.rpdu);

	var m = new Message("msg.execute");
	m.caller = "12345";
	m.called = msg.called;
	m["sms.caller"] = "12345";
	if (msg.text)
		m.text = msg.text;
	else if (msg.rpdu)	
		m.rpdu = msg.rpdu;

	m.callto = "ybts/IMSI"+msg.called;
	if (m.dispatch()) { 
		Engine.debug(Engine.DebugInfo, "msg.execute returned true");
		return true;
	}
	Engine.debug(Engine.DebugInfo, "msg.execute returned false");
	return false;
}

Engine.debugName("custom-sms");
Message.install(onControl,"chan.control",80);
