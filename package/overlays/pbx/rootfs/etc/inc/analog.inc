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

/* include all configuration functions */
require_once("functions.inc");

/**
 * Returns all providers sorted by name.
 * If no providers are defined, the global array is initialized.
 * This should be changed to return by reference to eliminate a lot of repetitive code.
 */
function analog_get_providers() {

	if (!is_array($GLOBALS['config']['analog']['provider'])) {
		$GLOBALS['config']['analog']['provider'] = array();
	}

	$tmp = $GLOBALS['config']['analog']['provider'];
	usort($tmp, "pbx_sort_by_name");

	return $tmp;
}

/**
 * Returns a specific provider based on its uniqid
 */
function analog_get_provider($uniqid) {
	global $uniqid_map;

	return $GLOBALS['config']['analog']['provider'][$uniqid_map[$uniqid]];
}

/**
 * Returns all phones sorted by extension.
 * If no phones are defined, the global array is initialized.
 */
function analog_get_phones() {

	if (!is_array($GLOBALS['config']['analog']['phone'])) {
		$GLOBALS['config']['analog']['phone'] = array();
	}

	$tmp = $GLOBALS['config']['analog']['phone'];
	usort($tmp, "pbx_sort_by_extension");

	return $tmp;
}

/**
 * Returns a specific phone based on its uniqid
 */
function analog_get_phone($uniqid) {
	global $uniqid_map;

	return $GLOBALS['config']['analog']['phone'][$uniqid_map[$uniqid]];
}

/**
 * Returns an array of all assigned phone extensions
 */
function analog_get_extensions() {

	$extensions = array();

	if (is_array($GLOBALS['config']['analog']['phone'])) {
		foreach($GLOBALS['config']['analog']['phone'] as $phone) {
			$extensions[] = $phone['extension'];
		}
	}

	return $extensions;
}

/**
 * Returns an array of all assigned provider dialpatterns
 */
function analog_get_dialpatterns() {

	$dialpatterns = array();

	if (is_array($GLOBALS['config']['analog']['provider'])) {
		foreach($GLOBALS['config']['analog']['provider'] as $provider) {
			$dialpatterns[$provider['uniqid']] = $provider['dialpattern'];
		}
	}

	return $dialpatterns;
}

function analog_autoconfigure_phones() {
	global $g;

	$all_ports = dahdi_get_ports("analog", "fxs");

	foreach ($all_ports as $port) {
		if (isset($port['unused'])) {
			$phone = analog_generate_default_phone();
			$phone['port'] = $port['uniqid'];
			analog_save_phone($phone);

			unset($port['unused']);
			dahdi_save_port($port);
		}
	}

	unlink_if_exists($g['analog_dirty_path']);
	unlink_if_exists($g['dahdi_dirty_path']);
}

function analog_generate_default_phone() {
	global $defaults;

	$phone = array();

	$phone['extension'] = pbx_get_next_default_phone_extension();
	$phone['callerid'] = $defaults['accounts']['phones']['callerid'];
	$phone['language'] = $defaults['accounts']['language'];
	$phone['ringlength'] = $defaults['accounts']['phones']['ringlength'];
	$phone['publicaccess'] = $defaults['accounts']['phones']['publicaccess'];
	$phone['uniqid'] = "ANALOG-PHONE-" . uniqid(rand());

	return $phone;
}

function analog_verify_phone($post, $errors) {

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

function analog_save_phone($phone) {
	global $g, $uniqid_map;

	if (analog_get_phone($phone['uniqid'])) {
		$GLOBALS['config']['analog']['phone'][$uniqid_map[$phone['uniqid']]] = $phone;
	} else {
		$GLOBALS['config']['analog']['phone'][] = $phone;
	}

	write_config();
	touch($g['analog_dirty_path']);
}

function analog_generate_default_provider() {
	global $defaults;

	$provider = array();

	$provider['language'] = $defaults['accounts']['language'];
	$provider['calleridsource'] = $defaults['accounts']['providers']['calleridsource'];
	$provider['uniqid'] = 'ANALOG-PROVIDER-' . uniqid(rand());

	return $provider;
}

function analog_verify_provider($post, $errors) {

	$provider = array();

	v_cleanliness(&$post, &$errors);
	v_required('name', gettext('Name'), &$provider, &$post, &$errors);
	v_numeric('number', gettext('Number'), &$provider, &$post, &$errors);
	v_incoming_prov_routing(&$provider, &$post, &$errors);
	v_incoming_prov_callerid(&$provider, &$post, &$errors);
	v_outgoing_prov_routing(&$provider, &$post, &$errors);
	v_outgoing_prov_callerid(&$provider, &$post, &$errors);
	v_manualattributes(&$provider, &$post, &$errors);

	$optional = array(
		'failover'
	);
	foreach ($optional as $o) {
		$provider[$o] = v_strip($post[$o]);
	}

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

function analog_save_provider($provider) {
	global $g, $uniqid_map;

	if (analog_get_provider($provider['uniqid'])) {
		$GLOBALS['config']['analog']['provider'][$uniqid_map[$provider['uniqid']]] = $provider;
	} else {
		$GLOBALS['config']['analog']['provider'][] = $provider;
	}

	write_config();
	touch($g['analog_dirty_path']);
}

?>
