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

require_once("functions.inc");

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

	$internal_phones = pbx_get_phones();
	$external_phones = external_get_phones(); // XXX : refactor needed here re: internal vs external function names
	$callgroups = callgroups_get_groups();
	$applications = applications_get_apps();
	$conference_rooms = conferencing_get_rooms();
	$providers = pbx_get_providers();
	$isdn_nt_interfaces = isdn_get_nt_interfaces();
	$isdn_te_interfaces = isdn_get_te_interfaces();
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

[macro-outgoing-uri]
exten => s,1,NoOp(Direct dialing via SIP uri : $\{ARG1})
exten => s,n,Dial(SIP/$\{ARG1},,To)


EOD;

	// application contexts
	foreach ($applications as $a) {
		$conf .= extensions_generate_application_context($a);
		$conf .= "\n";
	}
	$conf .= "\n";

	// "public" extensions context
	$conf .= "[public-direct-dial]\n";

	// internal phone extensions
	foreach ($internal_phones as $p) {
		if (isset($p['allowdirectdial'])) {
			$conf .= "; internal phone : {$p['callerid']} - <{$p['extension']}>\n";
			if (!$p['publicname']) {
				$conf .= "exten => {$p['extension']},1,NoOp()\n";
				$conf .= extensions_generate_internal_phone_extension($p, true);
			} else {
				$conf .= "exten => {$p['publicname']},1,NoOp()\n";
				$conf .= extensions_generate_internal_phone_extension($p, true, false, $p['publicname']);
			}
		}
	}

	// external phone extensions
	foreach ($external_phones as $p) {
		if (isset($p['allowdirectdial'])) {
			$conf .= "; external phone : {$p['name']} - <{$p['extension']}>\n";
			if (!$p['publicname']) {
				$conf .= "exten => {$p['extension']},1,NoOp()\n";
				$conf .= extensions_generate_external_phone_extension($p, true);
			} else {
				$conf .= "exten => {$p['publicname']},1,NoOp()\n";
				$conf .= extensions_generate_external_phone_extension($p, true, false, $p['publicname']);
			}
		}
	}

	// callgroup extensions
	foreach ($callgroups as $g) {
		if (!isset($g['extension']) || !strlen($g['extension'])) {
			continue;
		}
		if (isset($g['allowdirectdial'])) {
			$conf .= "; callgroup : {$g['name']} - <{$g['extension']}>\n";
			if (!$g['publicname']) {
				$conf .= "exten => {$g['extension']},1,NoOp()\n";
				$conf .= extensions_generate_callgroup_extension($g, true);
			} else {
				$conf .= "exten => {$g['publicname']},1,NoOp()\n";
				$conf .= extensions_generate_callgroup_extension($g, true, false, $g['publicname']);
			}
		}
	}

	// application extensions
	foreach ($applications as $a) {
		if (isset($a['allowdirectdial'])) {
			$conf .= "; application : {$a['name']}\n";
			if (!$a['publicname']) {
				$conf .= "exten => {$a['extension']},1,NoOp()\n";
				$conf .= extensions_generate_application_extension($a, true);
			} else {
				$conf .= "exten => {$a['publicname']},1,NoOp()\n";
				$conf .= extensions_generate_application_extension($a, true, $a['publicname']);
			}
		}
	}

	// conference room extensions
	foreach ($conference_rooms as $r) {
		if (isset($r['allowdirectdial'])) {
			$conf .= "; conference room : {$r['name']} - <{$r['number']}>\n";
			if (!$r['publicname']) {
				$conf .= "exten => {$r['number']},1,NoOp()\n";
				$conf .= extensions_generate_conference_room_extension($r, true);
			} else {
				$conf .= "exten => {$r['publicname']},1,NoOp()\n";
				$conf .= extensions_generate_conference_room_extension($r, true, $r['publicname']);
			}
		}
	}
	$conf .= "\n";


	// internal extensions context
	$conf .= "[internal]\n";

	// internal phone extensions
	foreach ($internal_phones as $p) {
		$conf .= "; internal phone : {$p['callerid']} - <{$p['extension']}>\n";
		$conf .= "exten => {$p['extension']},1,NoOp()\n";
		$conf .= extensions_generate_internal_phone_extension($p);
	}

	// external phone extensions
	foreach ($external_phones as $p) {
		$conf .= "; external phone : {$p['name']} - <{$p['extension']}>\n";
		$conf .= "exten => {$p['extension']},1,NoOp()\n";
		$conf .= extensions_generate_external_phone_extension($p);
	}

	// callgroup extensions
	foreach ($callgroups as $g) {
		if (!isset($g['extension']) || !strlen($g['extension'])) {
			continue;
		}
		$conf .= "; callgroup : {$g['name']} - <{$g['extension']}>\n";
		$conf .= "exten => {$g['extension']},1,NoOp()\n";
		$conf .= extensions_generate_callgroup_extension($g);
	}

	// application extensions
	foreach ($applications as $a) {
		$conf .= "; application : {$a['name']}\n";
		$conf .= "exten => {$a['extension']},1,NoOp()\n";
		$conf .= extensions_generate_application_extension($a);
	}

	// conference room extensions
	foreach ($conference_rooms as $r) {
		$conf .= "; conference room : {$r['name']} - <{$r['number']}>\n";
		$conf .= "exten => {$r['number']},1,NoOp()\n";
		$conf .= extensions_generate_conference_room_extension($r);
	}

	$conf .= "; parking lot\n";
	$conf .= extensions_generate_parkinglot();

	$conf .= "; sip device hints\n";
	$conf .= extensions_generate_hints();

	// providers contexts
	foreach ($providers as $p) {
		if (count($p['incomingextensionmap'])) {
			$conf .= "; incoming provider : {$p['name']}\n";
			$conf .= extensions_generate_provider_incoming_context($p);
		}
		if (count($p['dialpattern'])) {
			$conf .= "; outgoing provider : {$p['name']}\n";
			$conf .= extensions_generate_provider_outgoing_context($p);
		}
		$conf .= "\n";
	}

	// internal phone contexts
	foreach($internal_phones as $p) {
		$conf .= "; {$p['callerid']} - <{$p['extension']}>\n";
		$conf .= "[{$p['uniqid']}]\n";
		$conf .= "include => internal\n";
		if (isset($p['provider'])) {
			foreach ($p['provider'] as $provider) {
				$conf .= "include => $provider\n";
			}
		}
		// allow direct outward dialing via sip uris
		if (isset($p['outbounduridial'])) {
			$conf .= "include => outgoing-uri\n";
		}
		$conf .= "\n";
	}

	// isdn nt port contexts
	foreach ($isdn_nt_interfaces as $i) {
		$phones = isdn_get_phones_on_unit($i['unit']);
		if (!count($phones)) {
			continue;
		}
		$conf .= "; isdn nt interface \"{$i['name']}\" incoming\n";
		$conf .= "[isdn-unit-{$i['unit']}-incoming]\n";
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

	// isdn te port contexts
	foreach ($isdn_te_interfaces as $i) {
		$providers = isdn_get_providers_on_unit($i['unit']);
		if (!count($providers)) {
			continue;
		}
		$conf .= "; isdn te interface \"{$i['name']}\" incoming\n";
		$conf .= "[isdn-unit-{$i['unit']}-incoming]\n";
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

	$conf .= <<<EOD
[outgoing-uri]
exten => _[a-z]!,1,Macro(outgoing-uri,$\{EXTEN}@$\{SIPDOMAIN})
exten => _[A-Z]!,1,Macro(outgoing-uri,$\{EXTEN}@$\{SIPDOMAIN})
exten => _X!,1,Macro(outgoing-uri,$\{EXTEN}@$\{SIPDOMAIN})

EOD;

	$conf .= "\n";

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
	// XXX : direct call to parking info
	$pc = $config['dialplan']['callparking'];
	
	$parkstart = isset($pc['parkposstart']) ? $pc['parkposstart'] : "701";
	$parkend = isset($pc['parkposend']) ? $pc['parkposend'] : "720";

	$conf .= "include => parkedcalls\n";
	
	for ($i = $parkstart; $i <= $parkend; $i++) {
		$conf .= "exten => $i,1,ParkedCall($i)\n";
		$conf .= "exten => $i,hint,park:$i@parkedcalls\n";
	}
	
	return $conf;
}

function extensions_generate_hints() {

	$phones = sip_get_phones();
	foreach ($phones as $p) {
		$conf .= "exten => {$p['extension']},hint,SIP/{$p['extension']}\n";
	}
	$conf .= "\n";

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
		return "ZAP/" . $phone['interface'];
	
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
	$catchall = array("_X!", "s");

	$conf = "[{$p['uniqid']}-incoming]\n";
	$conf .= "include => public-direct-dial\n";

	// form "impromptu group arrays" for destinations mapped to the the same patterns
	$groupped_mapping = array();
	foreach ($p['incomingextensionmap'] as $map) {
		if (!isset($map['incomingpattern'])) {
			$groupped_mapping['all'][] = $map['destinationid'];
		} else {
			$groupped_mapping[$map['incomingpattern']][] = $map['destinationid'];
		}
	}

	// assign read back number for provider
	if ($tech == "CAPI") {
		$readbacknum = $p['msn'];
	} else if ($tech == "ZAP") {
		$readbacknum = $p['number'];
	} else {
		$readbacknum = false;
	}

	// does this channel need a wait
	if ($tech == "CAPI" || $tech == "ZAP") {
		$needswait = true;
	}

	// generate incoming extensions
	foreach ($groupped_mapping as $pattern => $destination) {
		// XXX : hacky way to avoid handling multiple conditions in extensions_generate_extension_from_uniqid
		// XXX : this could be stored correctly in the first place...
		if (is_array($destination) && (count($destination) == 1)) {
			$destination = $destination[0];
		}
		
		if ($pattern == "all") {
			foreach ($catchall as $catch) {
				$conf .= "exten => $catch,1,NoOp()\n";
				if ($needswait) {
					$conf .= "exten => $catch,n,Ringing()\n";
					$conf .= "exten => $catch,n,Wait(1)\n";
				}
				if ($p['override'] == "replacenamewithnum") {
					$conf .= "exten => $catch,n,Set(CALLERID(name)=$\{CALLERID(number)})\n";
				}
				$conf .= extensions_generate_extension_from_uniqid($destination, true, $readbacknum, $catch);
			}


		} else if (verify_is_numericint($pattern)) {
			$conf .= "exten => $pattern,1,NoOp()\n";
			if ($needswait) {
				$conf .= "exten => $pattern,n,Ringing()\n";
				$conf .= "exten => $pattern,n,Wait(1)\n";
			}
			if ($p['override'] == "replacenamewithnum") {
				$conf .= "exten => $pattern,n,Set(CALLERID(name)=$\{CALLERID(number)})\n";
			}
			$conf .= extensions_generate_extension_from_uniqid($destination, true, $pattern, $pattern);


		} else {
			$conf .= "exten => _$pattern,1,NoOp()\n";
			if ($needswait) {
				$conf .= "exten => _$pattern,n,Ringing()\n";
				$conf .= "exten => _$pattern,n,Wait(1)\n";
			}
			if ($p['override'] == "replacenamewithnum") {
				$conf .= "exten => _$pattern,n,Set(CALLERID(name)=$\{CALLERID(number)})\n";
			}
			$conf .= extensions_generate_extension_from_uniqid($destination, true, $readbacknum, "_ " . $pattern);
		}
		$conf .= "\n";
	}

	return $conf;
}

/**
 * Returns a context generated for outgoing calls to a provider
 */
function extensions_generate_provider_outgoing_context($p) {

	$tech = pbx_uniqid_to_technology($p['uniqid']);
	$cidflag = ($p['calleridsource'] == "string") ? "f" : "o";

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
 * Returns an extension for an internal phone.
 */
function extensions_generate_internal_phone_extension($p, $from_public=false, $readbacknum=false, $extension=false) {
	
	$tech = pbx_uniqid_to_technology($p['uniqid']);
	$transfer_flags = (!$from_public) ? "tT" : "t";
	$phone_exten = $p['extension'];
	$exten = ($extension !== false) ? $extension : $phone_exten;
	$readbacknum = ($readbacknum !== false) ? $readbacknum : $exten;
	// XXX : should be fixed by better handling of the readback number in voicemail patch or rethought
	if (!verify_is_numericint($readbacknum)) {
		$readbacknum = $phone_exten;
	}

	$conf .= extensions_generate_missed_call_notifications_line($exten, $p);
	if ($tech == "SIP") {
		$conf .= "exten => $exten,n,Macro(vm|SIP/$phone_exten|$phone_exten|$readbacknum|" . $transfer_flags . "o)\n";

	} else if ($tech == "IAX2") {
		$conf .= "exten => $exten,n,Macro(vm|IAX2/$phone_exten|$phone_exten|$readbacknum|" . $transfer_flags . "o)\n";

	} else if ($tech == "CAPI") {
		$conf .= "exten => $exten,n,Macro(vm|CAPI/{$p['interface']}/$phone_exten|$phone_exten|$readbacknum|" . $transfer_flags . "o)\n";

	} else if ($tech == "ZAP") {
		$conf .= "exten => $exten,n,Macro(vm|ZAP/{$p['interface']}|$phone_exten|$readbacknum|" . $transfer_flags . "o)\n";

	} else {
		$conf .= "ERROR: unmatched case in extensions_generate_internal_phone_extension!";
	}

	return $conf;
}

/**
 * Returns an extension for an external phone.
 */
function extensions_generate_external_phone_extension($p, $from_public=false, $readbacknum=false, $extension=false) {
	
	$tech = pbx_uniqid_to_technology($p['dialprovider']);
	$transfer_flags = (!$from_public) ? "tT" : "t";
	$phone_exten = $p['extension'];
	$exten = ($extension !== false) ? $extension : $phone_exten;
	$readbacknum = ($readbacknum !== false) ? $readbacknum : $exten;
	// XXX : should be fixed by better handling of the readback number in voicemail patch or rethought
	if (verify_is_numericint($readbacknum)) {
		$readbacknum = $phone_exten;
	}

	$conf .= extensions_generate_missed_call_notifications_line($exten, $p);
	if ($tech == "ZAP") {
		$prov = analog_get_provider($p['dialprovider']);
		$conf .= "exten => $exten,n,".
			"Macro(vm|$tech/{$prov['interface']}/{$p['dialstring']}|$phone_exten|$readbacknum|" . $transfer_flags . "o)\n";

	} else if ($tech == "CAPI") {
		$prov = isdn_get_provider($p['dialprovider']);
		$conf .= "exten => $exten,n,".
				"Macro(vm|$tech/{$prov['interface']}/{$p['dialstring']}|$phone_exten|$readbacknum|" . $transfer_flags . "o)\n";

	} else if ($tech == "SIP" || $tech == "IAX2") {
		$conf .= "exten => $exten,n,".
			"Macro(vm|$tech/{$p['dialprovider']}/{$p['dialstring']}|$phone_exten|$readbacknum|" . $transfer_flags . "o)\n";

	} else {
		$conf .= "ERROR: unmatched case in extensions_generate_external_phone_extension!";
	}
	
	return $conf;
}

/**
 * Generates a callgroup extension, returns nothing if
 * no extension is defined for this group
 */
function extensions_generate_callgroup_extension($g, $from_public=false, $readbacknum=false, $extension=false) {

	$transfer_flags = (!$from_public) ? "tT" : "t";
	$exten = ($extension !== false) ? $extension : $g['extension'];
	$dialstring = extensions_phone_uniqids_to_dialstring($g['groupmember']);
	$vm_phone = pbx_get_phone($g['groupmember'][0]);
	$readbacknum = ($readbacknum !== false) ? $readbacknum : $vm_phone['extension'];

	$conf .= extensions_generate_missed_call_notifications_line($exten, $vm_phone);
	$conf .= "exten => $exten,n,Macro(vm|$dialstring|{$vm_phone['extension']}|$readbacknum|$transfer_flags" . "o)\n";

	return $conf;
}

/**
 * Generates an application context
 */
function extensions_generate_application_context($a) {

	if (is_array($a['app-command'])) {
		$conf = "; " . $a['name'] . "\n";
		$conf .= "[" . $a['uniqid'] . "]\n";
		foreach ($a['app-command'] as $ac) {
			$conf .= "exten => s," . base64_decode($ac) . "\n";
		}
	}

	return $conf;
}

/**
 * Generates an extension which links to a local application
 */
function extensions_generate_application_extension($a, $from_public=false, $extension=false) {

	$exten = ($extension !== false) ? $extension : $a['extension'];

	return "exten => $exten,n,Dial(Local/s@" . $a['uniqid'] . "/n)\n";
}

function extensions_generate_conference_room_extension($r, $from_public=false, $extension=false) {

	$number = $r['number'];
	$exten = ($extension !== false) ? $extension : $number;

	$conf .= "exten => $exten,n,Answer()\n";
	$conf .= "exten => $exten,n,MeetMe($number,M)\n";
	$conf .= "exten => $exten,n,Hangup()\n";

	return $conf;
}

function extensions_generate_extension_from_uniqid($uniqid, $from_public=false, $readbacknum=false, $extension=false) {

	// handle impromptu call groups 
	if (is_array($uniqid)) {
		$impromptugroup = array();
		$impromptugroup['groupmember'] = $uniqid;
		$impromptugroup['name'] = "impromptu";
		$uniqid = "CALLGROUP-PARALLEL-impromptu";
	}

	$id = split("-", $uniqid);

	// external phones
	if ($id[0] == "EXTERNAL" && $id[1] == "PHONE") {
		$p = pbx_get_phone($uniqid);
		return extensions_generate_external_phone_extension($p, $from_public, $readbacknum, $extension);

	// internal phones
	} else if ($id[1] == "PHONE") {
		$p = pbx_get_phone($uniqid);
		return extensions_generate_internal_phone_extension($p, $from_public, $readbacknum, $extension);

	// conference rooms
	} else if ($id[0] == "CONFERENCE" && $id[1] == "ROOM") {
		$r = conferencing_get_room($uniqid);
		return extensions_generate_conference_room_extension($r, $from_public, $extension);

	// impromptu parallel callgroups
	} else if ($id[0] == "CALLGROUP" && $id[1] == "PARALLEL" && $id[2] == "impromptu") {
		return extensions_generate_callgroup_extension($impromptugroup, $from_public, $readbacknum, $extension);

	// stored parallel callgroups
	} else if ($id[0] == "CALLGROUP" && $id[1] == "PARALLEL") {
		$g = callgroups_get_group($uniqid);
		if (!isset($g['extension']) && $extension === false) {
			return;
		}
		return extensions_generate_callgroup_extension($g, $from_public, $readbacknum, $extension);

	// applications
	} else if ($id[0] == "APPLICATION" && $id[1] == "MAPPING") {
		$a = applications_get_app($uniqid);
		return extensions_generate_application_extension($a, $from_public, $extension);

	} else {
		return "ERROR: unmatched case in extensions_generate_extension_from_uniqid!";
	}
}

/**
 * Reloads dialplan to apply newly generated configuration changes
 */
function extensions_reload() {
	return pbx_exec("dialplan reload");
}

?>
