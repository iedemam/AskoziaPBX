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


function analog_get_providers() {
	global $config;
	
	if (!is_array($config['analog']['provider']))
		$config['analog']['provider'] = array();
		
	analog_sort_providers();
		
	return $config['analog']['provider'];
}

function analog_get_provider($uniqid) {
	global $config, $uniqid_map;
	return $config['analog']['provider'][$uniqid_map[$uniqid]];
}

function analog_get_phones() {
	global $config;
	
	if (!is_array($config['analog']['phone']))
		$config['analog']['phone'] = array();
		
	analog_sort_phones();
		
	return $config['analog']['phone'];
}

function analog_get_phone($uniqid) {
	global $config, $uniqid_map;
	return $config['analog']['phone'][$uniqid_map[$uniqid]];
}

function analog_get_extensions() {
	global $config;
	
	$extensions = array();
	if (is_array($config['analog']['phones']))
		foreach($config['analog']['phones'] as $phone)
			$extensions[] = $phone['extension'];

	return $extensions;
}

function analog_get_dialpatterns() {
	global $config;
	
	$dialpatterns = array();
	if (is_array($config['analog']['provider']))
		foreach($config['analog']['provider'] as $provider)
			$dialpatterns[$provider['uniqid']] = $provider['dialpattern'];

	return $dialpatterns;
}

function analog_sort_phones() {
	global $g, $config;

	usort($config['analog']['phone'], "_analogphonecmp");	
}
function _analogphonecmp($a, $b) {
    if ($a['extension'] == $b['extension']) {
        return 0;
    }
    return ($a['extension'] < $b['extension']) ? -1 : 1;
}


function analog_sort_providers() {
	global $g, $config;

	usort($config['analog']['provider'], "_analogprovidercmp");	
}
function _analogprovidercmp($a, $b) {
	return strcmp($a['name'], $b['name']);
}

// XXX multiple cards will still cause problems here
function analog_get_recognized_ab_unit_numbers() {
	global $g;

	exec("/sbin/dmesg", $dmesg_lines);
	
	/* find last copyright line (output from previous boots may be present) */
	$lastcpline = 0;
	
	for ($i = 0; $i < count($dmesg_lines); $i++) {
		if (strstr($dmesg_lines[$i], "Copyright (c) 1992-"))
			$lastcpline = $i;
	}
	
	$controllers = array();
	for ($i = $lastcpline; $i < count($dmesg_lines); $i++) {
		// fxo ports
		if (preg_match("/\b[0-9]+: Installed -- AUTO FXO/", $dmesg_lines[$i], &$matches)) {
			$num = substr($matches[0], 0, strpos($matches[0],":"));
			$controllers[$num+1] = "fxo";

		} else if (strpos($dmesg_lines[$i], "Found a Wildcard FXO: ") !== false) {
			$controllers[1] = "fxo";


		// fxs ports
		} else if (preg_match("/\b[0-9]+: Installed -- AUTO FXS/", $dmesg_lines[$i], &$matches)) {
			$num = substr($matches[0], 0, strpos($matches[0],":"));
			$controllers[$num+1] = "fxs";
		}
	}
	
	return $controllers;
}

function analog_get_ab_interfaces() {
	global $config;
	
	if (!is_array($config['interfaces']['ab-unit']))
		$config['interfaces']['ab-unit'] = array();
		
	analog_sort_ab_interfaces();
		
	return $config['interfaces']['ab-unit'];
}

function analog_get_ab_interface($unit) {
	
	$interfaces = analog_get_ab_interfaces();
	foreach ($interfaces as $interface) {
		if ($interface['unit'] == $unit) {
			return $interface;
		}
	}
}

function analog_sort_ab_interfaces() {
	global $config;

	usort($config['interfaces']['ab-unit'], "_abinterfacescmp");	
}
function _abinterfacescmp($a, $b) {
    if ($a['unit'] == $b['unit']) {
        return 0;
    }
    return ($a['unit'] < $b['unit']) ? -1 : 1;
}

function analog_configure() {	
	if(!count(analog_get_recognized_ab_unit_numbers())) {
		return 0;
	}
	$res = 0;
	$res |= mwexec("/sbin/ztcfg");
	return $res;
}

