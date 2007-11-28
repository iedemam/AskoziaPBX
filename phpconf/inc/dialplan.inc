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
	
	THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
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

/**
 * Returns all callgroups sorted by name.
 * If no callgroups are defined, the global array is initialized.
 * This should be changed to return by reference to eliminate a lot of repetitive code.
 */
function dialplan_get_callgroups() {
	global $config;
	
	if (!is_array($config['dialplan']['callgroup']))
		$config['dialplan']['callgroup'] = array();
		
	dialplan_sort_callgroups();
		
	return $config['dialplan']['callgroup'];
}

/**
 * Sorts callgroups by name
 */
function dialplan_sort_callgroups() {
	global $config;

	usort($config['dialplan']['callgroup'], "pbx_sort_by_name");	
}

/**
 * Returns a specific callgroup based on its uniqid
 */
function dialplan_get_callgroup($uniqid) {
	global $config, $uniqid_map;

	return $config['dialplan']['callgroup'][$uniqid_map[$uniqid]];
}

/**
 * Returns an array of all assigned callgroup extensions
 * XXX : an isset check may be missing here on 'extension'
 */
function dialplan_get_callgroup_extensions() {
	global $config;
	
	$extensions = array();

	if (is_array($config['dialplan']['callgroup'])) {
		foreach($config['dialplan']['callgroup'] as $group) {
			$extensions[] = $group['extension'];
		}
	}

	return $extensions;
}

/**
 * Returns all applications sorted by extension.
 * If no applications are defined, the global array is initialized.
 * This should be changed to return by reference to eliminate a lot of repetitive code.
 */
function dialplan_get_applications() {
	global $config;
	
	if (!is_array($config['dialplan']['application']))
		$config['dialplan']['application'] = array();
		
	dialplan_sort_applications();
		
	return $config['dialplan']['application'];
}

/**
 * Sorts applications by extension
 */
function dialplan_sort_applications() {
	global $config;

	usort($config['dialplan']['application'], "pbx_sort_by_extension");	
}

/**
 * Returns a specific application based on its uniqid
 */
function dialplan_get_application($uniqid) {
	global $config, $uniqid_map;

	return $config['dialplan']['application'][$uniqid_map[$uniqid]];
}

/**
 * Returns an array of all assigned application extensions
 */
function dialplan_get_application_extensions() {
	global $config;
	
	$extensions = array();

	if (is_array($config['dialplan']['application'])) {
		foreach($config['dialplan']['application'] as $app) {
			$extensions[] = $app['extension'];
		}
	}

	return $extensions;
}

/**
 * Generates features.conf which contains call parking, transfer settings and feature maps
 */
