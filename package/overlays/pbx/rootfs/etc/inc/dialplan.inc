<?php
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2008 tecema (a.k.a IKT) <http://www.tecema.de>.
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
 * Removes references to a provider from phone elements
 */
function dialplan_remove_provider_reference_from_phones($removed_id) {
	global $g;
	
	foreach ($g['technologies'] as $tech) {
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
 */
function dialplan_remove_incomingextensionmap_reference_from_providers($removed_id) {
	global $g;

	foreach ($g['technologies'] as $tech) {
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
 * Replaces references to an incoming extension mapping from provider elements with an updated uniqid
 */
function dialplan_replace_incomingextensionmap_reference_from_providers($removed_id, $new_id) {
	global $g;

	foreach ($g['technologies'] as $tech) {
		if (is_array($GLOBALS['config'][$tech]['provider'])) {
			$a_providers = &$GLOBALS['config'][$tech]['provider'];
			$n = count($a_providers);
			for ($i = 0; $i < $n; $i++) {
				if (is_array($a_providers[$i]['incomingextensionmap'])) {
					$nn = count($a_providers[$i]['incomingextensionmap']);
					for ($ii = 0; $ii < $nn; $ii++) {
						if ($a_providers[$i]['incomingextensionmap'][$ii]['destinationid'] == $removed_id) {
							$a_providers[$i]['incomingextensionmap'][$ii]['destinationid'] = $new_id;
						}
					}
				}
			}
		}
	}	
}

function dialplan_remove_member_reference_from_callgroups($removed_id) {
	global $g;

	if (is_array($GLOBALS['config']['dialplan']['callgroup'])) {
		$a_callgroups = &$GLOBALS['config']['dialplan']['callgroup'];
		$n = count($a_callgroups);
		for ($i = 0; $i < $n; $i++) {
			if (is_array($a_callgroups[$i]['groupmember'])) {
				$nn = count($a_callgroups[$i]['groupmember']);
				for ($ii = 0; $ii < $nn; $ii++) {
					if ($a_callgroups[$i]['groupmember'][$ii] == $removed_id) {
						unset($a_callgroups[$i]['groupmember'][$ii]);
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

?>
