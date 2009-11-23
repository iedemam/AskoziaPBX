#!/usr/bin/php
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

$pgtitle = array(gettext("Dialplan"), gettext("Call Groups"));
$pghelp = gettext("Groups of extensions can be configured to ring concurrently. These groups can then be dialed via their defined extensions or used as a destination for incoming calls from Providers.");
$pglegend = array("add", "edit", "delete");

// XXX this is_array, sort, reference stuff is all over...
if (!is_array($config['dialplan']['callgroup']))
	$config['dialplan']['callgroup'] = array();

callgroups_sort_groups();
$a_callgroups = &$config['dialplan']['callgroup'];


if ($_GET['action'] == "delete") {
	if ($a_callgroups[$_GET['id']]) {

		$removed_id = $a_callgroups[$_GET['id']]['uniqid'];
		unset($a_callgroups[$_GET['id']]);
		dialplan_remove_incomingextensionmap_reference_from_providers($removed_id);

		write_config();
		touch($d_extensionsconfdirty_path);
		header("Location: dialplan_callgroups.php");
		exit;
	}
}

if (file_exists($d_extensionsconfdirty_path)) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= extensions_conf_generate();
		config_unlock();

		$retval |= pbx_exec("dialplan reload");
	}

	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($d_extensionsconfdirty_path);
	}
}

include("fbegin.inc");
?><form action="dialplan_callgroups.php" method="post">
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="15%" class="listhdrr"><?=gettext("Name");?></td>
		<td width="15%" class="listhdrr"><?=gettext("Extension");?></td>
		<td width="30%" class="listhdrr"><?=gettext("Description");?></td>		
		<td width="30%" class="listhdr"><?=gettext("Members");?></td>
		<td width="10%" class="list"></td>
	</tr>

	<?php $i = 0; foreach ($a_callgroups as $cg): ?>
	<tr>
		<td class="listbgl"><?=htmlspecialchars($cg['name']);?></td>
		<td class="listr"><?=htmlspecialchars($cg['extension']);?>&nbsp;</td>
		<td class="listr"><?=htmlspecialchars($cg['descr']);?>&nbsp;</td>
		<td class="listr"><? 
		if (is_array($cg['groupmember'])) {
			echo @implode(", ", pbx_uniqid_to_name($cg['groupmember']));
		} 
		?>&nbsp;</td>
		<td valign="middle" nowrap class="list"><a href="dialplan_callgroups_edit.php?id=<?=$i;?>"><img src="edit.png" title="<?=gettext("edit call group");?>" border="0"></a>
			<a href="?action=delete&id=<?=$i;?>" onclick="return confirm('<?=gettext("Do you really want to delete this call group?");?>')"><img src="delete.png" title="<?=gettext("delete call group");?>" border="0"></a></td>
	</tr>
	<?php $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="4"></td>
		<td class="list"><a href="dialplan_callgroups_edit.php"><img src="add.png" title="<?=gettext("add call group");?>" border="0"></a></td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
