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

$pgtitle = array("Phones", "SIP");
require("guiconfig.inc");

if (!is_array($config['sip']['phone']))
	$config['sip']['phone'] = array();

sip_sort_phones();
$a_sipphones = &$config['sip']['phone'];


if ($_GET['act'] == "del") {
	if ($a_sipphones[$_GET['id']]) {
		
		$removed_id = $a_sipphones[$_GET['id']]['uniqid'];
		unset($a_sipphones[$_GET['id']]);
		dialplan_remove_incomingextension_reference_from_providers($removed_id);
		
		write_config();
		touch($d_sipconfdirty_path);
		header("Location: phones_sip.php");
		exit;
	}
}

if (file_exists($d_sipconfdirty_path)) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= sip_conf_generate();
		$retval |= extensions_conf_generate();
		$retval |= voicemail_conf_generate();
		config_unlock();
		
		$retval |= sip_reload();
		$retval |= extensions_reload();
		$retval |= voicemail_reload();
	}
	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($d_sipconfdirty_path);
	}
}
?>

<?php include("fbegin.inc"); ?>
<form action="phones_sip.php" method="post">
<?php if ($savemsg) print_info_box($savemsg); ?>

<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="15%" class="listhdrr">Extension</td>
		<td width="35%" class="listhdrr">Caller ID</td>
		<td width="40%" class="listhdr">Description</td>
		<td width="10%" class="list"></td>
	</tr>

	<?php $i = 0; foreach ($a_sipphones as $sp): ?>
	<tr>
		<td class="listlr">
			<?=htmlspecialchars($sp['extension']);?>
		</td>
		<td class="listbg">
			<?=htmlspecialchars($sp['callerid']);?>
		</td>
		<td class="listr">
			<?=htmlspecialchars($sp['descr']);?>&nbsp;
		</td>
		<td valign="middle" nowrap class="list"> <a href="phones_sip_edit.php?id=<?=$i;?>"><img src="e.gif" title="edit SIP phone" width="17" height="17" border="0"></a>
           &nbsp;<a href="phones_sip.php?act=del&id=<?=$i;?>" onclick="return confirm('Do you really want to delete this SIP phone?')"><img src="x.gif" title="delete SIP phone" width="17" height="17" border="0"></a></td>
	</tr>
	<?php $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="3"></td>
		<td class="list"> <a href="phones_sip_edit.php"><img src="plus.gif" title="add SIP phone" width="17" height="17" border="0"></a></td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
