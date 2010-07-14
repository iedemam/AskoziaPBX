<?php
/*
	$Id$
	originally part of m0n0wall (http://m0n0.ch/wall)
	continued modifications as part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
	Copyright (C) 2007-2010 tecema (a.k.a IKT) <http://www.tecema.de>.
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
 * Verifies input as being a numeric integer
 */
function verify_is_numericint($arg) {
	return (preg_match("/[^0-9]/", $arg) ? false : true);
}

/**
 * Returns true if $ipaddr is a valid dotted IPv4 address
 */
function verify_is_ipaddress($ipaddr) {

	if (!is_string($ipaddr)) {
		return false;
	}
	
	$ip_long = ip2long($ipaddr);
	$ip_reverse = long2ip($ip_long);
 
	if ($ipaddr == $ip_reverse) {
		return true;
	} else {
		return false;
	}
}

/**
 * Returns true if $subnet is a valid subnet in CIDR format
 */
function verify_is_subnet($subnet) {

	if (!is_string($subnet)) {
		return false;
	}
		
	list($hp,$np) = explode('/', $subnet);
	
	if (!verify_is_ipaddress($hp)) {
		return false;
	}
	
	if (!is_numeric($np) || ($np < 1) || ($np > 32)) {
		return false;
	}
		
	return true;
}

/**
 * Returns true if $hostname is a valid hostname
 */
function verify_is_hostname($hostname) {

	if (!is_string($hostname)) {
		return false;
	}
		
	if (preg_match("/^[a-z0-9\-]+$/i", $hostname)) {
		return true;
	} else {
		return false;
	}
}

/**
 * Returns true if $domain is a valid domain name
 */
function verify_is_domain($domain) {

	if (!is_string($domain)) {
		return false;
	}
		
	if (preg_match("/^([a-z0-9\-]+\.?)*$/i", $domain)) {
		return true;
	} else {
		return false;
	}
}

/**
 * Returns true if $uname is a valid DynDNS username
 */
function verify_is_dyndns_username($uname) {

	if (!is_string($uname)) {
		return false;
	}
	
	if (preg_match("/[^a-z0-9\-.@_]/i", $uname)) {
		return false;
	} else {
		return true;
	}
}

/**
 * Returns true if $macaddr is a valid MAC address
 */
function verify_is_macaddress($macaddr) {

	if (!is_string($macaddr)) {
		return false;
	}
		
	$maca = explode(":", $macaddr);
	if (count($maca) != 6) {
		return false;
	}
	
	foreach ($maca as $macel) {
		if (($macel === "") || (strlen($macel) > 2)) {
			return false;
		}
		if (preg_match("/[^0-9a-f]/i", $macel)) {
			return false;
		}
	}
	
	return true;
}

/**
 * Returns true if $port is a valid TCP/UDP port
 */
function verify_is_port($port) {

	if (!verify_is_numericint($port)) {
		return false;
	}
		
	if (($port < 1) || ($port > 65535)) {
		return false;
	} else {
		return true;
	}
}

/**
 * Returns true if $address is a RFC value e-mail address
 * XXX : switch to preg_match()
 */
function verify_is_email_address($address) {

	if (!eregi("^[A-Z0-9._%-]+@[A-Z0-9._%-]+\.[A-Z]{2,6}$", $address)) {
	   return false;
	}

	return true;
}

/**
 * Verifies (and remove) the digital signature on a file, returns 0 if OK
 */
function verify_digital_signature($fname) {
	global $g;

	return mwexec("/usr/bin/verifysig " .
		escapeshellarg("{$g['etc_path']}/pubkey.pem") . " " .
		escapeshellarg($fname));
}

/**
 * Returns the supplied argument if the string has length and is unequal to all default values
 */
