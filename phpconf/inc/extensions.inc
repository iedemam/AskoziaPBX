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

/**
 * Returns all phones sorted by extension.
 * If no phones are defined, the global array is initialized.
 */
function external_get_phones() {
	global $config;
	
	if (!is_array($config['external']['phone']))
		$config['external']['phone'] = array();
		
	external_sort_phones();
		
	return $config['external']['phone'];
}

/**
 * Sorts phones by extension
 */
function external_sort_phones() {
	global $g, $config;

	usort($config['external']['phone'], "pbx_sort_by_extension");	
}

/**
 * Returns a specific phone based on its uniqid
 */
function external_get_phone($uniqid) {
	global $config, $uniqid_map;

	return $config['external']['phone'][$uniqid_map[$uniqid]];
}

/**
 * Returns an array of all assigned phone extensions
 */
function external_get_extensions() {
	global $config;
	
	$extensions = array();

	if (is_array($config['external']['phones'])) {
		foreach($config['external']['phones'] as $phone) {
			$extensions[] = $phone['extension'];
		}
	}

	return $extensions;
}

/**
 * Responsible for generating extensions.conf, the entire system's dialplan
 * XXX : this is huge and needs a rewrite so some flexibility is possible
 */
function extensions_conf_generate() {
	global $config, $g, $uniqid_map;
	
	$fd = fopen("/usr/local/etc/asterisk/extensions.conf", "w");
	if (!$fd) {
		printf("Error: cannot open extensions.conf in extensions_conf_generate().\n");
		return 1;
	}
	
	$vmconfig = $config['voicemail'];
	
	$conf = <<<EOD

[macro-vm];
exten => s,1,Dial($\{ARG1},20,$\{ARG4})
exten => s,n,NoOp($\{DIALSTATUS})
exten => s,n,Goto(s-$\{DIALSTATUS},1)

exten => s-NOANSWER,1,Wait(1)
exten => s-NOANSWER,n,Set(MISSEDCAUSE=no-answer)
exten => s-NOANSWER,n,VoiceMail($\{ARG2},u,$\{ARG3})
exten => s-NOANSWER,n,Goto(exit-$\{VMSTATUS},1)

exten => s-BUSY,1,Wait(1)
exten => s-BUSY,n,Set(MISSEDCAUSE=busy)
exten => s-BUSY,n,VoiceMail($\{ARG2},b,$\{ARG3})
exten => s-BUSY,n,Goto(exit-$\{VMSTATUS},1)

exten => s-ANSWER,1,Set(MISSEDCAUSE=didntmiss)
exten => s-ANSWER,n,Hangup()

exten => _s-.,1,Goto(s-NOANSWER,1)

exten => h,1,NoOp($\{MISSEDCAUSE})
exten => h,n,GosubIf($["$\{MISSEDCAUSE}" = ""]?reallyhungup)
exten => h,n,GosubIf($["$\{SENDNOTIFICATIONS}" != "no"]?notify)
exten => h,n,Hangup()
exten => h,n(reallyhungup),Set(MISSEDCAUSE=hungup)
exten => h,n,Return()
exten => h,n(notify),Macro(email)
exten => h,n,Return()

exten => exit-FAILED,1,Answer()
exten => exit-FAILED,n,Wait(1)
exten => exit-FAILED,n,Playback(vm-theperson)
exten => exit-FAILED,n,SayDigits($\{ARG3})
exten => exit-FAILED,n,Playback(vm-isunavail)
exten => exit-FAILED,n,GosubIf($["$\{SENDNOTIFICATIONS}" != "no"]?notify)
exten => exit-FAILED,n,Hangup()
exten => exit-FAILED,n(notify),Macro(email)
exten => exit-FAILED,n,Return()

exten => exit-SUCCESS,1,Playback(vm-goodbye)
exten => exit-SUCCESS,n,GosubIf($["$\{SENDNOTIFICATIONS}" != "no"]?notify)
exten => exit-SUCCESS,n,Hangup()
exten => exit-SUCCESS,n(notify),Macro(email)
exten => exit-SUCCESS,n,Return()

exten => exit-USEREXIT,1,Playback(vm-goodbye)
exten => exit-USEREXIT,n,GosubIf($["$\{SENDNOTIFICATIONS}" != "no"]?notify)
exten => exit-USEREXIT,n,Hangup()
exten => exit-USEREXIT,n(notify),Macro(email)
exten => exit-USEREXIT,n,Return()


[macro-email]
exten => s,1,Set(TMPMAILFILE=/var/spool/asterisk/tmp/email-$\{MD5($\{UNIQUEID})})
exten => s,n,System(touch $\{TMPMAILFILE})
exten => s,n,System(echo "date: `date`" > $\{TMPMAILFILE})
exten => s,n,System(echo "to: $\{SENDNOTIFICATIONS}" >> $\{TMPMAILFILE})
exten => s,n,System(echo "subject: Missed Call ($\{MISSEDCAUSE})" >> $\{TMPMAILFILE})
exten => s,n,System(echo "from: AskoziaPBX Notifications <{$vmconfig['address']}>" >> $\{TMPMAILFILE})
exten => s,n,System(echo >> $\{TMPMAILFILE})
exten => s,n,System(echo "$\{STRFTIME($\{EPOCH},,%Y-%m-%d - %H:%M)} from $\{CALLERID(all)}" >> $\{TMPMAILFILE})
exten => s,n,System(/usr/local/bin/msmtp -C /etc/msmtp.conf -t < $\{TMPMAILFILE})
exten => s,n,System(rm $\{TMPMAILFILE})


EOD;

	// internal extensions
	$conf .= "[internal]\n";

	$a_sipphones = sip_get_phones();
	foreach ($a_sipphones as $phone) {
		$conf .= "exten => {$phone['extension']},hint,SIP/{$phone['extension']}\n";
		$conf .= "exten => {$phone['extension']},1,NoOp()\n";
		$conf .= extensions_generate_missed_call_notifications_line($phone['extension'], $phone);
		$conf .= "exten => {$phone['extension']},n,Macro(vm|SIP/$\{EXTEN}|$\{EXTEN}|$\{EXTEN}|tTo)\n";
	}

	$a_iaxphones = iax_get_phones();
	foreach ($a_iaxphones as $phone) {
		$conf .= "exten => {$phone['extension']},1,NoOp()\n";
		$conf .= extensions_generate_missed_call_notifications_line($phone['extension'], $phone);
		$conf .= "exten => {$phone['extension']},n,Macro(vm|IAX2/$\{EXTEN}|$\{EXTEN}|$\{EXTEN}|tTo)\n";
	}

	$a_isdnphones = isdn_get_phones();
	foreach ($a_isdnphones as $phone) {
		$conf .= "exten => {$phone['extension']},1,NoOp()\n";
		$conf .= extensions_generate_missed_call_notifications_line($phone['extension'], $phone);
		$conf .= "exten => {$phone['extension']},n,Macro(vm|CAPI/{$phone['interface']}/$\{EXTEN}|$\{EXTEN}|$\{EXTEN}|tTo)\n";
	}

	$a_analogphones = analog_get_phones();
	foreach ($a_analogphones as $phone) {
		$conf .= "exten => {$phone['extension']},1,NoOp()\n";
		$conf .= extensions_generate_missed_call_notifications_line($phone['extension'], $phone);
		$conf .= "exten => {$phone['extension']},n,Macro(vm|ZAP/{$phone['interface']}/$\{EXTEN}|$\{EXTEN}|$\{EXTEN}|tTo)\n";
	}

	$a_extphones = external_get_phones();
	foreach ($a_extphones as $phone) {
		$technology = pbx_uniqid_to_technology($phone['dialprovider']);
		$conf .= "exten => {$phone['extension']},1,NoOp()\n";
		$conf .= extensions_generate_missed_call_notifications_line($phone['extension'], $phone);
		if ($technology == "ZAP") {
			$prov = analog_get_provider($phone['dialprovider']);
			$conf .= "exten => {$phone['extension']},n,".
				"Macro(vm|$technology/{$prov['interface']}/{$phone['dialstring']}|{$phone['extension']}|{$phone['extension']}|tTo)\n";

		} else if ($technology == "CAPI") {
			$prov = isdn_get_provider($phone['dialprovider']);
			$conf .= "exten => {$phone['extension']},n,".
					"Macro(vm|$technology/{$prov['interface']}/{$phone['dialstring']}|{$phone['extension']}|{$phone['extension']}|tTo)\n";

		} else {
			$conf .= "exten => {$phone['extension']},n,".
				"Macro(vm|$technology/{$phone['dialprovider']}/{$phone['dialstring']}|{$phone['extension']}|{$phone['extension']}|to)\n";	
		}
	}

	$a_callgroups = callgroups_get_groups();
	foreach ($a_callgroups as $group) {
		if (!$group['extension']) {
			continue;
		}
		$conf .= pbx_uniqid_to_dial($group['extension'], $group['uniqid']);
	}

	$a_applications = applications_get_apps();
	foreach ($a_applications as $app) {
		$conf .= "exten => {$app['extension']},1,Answer()\n";
		if ($app['name'] == "WakeMe") {
			$conf .= "exten => {$app['extension']},n,Set(CHANNEL(language)=en)\n";
		}
		$conf .= "exten => {$app['extension']},n,{$app['name']}()\n";
		$conf .= "exten => {$app['extension']},n,Hangup()\n";
		$conf .= "\n";
	}

	// conference rooms
	$rooms = conferencing_get_rooms();
	foreach ($rooms as $room) {
		$conf .= "exten => {$room['number']},1,Answer()\n";
		$conf .= "exten => {$room['number']},n,MeetMe({$room['number']},M)\n";
		$conf .= "exten => {$room['number']},n,Hangup()\n\n";
	}

	$conf .= extensions_generate_parkinglot();

	// providers contexts
	$a_providers = pbx_get_providers();
	foreach ($a_providers as $p) {
		if (count($p['incomingextensionmap'])) {
			$conf .= extensions_generate_provider_incoming_context($p);
		}
		if (count($p['dialpattern'])) {
			$conf .= extensions_generate_provider_outgoing_context($p);
		}
		$conf .= "\n";
	}

	// internal phones
	$intphones = pbx_get_phones();
	foreach($intphones as $p) {
		$conf .= "; {$p['callerid']}\n";
		$conf .= "[{$p['uniqid']}]\n";
		$conf .= "include => internal\n";
		if (isset($p['provider'])) {
			foreach ($p['provider'] as $provider) {
				$conf .= "include => $provider\n";
			}
		}
		$conf .= "\n";
	}


	// isdn nt ports
	$isdn_interfaces = isdn_get_nt_interfaces();
	foreach ($isdn_interfaces as $interface) {
		$phones = isdn_get_phones_on_unit($interface['unit']);
		if (!count($phones)) {
			continue;
		}
		$conf .= "; isdn nt interface \"{$interface['name']}\" incoming\n";
		$conf .= "[isdn-unit-{$interface['unit']}-incoming]\n";
		foreach ($phones as $p) {
			$phone_lang = "en";
			if ($p['language']) {
				$phone_lang = $p['language'];
			}
			$conf .= "exten => _X!/{$p['extension']},1,Set(CHANNEL(language)=$phone_lang)\n";
			$conf .= "exten => _X!/{$p['extension']},n,Goto({$p['uniqid']},$\{EXTEN},1)\n";
		}
		$conf .= "\n";
	}


	// isdn te ports
	$isdn_interfaces = isdn_get_te_interfaces();
	foreach ($isdn_interfaces as $interface) {
		$providers = isdn_get_providers_on_unit($interface['unit']);
		if (!count($providers)) {
			continue;
		}
		$conf .= "; isdn te interface \"{$interface['name']}\" incoming\n";
		$conf .= "[isdn-unit-{$interface['unit']}-incoming]\n";
		foreach ($providers as $p) {
			// XXX : multiple providers on a single s0 is broken
			$provider_lang = "en";
			if ($p['language']) {
				$provider_lang = $p['language'];
			}
			$conf .= "exten => _X!,1,Set(CHANNEL(language)=$provider_lang)\n";
			$conf .= "exten => _X!,n,Goto({$p['uniqid']}-incoming,$\{EXTEN},1)\n";
		}
		$conf .= "\n";
	}


	fwrite($fd, $conf);
	fclose($fd);
	
	return 0;
}

