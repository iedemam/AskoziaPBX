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

/* include all configuration functions */
require_once("functions.inc");

/**
 * Generates all Asterisk configuration files and (re)starts the Asterisk process
 */
function pbx_configure() {
	global $config, $g;

	if (!$g['booting']) {
		pbx_stop();
	}

	$res = 0;
	if ($g['booting']) echo "   |- generating Asterisk process configuration...";
	$res |= asterisk_conf_generate();
	if ($g['booting']) echo "done\n";

	if ($g['booting']) echo "   |- generating modules configuration...";
	$res |= modules_conf_generate();
	if ($g['booting']) echo "done\n";

	if ($g['booting']) echo "   |- generating indications configuration...";
	$res |= indications_conf_generate();
	if ($g['booting']) echo "done\n";

	if ($g['booting']) echo "   |- generating manager configuration...";
	$res |= manager_conf_generate();
	if ($g['booting']) echo "done\n";

	if ($g['booting']) echo "   |- generating rtp configuration...";
	$res |= rtp_conf_generate();
	if ($g['booting']) echo "done\n";

	if ($g['booting']) echo "   |- generating sip configuration...";
	$res |= sip_conf_generate();
	if ($g['booting']) echo "done\n";

	if ($g['booting']) echo "   |- generating iax configuration...";
	$res |= iax_conf_generate();
	if ($g['booting']) echo "done\n";

	if ($g['booting']) echo "   |- generating skinny configuration...";
	$res |= skinny_conf_generate();
	if ($g['booting']) echo "done\n";

	if ($g['booting']) echo "   |- generating conferencing configuration...";
	$res |= meetme_conf_generate();
	if ($g['booting']) echo "done\n";

	if ($g['booting']) echo "   |- generating dialplan...";
	$res |= extensions_conf_generate();
	$res |= features_conf_generate();
	if ($g['booting']) echo "done\n";

	if ($g['booting']) echo "   |- generating voicemail configuration...";
	$res |= voicemail_conf_generate();
	$res |= notifications_msmtp_conf_generate();
	if ($g['booting']) echo "done\n";

	if ($g['booting']) echo "   |- generating DAHDI configuration...";
	$res |= dahdi_configure();
	if ($g['booting']) echo "done\n";

	if ($g['booting']) echo "   |- executing Asterisk...";
	$res |= pbx_start();
	if ($g['booting']) echo "done\n";

	return $res;
}

function pbx_start() {
	return mwexec_bg("/usr/sbin/asterisk");
}

function pbx_stop() {
	killbyname("astcanary");
	killbyname("asterisk");
}

function pbx_restart() {
	$res = 0;
	$res |= pbx_stop();
	$res |= pbx_start();
	return $res;
}

/**
 * Executes a command using the AMI
 */
function pbx_exec($cmd, $output=NULL) {

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

	// XXX : better command failure checking needed
	$output = $out;

	return 0; // XXX : this needs to return a value for SUCCESS/FAILURE
}

/**
 * Determines the validity of a given internal extension
 */
function pbx_is_valid_extension($extension) {
	if(!verify_is_numericint($extension) || (strlen($extension) < 1))
		return false;
	
	return true;
}

/**
 * Determines the validity of a given Caller ID
 * XXX : unimplemented
 */
function pbx_is_valid_callerid($id) {

	if(preg_match("/^[a-z0-9]+(\s{1}[a-z0-9]|[a-z0-9])+$/i", $id)) {
		return true;
	} else {
		return false;
	}
}

/**
 * Determines the validity of a given Caller ID string
 * XXX : unimplemented
 */
function pbx_is_valid_callerid_string($string) {

	if(preg_match("/^[a-z0-9]+(\s{1}[a-z0-9]|[a-z0-9])+\s{1}<{1}([0-9]+(\s{1}[0-9]|[0-9])+)+>$/i", $string)) {
		return true;
	} else {
		return false;
	}

}

/**
 * Determines the validity of a given secret
 * XXX : unimplemented
 */
function pbx_is_valid_secret($secret) {
	return true;
}

/**
 * Determines the validity of a given username
 * XXX : unimplemented
 */
