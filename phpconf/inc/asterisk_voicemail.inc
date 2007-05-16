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


function asterisk_voicemail_conf_generate() {
	
	$res = 0;
	$res |= voicemail_generate_msmtp_conf();
	$res |= voicemail_generate_voicemail_conf();
	return $res;
}

function voicemail_generate_voicemail_conf() {
	global $config;
	
	$fd = fopen("/usr/local/etc/asterisk/voicemail.conf", "w");
	if (!$fd) {
		printf("Error: cannot open voicemail.conf in voicemail_generate_voicemail_conf().\n");
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
emailbody=Dear $\{VM_NAME}:\\n\\nA call from $\{VM_CALLERID} just landed in your voicemail.\\n\\nHere's the $\{VM_DUR} long message they left for you on $\{VM_DATE}.\\n\\n--Your Phone System

emaildateformat=%A, %B %d, %Y at %r

mailcmd=/usr/sbin/msmtp -C /etc/msmtp.conf -t

sendvoicemail=yes	; Context to Send voicemail from [option 5 from the advanced menu]

[zonemessages]
eastern=America/New_York|'vm-received' Q 'digits/at' IMp
central=America/Chicago|'vm-received' Q 'digits/at' IMp
central24=America/Chicago|'vm-received' q 'digits/at' H N 'hours'
military=Zulu|'vm-received' q 'digits/at' H N 'hours' 'phonetic/z_p'
european=Europe/Copenhagen|'vm-received' a d b 'digits/at' HM

[default]

EOD;

	// sip phones
	if (!is_array($config['sip']['phone']))
		$config['sip']['phone'] = array();
	
	$sipphones = $config['sip']['phone'];

	foreach($sipphones as $sp) {
		if(!$sp['voicemailbox'])
			continue;
		// is eliminating VoiceMailMain from the dialplan enough to keep
		// this default password from being a privacy risk?
		$conf .= "{$sp['extension']} => 0000,{$sp['callerid']},{$sp['voicemailbox']},,delete=yes\n";
	}
	
	fwrite($fd, $conf);
	fclose($fd);
	
	return 0;
}


function voicemail_generate_msmtp_conf() {
	global $config;
	
	$fd = fopen("/etc/msmtp.conf", "w");
	if (!$fd) {
		printf("Error: cannot open msmtp.conf in voicemail_generate_msmtp_conf().\n");
		return 1;
	}
	
	$conf = <<<EOD

# A system wide configuration is optional.
# If it exists, it usually defines a default account.
# This allows msmtp to be used like /usr/sbin/sendmail.
#account default

# The SMTP smarthost.
#host mailhub.oursite.example

# Construct envelope-from addresses of the form "user@oursite.example".
#auto_from on
#maildomain oursite.example

# Use TLS.
#tls on
#tls_trust_file /etc/ssl/certs/ca-certificates.crt

# Syslog logging with facility LOG_MAIL instead of the default LOG_USER.
syslog LOG_MAIL


EOD;

	fwrite($fd, $conf);
	fclose($fd);
	
	chmod("/etc/msntp.conf", 600);

	return 0;
}

function asterisk_voicemail_reload() {
	
	return asterisk_exec("module reload app_voicemail.so");
}