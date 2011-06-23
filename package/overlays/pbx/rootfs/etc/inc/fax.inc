<?php
/*
	$Id: fax.inc 1515 2010-04-30 11:38:34Z michael.iedema $
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2010 tecema (a.k.a IKT) <http://www.tecema.de>.
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

function fax_get_extensions() {
	global $g;

	$extensions = array();

	foreach ($g['fax_types'] as $tech) {
		if (is_array($GLOBALS['config'][$tech]['fax'])) {
			foreach($GLOBALS['config'][$tech]['fax'] as $fax) {
				$extensions[$fax['uniqid']] = $fax['extension'];
			}
		}
	}

	return $extensions;
}

function virtual_verify_fax($post, $errors) {

	$fax = array();

	v_cleanliness(&$post, &$errors);
	v_extension(&$fax, &$post, &$errors);
	v_callerid(&$fax, &$post, &$errors);
	v_email(&$fax, &$post, &$errors);

	$trusted = array(
		'uniqid'
	);
	foreach ($trusted as $t) {
		$fax[$t] = $post[$t];
	}

	return $fax;
}

function virtual_save_fax($fax) {
	global $g, $uniqid_map;

	if (virtual_get_fax($fax['uniqid'])) {
		$GLOBALS['config']['virtual']['fax'][$uniqid_map[$fax['uniqid']]] = $fax;
	} else {
		$GLOBALS['config']['virtual']['fax'][] = $fax;
	}

	write_config();
	touch($g['virtualfax_dirty_path']);
}

function virtual_delete_fax($md5_filename)
{
	if ($disk = storage_service_is_active("faxarchive")) {
		$archivepath = $disk['mountpoint'] . "/askoziapbx/faxarchive/";
		$globber = glob($archivepath . "*.pdf");

		if(strlen($md5_filename) == 0) { return false; }
		
		// No direct delete. Could be a security risk.
		foreach ($globber as $match) {
			if(md5(basename($match)) == $md5_filename)	
			{
				unlink($match);
			}
		}
	} else {
		return false;
	}
	
	return true;
}

function virtual_get_faxes() {

	if (!is_array($GLOBALS['config']['virtual']['fax'])) {
		$GLOBALS['config']['virtual']['fax'] = array();
	}

	$tmp = $GLOBALS['config']['virtual']['fax'];
	usort($tmp, "pbx_sort_by_extension");

	return $tmp;
}

function virtual_get_fax($uniqid) {
	global $uniqid_map;

	return $GLOBALS['config']['virtual']['fax'][$uniqid_map[$uniqid]];
}

function virtual_generate_default_fax() {
	global $defaults;

	$fax = array();

	$fax['extension'] = pbx_get_next_default_phone_extension();
	$fax['name'] = $defaults['accounts']['phones']['callerid'];
	$fax['uniqid'] = "VIRTUAL-FAX-" . uniqid(rand());

	return $fax;
}

?>