function pbx_is_valid_username($username) {
	return true;
}

/**
 * Checks is the supplied dialpattern already exists.
 */
function pbx_dialpattern_exists($dialpattern, &$return_provider_name, $sending_provider_id=false) {
	$allpatterns = pbx_get_dialpatterns();

	if (is_array($allpatterns)) {
		foreach ($allpatterns as $provider_id => $patterns) {
			if ($provider_id == $sending_provider_id) {
				continue;
			}
			if (is_array($patterns) && in_array($dialpattern, $patterns)) {
				$return_provider_name = pbx_uniqid_to_name($provider_id);
				return true;
			}
		}
	}
	
	return false;
}

/**
 * Determines the validity of a given dialpattern
 */
function pbx_is_valid_dialpattern($dialpattern, &$error, $incoming=false) {
	
	$incoming_regex = "/^[XNZ0-9\-\[\]\.\!]+$/";
	$outgoing_regex = "/^[XNZ0-9\-\[\]\.\!\|\+\*\#]+$/";
	
	$invalid_regex = $incoming ? $incoming_regex : $outgoing_regex;
	
	// invalid character
	if (!preg_match($invalid_regex, $dialpattern)) {
		$error = gettext("It contains an invalid character");
		return false;
	}
	
	// doubled character
	if (substr_count($dialpattern, "|") > 1) {
		$error = gettext("It contains an doubled \"|\" character, only one is allowed.");
		return false;
	}
	if (substr_count($dialpattern, "+") > 1) {
		$error = gettext("It contains an doubled \"+\" character, only one is allowed.");
		return false;
	}
	if (substr_count($dialpattern, "-") > 1) {
		$error = gettext("It contains an doubled \"-\" character, only one is allowed.");
		return false;
	}
	
	// bracketing
	$len = strlen($dialpattern);
	$open = false;
	$open_index = 0;
	for($i = 0; $i < $len; $i++) {
		if ($dialpattern[$i] == "[") {
			if ($open) {
				$error = gettext("Nested square brackets are not allowed.");
				return false;
			} else {
				$open = true;
				$open_index = $i;
			}
		} else if ($dialpattern[$i] == "]") {
			if (!$open) {
				$error = gettext("Unmatched square bracket.");
				return false;
			} else if (($i - $open_index) < 3) {
				$error = gettext("Brackets are not needed for less than 2 characters.");
				return false;
			} else {
				$open = false;
			}
		}
	}
	
	// invalid characters in brackets
	/* XXX : not working...
	if (preg_match("/\[[XNZ\.\|\+]+\]/", $dialpattern)) {
		$error = "An invalid character is contained within square brackets, only digits and hyphens are allowed.";
		return false;
	}
	*/
	
	// invalid appended prefix
	// XXX : incorrectly identifies "1|0+X!" as an invalid pattern
	if (($plus_offset = strpos($dialpattern, "+")) !== false) {
		if (!preg_match("/^[0-9\*\#]+\+/", $dialpattern)) {
			$error = gettext("Appended prefixes may only contain digits, '*' and '#'.");
			return false;
		}
		$dialpattern = substr($dialpattern, $plus_offset+1);
	}
	
	// invalid chopped prefix
	if (($pipe_offset = strpos($dialpattern, "|")) !== false) {
		if (strpos(substr($dialpattern, 0, $pipe_offset), ".") !== false) {
			$error = gettext("The wildcard character \".\" is not allowed before a prefix which is to be removed.");
			return false;
		}
		if (strpos(substr($dialpattern, 0, $pipe_offset), "!") !== false) {
			$error = gettext("The wildcard character \"!\" is not allowed before a prefix which is to be removed.");
			return false;
		}
	}

	return true;
}

/**
 * Merges all *get_extensions() into a single array
 * XXX : should be reimplemented to have an optional "technology" parameter
 * so each of these sub _get_ functions can be generally implemented here
 */
function pbx_get_extensions() {
	global $config;

	return array_merge(
		sip_get_extensions(), 
		iax_get_extensions(),
		isdn_get_extensions(),
		analog_get_extensions(),
		skinny_get_extensions(),
		conferencing_get_extensions(),
		external_get_extensions(),
		callgroups_get_extensions(),
		applications_get_extensions()
	);
}

