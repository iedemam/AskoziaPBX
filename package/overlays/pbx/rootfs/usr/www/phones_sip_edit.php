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
$pgtitle = array(gettext("Phones"), gettext("Edit SIP Account"));


$uniqid = $_GET['uniqid'];
if (isset($_POST['uniqid'])) {
	$uniqid = $_POST['uniqid'];
}
$carryovers[] = "uniqid";


/* grab and sort the sip phones in our config */
if (!is_array($config['sip']['phone']))
	$config['sip']['phone'] = array();

sip_sort_phones();
$a_sipphones = &$config['sip']['phone'];

$pconfig['codec'] = array("ulaw");

$id = $_GET['id'];
if (isset($_POST['id']))
	$id = $_POST['id'];

/* pull current config into pconfig */
if (isset($id) && $a_sipphones[$id]) {
	$pconfig['extension'] = $a_sipphones[$id]['extension'];
	$pconfig['callerid'] = $a_sipphones[$id]['callerid'];
	$pconfig['secret'] = $a_sipphones[$id]['secret'];
	$pconfig['provider'] = $a_sipphones[$id]['provider'];
	$pconfig['voicemailbox'] = $a_sipphones[$id]['voicemailbox'];
	$pconfig['sendcallnotifications'] = isset($a_sipphones[$id]['sendcallnotifications']);
	$pconfig['publicaccess'] = $a_sipphones[$id]['publicaccess'];
	$pconfig['publicname'] = $a_sipphones[$id]['publicname'];
	$pconfig['language'] = $a_sipphones[$id]['language'];
	$pconfig['dtmfmode'] = $a_sipphones[$id]['dtmfmode'];
	$pconfig['natmode'] = $a_sipphones[$id]['natmode'];
	$pconfig['ringlength'] = $a_sipphones[$id]['ringlength'];
	$pconfig['qualify'] = $a_sipphones[$id]['qualify'];
	
	$pconfig['calllimit'] = isset($a_sipphones[$id]['calllimit']) ? $a_sipphones[$id]['calllimit'] : "2";
	$pconfig['busylimit'] = isset($a_sipphones[$id]['busylimit']) ? $a_sipphones[$id]['busylimit'] : "1";
	
	if(!is_array($pconfig['codec'] = $a_sipphones[$id]['codec']))
		$pconfig['codec'] = array("ulaw");
	$pconfig['descr'] = $a_sipphones[$id]['descr'];
	$pconfig['manual-attribute'] = $a_sipphones[$id]['manual-attribute'];
}

if ($_POST) {

	unset($input_errors);
	$_POST['manualattributes'] = split_and_clean_lines($_POST['manualattributes']);
	$pconfig = $_POST;
	parse_str($_POST['a_codecs']);
	parse_str($_POST['v_codecs']);
	$pconfig['codec'] = array_merge($ace, $vce);

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
	if (($_POST['secret'] && !pbx_is_valid_secret($_POST['secret']))) {
		$input_errors[] = gettext("A valid secret must be specified.");
	}
	if (!isset($id) && in_array($_POST['extension'], pbx_get_extensions())) {
		$input_errors[] = gettext("A phone with this extension already exists.");
	}
	if (($_POST['port'] && !verify_is_port($_POST['port']))) {
		$input_errors[] = gettext("A valid port must be specified.");
	}
	if (($_POST['voicemailbox'] && !verify_is_email_address($_POST['voicemailbox']))) {
		$input_errors[] = gettext("A valid e-mail address must be specified.");
	}
	if (($_POST['qualify'] && !verify_is_numericint($_POST['qualify']))) {
		$input_errors[] = gettext("A whole number of seconds must be entered for the \"qualify\" timeout.");
	}
	
	if (($_POST['calllimit'] && !verify_is_numericint($_POST['calllimit']))) {
		$input_errors[] = gettext("A whole number of calls must be entered for the \"call limit.\"");
	}
	if (($_POST['busylimit'] && !verify_is_numericint($_POST['busylimit']))) {
		$input_errors[] = gettext("A whole number of calls must be entered for the \"busy limit.\"");
	}
	if ($_POST['publicname'] && ($msg = verify_is_public_name($_POST['publicname']))) {
		$input_errors[] = $msg;
	}
	if ($msg = verify_manual_attributes($_POST['manualattributes'])) {
		$input_errors[] = $msg;
	}

	// this is a messy fix for properly and encoding the content
	$pconfig['manual-attribute'] = array_map("base64_encode", $_POST['manualattributes']);

	if (!$input_errors) {
		$sp = array();
		$sp['extension'] = $_POST['extension'];
		$sp['callerid'] = $_POST['callerid'];
		$sp['secret'] = $_POST['secret'];
		$sp['voicemailbox'] = verify_non_default($_POST['voicemailbox']);
		$sp['sendcallnotifications'] = $_POST['sendcallnotifications'] ? true : false;
		$sp['publicaccess'] = $_POST['publicaccess'];
		$sp['publicname'] = verify_non_default($_POST['publicname']);
		$sp['language'] = $_POST['language'];
		$sp['dtmfmode'] = $_POST['dtmfmode'];
		$sp['natmode'] = verify_non_default($_POST['natmode'], $defaults['sip']['natmode']);
		$sp['ringlength'] = verify_non_default($_POST['ringlength'], $defaults['accounts']['phones']['ringlength']);
		$sp['qualify'] = $_POST['qualify'];

		$sp['manual-attribute'] = array_map("base64_encode", $_POST['manualattributes']);

		// XXX the proper way of setting posted options with default values...
		$sp['calllimit'] = ($_POST['calllimit'] != "2") ? $_POST['calllimit'] : false;
		$sp['busylimit'] = ($_POST['busylimit'] != "1") ? $_POST['busylimit'] : false;
		
		$sp['descr'] = verify_non_default($_POST['descr']);

		$a_providers = pbx_get_providers();
		$sp['provider'] = array();
		foreach ($a_providers as $provider) {
			if($_POST[$provider['uniqid']] == true) {
				$sp['provider'][] = $provider['uniqid'];
			}
		}

		$sp['codec'] = array_merge($ace, $vce);

		if (isset($id) && $a_sipphones[$id]) {
			$sp['uniqid'] = $a_sipphones[$id]['uniqid'];
			$a_sipphones[$id] = $sp;
		 } else {
			$sp['uniqid'] = "SIP-PHONE-" . uniqid(rand());
			$a_sipphones[] = $sp;
		}
		
		touch($d_sipconfdirty_path);
		
		write_config();
		
		header("Location: accounts_phones.php");
		exit;
	}
}

