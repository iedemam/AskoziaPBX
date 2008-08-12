#!/usr/local/bin/php
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

$needs_scriptaculous = true;

require("guiconfig.inc");

$pgtitle = array(gettext("Phones"), gettext("Edit External Line"));

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
	$pconfig['novmwhenbusy'] = isset($a_extphones[$id]['novmwhenbusy']);
	$pconfig['allowdirectdial'] = isset($a_extphones[$id]['allowdirectdial']);
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
		$ep['novmwhenbusy'] = $_POST['novmwhenbusy'] ? true : false;
		$ep['allowdirectdial'] = $_POST['allowdirectdial'] ? true : false;
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
?>
<?php include("fbegin.inc"); ?>
<script type="text/JavaScript">
<!--
	<?=javascript_public_direct_dial_editor("functions");?>

	jQuery(document).ready(function(){

		<?=javascript_public_direct_dial_editor("ready");?>
		<?=javascript_advanced_settings("ready");?>

	});

//-->
</script>
<?php if ($input_errors) display_input_errors($input_errors); ?>
	<form action="phones_external_edit.php" method="post" name="iform" id="iform">
		<table width="100%" border="0" cellpadding="6" cellspacing="0">
			<tr> 
				<td width="20%" valign="top" class="vncellreq"><?=gettext("Extension");?></td>
				<td width="80%" class="vtable">
					<input name="extension" type="text" class="formfld" id="extension" size="20" value="<?=htmlspecialchars($pconfig['extension']);?>"> 
					<br><span class="vexpl"><?=gettext("The internal extension used to dial this phone.");?></span>
				</td>
			</tr>
			<tr> 
				<td valign="top" class="vncellreq"><?=gettext("Name");?></td>
				<td class="vtable">
					<input name="name" type="text" class="formfld" id="name" size="40" value="<?=htmlspecialchars($pconfig['name']);?>"> 
					<br><span class="vexpl"><?=gettext("Descriptive name for this phone.");?></span>
				</td>
			</tr>
			<tr> 
				<td valign="top" class="vncellreq"><?=gettext("Dialstring");?></td>
				<td class="vtable">
					<input name="dialstring" type="text" class="formfld" id="dialstring" size="40" value="<?=htmlspecialchars($pconfig['dialstring']);?>"> 
					<br><span class="vexpl"><?=gettext("Number or string to be dialed to reach this telephone. This will be dialed directly; outgoing pattern matching rules do not apply");?></span>
				</td>
			</tr>
			<tr> 
				<td valign="top" class="vncellreq"><?=gettext("Provider");?></td>
				<td class="vtable">
					<select name="dialprovider" class="formfld" id="dialprovider">
						<option></option>
						<? $a_providers = pbx_get_providers(); ?>
						<? foreach ($a_providers as $provider): ?>
						<option value="<?=$provider['uniqid'];?>" <?php 
						if ($provider['uniqid'] == $pconfig['dialprovider']) 
							echo "selected"; ?>><?=htmlspecialchars($provider['name']); ?></option>
						<? endforeach; ?>
						<option value="sipuri" <? if ($pconfig['dialprovider'] == "sipuri") echo "selected"; ?>>SIP URI</option>
						<option value="iaxuri" <? if ($pconfig['dialprovider'] == "iaxuri") echo "selected"; ?>>IAX URI</option>
					</select>
					<br><span class="vexpl"><?=gettext("Outgoing provider to be used to reach this telephone.");?></span>
				</td>
			</tr>
			<? display_call_notifications_editor($pconfig['voicemailbox'], $pconfig['sendcallnotifications'], $pconfig['novmwhenbusy'], 1); ?>
			<? display_public_direct_dial_editor($pconfig['allowdirectdial'], $pconfig['publicname'], 1); ?>
			<? display_channel_language_selector($pconfig['language'], 1); ?>
			<? display_description_field($pconfig['descr'], 1); ?>
			<? display_advanced_settings_begin(1); ?>
			<? display_phone_ringlength_selector($pconfig['ringlength'], 1); ?>
			<? display_advanced_settings_end(); ?>
			<tr> 
				<td valign="top">&nbsp;</td>
				<td>
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>">
					<?php if (isset($id) && $a_extphones[$id]): ?>
					<input name="id" type="hidden" value="<?=$id;?>"> 
					<?php endif; ?>
				</td>
			</tr>
		</table>
	</form>
<?php include("fend.inc"); ?>