function pbx_get_next_default_phone_extension() {
	global $defaults;

	$all_extensions = pbx_get_extensions();
	$new_extension = $defaults['accounts']['phones']['default-extension-start'];
	while(in_array($new_extension, $all_extensions)) {
		$new_extension++;
	}

	return $new_extension;
}

/**
 * Merges all *get_providers() into a single array
 * XXX : should be reimplemented to have an optional "technology" parameter
 * so each of these sub _get_ functions can be generally implemented here
 */
function pbx_get_providers() {
	global $config;
	
	return pbx_sort_providers(
				array_merge(
					sip_get_providers(),
					iax_get_providers(),
					isdn_get_providers(),
					analog_get_providers()
				));
}

/**
 * Returns a provider based on its uniqid
 */
function pbx_get_provider($uniqid) {

	if (strstr($uniqid, "SIP-PROVIDER")) {
		return sip_get_provider($uniqid);

	} else if (strstr($uniqid, "IAX-PROVIDER")) {
		return iax_get_provider($uniqid);

	} else if (strstr($uniqid, "ISDN-PROVIDER")) {
		return isdn_get_provider($uniqid);

	} else if (strstr($uniqid, "ANALOG-PROVIDER")) {
		return analog_get_provider($uniqid);
	}

	return;
}

/**
 * Deletes a provider from the configuration array.
 * XXX : as opposed to the pbx_get_* functions, this is implemented in pbx.inc itself
 * this seems more logical as there is nothing technology specific going on here.
 * The other functions should be refactored to use this method if no argument arises
 * against it.
 */
function pbx_delete_provider($uniqid) {
	global $g, $uniqid_map;

	$pieces = explode("-", $uniqid);
	$provider_type = strtolower($pieces[0]);

	if ($pieces[1] == "PROVIDER" &&
		in_array($provider_type, $g['provider_types']) &&
		is_array($GLOBALS['config'][$provider_type]['provider'][$uniqid_map[$uniqid]])) {

		unset($GLOBALS['config'][$provider_type]['provider'][$uniqid_map[$uniqid]]);
		dialplan_remove_provider_reference_from_phones($uniqid);
		return 0;

	} else {
		return "ERROR: invalid uniqid ($uniqid) provided in pbx_delete_provider!";
	}
}

/**
 * Marks a provider disabled
 */
function pbx_disable_provider($uniqid) {
	global $g, $uniqid_map;

	$pieces = explode("-", $uniqid);
	$provider_type = strtolower($pieces[0]);

	if ($pieces[1] == "PROVIDER" &&
		in_array($provider_type, $g['provider_types']) &&
		is_array($GLOBALS['config'][$provider_type]['provider'][$uniqid_map[$uniqid]])) {

		$GLOBALS['config'][$provider_type]['provider'][$uniqid_map[$uniqid]]['disabled'] = true;
		return 0;

	} else {
		return "ERROR: invalid uniqid ($uniqid) provided in pbx_disable_provider!";
	}
}

/**
 * Marks a provider enabled
 */
function pbx_enable_provider($uniqid) {
	global $g, $uniqid_map;

	$pieces = explode("-", $uniqid);
	$provider_type = strtolower($pieces[0]);

	if ($pieces[1] == "PROVIDER" &&
		in_array($provider_type, $g['provider_types']) &&
		is_array($GLOBALS['config'][$provider_type]['provider'][$uniqid_map[$uniqid]])) {

		unset($GLOBALS['config'][$provider_type]['provider'][$uniqid_map[$uniqid]]['disabled']);
		return 0;

	} else {
		return "ERROR: invalid uniqid ($uniqid) provided in pbx_enable_provider!";
	}
}

