<?php
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2008 IKT <http://itison-ikt.de>.
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

/**
 * Returns all providers sorted by name.
 * If no providers are defined, the global array is initialized.
 * This should be changed to return by reference to eliminate a lot of repetitive code.
 */
function isdn_get_providers() {
	global $config;
	
	if (!is_array($config['isdn']['provider']))
		$config['isdn']['provider'] = array();
		
	isdn_sort_providers();
		
	return $config['isdn']['provider'];
}

/**
 * Sorts providers by name
 */
function isdn_sort_providers() {
	global $g, $config;

	usort($config['isdn']['provider'], "pbx_sort_by_name");	
}

/**
 * Returns a specific provider based on its uniqid
 */
function isdn_get_provider($uniqid) {
	global $config, $uniqid_map;

	return $config['isdn']['provider'][$uniqid_map[$uniqid]];
}

/**
 * Returns all phones sorted by extension.
 * If no phones are defined, the global array is initialized.
 */
function isdn_get_phones() {
	global $config;
	
	if (!is_array($config['isdn']['phone']))
		$config['isdn']['phone'] = array();
		
	isdn_sort_phones();
		
	return $config['isdn']['phone'];
}

/**
 * Sorts phones by extension
 */
function isdn_sort_phones() {
	global $g, $config;

	usort($config['isdn']['phone'], "pbx_sort_by_extension");	
}

/**
 * Returns a specific phone based on its uniqid
 */
function isdn_get_phone($uniqid) {
	global $config, $uniqid_map;

	return $config['isdn']['phone'][$uniqid_map[$uniqid]];
}

/**
 * Returns an array of all assigned phone extensions
 */
function isdn_get_extensions() {
	global $config;
	
	$extensions = array();

	if (is_array($config['isdn']['phone'])) {
		foreach($config['isdn']['phone'] as $phone) {
			$extensions[] = $phone['extension'];
		}
	}

	return $extensions;
}

/**
 * Returns an array of all assigned provider dialpatterns
 */
function isdn_get_dialpatterns() {
	global $config;
	
	$dialpatterns = array();

	if (is_array($config['isdn']['provider'])) {
		foreach($config['isdn']['provider'] as $provider) {
			$dialpatterns[$provider['uniqid']] = $provider['dialpattern'];
		}
	}

	return $dialpatterns;
}

/**
 * Searches dmesg for recognized ISDN units.
 * Returns an array indexed by module number.
 */
function isdn_get_recognized_unit_numbers() {

	$dmesg_lines = system_get_dmesg_as_array();

	$controllers = array();
	foreach ($dmesg_lines as $line) {
		if (strpos($line, "Attaching I4B controller") !== false) {
			preg_match("/\b[0-9]+\b/", $line, &$matches);
			$controllers[$matches[0]] = "found";
		}
	}
	
	return $controllers;
}

/**
 * Returns all configured ISDN units still present in the system sorted by unit number.
 * If no units are configured, the global array is initialized.
 */
function isdn_get_interfaces() {
	global $config;
	
	if (!is_array($config['interfaces']['isdn-unit']))
		$config['interfaces']['isdn-unit'] = array();
		
	isdn_sort_interfaces();

	$recognized_units = array_keys(isdn_get_recognized_unit_numbers());
	$interfaces = array();
	foreach ($config['interfaces']['isdn-unit'] as $i) {
		if (in_array($i['unit'], $recognized_units)) {
			$interfaces[] = $i;
		}
	}

	return $interfaces;
}

/**
 * Sorts ISDN units by unit number
 */
function isdn_sort_interfaces() {
	global $config;

	usort($config['interfaces']['isdn-unit'], "pbx_sort_by_unit");	
}

/**
 * Returns an ISDN unit based on its unit number
 */
function isdn_get_interface($unit) {
	
	$interfaces = isdn_get_interfaces();

	foreach ($interfaces as $interface) {
		if ($interface['unit'] == $unit) {
			return $interface;
		}
	}
}

/**
 * Returns all "NT" configured ISDN units
 * If no units are configured, an empty array is returned
 */
function isdn_get_nt_interfaces() {
	
	$interfaces = isdn_get_interfaces();
	
	$nt_interfaces = array();
	foreach ($interfaces as $interface) {
		if (strpos($interface['mode'], "NT") === false) {
			continue;
		}
		$nt_interfaces[] = $interface;
	}
		
	return $nt_interfaces;
}

/**
 * Returns all "TE" configured ISDN units
 * If no units are configured, an empty array is returned
 */
