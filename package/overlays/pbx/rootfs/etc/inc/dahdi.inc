<?php
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2009-2010 IKT <http://itison-ikt.de>.
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
		// this is a new span definition, bump count and store span number
		if ($l[0] == '[') {
			$c++;
			$spans[$c]['span'] = ($c + 1);
		// ...otherwise store span's info
		} else {
			// explode lines in span into value pairs
			$p = explode("=", $l, 2);
			// there can be multiple port lines, store pair in array
			if ($p[0] == "port") {
				// further break apart port line
				$arr_port_specs = explode(",", $p[1]);
				// ignore if no module is present
				if ($arr_port_specs[1] == "none") {
					continue;
				}
				// ignore if the module does not have power
				if ($arr_port_specs[1] == "FXS FAILED") {
					continue;
				}
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
			$ports[$i]['span'] = $span['span'];
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

			$p['echo-taps'] = $defaults['dahdi']['echo-taps'];

			$p['rxgain'] = $defaults['dahdi']['rxgain'];
			$p['txgain'] = $defaults['dahdi']['txgain'];

			// setup analog specific defaults
			if ($p['technology'] == "analog") {
				$p['name'] = gettext("Port") . " " . $p['basechannel'];
				$p['startsignaling'] = $defaults['dahdi']['analog']['startsignaling'];

			// setup bri isdn specific defaults
			} else if ($p['technology'] == "isdn" && $p['totalchannels'] == 3) {
				$p['name'] = gettext("Port") . " " . $p['span'];

				if ($p['type'] == "nt") {
					// once PTMP is implemented...
					// $p['signaling'] = "bri_net_ptmp";
					$p['signaling'] = "bri_net";
				} else if ($p['type'] == "te") {
					$p['signaling'] = "bri_cpe_ptmp";
				}
			}

			$p['unused'] = true;

			dahdi_save_port($p);
		}
	}

	/* if there are multiple provider ports of the same type, auto-generate groups */
	$provider_types = array(
		"analog" => "fxo",
		"isdn" => "te"
	);
	foreach ($provider_types as $tech => $type) {
		$magic_uniqid = "DAHDIPORTGROUP-" . strtoupper($tech) . "-ALLPROVIDERS";
		if (!dahdi_get_portgroup($magicuniqid)) {
			$provider_ports = dahdi_get_ports($tech, $type);
			if (count($provider_ports) > 1) {
				$group = dahdi_generate_default_portgroup($tech);
				$group['uniqid'] = $magic_uniqid;
				$group['name'] = gettext("All Provider Ports");
				foreach ($provider_ports as $port) {
					$group['groupmember'][] = $port['uniqid'];
				}
				dahdi_save_portgroup($group);
			}
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

function dahdi_get_free_portgroup_number() {
	$available_groups = array();
	$ports = dahdi_get_ports();
	$groups = dahdi_get_portgroups();

	// group numbers range from 0 to 63 in chan_dahdi.conf
	//
	// mark port basechannel numbers as unavailable since these
	// are used for the automatically generated groups on ports
	foreach ($ports as $port) {
		$used_groups[$port['basechannel']] = true;
	}
	// mark existing groups numbers as unavailable
	foreach ($groups as $group) {
		$used_groups[$group['number']] = true;
	}
	// return the first available group number starting from the top
	for ($i = 63; $i >= 0; $i--) {
		if (!$used_groups[$i]) {
			return $i;
		}
	}

	return false;
}

function dahdi_generate_default_portgroup($technology) {

	$number = dahdi_get_free_portgroup_number();
	if ($number === false) {
		return false;
	}

	$group = array();

	$group['name'] = gettext("Group") . " " . $number;
	$group['number'] = $number;
	$group['technology'] = $technology;
	if ($technology == "analog") {
		$group['type'] = "fxo";
	} else if ($technology == "isdn") {
		$group['type'] = "te";
	}
	$group['uniqid'] = "DAHDIPORTGROUP-" . strtoupper($technology) . "-" . uniqid(rand());

	return $group;
}

function dahdi_get_portgroups($techfilter=null) {
	global $config;

	$all_groups = $config['interfaces']['dahdi-portgroup'];
	$all_groups = is_array($all_groups) ? $all_groups : array();

	if (!$techfilter) {
		return $all_groups;
	}

	$filtered_groups = array();
	foreach ($all_groups as $group) {
		if ($techfilter && $group['technology'] != $techfilter) {
			continue;
		}

		$filtered_groups[] = $group;
	}

	return $filtered_groups;
}

function dahdi_get_portgroup($uniqid) {
	global $uniqid_map;

	return $GLOBALS['config']['interfaces']['dahdi-portgroup'][$uniqid_map[$uniqid]];
}

function dahdi_verify_portgroup($post, $errors) {

	$group = array();

	v_cleanliness(&$post, &$errors);
	v_required('name', gettext('Name'), &$group, &$post, &$errors);
	v_draganddrop(&$group, &$post);

	$trusted = array(
		'uniqid',
		'technology',
		'type',
		'number'
	);
	foreach ($trusted as $t) {
		$group[$t] = $post[$t];
	}

	return $group;
}

function dahdi_save_portgroup($group) {
	global $g, $uniqid_map;

	if (dahdi_get_portgroup($group['uniqid'])) {
		$GLOBALS['config']['interfaces']['dahdi-portgroup'][$uniqid_map[$group['uniqid']]] = $group;
	} else {
		$GLOBALS['config']['interfaces']['dahdi-portgroup'][] = $group;
	}

	write_config();
	touch($g['dahdi_dirty_path']);
}

function dahdi_delete_portgroup($uniqid) {
	global $g, $uniqid_map;

	unset($GLOBALS['config']['interfaces']['dahdi-portgroup'][$uniqid_map[$uniqid]]);

	write_config();
	touch($g['dahdi_dirty_path']);
}

function dahdi_get_gnumber($uniqid) {
	if (strpos($uniqid, "DAHDIPORT-") !== false) {
		$port = dahdi_get_port($uniqid);
		return $port['basechannel'];
	} else if (strpos($uniqid, "DAHDIPORTGROUP-") !== false) {
		$group = dahdi_get_portgroup($uniqid);
		return $group['number'];
	} else {
		return "NO_GNUMBER_FOUND";
	}
}

function dahdi_generate_system_conf($return=false, $ports=false, $gateways=false) {
	global $g, $config, $defaults;

	if (!$ports) {
		$ports = dahdi_get_ports();
	}
	if (!$gateways) {
		$gateways = redfone_get_gateways();
	}

	$loadzones = dahdi_get_loadzones();

	$conf = "# Hardware Info\n";
	foreach ($gateways as $gateway) {
		$conf .= "# Redfone gateway (" . $gateway['gwname'] . ") with " . $gateway['spancount'] . " spans\n";
	}
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


	$timingprio = 1;


	$conf .= "# Dynamic Spans\n";
	// dynamic=ethmf,<ethN>/<FB MAC address>/<span number>,<numchans>,<timing>
	foreach ($gateways as $gateway) {
		for ($i = 1; $i < $gateway['spancount'] + 1; $i++) {
			$chancount = ($gateway['span' . $i . 'type'] == "E1") ? 31 : 24;
			$conf .= "dynamic=ethmf," . $gateway['localif'] . "/" .
				$gateway['remotemac'] . "/" . ($i - 1) . "," . $chancount . "," . ($gateway['spancount'] + 1 - $i) . "\n";
		}
	}
	$conf .= "\n";


	$conf .= "# Spans\n";
	// span=<span num>,<timing source>,<line build out (LBO)>,<framing>,<coding>[,yellow]
	foreach ($ports as $port) {
		if ($port['technology'] == "isdn" && $port['totalchannels'] == 3) {
			// bri 'phone' ports:
			//	- lbo = 0, not used in bri
			//	- framing and coding always ccs and ami respectively
			if ($port['type'] == "nt") {
				$conf .= "span=" . $port['span'] . ",0,0,ccs,ami\n";
			// bri 'provider' ports
			} else if ($port['type'] == "te") {
				$conf .= "span=" . $port['span'] . "," . $timingprio++ . ",0,ccs,ami\n";
			}
		}
	}
	$conf .= "\n";


	$conf .= "# Channels\n";
	// <device>=<channel list>
	foreach ($gateways as $gateway) {
		$ch = 1;
		for ($i = 1; $i < $gateway['spancount'] + 1; $i++) {
			if ($gateway['span' . $i . 'type'] == "E1") {
				// bchan=1-15,17-31
				// dchan=16
				// alaw=1-31
				$conf .= "bchan=" . $ch . "-" . ($ch + 14) . "," . ($ch + 16) . "-" . ($ch + 30) . "\n";
				$conf .= "dchan=" . ($ch + 15) . "\n";
				$conf .= "alaw=" . $ch . "-" . ($ch + 30) . "\n";
				$ch += 31;
			} else {
				// bchan=1-23
				// dchan=24
				$conf .= "bchan=" . $ch . "-" . ($ch + 22) . "\n";
				$conf .= "dchan=" . ($ch + 23) . "\n";
				$ch += 24;
			}
		}
	}
	foreach ($ports as $port) {
		// analog port config
		if ($port['technology'] == "analog") {
			// signaling override or default?
			$startsignaling = ($port['startsignaling']) ?
				$port['startsignaling'] : $defaults['dahdi']['analog']['startsignaling'];
			// use fxo signaling for fxs ports and fxs signaling for fxo
			$signaling = ($port['type'] == "fxs") ? "fxo" : "fxs";
			$conf .= $signaling . $startsignaling . "=" . $port['basechannel'] . "\n";

		// bri isdn
		} else if ($port['technology'] == "isdn" && $port['totalchannels'] == 3) {
			// bri 'phone' ports
			if ($port['type'] == "nt") {
				$conf .= "bchan=" . $port['basechannel'] . "\n";
				$conf .= "bchan=" . ($port['basechannel'] + 1) . "\n";
				if (strpos($port['card'], "AUERSWALD COMpact ISDN") !== false) {
					$conf .= "dchan=" . ($port['basechannel'] + 2) . "\n";
				} else {
					$conf .= "hardhdlc=" . ($port['basechannel'] + 2) . "\n";
				}
			} else if ($port['type'] == "te") {
				$conf .= "bchan=" . $port['basechannel'] . "\n";
				$conf .= "bchan=" . ($port['basechannel'] + 1) . "\n";
				if (strpos($port['card'], "AUERSWALD COMpact ISDN") !== false) {
					$conf .= "dchan=" . ($port['basechannel'] + 2) . "\n";
				} else {
					$conf .= "hardhdlc=" . ($port['basechannel'] + 2) . "\n";
				}
			}
		}
	}
	$conf .= "\n";


	$conf .= "# Tone Zones\n";
	// loadzone=<zone>
	// defaultzone=<zone>
	foreach ($loadzones as $zone) {
		$conf .= "loadzone=" . $zone . "\n";
	}
	$conf .= "defaultzone=" . $loadzones[0] . "\n";
	$conf .= "\n";


	$conf .= "# Echo Cancellers\n";
	// echocanceller=<echocanceller name>,<channel(s)>
	foreach ($ports as $port) {
		// analog port config
		if ($port['technology'] == "analog") {
			if ($port['echo-taps'] != "disable") {
				$conf .= "echocanceller=oslec," . $port['basechannel'] . "\n";
			}

		// bri ports
		} else if ($port['technology'] == "isdn" && $port['totalchannels'] == 3) {
			// 'phone' ports
			if ($port['type'] == "nt") {
				if ($port['echo-taps'] != "disable") {
					$conf .= "echocanceller=oslec," . $port['basechannel'] . "-" . ($port['basechannel'] + 1) . "\n";
				}
			} else if ($port['type'] == "te") {
				if ($port['echo-taps'] != "disable") {
					$conf .= "echocanceller=oslec," . $port['basechannel'] . "-" . ($port['basechannel'] + 1) . "\n";
				}
			}
		}
	}
	$conf .= "\n";

	return ($return) ? $conf : util_file_write("/etc/dahdi/system.conf", $conf);
}

function chan_dahdi_conf_generate($return=false, $ports=false, $gateways=false) {
	global $g, $config, $defaults;

	if (!$ports) {
		$ports = dahdi_get_ports();
	}
	if (!$gateways) {
		$gateways = redfone_get_gateways();
	}

	$conf = "; Hardware Info\n";
	foreach ($gateways as $gateway) {
		$conf .= "; Redfone gateway (" . $gateway['gwname'] . ") with " . $gateway['spancount'] . " spans\n";
	}
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

		$pports = array();

		if (strpos($provider['port'], "DAHDIPORT-") !== false) {
			$pports[] = dahdi_get_port($provider['port']);
		} else if (strpos($provider['port'], "DAHDIPORTGROUP-") !== false) {
			$portgroup = dahdi_get_portgroup($provider['port']);
			foreach ($portgroup['groupmember'] as $member) {
				$pports[] = dahdi_get_port($member);
			}
		}

		foreach ($pports as $pport) {
			$conf .= "; Provider: " . $provider['name'] . ", Port " . $pport['name'] . "\n";
			$conf .= "signalling=fxs_" . $pport['startsignaling'] . "\n";
			$conf .= "rxgain=" . $pport['rxgain'] . "\n";
			$conf .= "txgain=" . $pport['txgain'] . "\n";

			if ($pport['echo-taps'] == "disable") {
				$conf .= "echocancel=no\n";
			} else {
				$conf .= "echocancel=" . $pport['echo-taps'] . "\n";
			}

			$conf .= "busydetect=yes\n";
			$conf .= "busycount=3\n";
			$conf .= "language=" . $provider['language'] . "\n";
			$conf .= "context=" . $provider['uniqid'] . "-incoming\n";
			if ($portgroup) {
				$conf .= "group=" . $pport['basechannel'] . "," . $portgroup['number'] . "\n";
			} else {
				$conf .= "group=" . $pport['basechannel'] . "\n";
			}
			if ($provider['manualattributes']) {
				$conf .= "; manual attributes\n";
				$conf .= base64_decode($provider['manualattributes']) . "\n";
				$conf .= "; end manual attributes\n";
			}
			$conf .= "channel => " . $pport['basechannel'] . "\n";
			$conf .= "\n";
		}
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
		$conf .= "group=" . $port['basechannel'] . "\n";
		if ($phone['manualattributes']) {
			$conf .= "; manual attributes\n";
			$conf .= base64_decode($phone['manualattributes']) . "\n";
			$conf .= "; end manual attributes\n";
		}
		$conf .= "channel => " . $port['basechannel'] . "\n";
		$conf .= "\n";
	}

	$isdnproviders = isdn_get_providers();
	foreach ($isdnproviders as $provider) {
		if (isset($provider['disabled'])) {
			continue;
		}

		if (strpos($provider['port'], "REDFONE") !== false) {
			// in case redfone spans start going into the double-digits, watch out here
			$gwid = substr($provider['port'], 0, -2);
			$spannum = substr($provider['port'], -1);
			$gateway = redfone_get_gateway($gwid);
			$basechannel = redfone_get_span_basechannel($gwid, $spannum);

			$conf .= "; Provider: " . $provider['name'] . "\n";
			$conf .= "signalling=pri_cpe\n";
			//$conf .= "rxgain=" . $port['rxgain'] . "\n";
			//$conf .= "txgain=" . $port['txgain'] . "\n";

			if ($gateway['span' . $spannum . 'echo-taps'] == "disable") {
				$conf .= "echocancel=no\n";
			} else {
				$conf .= "echocancel=" . $gateway['span' . $spannum . 'echo-taps'] . "\n";
			}

			$conf .= "language=" . $provider['language'] . "\n";
			$conf .= "context=" . $provider['uniqid'] . "-incoming\n";
			$conf .= "group=" . $basechannel . "\n";
			if ($provider['manualattributes']) {
				$conf .= "; manual attributes\n";
				$conf .= base64_decode($provider['manualattributes']) . "\n";
				$conf .= "; end manual attributes\n";
			}
			if ($gateway['span' . $spannum . 'type'] == "E1") {
				$conf .= "switchtype=euroisdn\n";
				$conf .= "channel => " . $basechannel . "-" . ($basechannel + 14) . "," .
					($basechannel + 16) . "-" . ($basechannel + 30) . "\n";
			} else {
				$conf .= "switchtype=national\n";
				$conf .= "channel => " . $basechannel . "-" . ($basechannel + 22) . "\n";
			}
			$conf .= "\n";

		} else if (strpos($provider['port'], "DAHDI") !== false) {

			$pports = array();

			if (strpos($provider['port'], "DAHDIPORT-") !== false) {
				$pports[] = dahdi_get_port($provider['port']);
			} else if (strpos($provider['port'], "DAHDIPORTGROUP-") !== false) {
				$portgroup = dahdi_get_portgroup($provider['port']);
				foreach ($portgroup['groupmember'] as $member) {
					$pports[] = dahdi_get_port($member);
				}
			}

			foreach ($pports as $pport) {
				$conf .= "; Provider: " . $provider['name'] . ", Port " . $pport['name'] . "\n";
				$conf .= "signalling=" . $pport['signaling'] . "\n";
				$conf .= "rxgain=" . $pport['rxgain'] . "\n";
				$conf .= "txgain=" . $pport['txgain'] . "\n";

				if ($pport['echo-taps'] == "disable") {
					$conf .= "echocancel=no\n";
				} else {
					$conf .= "echocancel=" . $pport['echo-taps'] . "\n";
				}

				$conf .= "language=" . $provider['language'] . "\n";
				$conf .= "context=" . $provider['uniqid'] . "-incoming\n";
				if ($portgroup) {
					$conf .= "group=" . $pport['basechannel'] . "," . $portgroup['number'] . "\n";
				} else {
					$conf .= "group=" . $pport['basechannel'] . "\n";
				}
				if ($provider['manualattributes']) {
					$conf .= "; manual attributes\n";
					$conf .= base64_decode($provider['manualattributes']) . "\n";
					$conf .= "; end manual attributes\n";
				}
				$conf .= "channel => " . $pport['basechannel'] . "-" . ($pport['basechannel'] + 1) . "\n";
				$conf .= "\n";
			}
		}
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
		$conf .= "group=" . $port['basechannel'] . "\n";
		if ($phone['manualattributes']) {
			$conf .= "; manual attributes\n";
			$conf .= base64_decode($phone['manualattributes']) . "\n";
			$conf .= "; end manual attributes\n";
		}
		$conf .= "channel => " . $port['basechannel'] . "-" . ($port['basechannel'] + 1) . "\n";
		$conf .= "\n";
	}

	$conf .= "\n";

	return ($return) ? $conf : util_file_write("/etc/asterisk/chan_dahdi.conf", $conf);
}

function dahdi_get_loadzones() {
	global $defaults;

	if (is_array($GLOBALS['config']['interfaces']['dahdi-options']['loadzone'])) {
		return $GLOBALS['config']['interfaces']['dahdi-options']['loadzone'];
	}

	return $defaults['dahdi']['loadzones'];
}

function dahdi_save_loadzones($loadzones) {
	global $defaults, $g;

	if (is_array($loadzones)) {
		$GLOBALS['config']['interfaces']['dahdi-options']['loadzone'] = $loadzones;
	} else {
		$GLOBALS['config']['interfaces']['dahdi-options']['loadzone'] = $defaults['dahdi']['loadzones'];
	}

	write_config();
	touch($g['dahdi_dirty_path']);
}

function dahdi_save_daa_options($daa_setup) {
	global $config, $d_sysrebootreqd_path;

	$old_daa_setup = $config['system']['regional']['analog']['fxo']['daa'];
	if (!empty($daa_setup)) {
		if ($old_daa_setup != $daa_setup) {
		$config['system']['regional']['analog']['fxo']['daa'] = $daa_setup;  
		write_config();
		touch($d_sysrebootreqd_path);
		}
	}
}

/**
 * Because opermode support in wcfxo still incomplete that function return 
 * the integer value 1 only for regions where TBR21 mode required (like old CTR21),
 * else return zero.
 */

function dahdi_get_daa_fallback($text_opermode) {
	$tbr21_keylist = explode(',', 'TBR21,AUSTRIA,BELGIUM,DENMARK,FINLAND,FRANCE,GERMANY,GREECE,ICELAND,IRELAND,ITALY,LUXEMBOURG,NETHERLANDS,NORWAY,PORTUGAL,SPAIN,SWEDEN,SWITZERLAND,UK');
	$daa_intval = 0;

	if (in_array($text_opermode, $tbr21_keylist)) {
		$daa_intval = 1;
	}
	return $daa_intval;
}

function dahdi_configure() {
	global $g;

	$ports = dahdi_get_ports();
	$gateways = redfone_get_gateways();

	dahdi_generate_system_conf(false, $ports, $gateways);
	chan_dahdi_conf_generate(false, $ports, $gateways);

	mwexec("/usr/sbin/dahdi_cfg");

	unlink_if_exists($g['dahdi_dirty_path']);
}

?>
