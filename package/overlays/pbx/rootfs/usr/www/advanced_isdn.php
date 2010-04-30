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

$pgtitle = array(gettext("Advanced"), gettext("ISDN"));

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
	$reqdfieldsn = explode(",", "National Prefix, International Prefix");
	
	verify_input($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

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
		touch($g['isdn_dirty_path']);
		header("Location: advanced_isdn.php");
		exit;
	}
}

if (file_exists($g['isdn_dirty_path'])) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= isdn_conf_generate();
		$retval |= isdn_configure();
		config_unlock();
		
		$retval |= pbx_exec("module reload chan_iax2.so");
	}

	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($g['isdn_dirty_path']);
	}
}

include("fbegin.inc"); ?>
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
