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
$pgtitle = array(gettext("Phones"), gettext("Edit External Line"));


$uniqid = $_GET['uniqid'];
if (isset($_POST['uniqid'])) {
	$uniqid = $_POST['uniqid'];
}
$carryovers[] = "uniqid";


/* grab and sort the isdn phones in our config */
if (!is_array($config['external']['phone']))
	$config['external']['phone'] = array();

external_sort_phones();
$a_extphones = &$config['external']['phone'];

$id = $_GET['id'];
if (isset($_POST['id']))
	$id = $_POST['id'];

/* pull current config into pconfig */
if (isset($id) && $a_extphones[$id]) {
	$pconfig['extension'] = $a_extphones[$id]['extension'];
	$pconfig['name'] = $a_extphones[$id]['name'];
	$pconfig['dialstring'] = $a_extphones[$id]['dialstring'];
	$pconfig['dialprovider'] = $a_extphones[$id]['dialprovider'];
	$pconfig['voicemailbox'] = $a_extphones[$id]['voicemailbox'];
	$pconfig['sendcallnotifications'] = isset($a_extphones[$id]['sendcallnotifications']);
	$pconfig['publicaccess'] = $a_extphones[$id]['publicaccess'];
	$pconfig['publicname'] = $a_extphones[$id]['publicname'];
	$pconfig['language'] = $a_extphones[$id]['language'];
	$pconfig['descr'] = $a_extphones[$id]['descr'];
	$pconfig['ringlength'] = $a_extphones[$id]['ringlength'];
}

if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;
	
	/* input validation */
	$reqdfields = explode(" ", "extension name dialstring dialprovider");
	$reqdfieldsn = explode(",", "Extension,Name,Dialstring,Provider");
	
	verify_input($_POST, $reqdfields, $reqdfieldsn, &$input_errors);
	/* XXX : other strings should be verified with isset() to prevent false negatives */
	if (isset($_POST['dialstring']) && ($msg = verify_is_dialstring($_POST['dialstring']))) {
		$input_errors[] = $msg;
	}
	if (($_POST['voicemailbox'] && !verify_is_email_address($_POST['voicemailbox']))) {
		$input_errors[] = gettext("A valid e-mail address must be specified.");
	}
	if ($_POST['publicname'] && ($msg = verify_is_public_name($_POST['publicname']))) {
		$input_errors[] = $msg;
	}

	if (!$input_errors) {
		$ep = array();
		$ep['extension'] = $_POST['extension'];
		$ep['name'] = $_POST['name'];
		$ep['dialstring'] = $_POST['dialstring'];
		$ep['dialprovider'] = $_POST['dialprovider'];
		$ep['voicemailbox'] = verify_non_default($_POST['voicemailbox']);
		$ep['sendcallnotifications'] = $_POST['sendcallnotifications'] ? true : false;
		$ep['publicaccess'] = $_POST['publicaccess'];
		$ep['publicname'] = verify_non_default($_POST['publicname']);
		$ep['language'] = $_POST['language'];
		$ep['descr'] = verify_non_default($_POST['descr']);
		$ep['ringlength'] = verify_non_default($_POST['ringlength'], $defaults['accounts']['phones']['ringlength']);

		if (isset($id) && $a_extphones[$id]) {
			$ep['uniqid'] = $a_extphones[$id]['uniqid'];
			$a_extphones[$id] = $ep;
		 } else {
			$ep['uniqid'] = "EXTERNAL-PHONE-" . uniqid(rand());
			$a_extphones[] = $ep;
		}
		
		touch($d_extensionsconfdirty_path);
		
		write_config();
		
		header("Location: accounts_phones.php");
		exit;
	}
}


include("fbegin.inc");

$colspan = 1;
$form = $pconfig;

d_start("phones_external_edit.php");


	// General
	d_header(gettext("General"));

	d_field(gettext("Number"), "extension", 20,
		gettext("The number used to dial this phone."), "required");

	d_field(gettext("Name"), "name", 40,
		gettext("Descriptive name for this phone."), "required");

	display_channel_language_selector($form['language']);
	
	display_phone_ringlength_selector($form['ringlength']);
	
	d_field(gettext("Description"), "descr", 40,
		gettext("You may enter a description here for your reference (not parsed)."));
	d_spacer();


	// Routing
	d_header(gettext("Phone Connectivity"));
?>
	<tr> 
		<td valign="top" class="vncellreq"><?=gettext("Provider");?></td>
		<td class="vtable">
			<select name="dialprovider" class="formfld" id="dialprovider">
				<option></option>
				<? $a_providers = pbx_get_providers(); ?>
				<? foreach ($a_providers as $provider): ?>
				<option value="<?=$provider['uniqid'];?>" <?php 
				if ($provider['uniqid'] == $form['dialprovider']) 
					echo "selected"; ?>><?=htmlspecialchars($provider['name']); ?></option>
				<? endforeach; ?>
				<option value="sipuri" <? if ($form['dialprovider'] == "sipuri") echo "selected"; ?>>SIP URI</option>
				<option value="iaxuri" <? if ($form['dialprovider'] == "iaxuri") echo "selected"; ?>>IAX URI</option>
			</select>
			<br><span class="vexpl"><?=gettext("Outgoing provider used to reach this telephone.");?></span>
		</td>
	</tr>
<?
	d_field(gettext("Dialstring"), "dialstring", 40,
		gettext("Number or string to be dialed to reach this telephone."), "required");
	d_spacer();


	// Call Notifications & Voicemail
	d_header(gettext("Call Notifications & Voicemail"));

	d_notifications_editor($form['emailcallnotify'], $form['emailcallnotifyaddress']);

	d_voicemail_editor($form['vmtoemail'], $form['vmtoemailaddress']);
?>
	<tr> 
		<td valign="top">&nbsp;</td>
		<td>
			<input name="Submit" id="submit" type="submit" class="formbtn" value="<?=gettext("Save");?>">
			<?php if (isset($id) && $a_extphones[$id]): ?>
			<input name="id" type="hidden" value="<?=$id;?>"> 
			<?php endif; ?>
		</td>
	</tr>
</table>
</form>
<?
d_scripts();
include("fend.inc");