function analog_reload() {
	return asterisk_exec("module reload chan_zap.so");
}

function analog_conf_generate() {
	$res = 0;
	$res |= analog_zaptel_conf_generate();
	$res |= analog_zapata_conf_generate();
	return $res;
}

function analog_zaptel_conf_generate() {
	global $config, $g;

	$fd = fopen("/usr/local/etc/zaptel.conf", "w");
	if (!$fd) {
		printf("Error: cannot open zaptel.conf in analog_zaptel_conf_generate().\n");
		return 1;
	}

	// interface definitions
	$abinterfaces = analog_get_ab_interfaces();
	foreach($abinterfaces as $abi) {
		if ($abi['type'] == "fxo") {
			if (!$fxsks_line) {
				$fxsks_line = "fxsks={$abi['unit']}";
			} else {
				$fxsks_line .= ",{$abi['unit']}";
			}
		} else if ($abi['type'] == "fxs"){
			if (!$fxoks_line) {
				$fxoks_line = "fxoks={$abi['unit']}";
			} else {
				$fxoks_line .= ",{$abi['unit']}";
			}
		}
	}
	
	if ($fxsks_line) {
		$conf .= "$fxsks_line\n";
	}
	if ($fxoks_line) {
		$conf .= "$fxoks_line\n";
	}
	
	$analogconfig = $config['services']['analog'];
	
	if (!is_array($analogconfig['loadzone'])) {
		$loadzones[] = "us";
	} else {
		$loadzones = $analogconfig['loadzone'];
	}
	
	$n = count($loadzones);
	for ($i = 0; $i < $n; $i++) {
		$conf .= "loadzone={$loadzones[$i]}\n";
	}
	$conf .= "defaultzone={$loadzones[0]}\n";

	$conf .= "\n";

	fwrite($fd, $conf);
	fclose($fd);

	return 0;
	
}

function analog_zapata_conf_generate() {
	global $config, $g;

	$fd = fopen("/usr/local/etc/asterisk/zapata.conf", "w");
	if (!$fd) {
		printf("Error: cannot open zapata.conf in analog_zapata_conf_generate().\n");
		return 1;
	}
	
	$conf .= "[trunkgroups]\n\n";
	
	$conf .= "[channels]\n";
	
	$analogproviders = analog_get_providers();
	foreach ($analogproviders as $p) {

		if ($p['startsignal']) {
			$conf .= "signalling=fxs_{$p['startsignal']}\n";
		} else {
			$conf .= "signalling=fxs_ks\n";
		}

		$conf .= "busydetect=yes\n";
		$conf .= "busycount=3\n";

		if ($p['echocancel']) {
			$conf .= "echocancel=yes\n";
		} else {
			$conf .= "echocancel=no\n";
		}
		
		$conf .= "echocancelwhenbridged=no\n";
		
		if (!$p['language']) {
			$conf .= "language=en\n";
		} else {
			$conf .= "language={$p['language']}\n";
		}
		$conf .= "context={$p['uniqid']}\n";
		$conf .= "channel => {$p['interface']}\n";
		$conf .= "\n";
	}
	
	$analogphones = analog_get_phones();
	foreach ($analogphones as $p) {
		$conf .= "callerid=\"{$p['callerid']}\" <{$p['extension']}>\n";

		if ($p['startsignal']) {
			$conf .= "signalling=fxo_{$p['startsignal']}\n";
		} else {
			$conf .= "signalling=fxo_ks\n";
		}

		$conf .= "busydetect=yes\n";
		$conf .= "busycount=3\n";
		
		if ($p['echocancel']) {
			$conf .= "echocancel=yes\n";
		} else {
			$conf .= "echocancel=no\n";
		}
		
		$conf .= "echocancelwhenbridged=no\n";
		
		if (!$p['language']) {
			$conf .= "language=en\n";
		} else {
			$conf .= "language={$p['language']}\n";
		}
		$conf .= "context={$p['uniqid']}\n";
		$conf .= "channel => {$p['interface']}\n";
		$conf .= "\n";
	}

	$conf .= "\n";

	fwrite($fd, $conf);
	fclose($fd);

	return 0;
}



?>
