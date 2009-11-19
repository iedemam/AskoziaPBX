#!/usr/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2008 IKT <http://itison-ikt.de>.
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

$pgtitle = array(gettext("Phones"), gettext("Edit ISDN Line"));

/* grab and sort the isdn phones in our config */
if (!is_array($config['isdn']['phone']))
	$config['isdn']['phone'] = array();

isdn_sort_phones();
$a_isdnphones = &$config['isdn']['phone'];

$id = $_GET['id'];
if (isset($_POST['id']))
	$id = $_POST['id'];

/* pull current config into pconfig */
if (isset($id) && $a_isdnphones[$id]) {
	$pconfig['extension'] = $a_isdnphones[$id]['extension'];
	$pconfig['callerid'] = $a_isdnphones[$id]['callerid'];
	$pconfig['provider'] = $a_isdnphones[$id]['provider'];
	$pconfig['voicemailbox'] = $a_isdnphones[$id]['voicemailbox'];
	$pconfig['sendcallnotifications'] = isset($a_isdnphones[$id]['sendcallnotifications']);
	$pconfig['publicaccess'] = $a_isdnphones[$id]['publicaccess'];
	$pconfig['publicname'] = $a_isdnphones[$id]['publicname'];
	$pconfig['interface'] = $a_isdnphones[$id]['interface'];
	$pconfig['language'] = $a_isdnphones[$id]['language'];
	$pconfig['descr'] = $a_isdnphones[$id]['descr'];
	$pconfig['ringlength'] = $a_isdnphones[$id]['ringlength'];
}

if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;
	
	/* input validation */
	$reqdfields = explode(" ", "extension callerid");
	$reqdfieldsn = explode(",", "Extension,Caller ID");
	
	verify_input($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

	if (($_POST['extension'] && !pbx_is_valid_extension($_POST['extension']))) {
		$input_errors[] = gettext("A valid extension must be entered.");
	}
	if (($_POST['callerid'] && !pbx_is_valid_callerid($_POST['callerid']))) {
		$input_errors[] = gettext("A valid Caller ID must be specified.");
	}
	if (!isset($id) && in_array($_POST['extension'], pbx_get_extensions())) {
		$input_errors[] = gettext("A phone with this extension already exists.");
	}
	if (($_POST['voicemailbox'] && !verify_is_email_address($_POST['voicemailbox']))) {
		$input_errors[] = gettext("A valid e-mail address must be specified.");
	}
	if ($_POST['publicname'] && ($msg = verify_is_public_name($_POST['publicname']))) {
		$input_errors[] = $msg;
	}

	if (!$input_errors) {
		$ip = array();
		$ip['extension'] = $_POST['extension'];
		$ip['callerid'] = $_POST['callerid'];
		$ip['voicemailbox'] = verify_non_default($_POST['voicemailbox']);
		$ip['sendcallnotifications'] = $_POST['sendcallnotifications'] ? true : false;
		$ip['publicaccess'] = $_POST['publicaccess'];
		$ip['publicname'] = verify_non_default($_POST['publicname']);
		$ip['interface'] = $_POST['interface'];
		$ip['language'] = $_POST['language'];
		$ip['descr'] = verify_non_default($_POST['descr']);
		$ip['ringlength'] = verify_non_default($_POST['ringlength'], $defaults['accounts']['phones']['ringlength']);

		$a_providers = pbx_get_providers();
		$ip['provider'] = array();
		foreach ($a_providers as $provider) {
			if($_POST[$provider['uniqid']] == true) {
				$ip['provider'][] = $provider['uniqid'];
			}
		}
		
		if (isset($id) && $a_isdnphones[$id]) {
			$ip['uniqid'] = $a_isdnphones[$id]['uniqid'];
			$a_isdnphones[$id] = $ip;
		 } else {
			$ip['uniqid'] = "ISDN-PHONE-" . uniqid(rand());
			$a_isdnphones[] = $ip;
		}
		
		touch($d_isdnconfdirty_path);
		
		write_config();
		
		header("Location: accounts_phones.php");
		exit;
	}
}


$colspan = 1;
include("fbegin.inc");

$form = $pconfig; //$form = ($uniqid) ? isdn_get_phone($uniqid) : isdn_generate_default_phone();
d_start("phones_isdn_edit.php");


	// General
	d_header(gettext("General"));

	d_field(gettext("Number"), "extension", 20,
		gettext("The number used to dial this phone."), "required");

	d_field(gettext("Caller ID"), "callerid", 40,
		gettext("Text to be displayed for Caller ID."), "required");

	display_channel_language_selector($form['language']);

	display_phone_ringlength_selector($form['ringlength']);

	//d_hwport_selector("isdn", "asdf");

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


d_submit();
include("fend.inc");
?>