function verify_non_default($value, $defaults=false) {

	if (!$defaults) {
		return !strlen($value) ? false : $value;
	}

	if (!is_array($defaults)) {
		return $value == $defaults ? false : $value;
	}

	foreach ($defaults as $default) {
		if ($value == $default) {
			return false;
		}
	}
	return $value;
}

/**
 * Verifies the "user" substring in a sip uri
 */
function verify_is_public_name($name) {

	$regex = "/^[a-zA-Z0-9\-\(\)\.]+$/";

	if (!preg_match($regex, $name)) {
		return "An invalid public name was entered!";
	}
	return false;
}

/**
* check mixed matching parentheses
* 
* @param string $string
*/
function verify_match_parentheses($string) {
	$rcounter = 0;
	$scounter = 0;
	$ccounter = 0;
	$backtrace = array();
	$length = strlen($string);
	for($i = 0; $i < $length; $i ++) {
		$char = $string[$i];
		$bcount = count($backtrace);
		if (!($char == '(' | $char == ')' | $char == '[' | $char == ']' | $char == '{' | $char == '}')) { continue; }
		switch ($char) {
			case '(': {
				$rcounter++;
				$backtrace[] = $char;
				break;
			}
			case ')': {
				$rcounter--;
				if ($bcount > 0){
					if ($backtrace[$bcount-1] == '(') {
						array_pop($backtrace);
					}
					else return $i+1;
				}
				 break;
			}
			case '[': {
				$scounter++;
				$backtrace[] = $char;
				break;
			}
			case ']': {
				$scounter--;
				if ($bcount > 0){
					if ($backtrace[$bcount-1] == '[') {
						array_pop($backtrace);
					}
					else return $i+1;
				}
				 break;
			}
			case '{': {
				$ccounter++;
				$backtrace[] = $char;
				break;
			}
			case '}':{
				$ccounter--;
				if ($bcount > 0){
					if ($backtrace[$bcount-1] == '{') {
						array_pop($backtrace);
					}
					else return $i+1;
				}
				 break;
			}
		}
		if($rcounter < 0 | $scounter < 0 | $ccounter < 0) {
			return $i+1;
		}
	}
	if ($rcounter == 0 & $scounter == 0 & $ccounter == 0) {
		return true;
	}
	else return $i+1;
}

/**
 * Verifies user provided application logic
 * This is a very basic check at the moment.
 * Alloweds syntax:
 * - numbered priority,command -> 1,hangup()
 * - numbered priority(label),command -> 1(label),hangup()
 * - unnumbered priority,command -> n,hangup()
 * - unnumbered priority(label),command -> n(label),hangup()
 * Allowed additional directives:
 * - Include => <somecontext> 
 * Check for matching parentheses
 */
function verify_application_logic($app_lines) {
	$n = count($app_lines);
	$app_index = 0;
	$err_list = array();
	for ($i = 1; $i <= $n; $i++) {
		$app_index++;
		// check for include directive
		if (preg_match('/^include\\s=>\\s[A-Z0-9-_]+/i', $app_lines[$i-1])) {
			$app_index--;
			continue;
		}
		// check for application rules:
		if (preg_match('/^(?P<pri>[\d]+|n)(?:\((?P<lbl>[A-Za-z]{2,})\))??,(?P<cmd>[A-Za-z]{2,}(?:\(.*\)))/i', $app_lines[$i-1], $groups)) {
			// check for matching parentheses in commands
			$result = verify_match_parentheses($groups['cmd']);
			if ($result !== true) {
				$err_list[] = "Line $i -> Parenthesis mismatch in the dialplan command before position $result";
			}
			if ($groups['pri'] == 'n') {
				// first priority must be numbered
				if ($app_index == 1) {
					$err_list[] = "Line $i -> Wrong numbering, first priority must always be 1!";
				}
				continue; 
			}
			if ($groups['pri'] <> $app_index) {
				$err_list[] = "Line $i -> Incorrect priority number (expected $app_index, found {$groups['pri']}).";
			}
		}
		else {
			$err_list[] = "Line $i -> Invalid syntax (missing parenthesis, commas...).";
		}
	}
	// report found error(s) where found...
	if (count($err_list) > 0) {
		return $err_list;
	}
}

