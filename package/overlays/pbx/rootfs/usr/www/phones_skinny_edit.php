#!/usr/bin/php
<?php 
/*
	$Id: phones_analog_edit.php 1316 2010-01-08 04:24:39Z michael.iedema $
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2010 IKT <http://itison-ikt.de>.
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


require("guiconfig.inc");
$pgtitle = array(gettext("Accounts"), gettext("Edit Skinny Phone"));


if ($_POST) {
	unset($input_errors);

	$phone = skinny_verify_phone(&$_POST, &$input_errors);
	if (!$input_errors) {
		skinny_save_phone($phone);
		header("Location: accounts_phones.php");
		exit;
	}
}


$colspan = 1;
$carryovers[] = "uniqid";

$uniqid = $_GET['uniqid'];
if (isset($_POST['uniqid'])) {
	$uniqid = $_POST['uniqid'];
}

if ($_POST) {
	$form = $_POST;
} else if ($uniqid) {
	$form = skinny_get_phone($uniqid);
} else {
	$form = skinny_generate_default_phone();
}


include("fbegin.inc");
d_start("phones_skinny_edit.php");


	// General
	d_header(gettext("General Settings"));

	d_field(gettext("Number"), "extension", 20,
		gettext("The number used to dial this phone."), "required");

	d_field(gettext("Caller ID"), "callerid", 40,
		gettext("Text to be displayed for Caller ID."), "required");

	display_channel_language_selector($form['language']);

	display_phone_ringlength_selector($form['ringlength']);

	d_field(gettext("Device"), "device", 40,
		gettext("The MAC address of this phone."), "required");

	d_field(gettext("Description"), "descr", 40,
		gettext("You may enter a description here for your reference (not parsed)."));
	d_spacer();


	// Security
	d_header(gettext("Security"));

	display_public_access_editor($form['publicaccess'], $form['publicname']);

	d_provider_access_selector($form['provider']);
	d_spacer();


	// Call Notifications & Voicemail
	d_header(gettext("Call Notifications & Voicemail"));

	d_notifications_editor($form['emailcallnotify'], $form['emailcallnotifyaddress']);

	d_voicemail_editor($form['vmtoemail'], $form['vmtoemailaddress']);
	d_spacer();


	// Advanced Options
	d_collapsible(gettext("Advanced Options"));
	d_manualattributes_editor($form['manualattributes']);
	d_collapsible_end();
	d_spacer();


d_submit();
include("fend.inc");
