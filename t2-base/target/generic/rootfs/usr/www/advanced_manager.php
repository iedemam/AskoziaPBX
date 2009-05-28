#!/usr/bin/php
<?php 
/*
	$Id: services_conferencing.php 110 2007-05-29 11:39:10Z michael.iedema $
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

$pgtitle = array(gettext("Advanced"), gettext("Manager Interface"));

if (!is_array($config['services']['manager']['manager-user']))
	$config['services']['manager']['manager-user'] = array();

manager_sort_users();
$a_users = &$config['services']['manager']['manager-user'];

if ($_GET['action'] == "delete") {
	if ($a_users[$_GET['id']]) {
		unset($a_users[$_GET['id']]);
		write_config();
		touch($d_managerconfdirty_path);
		header("Location: advanced_manager.php");
		exit;
	}
}

if (file_exists($d_managerconfdirty_path)) {
	$retval = 0;
	config_lock();
	$retval |= manager_conf_generate();
	config_unlock();

	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($d_managerconfdirty_path);
	}
}

?>

<?php include("fbegin.inc"); ?>
<form action="advanced_manager.php" method="post">
<?php if ($savemsg) display_info_box($savemsg); ?>

<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="20%" class="listhdrr"><?=gettext("Username");?></td>
		<td width="70%" class="listhdrr"><?=gettext("Permissions");?></td>
		<td width="10%" class="list"></td>
	</tr>

	<?php $i = 0; foreach ($a_users as $user): ?>
	<tr>
		<td class="listbgl">
			<?=htmlspecialchars($user['username']);?>
		</td>
		<td class="listr"><?

			if (is_array($user['read-permission'])) {
				sort($user['read-permission']);
				?><strong><?=gettext("read:");?>&nbsp;</strong>&nbsp;<?
				echo htmlspecialchars(implode(", ", $user['read-permission']));
				echo "<br>";
			}
			if (is_array($user['write-permission'])) {
				sort($user['write-permission']);
				?><strong><?=gettext("write:");?>&nbsp;</strong>&nbsp;<?
				echo htmlspecialchars(implode(", ", $user['write-permission']));
			}

		?></td>
		<td valign="middle" nowrap class="list"><a href="advanced_manager_edit.php?id=<?=$i;?>"><img src="edit.png" title="<?=gettext("edit manager room");?>" border="0"></a>
           <a href="?action=delete&id=<?=$i;?>" onclick="return confirm('<?=gettext("Do you really want to delete this manager user?");?>')"><img src="delete.png" title="<?=gettext("delete manager user");?>" border="0"></a>
		</td>
	</tr>
	<?php $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="2"></td>
		<td class="list"><a href="advanced_manager_edit.php"><img src="add.png" title="<?=gettext("add manager user");?>" border="0"></a></td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
