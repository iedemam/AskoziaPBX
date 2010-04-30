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

$pgtitle = array(gettext("Advanced"), gettext("RTP"));

$pconfig['highport'] = rtp_high_port();
$pconfig['lowport'] = rtp_low_port();

if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;

	/* input validation */
	$reqdfields = explode(" ", "lowport highport");
	$reqdfieldsn = explode(",", "Low RTP Port,High RTP Port");
	
	verify_input($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

	if ($_POST['lowport'] && ($msg = rtp_low_port($_POST['lowport']))) {
		$input_errors[] = $msg;
	}
	if ($_POST['highport'] && ($msg = rtp_high_port($_POST['highport']))) {
		$input_errors[] = $msg;
	}
	if ($_POST['highport'] <= $_POST['lowport']) {
		$input_errors[] = gettext("The high RTP port must be greater than the low RTP port.");
	}

	if (!$input_errors) {
		write_config();
		touch($d_rtpconfdirty_path);
		header("Location: advanced_rtp.php");
		exit;
	}
}

if (file_exists($d_rtpconfdirty_path)) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= rtp_conf_generate();
		config_unlock();
		
		$retval |= pbx_configure();
	}
	
	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($d_rtpconfdirty_path);
	}
}
include("fbegin.inc");
?><form action="advanced_rtp.php" method="post" name="iform" id="iform">
	<table width="100%" border="0" cellpadding="6" cellspacing="0">
		<tr> 
			<td width="20%" valign="top" class="vncell"><?=gettext("RTP Port Range");?></td>
			<td width="80%" class="vtable">
				<input name="lowport" type="text" class="formfld" id="lowport" size="20" maxlength="5"  value="<?=htmlspecialchars($pconfig['lowport']);?>">
				-
				<input name="highport" type="text" class="formfld" id="highport" size="20" maxlength="5"  value="<?=htmlspecialchars($pconfig['highport']);?>">
				<br><span class="vexpl"><?=gettext("The port range which RTP streams should use. (default:");?> <?=javascript_default_value_setter("lowport", $defaults['rtp']['lowport']);?>-<?=javascript_default_value_setter("highport", $defaults['rtp']['highport']);?>)</span>
			</td>
		</tr>
		<tr> 
			<td valign="top">&nbsp;</td>
			<td>
				<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>">
			</td>
		</tr>
		<tr>
			<td>&nbsp;</td>
			<td><span class="vexpl"><span class="red"><strong><?=gettext("Warning");?>:<br>
			</strong></span><?=gettext("after you click &quot;Save&quot;, all current calls will be dropped.");?></td>
		</tr>
	</table>
</form>
<?php include("fend.inc"); ?>
