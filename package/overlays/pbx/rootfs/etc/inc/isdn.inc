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
 * Returns all providers sorted by name.
 * If no providers are defined, the global array is initialized.
 * This should be changed to return by reference to eliminate a lot of repetitive code.
 */
function isdn_get_providers() {

	if (!is_array($GLOBALS['config']['isdn']['provider'])) {
		$GLOBALS['config']['isdn']['provider'] = array();
	}
	isdn_sort_providers();

	return $GLOBALS['config']['isdn']['provider'];
}

/**
 * Sorts providers by name
 */
function isdn_sort_providers() {

	usort($GLOBALS['config']['isdn']['provider'], "pbx_sort_by_name");	
}

/**
 * Returns a specific provider based on its uniqid
 */
function isdn_get_provider($uniqid) {
	global $uniqid_map;

	return $GLOBALS['config']['isdn']['provider'][$uniqid_map[$uniqid]];
}

/**
 * Returns all phones sorted by extension.
 * If no phones are defined, the global array is initialized.
 */
function isdn_get_phones() {

	if (!is_array($GLOBALS['config']['isdn']['phone'])) {
		$GLOBALS['config']['isdn']['phone'] = array();
	}
	isdn_sort_phones();

	return $GLOBALS['config']['isdn']['phone'];
}

/**
 * Sorts phones by extension
 */
function isdn_sort_phones() {

	usort($GLOBALS['config']['isdn']['phone'], "pbx_sort_by_extension");	
}

/**
 * Returns a specific phone based on its uniqid
 */
function isdn_get_phone($uniqid) {
	global $uniqid_map;

	return $GLOBALS['config']['isdn']['phone'][$uniqid_map[$uniqid]];
}

/**
 * Returns an array of all assigned phone extensions
 */
function isdn_get_extensions() {

	$extensions = array();

	if (is_array($GLOBALS['config']['isdn']['phone'])) {
		foreach($GLOBALS['config']['isdn']['phone'] as $phone) {
			$extensions[] = $phone['extension'];
		}
	}

	return $extensions;
}

/**
 * Returns an array of all assigned provider dialpatterns
 */
function isdn_get_dialpatterns() {

	$dialpatterns = array();

	if (is_array($GLOBALS['config']['isdn']['provider'])) {
		foreach($GLOBALS['config']['isdn']['provider'] as $provider) {
			$dialpatterns[$provider['uniqid']] = $provider['dialpattern'];
		}
	}

	return $dialpatterns;
}

/**
 * Returns all phones associated with a particular ISDN unit
 * If no phones are associated with the supplied unit, an empty array is returned
 */
function isdn_get_phones_on_unit($uniqid) {
	
	$phones = isdn_get_phones();
	
	$unit_phones = array();
	foreach ($phones as $p) {
		if (isset($p['disabled'])) {
			continue;
		}
		if ($p['port'] != $uniqid) {
			continue;
		}
		$unit_phones[] = $p;
	}
		
	return $unit_phones;
}

/**
 * Returns all providers associated with a particular ISDN unit
 * If no providers are associated with the supplied unit, an empty array is returned
 */
function isdn_get_providers_on_unit($uniqid) {
	
	$providers = isdn_get_providers();

	$unit_providers = array();
	foreach ($providers as $p) {
		if (isset($p['disabled'])) {
			continue;
		}
		if ($p['port'] != $uniqid) {
			continue;
		}
		$unit_providers[] = $p;
	}
		
	return $unit_providers;
}


/**
 * Generates capi.conf
 */
