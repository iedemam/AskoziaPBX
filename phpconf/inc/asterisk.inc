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

/* include all configuration functions */
require_once("functions.inc");


function asterisk_configure() {
	global $config, $g;

	if ($g['booting']) {
		echo "Starting Asterisk...\n";
		echo " - loading zaptel\n";
		mwexec("/sbin/kldload zaptel");
		echo " - loading ztdummy\n";
		mwexec("/sbin/kldload ztdummy");
		//echo " - loading isdn4bsd\n";
		//mwexec("/sbin/kldload i4b");
	} else
		killbypid("{$g['varrun_path']}/asterisk.pid");

	
	$res = 0;
	if ($g['booting']) echo " - generating sip configuration...";
	$res |= sip_conf_generate();
	if ($g['booting']) echo "done\n";
	
	if ($g['booting']) echo " - generating iax configuration...";
	$res |= iax_conf_generate();
	if ($g['booting']) echo "done\n";
	
	if ($g['booting']) echo " - generating conferencing configuration...";
	$res |= conferencing_conf_generate();
	if ($g['booting']) echo "done\n";
	
	if ($g['booting']) echo " - generating dialplan...";
	$res |= extensions_conf_generate();
	if ($g['booting']) echo "done\n";
	
	if ($g['booting']) echo " - generating voicemail configuration...";
	$res |= voicemail_conf_generate();
	$res |= msmtp_conf_generate();
	if ($g['booting']) echo "done\n";
	
	if ($g['booting']) echo " - executing Asterisk...";
	$res |= mwexec("/usr/local/sbin/asterisk");
	if ($g['booting']) echo "done\n";

	if ($g['booting']) {
		if ($res == 0)
			echo "done\n";
		else
			echo "failed\n";
	}

	return $res;
}


function asterisk_exec($cmd, $output=NULL) {

	$token = md5(uniqid(rand()));
	$errno = 0;
	$errstr = 0;
	$fp = fsockopen("localhost", 5038, &$errno, &$errstr, 20);
	if (!$fp) {
	  return 1;
	}
	
	fputs($fp, "Action: login\r\n");
	fputs($fp, "Username: admin\r\n");
	fputs($fp, "Secret: askozia\r\n");
	fputs($fp, "Events: off\r\n\r\n");
	usleep(500);
	
	fputs($fp, "Action: COMMAND\r\n");
	fputs($fp, "command: $cmd\r\n");
	fputs($fp, "ActionID: $token\r\n\r\n");
	usleep(500);
	
	$out = fread($fp, 38000); 
	while(strpos($out,"--END COMMAND--")==0) {
		$out .= fread($fp, 38000); 	  
	}
	fclose ($fp);
	
	$out = substr($out, strpos($out, "ActionID"));
	$out = substr($out, strpos($out, "\n") + 1);
	$out = substr($out, 0, strpos($out, "--END COMMAND--") - 1);

	// better command failure checking needed
	$output = $out;

	return 0; // this needs to return a value for SUCCESS/FAILURE
}


function asterisk_is_valid_extension($extension) {
	if(!is_numericint($extension) || (strlen($extension) > 4) || (strlen($extension) < 1))
		return false;
	
	return true;
}


function asterisk_is_valid_callerid($id) {
	return true;
}


function asterisk_is_valid_secret($secret) {
	return true;
}

function asterisk_is_valid_username($username) {
	return true;
}

function asterisk_dialpattern_exists($dialpattern, &$return_provider_name, $current_provider_id=false) {
	$allpatterns = asterisk_get_dialpatterns();

	if (is_array($allpatterns)) {
		foreach ($allpatterns as $provider_id => $patterns) {
			if ($provider_id == $current_provider_id) {
				continue;
			}
			if (in_array($dialpattern, $patterns)) {
				$return_provider_name = asterisk_uniqid_to_name($provider_id);
				return true;
			}
		}
	}
	
	return false;
}

function asterisk_is_valid_dialpattern($dialpattern, &$error) {
	
	// invalid character
	if (!preg_match("/^[XNZ0-9\-\[\]\.\|\+]+$/", $dialpattern)) {
		$error = "It contains an invalid character";
		return false;
	}
	
	// doubled character
	if (substr_count($dialpattern, "|") > 1) {
		$error = "It contains an doubled \"|\" character, only one is allowed.";
		return false;
	}
	if (substr_count($dialpattern, "+") > 1) {
		$error = "It contains an doubled \"+\" character, only one is allowed.";
		return false;
	}
	if (substr_count($dialpattern, "-") > 1) {
		$error = "It contains an doubled \"-\" character, only one is allowed.";
		return false;
	}
	
	// bracketing
	$len = strlen($dialpattern);
	$open = false;
	$open_index = 0;
	for($i = 0; $i < $len; $i++) {
		if ($dialpattern[$i] == "[") {
			if ($open) {
				$error = "Nested square brackets are not allowed.";
				return false;
			} else {
				$open = true;
				$open_index = $i;
			}
		} else if ($dialpattern[$i] == "]") {
			if (!$open) {
				$error = "Unmatched square bracket.";
				return false;
			} else if (($i - $open_index) < 3) {
				$error = "Brackets are not needed for less than 2 characters.";
				return false;
			} else {
				$open = false;
			}
		}
	}
	
	// invalid characters in brackets
	/* FIXME not working...
	if (preg_match("/\[[XNZ\.\|\+]+\]/", $dialpattern)) {
		$error = "An invalid character is contained within square brackets, only digits and hyphens are allowed.";
		return false;
	}
	*/
	
	// invalid appended prefix
	if (($plus_offset = strpos($dialpattern, "+")) !== false) {
		if (!preg_match("/^[0-9]+\+/", $dialpattern)) {
			$error = "Appended prefixes may only contain digits.";
			return false;
		}
		$dialpattern = substr($dialpattern, $plus_offset+1);
	}
	
	// invalid chopped prefix
	if (($pipe_offset = strpos($dialpattern, "|")) !== false) {
		if (strpos(substr($dialpattern, 0, $pipe_offset), ".") !== false) {
			$error = "The wildcard character \".\" is not allowed before a prefix which is to be removed.";
			return false;
		}
	}

	return true;
}

