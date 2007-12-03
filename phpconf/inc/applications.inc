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
 * Returns all applications sorted by extension.
 * If no applications are defined, the global array is initialized.
 * This should be changed to return by reference to eliminate a lot of repetitive code.
 */
function applications_get_apps() {
	global $config;
	
	if (!is_array($config['dialplan']['application']))
		$config['dialplan']['application'] = array();
		
	applications_sort_apps();
		
	return $config['dialplan']['application'];
}

/**
 * Sorts applications by extension
 */
function applications_sort_apps() {
	global $config;

	usort($config['dialplan']['application'], "pbx_sort_by_extension");	
}

/**
 * Returns a specific application based on its uniqid
 */
function applications_get_app($uniqid) {
	global $config, $uniqid_map;

	return $config['dialplan']['application'][$uniqid_map[$uniqid]];
}

/**
 * Returns an array of all assigned application extensions
 */
function applications_get_extensions() {
	global $config;
	
	$extensions = array();

	if (is_array($config['dialplan']['application'])) {
		foreach($config['dialplan']['application'] as $app) {
			$extensions[] = $app['extension'];
		}
	}

	return $extensions;
}

?>
