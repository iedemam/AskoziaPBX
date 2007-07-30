#!/usr/local/bin/php
<?php 
/*
	$Id: phones_iax.php 120 2007-06-07 13:35:39Z michael.iedema $
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

$pgtitle = array("Phones", "ISDN");
require("guiconfig.inc");

if (!is_array($config['isdn']['phone']))
	$config['isdn']['phone'] = array();

isdn_sort_phones();
$a_isdnphones = &$config['isdn']['phone'];


if ($_GET['act'] == "del") {
	if ($a_isdnphones[$_GET['id']]) {
		unset($a_isdnphones[$_GET['id']]);
		write_config();
		touch($d_isdnconfdirty_path);
		header("Location: phones_isdn.php");
		exit;
	}
}

if (file_exists($d_isdnconfdirty_path)) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= isdn_conf_generate();
		$retval |= extensions_conf_generate();
		$retval |= voicemail_conf_generate();
		config_unlock();
		
		$retval |= isdn_reload();
		$retval |= extensions_reload();
		$retval |= voicemail_reload();
	}
	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($d_isdnconfdirty_path);
	}
}
?>

<?php include("fbegin.inc"); ?>
<form action="phones_isdn.php" method="post">
<?php if ($savemsg) print_info_box($savemsg); ?>

<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="15%" class="listhdrr">Extension</td>
		<td width="35%" class="listhdrr">Caller ID</td>
		<td width="40%" class="listhdr">Description</td>
		<td width="10%" class="list"></td>
	</tr>

	<?php $i = 0; foreach ($a_isdnphones as $ip): ?>
	<tr>
		<td class="listlr">
			<?=htmlspecialchars($ip['extension']);?>
		</td>
		<td class="listbg">
			<?=htmlspecialchars($ip['callerid']);?>
		</td>
		<td class="listr">
			<?=htmlspecialchars($ip['descr']);?>&nbsp;
		</td>
		<td valign="middle" nowrap class="list"> <a href="phones_isdn_edit.php?id=<?=$i;?>"><img src="e.gif" title="edit ISDN phone" width="17" height="17" border="0"></a>
           &nbsp;<a href="phones_isdn.php?act=del&id=<?=$i;?>" onclick="return confirm('Do you really want to delete this ISDN phone?')"><img src="x.gif" title="delete ISDN phone" width="17" height="17" border="0"></a></td>
	</tr>
	<?php $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="3"></td>
		<td class="list"> <a href="phones_isdn_edit.php"><img src="plus.gif" title="add ISDN phone" width="17" height="17" border="0"></a></td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
