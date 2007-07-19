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

/* include all configuration functions */
require_once("functions.inc");

/*
	isdnconfig - configure ISDN4BSD, version 1.6.0, compiled Jul  9 2007 16:34:01
	usage:  isdnconfig -u <unit> -i <number> -p <protocol> [parameters]
		-u unit         set controller unit (default is zero)
		-c channel      set channel number (default is zero)
		-p enum         set D-channel protocol
		-i value        set D-channel serial number
		-a              activate PH-line
		-D              deactivate PH-line
		-t              set TE-mode
		-n              set NT-mode
		-r              reset device
		-m              set PCM cable unit (default is zero)
		-E enum         display information about enum

*/

function isdn_get_providers() {
	global $config;
	
	if (!is_array($config['isdn']['provider']))
		$config['isdn']['provider'] = array();
		
	isdn_sort_providers();
		
	return $config['isdn']['provider'];
}

function isdn_get_phones() {
	global $config;
	
	if (!is_array($config['isdn']['phone']))
		$config['isdn']['phone'] = array();
		
	isdn_sort_phones();
		
	return $config['isdn']['phone'];
}

function isdn_get_phone($uniqid) {
	global $config, $uniqid_map;
	return $config['isdn']['phone'][$uniqid_map[$uniqid]];
}

function isdn_get_extensions() {
	global $config;
	
	$extensions = array();
	if (is_array($config['isdn']['phones']))
		foreach($config['isdn']['phones'] as $phone)
			$extensions[] = $phone['extension'];

	return $extensions;
}

function isdn_get_dialpatterns() {
	global $config;
	
	$dialpatterns = array();
	if (is_array($config['isdn']['provider']))
		foreach($config['isdn']['provider'] as $provider)
			$dialpatterns[$provider['uniqid']] = $provider['dialpattern'];

	return $dialpatterns;
}

function isdn_sort_phones() {
	global $g, $config;

	usort($config['isdn']['phone'], "_isdnphonecmp");	
}
function _isdnphonecmp($a, $b) {
	return strcmp($a['extension'], $b['extension']);
}


function isdn_sort_providers() {
	global $g, $config;

	usort($config['isdn']['provider'], "_isdnprovidercmp");	
}
function _isdnprovidercmp($a, $b) {
	return strcmp($a['name'], $b['name']);
}

function isdn_get_recognized_unit_numbers() {
	global $g;
	
	$dmesg_lines = explode("\n", file_get_contents("{$g['varlog_path']}/dmesg.boot"));
	
	$controllers = array();
	foreach($dmesg_lines as $line) {
		if (strpos($line, "Attaching I4B controller") !== false) {
			preg_match("/\b[0-9]+\b/", $line, &$matches);
			$controllers[$matches[0]] = "found";
		}
	}
	
	return $controllers;
}

function isdn_get_interfaces() {
	global $config;
	
	if (!is_array($config['interfaces']['isdn-unit']))
		$config['interfaces']['isdn-unit'] = array();
		
	isdn_sort_interfaces();
		
	return $config['interfaces']['isdn-unit'];
}

function isdn_get_interface($unit) {
	
	$interfaces = isdn_get_interfaces();
	foreach ($interfaces as $interface) {
		if ($interface['unit'] == $unit) {
			return $interface;
		}
	}
}

function isdn_sort_interfaces() {
	global $config;

	usort($config['interfaces']['isdn-unit'], "_isdninterfacescmp");	
}
function _isdninterfacescmp($a, $b) {
	return strcmp($a['unit'], $b['unit']);
}

function isdn_configure() {
	
	$res = 0;
	$interfaces = isdn_get_interfaces();
	foreach ($interfaces as $interface) {
		
		$command = "/sbin/isdnconfig -u {$interface['unit']} -p {$interface['mode']}";
		
		if ($interface['pcmmaster']) {
			$command .= " pcm_master";
		} else {
			$command .= " pcm_slave";
		}
		
		if ($interface['nopwrsave']) {
			$command .= " power_on";
		} else {
			$command .= " pwr_save";
		}
		
		if ($interface['pollmode']) {
			$command .= " poll_mode";
		} else {
			$command .= " intr_mode";
		}
		
		$res |= mwexec($command);
	}

	return $res;
}

