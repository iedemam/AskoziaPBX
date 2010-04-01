#!/usr/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)

	Copyright (C) 2010 IKT <http://itison-ikt.de>.
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
$pgtitle = array(gettext("System"), gettext("Storage"));

if ($_GET['action'] == "forget") { 
	storage_forget_disk($_GET['uniqid']);
	header("Location: system_storage.php"); 
	exit;
}

if ($_GET['action'] == "mount") { 
	storage_mount_disk($_GET['uniqid']);
	header("Location: system_storage.php"); 
	exit;
}

if ($_GET['action'] == "unmount") { 
	storage_unmount_disk($_GET['uniqid']);
	header("Location: system_storage.php"); 
	exit;
}

if (file_exists($g['storage_dirty_path'])) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= storage_configure();
		config_unlock();
	}

	$savemsg = get_std_save_message($retval);
}

$disks = storage_get_disks();

include("fbegin.inc");

?><form action="system_storage.php" method="post">
	<table width="100%" border="0" cellpadding="6" cellspacing="0">
		<tr>
			<td width="20%" class="listhdrr"><?=gettext("Name");?></td>
			<td width="20%" class="listhdrr"><?=gettext("Mount Point");?></td>
			<td width="50%" class="listhdrr"><?=gettext("Services");?></td>
			<td width="10%" class="list"></td>
		</tr><?

	foreach ($disks as $disk) {

		?><tr>
			<td class="listbgl"><?=htmlspecialchars($disk['name']);?></td>
			<td class="listr"><?=htmlspecialchars($disk['mountpoint']);?></td><?
			$services = array();
			if (isset($disk['media'])) {
				$services[] = gettext("Media");
			}
			?><td class="listr"><?=htmlspecialchars(implode(", ", $services));?>&nbsp;</td>
			<td valign="middle" nowrap class="list"><a href="system_storage_edit.php?uniqid=<?=$disk['uniqid'];?>"><img src="edit.png" title="<?=gettext("edit disk");?>" border="0"></a>
			<a href="?action=forget&uniqid=<?=$disk['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to forget this disk?");?>')"><img src="delete.png" title="<?=gettext("forget disk settings");?>" border="0"></a></td>
		</tr><?

	}

		?><tr>
			<td class="list" colspan="3"></td>
			<td class="list"><a href="system_storage_edit.php"><img src="add.png" title="<?=gettext("add disk");?>" border="0"></a></td>
		</tr>
	</table>
</form>
<?php include("fend.inc"); ?>
