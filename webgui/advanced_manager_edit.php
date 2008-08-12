#!/usr/local/bin/php
<?php 
/*
	$Id: services_conferencing_edit.php 120 2007-06-07 13:35:39Z michael.iedema $
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

$pgtitle = array(gettext("Advanced"), gettext("Manager Interface"), gettext("Edit User"));

if (!is_array($config['services']['manager']['manager-user']))
	$config['services']['manager']['manager-user'] = array();

manager_sort_users();
$a_users = &$config['services']['manager']['manager-user'];

$id = $_GET['id'];
if (isset($_POST['id']))
	$id = $_POST['id'];

/* pull current config into pconfig */
if (isset($id) && $a_users[$id]) {
	$pconfig['username'] = $a_users[$id]['username'];
	$pconfig['secret'] = $a_users[$id]['secret'];
	$pconfig['denyip'] = $a_users[$id]['denyip'];
	$pconfig['denynetmask'] = $a_users[$id]['denynetmask'];
	$pconfig['permitip'] = $a_users[$id]['permitip'];
	$pconfig['permitnetmask'] = $a_users[$id]['permitnetmask'];
	$pconfig['read-permission'] = $a_users[$id]['read-permission'];
	$pconfig['write-permission'] = $a_users[$id]['write-permission'];
}


if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;

	/* input validation */
	$reqdfields = explode(" ", "username secret denyip denynetmask permitip permitnetmask");
	$reqdfieldsn = explode(",", "Username,Secret,Deny IP,Deny Netmask,Permit IP,Permit Netmask");

	verify_input($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

	if ($_POST['username'] == "admin") {
		$input_errors[] = gettext("The username of \"admin\" is already used internally by AskoziaPBX.");
	}
	// XXX : check doubled username



	if (!$input_errors) {
		$user = array();
		$user['username'] = $_POST['username'];
		$user['secret'] = $_POST['secret'];
		$user['denyip'] = $_POST['denyip'];
		$user['denynetmask'] = $_POST['denynetmask'];
		$user['permitip'] = $_POST['permitip'];
		$user['permitnetmask'] = $_POST['permitnetmask'];
		
		$keys = array_keys($_POST);
		foreach ($keys as $key) {
			if (strpos($key, "read-") !== false) {
				$read_perms[] = substr($key, strpos($key, "-") + 1);
			} else if (strpos($key, "write-") !== false) {
				$write_perms[] = substr($key, strpos($key, "-") + 1);
			}
		}
		$user['read-permission'] = $read_perms;
		$user['write-permission'] = $write_perms;
		
		if (isset($id) && $a_users[$id]) {
			$a_users[$id] = $user;
		} else {
			$a_users[] = $user;
		}
		
		touch($d_managerconfdirty_path);

		write_config();
		
		header("Location: advanced_manager.php");
		exit;
	}
}

include("fbegin.inc");

if ($input_errors) display_input_errors($input_errors);

	?><form action="advanced_manager_edit.php" method="post" name="iform" id="iform">
		<table width="100%" border="0" cellpadding="6" cellspacing="0">		
			<tr> 
				<td width="20%" valign="top" class="vncellreq"><?=gettext("Username");?></td>
				<td width="80%" class="vtable" colspan="2">
					<input name="username" type="text" class="formfld" id="username" size="20" value="<?=htmlspecialchars($pconfig['username']);?>"> 
					<br><span class="vexpl"></span>
				</td>
			</tr>
			<tr> 
				<td valign="top" class="vncellreq"><?=gettext("Secret");?></td>
				<td colspan="2" class="vtable">
					<input name="secret" type="password" class="formfld" id="secret" size="40" value="<?=htmlspecialchars($pconfig['secret']);?>"> 
                    <br><span class="vexpl"></span>
				</td>
			</tr>
			<tr> 
				<td width="20%" valign="top" class="vncellreq"><?=gettext("Deny Network");?></td>
				<td width="80%" class="vtable" colspan="2">
					<input name="denyip" type="text" class="formfld" id="denyip" size="20" value="<?=htmlspecialchars($pconfig['denyip']);?>">
					/
					<input name="denynetmask" type="text" class="formfld" id="denynetmask" size="20" value="<?=htmlspecialchars($pconfig['denynetmask']);?>">
					<br><span class="vexpl"><?=gettext("set to '0.0.0.0 / 0.0.0.0' to deny everything");?></span>
				</td>
			</tr>
			<tr> 
				<td width="20%" valign="top" class="vncellreq"><?=gettext("Permit Network");?></td>
				<td width="80%" class="vtable" colspan="2">
					<input name="permitip" type="text" class="formfld" id="permitip" size="20" value="<?=htmlspecialchars($pconfig['permitip']);?>">
					/
					<input name="permitnetmask" type="text" class="formfld" id="permitnetmask" size="20" value="<?=htmlspecialchars($pconfig['permitnetmask']);?>">
					<br><span class="vexpl"><?=gettext("set to '0.0.0.0 / 0.0.0.0' to permit everything");?></span>
				</td>
			</tr>
			<tr>
				<td width="20%" valign="top" class="vncellreq"><?=gettext("Permissions");?></td>
				<td width="25%" class="vtable" valign="top">
					<strong><?=gettext("Read");?></strong><br><?
				foreach ($manager_permissions as $perm) {
					?><input name="read-<?=$perm;?>" id="read-<?=$perm;?>" type="checkbox" value="yes" <?
					if (in_array($perm, $pconfig['read-permission'])) echo "checked"; 
					?>><?=$perm;?><br><?
				}
				?></td>
				<td width="55%" class="vtable" valign="top">
					<strong><?=gettext("Write");?></strong><br><?
				foreach ($manager_permissions as $perm) {
					?><input name="write-<?=$perm;?>" id="write-<?=$perm;?>" type="checkbox" value="yes" <?
					if (in_array($perm, $pconfig['write-permission'])) echo "checked"; 
					?>><?=$perm;?><br><?
				}
				?></td>
			</tr>
			<tr> 
				<td valign="top">&nbsp;</td>
				<td colspan="2">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>" onclick="save_codec_states()">
					<?php if (isset($id) && $a_users[$id]): ?>
					<input name="id" type="hidden" value="<?=$id;?>"> 
					<?php endif; ?>
				</td>
			</tr>
		</table>
	</form>
<?php include("fend.inc"); ?>
