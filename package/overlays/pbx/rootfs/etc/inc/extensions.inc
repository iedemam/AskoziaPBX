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
 */
function extensions_conf_generate() {
	global $config, $g, $uniqid_map;
	
	$fd = fopen("/etc/asterisk/extensions.conf", "w");
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

	$product_name = system_get_product_name();

	$conf = <<<EOD

[macro-main];
exten => s,1,Dial($\{ARG1},$\{ARG5},$\{ARG4})
exten => s,n,NoOp(DIALSTATUS: $\{DIALSTATUS})
exten => s,n,Goto(s-$\{DIALSTATUS},1)

;exten => s-ANSWER
;exten => s-CANCEL
;exten => s-CHANUNAVAIL
;exten => s-CONGESTION
;exten => s-DONTCALL
;exten => s-TORTURE
;exten => s-INVALIDARGS
exten => s-BUSY,1,Goto(busy,1)
exten => s-NOANSWER,1,Goto(noanswer,1)
exten => _s-.,1,Goto(noanswer,1)

; busy
exten => busy,1,GotoIf($["$\{HASVOICEMAIL}" != ""]?vm-busy,1)
exten => busy,n,Busy(10)
exten => vm-busy,1,Wait(1)
exten => vm-busy,n,Set(MISSEDCAUSE=busy)
exten => vm-busy,n,GosubIf($["$\{SENDNOTIFICATIONS}" != ""]?mailnotification,1)
exten => vm-busy,n,ExecIf($["$\{NOVOICEMAILWHENBUSY}" != ""]|Busy|10)
exten => vm-busy,n,VoiceMail($\{ARG2},b,$\{ARG3})
exten => vm-busy,n,Playback(vm-goodbye)
exten => vm-busy,n,GotoIf($["$\{VMSTATUS}" = "SUCCESS"]?vm-message-exit,1:vm-nomessage-noexit,1)

; no answer
exten => noanswer,1,GotoIf($["$\{HASVOICEMAIL}" != ""]?vm-noanswer,1)
exten => noanswer,n,Hangup()
exten => vm-noanswer,1,Wait(1)
exten => vm-noanswer,n,Set(MISSEDCAUSE=no-answer)
exten => vm-noanswer,n,VoiceMail($\{ARG2},u,$\{ARG3})
exten => vm-noanswer,n,Playback(vm-goodbye)
exten => vm-noanswer,n,GotoIf($["$\{VMSTATUS}" = "SUCCESS"]?vm-message-exit,1:vm-nomessage-noexit,1)

; voicemail exit
exten => vm-message-exit,1,Set(MISSEDCAUSE=didntmiss)
exten => vm-message-exit,n,Hangup()
exten => vm-nomessage-exit,1,GosubIf($["$\{SENDNOTIFICATIONS}" != ""]?mailnotification,1)
exten => vm-nomessage-exit,n,Hangup()

; hangup
exten => h,1,GotoIf($["$\{HASVOICEMAIL}" != ""]?vm-h,1)
exten => h,n,Hangup()
exten => vm-h,1,ExecIf($["$\{DIALSTATUS}" = "ANSWER"]|Hangup)
exten => vm-h,n,ExecIf($["$\{MISSEDCAUSE}" = ""]|Set|MISSEDCAUSE=hungup)
exten => vm-h,n,GosubIf($[("$\{SENDNOTIFICATIONS}" != "") & ("$\{MISSEDCAUSE}" != "didntmiss")]?mailnotification,1)
exten => vm-h,n,Hangup()

; mail missed call notification
exten => mailnotification,1,Set(TMPMAILFILE=/var/spool/asterisk/tmp/email-$\{MD5($\{UNIQUEID})})
exten => mailnotification,n,System(touch $\{TMPMAILFILE})
exten => mailnotification,n,System(echo "date: `date`" > $\{TMPMAILFILE})
exten => mailnotification,n,System(echo "to: $\{SENDNOTIFICATIONS}" >> $\{TMPMAILFILE})
exten => mailnotification,n,System(echo "subject: Missed Call ($\{MISSEDCAUSE})" >> $\{TMPMAILFILE})
exten => mailnotification,n,System(echo "from: $product_name Notifications <{$vmconfig['address']}>" >> $\{TMPMAILFILE})
exten => mailnotification,n,System(echo >> $\{TMPMAILFILE})
exten => mailnotification,n,System(echo "$\{STRFTIME($\{EPOCH},,%Y-%m-%d - %H:%M)} from $\{CALLERID(all)}" >> $\{TMPMAILFILE})
exten => mailnotification,n,System(/usr/bin/msmtp -C /etc/msmtp.conf -t < $\{TMPMAILFILE})
exten => mailnotification,n,System(rm $\{TMPMAILFILE})
exten => mailnotification,n,Return()

;i - Invalid
;s - Start
;h - Hangup
;t - Timeout
;T - AbsoluteTimeout
;a - Asterisk extension
;o - Operator


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
		if (isset($p['disabled'])) {
			continue;
		}
		if (isset($p['allowdirectdial'])) {
			$conf .= "; internal phone : {$p['callerid']} - <{$p['extension']}>\n";
			if (!$p['publicname']) {
				$conf .= "exten => {$p['extension']},1,NoOp(public calling internal phone: {$p['callerid']} - <{$p['extension']}>)\n";
				$conf .= extensions_generate_internal_phone_extension($p, true);
			} else {
				$conf .= "exten => {$p['publicname']},1,NoOp(public calling internal phone: {$p['callerid']} - <{$p['extension']}>)\n";
				$conf .= extensions_generate_internal_phone_extension($p, true, false, $p['publicname']);
			}
		}
	}

	// external phone extensions
	foreach ($external_phones as $p) {
		if (isset($p['disabled'])) {
			continue;
		}
		if (isset($p['allowdirectdial'])) {
			$conf .= "; external phone : {$p['name']} - <{$p['extension']}>\n";
			if (!$p['publicname']) {
				$conf .= "exten => {$p['extension']},1,NoOp(public calling external phone: {$p['name']} - <{$p['extension']}>)\n";
				$conf .= extensions_generate_external_phone_extension($p, true);
			} else {
				$conf .= "exten => {$p['publicname']},1,NoOp(public calling external phone: {$p['name']} - <{$p['extension']}>)\n";
				$conf .= extensions_generate_external_phone_extension($p, true, false, $p['publicname']);
			}
		}
	}

	// callgroup extensions
	foreach ($callgroups as $cg) {
		if (!isset($cg['extension']) || !strlen($cg['extension'])) {
			continue;
		}
		if (isset($cg['allowdirectdial'])) {
			$conf .= "; callgroup : {$cg['name']} - <{$cg['extension']}>\n";
			if (!$cg['publicname']) {
				$conf .= "exten => {$cg['extension']},1,NoOp(public calling callgroup: {$cg['name']} - <{$cg['extension']}>)\n";
				$conf .= extensions_generate_callgroup_extension($cg, true);
			} else {
				$conf .= "exten => {$cg['publicname']},1,NoOp(public calling callgroup: {$cg['name']} - <{$cg['extension']}>)\n";
				$conf .= extensions_generate_callgroup_extension($cg, true, false, $cg['publicname']);
			}
		}
	}

	// application extensions
	foreach ($applications as $a) {
		if (isset($a['allowdirectdial'])) {
			$conf .= "; application : {$a['name']}\n";
			if (!$a['publicname']) {
				$conf .= "exten => {$a['extension']},1,NoOp(public calling application: {$a['name']})\n";
				$conf .= extensions_generate_application_extension($a, true);
			} else {
				$conf .= "exten => {$a['publicname']},1,NoOp(public calling application: {$a['name']})\n";
				$conf .= extensions_generate_application_extension($a, true, $a['publicname']);
			}
		}
	}

	// conference room extensions
	foreach ($conference_rooms as $r) {
		if (isset($r['allowdirectdial'])) {
			$conf .= "; conference room : {$r['name']} - <{$r['number']}>\n";
			if (!$r['publicname']) {
				$conf .= "exten => {$r['number']},1,NoOp(public calling conference: {$r['name']} - <{$r['number']}>)\n";
				$conf .= extensions_generate_conference_room_extension($r, true);
			} else {
				$conf .= "exten => {$r['publicname']},1,NoOp(public calling conference: {$r['name']} - <{$r['number']}>)\n";
				$conf .= extensions_generate_conference_room_extension($r, true, $r['publicname']);
			}
		}
	}
	$conf .= "\n";


	// internal extensions context
	$conf .= "[internal]\n";

	// internal phone extensions
	foreach ($internal_phones as $p) {
		if (isset($p['disabled'])) {
			continue;
		}
		$conf .= "; internal phone : {$p['callerid']} - <{$p['extension']}>\n";
		$conf .= "exten => {$p['extension']},1,NoOp(internal calling internal phone: {$p['callerid']} - <{$p['extension']}>)\n";
		$conf .= extensions_generate_internal_phone_extension($p);
	}

	// external phone extensions
	foreach ($external_phones as $p) {
		if (isset($p['disabled'])) {
			continue;
		}
		$conf .= "; external phone : {$p['name']} - <{$p['extension']}>\n";
		$conf .= "exten => {$p['extension']},1,NoOp(internal calling external phone: {$p['name']} - <{$p['extension']}>)\n";
		$conf .= extensions_generate_external_phone_extension($p);
	}

	// callgroup extensions
	foreach ($callgroups as $cg) {
		if (!isset($cg['extension']) || !strlen($cg['extension'])) {
			continue;
		}
		$conf .= "; callgroup : {$cg['name']} - <{$cg['extension']}>\n";
		$conf .= "exten => {$cg['extension']},1,NoOp(internal calling callgroup: {$cg['name']} - <{$cg['extension']}>)\n";
		$conf .= extensions_generate_callgroup_extension($cg);
	}

	// application extensions
	foreach ($applications as $a) {
		$conf .= "; application : {$a['name']}\n";
		$conf .= "exten => {$a['extension']},1,NoOp(internal calling application: {$a['name']})\n";
		$conf .= extensions_generate_application_extension($a);
	}

	// conference room extensions
	foreach ($conference_rooms as $r) {
		$conf .= "; conference room : {$r['name']} - <{$r['number']}>\n";
		$conf .= "exten => {$r['number']},1,NoOp(internal calling conference: {$r['name']} - <{$r['number']}>)\n";
		$conf .= extensions_generate_conference_room_extension($r);
	}

	$conf .= "; parking lot\n";
	$conf .= extensions_generate_parkinglot();

	$conf .= "; sip device hints\n";
	$conf .= extensions_generate_hints();

	// providers contexts
	foreach ($providers as $p) {
		if (isset($p['disabled'])) {
			continue;
		}
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

	// handling for multiple sip providers using same host
	$sip_multi = sip_get_hosts_with_multiple_accounts();
	foreach ($sip_multi as $host => $ids) {
		$conf .= "; multiple SIP Provider accounts found for host : $host\n";
		$conf .= "[SIP-PROVIDER-$host-incoming]\n";
		foreach ($ids as $id) {
			$sp = sip_get_provider($id);
			$conf .= "exten => {$sp['username']},1,Goto({$sp['uniqid']}-incoming,s,1)\n";
		}
	}
	$conf .= "\n";

	// internal phone contexts
	foreach($internal_phones as $p) {
		if (isset($p['disabled'])) {
			continue;
		}
		$conf .= "; {$p['callerid']} - <{$p['extension']}>\n";
		$conf .= "[{$p['uniqid']}]\n";
		$conf .= "include => internal\n";
		if (isset($p['provider'])) {
			foreach ($p['provider'] as $provider) {
				if (pbx_provider_is_active($provider)) {
					$conf .= "include => $provider\n";
				}
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
			$conf .= "exten => _[0-9*#]!/{$p['extension']},1,Set(CHANNEL(language)=$phone_lang)\n";
			$conf .= "exten => _[0-9*#]!/{$p['extension']},n,Goto({$p['uniqid']},$\{EXTEN},1)\n";
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
			$conf .= "exten => _[0-9*#]!,1,Set(CHANNEL(language)=$provider_lang)\n";
			$conf .= "exten => _[0-9*#]!,n,Goto({$p['uniqid']}-incoming,$\{EXTEN},1)\n";
			$conf .= "exten => s,1,Set(CHANNEL(language)=$provider_lang)\n";
			$conf .= "exten => s,n,Goto({$p['uniqid']}-incoming,$\{EXTEN},1)\n";
		}
		$conf .= "\n";
	}

	$conf .= <<<EOD
[outgoing-uri]
exten => _[0-9a-zA-Z*#].,1,NoOp(Direct dialing via SIP uri : $\{EXTEN}@$\{SIPDOMAIN})
exten => _[0-9a-zA-Z*#].,n,Dial(SIP/$\{EXTEN}@$\{SIPDOMAIN},,To)

EOD;

	$conf .= "\n";

	fwrite($fd, $conf);
	fclose($fd);
	
	return 0;
}

/**
 * 1. Adds a "Notifications" line containing the user's e-mail address
 * if configured. If not, this channel variable is set to "no"
 *
 * 2. Adds a variable indicating if voicemail is desired when channel is busy
 */
function extensions_generate_call_notification_variables($exten, $phone) {

	if ($phone['voicemailbox']) {

		$dial = "exten => $exten,n,Set(HASVOICEMAIL=\"yes\")\n";

		if (isset($phone['sendcallnotifications'])) {
			$dial .= "exten => $exten,n,Set(SENDNOTIFICATIONS=\"{$phone['voicemailbox']}\")\n";
		}

		if (isset($phone['novmwhenbusy'])) {
			$dial .= "exten => $exten,n,Set(NOVOICEMAILWHENBUSY=\"yes\")\n";
		}
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
		if (isset($p['disabled'])) {
			continue;
		}
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
		return "DAHDI/" . $phone['interface'];
	
	} else if (strstr($uniqid, "EXTERNAL-PHONE")) {

		if ($phone['dialprovider'] == "sipuri") {
			return "SIP/" . $phone['dialstring'];

		} else if ($phone['dialprovider'] == "iaxuri") {
			return "IAX2/" . $phone['dialstring'];
		}

		$technology = pbx_uniqid_to_technology($phone['dialprovider']);
		if ($technology == "DAHDI") {
			$prov = analog_get_provider($phone['dialprovider']);
			return "DAHDI/" . $prov['interface'] . "/" . $phone['dialstring'];

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
	$readbacknum = false;

	if ($tech == "CAPI") {
		$readbacknum = $p['msn'];

	} else if ($tech == "DAHDI") {
		$readbacknum = $p['number'];

	} else if ($tech == "SIP" || $tech == "IAX") {
		// read back username if it is numeric
		if (verify_is_numericint($p['username'])) {
			$readbacknum = $p['username'];
		}
		// override with readback number if it is set
		if (isset($p['readbacknumber'])) {
			$readbacknum = $p['readbacknumber'];
		}
	}

	// does this channel need a wait
	if ($tech == "CAPI" || $tech == "DAHDI") {
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
			$catchall = array("_[0-9*#]!", "s");
			// can we include the incoming username as a catch all extension
			if ($tech == "SIP") {
				$catchall[] = $p['username'];
			}
			foreach ($catchall as $catch) {
				$conf .= "exten => $catch,1,NoOp(incoming call from provider: {$p['name']} to $\{EXTEN})\n";
				if ($needswait) {
					$conf .= "exten => $catch,n,Ringing()\n";
					$conf .= "exten => $catch,n,Wait(1)\n";
				}

				$conf .= extensions_generate_incoming_callerid_overrides($catch, $p['override'], $p['overridestring']);
				$conf .= extensions_generate_extension_from_uniqid($destination, true, $readbacknum, $catch);
			}


		} else if (verify_is_numericint($pattern)) {
			$conf .= "exten => $pattern,1,NoOp(incoming call from provider: {$p['name']} to $\{EXTEN})\n";
			if ($needswait) {
				$conf .= "exten => $pattern,n,Ringing()\n";
				$conf .= "exten => $pattern,n,Wait(1)\n";
			}
			$conf .= extensions_generate_incoming_callerid_overrides($pattern, $p['override'], $p['overridestring']);
			$conf .= extensions_generate_extension_from_uniqid($destination, true, $pattern, $pattern);


		} else {
			$conf .= "exten => _$pattern,1,NoOp(incoming call from provider: {$p['name']} to $\{EXTEN})\n";
			if ($needswait) {
				$conf .= "exten => _$pattern,n,Ringing()\n";
				$conf .= "exten => _$pattern,n,Wait(1)\n";
			}
			$conf .= extensions_generate_incoming_callerid_overrides("_" . $pattern, $p['override'], $p['overridestring']);
			$conf .= extensions_generate_extension_from_uniqid($destination, true, $readbacknum, '_' . $pattern);
		}
		$conf .= "\n";
	}

	return $conf;
}

/**
 * Generates appropriate incoming caller id overrides for calls from providers
 */
function extensions_generate_incoming_callerid_overrides($extension, $override, $overridestring) {

	// - replace the incoming name with the incoming number
	if ($override == "replacenamewithnum") {
		return "exten => $extension,n,Set(CALLERID(name)=$\{CALLERID(number)})\n";

	// - replace the incoming caller id string
	} else if ($override == "replace") {
		return "exten => $extension,n,Set(CALLERID(all)=$overridestring)\n";

	// - prepend the incoming caller id string
	} else if ($override == "prepend") {
		return "exten => $extension,n,Set(CALLERID(name)=$overridestring $\{CALLERID(name)})\n";

	} else if ($override) {
		return "ERROR: unmatched case (override = $override) in extensions_generate_incoming_callerid_overrides!\n";

	} else {
		return NULL;
	}
}

/**
 * Returns a context generated for outgoing calls to a provider
 */
function extensions_generate_provider_outgoing_context($p) {

	$tech = pbx_uniqid_to_technology($p['uniqid']);
	
	$conf .= "[{$p['uniqid']}]\n";

	// outgoing caller id should be that of the phone originating the call
	$cidflag = "o";
	// set the appropriate flag if an override is wished and defined
	if ($p['calleridsource'] == "string" && $p['calleridstring']) {
		$cidflag = "f";
		$cidoverride = $p['calleridstring'];
	}

	foreach ($p['dialpattern'] as $pattern) {
		dialplan_parse_pattern(&$pattern, &$prefix, &$offset, &$exten);

		$conf .= "exten => _$pattern,1,NoOp(outgoing call to provider: {$p['name']})\n";

		// apply caller id override
		if ($cidoverride) {
			$conf .= "exten => _$pattern,n,Set(CALLERID(all)=$cidoverride)\n";
		}

		// generate technology specific dial()
		if ($tech == "SIP") {
			$conf .= "exten => _$pattern,n,Dial(SIP/$prefix$exten@{$p['uniqid']},,T$cidflag)\n";

		} else if ($tech == "IAX2") {
			$conf .= "exten => _$pattern,n,Dial(IAX2/{$p['uniqid']}/$prefix$exten,,T$cidflag)\n";

		} else if ($tech == "CAPI") {
			$conf .= "exten => _$pattern,n,Dial(CAPI/{$p['interface']}/$prefix$exten/bl,,T$cidflag)\n";

		} else if ($tech == "DAHDI") {
			$conf .= "exten => _$pattern,n,Dial(DAHDI/{$p['interface']}/$prefix$exten,,T$cidflag)\n";

		} else {
			$conf .= "ERROR: unmatched case in extensions_generate_provider_outgoing_context!";
		}

		// hangup
		$conf .= "exten => _$pattern,n,Hangup()\n";
	}

	return $conf;
}

/**
 * Returns an extension for an internal phone.
 */
function extensions_generate_internal_phone_extension($p, $from_public=false, $readbacknum=false, $extension=false) {
	global $defaults;
	
	$tech = pbx_uniqid_to_technology($p['uniqid']);
	$transfer_flags = (!$from_public) ? "tT" : "t";
	$phone_exten = $p['extension'];
	$exten = ($extension !== false) ? $extension : $phone_exten;
	$readbacknum = ($readbacknum !== false) ? $readbacknum : $exten;
	// XXX : should be fixed by better handling of the readback number in voicemail patch or rethought
	if (!verify_is_numericint($readbacknum)) {
		$readbacknum = $phone_exten;
	}

	// ring length
	$ringlength = $defaults['accounts']['phones']['ringlength'];
	if ($p['ringlength'] == "indefinitely") {
		$ringlength = "";
	} else if ($p['ringlength']) {
		$ringlength = $p['ringlength'];
	}

	$conf .= extensions_generate_call_notification_variables($exten, $p);
	if ($tech == "SIP") {
		$conf .= "exten => $exten,n,Macro(main|SIP/$phone_exten|$phone_exten|$readbacknum|" . $transfer_flags . "o|$ringlength)\n";

	} else if ($tech == "IAX2") {
		$conf .= "exten => $exten,n,Macro(main|IAX2/$phone_exten|$phone_exten|$readbacknum|" . $transfer_flags . "o|$ringlength)\n";

	} else if ($tech == "CAPI") {
		$conf .= "exten => $exten,n,Macro(main|CAPI/{$p['interface']}/$phone_exten|$phone_exten|$readbacknum|" . $transfer_flags . "o|$ringlength)\n";

	} else if ($tech == "DAHDI") {
		$conf .= "exten => $exten,n,Macro(main|DAHDI/{$p['interface']}|$phone_exten|$readbacknum|" . $transfer_flags . "o|$ringlength)\n";

	} else {
		$conf .= "ERROR: unmatched case in extensions_generate_internal_phone_extension!";
	}

	return $conf;
}

/**
 * Returns an extension for an external phone.
 */
function extensions_generate_external_phone_extension($p, $from_public=false, $readbacknum=false, $extension=false) {
	global $defaults;

	if ($p['dialprovider'] == "sipuri") {
		$tech = "SIP";
		$directdial = true;
	} else if ($p['dialprovider'] == "iaxuri") {
		$tech = "IAX2";
		$directdial = true;
	} else {
		$tech = pbx_uniqid_to_technology($p['dialprovider']);
	}

	$transfer_flags = (!$from_public) ? "tT" : "t";
	$phone_exten = $p['extension'];
	$exten = ($extension !== false) ? $extension : $phone_exten;
	$readbacknum = ($readbacknum !== false) ? $readbacknum : $exten;
	// XXX : should be fixed by better handling of the readback number in voicemail patch or rethought
	if (verify_is_numericint($readbacknum)) {
		$readbacknum = $phone_exten;
	}

	// ring length
	$ringlength = $defaults['accounts']['phones']['ringlength'];
	if ($p['ringlength'] == "indefinitely") {
		$ringlength = "";
	} else if ($p['ringlength']) {
		$ringlength = $p['ringlength'];
	}

	$conf .= extensions_generate_call_notification_variables($exten, $p);
	if ($tech == "DAHDI") {
		$prov = analog_get_provider($p['dialprovider']);
		$conf .= "exten => $exten,n,".
			"Macro(main|$tech/{$prov['interface']}/{$p['dialstring']}|$phone_exten|$readbacknum|" . $transfer_flags . "o|$ringlength)\n";

	} else if ($tech == "CAPI") {
		$prov = isdn_get_provider($p['dialprovider']);
		$conf .= "exten => $exten,n,".
				"Macro(main|$tech/{$prov['interface']}/{$p['dialstring']}|$phone_exten|$readbacknum|" . $transfer_flags . "o|$ringlength)\n";

	} else if (($tech == "SIP" || $tech == "IAX2") && $directdial) {
		$conf .= "exten => $exten,n,".
			"Macro(main|$tech/{$p['dialstring']}|$phone_exten|$readbacknum|" . $transfer_flags . "o|$ringlength)\n";

	} else if ($tech == "SIP" || $tech == "IAX2") {
		$conf .= "exten => $exten,n,".
			"Macro(main|$tech/{$p['dialprovider']}/{$p['dialstring']}|$phone_exten|$readbacknum|" . $transfer_flags . "o|$ringlength)\n";

	} else {
		$conf .= "ERROR: unmatched case in extensions_generate_external_phone_extension!";
	}
	
	return $conf;
}

/**
 * Generates a callgroup extension, returns nothing if
 * no extension is defined for this group
 */
function extensions_generate_callgroup_extension($group, $from_public=false, $readbacknum=false, $extension=false) {
	global $defaults;

	if (!is_array($group['groupmember'])) {
		return;
	}

	$transfer_flags = (!$from_public) ? "tT" : "t";
	$exten = ($extension !== false) ? $extension : $group['extension'];
	$dialstring = extensions_phone_uniqids_to_dialstring($group['groupmember']);
	$vm_phone = pbx_get_phone($group['groupmember'][0]);
	$readbacknum = ($readbacknum !== false) ? $readbacknum : $vm_phone['extension'];

	// ring length
	$ringlength = $defaults['accounts']['phones']['ringlength'];
	if ($group['ringlength'] == "indefinitely") {
		$ringlength = "";
	} else if ($group['ringlength']) {
		$ringlength = $group['ringlength'];
	}

	$conf .= extensions_generate_call_notification_variables($exten, $vm_phone);
	$conf .= "exten => $exten,n,Macro(main|$dialstring|{$vm_phone['extension']}|$readbacknum|$transfer_flags" . "o|$ringlength)\n";

	return $conf;
}

/**
 * Generates an application context
 */
function extensions_generate_application_context($a) {
	// init local vars
	$conf = '';
	$conf_pre ='';
	$conf_act ='';
	//	
	if (is_array($a['app-command'])) {
		$conf = "; " . $a['name'] . "\n";
		$conf .= "[" . $a['uniqid'] . "]\n";
		//
		foreach ($a['app-command'] as $ac) {
			// allow for include statements, don't process it
			if (preg_match('/^include\\s=>\\s[A-Z0-9-_]+/i', base64_decode($ac))) {
				$conf .= base64_decode($ac) . "\n";
			}
			else {
				$conf_pre .= "exten => _[0-9a-zA-Z*#]!," . base64_decode($ac) . "\n";
				$conf_act .= "exten => s," . base64_decode($ac) . "\n";
			}
		}
	}
	$conf .= $conf_pre . $conf_act;
	return $conf;
}

/**
 * Generates an extension which links to a local application
 */
function extensions_generate_application_extension($a, $from_public=false, $extension=false) {

	$exten = ($extension !== false) ? $extension : $a['extension'];

	return "exten => $exten,n,Dial(Local/$\{EXTEN}@" . $a['uniqid'] . "/n)\n";
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
		return (isset($p['disabled'])) ? "" : extensions_generate_external_phone_extension($p, $from_public, $readbacknum, $extension);

	// internal phones
	} else if ($id[1] == "PHONE") {
		$p = pbx_get_phone($uniqid);
		return (isset($p['disabled'])) ? "" : extensions_generate_internal_phone_extension($p, $from_public, $readbacknum, $extension);

	// conference rooms
	} else if ($id[0] == "CONFERENCE" && $id[1] == "ROOM") {
		$r = conferencing_get_room($uniqid);
		return extensions_generate_conference_room_extension($r, $from_public, $extension);

	// impromptu parallel callgroups
	} else if ($id[0] == "CALLGROUP" && $id[1] == "PARALLEL" && $id[2] == "impromptu") {
		return extensions_generate_callgroup_extension($impromptugroup, $from_public, $readbacknum, $extension);

	// stored parallel callgroups
	} else if ($id[0] == "CALLGROUP" && $id[1] == "PARALLEL") {
		$group = callgroups_get_group($uniqid);
		if (!isset($group['extension']) && $extension === false) {
			return;
		}
		return extensions_generate_callgroup_extension($group, $from_public, $readbacknum, $extension);

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
