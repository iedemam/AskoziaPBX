<?php
/*
	$Id: voicemail.inc 1189 2009-10-22 12:41:05Z michael.iedema $
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2009 IKT <http://itison-ikt.de>.
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


function notifications_get_email_configuration() {
	return $GLOBALS['config']['notifications']['email'];
}

function notifications_save_email_configuration($newconfig) {
	$GLOBALS['config']['notifications']['email'] = $newconfig;
	write_config();
}

function notifications_msmtp_conf_generate() {

	$voicemail_config = voicemail_get_configuration();
	$email_config = notifications_get_email_configuration();

	$address = explode("@", $email_config['address']);
	$domain = $address[1];

	$conf = "account default\n";
	$conf .= "host " . $email_config['host'] . "\n";

	if ($email_config['username']) {
		if ($email_config['authtype']) {
			$conf .= "auth " . $email_config['authtype'] . "\n";
		}
		$conf .= "user " . $email_config['username'] . "\n";
		$conf .= "password " . $email_config['password'] . "\n";
	}

	if ($email_config['enctype'] == "tls") {
		$conf .= "tls on\n";
		$conf .= "tls_starttls on\n";
		$conf .= "tls_certcheck on\n";
		$conf .= "tls_trust_file /etc/ssl/certs/ca-certificates.crt\n";
	} else if ($email_config['enctype'] == "smtps") {
		$conf .= "tls on\n";
		$conf .= "tls_starttls off\n";
		$conf .= "tls_certcheck on\n";
		$conf .= "tls_trust_file /etc/ssl/certs/ca-certificates.crt\n";
	}

	if ($voicemail_config['fromaddress']) {
		$conf .= "from " . $voicemail_config['fromaddress'] . "\n";
	} else {
		$conf .= "from " . $email_config['address'] . "\n";
	}

	$conf .= "maildomain " . $domain . "\n";

	if ($email_config['port']) {
		$conf .= "port " . $email_config['port'] . "\n";
	}

	$conf .= "syslog LOG_LOCAL0\n";


	$fd = fopen("/etc/msmtp.conf", "w");
	if (!$fd) {
		printf("Error: cannot open msmtp.conf in notifications_msmtp_conf_generate().\n");
		return 1;
	}
	fwrite($fd, $conf);
	fclose($fd);


	chmod("/etc/msmtp.conf", 0600);

	return 0;
}

?>
