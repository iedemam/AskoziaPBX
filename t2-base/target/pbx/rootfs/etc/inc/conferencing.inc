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

require_once("functions.inc");

/**
 * Returns an array of all assigned conference room numbers / extensions
 */
function conferencing_get_extensions() {
	global $config;

	$extensions = array();

	if (is_array($config['conferencing']['room'])) {
		foreach($config['conferencing']['room'] as $room) {
			$extensions[] = $room['number'];
		}
	}

	return $extensions;
}

/**
 * Returns an array of all assigned conference rooms
 */
function conferencing_get_rooms() {
	global $config;
	
	if (!is_array($config['conferencing']['room']))
		$config['conferencing']['room'] = array();
		
	conferencing_sort_rooms();
		
	return $config['conferencing']['room'];
}

/**
 * Sorts conference rooms by their room number.
 */
function conferencing_sort_rooms() {
	global $config;

	usort($config['conferencing']['room'], "_confroomcmp");	
}

/* XXX : replace with "sort_by_number_field" */
function _confroomcmp($a, $b) {
    if ($a['number'] == $b['number']) {
        return 0;
    }
    return ($a['number'] < $b['number']) ? -1 : 1;
}

/**
 * Returns a specific conference room based on its uniqid
 */
function conferencing_get_room($uniqid) {
	global $config, $uniqid_map;

	return $config['conferencing']['room'][$uniqid_map[$uniqid]];
}

/**
 * Parses "meetme list room#" output to return an array of its members
 */
function conferencing_get_members($room_number) {
	global $config;
	
	pbx_exec("meetme list $room_number", &$output);

	$members = array();
	$output = explode("\n", $output);
	// inactive conference
	if (count($output) == 1) {
		return $members;
	}
	// remove user count line
	unset($output[count($output)-1]);
	
	$n = count($output);
	for($i = 0; $i < $n; $i++) {
		// strip off "User #: "
		$output[$i] = substr($output[$i], 8);

		// grab user number
		preg_match("/^[0-9]+/", $output[$i], &$matches);
		if ($matches[0][0] == "0") {
			$matches[0] = $matches[0][1];
		}
		$memnum = $matches[0];

		// strip user number and whitespace
		$output[$i] = substr($output[$i], strlen($matches[0]));
		preg_match("/\s+/", $output[$i], &$matches);
		$output[$i] = substr($output[$i], strlen($matches[0]));

		// grab caller id
		$members[$memnum]['callerid'] = trim(substr($output[$i], 0, strpos($output[$i], "Channel: ")));
		
		// strip caller id and "Channel: "
		$output[$i] = substr($output[$i], strpos($output[$i], "Channel: ") + 9);
		
		// grab channel
		preg_match("/\S+/", $output[$i], &$matches);
		$members[$memnum]['channel'] = substr($matches[0], 0, strpos($matches[0], "-"));
		
		// grab connect time
		preg_match("/[0-9]+:[0-9]+:[0-9]+/", $output[$i], &$matches);
		$members[$memnum]['connecttime'] = $matches[0];
	}
	
	return $members;
}

/**
 * Generates meetme.conf which defines conference rooms and their access information
 */
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

?>