function dialplan_features_conf_generate() {
	global $config;
	
	$parkingconfig = $config['dialplan']['callparking'];
	$pc['parkext'] = isset($parkingconfig['parkext']) ? $parkingconfig['parkext'] : "700";
	$pc['parkposstart'] = isset($parkingconfig['parkposstart']) ? $parkingconfig['parkposstart'] : "701";
	$pc['parkposend'] = isset($parkingconfig['parkposend']) ? $parkingconfig['parkposend'] : "720";
	$pc['parktime'] = isset($parkingconfig['parktime']) ? $parkingconfig['parktime'] : "30";
	
	$featuremapconfig = $config['dialplan']['featuremap'];
	$pc['attendedtransfer'] = isset($featuremapconfig['attendedtransfer']) ? $featuremapconfig['attendedtransfer'] : "**";
	$pc['blindtransfer'] = isset($featuremapconfig['blindtransfer']) ? $featuremapconfig['blindtransfer'] : "##";
	//$pc['disconnect'] = isset($featuremapconfig['disconnect']) ? $featuremapconfig['disconnect'] : "*0";
	$pc['featuredigittimeout'] = isset($featuremapconfig['featuredigittimeout']) ? $featuremapconfig['featuredigittimeout'] : "500";
	$pc['atxfernoanswertimeout'] = isset($featuremapconfig['atxfernoanswertimeout']) ? $featuremapconfig['atxfernoanswertimeout'] : "15";
	$pc['transferdigittimeout'] = isset($featuremapconfig['transferdigittimeout']) ? $featuremapconfig['transferdigittimeout'] : "3";
	
	$fd = fopen("/usr/local/etc/asterisk/features.conf", "w");
	if (!$fd) {
		printf("Error: cannot open features.conf in dialplan_features_conf_generate().\n");
		return 1;
	}	

	$conf = "[general]\n";

	$conf .= "parkext => " . $pc['parkext'] . "\n";
	$conf .= "parkpos => " . $pc['parkposstart'] . "-" . $pc['parkposend'] . "\n";
	$conf .= "context => parkedcalls\n";
	$conf .= "parkingtime => " . $pc['parktime'] . "\n";
	$conf .= "featuredigittimeout = " . $pc['featuredigittimeout'] . "\n";
	$conf .= "atxfernoanswertimeout = " . $pc['atxfernoanswertimeout'] . "\n";
	$conf .= "transferdigittimeout = " . $pc['transferdigittimeout'] . "\n";
	$conf .= "\n";
	
	$conf .= "[featuremap]\n";
	$conf .= "atxfer => " . $pc['attendedtransfer'] . "\n";
	$conf .= "blindxfer => " . $pc['blindtransfer'] . "\n";
//	$conf .= "disconnect => ". $pc['disconnect'] . "\n";
//	$conf .= "automon => *1\n";//                  ; One Touch Record
	
	$conf .= "\n";
	
	fwrite($fd, $conf);
	fclose($fd);
	
	return 0;
}

/**
 * Removes references to a provider from phone elements
 * XXX : should be reimplemented to utilize a global tech array
 */
function dialplan_remove_provider_reference_from_phones($removed_id) {
	
	$technologies = array("sip", "iax", "isdn", "analog");

	foreach ($technologies as $tech) {
		if (is_array($GLOBALS['config'][$tech]['phone'])) {
			$a_phones = &$GLOBALS['config'][$tech]['phone'];
			$n = count($a_phones);
			for ($i = 0; $i < $n; $i++) {
				if (is_array($a_phones[$i]['provider'])) {
					$nn = count($a_phones[$i]['provider']);
					for ($j = 0; $j < $nn; $j++) {
						if ($a_phones[$i]['provider'][$j] == $removed_id) {
							unset($a_phones[$i]['provider'][$j]);
						}
					}
				}
			}
		}
	}
}

/**
 * Removes references to an incoming extension mapping from provider elements
 * XXX : should be reimplemented to utilize a global tech array
 */
function dialplan_remove_incomingextensionmap_reference_from_providers($removed_id) {
	
	$technologies = array("sip", "iax", "isdn", "analog");
	
	foreach ($technologies as $tech) {
		if (is_array($GLOBALS['config'][$tech]['provider'])) {
			$a_providers = &$GLOBALS['config'][$tech]['provider'];
			$n = count($a_providers);
			for ($i = 0; $i < $n; $i++) {
				if (is_array($a_providers[$i]['incomingextensionmap'])) {
					$nn = count($a_providers[$i]['incomingextensionmap']);
					for ($ii = 0; $ii < $nn; $ii++) {
						if ($a_providers[$i]['incomingextensionmap'][$ii]['destinationid'] == $removed_id) {
							unset($a_providers[$i]['incomingextensionmap'][$ii]);
						}
					}
				}
			}
		}
	}	
}

/**
 * Parses out elements for a dialstring from the given pattern.
 */
function dialplan_parse_pattern(&$pattern, &$prefix, &$offset, &$exten) {

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
}

/**
 * Reloads res_features to apply newly generated configuration changes
 */
function dialplan_features_reload() {
	return pbx_exec("module reload res_features.so");
}

?>
