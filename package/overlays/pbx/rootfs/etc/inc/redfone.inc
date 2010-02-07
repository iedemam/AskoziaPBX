<?php
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2010 IKT <http://itison-ikt.de>.
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

function redfone_verify_gateway($post, $errors) {

	$gw = array();

	v_cleanliness(&$post, &$errors);
	v_required('gwname', gettext('Gateway Name'), &$gw, &$post, &$errors);

	$trusted = array(
		'uniqid',
		'localif',
		'remoteif',
		'mac',
		'ip',
		'firmwareversion',
		'hardwareversion',
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
		$gw['span' . $i . 'slaveormaster'] = $post['span' . $i . 'slaveormaster'];
		$gw['span' . $i . 'needscrc4'] = ($post['span' . $i . 'needscrc4'] == "yes");
	}

	return $gw;
}

function redfone_fonulator_query() {
	$gw = array();
	$gw['localif'] = 'eth1';
	$gw['mac'] = '01234567eth1';
	$gw['ip'] = '192.168.111.eth1';
	$gw['spancount'] = '4';
	$gw['firmwareversion'] = '1.0';
	$gw['hardwareversion'] = '2.0';

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

function redfone_get_gateways() {
	$all_gws = $GLOBALS['config']['interfaces']['redfone-gateway'];
	return is_array($all_gws) ? $all_gws : array();
}

function redfone_get_gateway($uniqid) {
	global $uniqid_map;

	return $GLOBALS['config']['interfaces']['redfone-gateway'][$uniqid_map[$uniqid]];
}

function redfone_generate_default_gateway() {
	
}

function redfone_generate_confs($return=false, $tmpconfig=false) {
	global $g, $config;

	if ($tmpconfig) {
		$gateways[] = $tmpconfig;
	} else {
		$gateways = redfone_get_gateways();
	}

	foreach ($gateways as $gw) {
		$conf = "[globals]\n";

		// IP-address of the IP Configuration port
		// Factory defaults are; FB1=192.168.1.254 FB2=192.168.1.253
		$conf .= "fb=" . $gw['ip'] . "\n";

		// Which port to use for TDMoE Traffic (1 or 2)
		$conf .= "port=" . $gw['remoteif'] . "\n";

		// Which Asterisk server destination MAC address for TDMoE Traffic?
		$tmpif = network_get_interface($gw['localif']);
		$conf .= "server=" . $tmpif['mac'] . "\n";

		// Determines which span is to derive timing from telco
		// priorities=0,1,2,3

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
				// rbs
				// loopback
				$conf .= "\n";
			}
		}

		util_file_write("/etc/redfone/" . $gw['uniqid'] . ".conf", $conf);
	}
}

?>