function isdn_reload() {
	return asterisk_exec("module reload chan_capi.so");
}

function isdn_conf_generate() {
	global $config, $g;
	
	$fd = fopen("/usr/local/etc/asterisk/capi.conf", "w");
	if (!$fd) {
		printf("Error: cannot open capi.conf in isdn_conf_generate().\n");
		return 1;
	}
	
	$isdnconfig = $config['services']['isdn'];
	
	$law = "alaw";
	if (isset($isdnconfig['law']))
		$law = $isdnconfig['law'];

	$nationalprefix = "0";
	if (isset($isdnconfig['nationalprefix']))
		$nationalprefix = $isdnconfig['nationalprefix'];

	$internationalprefix = "00";
	if (isset($isdnconfig['internationalprefix']))
		$internationalprefix = $isdnconfig['internationalprefix'];
		
	$conf = <<<EOD
	
[general]
nationalprefix=$nationalprefix
internationalprefix=$internationalprefix
rxgain=1.0
txgain=1.0
;debug=yes

EOD;

	if ($law != "alaw") {
		$conf .= "ulaw=yes\n";
	}
	
	$conf .= "\n\n";
	
	// provider definitions
	$isdnproviders = isdn_get_providers();
	foreach($isdnproviders as $p) {
		$conf .= "; {$p['name']}\n";
		$conf .= "[{$p['uniqid']}]\n";

		$interface = isdn_get_interface($p['interface']);
		
		$conf .= "isdnmode=msn\n";
		$conf .= "msn={$p['msn']}\n";
		$conf .= "incomingmsn=*\n";
		$conf .= "controller={$interface['unit']}\n";
		$conf .= "group=1\n";
		//$conf .= ";prefix=0\n";
		$conf .= "softdtmf=off\n";
		$conf .= "relaxdtmf=off\n";
		//$conf .= "accountcode=\n";
		
		$conf .= "context={$p['uniqid']}\n";

		$conf .= "holdtype=local\n";
		//$conf .= ";immediate=yes\n";
		$conf .= "echocancel=yes\n";
		//$conf .= "echosquelch=yes\n";
		//$conf .= ";echocancelold=yes\n";
		//$conf .= ";echotail=64\n";
		//$conf .= ";bridge=yes\n";
		//$conf .= ";callgroup=1\n";
		//$conf .= ";deflect=1234567\n";
		$conf .= "devices=2\n";
		$conf .= "\n";
	}
	
	$conf .= "\n";
	
	// phone definitions
	$isdnphones = isdn_get_phones();
	foreach($isdnphones as $p) {
		$conf .= "; {$p['callerid']}\n";
		$conf .= "[{$p['extension']}]\n";

		$interface = isdn_get_interface($p['interface']);
		
		$conf .= "ntmode=yes\n";
		$conf .= "isdnmode=did\n";
		$conf .= "incomingmsn=*\n";
		$conf .= "controller={$interface['unit']}\n";
		$conf .= "group=1\n";
		//$conf .= ";prefix=0\n";
		$conf .= "softdtmf=off\n";
		$conf .= "relaxdtmf=off\n";
		//$conf .= "accountcode=\n";
		
		$conf .= "context={$p['uniqid']}\n";

		$conf .= "holdtype=local\n";
		//$conf .= ";immediate=yes\n";
		$conf .= "echocancel=yes\n";
		//$conf .= "echosquelch=yes\n";
		//$conf .= ";echocancelold=yes\n";
		//$conf .= ";echotail=64\n";
		//$conf .= ";bridge=yes\n";
		//$conf .= ";callgroup=1\n";
		//$conf .= ";deflect=1234567\n";
		$conf .= "devices=2\n";
		$conf .= "\n";
	}

	fwrite($fd, $conf);
	fclose($fd);

	return 0;
}


?>