/**
 * Adds a "Notifications" line containing the user's e-mail address
 * if configured. If not, this channel variable is set to "no"
 */
function extensions_generate_missed_call_notifications_line($exten, $phone) {

	if ($phone['voicemailbox'] && isset($phone['sendcallnotifications'])) {
		$dial = "exten => $exten,n,Set(SENDNOTIFICATIONS=\"{$phone['voicemailbox']}\")\n";
	} else {
		$dial = "exten => $exten,n,Set(SENDNOTIFICATIONS=\"no\")\n";
	}

	return $dial;
}

/**
 * Generates parking lot hint entries
 */
function extensions_generate_parkinglot() {
	global $config;
	
	$pc = $config['dialplan']['callparking'];
	
	$parkstart = isset($pc['parkposstart']) ? $pc['parkposstart'] : "701";
	$parkend = isset($pc['parkposend']) ? $pc['parkposend'] : "720";
	
	$conf .= "include => parkedcalls\n";
	
	for ($i = $parkstart; $i <= $parkend; $i++) {
		$conf .= "exten => $i,1,ParkedCall($i)\n";
		$conf .= "exten => $i,hint,park:$i@parkedcalls\n";
	}
	$conf .= "\n\n";
	
	return $conf;
}

/**
 * Returns a group dialstring for an array of phone uniqids
 */
