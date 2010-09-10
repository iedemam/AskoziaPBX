#!/usr/bin/php
<?php 
/*
	$Id$
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

require("guiconfig.inc");

$pgtitle = array(gettext("Services"), gettext("Voicemail"));

$pconfig = voicemail_get_configuration();

if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;
	$pconfig['user_subject'] = base64_encode($_POST['user_subject']);
	$pconfig['user_body'] = base64_encode($_POST['user_body']);

	/* input validation */
	if ($_POST['user_defined']) {
		$reqdfields = explode(" ", "user_subject user_body");
		$reqdfieldsn = explode(",", "Subject,Body");
	}

	verify_input($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

	if ($_POST['fromaddress'] && !verify_is_email_address($_POST['fromaddress'])) {
		$input_errors[] = gettext("A valid e-mail address must be specified for the \"from address\".");
	}

	if (!$input_errors) {
		$vmconfig['fromaddress'] = $_POST['fromaddress'];
		$vmconfig['maillanguage'] = $_POST['maillanguage'];
		$vmconfig['user_defined'] = isset($_POST['user_defined']);
		$vmconfig['user_subject'] = $pconfig['user_subject'];
		$vmconfig['user_body'] = $pconfig['user_body'];

		voicemail_save_configuration($vmconfig);

		config_lock();
		$retval |= voicemail_conf_generate();
		$retval |= notifications_msmtp_conf_generate();
		config_unlock();

		$retval |= pbx_exec("module reload app_voicemail.so");

		$savemsg = get_std_save_message($retval);
	}
}


include("fbegin.inc"); 

?><script type="text/JavaScript">
<!--
	<?=javascript_userdefined_email_notification("functions");?>

	jQuery(document).ready(function(){

		<?=javascript_userdefined_email_notification("ready");?>
	});

//-->
</script>
<form action="services_voicemail.php" method="post" name="iform" id="iform">
	<table width="100%" border="0" cellpadding="6" cellspacing="0">
		<tr> 
			<td colspan="2" valign="top" class="listtopic"><?=gettext("Voicemail to E-Mail Presentation");?></td>
		</tr>
		<tr>
			<td width="20%" valign="top" class="vncell"><?=gettext("From Address");?></td>
			<td width="80%" class="vtable">
				<input name="fromaddress" type="text" class="formfld" id="fromaddress" size="40" value="<?=htmlspecialchars($pconfig['fromaddress']);?>">
				<br>
				<?=gettext("This will be shown as voicemail service's sending address.");?>
				<br><?=sprintf(
					gettext("Defaults to the e-mail address used in the %s setup."),
					"<a href=\"notifications_email.php\">" .
					gettext("Notifications") . " &raquo; " . gettext("E-Mail") .
					"</a>");?>
			</td>
		</tr>
		<tr> 
			<td width="20%" valign="top" class="vncell"><?=gettext("Language");?></td>
			<td width="80%" class="vtable">
				<select name="maillanguage" class="formfld" id="maillanguage">
				<? foreach($vm_email_languages as $vm_email_language=>$friendly) : ?>
				<option value="<?=$vm_email_language;?>" <?
				if ($vm_email_language == $pconfig['maillanguage'])
					echo "selected"; ?>
				><?=$friendly;?></option>
				<? endforeach; ?>
				</select>
				<br><span class="vexpl"><?=gettext("E-mail notifications for new voicemail will be delivered in this language.");?></span>
				<br><input name="user_defined" type="checkbox" id="user_defined" value="yes" <?php if (isset($pconfig['user_defined'])) echo "checked"; ?>> <?=gettext("User defined e-mail text");?>
				<? display_userdefined_email_notification(base64_decode($pconfig['user_subject']), base64_decode($pconfig['user_body'])); ?>
			</td>
		</tr>
		<tr> 
			<td valign="top">&nbsp;</td>
			<td>
				<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>">
			</td>
		</tr>
	</table>
</form>
<?php include("fend.inc"); ?>