include("fbegin.inc"); ?>
<script type="text/JavaScript">
<!--
	<?=javascript_public_access_editor("functions");?>
	<?=javascript_notifications_editor("functions");?>
	<?=javascript_voicemail_editor("functions");?>
	<?=javascript_codec_selector("functions");?>

	jQuery(document).ready(function(){

		<?=javascript_public_access_editor("ready");?>
		<?=javascript_notifications_editor("ready");?>
		<?=javascript_voicemail_editor("ready");?>
		<?=javascript_advanced_settings("ready");?>
		<?=javascript_generate_passwd("ready");?>
		<?=javascript_codec_selector("ready");?>

	});

//-->
</script>
<?

$colspan = 2;
$form = $pconfig;

d_start("phones_sip_edit.php");


	// General
	d_header(gettext("General"));

	d_field(gettext("Number"), "extension", 20,
		gettext("The number used to dial this phone.") . " " .
		gettext("Use this number as your username."), "required");

	d_field(gettext("Caller ID"), "callerid", 40,
		gettext("Text to be displayed for Caller ID."), "required");

	display_channel_language_selector($form['language'], 2);

	display_phone_ringlength_selector($form['ringlength'], 2);

	d_field(gettext("Description"), "descr", 40,
		gettext("You may enter a description here for your reference (not parsed)."));
	d_spacer();


	// Security
	d_header(gettext("Security"));

	?><tr> 
		<td valign="top" class="vncell"><?=gettext("Password");?></td>
		<td colspan="2" class="vtable">
			<input name="secret" type="password" class="formfld" id="secret" size="40" value="<?=htmlspecialchars($form['secret']);?>"> 
		<? display_passwd_generation(); ?>
            <br><span class="vexpl"><?=gettext("This account's password.");?></span>
		</td>
	</tr><?

	display_public_access_editor($form['publicaccess'], $form['publicname'], 2);

	d_provider_access_selector($form['provider']);
	d_spacer();


	// Call Notifications & Voicemail
	d_header(gettext("Call Notifications & Voicemail"));

	d_notifications_editor($form['emailcallnotify'], $form['emailcallnotifyaddress']);

	d_voicemail_editor($form['vmtoemail'], $form['vmtoemailaddress']);
	d_spacer();


	// Codecs
	d_header(gettext("Codecs"));

	display_audio_codec_selector($form['codec']);

	display_video_codec_selector($form['codec']);
	d_spacer();


	// Advanced SIP Options
	d_header(gettext("Advanced SIP Options"));
	display_natmode_selector($form['natmode'], 2);
	display_qualify_options($form['qualify'], 2);
	display_dtmfmode_selector($form['dtmfmode'], 2);
	display_call_and_busy_limit_selector($form['calllimit'], $form['busylimit'], 2);

?>
	<tr> 
		<td valign="top">&nbsp;</td>
		<td colspan="2">
			<input name="Submit" id="submit" type="submit" class="formbtn" value="<?=gettext("Save");?>">
			<input id="a_codecs" name="a_codecs" type="hidden" value="">
			<input id="v_codecs" name="v_codecs" type="hidden" value="">
			<?php if (isset($id) && $a_sipphones[$id]): ?>
			<input name="id" type="hidden" value="<?=$id;?>"> 
			<?php endif; ?>
		</td>
	</tr>
</table>
</form>
<?
d_scripts();
include("fend.inc");
