#!/usr/bin/php
<?php 
/*
	$Id: index.php 1104 2009-09-03 14:54:33Z michael.iedema $
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2009-2010 tecema (a.k.a IKT) <http://www.tecema.de>.
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

function util_dir_to_contents_array($path, &$output) {
	$path = rtrim($path, '/');

	if (is_dir($path)) {
		$handle = opendir($path);
		while ($entry = readdir($handle)) {
			if ($entry != '.' && $entry != '..') {
				util_dir_to_contents_array($path . '/' . $entry, $output);
			}
		}
		closedir($handle);

	} else if (is_file($path)) {
		$output[$path] = file_get_contents($path);

	} else {
		$output[$path] = "path not found!";
	}
}

if ($_GET['genfile']) {
	require_once('functions.inc');

	$gen = file_path_to_generate_function($_GET['genfile']);
	echo ($gen) ? $gen('return') : "unable to load system generated contents... (" . $gen . ")";


} else if ($_GET['getfile']) {
	require_once('JSON.php');
	$json = new Services_JSON();

	require_once('config.inc');
	echo $json->encode(util_file_get($_GET['getfile']));

} else if ($_POST['putfile']) {
	require_once('functions.inc');
	// store in config
	util_file_put($_POST['fullpath'], $_POST['contents'], $_POST['mode']);
	// write out updated file
	$gen = file_path_to_generate_function($_POST['fullpath']);
	if ($gen) {
		$gen();
	}
}

function file_path_to_generate_function($path) {

	if (dirname($path) == '/etc/asterisk') {
		$genfunc = substr(basename($path), 0, -5) . '_conf_generate';
		return function_exists($genfunc) ? $genfunc : false;
	} else if ($path == '/etc/dahdi/system.conf') {
		return 'dahdi_generate_system_conf';
	} else if ($path == '/var/spool/cron/crontabs/root') {
		return 'system_cron_generate';
	}

	return false;
}

?>
