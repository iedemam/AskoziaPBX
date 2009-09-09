<?php
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2009 IKT <http://itison-ikt.de>.
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
 * Returns dahdi_scan output split into an array of span information
 */
function dahdi_scan_to_spans() {
	// get output from dahdi_scan
	exec("dahdi_scan", $out);
	// start our count at -1 to avoid initial condition logic
	$c = -1;
	// for each line of output...
	foreach ($out as $l) {
		// this is a new span definition, bump count
		if ($l[0] == '[') {
			$c++;
		// ...otherwise store span's info
		} else {
			// explode lines in span into value pairs
			$p = explode("=", $l, 2);
			// there can be multiple port lines, store pair in array
			if ($p[0] == "port") {
				$spans[$c][$p[0]][] = $p[1];
			// options lists should be split out into arrays
			} else if ($p[0] == "coding_opts" || $p[0] == "framing_opts") {
				$spans[$c][$p[0]] = explode(",", $p[1]);
			// ...otherwise simply store pair
			} else {
				$spans[$c][$p[0]] = $p[1];
			}
		}
	}
	return $spans;
}

function dahdi_spans_to_isdn_cards($spans) {
	foreach ($spans as $s) {
		if (strpos($s['type'], "digital-") !== false) {
			$cards[md5($s['location'])][] = $s;
		}
	}
	return $cards;
}

function dahdi_spans_to_analog_cards($spans) {
	foreach ($spans as $s) {
		if ($s['type'] == "analog") {
			$cards[md5($s['location'])][] = $s;
		}
	}
	return $cards;
}
