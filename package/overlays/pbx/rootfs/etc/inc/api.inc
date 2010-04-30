<?php 
/*
	$Id: api.inc 356 2008-01-02 15:43:21Z michael.iedema $
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

/* gather package information */
$pkg_path = pathinfo($argv[0]);
$tmp = explode("/", $pkg_path['dirname']);
$tmp = $tmp[count($tmp)-1];
$pkg_name = substr($tmp, 0, strpos($tmp, ".pkg"));
$pkg_info = packages_get_package($pkg_name);

/* execute first argument as function */
$pkg_retval = 0;
$f = $argv[1];
if (!function_exists($f)) {
	echo "ERROR: $pkg_name package attempted execution of unimplemented function " . $f . "()!\n";
	exit(1);
} else {
	$pkg_retval |= $f();
	exit($pkg_retval);	
}


function activate() {
	global $pkg_info;

	$ret = 0;

	/* initialize package if this is the first activation */
	if (!file_exists($pkg_info['path'] . "/pkg.initialized")) {
		$ret |= initialize();

		if (!$ret) {
			touch($pkg_info['path'] . "/pkg.initialized");
		}
	}

	if (!$ret) {
		touch($pkg_info['path'] . "/pkg.active");
		$ret |= apply();
	}

	return $ret;
}

function deactivate() {
	global $pkg_info;

	unlink_if_exists($pkg_info['path'] . "/pkg.active");
	apply();

	return 0;
}

function delete() {
	global $pkg_info;

	deactivate();
	mwexec("rm -rf " . $pkg_info['path']);

	return 0;
}

?>
