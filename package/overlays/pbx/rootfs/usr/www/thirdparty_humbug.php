#!/usr/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2011 tecema (a.k.a IKT) <http://www.tecema.de>.
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

require("guiconfig.inc");

$pgtitle = array(gettext("Third Party"), gettext("Humbug Call Analytics"));

$form = $GLOBALS['config']['thirdparty']['humbug'];

if ($_POST) {

	unset($input_errors);
	$form = $_POST;

	/* input validation */
	$reqdfields = explode(" ", "apikey encryptionkey");
	$reqdfieldsn = explode(",", "API Key,Encryption Key");

	verify_input($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

	if (!$input_errors) {
		$humbugconfig['apikey'] = $_POST['apikey'];
		$humbugconfig['encryptionkey'] = $_POST['encryptionkey'];
		$humbugconfig['enabled'] = isset($_POST['enabled']);

		$GLOBALS['config']['thirdparty']['humbug'] = $humbugconfig;
		write_config();

		config_lock();
		$retval |= humbug_configure();
		config_unlock();
		
		touch($d_sysrebootreqd_path);

		$savemsg = get_std_save_message($retval);
	}
}

$colspan = 2;

include("fbegin.inc");

	d_start("thirdparty_humbug.php");

		d_header(gettext("Humbug Analytics"));
		
		if (isset($GLOBALS['config']['thirdparty']['humbug']['enabled'])) {
			d_blanklabel(gettext("Your account"), sprintf(gettext("<a href='%s' target='_blank'>Login to your Humbug account.</a>"),"https://analytics.humbuglabs.org/"));
		}
		
		d_checkbox(gettext("Humbug"), "enable Humbug", "enabled", sprintf(gettext("Humbug Labs offer Telecom Analytics & Fraud Detection in the Cloud. <a href='%s' target='_blank'>Click here to sign up.</a>"),"http://humbuglabs.org/askozia"));

		d_field(gettext("API Key"), "apikey", 60, false, "required");
		
		d_field(gettext("Encryption Key"), "encryptionkey", 60, false, "required");

		d_spacer();

	d_submit(gettext("Save"));

include("fend.inc");

?>