function extensions_phone_uniqids_to_dialstring($uniqid_array) {

	$dial_strings = array();
	foreach ($uniqid_array as $uniqid) {
		$dial_strings[] = extensions_phone_uniqid_to_dialstring($uniqid);
	}

	return implode("&", $dial_strings);
}

/**
 * Returns the dialstring for a phone based on its uniqid
 */
function extensions_phone_uniqid_to_dialstring($uniqid) {

	$phone = pbx_get_phone($uniqid);

	if (strstr($uniqid, "SIP-PHONE")) {
		return "SIP/" . $phone['extension'];
			
	} else if (strstr($uniqid, "IAX-PHONE")) {
		return "IAX2/" . $phone['extension'];

	} else if (strstr($uniqid, "ISDN-PHONE")) {
		return "CAPI/" . $phone['interface'] . "/" . $phone['extension'];

	} else if (strstr($uniqid, "ANALOG-PHONE")) {
		return "ZAP/" . $phone['interface'] . "/" . $phone['extension'];
	
	} else if (strstr($uniqid, "EXTERNAL-PHONE")) {
		$technology = pbx_uniqid_to_technology($phone['dialprovider']);
		if ($technology == "ZAP") {
			$prov = analog_get_provider($phone['dialprovider']);
			return "ZAP/" . $prov['interface'] . "/" . $phone['dialstring'];

		} else if ($technology == "CAPI"){
			$prov = isdn_get_provider($phone['dialprovider']);
			return "CAPI/" . $prov['interface'] . "/" . $phone['dialstring'];

		} else {
			return $technology . "/" . $phone['dialprovider'] . "/" . $phone['dialstring'];
		}

	} else {
		return "ERROR: unmatched uniqid in extensions_phone_uniqid_to_dialstring!";
	}
}

