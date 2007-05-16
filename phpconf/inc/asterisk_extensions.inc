<?php
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007 IKT <http://itison-ikt.de>.
	All rights reserved.
	
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
	
	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.
	
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	
	THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
	AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
	OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

require_once("functions.inc");


function asterisk_extensions_conf_generate() {
	global $config, $g;
	
	$fd = fopen("/usr/local/etc/asterisk/extensions.conf", "w");
	if (!$fd) {
		printf("Error: cannot open extensions.conf in asterisk_extensions_conf_generate().\n");
		return 1;
	}
	
	$conf = <<<EOD

[macro-normal];
exten => s,1,Dial($\{ARG1},30)
exten => s,n,Goto(s-$\{DIALSTATUS},1)
exten => s-NOANSWER,1,VoiceMail($\{ARG2},u)
exten => s-BUSY,1,VoiceMail($\{ARG2},b)
exten => s-ANSWER,1,Hangup()
exten => _s-.,1,Goto(s-NOANSWER,1)

[internal]
exten => _XXXX,1,Macro(normal|SIP/$\{EXTEN}|$\{EXTEN})

exten => 00003246,1,Answer()
exten => 00003246,2,Echo()
exten => 00003246,3,Hangup()

exten => 000064554,1,Answer()
exten => 000064554,2,Milliwatt()
exten => 000064554,3,Hangup()


EOD;
	
	// conference rooms
	$rooms = asterisk_conferencing_get_rooms();
	
	foreach ($rooms as $room) {
		$conf .= "exten => {$room['number']},1,Answer()\n";
		$conf .= "exten => {$room['number']},2,MeetMe({$room['number']},M)\n";
		$conf .= "exten => {$room['number']},3,Hangup()\n\n";
	}

	$conf .= "\n\n";

	// sip providers
	$a_sipproviders = asterisk_sip_get_providers();

	foreach ($a_sipproviders as $sp) {
		$conf .= "[{$sp['uniqid']}]\n";
		$conf .= "exten => {$sp['username']},1,Answer()\n";
		$conf .= "exten => {$sp['username']},2,Dial(". 
			asterisk_uniqid_to_dial($sp['incomingextension']) .",30)\n";
		$conf .= "exten => {$sp['username']},3,Hangup()\n";
		$conf .= "exten => _{$sp['prefix']}.,1,Dial(SIP/$\{EXTEN:". strlen($sp['prefix']) ."}@{$sp['uniqid']},30)\n";
		$conf .= "\n";
	}

	// sip phones
	$a_sipphones = asterisk_sip_get_phones();

	foreach ($a_sipphones as $sp) {
		$conf .= "[{$sp['uniqid']}]\n";
		if (isset($sp['provider'])) {
			foreach ($sp['provider'] as $provider) {
				$conf .= "include => $provider\n";
			}
		}
		$conf .= "include => internal\n";
		$conf .= "\n";
	}
	
	fwrite($fd, $conf);
	fclose($fd);
	
	return 0;
}


function asterisk_extensions_reload() {
	
	return asterisk_exec("dialplan reload");

}