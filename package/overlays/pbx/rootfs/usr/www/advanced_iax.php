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

$pgtitle = array(gettext("Advanced"), gettext("IAX"));

$iaxconfig = &$config['services']['iax'];

$pconfig['port'] = isset($iaxconfig['port']) ? $iaxconfig['port'] : "4569";
$pconfig['jbenable'] = isset($iaxconfig['jbenable']);
$pconfig['jbforce'] = isset($iaxconfig['jbforce']);
$pconfig['manual-attribute'] = $iaxconfig['manual-attribute'];

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
	if ($msg = verify_manual_attributes($_POST['manualattributes'])) {
		$input_errors[] = $msg;
	}

	// this is a messy fix for properly and encoding the content
	$pconfig['manual-attribute'] = array_map("base64_encode", $_POST['manualattributes']);

	if (!$input_errors) {
		$iaxconfig['port'] = $_POST['port'];		
		$iaxconfig['jbenable'] = $_POST['jbenable'] ? true : false;
		$iaxconfig['jbforce'] = $_POST['jbforce'] ? true : false;
		$iaxconfig['manual-attribute'] = array_map("base64_encode", $_POST['manualattributes']);
		
		write_config();
		touch($g['iax_dirty_path']);
		header("Location: advanced_iax.php");
		exit;
	}
}

if (file_exists($g['iax_dirty_path'])) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= iax_conf_generate();
		config_unlock();
		
		$retval |= pbx_exec("module reload chan_iax2.so");
	}

	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($g['iax_dirty_path']);
	}
}


include("fbegin.inc");
?><script language="JavaScript">
<!--
function jb_enable_click() {
	if (document.iform.jbenable.checked) {
		document.iform.jbforce.disabled = 0;
	} else {
		document.iform.jbforce.disabled = 1;
		document.iform.jbforce.checked = false;
	}
}
//-->
</script>
<form action="advanced_iax.php" method="post" name="iform" id="iform">
	<table width="100%" border="0" cellpadding="6" cellspacing="0">
		<tr> 
			<td valign="top" colspan="2" class="listtopic"><?=gettext("General");?></td>
		</tr>
		<tr>
			<td width="20%" valign="top" class="vncell"><?=gettext("Binding Port");?></td>
			<td width="80%" class="vtable">
				<input name="port" type="text" class="formfld" id="port" size="10" maxlength="5" value="<?=htmlspecialchars($pconfig['port']);?>">
			</td>
		</tr>
		<? display_manual_attributes_editor($pconfig['manual-attribute'], 1); ?>
		<tr> 
			<td class="list" colspan="2" height="12">&nbsp;</td>
		</tr>
		<tr> 
			<td valign="top" colspan="2" class="listtopic"><?=gettext("Jitterbuffer");?></td>
		</tr>
		<tr> 
			<td valign="top" class="vncell"><?=gettext("Enable");?></td>
			<td class="vtable">
				<input name="jbenable" id="jbenable" type="checkbox" onchange="jb_enable_click()" value="yes" <? if ($pconfig['jbenable']) echo "checked"; ?>><?=gettext("Enable Jitterbuffer on IAX connections terminated by the system.");?>
			</td>
		</tr>
		<tr> 
			<td valign="top" class="vncell"><?=gettext("Force");?></td>
			<td class="vtable">
				<input name="jbforce" id="jbforce" type="checkbox" value="yes" <? if ($pconfig['jbforce']) echo "checked"; ?>><?=gettext("Use Jitterbuffer even when bridging two endpoints.");?>
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
<script language="JavaScript">
<!--
jb_enable_click();
//-->
</script>
<?php include("fend.inc"); ?>