/**
 * Verifies manual attributes
 * This is a very basic check at the moment.
 */
function verify_manual_attributes($kv_lines) {

	$n = count($kv_lines);
	for ($i = 1; $i <= $n; $i++) {
		if (!preg_match("/^.+=.+/", $kv_lines[$i-1])) {
			return "Manual attribute line number $i is invalid.";
		}	
	}
}

/**
 * Verifies an outgoing dialstring used in external phones
 */
function verify_is_dialstring($dialstring) {

	$regex = "/^[a-zA-Z0-9\-\.\+\@]+$/";

	if (!preg_match($regex, $dialstring)) {
		return "An invalid dialstring was entered!";
	}
	return false;
}

/**
 * Verifies the presentation or "readback" number
 */
function verify_readback_number($readbacknum) {

	if (!verify_is_numericint($readbacknum)) {
		return "An invalid readback number was entered. Readback numbers must be numeric.";
	}
	return false;
}

function verify_input($postdata, $reqdfields, $reqdfieldsn, $input_errors) {
	
	/* check for bad control characters */	
	foreach ($postdata as $pn => $pd) {
		if (is_string($pd) && preg_match("/[\\x00-\\x08\\x0b\\x0c\\x0e-\\x1f]/", $pd)) {
			$input_errors[] = sprintf(gettext("The field '%s' contains invalid characters."), $pn);
		}
	}

	for ($i = 0; $i < count($reqdfields); $i++) {
		if (!isset($_POST[$reqdfields[$i]]) || ($_POST[$reqdfields[$i]] === "")) {
			$input_errors[] = sprintf(gettext("The field '%s' is required."), gettext($reqdfieldsn[$i]));
		}
	}
}








function v_cleanliness($post, $errors) {
	foreach ($post as $pn => $pd) {
		if (is_string($pd) && preg_match("/[\\x00-\\x08\\x0b\\x0c\\x0e-\\x1f]/", $pd)) {
			$errors[] = sprintf(gettext("The field '%s' contains invalid characters."), $pn);
		}
	}
}


/**
 * checks the extension number for collisions
 * returns true if $ext is not used, false otherwise
*/
function check_uniq ($ext)
{
	global $g;
	
	//go through config and compare $ext with every number:
	foreach($GLOBALS['config']['sip']['phone'] as $phone) {
		if($phone['extension'] == $ext) return false;
	}
	foreach($GLOBALS['config']['iax']['phone'] as $phone) {
		if($phone['extension'] == $ext) return false;
	}
	foreach($GLOBALS['config']['isdn']['phone'] as $phone) {
		if($phone['extension'] == $ext) return false;
	}
	foreach($GLOBALS['config']['analog']['phone'] as $phone) {
		if($phone['extension'] == $ext) return false;
	}
	return true;
	
}

function v_extension($element, $post, $errors) {
	if (!strlen($post['extension'])) {
		$errors[] = gettext("Missing extension number.");
	} else if (preg_match("/[^0-9]/", $post['extension'])) {
		$errors[] = gettext("Invalid extension number. Use only numerals.");
	}
	else if (!check_uniq($post['extension']))
	{
		$errors[] = gettext("This extension is already used. Choose another number.");
	}
	$element['extension'] = $post['extension'];
}

function v_callerid($element, $post, $errors) {
	if (!strlen($post['callerid'])) { //|| preg_match("/^[a-z0-9]+(\s{1}[a-z0-9]|[a-z0-9])+$/i", $post['callerid'])) {
		$errors[] = gettext("Missing caller id.");
	}
	$element['callerid'] = $post['callerid'];
}

