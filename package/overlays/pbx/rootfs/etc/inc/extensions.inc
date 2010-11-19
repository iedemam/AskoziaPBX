<?php
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2010 tecema (a.k.a IKT) <http://www.tecema.de>.
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

/**
 * Responsible for generating extensions.conf, the entire system's dialplan
 */
function extensions_conf_generate($return=false) {
	global $config, $g, $uniqid_map;

	$internal_phones = pbx_get_phones();
	$external_phones = external_get_phones(); // XXX : refactor needed here re: internal vs external function names
	$callgroups = callgroups_get_groups();
	$applications = applications_get_applications();
	$conference_rooms = conferencing_get_rooms();
	$virtual_faxes = virtual_get_faxes();
	$analog_faxes = analog_get_faxes();
	$providers = pbx_get_providers();
	$isdn_nt_interfaces = dahdi_get_ports("isdn", "nt");
	$isdn_te_interfaces = dahdi_get_ports("isdn", "te");
	$vmconfig = voicemail_get_configuration();
	$emailconfig = notifications_get_email_configuration();

	$product_name = system_get_product_name();

	$conf = <<<EOD

[macro-main];
exten => s,1,Dial($\{ARG1},$\{ARG5},$\{ARG4})
exten => s,2,NoOp(DIALSTATUS: $\{DIALSTATUS})
exten => s,3,Goto(s-$\{DIALSTATUS},1)

;exten => s-ANSWER
;exten => s-CANCEL
;exten => s-CHANUNAVAIL
;exten => s-CONGESTION
;exten => s-DONTCALL
;exten => s-TORTURE
;exten => s-INVALIDARGS
exten => s-BUSY,1,Set(MISSEDCAUSE=busy)
exten => s-BUSY,2,Goto(busy,1)
exten => s-NOANSWER,1,Set(MISSEDCAUSE=no-answer)
exten => s-NOANSWER,2,Goto(noanswer,1)
exten => _s-.,1,Goto(noanswer,1)

; busy
exten => busy,1,Busy(10)
exten => busy,2,Hangup()

; no answer
exten => noanswer,1,NoOp(Call not answered in time)
exten => noanswer,2,GotoIf($["$\{HASVOICEMAIL}" != ""]?vm-noanswer,1)
exten => noanswer,3,Hangup()
exten => vm-noanswer,1,NoOp(Callee has voicemail, sending to voicemail)
exten => vm-noanswer,2,Wait(1)
exten => vm-noanswer,3,VoiceMail($\{ARG2},u,$\{ARG3})
exten => vm-noanswer,4,Playback(vm-goodbye)
exten => vm-noanswer,5,Hangup()

; hangup
exten => h,1,NoOp(Entering the main macro h hangup extension)

;i - Invalid
;s - Start
;h - Hangup
;t - Timeout
;T - AbsoluteTimeout
;a - Asterisk extension
;o - Operator


; missed call notification mail macro
[macro-missedcallmail];
exten => s,1,NoOp(Entering the missed call mail macro)
exten => s,n,NoOp(DIALSTATUS: $\{DIALSTATUS})
exten => s,n,ExecIf($["$\{DIALSTATUS}" = "ANSWER"]?MacroExit())
exten => s,n,NoOp(SENDNOTIFICATIONS: $\{SENDNOTIFICATIONS})
exten => s,n,ExecIf($["$\{SENDNOTIFICATIONS}" = ""]?MacroExit())
exten => s,n,NoOp(MISSEDCAUSE: $\{MISSEDCAUSE})
exten => s,n,ExecIf($["$\{MISSEDCAUSE}" = "didntmiss"]?MacroExit())
exten => s,n,NoOp(VMSTATUS: $\{VMSTATUS})
exten => s,n,ExecIf($[$["$\{VMSTATUS}" != ""] & $["$\{VMSTATUS}" != "FAILED"]]?MacroExit())
exten => s,n,Set(TMPMAILFILE=/var/asterisk/spool/tmp/email-$\{MD5($\{UNIQUEID})})
exten => s,n,System(touch $\{TMPMAILFILE})
exten => s,n,System(echo "date: `date`" > $\{TMPMAILFILE})
exten => s,n,System(echo "to: $\{SENDNOTIFICATIONS}" >> $\{TMPMAILFILE})
exten => s,n,System(echo "subject: Missed Call ($\{MISSEDCAUSE})" >> $\{TMPMAILFILE})
exten => s,n,System(echo "from: $product_name Notifications <{$emailconfig['address']}>" >> $\{TMPMAILFILE})
exten => s,n,System(echo >> $\{TMPMAILFILE})
exten => s,n,System(echo "$\{STRFTIME($\{EPOCH},,%Y-%m-%d - %H:%M)} from $\{CALLERID(all)}" >> $\{TMPMAILFILE})
exten => s,n,System(/usr/bin/msmtp -C /etc/msmtp.conf -t < $\{TMPMAILFILE})
exten => s,n,System(rm $\{TMPMAILFILE})


EOD;


	// application contexts
	foreach ($applications as $a) {
		if (isset($a['disabled'])) {
			continue;
		}
		$conf .= extensions_generate_application_context2($a);
		$conf .= "\n";
	}
	$conf .= "\n";

	// virtual fax contexts
	foreach ($virtual_faxes as $f) {
		if (isset($f['disabled'])) {
			continue;
		}
		$conf .= extensions_generate_virtual_fax_context($f);
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
		if ($p['publicaccess'] == "enabled") {
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
		if ($p['publicaccess'] == "enabled") {
			$conf .= "; external phone : {$p['callerid']} - <{$p['extension']}>\n";
			if (!$p['publicname']) {
				$conf .= "exten => {$p['extension']},1,NoOp(public calling external phone: {$p['callerid']} - <{$p['extension']}>)\n";
				$conf .= extensions_generate_external_phone_extension($p, true);
			} else {
				$conf .= "exten => {$p['publicname']},1,NoOp(public calling external phone: {$p['callerid']} - <{$p['extension']}>)\n";
				$conf .= extensions_generate_external_phone_extension($p, true, false, $p['publicname']);
			}
		}
	}

	// callgroup extensions
	foreach ($callgroups as $cg) {
		if (!isset($cg['extension']) || !strlen($cg['extension'])) {
			continue;
		}
		if ($cg['publicaccess'] == "enabled") {
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
		if (isset($a['disabled'])) {
			continue;
		}
		if ($a['publicaccess'] == "enabled") {
			$conf .= "; application : {$a['name']}\n";
			if (!$a['publicname']) {
				$extension = extensions_patternify($a['extension']);
				$conf .= "exten => $extension,1,NoOp(public calling application: {$a['name']})\n";
				$conf .= extensions_generate_application_extension($a, true, $extension);
			} else {
				$conf .= "exten => {$a['publicname']},1,NoOp(public calling application: {$a['name']})\n";
				$conf .= extensions_generate_application_extension($a, true, $a['publicname']);
			}
		}
	}

	// conference room extensions
	foreach ($conference_rooms as $r) {
		if ($r['publicaccess'] == "enabled") {
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
		$conf .= "; external phone : {$p['callerid']} - <{$p['extension']}>\n";
		$conf .= "exten => {$p['extension']},1,NoOp(internal calling external phone: {$p['callerid']} - <{$p['extension']}>)\n";
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
		if (isset($a['disabled'])) {
			continue;
		}
		$extension = extensions_patternify($a['extension']);
		$conf .= "; application : {$a['name']}\n";
		$conf .= "exten => $extension,1,NoOp(internal calling application: {$a['name']})\n";
		$conf .= extensions_generate_application_extension($a, false, $extension);
	}

	// conference room extensions
	foreach ($conference_rooms as $r) {
		$conf .= "; conference room : {$r['name']} - <{$r['number']}>\n";
		$conf .= "exten => {$r['number']},1,NoOp(internal calling conference: {$r['name']} - <{$r['number']}>)\n";
		$conf .= extensions_generate_conference_room_extension($r);
	}

	// virtual fax extensions
	foreach ($virtual_faxes as $f) {
		if (isset($f['disabled'])) {
			continue;
		}
		$conf .= "; virtual fax : {$f['callerid']} - <{$f['extension']}>\n";
		$conf .= "exten => {$f['extension']},1,NoOp(internal calling virtual fax: {$f['callerid']} - <{$f['extension']}>)\n";
		$conf .= extensions_generate_virtual_fax_extension($f);
	}

	// analog fax extensions
	foreach ($analog_faxes as $f) {
		if (isset($f['disabled'])) {
			continue;
		}
		$conf .= "; analog fax : {$f['callerid']} - <{$f['extension']}>\n";
		$conf .= "exten => {$f['extension']},1,NoOp(internal calling analog fax: {$f['callerid']} - <{$f['extension']}>)\n";
		$conf .= extensions_generate_analog_fax_extension($f);
	}

	$conf .= "; parking lot\n";
	$conf .= extensions_generate_parkinglot();

	$conf .= "; sip device hints\n";
	$conf .= extensions_generate_hints();

	$conf .= "\n";
	$conf .= "; hangup logic\n";
	$conf .= "exten => h,1,NoOp(Entering the internal context hangup extension)\n";
	$conf .= "exten => h,n,Macro(missedcallmail)\n";
	$conf .= "\n";


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
		$conf .= "include => {$p['uniqid']}-outgoing\n";
		$conf .= "include => outgoing-uri\n";
		$conf .= "\n";
		$conf .= extensions_generate_phone_outgoing_context($p);
		$conf .= "\n";
	}

	// analog fax contexts
	foreach($analog_faxes as $f) {
		if (isset($f['disabled'])) {
			continue;
		}
		$conf .= "; {$f['callerid']} - <{$f['extension']}>\n";
		$conf .= "[{$f['uniqid']}]\n";
		$conf .= "include => internal\n";
		$conf .= "include => {$f['uniqid']}-outgoing\n";
		$conf .= "\n";
		$conf .= extensions_generate_phone_outgoing_context($f);
		$conf .= "\n";
	}

	$conf .= <<<EOD
[outgoing-uri]
exten => _[0-9a-zA-Z*#].,1,NoOp(Direct dialing via SIP uri : $\{EXTEN}@$\{SIPDOMAIN})
exten => _[0-9a-zA-Z*#].,n,Dial(SIP/$\{EXTEN}@$\{SIPDOMAIN},,To)

EOD;

	$conf .= "\n";

	return ($return) ? $conf : util_file_write("/etc/asterisk/extensions.conf", $conf);
}

/**
 * 1. Adds a "Notifications" line containing the user's e-mail address
 * if configured. If not, this channel variable is set to "no"
 *
 * 2. Adds a variable indicating if voicemail is desired when channel is busy
 */
function extensions_generate_call_notification_variables($exten, $phone) {

	if ($phone['vmtoemail'] == "enabled") {
		$dial .= "exten => $exten,n,Set(HASVOICEMAIL=\"yes\")\n";
	}
	if ($phone['emailcallnotify'] == "enabled") {
		$dial .= "exten => $exten,n,Set(MISSEDCAUSE=hungup)\n";
		$dial .= "exten => $exten,n,Set(SENDNOTIFICATIONS=\"{$phone['emailcallnotifyaddress']}\")\n";
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

	} else if (strstr($uniqid, "SKINNY-PHONE")) {
		return "SKINNY/" . $phone['extension'];

	} else if (strstr($uniqid, "ANALOG-PHONE") || strstr($uniqid, "ISDN-PHONE")) {
		$dahdi_port = dahdi_get_port($phone['port']);
		return "DAHDI/g" . $dahdi_port['basechannel'];

	} else if (strstr($uniqid, "EXTERNAL-PHONE")) {

		if ($phone['dialprovider'] == "sipuri") {
			return "SIP/" . $phone['dialstring'];

		} else if ($phone['dialprovider'] == "iaxuri") {
			return "IAX2/" . $phone['dialstring'];
		}

		$technology = pbx_uniqid_to_technology($phone['dialprovider']);
		if ($technology == "DAHDI") {
			$provider = pbx_get_provider($phone['dialprovider']);
			return "DAHDI/g" . dahdi_get_gnumber($provider['port']) . "/" . $phone['dialstring'];

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


	// fax handling is special and shows why this function should be rewritten
	if ($p['defaultfax']) {
		$conf .= "exten => fax,1,NoOp(incoming fax from provider: {$p['name']} to $\{EXTEN})\n";
		$conf .= extensions_generate_extension_from_uniqid($p['defaultfax'], true, false, "fax");
	}


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

	if ($tech == "DAHDI") {
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
				$conf .= extensions_generate_incoming_callerid_overrides($catch, $p['override'], $p['overridestring']);
				$conf .= extensions_generate_extension_from_uniqid($destination, true, $readbacknum, $catch);
			}

		} else if (verify_is_numericint($pattern)) {
			$conf .= "exten => $pattern,1,NoOp(incoming call from provider: {$p['name']} to $\{EXTEN})\n";
			$conf .= extensions_generate_incoming_callerid_overrides($pattern, $p['override'], $p['overridestring']);
			$conf .= extensions_generate_extension_from_uniqid($destination, true, $pattern, $pattern);

		} else {
			$conf .= "exten => _$pattern,1,NoOp(incoming call from provider: {$p['name']} to $\{EXTEN})\n";
			$conf .= extensions_generate_incoming_callerid_overrides("_" . $pattern, $p['override'], $p['overridestring']);
			$conf .= extensions_generate_extension_from_uniqid($destination, true, $readbacknum, '_' . $pattern);
		}
		$conf .= "; hangup logic\n";
		$conf .= "exten => h,1,NoOp(Hanging up incoming call from provider: {$p['name']} to $\{EXTEN})\n";
		$conf .= "exten => h,n,Macro(missedcallmail)\n";
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

	foreach ($p['dialpattern'] as $pattern) {
		dialplan_parse_pattern(&$pattern, &$prefix, &$offset, &$exten);

		$conf .= "exten => _$pattern,1,NoOp(outgoing call to provider: {$p['name']})\n";

		// generate technology specific dial()
		if ($tech == "SIP") {
			$conf .= "exten => _$pattern,n,Dial(SIP/$prefix$exten@{$p['uniqid']},,T$cidflag)\n";

		} else if ($tech == "IAX2") {
			$conf .= "exten => _$pattern,n,Dial(IAX2/{$p['uniqid']}/$prefix$exten,,T$cidflag)\n";

		} else if ($tech == "DAHDI") {
			$gnumber = dahdi_get_gnumber($p['port']);
			$conf .= "exten => _$pattern,n,Dial(DAHDI/g$gnumber/$prefix$exten,,T$cidflag)\n";

		} else {
			$conf .= "ERROR: unmatched case in extensions_generate_provider_outgoing_context!";
		}

		if ($p['failover'] && ($failover = pbx_get_provider($p['failover']))) {
			$conf .= "exten => _$pattern,n,NoOp(DIALSTATUS = $\{DIALSTATUS})\n";
			$conf .= "exten => _$pattern,n,Goto({$p['uniqid']}-$\{DIALSTATUS},1)\n";
			$conf .= "exten => {$p['uniqid']}-NOANSWER,1,Hangup()\n";
			$conf .= "exten => {$p['uniqid']}-CANCEL,1,Hangup()\n";
			$conf .= "exten => {$p['uniqid']}-BUSY,1,Hangup()\n";
			$conf .= "exten => {$p['uniqid']}-CONGESTION,1,NoOp(Failover needed, now using : " . $failover['name'] . ")\n";
			$conf .= "exten => {$p['uniqid']}-CONGESTION,n,Goto({$p['failover']},$\{ORIGEXTENSION},1)\n";
			$conf .= "exten => {$p['uniqid']}-CHANUNAVAIL,1,NoOp(Failover needed, now using : " . $failover['name'] . ")\n";
			$conf .= "exten => {$p['uniqid']}-CHANUNAVAIL,n,Goto({$p['failover']},$\{ORIGEXTENSION},1)\n";

		} else {
			$conf .= "exten => _$pattern,n,Hangup()\n";
		}
	}

	return $conf;
}

function extensions_generate_phone_outgoing_context($phone) {

	$providers = pbx_get_providers();

	$conf .= "[{$phone['uniqid']}-outgoing]\n";

	// include each provider's patterns unless blocked
	foreach ($providers as $provider) {
		if (!isset($provider['disabled'])) {
			if ((!isset($phone['provider'])) || (!in_array($provider['uniqid'], $phone['provider']))) {
				// outgoing caller id should be that of the phone originating the call
				$cidflag = "o";
				// set the appropriate flag if an override is wished and defined
				if ($provider['calleridsource'] == "string" && $provider['calleridstring']) {
					$cidflag = "f";
					$cidoverride = $provider['calleridstring'];
				}
				// set the appropriate caller id override if this phone has one set
				$indexed_map = array();
				if ($phone['outgoingcalleridmap']) {
					foreach ($phone['outgoingcalleridmap'] as $map) {
						if ($map['provideruniqid'] == $provider['uniqid']) {
							$cidflag = "f";
							$cidoverride = $map['cidstring'];
						}
					}
				}

				foreach ($provider['dialpattern'] as $pattern) {
					dialplan_parse_pattern(&$pattern, &$prefix, &$offset, &$exten);

					$conf .= "exten => _$pattern,1,NoOp(pre-processing for outgoing call to provider: {$provider['name']})\n";
					$conf .= "exten => _$pattern,n,Set(ORIGEXTENSION=$\{EXTEN})\n";
					$conf .= "exten => _$pattern,n,NoOp( - original extension = $\{ORIGEXTENSION})\n";

					// apply caller id override
					if ($cidoverride) {
						$conf .= "exten => _$pattern,n,Set(CALLERID(all)=$cidoverride)\n";
						$conf .= "exten => _$pattern,n,NoOp( - caller id set = $cidoverride)\n";
					} else {
						$conf .= "exten => _$pattern,n,NoOp( - sending phone caller id)\n";
					}

					$conf .= "exten => _$pattern,n,Goto({$provider['uniqid']},$\{EXTEN},1)\n";
				}
			}
		}
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
	if ($p['ringlength'] != "indefinitely") {
		$ringlength = $p['ringlength'];
	}

	$conf .= extensions_generate_call_notification_variables($exten, $p);
	if ($tech == "SIP" || $tech == "IAX2") {
		$conf .= "exten => $exten,n,Macro(main,$tech/$phone_exten,$phone_exten,$readbacknum," . $transfer_flags . "o,$ringlength)\n";

	} else if ($tech == "SKINNY") {
		$conf .= "exten => $exten,n,Macro(main,SKINNY/$phone_exten@$phone_exten,$phone_exten,$readbacknum," . $transfer_flags . "o,$ringlength)\n";

	} else if ($tech == "DAHDI") {
		$gnumber = dahdi_get_gnumber($p['port']);
		$conf .= "exten => $exten,n,Macro(main,DAHDI/g$gnumber,$phone_exten,$readbacknum," . $transfer_flags . "o,$ringlength)\n";

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
	if ($p['ringlength'] != "indefinitely") {
		$ringlength = $p['ringlength'];
	}

	$conf .= extensions_generate_call_notification_variables($exten, $p);
	if ($tech == "DAHDI") {
		$prov = pbx_get_provider($p['dialprovider']);
		$gnumber = dahdi_get_gnumber($prov['port']);
		$conf .= "exten => $exten,n,".
			"Macro(main,DAHDI/g$gnumber/{$p['dialstring']},$phone_exten,$readbacknum," . $transfer_flags . "o,$ringlength)\n";

	} else if (($tech == "SIP" || $tech == "IAX2") && $directdial) {
		$conf .= "exten => $exten,n,".
			"Macro(main,$tech/{$p['dialstring']},$phone_exten,$readbacknum," . $transfer_flags . "o,$ringlength)\n";

	} else if ($tech == "SIP" || $tech == "IAX2") {
		$conf .= "exten => $exten,n,".
			"Macro(main,$tech/{$p['dialprovider']}/{$p['dialstring']},$phone_exten,$readbacknum," . $transfer_flags . "o,$ringlength)\n";

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
	if ($group['ringlength'] != "indefinitely") {
		$ringlength = $group['ringlength'];
	}

	$conf .= extensions_generate_call_notification_variables($exten, $vm_phone);
	$conf .= "exten => $exten,n,Macro(main,$dialstring,{$vm_phone['extension']},$readbacknum," . $transfer_flags . "o,$ringlength)\n";

	return $conf;
}

function extensions_generate_application_context2($a) {

	if ($a['type'] == "php") {

		util_file_write(
			"/etc/asterisk/agi-bin/" . $a['uniqid'] . ".php",
		 	"#!/usr/bin/php -f\n" . base64_decode($a['applicationlogic'])
		);
		chmod("/etc/asterisk/agi-bin/" . $a['uniqid'] . ".php", 0755);

		$conf = "; " . $a['name'] . "\n";
		$conf .= "[" . $a['uniqid'] . "]\n";
		$conf .= "exten => _[0-9a-zA-Z*#]!,1,AGI(" . $a['uniqid'] . ".php)\n";
		$conf .= "exten => _[0-9a-zA-Z*#]!,2,Hangup()\n";
		$conf .= "exten => s,1,AGI(" . $a['uniqid'] . ".php)\n";
		$conf .= "exten => s,2,Hangup()\n";
		$conf .= "exten => h,1,Hangup()\n";

	} else if ($a['type'] == "plaintext") {
		$lines = explode("\n", base64_decode($a['applicationlogic']));

		$conf = "; " . $a['name'] . "\n";
		$conf .= "[" . $a['uniqid'] . "]\n";
		foreach ($lines as $l) {
			if (preg_match('/^include\\s=>\\s[A-Z0-9-_]+/i', $l)) {
				$conf .= $l . "\n";
			} else {
				$conf_ext1 .= "exten => _[0-9a-zA-Z*#]!," . $l . "\n";
				$conf_ext2 .= "exten => s," . $l . "\n";
			}
		}
		$conf .= $conf_ext1;
		$conf .= $conf_ext2;
	}

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

function extensions_generate_virtual_fax_context($f, $from_public=false, $extension=false) {

	$emailconfig = notifications_get_email_configuration();
	$product_name = system_get_product_name();

	$conf = "; " . $f['callerid'] . "\n";
	$conf .= "[" . $f['uniqid'] . "]\n";
	$conf .= "exten => _[0-9a-zA-Z*#]!,1,Ringing()\n";
	$conf .= "exten => _[0-9a-zA-Z*#]!,n,Wait(6)\n";
	$conf .= "exten => _[0-9a-zA-Z*#]!,n,ReceiveFAX(/tmp/fax-$\{UNIQUEID})\n";
	$conf .= "exten => _[0-9a-zA-Z*#]!,n,Hangup()\n";
	$conf .= "exten => s,1,Ringing()\n";
	$conf .= "exten => s,n,Wait(6)\n";
	$conf .= "exten => s,n,ReceiveFAX(/tmp/fax-$\{UNIQUEID})\n";
	$conf .= "exten => s,n,Hangup()\n";
	$conf .= "exten => h,1,GotoIf($[\"$\{FAXSTATUS}\"!=\"SUCCESS\"]?transactionend)\n";
	$conf .= "exten => h,n,Set(FAXFILENAME=virtualfax-" . $f['extension'] . "-$\{STRFTIME($\{EPOCH},,%Y-%m-%d-%H-%M-%S)}.pdf)\n";
	$conf .= "exten => h,n,System(tiff2pdf /tmp/fax-$\{UNIQUEID} -o /tmp/$\{FAXFILENAME})\n";
	$conf .= "exten => h,n,System(touch /tmp/email-$\{UNIQUEID})\n";
	$conf .= "exten => h,n,System(echo \"date: \`date\`\" > /tmp/email-$\{UNIQUEID})\n";
	$conf .= "exten => h,n,System(echo \"to: " . $f['email'] . "\" >> /tmp/email-$\{UNIQUEID})\n";
	$conf .= "exten => h,n,System(echo \"subject: New Fax from \$\{CALLERID(all)}\" >> /tmp/email-$\{UNIQUEID})\n";
	$conf .= "exten => h,n,System(echo \"from: $product_name Fax <{$emailconfig['address']}>\" >> /tmp/email-$\{UNIQUEID})\n";
	$conf .= "exten => h,n,System(echo >> /tmp/email-$\{UNIQUEID})\n";
	$conf .= "exten => h,n,System(echo \"$\{STRFTIME($\{EPOCH},,%Y-%m-%d - %H:%M)} from $\{CALLERID(all)}\" >> /tmp/email-$\{UNIQUEID})\n";
	$conf .= "exten => h,n,System(uuencode /tmp/$\{FAXFILENAME} $\{FAXFILENAME} >> /tmp/email-$\{UNIQUEID})\n";
	$conf .= "exten => h,n,System(/usr/bin/msmtp -C /etc/msmtp.conf -t < /tmp/email-$\{UNIQUEID})\n";
	if ($disk = storage_service_is_active("faxarchive")) {
		$archivepath = $disk['mountpoint'] . "/askoziapbx/faxarchive/";
		$conf .= "exten => h,n,System(cp /tmp/$\{FAXFILENAME} $archivepath)\n";
	}
	$conf .= "exten => h,n,System(rm /tmp/$\{FAXFILENAME})\n";
	$conf .= "exten => h,n,System(rm /tmp/fax-$\{UNIQUEID})\n";
	$conf .= "exten => h,n,System(rm /tmp/email-$\{UNIQUEID})\n";
	$conf .= "exten => h,n(transactionend),NoOp(Receive fax terminated returning status: $\{FAXSTATUS})\n";
	$conf .= "\n";

	return $conf;
}

function extensions_generate_virtual_fax_extension($f, $from_public=false, $extension=false) {

	$internal_number = $f['extension'];
	$exten = ($extension !== false) ? $extension : $internal_number;

	return "exten => $exten,n,Dial(Local/$\{EXTEN}@" . $f['uniqid'] . "/n)\n";
}

function extensions_generate_analog_fax_extension($f, $from_public=false, $extension=false) {

	$internal_number = $f['extension'];
	$exten = ($extension !== false) ? $extension : $internal_number;
	$dahdi_port = dahdi_get_port($f['port']);

	return "exten => $exten,n,Dial(DAHDI/g" . $dahdi_port['basechannel'] . ")\n";
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

	// virtual faxes
	} else if ($id[0] == "VIRTUAL" && $id[1] == "FAX") {
		$f = virtual_get_fax($uniqid);
		return extensions_generate_virtual_fax_extension($f, $from_public, $extension);

	// analog faxes
	} else if ($id[0] == "ANALOG" && $id[1] == "FAX") {
		$f = analog_get_fax($uniqid);
		return extensions_generate_analog_fax_extension($f, $from_public, $extension);

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
	} else if ($id[0] == "DIALPLAN" && $id[1] == "APPLICATION") {
		$a = applications_get_application($uniqid);
		return extensions_generate_application_extension($a, $from_public, $extension);

	} else {
		return "ERROR: unmatched case in extensions_generate_extension_from_uniqid!";
	}
}

function extensions_patternify($extension) {
	return (!preg_match("/^[0-9]+$/", $extension)) ? "_" . $extension : $extension;
}

?>
