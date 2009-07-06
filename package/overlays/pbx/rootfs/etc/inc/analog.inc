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
function analog_get_providers() {
	global $config;
	
	if (!is_array($config['analog']['provider']))
		$config['analog']['provider'] = array();
		
	analog_sort_providers();
		
	return $config['analog']['provider'];
}

/**
 * Sorts providers by name
 */
function analog_sort_providers() {
	global $g, $config;

	usort($config['analog']['provider'], "pbx_sort_by_name");	
}

/**
 * Returns a specific provider based on its uniqid
 */
function analog_get_provider($uniqid) {
	global $config, $uniqid_map;

	return $config['analog']['provider'][$uniqid_map[$uniqid]];
}

/**
 * Returns all phones sorted by extension.
 * If no phones are defined, the global array is initialized.
 */
function analog_get_phones() {
	global $config;
	
	if (!is_array($config['analog']['phone']))
		$config['analog']['phone'] = array();
		
	analog_sort_phones();
		
	return $config['analog']['phone'];
}

/**
 * Sorts phones by extension
 */
function analog_sort_phones() {
	global $g, $config;

	usort($config['analog']['phone'], "pbx_sort_by_extension");	
}

/**
 * Returns a specific phone based on its uniqid
 */
function analog_get_phone($uniqid) {
	global $config, $uniqid_map;

	return $config['analog']['phone'][$uniqid_map[$uniqid]];
}

/**
 * Returns an array of all assigned phone extensions
 */
function analog_get_extensions() {
	global $config;
	
	$extensions = array();

	if (is_array($config['analog']['phone'])) {
		foreach($config['analog']['phone'] as $phone) {
			$extensions[] = $phone['extension'];
		}
	}

	return $extensions;
}

/**
 * Returns an array of all assigned provider dialpatterns
 */
function analog_get_dialpatterns() {
	global $config;
	
	$dialpatterns = array();

	if (is_array($config['analog']['provider'])) {
		foreach($config['analog']['provider'] as $provider) {
			$dialpatterns[$provider['uniqid']] = $provider['dialpattern'];
		}
	}

	return $dialpatterns;
}

/**
 * Searches dmesg for recognized a/b modules.
 * Returns an array indexed by module number containing the module type as value.
 * XXX : multiple cards will still cause problems here
 */
function analog_get_recognized_ab_unit_numbers() {

	$dmesg_lines = system_get_dmesg_as_array();

	$controllers = array();
	foreach ($dmesg_lines as $line) {
		// fxo ports
		if (preg_match("/\b[0-9]+: Installed -- AUTO FXO/", $line, &$matches)) {
			$num = substr($matches[0], 0, strpos($matches[0],":"));
			$controllers[$num+1] = "fxo";

		} else if (strpos($line, "Found a Wildcard FXO: ") !== false) {
			$controllers[1] = "fxo";


		// fxs ports
		} else if (preg_match("/\b[0-9]+: Installed -- AUTO FXS/", $line, &$matches)) {
			$num = substr($matches[0], 0, strpos($matches[0],":"));
			$controllers[$num+1] = "fxs";
		}
	}
	
	return $controllers;
}

/**
 * Returns all configured a/b units sorted by unit number.
 * If no units are configured, the global array is initialized.
 * XXX : should be switched to return both recognized and configured units
 */
function analog_get_ab_interfaces($type=null) {
	global $config;
	
	if (!is_array($config['interfaces']['ab-unit']))
		$config['interfaces']['ab-unit'] = array();
		
	analog_sort_ab_interfaces();
		
	if ($type) {
		$filtered = array();
		foreach ($config['interfaces']['ab-unit'] as $ab_unit) {
			if ($ab_unit['type'] == $type) {
				$filtered[] = $ab_unit;
			}
		}
		return $filtered;
	} else {
		return $config['interfaces']['ab-unit'];
	}
}

/**
 * Sorts a/b units by unit number
 */
