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

$pgtitle = array("Phones", "External");
require("guiconfig.inc");

if (!is_array($config['external']['phone']))
	$config['external']['phone'] = array();

external_sort_phones();
$a_extphones = &$config['external']['phone'];


if ($_GET['act'] == "del") {
	if ($a_extphones[$_GET['id']]) {
		
		$removed_id = $a_extphones[$_GET['id']]['uniqid'];
		unset($a_extphones[$_GET['id']]);
		dialplan_remove_incomingextension_reference_from_providers($removed_id);

		write_config();
		touch($d_extensionsconfdirty_path);
		header("Location: phones_external.php");
		exit;
	}
}

if (file_exists($d_extensionsconfdirty_path)) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= extensions_conf_generate();
		$retval |= voicemail_conf_generate();
		config_unlock();
		
		$retval |= extensions_reload();
		$retval |= voicemail_reload();
	}
	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($d_extensionsconfdirty_path);
	}
}
?>

<?php include("fbegin.inc"); ?>
<form action="phones_external.php" method="post">
<?php if ($savemsg) print_info_box($savemsg); ?>

<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="15%" class="listhdrr">Extension</td>
		<td width="25%" class="listhdrr">Name</td>
		<td width="20%" class="listhdrr">Dialstring</td>
		<td width="30%" class="listhdr">Provider</td>
		<td width="10%" class="list"></td>
	</tr>

	<?php $i = 0; foreach ($a_extphones as $ep): ?>
	<tr>
		<td class="listlr">
			<?=htmlspecialchars($ep['extension']);?>
		</td>
		<td class="listbg">
			<?=htmlspecialchars($ep['name']);?>
		</td>
		<td class="listr">
			<?=htmlspecialchars($ep['dialstring']);?>
		</td>
		<td class="listr">
			<?=htmlspecialchars(asterisk_uniqid_to_name($ep['dialprovider']));?>&nbsp;
		</td>
		<td valign="middle" nowrap class="list"> <a href="phones_external_edit.php?id=<?=$i;?>"><img src="e.gif" title="edit external phone" width="17" height="17" border="0"></a>
           &nbsp;<a href="phones_external.php?act=del&id=<?=$i;?>" onclick="return confirm('Do you really want to delete this external phone?')"><img src="x.gif" title="delete external phone" width="17" height="17" border="0"></a></td>
	</tr>
	<?php $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="4"></td>
		<td class="list"> <a href="phones_external_edit.php"><img src="plus.gif" title="add external phone" width="17" height="17" border="0"></a></td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
