#!/usr/bin/php
<?php 
/*
	$Id: services_voicemail.php 906 2009-05-20 13:41:18Z michael.iedema $
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

$pgtitle = array(gettext("Notifications"), gettext("E-Mail"));
$pghelp = gettext("In order to send missed call notifications and recorded voicemail messages via e-mail, a mail server must be correctly configured here.");

$pconfig = notifications_get_email_configuration();

if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;

	/* input validation */
	$reqdfields = explode(" ", "host address");
	$reqdfieldsn = explode(",", "Host,E-mail Address");

	verify_input($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

	if ($_POST['address'] && !verify_is_email_address($_POST['address'])) {
		$input_errors[] = gettext("A valid e-mail address must be specified.");
	}

	if ($_POST['password'] && !$_POST['username']) {
		$input_errors[] = gettext("A username must be specified.");
	}

	if ($_POST['port'] && !verify_is_port($_POST['port'])) {
		$input_errors[] = gettext("A valid port must be specified.");
	}

	if (!$input_errors) {
		$emailconfig['host'] = $_POST['host'];
		$emailconfig['address'] = $_POST['address'];
		$emailconfig['username'] = $_POST['username'];
		$emailconfig['password'] = $_POST['password'];
		$emailconfig['port'] = $_POST['port'];
		$emailconfig['authtype'] = $_POST['authtype'];
		$emailconfig['enctype'] = $_POST['enctype'];
		$emailconfig['disablecertcheck'] = isset($_POST['disablecertcheck']);

		notifications_save_email_configuration($emailconfig);

		config_lock();
		$retval |= voicemail_conf_generate();
		$retval |= notifications_msmtp_conf_generate();
		config_unlock();
		
		$retval |= pbx_exec("module reload app_voicemail.so");

		if ($_POST['testmail']) {
			$mail_sent = mail(
				$_POST['email_addr'],
				gettext('AskoziaPBX Test E-mail'),
				wordwrap(gettext('Your SMTP settings are working correctly. Voicemail to E-mail and missed-call notifications can now be used.'), 70),
				"From: " . $_POST['address'] . "\r\n" .
				"Date: " . gmdate("D, d M Y H:i:s") . " +0000\r\n"
			);
			if ($mail_sent) {
				$savemsg = gettext("E-mail has been sent successfully.");
			} else {
				$input_errors[] = gettext("E-mail was not sent, check your settings.");
			}
		} else {
			$savemsg = get_std_save_message($retval);
		}
	}
}


include("fbegin.inc"); 
?><form action="notifications_email.php" method="post" name="iform" id="iform">
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<? display_notifications_tab_menu(); ?>
	<tr>
		<td class="tabcont">
			<table width="100%" border="0" cellpadding="6" cellspacing="0">
				<tr>
					<td colspan="2" valign="top" class="listtopic"><?=gettext("SMTP Client Setup");?></td>
				</tr>
				<tr>
					<td width="20%" valign="top" class="vncellreq"><?=gettext("Host");?></td>
					<td width="80%" class="vtable">
						<input name="host" type="text" class="formfld" id="host" size="40" value="<?=htmlspecialchars($pconfig['host']);?>">
						:
						<input name="port" type="text" class="formfld" id="port" size="10" maxlength="5" value="<?=htmlspecialchars($pconfig['port']);?>">
						<br><span class="vexpl"><?=gettext("SMTP host URL or IP address and optional port.");?></span>
					</td>
				</tr>
				<tr>
					<td valign="top" class="vncellreq"><?=gettext("E-mail Address");?></td>
					<td class="vtable">
						<input name="address" type="text" class="formfld" id="address" size="40" value="<?=htmlspecialchars($pconfig['address']);?>">
					</td>
				</tr>
				<tr>
					<td valign="top" class="vncell"><?=gettext("Username");?></td>
					<td class="vtable">
						<input name="username" type="text" class="formfld" id="username" size="40" value="<?=htmlspecialchars($pconfig['username']);?>">
					</td>
				</tr>
				<tr>
					<td valign="top" class="vncell"><?=gettext("Password");?></td>
					<td class="vtable">
						<input name="password" type="password" class="formfld" id="password" size="20" value="<?=htmlspecialchars($pconfig['password']);?>">
					</td>
				</tr><?

				$authtypes = array(
					"on" => "auto",
					"plain" => "plain",
					"login" => "login",
					"cram-md5" => "cram-md5",
					//"digest-md5", (needs gnu sasl)
					//"gssapi", (needs gnu sasl)
					"external" => "external"
					//"ntlm" (needs gnu sasl)
				);

				?><tr>
					<td valign="top" class="vncell"><?=gettext("Authentication Method");?></td>
					<td class="vtable">
						<select name="authtype" class="formfld" id="authtype">
						<? foreach ($authtypes as $authval => $authstring) : ?>
						<option value="<?=$authval;?>" <?
						if ($authval == $pconfig['authtype'])
							echo "selected"; ?>
						><?=$authstring;?></option>
						<? endforeach; ?>
						</select>
						<br><span class="vexpl"><?=gettext("Most accounts will work using 'auto'. If this fails, select an authentication method appropriate for your server.");?></span>
					</td>
				</tr><?

				$enctypes = array(
					"none" => gettext("none"),
					"tls" => "TLS",
					"smtps" => "SMTPS"
				);

				?><tr>
					<td valign="top" class="vncell"><?=gettext("Encryption Method");?></td>
					<td class="vtable">
						<select name="enctype" class="formfld" id="enctype">
						<? foreach ($enctypes as $encval => $encstring) : ?>
						<option value="<?=$encval;?>" <?
						if ($encval == $pconfig['enctype'])
							echo "selected"; ?>
						><?=$encstring;?></option>
						<? endforeach; ?>
						</select>
						<br><input name="disablecertcheck" type="checkbox" class="formfld" id="disablecertcheck" value="yes" <?
						if (isset($pconfig['disablecertcheck'])) echo "checked"; ?>
						>&nbsp;<?=spanify(gettext("disable certificate checking"));?>
					</td>
				</tr>
				<tr>
					<td valign="top" class="vncell"><?=gettext("Test E-mail");?></td>
					<td class="vtable">
						<input name="email_addr" type="text" class="formfld" id="email_addr" size="40" value="">
						<input name="testmail" type="submit" class="formbtn" value="<?=gettext("E-mail Me");?>"><br>
						<?=gettext("Type in an e-mail address and click &quot;E-mail Me&quot; to test your SMTP settings.");?>
					</td>
				</tr>
				<tr>
					<td valign="top">&nbsp;</td>
					<td>
						<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>">
					</td>
				</tr>
			</table>
		</td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
