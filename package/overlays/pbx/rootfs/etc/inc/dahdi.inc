<?php
/*
	$Id$
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

/**
 * Returns dahdi_scan output split into an array of span information
 */
function dahdi_scan_to_spans() {
	$spans = array();
	// get output from dahdi_scan
	exec("dahdi_scan", $out);
	// start our count at -1 to avoid initial condition logic
	$c = -1;
	// for each line of output...
	foreach ($out as $l) {
		// this is a new span definition, bump count
		if ($l[0] == '[') {
			$c++;
		// ...otherwise store span's info
		} else {
			// explode lines in span into value pairs
			$p = explode("=", $l, 2);
			// there can be multiple port lines, store pair in array
			if ($p[0] == "port") {
				$spans[$c][$p[0]][] = $p[1];
			// options lists should be split out into arrays
			} else if ($p[0] == "coding_opts" || $p[0] == "framing_opts") {
				$spans[$c][$p[0]] = explode(",", $p[1]);
			// ...otherwise simply store pair
			} else {
				$spans[$c][$p[0]] = $p[1];
			}
		}
	}
	return $spans;
}

function dahdi_get_physical_ports() {
	$ports = array();

	$spans = dahdi_scan_to_spans();
	$i = 0;
	foreach ($spans as $span) {

		// analog span, multiple ports per span
		if ($span['type'] == "analog") {
			foreach ($span['port'] as $port) {
				// split up "basechannel,porttype"
				$port = explode(',', $port);

				// common info from card
				$ports[$i]['location'] = $span['location'];
				$ports[$i]['card'] = $span['devicetype'];
				$ports[$i]['technology'] = "analog";

				// per port specifics
				$ports[$i]['basechannel'] = $port[0];
				$ports[$i]['type'] = strtolower(substr($port[1], 0, 3));

				$i++;
			}

		// digital span, single port per span
		} else if (strpos($span['type'], "digital-") !== false) {
			// common info from card
			$ports[$i]['location'] = $span['location'];
			$ports[$i]['card'] = $span['devicetype'];
			$ports[$i]['technology'] = "isdn";

			// per port specifics
			$ports[$i]['span'] = strtolower(substr($span['name'], strrpos($span['name'], '/') + 1));
			$ports[$i]['basechannel'] = $span['basechan'];
			$ports[$i]['totalchannels'] = $span['totchans'];
			$ports[$i]['type'] = strtolower(substr($span['type'], strrpos($span['type'], '-') + 1));

			$i++;
		}
	}

	return $ports;
}

function dahdi_autoconfigure_ports() {
	global $defaults;

	$ports = dahdi_get_physical_ports();
	foreach ($ports as $p) {
		$uniqid = "DAHDIPORT-" . strtoupper($p['technology']) . "-" . 
			md5($p['location'] . $p['basechannel']);

		if (!dahdi_get_port($uniqid)) {
			$p['uniqid'] = $uniqid;

			$p['name'] = gettext("Port") . " " . $p['basechannel'];

			$p['echo-taps'] = $defaults['dahdi']['echo-taps'];

			$p['rxgain'] = $defaults['dahdi']['rxgain'];
			$p['txgain'] = $defaults['dahdi']['txgain'];

			if ($p['technology'] == "analog") {
				$p['startsignaling'] = $defaults['dahdi']['analog']['startsignaling'];

			} else if ($p['technology'] == "isdn") {
				if ($p['totalchannels'] == 3 && $p['type'] == "nt") {
					$p['signaling'] = "bri_net_ptmp";
				}
			}

			$p['unused'] = true;

			dahdi_save_port($p);
		}
	}
}

function dahdi_get_ports($techfilter=null, $typefilter=null, $chancountfilter=null) {
	global $config;

	$all_ports = $config['interfaces']['dahdi-port'];
	$all_ports = is_array($all_ports) ? $all_ports : array();

	if (!$techfilter && !$typefilter && !$chancountfilter) {
		return $all_ports;
	}

	$filtered_ports = array();
	foreach ($all_ports as $port) {
		if ($techfilter && $port['technology'] != $techfilter) {
			continue;
		}
		if ($typefilter && $port['type'] != $typefilter) {
			continue;
		}
		if ($chancountfilter && $port['totalchannels'] != $chancountfilter) {
			continue;
		}

		$filtered_ports[] = $port;
	}

	return $filtered_ports;
}

function dahdi_get_port($uniqid) {
	global $config, $uniqid_map;

	return $config['interfaces']['dahdi-port'][$uniqid_map[$uniqid]];
}

function dahdi_save_port($port) {
	global $g, $config, $uniqid_map;

	print_r($uniqid_map);
	if (dahdi_get_port($port['uniqid'])) {
		$config['interfaces']['dahdi-port'][$uniqid_map[$port['uniqid']]] = $port;
	} else {
		$config['interfaces']['dahdi-port'][] = $port;
	}

	write_config();
	touch($g['dahdi_dirty_path']);
}

