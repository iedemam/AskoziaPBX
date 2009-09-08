<?php
/*
	$Id$
	originally part of m0n0wall (http://m0n0.ch/wall)
	continued modifications as part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
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

?>
