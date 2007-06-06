<?php
/*
	$Id: iax.inc 117 2007-06-04 12:13:10Z michael.iedema $
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


function iax_get_providers() {
	global $config;
	
	if (!is_array($config['iax']['provider']))
		$config['iax']['provider'] = array();
		
	iax_sort_providers();
		
	return $config['iax']['provider'];
}

function iax_get_phones() {
	global $config;
	
	if (!is_array($config['iax']['phone']))
		$config['iax']['phone'] = array();
		
	iax_sort_phones();
		
	return $config['iax']['phone'];
}

function iax_get_phone($uniqid) {
	$phones = iax_get_phones();
	foreach ($phones as $phone)
		if($phone['uniqid'] == $uniqid)
			return $phone;
}

function iax_get_extensions() {
	global $config;
	
	$extensions = array();
	if (is_array($config['iax']['phones']))
		foreach($config['iax']['phones'] as $phone)
			$extensions[] = $phone['extension'];

	return $extensions;
}

function iax_get_prefixes() {
	global $config;
	
	$prefixes = array();
	if (is_array($config['iax']['provider']))
		foreach($config['iax']['provider'] as $provider)
			$prefixes[] = $provider['prefix'];

	return $prefixes;	
}

function iax_conf_generate() {
	global $config, $g;
	
	$fd = fopen("/usr/local/etc/asterisk/iax.conf", "w");
	if (!$fd) {
		printf("Error: cannot open iax.conf in iax_conf_generate().\n");
		return 1;
	}
	
	$port = "4569";
	if (isset($config['iax']['port']))
		$port = $config['iax']['port'];

	$conf = <<<EOD
	
[general]
context=default
bindport=$port
bindaddr=0.0.0.0
delayreject=yes



EOD;

	
	// providers
	if (!is_array($config['iax']['provider']))
		$config['iax']['provider'] = array();
		
	$iaxproviders = $config['iax']['provider'];

	// provider registrations
	foreach($iaxproviders as $p) {
		$conf .= "register => {$p['username']}";
		if($p['secret']) {
			$conf .= ":{$p['secret']}";
		}
		$conf .= "@{$p['host']}";
		if($p['port']) {
			$conf .= ":{$p['port']}";	
		}
		$conf .= "\n\n";
	}
	
	// provider definitions
	foreach($iaxproviders as $p) {
		$conf .= "[{$p['uniqid']}]\n";
		$conf .= "type=friend\n";
		$conf .= "username={$p['username']}\n";
		$conf .= "secret={$p['secret']}\n";
		$conf .= "context={$p['uniqid']}\n";
		$conf .= "host={$p['host']}\n";
		if ($p['port'])
			$conf .= "port={$p['port']}";		
		$conf .= "qualify=15000\n";
		$conf .= "notransfer=yes\n";			
		$conf .= "disallow=all\n";
	
		if(is_array($p['codec'])) {
			foreach ($p['codec'] as $codec)
				$conf .= "allow=$codec\n";
		} else {
			$conf .= "allow={$p['codec']}\n";
		}
		
		$conf .= "\n\n";
	}

	// phones
	if (!is_array($config['iax']['phone']))
		$config['iax']['phone'] = array();
		
	$iaxphones = $config['iax']['phone'];
	
	foreach($iaxphones as $p) {
		$conf .= "[{$p['extension']}]\n";
		$conf .= "type=friend\n";
		$conf .= "secret={$p['secret']}\n";
		$conf .= "context={$p['uniqid']}\n";
		$conf .= "callerid=\"{$p['callerid']}\" <{$p['extension']}>\n";
		$conf .= "host=dynamic\n";
		$conf .= "qualify=15000\n";
		$conf .= "notransfer=yes\n";
		$conf .= "disallow=all\n";
	
		if(is_array($p['codec'])) {
			foreach ($p['codec'] as $codec)
				$conf .= "allow=$codec\n";
		} else {
			$conf .= "allow={$p['codec']}\n";
		}
		
		$conf .= "\n\n";
	}

	fwrite($fd, $conf);
	fclose($fd);
	
	return 0;
}


function iax_reload() {
	
	return asterisk_exec("module reload chan_iax2.so");
}


function iax_sort_phones() {
	global $g, $config;

	usort($config['iax']['phone'], "_iaxphonecmp");	
}
function _iaxphonecmp($a, $b) {
	return strcmp($a['extension'], $b['extension']);
}


function iax_sort_providers() {
	global $g, $config;

	usort($config['iax']['provider'], "_iaxprovidercmp");	
}
function _iaxprovidercmp($a, $b) {
	return strcmp($a['prefix'], $b['prefix']);
}

?>