function isdn_get_te_interfaces() {
	
	$interfaces = isdn_get_interfaces();
	
	$te_interfaces = array();
	foreach ($interfaces as $interface) {
		if (strpos($interface['mode'], "TE") === false) {
			continue;
		}
		$te_interfaces[] = $interface;
	}
		
	return $te_interfaces;
}

/**
 * Returns all phones associated with a particular ISDN unit
 * If no phones are associated with the supplied unit, an empty array is returned
 */
function isdn_get_phones_on_unit($unit_number) {
	
	$phones = isdn_get_phones();
	
	$unit_phones = array();
	foreach ($phones as $p) {
		if ($p['interface'] != $unit_number) {
			continue;
		}
		$unit_phones[] = $p;
	}
		
	return $unit_phones;
}

/**
 * Returns all providers associated with a particular ISDN unit
 * If no providers are associated with the supplied unit, an empty array is returned
 */
function isdn_get_providers_on_unit($unit_number) {
	
	$providers = isdn_get_providers();

	$unit_providers = array();
	foreach ($providers as $p) {
		if ($p['interface'] != $unit_number) {
			continue;
		}
		$unit_providers[] = $p;
	}
		
	return $unit_providers;
}

/**
 * Forgets previously defined interface settings
 */
function isdn_forget_interface($unit_number) {
	global $config;

	$interfaces = $config['interfaces']['isdn-unit'];
	$n = count($interfaces);
	$unset = false;

	for ($i = 0; $i < $n; $i++) {
		if ($interfaces[$i]['unit'] == $unit_number) {
			unset($GLOBALS['config']['interfaces']['isdn-unit'][$i]);
			$unset = true;
			break;
		}
	}

	if (!$unset) {
		return "Interface unable to be found, settings remain.";
	}

	return 0;
}

/**
 * Generates capi.conf
 */
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
	
	// provider (te) interfaces
	$interfaces = isdn_get_te_interfaces();
	foreach($interfaces as $interface) {
		$conf .= "; {$interface['name']}\n";
		$conf .= "[{$interface['unit']}]\n";;
		
		$conf .= "isdnmode=msn\n";
		//$conf .= "msn={$p['msn']}\n";
		$conf .= "incomingmsn=*\n";
		$conf .= "controller={$interface['unit']}\n";
		$conf .= "group=1\n";
		//$conf .= ";prefix=0\n";
		$conf .= "softdtmf=off\n";
		$conf .= "relaxdtmf=off\n";
		//$conf .= "accountcode=\n";
		
		$conf .= "context=isdn-unit-{$interface['unit']}-incoming\n";
    
		$conf .= "holdtype=local\n";
		//$conf .= ";immediate=yes\n";
    
		if ($interface['echocancel']) {
			$conf .= "echocancel=yes\n";
		} else {
			$conf .= "echocancel=no\n";
		}
    
		//$conf .= ";echocancelold=yes\n";
		$conf .= "echotail=64\n";
		//$conf .= ";bridge=yes\n";
		//$conf .= ";callgroup=1\n";
		//$conf .= ";deflect=1234567\n";
		$conf .= "devices=2\n";
		$conf .= "\n";
	}
	
	$conf .= "\n";
	
	// phone (nt) interfaces
	$interfaces = isdn_get_nt_interfaces();
	foreach($interfaces as $interface) {
		$conf .= "; {$interface['name']}\n";
		$conf .= "[{$interface['unit']}]\n";
		
		$conf .= "ntmode=yes\n";
		$conf .= "isdnmode=did\n";
		$conf .= "incomingmsn=*\n";
		$conf .= "controller={$interface['unit']}\n";
		$conf .= "group=1\n";
		//$conf .= ";prefix=0\n";
		$conf .= "softdtmf=off\n";
		$conf .= "relaxdtmf=off\n";
		//$conf .= "accountcode=\n";
		
		$conf .= "context=isdn-unit-{$interface['unit']}-incoming\n";
    
		$conf .= "holdtype=local\n";
		//$conf .= ";immediate=yes\n";
		if ($interface['echocancel']) {
			$conf .= "echocancel=yes\n";
		} else {
			$conf .= "echocancel=no\n";
		}

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

/**
 * Executes isdnconfig to apply newly generated configuration changes
 */
function isdn_configure() {
	
	$res = 0;
	$interfaces = isdn_get_interfaces();
	foreach ($interfaces as $interface) {
		
		$command = "/usr/sbin/isdnconfig -u {$interface['unit']} -p {$interface['mode']}";
		
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

/**
 * Reloads chan_capi to apply newly generated configuration changes
 */
function isdn_reload() {
	return pbx_exec("module reload chan_capi.so");
}

?>
