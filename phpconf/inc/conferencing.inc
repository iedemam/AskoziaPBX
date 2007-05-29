<?php
/*
	$Id: asterisk_sip.inc 75 2007-04-27 10:42:05Z michael.iedema $
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

require_once("functions.inc");

function conferencing_get_extensions() {
	global $config;

	$extensions = array();
	if (is_array($config['conferencing']['room']))
		foreach($config['conferencing']['room'] as $room)
			$extensions[] = $room['number'];

	return $extensions;
}

function conferencing_get_rooms() {
	global $config;
	
	if (!is_array($config['conferencing']['room']))
		$config['conferencing']['room'] = array();
		
	conferencing_sort_rooms();
		
	return $config['conferencing']['room'];
}

function conferencing_get_room($uniqid) {
	$rooms = conferencing_get_rooms();
	foreach ($rooms as $room)
		if($room['uniqid'] == $uniqid)
			return $room;	
}


function conferencing_sort_rooms() {
	global $config;

	usort($config['conferencing']['room'], "_confroomcmp");	
}
function _confroomcmp($a, $b) {
	return strcmp($a['number'], $b['number']);
}


function conferencing_conf_generate() {
	global $config;
	
	$fd = fopen("/usr/local/etc/asterisk/meetme.conf", "w");
	if (!$fd) {
		printf("Error: cannot open meetme.conf in conferencing_conf_generate().\n");
		return 1;
	}	

	$conf = "[general]\n\n".
			"[rooms]\n";

	$rooms = conferencing_get_rooms();
	
	// Usage is conf => confno[,pin][,adminpin]
	foreach ($rooms as $room) {
		//$conf .= "conf => {$room['number']},{$room['pin']},{$room['adminpin']}\n";
		$conf .= "conf => {$room['number']},{$room['pin']}\n";
	}
	
	$conf .= "\n";
	
	fwrite($fd, $conf);
	fclose($fd);
	
	return 0;
}
