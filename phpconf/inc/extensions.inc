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
	global $config, $g;
	
	$fd = fopen("/usr/local/etc/asterisk/extensions.conf", "w");
	if (!$fd) {
		printf("Error: cannot open extensions.conf in extensions_conf_generate().\n");
		return 1;
	}
	
	$conf = <<<EOD

[macro-vm];
exten => s,1,Dial($\{ARG1},30,tT)
exten => s,n,Goto(s-$\{DIALSTATUS},1)
exten => s-NOANSWER,1,VoiceMail($\{ARG2},u)
exten => s-NOANSWER,n,Goto(exit-$\{VMSTATUS},1)
exten => s-BUSY,1,VoiceMail($\{ARG2},b)
exten => s-BUSY,n,Goto(exit-$\{VMSTATUS},1)
exten => s-ANSWER,1,Hangup()
exten => _s-.,1,Goto(s-NOANSWER,1)
exten => exit-FAILED,1,Answer()
exten => exit-FAILED,2,Playback(vm-theperson)
exten => exit-FAILED,3,SayDigits($\{ARG2})
exten => exit-FAILED,4,Playback(vm-isunavail)
exten => exit-FAILED,5,Hangup()
exten => exit-SUCCESS,1,Playback(vm-goodbye)
exten => exit-SUCCESS,n,Hangup()
exten => exit-USEREXIT,1,Playback(vm-goodbye)
exten => exit-USEREXIT,n,Hangup()


EOD;

	// internal extensions
	$conf .= "[internal]\n";

	$a_sipphones = sip_get_phones();
	foreach ($a_sipphones as $phone) {
		$conf .= "exten => {$phone['extension']},1,Macro(vm|SIP/$\{EXTEN}|$\{EXTEN})\n";
	}
	
	$a_iaxphones = iax_get_phones();
	foreach ($a_iaxphones as $phone) {
		$conf .= "exten => {$phone['extension']},1,Macro(vm|IAX2/$\{EXTEN}|$\{EXTEN})\n";
	}
	
	$a_isdnphones = isdn_get_phones();
	foreach ($a_isdnphones as $phone) {
		$conf .= "exten => {$phone['extension']},1,Macro(vm|CAPI/$\{EXTEN}|$\{EXTEN})\n";
	}
	
	$a_extphones = external_get_phones();
	foreach ($a_extphones as $phone) {
		$technology = asterisk_uniqid_to_technology($phone['dialprovider']);
		$conf .= "exten => {$phone['extension']},1,".
			"Macro(vm|$technology/{$phone['dialprovider']}/{$phone['dialstring']}|{$phone['extension']})\n";
	}
	
	$a_callgroups = dialplan_get_callgroups();
	foreach ($a_callgroups as $group) {
		$conf .= asterisk_uniqid_to_dial($group['extension'], $group['uniqid']);
	}
	
	$conf .= <<<EOD
exten => 00003246,1,Answer()
exten => 00003246,2,Echo()
exten => 00003246,3,Hangup()

exten => 000064554,1,Answer()
exten => 000064554,2,Milliwatt()
exten => 000064554,3,Hangup()

include => parkedcalls