function asterisk_get_extensions() {
	global $config;

	return array_merge(
		sip_get_extensions(), 
		iax_get_extensions(),
		conferencing_get_extensions()
	);
}

// providers
function asterisk_get_providers() {
	global $config;
	
	return asterisk_sort_providers(
				array_merge(
					sip_get_providers(),
					iax_get_providers()
				));
}
function asterisk_sort_providers($providers) {
	usort($providers, "_a_sortproviders");
	return $providers;
}
function _a_sortproviders($a, $b) {
	return strcmp($a['name'], $b['name']);
}

// phones
function asterisk_get_phones() {
	global $config;
	
	return asterisk_sort_phones(
				array_merge(
					sip_get_phones(),
					iax_get_phones()
				));
}
function asterisk_sort_phones($phones) {
	usort($phones, "_a_sortphones");
	return $phones;
}
function _a_sortphones($a, $b) {
	return strcmp($a['callerid'], $b['callerid']);
}

// dialpatterns
function asterisk_get_dialpatterns() {
	global $config;

	return array_merge(
		sip_get_dialpatterns(),
		iax_get_dialpatterns()
	);
}

function asterisk_get_active_calls(&$active_calls, &$active_channels, &$channel_list) {
	
	asterisk_exec("core show channels", &$output);
	$lines = explode("\n", $output);

	$n = count($lines) - 3;
	for ($i = 1; $i <= $n; $i++) {
		$channel_list[] = $lines[$i];
	}
	$active_channels = substr($lines[$n+1], 0, strpos($lines[$n+1], " "));
	$active_calls = substr($lines[$n+2], 0, strpos($lines[$n+2], " "));

	return 0; // XXX could do some error checking here
}

function asterisk_uniqid_to_dial($exten, $uniqid) {
	global $config;
	
	$id = split("-", $uniqid);
	
	if ($id[0] == "SIP" && $id[1] == "PHONE") {
		$phone = sip_get_phone($uniqid);
		$dial = "exten => $exten,1,".
			"Macro(vm|SIP/{$phone['extension']}|{$phone['extension']})\n";
			
	} else if ($id[0] == "IAX" && $id[1] == "PHONE") {
		$phone = iax_get_phone($uniqid);
		$dial = "exten => $exten,1,".
			"Macro(vm|IAX2/{$phone['extension']}/{$phone['extension']}|{$phone['extension']})\n";

	} else if ($id[0] == "CONFERENCE" && $id[1] == "ROOM") {
		$room = conferencing_get_room($uniqid);
		$dial = "exten => $exten,1,Answer()\n";
		$dial .= "exten => $exten,2,MeetMe({$room['number']},M)\n";
		$dial .= "exten => $exten,3,Hangup()\n";

	} else if ($id[0] == "CALLGROUP" && $id[1] == "PARALLEL") {
		$group = dialplan_get_callgroup($uniqid);
		$dial_strings = array();
		$vm_account = "";
		$n = count($group['groupmember']);
		for ($i = 0; $i < $n; $i++) {
			$subid = split("-", $group['groupmember'][$i]);
			if ($subid[0] == "SIP" && $subid[1] == "PHONE") {
				$phone = sip_get_phone($group['groupmember'][$i]);
				if ($i == 0) {
					$vm_account = $phone['extension'];
				}
				$dial_strings[] = "SIP/{$phone['extension']}";
        	
			} else if ($subid[0] == "IAX" && $subid[1] == "PHONE") {
				$phone = iax_get_phone($group['groupmember'][$i]);
				if ($i == 0) {
					$vm_account = $phone['extension'];
				}
				$dial_strings[] = "IAX2/{$phone['extension']}";        	
			}
		}
		
		$n = count($dial_strings);
		$dstring = $dial_strings[0];
		for ($i = 1; $i < $n; $i++) {
			$dstring .= "&" . $dial_strings[$i];
		}
		
		$dial = "exten => $exten,1,".
			"Macro(vm|$dstring|$vm_account)\n";
	}
	
	return $dial;
}

function asterisk_uniqid_to_name($uniqid) {
	global $config, $uniqid_map;
	
	// sip phone
	if (strpos($uniqid, "SIP-PHONE") !== false) {
		return $config['sip']['phone'][$uniqid_map[$uniqid]]['callerid'];

	// iax phone
	} else if (strpos($uniqid, "IAX-PHONE") !== false) {
		return $config['iax']['phone'][$uniqid_map[$uniqid]]['callerid'];

	// sip provider
	} else if (strpos($uniqid, "SIP-PROVIDER") !== false) {
		return $config['sip']['provider'][$uniqid_map[$uniqid]]['name'];
		
	// sip provider
	} else if (strpos($uniqid, "IAX-PROVIDER") !== false) {
		return $config['iax']['provider'][$uniqid_map[$uniqid]]['name'];
	}
}

?>
