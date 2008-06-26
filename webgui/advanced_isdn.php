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

$pgtitle = array(gettext("Advanced"), gettext("ISDN"));
require("guiconfig.inc");

$isdnconfig = &$config['services']['isdn'];

$pconfig['law'] = 
	isset($isdnconfig['law']) ? $isdnconfig['law'] : "alaw";
$pconfig['nationalprefix'] = 
	isset($isdnconfig['nationalprefix']) ? $isdnconfig['nationalprefix'] : "0";
$pconfig['internationalprefix'] = 
	isset($isdnconfig['internationalprefix']) ? $isdnconfig['internationalprefix'] : "00";


if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;

	/* input validation */
	$reqdfields = explode(" ", "nationalprefix internationalprefix");
	$reqdfieldsn = explode(",", gettext("National Prefix, International Prefix"));
	
	do_input_validation($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

	// is valid nationalprefix
	if ($_POST['nationalprefix'] && !verify_is_numericint($_POST['nationalprefix'])) {
		$input_errors[] = gettext("A valid national prefix must be specified.");
	}
	// is valid internationalprefix
	if ($_POST['internationalprefix'] && !verify_is_numericint($_POST['internationalprefix'])) {
		$input_errors[] = gettext("A valid international prefix must be specified.");
	}

	if (!$input_errors) {
		$isdnconfig['law'] = $_POST['law'];
		$isdnconfig['nationalprefix'] = $_POST['nationalprefix'];
		$isdnconfig['internationalprefix'] = $_POST['internationalprefix'];
		
		write_config();
		touch($d_isdnconfdirty_path);
		header("Location: advanced_isdn.php");
		exit;
	}
}

if (file_exists($d_isdnconfdirty_path)) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= isdn_conf_generate();
		$retval |= isdn_configure();
		config_unlock();
		
		$retval |= isdn_reload();
	}

	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($d_isdnconfdirty_path);
	}
}

?>
<?php include("fbegin.inc"); ?>
<?php if ($input_errors) print_input_errors($input_errors); ?>
<?php if ($savemsg) print_info_box($savemsg); ?>
<form action="advanced_isdn.php" method="post" name="iform" id="iform">
	<table width="100%" border="0" cellpadding="6" cellspacing="0">
		<tr>
			<td width="20%" valign="top" class="vncell"><?=gettext("National Prefix");?></td>
			<td width="80%" class="vtable">
				<input name="nationalprefix" type="text" class="formfld" id="nationalprefix" size="10" maxlength="5" value="<?=htmlspecialchars($pconfig['nationalprefix']);?>">
			</td>
		</tr>
		<tr>
			<td valign="top" class="vncell"><?=gettext("International Prefix");?></td>
			<td class="vtable">
				<input name="internationalprefix" type="text" class="formfld" id="internationalprefix" size="10" maxlength="5" value="<?=htmlspecialchars($pconfig['internationalprefix']);?>">
			</td>
		</tr>
		<tr>
			<td valign="top" class="vncell"><?=gettext("Audio");?></td>
			<td class="vtable">
				<input name="law" type="radio" value="alaw" 
				<?php if ($pconfig['law'] == "alaw") echo "checked"; ?>><?=gettext("A-law");?>
				&nbsp;&nbsp;&nbsp;
				<input name="law" type="radio" value="ulaw"
				<?php if ($pconfig['law'] == "ulaw") echo "checked"; ?>><?=gettext("u-law");?>
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