function dahdi_forget_port($uniqid) {
	global $g, $uniqid_map;

	unset($GLOBALS['config']['interfaces']['dahdi-port'][$uniqid_map[$uniqid]]);

	write_config();
	touch($g['dahdi_dirty_path']);
}

function dahdi_generate_system_conf($return=false, $ports=false) {
	global $g, $config, $defaults;

	if (!$ports) {
		$ports = dahdi_get_ports();
	}

	$conf = "# Hardware Info\n";
	foreach ($ports as $port) {
		if ($port['technology'] == "analog") {
			$name = ($port['name']) ? $port['name'] : "default";
			$conf .= "# analog " . $port['type'] . " channel " . $port['basechannel'] . " (" . $name . ") is on card " . $port['card'] . "\n";

		} else if ($port['technology'] == "isdn") {
			$name = ($port['name']) ? $port['name'] : "default";
			$conf .= "# isdn " . $port['type'] . " port, basechannel " . $port['basechannel'] . " (" . $name . ") is on card " . $port['card'] . " w/" . $port['totalchannels'] .  " channels\n";
		}
	}
	$conf .= "\n";


	$conf .= "# Spans\n";
	// span=<span num>,<timing source>,<line build out (LBO)>,<framing>,<coding>[,yellow]
	$timingprio = 1;
	foreach ($ports as $port) {
		if ($port['technology'] == "isdn" && $port['totalchannels'] == 3) {
			// bri 'phone' ports:
			//	- lbo = 0, not used in bri
			//	- framing and coding always ccs and ami respectively
			if ($port['type'] == "nt") {
				$conf .= "span=" . $port['span'] . ",0,0,ccs,ami\n";
			} else {
				$conf .= "span=" . $port['span'] . "," . $timingprio++ . ",0,ccs,ami\n";
			}
		}
	}
	$conf .= "\n";


	$conf .= "# Channels\n";
	// <device>=<channel list>
	foreach ($ports as $port) {
		// analog port config
		if ($port['technology'] == "analog") {
			// signaling override or default?
			$startsignaling = ($port['startsignaling']) ?
				$port['startsignaling'] : $defaults['dahdi']['analog']['startsignaling'];
			// use fxo signaling for fxs ports and fxs signaling for fxo
			$signaling = ($port['type'] == "fxs") ? "fxo" : "fxs";
			$conf .= $signaling . $startsignaling . "=" . $port['basechannel'] . "\n";

		} else if ($port['technology'] == "isdn" && $port['totalchannels'] == 3) {
			// bri 'phone' ports
			if ($port['type'] == "nt") {
				$conf .= "bchan=" . $port['basechannel'] . "\n";
				$conf .= "bchan=" . ($port['basechannel'] + 1) . "\n";
				$conf .= "hardhdlc=" . ($port['basechannel'] + 2) . "\n";
			}
		}
	}
	$conf .= "\n";


	$conf .= "# Tone Zones\n";
	// loadzone=<zone>
	// defaultzone=<zone>

	//$analogconfig = $config['services']['analog'];
	//
	//if (!is_array($analogconfig['loadzone'])) {
	//	$loadzones[] = "us";
	//} else {
	//	$loadzones = $analogconfig['loadzone'];
	//}
	//
	//$n = count($loadzones);
	//for ($i = 0; $i < $n; $i++) {
	//	$conf .= "loadzone={$loadzones[$i]}\n";
	//}
	//$conf .= "defaultzone={$loadzones[0]}\n";

	$conf .= "\n";


	$conf .= "# PCM Encoding\n";
	// mulaw=1-4
	// alaw=1-4
	$conf .= "\n";


	$conf .= "# Echo Cancellers\n";
	// echocanceller=<echocanceller name>,<channel(s)>
	foreach ($ports as $port) {
		// analog port config
		if ($port['technology'] == "analog") {
			if ($port['echo-taps'] != "disable") {
				$conf .= "echocanceller=mg2," . $port['basechannel'] . "\n";
			}

		} else if ($port['technology'] == "isdn" && $port['totalchannels'] == 3) {
			// bri 'phone' ports
			if ($port['type'] == "nt") {
				if ($port['echo-taps'] != "disable") {
					$conf .= "echocanceller=mg2," . $port['basechannel'] . "-" . ($port['basechannel'] + 1) . "\n";
				}
			}
		}
	}
	$conf .= "\n";

	return ($return) ? $conf : util_file_write("/etc/dahdi/system.conf", $conf);
}

