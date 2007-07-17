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

/*
	isdnconfig - configure ISDN4BSD, version 1.6.0, compiled Jul  9 2007 16:34:01
	usage:  isdnconfig -u <unit> -i <number> -p <protocol> [parameters]
		-u unit         set controller unit (default is zero)
		-c channel      set channel number (default is zero)
		-p enum         set D-channel protocol
		-i value        set D-channel serial number
		-a              activate PH-line
		-D              deactivate PH-line
		-t              set TE-mode
		-n              set NT-mode
		-r              reset device
		-m              set PCM cable unit (default is zero)
		-E enum         display information about enum

*/

function isdn_get_recognized_unit_numbers() {
	global $g;
	
	$dmesg_lines = explode("\n", file_get_contents("{$g['varlog_path']}/dmesg.boot"));
	
	$controllers = array();
	foreach($dmesg_lines as $line) {
		if (strpos($line, "Attaching I4B controller") !== false) {
			preg_match("/\b[0-9]+\b/", $line, &$matches);
			$controllers[$matches[0]] = "found";
		}
	}
	
	return $controllers;
}

function isdn_get_interfaces() {
	global $config;
	
	if (!is_array($config['interfaces']['isdn']))
		$config['interfaces']['isdn'] = array();
		
	isdn_sort_interfaces();
		
	return $config['interfaces']['isdn'];
}

function isdn_sort_interfaces() {
	global $config;

	usort($config['interfaces']['isdn'], "_isdninterfacescmp");	
}
function _isdninterfacescmp($a, $b) {
	return strcmp($a['unit'], $b['unit']);
}

function isdn_configure() {
	
	$res = 0;
	$interfaces = isdn_get_interfaces();
	foreach ($interfaces as $interface) {
		$res |= mwexec("/sbin/isdnconfig -u {$interface['unit']} -p {$interface['mode']} pcm_master");
	}

	return $res;
}


?>