/**
 * Returns a context generated for incoming calls from a provider
 */
function extensions_generate_provider_incoming_context($p) {

	$tech = pbx_uniqid_to_technology($p['uniqid']);

	$conf .= "; {$p['name']} incoming\n";
	$conf .= "[{$p['uniqid']}-incoming]\n";

	$groupped_mapping = array();
	foreach ($p['incomingextensionmap'] as $map) {
		if (!isset($map['incomingpattern'])) {
			$groupped_mapping['all'][] = $map['destinationid'];
		} else {
			$groupped_mapping[$map['incomingpattern']][] = $map['destinationid'];
		}
	}
	
	if ($tech == "SIP") {
		foreach ($groupped_mapping as $pattern => $destination) {
			if ($pattern == "all") {
				$conf .= pbx_uniqid_to_dial("_X!", $destination, false, $p['override']);
				$conf .= pbx_uniqid_to_dial("s", $destination, false, $p['override']);
			} else if (is_numeric($pattern)) {
				$conf .= pbx_uniqid_to_dial($pattern, $destination, false, $p['override'], $pattern);
			} else {
				$conf .= pbx_uniqid_to_dial("_" . $pattern, $destination, false, $p['override']);
			}
			$conf .= "\n";
		}

	} else if ($tech == "IAX2") {		
		foreach ($groupped_mapping as $pattern => $destination) {
			if ($pattern == "all") {
				$conf .= pbx_uniqid_to_dial("_X!", $destination, false, $p['override']);
				$conf .= pbx_uniqid_to_dial("s", $destination, false, $p['override']);
			} else if (is_numeric($pattern)) {
				$conf .= pbx_uniqid_to_dial($pattern, $destination, false, $p['override'], $pattern);
			} else {
				$conf .= pbx_uniqid_to_dial("_" . $pattern, $destination, false, $p['override']);
			}
			$conf .= "\n";
		}

	} else if ($tech == "CAPI") {
		foreach ($groupped_mapping as $pattern => $destination) {
			if ($pattern == "all") {
				$conf .= pbx_uniqid_to_dial("_X!", $destination, 1, $p['override'], $p['msn']);
				$conf .= pbx_uniqid_to_dial("s", $destination, 1, $p['override'], $p['msn']);
			} else if (is_numeric($pattern)) {
				$conf .= pbx_uniqid_to_dial($pattern, $destination, 1, $p['override'], $pattern);
			} else {
				$conf .= pbx_uniqid_to_dial("_" . $pattern, $destination, 1, $p['override'], $p['msn']);
			}
			$conf .= "\n";
		}

	} else if ($tech == "ZAP") {
		foreach ($groupped_mapping as $pattern => $destination) {
			if ($pattern == "all") {
				$conf .= pbx_uniqid_to_dial("_X!", $destination, 1, $p['override'], $p['number']);
				$conf .= pbx_uniqid_to_dial("s", $destination, 1, $p['override'], $p['number']);
			} else if (is_numeric($pattern)) {
				$conf .= pbx_uniqid_to_dial($pattern, $destination, 1, $p['override'], $pattern);
			} else {
				$conf .= pbx_uniqid_to_dial("_" . $pattern, $destination, 1, $p['override'], $p['number']);
			}
			$conf .= "\n";
		}
	
	} else {
		$conf .= "ERROR: unmatched case in extensions_generate_provider_incoming_context!";
	}
	
	return $conf;
}

