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

function external_get_phones() {
	global $config;
	
	if (!is_array($config['external']['phone']))
		$config['external']['phone'] = array();
		
	external_sort_phones();
		
	return $config['external']['phone'];
}

function external_get_phone($uniqid) {
	global $config, $uniqid_map;
	return $config['external']['phone'][$uniqid_map[$uniqid]];
}

function external_sort_phones() {
	global $g, $config;

	usort($config['external']['phone'], "_extphonecmp");	
}
function _extphonecmp($a, $b) {
    if ($a['extension'] == $b['extension']) {
        return 0;
    }
    return ($a['extension'] < $b['extension']) ? -1 : 1;
}

function external_get_extensions() {
	global $config;
	
	$extensions = array();
	if (is_array($config['external']['phones']))
		foreach($config['external']['phones'] as $phone)
			$extensions[] = $phone['extension'];

	return $extensions;
}


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
exten => s,1,Dial($\{ARG1},30,$\{ARG4})
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

exten => h,1,GosubIf($["$\{MISSEDCAUSE}" = ""]?reallyhungup)
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
		$conf .= _extensions_add_sendnotifications_line($phone);
		$conf .= "exten => {$phone['extension']},n,Macro(vm|SIP/$\{EXTEN}|$\{EXTEN}|$\{EXTEN}|tTo)\n";
	}
	
	$a_iaxphones = iax_get_phones();
	foreach ($a_iaxphones as $phone) {
		$conf .= "exten => {$phone['extension']},1,NoOp()\n";
		$conf .= _extensions_add_sendnotifications_line($phone);
		$conf .= "exten => {$phone['extension']},n,Macro(vm|IAX2/$\{EXTEN}|$\{EXTEN}|$\{EXTEN}|tTo)\n";
	}
	
	$a_isdnphones = isdn_get_phones();
	foreach ($a_isdnphones as $phone) {
		$conf .= "exten => {$phone['extension']},1,NoOp()\n";
		$conf .= _extensions_add_sendnotifications_line($phone);
		$conf .= "exten => {$phone['extension']},n,Macro(vm|CAPI/$\{EXTEN}|$\{EXTEN}|$\{EXTEN}|tTo)\n";
	}
	
	$a_analogphones = analog_get_phones();
	foreach ($a_analogphones as $phone) {
		$conf .= "exten => {$phone['extension']},1,NoOp()\n";
		$conf .= _extensions_add_sendnotifications_line($phone);
		$conf .= "exten => {$phone['extension']},n,Macro(vm|ZAP/{$phone['interface']}/$\{EXTEN}|$\{EXTEN}|$\{EXTEN}|tTo)\n";
	}
	
	$a_extphones = external_get_phones();
	foreach ($a_extphones as $phone) {
		$technology = asterisk_uniqid_to_technology($phone['dialprovider']);
		$conf .= "exten => {$phone['extension']},1,NoOp()\n";
		$conf .= _extensions_add_sendnotifications_line($phone);
		if ($technology == "ZAP") {
			$zprov = analog_get_provider($phone['dialprovider']);
			$conf .= "exten => {$phone['extension']},n,".
				"Macro(vm|$technology/{$zprov['interface']}/{$phone['dialstring']}|{$phone['extension']}|{$phone['extension']}|tTo)\n";
		} else {
			$conf .= "exten => {$phone['extension']},n,".
				"Macro(vm|$technology/{$phone['dialprovider']}/{$phone['dialstring']}|{$phone['extension']}|{$phone['extension']}|to)\n";	
		}
	}
	
	$a_callgroups = dialplan_get_callgroups();
	foreach ($a_callgroups as $group) {
		if (!$group['extension']) {
			continue;
		}
		$conf .= asterisk_uniqid_to_dial($group['extension'], $group['uniqid']);
	}
	
	$a_applications = dialplan_get_applications();
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

	$conf .= _extensions_generate_parkinglot();

	// sip providers
	$a_sipproviders = sip_get_providers();
	foreach ($a_sipproviders as $p) {
		if ($p['incomingextension']) {
			$conf .= "; {$p['name']} incoming\n";
			$conf .= "[{$p['uniqid']}-incoming]\n";
			$conf .= asterisk_uniqid_to_dial("_X!", $p['incomingextension'], false, $p['override']);
			$conf .= asterisk_uniqid_to_dial("s", $p['incomingextension'], false, $p['override']);
			$conf .= "\n";
		}
		$conf .= "; {$p['name']} outgoing\n";
		$conf .= "[{$p['uniqid']}]\n";
		if (is_array($p['dialpattern'])) {
			$cidflag = ($p['calleridsource'] == "string") ? "f" : "o";
			foreach($p['dialpattern'] as $pattern) {
				dialplan_parse_pattern(&$pattern, &$prefix, &$offset, &$exten);
				$conf .= "exten => _$pattern,1,Dial(SIP/$prefix$exten@{$p['uniqid']},30,T$cidflag)\n";
				$conf .= "exten => _$pattern,2,Hangup()\n";
			}
		}
		$conf .= "\n";
	}
	
	// iax providers
	$a_iaxproviders = iax_get_providers();
	foreach ($a_iaxproviders as $p) {
		if ($p['incomingextension']) {
			$conf .= "; {$p['name']} incoming\n";
			$conf .= "[{$p['uniqid']}-incoming]\n";
			$conf .= asterisk_uniqid_to_dial("_X!", $p['incomingextension'], false, $p['override']);
			$conf .= asterisk_uniqid_to_dial("s", $p['incomingextension'], false, $p['override']);
			$conf .= "\n";
		}
		$conf .= "; {$p['name']} outgoing\n";
		$conf .= "[{$p['uniqid']}]\n";
		if (is_array($p['dialpattern'])) {
			$cidflag = ($p['calleridsource'] == "string") ? "f" : "o";
			foreach($p['dialpattern'] as $pattern) {
				dialplan_parse_pattern(&$pattern, &$prefix, &$offset, &$exten);
				$conf .= "exten => _$pattern,1,Dial(IAX2/{$p['uniqid']}/$prefix$exten,30,T$cidflag)\n";
				$conf .= "exten => _$pattern,2,Hangup()\n";
			}
		}
		$conf .= "\n";
	}
	
	// isdn providers
	$a_isdnproviders = isdn_get_providers();
	foreach ($a_isdnproviders as $p) {
		if ($p['incomingextension']) {
			$conf .= "; {$p['name']} incoming\n";
			$conf .= "[{$p['uniqid']}-incoming]\n";
			$conf .= asterisk_uniqid_to_dial("_X!", $p['incomingextension'], 1, $p['override'], $p['msn']);
			$conf .= asterisk_uniqid_to_dial("s", $p['incomingextension'], 1, $p['override'], $p['msn']);
			$conf .= "\n";
		}
		$conf .= "; {$p['name']} outgoing\n";
		$conf .= "[{$p['uniqid']}]\n";
		if (is_array($p['dialpattern'])) {
			foreach($p['dialpattern'] as $pattern) {
				dialplan_parse_pattern(&$pattern, &$prefix, &$offset, &$exten);
				$conf .= "exten => _$pattern,1,Set(CALLERID(number)={$p['msn']})\n";
				$conf .= "exten => _$pattern,2,Dial(CAPI/{$p['uniqid']}/$prefix$exten/bl,30,Tf)\n";
				$conf .= "exten => _$pattern,3,Hangup()\n";
			}
		}
		$conf .= "\n";
	}
	
	// analog providers
	$a_analogproviders = analog_get_providers();
	foreach ($a_analogproviders as $p) {
		if ($p['incomingextension']) {
			$conf .= "; {$p['name']} incoming\n";
			$conf .= "[{$p['uniqid']}-incoming]\n";
			$conf .= asterisk_uniqid_to_dial("_X!", $p['incomingextension'], 1, $p['override'], $p['number']);
			$conf .= asterisk_uniqid_to_dial("s", $p['incomingextension'], 1, $p['override'], $p['number']);
			$conf .= "\n";
		}
		$conf .= "; {$p['name']} outgoing\n";
		$conf .= "[{$p['uniqid']}]\n";
		if (is_array($p['dialpattern'])) {
			foreach($p['dialpattern'] as $pattern) {
				dialplan_parse_pattern(&$pattern, &$prefix, &$offset, &$exten);
				$conf .= "exten => _$pattern,1,Set(CALLERID(number)={$p['number']})\n";
				$conf .= "exten => _$pattern,2,Dial(ZAP/{$p['interface']}/$prefix$exten,30,Tf)\n";
				$conf .= "exten => _$pattern,3,Hangup()\n";
			}
		}
		$conf .= "\n";
	}
	

	// internal phones
	$intphones = asterisk_get_phones();
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
	
	fwrite($fd, $conf);
	fclose($fd);
	
	return 0;
}

function _extensions_add_sendnotifications_line($phone) {

	if ($phone['voicemailbox'] && isset($phone['sendcallnotifications'])) {
		$dial = "exten => {$phone['extension']},n,Set(SENDNOTIFICATIONS=\"{$phone['voicemailbox']}\")\n";
	} else {
		$dial = "exten => {$phone['extension']},n,Set(SENDNOTIFICATIONS=\"no\")\n";
	}

	return $dial;
}

function _extensions_generate_parkinglot() {
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


function extensions_reload() {
	return asterisk_exec("dialplan reload");
}
