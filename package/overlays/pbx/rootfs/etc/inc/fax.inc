<?php
/*
	$Id: fax.inc 1515 2010-04-30 11:38:34Z michael.iedema $
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
 * Returns an array of all assigned fax machine numbers / extensions
 */
function fax_get_extensions() {
	global $config;

	$extensions = array();

	if (is_array($config['fax']['machine'])) {
		foreach($config['fax']['machine'] as $fax) {
			$extensions[] = $fax['number'];
		}
	}

	return $extensions;
}

/**
 * Returns an array of all assigned conference rooms
 */
function fax_get_machines() {
	global $config;
	
	if (!is_array($config['fax']['machine']))
		$config['fax']['machine'] = array();
		
	fax_sort_machines();
		
	return $config['fax']['machine'];
}

/**
 * Sorts fax machines by their machine number.
 */
function fax_sort_machines() {
	global $config;

	usort($config['fax']['machine'], "_faxcmp");	
}

/* XXX : replace with "sort_by_number_field" */
function _faxcmp($a, $b) {
    if ($a['number'] == $b['number']) {
        return 0;
    }
    return ($a['number'] < $b['number']) ? -1 : 1;
}

/**
 * Returns a specific fax machine based on its uniqid
 */
function fax_get_machine($uniqid) {
	global $config, $uniqid_map;

	return $config['fax']['machine'][$uniqid_map[$uniqid]];
}


?>