function v_public_access_editor($element, $post, $errors) {
	if ($post['publicaccess'] == 'enabled' && !v_is_publicname($post['publicname'])) {
		$errors[] = gettext("Invalid public access alias.");
	}
	$element['publicaccess'] = v_strip($post['publicaccess']);
	$element['publicname'] = v_strip($post['publicname']);
}

function v_is_publicname($s) {
	if (strlen($s)) {
		return preg_match("/^[a-zA-Z0-9\-\(\)\.]+$/", $s);
	} else {
		return true;
	}
}

function v_notifications_editor($element, $post, $errors) {
	if ($post['emailcallnotify'] == 'enabled' && !v_is_email($post['emailcallnotifyaddress'])) {
		$errors[] = gettext("E-Mail notifications enabled but the e-mail address is invalid or missing.");
	}
	$element['emailcallnotify'] = v_strip($post['emailcallnotify']);
	$element['emailcallnotifyaddress'] = v_strip($post['emailcallnotifyaddress']);
}

function v_voicemail_editor($element, $post, $errors) {
	if ($post['vmtoemail'] == 'enabled' && !v_is_email($post['vmtoemailaddress'])) {
		$errors[] = gettext("Voicemail to e-mail enabled but the e-mail address is invalid or missing.");
	}
	$element['vmtoemail'] = v_strip($post['vmtoemail']);
	$element['vmtoemailaddress'] = v_strip($post['vmtoemailaddress']);
}

function v_is_email($s) {
	return eregi("^[A-Z0-9._%-]+@[A-Z0-9._%-]+\.[A-Z]{2,6}$", $s);
}

function v_provider_access_selector($element, $post) {
	$all_providers = pbx_get_providers();
	foreach ($all_providers as $provider) {
		if ($post[$provider['uniqid']] == true) {
			$element['provider'][] = $provider['uniqid'];
		}
	}
}

function v_strip($s) {
	return strlen($s) ? $s : false;
}

function v_codecs($element, $post) {
	parse_str($post['a_codecs']);
	parse_str($post['v_codecs']);
	$element['codec'] = $post['codec'] = array_merge($ace, $vce);
}

function v_draganddrop($element, $post) {
	parse_str($post['groupmembers']);
	$element['groupmember'] = $post['groupmember'] = $gme;
}

function v_password($element, $post) {
	if (!strlen($s)) {
		$errors[] = gettext("Missing password.");
	}
	$element['secret'] = $post['secret'];
}

function v_dialstring($element, $post, $errors) {
	if (!strlen($post['dialstring'])) {
		$errors[] = gettext("Missing dialstring.");
	} else if (!preg_match("/^[a-zA-Z0-9\-\.\+\@]+$/", $post['dialstring'])) {
		$errors[] = gettext("An invalid dialstring was entered.");
	}
	$element['dialstring'] = $post['dialstring'];
}

function v_port($element, $post, $errors) {
	if (strlen($post['port']) && ($post['port'] < 1 || $post['port'] > 65535)) {
		$errors[] = gettext("An invalid port was entered.");
	}
	$element['port'] = v_strip($post['port']);
}

function v_incoming_prov_routing($element, $post, $errors) {
	$post['incomingextensionmap'] = gather_incomingextensionmaps($post);
	if (is_array($post['incomingextensionmap'])) {
		foreach($post['incomingextensionmap'] as $map) {
			if ($map['incomingpattern'] && !pbx_is_valid_dialpattern($map['incomingpattern'], &$internal_error, true)) {
				$errors[] = sprintf(gettext("The incoming extension pattern \"%s\" is invalid. %s"), $map['incomingpattern'], $internal_error);
			}
		}
	}
	$element['incomingextensionmap'] = $post['incomingextensionmap'];
}

function v_outgoing_prov_routing($element, $post, $errors) {
	$post['dialpattern'] = split_and_clean_lines($post['dialpattern']);
	if (is_array($post['dialpattern'])) {
		foreach($post['dialpattern'] as $p) {
			if (!pbx_is_valid_dialpattern($p, &$internal_error)) {
				$errors[] = sprintf(gettext("The outgoing dial-pattern \"%s\" is invalid. %s"), $p, $internal_error);
			}
		}
	}
	$element['dialpattern'] = $post['dialpattern'];
}

