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



function voicemail_conf_generate() {
	global $config;
	
	$fd = fopen("/usr/local/etc/asterisk/voicemail.conf", "w");
	if (!$fd) {
		printf("Error: cannot open voicemail.conf in voicemail_conf_generate().\n");
		return 1;
	}
	
	$conf = <<<EOD

[general]
format=wav49
attach=yes
skipms=3000
maxsilence=10
silencethreshold=128
maxlogins=3

serveremail=AskoziaPBX
fromstring=AskoziaPBX Voicemail
emailsubject=New voicemail message (box: $\{VM_MAILBOX})
emailbody=Dear $\{VM_NAME}:\\n\\nA call from $\{VM_CALLERID} just landed in your voicemail.\\n\\nHere's the $\{VM_DUR} long message that was left for you on $\{VM_DATE}.\\n\\n--Your Phone System

emaildateformat=%A, %B %d, %Y at %r

mailcmd=/usr/local/bin/msmtp -C /etc/msmtp.conf -t

[zonemessages]
eastern=America/New_York|'vm-received' Q 'digits/at' IMp
central=America/Chicago|'vm-received' Q 'digits/at' IMp
central24=America/Chicago|'vm-received' q 'digits/at' H N 'hours'
military=Zulu|'vm-received' q 'digits/at' H N 'hours' 'phonetic/z_p'
european=Europe/Copenhagen|'vm-received' a d b 'digits/at' HM

[default]

EOD;

	// sip phones
	$sipphones = sip_get_phones();

	foreach($sipphones as $p) {
		if(!$p['voicemailbox'])
			continue;
		// is eliminating VoiceMailMain from the dialplan enough to keep
		// this default password from being a privacy risk?
		$conf .= "{$p['extension']} => 0000,{$p['callerid']},{$p['voicemailbox']},,delete=yes\n";
	}
	
	// iax phones
	$iaxphones = iax_get_phones();

	foreach($iaxphones as $p) {
		if(!$p['voicemailbox'])
			continue;
		$conf .= "{$p['extension']} => 0000,{$p['callerid']},{$p['voicemailbox']},,delete=yes\n";
	}	
	
	fwrite($fd, $conf);
	fclose($fd);
	
	return 0;
}


function msmtp_conf_generate() {
	global $config;
	
	$fd = fopen("/etc/msmtp.conf", "w");
	if (!$fd) {
		printf("Error: cannot open msmtp.conf in msmtp_conf_generate().\n");
		return 1;
	}
	
	$vmconfig = $config['voicemail'];

	$address = explode("@", $vmconfig['address']);
	$domain = $address[1];
	
	$conf = "account default\n";
	$conf .= "host {$vmconfig['host']}\n";
	
	if ($vmconfig['username']) {
		$conf .= "auth on\n";
		$conf .= "user {$vmconfig['username']}\n";
		$conf .= "password {$vmconfig['password']}\n";
	}
	
	if ($vmconfig['tls']) {
		$conf .= "tls on\n";
		$conf .= "tls_starttls on\n";
		$conf .= "tls_certcheck off\n";
	}
	
	if ($vmconfig['fromaddress']) {
		$conf .= "from {$vmconfig['fromaddress']}\n";
	} else {
		$conf .= "from {$vmconfig['address']}\n";
	}
	
	$conf .= "maildomain $domain\n";
	
	if ($vmconfig['port']) {
		$conf .= "port {$vmconfig['port']}\n";
	}
	
	$conf .= "syslog LOG_MAIL\n";

	fwrite($fd, $conf);
	fclose($fd);
	
	chmod("/etc/msmtp.conf", 0600);

	return 0;
}

function voicemail_reload() {
	
	return asterisk_exec("module reload app_voicemail.so");
}