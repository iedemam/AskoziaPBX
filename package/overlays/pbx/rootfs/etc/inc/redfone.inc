<?php
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2010 tecema (a.k.a IKT) <http://www.tecema.de>.
	All rights reserved.
	
	Askozia®PBX is a registered trademark of tecema. Any unauthorized use of
	this trademark is prohibited by law and international treaties.
	
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
	
	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.
	
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	
	3. Redistribution in any form at a charge, that in whole or in part
	   contains or is derived from the software, including but not limited to
	   value added products, is prohibited without prior written consent of
	   tecema.
	
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

function redfone_initial_connect($settings) {

	$tmpif = network_get_interface($settings['localif']);

	$conf = "[globals]\n";
	$conf .= "fb=" . $settings['remoteip'] . "\n";
	$conf .= "port=" . $settings['remoteif'] . "\n";
	$conf .= "server=" . $tmpif['mac'] . "\n";
	$conf .= "\n";
	util_file_write("/etc/redfone/tmp.conf", $conf);

	if ($settings['localif'] != $GLOBALS['config']['interfaces']['lan']['if']) {
		$ip = explode(".", $settings['remoteip']);
		for ($i = 100; $i < 150; $i++) {
			if ($ip[3] != $i) {
				$ip[3] = $i;
				break;
			}
		}
		mwexec("ifconfig " . $settings['localif'] . " " . implode(".", $ip));
	}

	$query = redfone_fonulator_query("/etc/redfone/tmp.conf");
	unlink("/etc/redfone/tmp.conf");

	return $query;
}

function redfone_fonulator_query($conffile) {
	exec("fonulator -qv " . $conffile, $output, $retval);
	$query = array();
	$spancount = 0;
	foreach ($output as $out) {
		$out =  explode(": ", $out, 2);
		$tmpindex = strtolower(str_replace(array(" ", "[", "]"), "", $out[0]));
		if (strpos($tmpindex, "span") !== false) {
			$spancount++;
			$spannumber = substr($tmpindex, 4, 1);
			$spansettings = explode(",", $out[1]);
			$query['span' . $spannumber . 'type'] = $spansettings[0];
			$query['span' . $spannumber . 'framing'] = strtolower($spansettings[1]);
			$query['span' . $spannumber . 'encoding'] = strtolower($spansettings[2]);
		} else {
			$query[$tmpindex] = $out[1];
		}
	}
	$query['spancount'] = $spancount;
	$query['firmwareversion'] = $query['swver'] . " (build " . $query['swbuild'] . "), " . $query['swcompiledate'];
	$query['remotemac'] = $query['macaddress'];
	$query['retval'] = $retval;

	return $query;
}

function redfone_get_gateways() {
	$all_gws = $GLOBALS['config']['interfaces']['redfone-gateway'];
	return is_array($all_gws) ? $all_gws : array();
}

function redfone_get_gateway($uniqid) {
	global $uniqid_map;

	return $GLOBALS['config']['interfaces']['redfone-gateway'][$uniqid_map[$uniqid]];
}

function redfone_verify_gateway($post, $errors) {

	$gw = array();

	v_cleanliness(&$post, &$errors);
	v_required('gwname', gettext('Gateway Name'), &$gw, &$post, &$errors);

	$trusted = array(
		'uniqid',
		'localif',
		'remoteif',
		'remoteip',
		'remotemac',
		'firmwareversion',
		'spancount'
	);
	foreach ($trusted as $t) {
		$gw[$t] = $post[$t];
	}

	for ($i = 1; $i < $post['spancount'] + 1; $i++) {
		v_required('span' . $i . 'name', gettext('Span') . " #" . $i . ": " . gettext('Name'), &$gw, &$post, &$errors);
		$gw['span' . $i . 'type'] = $post['span' . $i . 'type'];
		$gw['span' . $i . 'framing'] = $post['span' . $i . 'framing'];
		$gw['span' . $i . 'encoding'] = $post['span' . $i . 'encoding'];
		$gw['span' . $i . 'echo-taps'] = $post['span' . $i . 'echo-taps'];
		$gw['span' . $i . 'needscrc4'] = ($post['span' . $i . 'needscrc4'] == "yes");
		$gw['span' . $i . 'needsloopback'] = ($post['span' . $i . 'needsloopback'] == "yes");
	}

	return $gw;
}

function redfone_save_gateway($gw) {
	global $g, $uniqid_map;

	if (redfone_get_gateway($gw['uniqid'])) {
		$GLOBALS['config']['interfaces']['redfone-gateway'][$uniqid_map[$gw['uniqid']]] = $gw;
	} else {
		$GLOBALS['config']['interfaces']['redfone-gateway'][] = $gw;
	}

	write_config();
	touch($g['redfone_dirty_path']);
}

function redfone_forget_gateway($uniqid) {
	global $g, $uniqid_map;

	unset($GLOBALS['config']['interfaces']['redfone-gateway'][$uniqid_map[$uniqid]]);

	write_config();
	touch($g['redfone_dirty_path']);
}

function redfone_generate_default_gateway() {
	
}

function redfone_get_span_basechannel($uniqid, $spannumber) {
	$gateway = redfone_get_gateway($uniqid);
	$basechannel = 1;
	for ($i = 1; $i < $spannumber; $i++) {
		if ($gateway['span' . $i . 'type'] == "E1") {
			$basechannel += 31;
		} else {
			$basechannel += 24;
		}
	}

	return $basechannel;
}

function redfone_configure() {
	global $g;

	$gateways = redfone_get_gateways();

	foreach ($gateways as $gw) {
		// set the network interface talking on the correct subnet...
		if ($gw['localif'] != $GLOBALS['config']['interfaces']['lan']['if']) {
			$ip = explode(".", $gw['remoteip']);
			for ($i = 100; $i < 150; $i++) {
				if ($ip[3] != $i) {
					$ip[3] = $i;
					break;
				}
			}
			mwexec("ifconfig " . $gw['localif'] . " " . implode(".", $ip));
		}

		// generate configuration file...
		$conf = "[globals]\n";

		// IP-address of the IP Configuration port
		// Factory defaults are; FB1=192.168.1.254 FB2=192.168.1.253
		$conf .= "fb=" . $gw['remoteip'] . "\n";

		// Which port to use for TDMoE Traffic (1 or 2)
		$conf .= "port=" . $gw['remoteif'] . "\n";

		// Which Asterisk server destination MAC address for TDMoE Traffic?
		$tmpif = network_get_interface($gw['localif']);
		$conf .= "server=" . $tmpif['mac'] . "\n";

		// Determines which span is to derive timing from telco
		$conf .= "priorities=0,1,2,3\n";

		$conf .= "\n";

		if (!$tmpconfig) {
			for ($i = 1; $i < $gw['spancount'] + 1; $i++) {
				$conf .= "# " . $gw['span' . $i . 'name'] . "\n";
				$conf .= "[span" . $i . "]\n";
				$conf .= "framing=" . $gw['span' . $i . 'framing'] . "\n";
				$conf .= "encoding=" . $gw['span' . $i . 'encoding'] . "\n";
				if (isset($gw['span' . $i . 'needscrc4'])) {
					$conf .= "crc4\n";
				}
				if (isset($gw['span' . $i . 'needsloopback'])) {
					$conf .= "loopback\n";
				}
				$conf .= "\n";
			}
		}

		$path = "/etc/redfone/" . $gw['uniqid'] . ".conf";
		util_file_write($path, $conf);

		// write configuration to gateway...
		mwexec("fonulator --write-config " . $path);
	}
}

?>