function v_outgoing_prov_callerid($element, $post, $errors) {
	if ($post['calleridsource'] == "string" && !pbx_is_valid_callerid_string($post['calleridstring'])) {
		$errors[] = gettext("A valid Caller ID string must be specified.");
	}
	$element['calleridsource'] = $post['calleridsource'];
	$element['calleridstring'] = v_strip($post['calleridstring']);
}

function v_incoming_prov_callerid($element, $post, $errors) {
	if (($post['override'] == "prepend" || $post['override'] == "replace") && !$post['overridestring']) {
		$errors[] = gettext("An incoming Caller ID override string must be specified.");
	}
	$element['override'] = ($post['override'] != "disable") ? $post['override'] : false;
	$element['overridestring'] = v_strip($post['overridestring']);
}

function v_numeric($field, $name, $element, $post, $errors, $required=false) {
	if ($required) {
		if (!strlen($post[$field])) {
			$errors[] = sprintf(gettext("The %s field is required."), $name);
		} else if (preg_match("/[^0-9]/", $post[$field])) {
			$errors[] = sprintf(gettext("The %s field must be numeric."), $name);
		}
		$element[$field] = $post[$field];

	} else {
		if (strlen($post[$field]) && preg_match("/[^0-9]/", $post[$field])) {
			$errors[] = sprintf(gettext("The %s field must be numeric."), $name);
		}
		$element[$field] = v_strip($post[$field]);
	}
}

function v_required($field, $name, $element, $post, $errors) {
	if (!strlen($post[$field])) {
		$errors[] = sprintf(gettext("The %s field is required."), $name);
	}
	$element[$field] = $post[$field];
}

function v_optional($field, $element, $post, $base64=false) {
	if ($base64) {
		$post[$field] = $element[$field] = v_strip(base64_encode($post[$field]));
	} else {
		$element[$field] = v_strip($post[$field]);
	}
}

function v_checkbox($field, $element, $post) {
	$element[$field] = isset($post[$field]);
}

function v_manualattributes($element, $post, $errors) {
	// split textfield into lines
	$lines = preg_split("/[\x0D\x0A]+/", $post['manualattributes'], -1, PREG_SPLIT_NO_EMPTY);
	$n = count($lines);
	for ($i = 0; $i < $n; $i++) {
		// clean lines from trailing whitespace
		$lines[$i] = trim($lines[$i], " ");
		// basic formatting check
		if (!preg_match("/^.+=.+/", $lines[$i])) {
			$errors[] = sprintf(gettext("Manual Attribute line number %s is invalid."), ($i + 1));
		}
	}

	$post['manualattributes'] = $element['manualattributes'] = v_strip(base64_encode(implode("\n", $lines)));
}

function v_applicationlogic($element, $post, $errors) {

	$code = str_replace("\r\n", "\n", $post['code']);

	if ($post['type'] == "php") {
		$tmpfilepath = "/tmp/" . uniqid(rand());
		util_file_write($tmpfilepath, $code);
		exec("/etc/scripts/phpparsecheck.sh " . $tmpfilepath, $output, $retval);
		unlink($tmpfilepath);
		if ($retval != 0) {
			$errormessage = gettext("PHP parsing error") . ":";
			foreach ($output as $out) {
				if (strpos($out, "Parse error: ") !== false) {
					$out = str_replace("Parse error: ", "", $out);
					$out = str_replace("in " . $tmpfilepath . " ", "", $out);
					$errormessage .= " " . $out;
				}
			}
			$errors[] = $errormessage;
		}
	}

	$post['applicationlogic'] = $element['applicationlogic'] = base64_encode($code);
}

?>
