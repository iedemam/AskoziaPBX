<?php
/*
	$Id: services.inc 171 2006-12-02 18:47:31Z mkasper $
	part of m0n0wall (http://m0n0.ch/wall)
	
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
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

function asterisk_sip_get_settings() {
	global $config;
	
	$sipsettings = <<<EOD

[general]
context=default
allowoverlap=no
bindport=5060
bindaddr=0.0.0.0
srvlookup=yes
useragent=AskoziaPBX

EOD;

	return $sipsettings;
}


function asterisk_sip_get_allowed_codecs() {
	
	$codecs = <<<EOD

disallow=all
allow=ulaw
allow=gsm

EOD;

	return $codecs;
}


function asterisk_sip_get_providers() {
	
	$providers = <<<EOD

;register => 1234:password@mysipprovider.com	
;register => 2345:password@sip_proxy/1234
;registertimeout=20		; retry registration calls every 20 seconds (default)
;registerattempts=10		; Number of registration attempts before we give up	

EOD;
	
	return $providers;
}


function asterisk_sip_get_nat_config() {
	
	$natconfig = <<<EOD
	
;----------------------------------------- NAT SUPPORT ------------------------
;externip = 200.201.202.203	; Address that we're going to put in outbound SIP
;externhost=foo.dyndns.net	; Alternatively you can specify an 
;externrefresh=10		; How often to refresh externhost if 
;localnet=192.168.0.0/255.255.0.0; All RFC 1918 addresses are local networks
;localnet=10.0.0.0/255.0.0.0	; Also RFC1918
;localnet=172.16.0.0/12		; Another RFC1918 with CIDR notation
;localnet=169.254.0.0/255.255.0.0 ;Zero conf local network
;nat=no				; Global NAT settings  (Affects all peers and users)

EOD;

	return $natconfig;
}


function asterisk_sip_get_phones() {
	global $config, $g;
	
	if (!is_array($config['phones']['sipphone']))
		$config['phones']['sipphone'] = array();
		
	$sipphones = $config['phones']['sipphone'];
	
	foreach($sipphones as $sp) {
		$conf .= "[{$sp['extension']}]\n";
		$conf .= "type=friend\n";
		$conf .= "secret={$sp['secret']}\n";
		$conf .= "context=default\n";
		$conf .= "callerid=\"{$sp['name']}\" <{$sp['extension']}>\n";
		$conf .= "host=dynamic\n";
		$conf .= "nat=no\n";
		$conf .= "canreinvite=yes\n";
		$conf .= "\n";
	}

	return $conf;
}


function asterisk_sip_conf_generate() {
	global $config, $g;
	
	$fd = fopen("/usr/local/etc/asterisk/sip.conf", "w");
	if (!$fd) {
		printf("Error: cannot open sip.conf in asterisk_write_sip_conf().\n");
		return 1;
	}

	$sipconf = asterisk_sip_get_settings();
	$sipconf .= asterisk_sip_get_allowed_codecs();
	$sipconf .= asterisk_sip_get_providers();
	$sipconf .= asterisk_sip_get_nat_config();
	$sipconf .= asterisk_sip_get_phones();

	fwrite($fd, $sipconf);
	fclose($fd);
	
	return 0;
}


function asterisk_sip_reload() {
	
	asterisk_exec("module reload chan_sip.so");
	
}

?>