function chan_dahdi_conf_generate($return=false, $ports=false) {
	global $g, $config, $defaults;

	if (!$ports) {
		$ports = dahdi_get_ports();
	}

	$conf = "; Hardware Info\n";
	foreach ($ports as $port) {
		if ($port['technology'] == "analog") {
			$name = ($port['name']) ? $port['name'] : "default";
			$conf .= "; analog " . $port['type'] . " channel " . $port['basechannel'] . " (" . $name . ") is on card " . $port['card'] . "\n";

		} else if ($port['technology'] == "isdn") {
			$name = ($port['name']) ? $port['name'] : "default";
			$conf .= "; isdn " . $port['type'] . " port, basechannel " . $port['basechannel'] . " (" . $name . ") is on card " . $port['card'] . " w/" . $port['totalchannels'] .  " channels\n";
		}
	}
	$conf .= "\n";

	$conf .= "[trunkgroups]\n\n";

	$conf .= "[channels]\n";
	$analogproviders = analog_get_providers();
	foreach ($analogproviders as $provider) {
		if (isset($provider['disabled'])) {
			continue;
		}
		$port = dahdi_get_port($provider['port']);

		$conf .= "; Provider: " . $provider['name'] . "\n";
		$conf .= "signalling=fxs_" . $port['startsignaling'] . "\n";
		$conf .= "rxgain=" . $port['rxgain'] . "\n";
		$conf .= "txgain=" . $port['txgain'] . "\n";

		if ($port['echo-taps'] == "disable") {
			$conf .= "echocancel=no\n";
		} else {
			$conf .= "echocancel=" . $port['echo-taps'] . "\n";
		}

		$conf .= "busydetect=yes\n";
		$conf .= "busycount=3\n";
		$conf .= "language=" . $provider['language'] . "\n";
		$conf .= "context=" . $provider['uniqid'] . "-incoming\n";
		//$conf .= util_gen_manual_attributes($interface['manual-attribute']);
		$conf .= "channel => " . $port['basechannel'] . "\n";
		$conf .= "\n";
	}
	
	$analogphones = analog_get_phones();
	foreach ($analogphones as $phone) {
		if (isset($phone['disabled'])) {
			continue;
		}
		$port = dahdi_get_port($phone['port']);

		$conf .= "; Phone: " . $phone['extension'] . "\n";
		$conf .= "callerid=" . $phone['callerid'] . " <" . $phone['extension'] . ">\n";
		$conf .= "signalling=fxo_" . $port['startsignaling'] . "\n";
		$conf .= "rxgain=" . $port['rxgain'] . "\n";
		$conf .= "txgain=" . $port['txgain'] . "\n";

		if ($port['echo-taps'] == "disable") {
			$conf .= "echocancel=no\n";
		} else {
			$conf .= "echocancel=" . $port['echo-taps'] . "\n";
		}

		$conf .= "busydetect=yes\n";
		$conf .= "busycount=3\n";
		$conf .= "language=" . $phone['language'] . "\n";
		$conf .= "context=" . $phone['uniqid'] . "\n";
		//$conf .= util_gen_manual_attributes($interface['manual-attribute']);
		$conf .= "channel => " . $port['basechannel'] . "\n";
		$conf .= "\n";
	}

	$isdnphones = isdn_get_phones();
	foreach ($isdnphones as $phone) {
		if (isset($phone['disabled'])) {
			continue;
		}
		$port = dahdi_get_port($phone['port']);

		$conf .= "; Phone: " . $phone['extension'] . "\n";
		$conf .= "callerid=" . $phone['callerid'] . " <" . $phone['extension'] . ">\n";
		$conf .= "signalling=bri_net_ptmp\n";
		//$conf .= "overlapdial=yes\n";
		$conf .= "rxgain=" . $port['rxgain'] . "\n";
		$conf .= "txgain=" . $port['txgain'] . "\n";

		if ($port['echo-taps'] == "disable") {
			$conf .= "echocancel=no\n";
		} else {
			$conf .= "echocancel=" . $port['echo-taps'] . "\n";
		}

		$conf .= "language=" . $phone['language'] . "\n";
		$conf .= "context=" . $phone['uniqid'] . "\n";
		//$conf .= util_gen_manual_attributes($interface['manual-attribute']);
		$conf .= "channel => " . $port['basechannel'] . "-" . ($port['basechannel'] + 1) . "\n";
		$conf .= "\n";
	}

	$conf .= "\n";

	return ($return) ? $conf : util_file_write("/etc/asterisk/chan_dahdi.conf", $conf);
}

function dahdi_configure() {
	global $g;

	$ports = dahdi_get_ports();

	dahdi_generate_system_conf(false, $ports);
	chan_dahdi_conf_generate(false, $ports);

	mwexec("/usr/sbin/dahdi_cfg");

	unlink_if_exists($g['dahdi_dirty_path']);
}

?>