function pbx_provider_is_active($uniqid) {
	global $g, $uniqid_map;

	$pieces = explode("-", $uniqid);
	$provider_type = strtolower($pieces[0]);

	if ($pieces[1] == "PROVIDER" &&
		in_array($provider_type, $g['provider_types']) &&
		is_array($GLOBALS['config'][$provider_type]['provider'][$uniqid_map[$uniqid]])) {

		return !isset($GLOBALS['config'][$provider_type]['provider'][$uniqid_map[$uniqid]]['disabled']);

	} else {
		return "ERROR: invalid uniqid ($uniqid) provided in pbx_provider_is_active!";
	}
}

/**
 * Sorts providers by name
 */
function pbx_sort_providers($providers) {
	usort($providers, "pbx_sort_by_name");

	return $providers;
}

/**
 * Merges all *get_phones() into a single array
 * XXX : should be reimplemented to have an optional "technology" parameter
 * so each of these sub _get_ functions can be generally implemented here
 */
function pbx_get_phones() {
	global $config;
	
	return pbx_sort_phones(
				array_merge(
					sip_get_phones(),
					iax_get_phones(),
					isdn_get_phones(),
					skinny_get_phones(),
					analog_get_phones()
				));
}

/**
 * Returns a phone based on its uniqid
 */
function pbx_get_phone($uniqid) {

	if (strstr($uniqid, "SIP-PHONE")) {
		return sip_get_phone($uniqid);
			
	} else if (strstr($uniqid, "IAX-PHONE")) {
		return iax_get_phone($uniqid);

	} else if (strstr($uniqid, "ISDN-PHONE")) {
		return isdn_get_phone($uniqid);

	} else if (strstr($uniqid, "ANALOG-PHONE")) {
		return analog_get_phone($uniqid);

	} else if (strstr($uniqid, "SKINNY-PHONE")) {
		return skinny_get_phone($uniqid);

	} else if (strstr($uniqid, "EXTERNAL-PHONE")) {
		return external_get_phone($uniqid);

	} else {
		return "ERROR: unmatched uniqid in pbx_get_phone!";
	}
}

/**
 * Deletes a phone from the configuration array.
 * XXX : as opposed to the pbx_get_* functions, this is implemented in pbx.inc itself
 * this seems more logical as there is nothing technology specific going on here.
 * The other functions should be refactored to use this method if no argument arises
 * against it.
 */
function pbx_delete_phone($uniqid) {
	global $g, $uniqid_map;

	$pieces = explode("-", $uniqid);
	$phone_type = strtolower($pieces[0]);

	if ($pieces[1] == "PHONE" &&
		in_array($phone_type, $g['phone_types']) &&
		is_array($GLOBALS['config'][$phone_type]['phone'][$uniqid_map[$uniqid]])) {

		unset($GLOBALS['config'][$phone_type]['phone'][$uniqid_map[$uniqid]]);
		dialplan_remove_incomingextensionmap_reference_from_providers($uniqid);
		dialplan_remove_member_reference_from_callgroups($uniqid);
		return 0;

	} else {
		return "ERROR: invalid uniqid ($uniqid) provided in pbx_delete_phone!";
	}
}

/**
 * Marks a phone disabled
 */
function pbx_disable_phone($uniqid) {
	global $g, $uniqid_map;

	$pieces = explode("-", $uniqid);
	$phone_type = strtolower($pieces[0]);

	if ($pieces[1] == "PHONE" &&
		in_array($phone_type, $g['phone_types']) &&
		is_array($GLOBALS['config'][$phone_type]['phone'][$uniqid_map[$uniqid]])) {

		$GLOBALS['config'][$phone_type]['phone'][$uniqid_map[$uniqid]]['disabled'] = true;
		return 0;

	} else {
		return "ERROR: invalid uniqid ($uniqid) provided in pbx_disable_phone!";
	}
}

/**
 * Marks a phone enabled
 */
function pbx_enable_phone($uniqid) {
	global $g, $uniqid_map;

	$pieces = explode("-", $uniqid);
	$phone_type = strtolower($pieces[0]);

	if ($pieces[1] == "PHONE" &&
		in_array($phone_type, $g['phone_types']) &&
		is_array($GLOBALS['config'][$phone_type]['phone'][$uniqid_map[$uniqid]])) {

		unset($GLOBALS['config'][$phone_type]['phone'][$uniqid_map[$uniqid]]['disabled']);
		return 0;

	} else {
		return "ERROR: invalid uniqid ($uniqid) provided in pbx_enable_phone!";
	}
}

