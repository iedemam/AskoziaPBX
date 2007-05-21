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

$pgtitle = array("Services", "Voicemail");
require("guiconfig.inc");

$vmconfig = &$config['voicemail'];

$pconfig['host'] = $vmconfig['host'];
$pconfig['address'] = $vmconfig['address'];
$pconfig['username'] = $vmconfig['username'];
$pconfig['password'] = $vmconfig['password'];
$pconfig['port'] = $vmconfig['port'];
$pconfig['tls'] = $vmconfig['tls'];
$pconfig['fromaddress'] = $vmconfig['fromaddress'];

if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;

	/* input validation */
	$reqdfields = explode(" ", "host address username password");
	$reqdfieldsn = explode(",", "Host,E-mail Address,Username,Password");
	
	do_input_validation($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

	if (!$input_errors) {
		$vmconfig['host'] = $_POST['host'];
		$vmconfig['address'] = $_POST['address'];
		$vmconfig['username'] = $_POST['username'];
		$vmconfig['password'] = $_POST['password'];
		$vmconfig['port'] = $_POST['port'];
		$vmconfig['tls'] = $_POST['tls'];
		$vmconfig['fromaddress'] = $_POST['fromaddress'];
		
		write_config();
		
		config_lock();
		$retval |= asterisk_voicemail_conf_generate();
		config_unlock();
		
		$retval |= asterisk_voicemail_reload();
		
		$savemsg = get_std_save_message($retval);
	}
}
?>
<?php include("fbegin.inc"); ?>
<?php if ($input_errors) print_input_errors($input_errors); ?>
<?php if ($savemsg) print_info_box($savemsg); ?>
<form action="services_voicemail.php" method="post" name="iform" id="iform">
	<table width="100%" border="0" cellpadding="6" cellspacing="0">
		<tr> 
			<td colspan="2" valign="top" class="listtopic">SMTP</td>
		</tr>
		<tr>
			<td width="20%" valign="top" class="vncellreq">Host</td>
			<td width="80%" class="vtable">
				<input name="host" type="text" class="formfld" id="host" size="40" value="<?=htmlspecialchars($pconfig['host']);?>">
			</td>
		</tr>
		<tr>
			<td valign="top" class="vncellreq">E-mail Address</td>
			<td class="vtable">
				<input name="address" type="text" class="formfld" id="address" size="40" value="<?=htmlspecialchars($pconfig['address']);?>">
			</td>
		</tr>
		<tr>
			<td valign="top" class="vncellreq">Username</td>
			<td class="vtable">
				<input name="username" type="text" class="formfld" id="username" size="20" value="<?=htmlspecialchars($pconfig['username']);?>">
			</td>
		</tr>
		<tr>
			<td valign="top" class="vncellreq">Password</td>
			<td class="vtable">
				<input name="password" type="password" class="formfld" id="password" size="20" value="<?=htmlspecialchars($pconfig['password']);?>">
			</td>
		</tr>
		<tr>
			<td valign="top" class="vncell">Port</td>
			<td class="vtable">
				<input name="port" type="text" class="formfld" id="port" size="20" maxlength="5" value="<?=htmlspecialchars($pconfig['port']);?>">
				<br>
				If your server uses a nonstandard port, enter it here.
			</td>
		</tr>
		<tr> 
			<td valign="top" class="vncell">Options</td>
			<td class="vtable"> 
				<input name="tls" type="checkbox" id="tls" value="yes" <?php if ($pconfig['tls']) echo "checked"; ?>>
            	<strong>Account uses TLS</strong><span class="vexpl">
				<br><span class="red"><strong>Warning: </strong></span>
                TLS certificates are currently not verified.</span>
			</td>
        </tr>
		<tr> 
			<td colspan="2" class="list" height="12"></td>
		</tr>
		<tr> 
			<td colspan="2" valign="top" class="listtopic">Presentation</td>
		</tr>
		<tr>
			<td width="20%" valign="top" class="vncell">From Address</td>
			<td width="80%" class="vtable">
				<input name="fromaddress" type="text" class="formfld" id="fromaddress" size="40" value="<?=htmlspecialchars($pconfig['fromaddress']);?>">
				<br>
				This will be shown as voicemail service's sending address. Defaults to the e-mail account entered above.
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
<?php include("fend.inc"); ?>
