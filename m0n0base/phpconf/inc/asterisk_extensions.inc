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

[macro-internal]
exten => s,1,Dial($\{ARG2},30)
exten => s,2,Hangup()

[internal]
exten => _XXXX,1,Macro(internal|$\{EXTEN}|SIP/$\{EXTEN})
exten => 512,1,Answer()
exten => 512,2,Echo()
exten => 512,3,Hangup()

EOD;

	$conf .= "\n\n";

	// sip providers
	if (!is_array($config['providers']['sipprovider']))
		$config['providers']['sipprovider'] = array();

	$a_sipproviders = $config['providers']['sipprovider'];

	foreach ($a_sipproviders as $sp) {
		$conf .= "[{$sp['uniqid']}]\n";
		$conf .= "exten => {$sp['username']},1,Answer()\n";
		$conf .= "exten => {$sp['username']},2,Dial(". 
			asterisk_uniqid_to_dial($sp['incomingphone']) .",30)\n";
		$conf .= "exten => {$sp['username']},3,Hangup()\n";
		$conf .= "exten => _{$sp['prefix']}.,1,Dial(SIP/$\{EXTEN:". strlen($sp['prefix']) ."}@{$sp['uniqid']},30)\n";
		$conf .= "\n";
	}

	// sip phones
	if (!is_array($config['phones']['sipphone']))
		$config['phones']['sipphone'] = array();

	$a_sipphones = $config['phones']['sipphone'];

	foreach ($a_sipphones as $sp) {
		$conf .= "[{$sp['uniqid']}]\n";
		if (isset($sp['provider']))
			foreach ($sp['provider'] as $provider) {
				$conf .= "include => $provider\n";
		}
		$conf .= "include => internal\n";
		$conf .= "\n";
	}
	
	fwrite($fd, $conf);
	fclose($fd);
	
	return 0;
}


function asterisk_extensions_reload() {
	
	asterisk_exec("dialplan reload");

}