/**
 * Sorts phones by Caller ID
 * XXX : this sort differs in behavior from the individual _sort_phones() functions
 */
function pbx_sort_phones($phones) {
	usort($phones, "_a_sortphones");

	return $phones;
}

/* XXX : replace with "sort_by_callerid_field" */
function _a_sortphones($a, $b) {
	return strcmp($a['callerid'], $b['callerid']);
}

/**
 * Merges all *get_dialpatterns() into a single array
 * XXX : should be reimplemented to have an optional "technology" parameter
 * so each of these sub _get_ functions can be generally implemented here
 */
function pbx_get_dialpatterns() {
	global $config;

	return array_merge(
		sip_get_dialpatterns(),
		iax_get_dialpatterns(),
		isdn_get_dialpatterns(),
		analog_get_dialpatterns()
	);
}

/**
 * usort callback for sorting an array alphabetically by name field
 */
function pbx_sort_by_name($a, $b) {
	return strcmp($a['name'], $b['name']);
}

/**
 * usort callback for sorting an array numerically by extension field
 */
function pbx_sort_by_extension($a, $b) {
    if ($a['extension'] == $b['extension']) {
        return 0;
    }
    return ($a['extension'] < $b['extension']) ? -1 : 1;
}

/**
 * usort callback for sorting an array numerically by unit field
 */
function pbx_sort_by_unit($a, $b) {

    if ($a['unit'] == $b['unit']) {
        return 0;
    }
    return ($a['unit'] < $b['unit']) ? -1 : 1;
}

/**
 * usort callback for sorting an array numerically by number field
 */
function pbx_sort_by_number($a, $b) {

    if ($a['number'] == $b['number']) {
        return 0;
    }
    return ($a['number'] < $b['number']) ? -1 : 1;
}

/**
 * Parses "core show channels" output to return a channel list
 */
function pbx_get_active_channels(&$channel_list) {
	
	pbx_exec("core show channels", &$output);
	$lines = explode("\n", $output);

	$n = count($lines) - 4;
	for ($i = 1; $i <= $n; $i++) {
		$channel_list[] = $lines[$i];
	}

	return 0;
}

function pbx_get_peer_statuses() {
/*
	65 OK (199 ms)
	SIP-PROVIDER-10758720864848013d90e3b OK (101 ms)
	SIP-PROVIDER-342863348484e7bf9a9d4f OK (119 ms)
*/

	$statuses = array();

	/* sip */
	pbx_exec("sip show statuses", &$raw);
	$lines = explode("\n", $raw);
	foreach ($lines as $line) {
		$line = preg_split("/\s+/", $line);
		$statuses[$line[0]] = $line[1];
	}
	unset($raw);

	/* iax */
	pbx_exec("iax2 show statuses", &$raw);
	$lines = explode("\n", $raw);
	foreach ($lines as $line) {
		$line = preg_split("/\s+/", $line);
		$statuses[$line[0]] = $line[1];
	}
	unset($raw);

	/* isdn */

	/* analog */

	return $statuses;
}

/**
 * Replace a string's contents with the name equivalents of the uniqids contained within.
 */
function pbx_replace_uniqids_with_names($string) {
	
	$uniqid_regex = "/(SIP|IAX|ISDN|ANALOG|SKINNY|CONFERENCE|CALLGROUP|EXTERNAL){1}-(PROVIDER|PHONE|ROOM|PARALLEL){1}-[0-9a-zA-Z]+/";

	preg_match_all($uniqid_regex, $string, $uniqids);
	if (!is_array($uniqids)) {
		return;
	}

	$uniqids = array_keys(array_flip($uniqids[0]));

	$names = array();
	$n = count($uniqids);
	for ($i = 0; $i < $n; $i++) {
		$names[$i] = pbx_uniqid_to_name($uniqids[$i]);
	}

	return str_replace($uniqids, $names, $string);
}

/**
 * Returns an appropriate "name" for the supplied uniqid or array of uniqids
 */
