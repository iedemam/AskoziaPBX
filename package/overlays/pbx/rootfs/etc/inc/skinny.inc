<?php
/*
	$Id: skinny.inc 1316 2010-01-08 04:24:39Z michael.iedema $
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

/* include all configuration functions */
require_once("functions.inc");


function skinny_get_phones() {

	if (!is_array($GLOBALS['config']['skinny']['phone'])) {
		$GLOBALS['config']['skinny']['phone'] = array();
	}

	$tmp = $GLOBALS['config']['skinny']['phone'];
	usort($tmp, "pbx_sort_by_extension");

	return $tmp;
}

function skinny_get_phone($uniqid) {
	global $uniqid_map;

	return $GLOBALS['config']['skinny']['phone'][$uniqid_map[$uniqid]];
}

function skinny_get_extensions() {

	$extensions = array();

	if (is_array($GLOBALS['config']['skinny']['phone'])) {
		foreach($GLOBALS['config']['skinny']['phone'] as $phone) {
			$extensions[$phone['uniqid']] = $phone['extension'];
		}
	}

	return $extensions;
}

function skinny_generate_default_phone() {
	global $defaults;

	$phone = array();

	$phone['extension'] = pbx_get_next_default_phone_extension();
	$phone['callerid'] = $defaults['accounts']['phones']['callerid'];
	$phone['language'] = $defaults['accounts']['language'];
	$phone['ringlength'] = $defaults['accounts']['phones']['ringlength'];
	$phone['publicaccess'] = $defaults['accounts']['phones']['publicaccess'];
	$phone['uniqid'] = "SKINNY-PHONE-" . uniqid(rand());
	$phone['vmpin'] = "0000";

	return $phone;
}

function skinny_verify_phone($post, $errors) {

	$phone = array();

	v_cleanliness(&$post, &$errors);
	v_extension(&$phone, &$post, &$errors);
	v_callerid(&$phone, &$post, &$errors);
	v_optional('descr', &$phone, &$post);
	v_public_access_editor(&$phone, &$post, &$errors);
	v_notifications_editor(&$phone, &$post, &$errors);
	v_voicemail_editor(&$phone, &$post, &$errors);
	v_provider_access_selector(&$phone, &$post);
	v_outgoing_callerid_map(&$phone, &$post, &$errors);
	v_manualattributes(&$phone, &$post, &$errors);

	$trusted = array(
		'uniqid',
		'port',
		'language',
		'device',			// <-- needs to be replaced with a v_* function
		'ringlength'
	);
	foreach ($trusted as $t) {
		$phone[$t] = $post[$t];
	}

	return $phone;
}

function skinny_save_phone($phone) {
	global $g, $uniqid_map;

	if (skinny_get_phone($phone['uniqid'])) {
		$GLOBALS['config']['skinny']['phone'][$uniqid_map[$phone['uniqid']]] = $phone;
	} else {
		$GLOBALS['config']['skinny']['phone'][] = $phone;
	}

	write_config();
	touch($g['skinny_dirty_path']);
}

function skinny_conf_generate($return=false) {

	$conf = <<<EOD

[general]
bindaddr=0.0.0.0
bindport=2000
dateformat=M-D-Y
keepalive=120


EOD;

	// phones
	$phones = skinny_get_phones();
	foreach($phones as $p) {
		if (isset($p['disabled'])) {
			continue;
		}
		$conf .= "; Caller ID: " . $p['callerid'] . "\n";
		if ($p['descr']) {
			$conf .= "; Description: " . $p['descr'] . "\n";
		}
		$conf .= "[" . $p['extension'] . "]\n";
		$conf .= "callerid=" . $p['callerid'] . " <" . $p['extension'] . ">\n";
		$conf .= "device=" . $p['device'] . "\n";
		$conf .= "context=" . $p['uniqid'] . "\n";
		$conf .= "line => " . $p['extension'] . "\n";
		$conf .= "directmedia=no\n";
		if ($p['manualattributes']) {
			$conf .= "; manual attributes\n";
			$conf .= base64_decode($p['manualattributes']) . "\n";
			$conf .= "; end manual attributes\n";
		}
	
		$conf .= "\n\n";
	}

	return ($return) ? $conf : util_file_write("/etc/asterisk/skinny.conf", $conf);
}

?>
