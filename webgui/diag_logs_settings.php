#!/usr/local/bin/php
<?php 
/*
	$Id$
	part of m0n0wall (http://m0n0.ch/wall)
	
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
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

$pgtitle = array("Diagnostics", "Logs");
require("guiconfig.inc");

$pconfig['reverse'] = isset($config['syslog']['reverse']);
$pconfig['nentries'] = $config['syslog']['nentries'];
$pconfig['remoteserver'] = $config['syslog']['remoteserver'];
$pconfig['system'] = isset($config['syslog']['system']);
$pconfig['asterisk'] = isset($config['syslog']['asterisk']);
$pconfig['cdr'] = isset($config['syslog']['cdr']);
$pconfig['enable'] = isset($config['syslog']['enable']);

if (!$pconfig['nentries'])
	$pconfig['nentries'] = 50;

if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;

	/* input validation */
	if ($_POST['enable'] && !is_ipaddr($_POST['remoteserver'])) {
		$input_errors[] = "A valid IP address must be specified.";
	}
	if (($_POST['nentries'] < 5) || ($_POST['nentries'] > 1000)) {
		$input_errors[] = "Number of log entries to show must be between 5 and 1000.";
	}

	if (!$input_errors) {
		$config['syslog']['reverse'] = $_POST['reverse'] ? true : false;
		$config['syslog']['nentries'] = (int)$_POST['nentries'];
		$config['syslog']['remoteserver'] = $_POST['remoteserver'];
		$config['syslog']['system'] = $_POST['system'] ? true : false;
		$config['syslog']['asterisk'] = $_POST['asterisk'] ? true : false;
		$config['syslog']['cdr'] = $_POST['cdr'] ? true : false;
		$config['syslog']['enable'] = $_POST['enable'] ? true : false;
		
		write_config();
		
		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval = system_syslogd_start();
			config_unlock();
		}
		$savemsg = get_std_save_message($retval);	
	}
}

?>
<?php include("fbegin.inc"); ?>
<script language="JavaScript">
<!--
function enable_change(enable_over) {
	if (document.iform.enable.checked || enable_over) {
		document.iform.remoteserver.disabled = 0;
		document.iform.system.disabled = 0;
		document.iform.asterisk.disabled = 0;
		document.iform.cdr.disabled = 0;
	} else {
		document.iform.remoteserver.disabled = 1;
		document.iform.system.disabled = 1;
		document.iform.asterisk.disabled = 1;
		document.iform.cdr.disabled = 1;
	}
}
// -->
</script>
<form action="diag_logs_settings.php" method="post" name="iform" id="iform">
<?php if ($input_errors) print_input_errors($input_errors); ?>
<?php if ($savemsg) print_info_box($savemsg); ?>

<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav"><?php 

	$tabs = array('System' => 'diag_logs.php',
					'PBX' => 'diag_logs_pbx.php',
					'Calls' => 'diag_logs_calls.php',
					'Settings' => 'diag_logs_settings.php');
	dynamic_tab_menu($tabs);
	
			?></ul>
		</td>
	</tr>
	<tr> 
		<td class="tabcont">
			<table width="100%" border="0" cellpadding="6" cellspacing="0">
				<tr> 
					<td width="22%" valign="top" class="vtable">&nbsp;</td>
					<td width="78%" class="vtable">
						<input name="reverse" type="checkbox" id="reverse" value="yes" <?php if ($pconfig['reverse']) echo "checked"; ?>>
						<strong>Show log entries in reverse order (newest entries 
						on top)</strong>
					</td>
				</tr>
				<tr> 
					<td width="22%" valign="top" class="vtable">&nbsp;</td>
					<td width="78%" class="vtable">
						Number of log entries to show: 
						<input name="nentries" id="nentries" type="text" class="formfld" size="4" value="<?=htmlspecialchars($pconfig['nentries']);?>">
					</td>
				</tr>
				<tr> 
					<td width="22%" valign="top" class="vtable">&nbsp;</td>
					<td width="78%" class="vtable">
						<input name="enable" type="checkbox" id="enable" value="yes" <?php if ($pconfig['enable']) echo "checked"; ?> onClick="enable_change(false)">
						<strong>Enable syslog'ing to remote syslog server</strong>
					</td>
				</tr>
				<tr> 
					<td width="22%" valign="top" class="vncell">Remote syslog server</td>
					<td width="78%" class="vtable">
						<input name="remoteserver" id="remoteserver" type="text" class="formfld" size="20" value="<?=htmlspecialchars($pconfig['remoteserver']);?>"> 
						<br>
						IP address of remote syslog server
						<br>
						<br>
						<input name="system" id="system" type="checkbox" value="yes" onclick="enable_change(false)" <?php if ($pconfig['system']) echo "checked"; ?>>system events
						<br>
						<input name="asterisk" id="asterisk" type="checkbox" value="yes" onclick="enable_change(false)" <?php if ($pconfig['asterisk']) echo "checked"; ?>>asterisk events
						<br>
						<input name="cdr" id="cdr" type="checkbox" value="yes" onclick="enable_change(false)" <?php if ($pconfig['cdr']) echo "checked"; ?>>call detail records
					</td>
				</tr>
				<tr> 
					<td width="22%" valign="top">&nbsp;</td>
					<td width="78%">
						<input name="Submit" type="submit" class="formbtn" value="Save" onclick="enable_change(true)"> 
					</td>
				</tr>
				<tr> 
					<td width="22%" valign="top">&nbsp;</td>
					<td width="78%">
						<strong><span class="red">Note:</span></strong>
						<br>
						syslog sends UDP datagrams to port 514 on the specified 
						remote syslog server. Be sure to set syslogd on the 
						remote server to accept syslog messages from AskoziaPBX. 
					</td>
				</tr>
			</table>
		</td>
	</tr>
</table>
</form>
<script language="JavaScript">
<!--
enable_change(false);
//-->
</script>
<?php include("fend.inc"); ?>