/**
 * Returns a context generated for outgoing calls to a provider
 */
function extensions_generate_provider_outgoing_context($p) {

	$tech = pbx_uniqid_to_technology($p['uniqid']);
	$cidflag = ($p['calleridsource'] == "string") ? "f" : "o";

	$conf .= "; {$p['name']} outgoing\n";
	$conf .= "[{$p['uniqid']}]\n";
	
	if ($tech == "SIP") {
		foreach ($p['dialpattern'] as $pattern) {
			dialplan_parse_pattern(&$pattern, &$prefix, &$offset, &$exten);
			$conf .= "exten => _$pattern,1,Dial(SIP/$prefix$exten@{$p['uniqid']},,T$cidflag)\n";
			$conf .= "exten => _$pattern,2,Hangup()\n";
		}

	} else if ($tech == "IAX2") {
		foreach ($p['dialpattern'] as $pattern) {
			dialplan_parse_pattern(&$pattern, &$prefix, &$offset, &$exten);
			$conf .= "exten => _$pattern,1,Dial(IAX2/{$p['uniqid']}/$prefix$exten,,T$cidflag)\n";
			$conf .= "exten => _$pattern,2,Hangup()\n";
		}

	} else if ($tech == "CAPI") {
		foreach ($p['dialpattern'] as $pattern) {
			dialplan_parse_pattern(&$pattern, &$prefix, &$offset, &$exten);				
			$conf .= "exten => _$pattern,1,Set(CALLERID(number)={$p['msn']})\n";
			$conf .= "exten => _$pattern,2,Dial(CAPI/{$p['interface']}/$prefix$exten/bl,,Tf)\n";
			$conf .= "exten => _$pattern,3,Hangup()\n";
		}

	} else if ($tech == "ZAP") {
		foreach ($p['dialpattern'] as $pattern) {
			dialplan_parse_pattern(&$pattern, &$prefix, &$offset, &$exten);				
			$conf .= "exten => _$pattern,1,Set(CALLERID(number)={$p['number']})\n";
			$conf .= "exten => _$pattern,2,Dial(ZAP/{$p['interface']}/$prefix$exten,,Tf)\n";
			$conf .= "exten => _$pattern,3,Hangup()\n";
		}
	} else {
		$conf .= "ERROR: unmatched case in extensions_generate_provider_outgoing_context!";
	}

	return $conf;
}

/**
 * Reloads dialplan to apply newly generated configuration changes
 */
function extensions_reload() {
	return pbx_exec("dialplan reload");
}

?>