EOD;
	
	// conference rooms
	$rooms = conferencing_get_rooms();
	
	foreach ($rooms as $room) {
		$conf .= "exten => {$room['number']},1,Answer()\n";
		$conf .= "exten => {$room['number']},2,MeetMe({$room['number']},M)\n";
		$conf .= "exten => {$room['number']},3,Hangup()\n\n";
	}

	$conf .= "\n\n";

	// sip providers
	$a_sipproviders = sip_get_providers();

	foreach ($a_sipproviders as $p) {
		$conf .= "; {$p['name']}\n";
		$conf .= "[{$p['uniqid']}]\n";
		// FIXME give some options for this ... kind of hacky
		$conf .= asterisk_uniqid_to_dial($p['username'], $p['incomingextension'], false, $p['override']);
		$conf .= asterisk_uniqid_to_dial("s", $p['incomingextension'], false, $p['override']);
		
		if (is_array($p['dialpattern'])) {
			foreach($p['dialpattern'] as $pattern) {
				$prefix = $offset = "";
				$exten = "$\{EXTEN}";
				// prefix insertion
				if (($offset = strpos($pattern, "+")) !== false) {
					$prefix = substr($pattern, 0, $offset);
					$pattern = substr($pattern, $offset+1);
				}
				// prefix deletion
				if (($offset = strpos($pattern, "|")) !== false) {
					$pattern = str_replace("|", "", $pattern);
					$exten = "$\{EXTEN:$offset}";
				}
				
				$conf .= "exten => _$pattern,1,Dial(SIP/$prefix$exten@{$p['uniqid']},30)\n";
				$conf .= "exten => _$pattern,2,Hangup()\n";
			}
		}
		$conf .= "\n";
	}
	
	// iax providers
	$a_iaxproviders = iax_get_providers();

	foreach ($a_iaxproviders as $p) {
		$conf .= "; {$p['name']}\n";
		$conf .= "[{$p['uniqid']}]\n";
		$conf .= asterisk_uniqid_to_dial($p['username'], $p['incomingextension'], false, $p['override']);
		$conf .= asterisk_uniqid_to_dial("s", $p['incomingextension'], false, $p['override']);

		if (is_array($p['dialpattern'])) {
			foreach($p['dialpattern'] as $pattern) {
				$prefix = $offset = "";
				$exten = "$\{EXTEN}";
				// prefix insertion
				if (($offset = strpos($pattern, "+")) !== false) {
					$prefix = substr($pattern, 0, $offset);
					$pattern = substr($pattern, $offset+1);
				}
				// prefix deletion
				if (($offset = strpos($pattern, "|")) !== false) {
					$pattern = str_replace("|", "", $pattern);
					$exten = "$\{EXTEN:$offset}";
				}
				
				$conf .= "exten => _$pattern,1,Dial(IAX2/{$p['uniqid']}/$prefix$exten,30)\n";
				$conf .= "exten => _$pattern,2,Hangup()\n";
			}
		}
		$conf .= "\n";
	}
	
	// isdn providers
	$a_isdnproviders = isdn_get_providers();

	foreach ($a_isdnproviders as $p) {
		$conf .= "; {$p['name']}\n";
		$conf .= "[{$p['uniqid']}]\n";
		$conf .= asterisk_uniqid_to_dial($p['msn'], $p['incomingextension'], 1, $p['override']);
		$conf .= asterisk_uniqid_to_dial("s", $p['incomingextension'], 1, $p['override']);

		if (is_array($p['dialpattern'])) {
			foreach($p['dialpattern'] as $pattern) {
				$prefix = $offset = "";
				$exten = "$\{EXTEN}";
				// prefix insertion
				if (($offset = strpos($pattern, "+")) !== false) {
					$prefix = substr($pattern, 0, $offset);
					$pattern = substr($pattern, $offset+1);
				}
				// prefix deletion
				if (($offset = strpos($pattern, "|")) !== false) {
					$pattern = str_replace("|", "", $pattern);
					$exten = "$\{EXTEN:$offset}";
				}
				
				$conf .= "exten => _$pattern,1,Set(CALLERID(number)={$p['msn']})\n";
				$conf .= "exten => _$pattern,2,Dial(CAPI/{$p['uniqid']}/$prefix$exten/bl,30)\n";
				$conf .= "exten => _$pattern,3,Hangup()\n";
			}
		}
		$conf .= "\n";
	}
	

	// sip phones
	foreach ($a_sipphones as $p) {
		$conf .= "; {$p['callerid']}\n";
		$conf .= "[{$p['uniqid']}]\n";
		$conf .= "include => internal\n";
		if (isset($p['provider'])) {
			foreach ($p['provider'] as $provider) {
				$conf .= "include => $provider\n";
			}
		}
		$conf .= "exten => {$p['extension']},hint,SIP/{$p['extension']}\n";
		$conf .= "\n";
	}
	
	// iax phones
	foreach ($a_iaxphones as $p) {
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
	
	// isdn phones
	foreach ($a_isdnphones as $p) {
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


function extensions_reload() {
	
	return asterisk_exec("dialplan reload");

}