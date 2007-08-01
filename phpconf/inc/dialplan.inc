<?php
/*
	$Id: sip.inc 144 2007-07-04 17:05:34Z michael.iedema $
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


function dialplan_get_callgroups() {
	global $config;
	
	if (!is_array($config['dialplan']['callgroup']))
		$config['dialplan']['callgroup'] = array();
		
	dialplan_sort_callgroups();
		
	return $config['dialplan']['callgroup'];
}

function dialplan_get_callgroup($uniqid) {
	global $config, $uniqid_map;
	return $config['dialplan']['callgroup'][$uniqid_map[$uniqid]];
}

function dialplan_sort_callgroups() {
	global $config;

	usort($config['dialplan']['callgroup'], "_dpcallgroupcmp");	
}
function _dpcallgroupcmp($a, $b) {
	return strcmp($a['name'], $b['name']);
}

function dialplan_get_callgroup_extensions() {
	global $config;
	
	$extensions = array();
	if (is_array($config['dialplan']['callgroup']))
		foreach($config['dialplan']['callgroup'] as $group)
			$extensions[] = $group['extension'];

	return $extensions;
}

function dialplan_callparking_conf_generate() {
	global $config;
	
	$parkingconfig = &$config['dialplan']['callparking'];
	$pc['parkext'] = isset($parkingconfig['parkext']) ? $parkingconfig['parkext'] : "700";
	$pc['parkposstart'] = isset($parkingconfig['parkposstart']) ? $parkingconfig['parkposstart'] : "701";
	$pc['parkposend'] = isset($parkingconfig['parkposend']) ? $parkingconfig['parkposend'] : "720";
	$pc['parktime'] = isset($parkingconfig['parktime']) ? $parkingconfig['parktime'] : "30";
	
	$fd = fopen("/usr/local/etc/asterisk/features.conf", "w");
	if (!$fd) {
		printf("Error: cannot open features.conf in dialplan_callparking_conf_generate().\n");
		return 1;
	}	

	$conf = "[general]\n";

	$conf .= "parkext => " . $pc['parkext'] . "\n";
	$conf .= "parkpos => " . $pc['parkposstart'] . "-" . $pc['parkposend'] . "\n";
	$conf .= "context => parkedcalls\n";
	$conf .= "parkingtime => " . $pc['parktime'] . "\n";
	$conf .= "\n";
	
	fwrite($fd, $conf);
	fclose($fd);
	
	return 0;
}

?>