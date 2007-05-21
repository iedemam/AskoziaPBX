#!/usr/local/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007 IKT <http://itison-ikt.de>.
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

$pgtitle = array("Advanced", "SIP");
require("guiconfig.inc");

$sipconfig = &$config['services']['sip'];

$pconfig['port'] = "5060";
if (isset($sipconfig['port'])) {
	$pconfig['port'] = $sipconfig['port'];	
}

if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;

	/* input validation */
	//$reqdfields = explode(" ", "name username host prefix");
	//$reqdfieldsn = explode(",", "Name,Username,Host,Prefix");
	
	//do_input_validation($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

	if (($_POST['port'] && !is_port($_POST['port']))) {
		$input_errors[] = "A valid port must be specified.";
	}

	if (!$input_errors) {
		$sipconfig['port'] = $_POST['port'];
		
		write_config();
		
		config_lock();
		$retval |= asterisk_sip_conf_generate();
		config_unlock();
		
		$retval |= asterisk_sip_reload();
		
		$savemsg = get_std_save_message($retval);
		
		header("Location: advanced_sip.php");
		exit;
	}
}
?>
<?php include("fbegin.inc"); ?>
<script language="JavaScript">
<!--
function typesel_change() {

}
//-->
</script>
<?php if ($input_errors) print_input_errors($input_errors); ?>
<?php if ($savemsg) print_info_box($savemsg); ?>
<form action="advanced_sip.php" method="post" name="iform" id="iform">
	<table width="100%" border="0" cellpadding="6" cellspacing="0">
		<tr>
			<td width="20%" valign="top" class="vncell">Binding Port</td>
			<td width="80%" class="vtable">
				<input name="port" type="text" class="formfld" id="port" size="20" maxlength="5" value="<?=htmlspecialchars($pconfig['port']);?>">
			</td>
		</tr>
		<tr> 
			<td valign="top">&nbsp;</td>
			<td>
				<input name="Submit" type="submit" class="formbtn" value="Save">
			</td>
		</tr>
	</table>
</form>
<script language="JavaScript">
<!--
typesel_change();
//-->
</script>
<?php include("fend.inc"); ?>