function pbx_uniqid_to_name($uniqid) {

	if (is_array($uniqid)) {
		$n = count($uniqid);
		for ($i = 0; $i < $n; $i++) {
			$uniqid[$i] = pbx_uniqid_to_name_sub($uniqid[$i]);
		}
		return $uniqid;

	} else {
		return pbx_uniqid_to_name_sub($uniqid);
	}
}

function pbx_uniqid_to_name_sub($uniqid) {
	global $config, $uniqid_map;
	
	$pair = explode("-", substr(strtolower($uniqid), 0 , strrpos($uniqid, "-")));

	if ($pair[1] == "phone" && $pair[0] == "external") {
		return $config[$pair[0]][$pair[1]][$uniqid_map[$uniqid]]['callerid'];
	} else if ($pair[1] == "phone") {
		return $config[$pair[0]][$pair[1]][$uniqid_map[$uniqid]]['callerid'];
	}
	if ($pair[1] == "provider") {
		return $config[$pair[0]][$pair[1]][$uniqid_map[$uniqid]]['name'];
	}
	if ($pair[1] == "room") {
		return $config[$pair[0]][$pair[1]][$uniqid_map[$uniqid]]['name'];
	}
	if ($pair[1] == "callgroup") {
		return $config[$pair[0]][$pair[1]][$uniqid_map[$uniqid]]['name'];
	}

	return "ERROR:UNIQID_TO_NAME_FAILED($uniqid)";
}

/**
 * Returns the channel technology associated with the supplied uniqid.
 */
function pbx_uniqid_to_technology($uniqid) {
	
	$parts = explode("-", $uniqid);
	switch($parts[0]) {
		case "SIP":
			return "SIP";
		case "IAX":
			return "IAX2";
		case "ISDN":
			return "DAHDI";
		case "ANALOG":
			return "DAHDI";
		case "SKINNY":
			return "SKINNY";
	}

	return "ERROR:UNIQID_TO_TECHNOLOGY_FAILED($uniqid)";
}

function modules_conf_generate($return=false) {

	$conf = <<<EOD
[modules]
autoload=no
;
; this is the minimum set of modules to support the
; functionality built into the AskoziaPBX GUI
;
load => app_dial.so
load => app_echo.so
load => app_exec.so
load => app_macro.so
load => app_meetme.so
load => app_milliwatt.so
load => app_playback.so
load => app_setcallerid.so
load => app_stack.so
load => app_system.so
load => app_verbose.so
load => app_voicemail.so
load => app_wakeme.so
load => app_while.so
load => chan_dahdi.so
load => chan_iax2.so
load => chan_local.so
load => chan_sip.so
load => chan_skinny.so
load => codec_a_mu.so
load => codec_adpcm.so
load => codec_alaw.so
load => codec_dahdi.so
load => codec_g722.so
load => codec_gsm.so
load => codec_ulaw.so
load => format_gsm.so
load => format_pcm.so
load => format_sln.so
load => format_wav.so
load => format_wav_gsm.so
load => func_callerid.so
load => func_channel.so
load => func_config.so
load => func_cut.so
load => func_db.so
load => func_logic.so
load => func_md5.so
load => func_shell.so
load => func_strings.so
load => pbx_config.so
load => pbx_loopback.so
load => pbx_spool.so
load => res_adsi.so
load => res_indications.so
load => res_limit.so
load => res_musiconhold.so
load => res_smdi.so

EOD;

	return ($return) ? $conf : util_file_write("/etc/asterisk/modules.conf", $conf);
}

function asterisk_conf_generate($return=false) {

	$conf = <<<EOD
[directories]
astetcdir => /etc/asterisk
astmoddir => /offload/asterisk/modules
astvarlibdir => /offload/asterisk
astdatadir => /offload/asterisk
astagidir => /offload/asterisk/agi-bin
astspooldir => /var/asterisk/spool
astrundir => /var/asterisk/run
astlogdir => /var/asterisk/log

[options]
verbose = 3
dumpcore = no
internal_timing = yes
highpriority = yes


EOD;

	return ($return) ? $conf : util_file_write("/etc/asterisk/asterisk.conf", $conf);
}

?>
