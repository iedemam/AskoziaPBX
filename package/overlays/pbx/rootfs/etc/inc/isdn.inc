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
	v_manualattributes(&$phone, &$post, &$errors);

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

function isdn_generate_default_provider() {
	global $defaults;

	$provider = array();

	$provider['language'] = $defaults['accounts']['language'];
	$provider['calleridsource'] = $defaults['accounts']['providers']['calleridsource'];
	$provider['uniqid'] = 'ISDN-PROVIDER-' . uniqid(rand());

	return $provider;
}

function isdn_verify_provider($post, $errors) {

	$provider = array();

	v_cleanliness(&$post, &$errors);
	v_required('name', gettext('Name'), &$provider, &$post, &$errors);
	v_numeric('mainnumber', gettext('Main Number'), &$provider, &$post, &$errors);
	v_incoming_prov_routing(&$provider, &$post, &$errors);
	v_incoming_prov_callerid(&$provider, &$post, &$errors);
	v_outgoing_prov_routing(&$provider, &$post, &$errors);
	v_outgoing_prov_callerid(&$provider, &$post, &$errors);
	v_manualattributes(&$provider, &$post, &$errors);

	$trusted = array(
		'uniqid',
		'language',
		'port'
	);
	foreach ($trusted as $t) {
		$provider[$t] = $post[$t];
	}

	return $provider;
}

function isdn_save_provider($provider) {
	global $g, $uniqid_map;

	if (isdn_get_provider($provider['uniqid'])) {
		$GLOBALS['config']['isdn']['provider'][$uniqid_map[$provider['uniqid']]] = $provider;
	} else {
		$GLOBALS['config']['isdn']['provider'][] = $provider;
	}

	write_config();
	touch($g['isdn_dirty_path']);
}

?>