function analog_sort_ab_interfaces() {
	global $config;

	usort($config['interfaces']['ab-unit'], "pbx_sort_by_unit");	
}

/**
 * Returns an a/b unit based on its unit number
 */
function analog_get_ab_interface($unit) {
	
	$interfaces = analog_get_ab_interfaces();

	foreach ($interfaces as $interface) {
		if ($interface['unit'] == $unit) {
			return $interface;
		}
	}
}

/**
 * Forgets previously defined interface settings
 */
function analog_forget_interface($unit_number) {
	global $config;

	$interfaces = $config['interfaces']['ab-unit'];
	$n = count($interfaces);
	$unset = false;

	for ($i = 0; $i < $n; $i++) {
		if ($interfaces[$i]['unit'] == $unit_number) {
			unset($GLOBALS['config']['interfaces']['ab-unit'][$i]);
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
 * Generates all configuration files necessary when changes are made.
 */
function analog_conf_generate() {

	$res = 0;
	$res |= analog_system_conf_generate();
	$res |= analog_chan_dahdi_conf_generate();

	return $res;
}

/**
 * Generates dahdi's system.conf which defines modules, signaling and loadzones.
 */
function analog_system_conf_generate() {
	global $config, $g;

	$fd = fopen("/etc/dahdi/system.conf", "w");
	if (!$fd) {
		printf("Error: cannot open system.conf in analog_system_conf_generate().\n");
		return 1;
	}

	$abinterfaces = analog_get_ab_interfaces();
	foreach($abinterfaces as $abi) {
		// fxo
		if ($abi['type'] == "fxo") {
			if (!$abi['startsignal'] || $abi['startsignal'] == "ks") {
				if (!$fxsks_line) {
					$fxsks_line = "fxsks={$abi['unit']}";
				} else {
					$fxsks_line .= ",{$abi['unit']}";
				}
			} else if ($abi['startsignal'] == "gs") {
				if (!$fxsgs_line) {
					$fxsgs_line = "fxsgs={$abi['unit']}";
				} else {
					$fxsgs_line .= ",{$abi['unit']}";
				}
			} else if ($abi['startsignal'] == "ls") {
				if (!$fxsls_line) {
					$fxsls_line = "fxsls={$abi['unit']}";
				} else {
					$fxsls_line .= ",{$abi['unit']}";
				}
			}
		// fxs
		} else if ($abi['type'] == "fxs") {
			if (!$abi['startsignal'] || $abi['startsignal'] == "ks") {
				if (!$fxoks_line) {
					$fxoks_line = "fxoks={$abi['unit']}";
				} else {
					$fxoks_line .= ",{$abi['unit']}";
				}
			} else if ($abi['startsignal'] == "gs") {
				if (!$fxogs_line) {
					$fxogs_line = "fxogs={$abi['unit']}";
				} else {
					$fxogs_line .= ",{$abi['unit']}";
				}
			} else if ($abi['startsignal'] == "ls") {
				if (!$fxols_line) {
					$fxols_line = "fxols={$abi['unit']}";
				} else {
					$fxols_line .= ",{$abi['unit']}";
				}
			}
		}
	}
	
	if ($fxsks_line) {
		$conf .= "$fxsks_line\n";
	}
	if ($fxsgs_line) {
		$conf .= "$fxsgs_line\n";
	}
	if ($fxsls_line) {
		$conf .= "$fxsls_line\n";
	}
	
	if ($fxoks_line) {
		$conf .= "$fxoks_line\n";
	}
	if ($fxogs_line) {
		$conf .= "$fxogs_line\n";
	}
	if ($fxols_line) {
		$conf .= "$fxols_line\n";
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

/**
 * Generates chan_dahdi.conf which defines modules, signaling, ec options, busy detection etc.
 */
function analog_chan_dahdi_conf_generate() {
	global $config, $g, $defaults;

	$fd = fopen("/etc/asterisk/chan_dahdi.conf", "w");
	if (!$fd) {
		printf("Error: cannot open chan_dahdi.conf in analog_chan_dahdi_conf_generate().\n");
		return 1;
	}
	
	$conf .= "[trunkgroups]\n\n";
	
	$conf .= "[channels]\n";
	
	$analogproviders = analog_get_providers();
	foreach ($analogproviders as $p) {
		if (isset($p['disabled'])) {
			continue;
		}
		$interface = analog_get_ab_interface($p['interface']);

		if ($interface['startsignal']) {
			$conf .= "signalling=fxs_{$interface['startsignal']}\n";
		} else {
			$conf .= "signalling=fxs_ks\n";
		}

		if ($interface['rxgain']) {
			$conf .= "rxgain=" . $interface['rxgain'] . "\n";
		} else {
			$conf .= "rxgain=" . $defaults['analog']['interface']['rxgain'] . "\n";
		}
		if ($interface['txgain']) {
			$conf .= "txgain=" . $interface['txgain'] . "\n";
		} else {
			$conf .= "txgain=" . $defaults['analog']['interface']['txgain'] . "\n";
		}

		if ($interface['echocancel']) {
			$conf .= "echocancel={$interface['echocancel']}\n";
		} else {
			$conf .= "echocancel=128\n";
		}
		$conf .= "echocancelwhenbridged=no\n";
		
		$conf .= "busydetect=yes\n";
		$conf .= "busycount=3\n";
		
		if (!$p['language']) {
			$conf .= "language=en\n";
		} else {
			$conf .= "language={$p['language']}\n";
		}
		$conf .= "context={$p['uniqid']}-incoming\n";
		$conf .= util_gen_manual_attributes($interface['manual-attribute']);
		$conf .= "channel => {$p['interface']}\n";
		$conf .= "\n";
	}
	
	$analogphones = analog_get_phones();
	foreach ($analogphones as $p) {
		$interface = analog_get_ab_interface($p['interface']);
		
		$conf .= "callerid={$p['callerid']} <{$p['extension']}>\n";

		if ($interface['startsignal']) {
			$conf .= "signalling=fxo_{$interface['startsignal']}\n";
		} else {
			$conf .= "signalling=fxo_ks\n";
		}

		if ($interface['rxgain']) {
			$conf .= "rxgain=" . $interface['rxgain'] . "\n";
		} else {
			$conf .= "rxgain=" . $defaults['analog']['interface']['rxgain'] . "\n";
		}
		if ($interface['txgain']) {
			$conf .= "txgain=" . $interface['txgain'] . "\n";
		} else {
			$conf .= "txgain=" . $defaults['analog']['interface']['txgain'] . "\n";
		}

		if ($interface['echocancel']) {
			$conf .= "echocancel={$interface['echocancel']}\n";
		} else {
			$conf .= "echocancel=128\n";
		}
		$conf .= "echocancelwhenbridged=no\n";
		
		$conf .= "busydetect=yes\n";
		$conf .= "busycount=3\n";
		
		if (!$p['language']) {
			$conf .= "language=en\n";
		} else {
			$conf .= "language={$p['language']}\n";
		}
		$conf .= "context={$p['uniqid']}\n";
		$conf .= util_gen_manual_attributes($interface['manual-attribute']);
		$conf .= "channel => {$p['interface']}\n";
		$conf .= "\n";
	}

	$conf .= "\n";

	fwrite($fd, $conf);
	fclose($fd);

	return 0;
}

/**
 * Executes dahdi_cfg to apply newly generated configuration changes, returns if no a/b units are present.
 */
function analog_configure() {

	if(!count(analog_get_recognized_ab_unit_numbers())) {
		return 0;
	}
	$res = 0;
	$res |= mwexec("/usr/sbin/dahdi_cfg");

	return $res;
}

/**
 * Reloads chan_dahdi to apply newly generated configuration changes
 */
function analog_reload() {

	$res = 0;
	$res |= pbx_exec("module reload chan_dahdi.so");
	$res |= pbx_exec("dahdi restart");

	return $res;
}

?>
