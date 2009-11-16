<?php
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2009 IKT <http://itison-ikt.de>.
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
function analog_get_providers() {
	global $config;
	
	if (!is_array($config['analog']['provider']))
		$config['analog']['provider'] = array();
		
	analog_sort_providers();
		
	return $config['analog']['provider'];
}

/**
 * Sorts providers by name
 */
function analog_sort_providers() {
	global $g, $config;

	usort($config['analog']['provider'], "pbx_sort_by_name");	
}

/**
 * Returns a specific provider based on its uniqid
 */
function analog_get_provider($uniqid) {
	global $config, $uniqid_map;

	return $config['analog']['provider'][$uniqid_map[$uniqid]];
}

/**
 * Returns all phones sorted by extension.
 * If no phones are defined, the global array is initialized.
 */
function analog_get_phones() {
	global $config;
	
	if (!is_array($config['analog']['phone']))
		$config['analog']['phone'] = array();
		
	analog_sort_phones();
		
	return $config['analog']['phone'];
}

/**
 * Sorts phones by extension
 */
function analog_sort_phones() {
	global $g, $config;

	usort($config['analog']['phone'], "pbx_sort_by_extension");	
}

/**
 * Returns a specific phone based on its uniqid
 */
function analog_get_phone($uniqid) {
	global $config, $uniqid_map;

	return $config['analog']['phone'][$uniqid_map[$uniqid]];
}

/**
 * Returns an array of all assigned phone extensions
 */
function analog_get_extensions() {
	global $config;
	
	$extensions = array();

	if (is_array($config['analog']['phone'])) {
		foreach($config['analog']['phone'] as $phone) {
			$extensions[] = $phone['extension'];
		}
	}

	return $extensions;
}

/**
 * Returns an array of all assigned provider dialpatterns
 */
function analog_get_dialpatterns() {
	global $config;
	
	$dialpatterns = array();

	if (is_array($config['analog']['provider'])) {
		foreach($config['analog']['provider'] as $provider) {
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

function analog_save_phone($phone) {
	global $g, $config, $uniqid_map;
    
	if (analog_get_phone($port['uniqid'])) {
		$config['analog']['phone'][$uniqid_map[$port['uniqid']]] = $phone;
	} else {
		$config['analog']['phone'][] = $phone;
	}
    
	write_config();
	touch($g['analog_dirty_path']);
}




function analog_autoconfigure_providers() {
	global $g;

	$all_ports = dahdi_get_ports("analog", "fxo");

	foreach ($all_ports as $port) {
		if (isset($port['unused'])) {
			$provider = analog_generate_default_provider();
			$provider['port'] = $provider['uniqid'];
			analog_save_provider($provider);

			unset($port['unused']);
			dahdi_save_port($port);
		}
	}

	unlink_if_exists($g['analog_dirty_path']);
	unlink_if_exists($g['dahdi_dirty_path']);
}

function analog_generate_default_provider() {
	global $defaults;

	$provider = array();
	$provider['language'] = $defaults['accounts']['language'];
	$provider['uniqid'] = "ANALOG-PROVIDER-" . uniqid(rand());

	return $phone;
}

function analog_save_provider($provider) {
	global $g, $config, $uniqid_map;
    
	if (analog_get_provider($port['uniqid'])) {
		$config['analog']['provider'][$uniqid_map[$port['uniqid']]] = $provider;
	} else {
		$config['analog']['provider'][] = $provider;
	}
    
	write_config();
	touch($g['analog_dirty_path']);
}

?>
