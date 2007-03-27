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


function asterisk_sip_get_providers() {
	global $config;
	
	if (!is_array($config['providers']['sipprovider']))
		$config['providers']['sipprovider'] = array();
		
	asterisk_sip_sort_providers();
		
	return $config['providers']['sipprovider'];
}

function asterisk_sip_get_phones() {
	global $config;
	
	if (!is_array($config['phones']['sipphone']))
		$config['phones']['sipphone'] = array();
		
	asterisk_sip_sort_phones();
		
	return $config['phones']['sipphone'];
}

function asterisk_sip_get_phone($uniqid) {
	$phones = asterisk_sip_get_phones();
	foreach ($phones as $phone)
		if($phone['uniqid'] == $uniqid)
			return $phone;
}

function asterisk_sip_get_extensions() {
	global $config;
	
	$extensions = array();
	if (is_array($config['phones']['sipphone']))
		foreach($config['phones']['sipphone'] as $phone)
			$extensions[] = $phone['extension'];

	return $extensions;
}

function asterisk_sip_get_prefixes() {
	global $config;
	
	$prefixes = array();
	if (is_array($config['providers']['sipprovider']))
		foreach($config['providers']['sipprovider'] as $provider)
			$prefixes[] = $provider['prefix'];

	return $prefixes;	
}

function asterisk_sip_conf_generate() {
	global $config, $g;
	
	$fd = fopen("/usr/local/etc/asterisk/sip.conf", "w");
	if (!$fd) {
		printf("Error: cannot open sip.conf in asterisk_sip_conf_generate().\n");
		return 1;
	}
	
	$port = "5060";
	if(isset($config['services']['sip']['port']))
		$port = $config['services']['sip']['port'];

	$conf = <<<EOD
	
[general]
context=default
allowoverlap=no
bindport=$port
bindaddr=0.0.0.0
srvlookup=yes
useragent=AskoziaPBX
videosupport=yes

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

	$conf .= "\n\n";
	
	// providers
	if (!is_array($config['providers']['sipprovider']))
		$config['providers']['sipprovider'] = array();
		
	$sipproviders = $config['providers']['sipprovider'];

	// provider registrations
	foreach($sipproviders as $sp) {
		$conf .= "register => {$sp['username']}";
		if($sp['secret']) {
			$conf .= ":{$sp['secret']}";
			if($sp['authuser']) {
				$conf.= ":{$sp['authuser']}";
			}
		}
		$conf .= "@{$sp['host']}";
		if($sp['port']) {
			$conf .= ":{$sp['port']}";	
		}
		$conf .= "/{$sp['username']}";	
		$conf .= "\n\n";
	}
	
	// provider definitions
	foreach($sipproviders as $sp) {
		$conf .= "[{$sp['uniqid']}]\n";
		$conf .= "type=friend\n";
		$conf .= "username={$sp['username']}\n";
		$conf .= "secret={$sp['secret']}\n";
		$conf .= "fromuser={$sp['username']}\n";
		$conf .= "context={$sp['uniqid']}\n";
		$conf .= "host={$sp['host']}\n";
		$conf .= "nat=no\n";
		$conf .= "qualify=60000\n";
		$conf .= "canreinvite=no\n";
		$conf .= "insecure=very\n";
		$conf .= "disallow=all\n";
	
		if(is_array($sp['codec'])) {
			foreach ($sp['codec'] as $codec)
				$conf .= "allow=$codec\n";
		} else {
			$conf .= "allow={$sp['codec']}\n";
		}
		
		$conf .= "\n\n";
	}

	// phones
	if (!is_array($config['phones']['sipphone']))
		$config['phones']['sipphone'] = array();
		
	$sipphones = $config['phones']['sipphone'];
	
	foreach($sipphones as $sp) {
		$conf .= "[{$sp['extension']}]\n";
		$conf .= "type=friend\n";
		$conf .= "secret={$sp['secret']}\n";
		$conf .= "context={$sp['uniqid']}\n";
		$conf .= "callerid=\"{$sp['callerid']}\" <{$sp['extension']}>\n";
		$conf .= "host=dynamic\n";
		$conf .= "nat=yes\n";
		$conf .= "qualify=60000\n";
		$conf .= "canreinvite=no\n";
		$conf .= "disallow=all\n";
	
		if(is_array($sp['codec'])) {
			foreach ($sp['codec'] as $codec)
				$conf .= "allow=$codec\n";
		} else {
			$conf .= "allow={$sp['codec']}\n";
		}
		
		$conf .= "\n\n";
	}

	fwrite($fd, $conf);
	fclose($fd);
	
	return 0;
}


function asterisk_sip_reload() {
	
	asterisk_exec("module reload chan_sip.so");
	
}


function asterisk_sip_sort_phones() {
	global $g, $config;

	usort($config['phones']['sipphone'], "_sipphonecmp");	
}
function _sipphonecmp($a, $b) {
	return strcmp($a['extension'], $b['extension']);
}


function asterisk_sip_sort_providers() {
	global $g, $config;

	usort($config['providers']['sipprovider'], "_sipprovidercmp");	
}
function _sipprovidercmp($a, $b) {
	return strcmp($a['prefix'], $b['prefix']);
}

?>