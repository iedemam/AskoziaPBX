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
	
	THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
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

$pgtitle = array(gettext("Advanced"), gettext("SIP"));

$sipconfig = &$config['services']['sip'];

$pconfig['port'] = isset($sipconfig['port']) ? $sipconfig['port'] : "5060";
$pconfig['defaultexpiry'] = isset($sipconfig['defaultexpiry']) ? $sipconfig['defaultexpiry'] : "120";
$pconfig['minexpiry'] = isset($sipconfig['minexpiry']) ? $sipconfig['minexpiry'] : "60";
$pconfig['maxexpiry'] = isset($sipconfig['maxexpiry']) ? $sipconfig['maxexpiry'] : "3600";
$pconfig['disablesrv'] = isset($sipconfig['disablesrv']);
$pconfig['manual-attribute'] = $sipconfig['manual-attribute'];

if ($_POST) {

	unset($input_errors);
	$_POST['manualattributes'] = split_and_clean_lines($_POST['manualattributes']);
	$pconfig = $_POST;

	/* input validation */
	$reqdfields = explode(" ", "port");
	$reqdfieldsn = explode(",", "Port");
	
	verify_input($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

	// is valid port
	if ($_POST['port'] && !verify_is_port($_POST['port'])) {
		$input_errors[] = gettext("A valid port must be specified.");
	}
	// defaultexpiry is numeric
	if ($_POST['defaultexpiry'] && !verify_is_numericint($_POST['defaultexpiry'])) {
		$input_errors[] = gettext("A whole number of seconds must be entered for the \"Default Registration Expiration\" timeout.");
	}
	// minexpiry is numeric
	if ($_POST['minexpiry'] && !verify_is_numericint($_POST['minexpiry'])) {
		$input_errors[] = gettext("A whole number of seconds must be entered for the \"Minimum Registration Expiration\" timeout.");
	}
	// maxexpiry is numeric
	if ($_POST['maxexpiry'] && !verify_is_numericint($_POST['maxexpiry'])) {
		$input_errors[] = gettext("A whole number of seconds must be entered for the \"Maximum Registration Expiration\" timeout.");
	}
	if ($msg = verify_manual_attributes($_POST['manualattributes'])) {
		$input_errors[] = $msg;
	}

	// this is a messy fix for properly and encoding the content
	$pconfig['manual-attribute'] = array_map("base64_encode", $_POST['manualattributes']);

	if (!$input_errors) {
		$sipconfig['port'] = $_POST['port'];
		$sipconfig['defaultexpiry'] = $_POST['defaultexpiry'];
		$sipconfig['minexpiry'] = $_POST['minexpiry'];
		$sipconfig['maxexpiry'] = $_POST['maxexpiry'];
		$sipconfig['disablesrv'] = $_POST['disablesrv'] ? true : false;
		$sipconfig['manual-attribute'] = array_map("base64_encode", $_POST['manualattributes']);

		write_config();
		touch($d_sipconfdirty_path);
		header("Location: advanced_sip.php");
		exit;
	}
}

if (file_exists($d_sipconfdirty_path)) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= sip_conf_generate();
		config_unlock();
		
		$retval |= sip_reload();
	}
	
	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($d_sipconfdirty_path);
	}
}

include("fbegin.inc");
if ($input_errors) display_input_errors($input_errors);
if ($savemsg) display_info_box($savemsg);
?><form action="advanced_sip.php" method="post" name="iform" id="iform">
	<table width="100%" border="0" cellpadding="6" cellspacing="0">
		<tr>
			<td width="20%" valign="top" class="vncell"><?=gettext("Binding Port");?></td>
			<td width="80%" class="vtable">
				<input name="port" type="text" class="formfld" id="port" size="10" maxlength="5" value="<?=htmlspecialchars($pconfig['port']);?>">
			</td>
		</tr>
		<tr> 
			<td valign="top" class="vncell"><?=gettext("Registration Expiration Timeouts");?></td>
			<td class="vtable">
				<?=gettext("min:");?>&nbsp;<input name="minexpiry" type="text" class="formfld" id="minexpiry" size="10" value="<?=htmlspecialchars($pconfig['minexpiry']);?>">&nbsp;&nbsp;<?=gettext("max:");?>&nbsp;<input name="maxexpiry" type="text" class="formfld" id="maxexpiry" size="10" value="<?=htmlspecialchars($pconfig['maxexpiry']);?>">&nbsp;&nbsp;<?=gettext("default:");?>&nbsp;<input name="defaultexpiry" type="text" class="formfld" id="defaultexpiry" size="10" value="<?=htmlspecialchars($pconfig['defaultexpiry']);?>"><br>
				<br><span class="vexpl"><?=gettext("The minimum, maximum and default number of seconds that incoming and outgoing registrations and subscriptions remain valid.");?>
				<br><?=gettext("Default values are 60, 3600 and 120 respectively.");?></span>
			</td>
		</tr>
		<tr> 
			<td valign="top" class="vncell"><?=gettext("DNS Service Records");?></td>
			<td class="vtable">
				<input name="disablesrv" id="disablesrv" type="checkbox" value="yes" <? if ($pconfig['disablesrv']) echo "checked"; ?>><?=gettext("Disable DNS SRV lookups.");?>
			</td>
		</tr>
		<? display_manual_attributes_editor($pconfig['manual-attribute'], 1); ?>
		<tr> 
			<td valign="top">&nbsp;</td>
			<td>
				<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>">
			</td>
		</tr>
	</table>
</form>
<?php include("fend.inc"); ?>