function isdn_conf_generate() {
	global $config, $g;
	
	$fd = fopen("/etc/asterisk/capi.conf", "w");
	if (!$fd) {
		printf("Error: cannot open capi.conf in isdn_conf_generate().\n");
		return 1;
	}
	
	$isdnconfig = $config['services']['isdn'];
	
	$law = "alaw";
	if (isset($isdnconfig['law']))
		$law = $isdnconfig['law'];

	$nationalprefix = "0";
	if (isset($isdnconfig['nationalprefix']))
		$nationalprefix = $isdnconfig['nationalprefix'];

	$internationalprefix = "00";
	if (isset($isdnconfig['internationalprefix']))
		$internationalprefix = $isdnconfig['internationalprefix'];
		
	$conf = <<<EOD
	
[general]
nationalprefix=$nationalprefix
internationalprefix=$internationalprefix
rxgain=1.0
txgain=1.0
;debug=yes

EOD;

	if ($law != "alaw") {
		$conf .= "ulaw=yes\n";
	}
	
	$conf .= "\n\n";
	
	// provider (te) interfaces
	$interfaces = isdn_get_te_interfaces();
	foreach($interfaces as $interface) {
		$conf .= "; {$interface['name']}\n";
		$conf .= "[{$interface['unit']}]\n";;
		
		$conf .= "isdnmode=msn\n";
		//$conf .= "msn={$p['msn']}\n";
		$conf .= "incomingmsn=*\n";
		$conf .= "controller={$interface['unit']}\n";
		$conf .= "group=1\n";
		//$conf .= ";prefix=0\n";
		$conf .= "softdtmf=off\n";
		$conf .= "relaxdtmf=off\n";
		//$conf .= "accountcode=\n";
		
		$conf .= "context=isdn-unit-{$interface['unit']}-incoming\n";
    
		$conf .= "holdtype=local_moh\n";
		//$conf .= "immediate=yes\n";
    
		if ($interface['echocancel']) {
			$conf .= "echocancel=yes\n";
		} else {
			$conf .= "echocancel=no\n";
		}
    
		//$conf .= ";echocancelold=yes\n";
		$conf .= "echotail=64\n";
		//$conf .= ";bridge=yes\n";
		//$conf .= ";callgroup=1\n";
		//$conf .= ";deflect=1234567\n";
		$conf .= "devices=2\n";
		$conf .= util_gen_manual_attributes($interface['manual-attribute']);
		$conf .= "\n";
	}
	
	$conf .= "\n";
	
	// phone (nt) interfaces
	$interfaces = isdn_get_nt_interfaces();
	foreach($interfaces as $interface) {
		$conf .= "; {$interface['name']}\n";
		$conf .= "[{$interface['unit']}]\n";
		
		$conf .= "ntmode=yes\n";
		$conf .= "isdnmode=did\n";
		$conf .= "incomingmsn=*\n";
		$conf .= "controller={$interface['unit']}\n";
		$conf .= "group=1\n";
		//$conf .= ";prefix=0\n";
		$conf .= "softdtmf=off\n";
		$conf .= "relaxdtmf=off\n";
		//$conf .= "accountcode=\n";
		
		$conf .= "context=isdn-unit-{$interface['unit']}-incoming\n";
    
		$conf .= "holdtype=local_moh\n";
		//$conf .= "immediate=yes\n";

		if ($interface['echocancel']) {
			$conf .= "echocancel=yes\n";
		} else {
			$conf .= "echocancel=no\n";
		}

		//$conf .= ";echocancelold=yes\n";
		//$conf .= ";echotail=64\n";
		//$conf .= ";bridge=yes\n";
		//$conf .= ";callgroup=1\n";
		//$conf .= ";deflect=1234567\n";
		$conf .= "devices=2\n";
		$conf .= util_gen_manual_attributes($interface['manual-attribute']);
		$conf .= "\n";
	}

	fwrite($fd, $conf);
	fclose($fd);

	return 0;
}











function isdn_autoconfigure_phones() {
	global $g;

	$all_ports = dahdi_get_ports("isdn", "nt", 3);

	foreach ($all_ports as $port) {
		if (isset($port['unused'])) {
			$phone = isdn_generate_default_phone();
			$phone['port'] = $port['uniqid'];
			isdn_save_phone($phone);

			unset($port['unused']);
			dahdi_save_port($port);
		}
	}

	unlink_if_exists($g['isdn_dirty_path']);
	unlink_if_exists($g['dahdi_dirty_path']);
}

function isdn_generate_default_phone() {
	global $defaults;

	$phone = array();

	$phone['extension'] = pbx_get_next_default_phone_extension();
	$phone['callerid'] = $defaults['accounts']['phones']['callerid'];
	$phone['language'] = $defaults['accounts']['language'];
	$phone['ringlength'] = $defaults['accounts']['phones']['ringlength'];
	$phone['publicaccess'] = $defaults['accounts']['phones']['publicaccess'];
	$phone['uniqid'] = "ISDN-PHONE-" . uniqid(rand());

	return $phone;
}

function isdn_verify_phone($post, $errors) {

	$phone = array();

	v_cleanliness(&$post, &$errors);
	v_extension(&$phone, &$post, &$errors);
	v_callerid(&$phone, &$post, &$errors);
	v_optional('descr', &$phone, &$post);
	v_public_access_editor(&$phone, &$post, &$errors);
	v_notifications_editor(&$phone, &$post, &$errors);
	v_voicemail_editor(&$phone, &$post, &$errors);
	v_provider_access_selector(&$phone, &$post);

	$trusted = array(
		'uniqid',
		'port',
		'language',
		'ringlength'
	);
	foreach ($trusted as $t) {
		$phone[$t] = $post[$t];
	}

	return $phone;
}

function isdn_save_phone($phone) {
	global $g, $uniqid_map;

	if (isdn_get_phone($phone['uniqid'])) {
		$GLOBALS['config']['isdn']['phone'][$uniqid_map[$phone['uniqid']]] = $phone;
	} else {
		$GLOBALS['config']['isdn']['phone'][] = $phone;
	}

	write_config();
	touch($g['isdn_dirty_path']);
}

?>
