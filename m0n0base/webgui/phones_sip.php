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

if (!is_array($config['phones']['sipphone']))
	$config['phones']['sipphone'] = array();

asterisk_sip_sort_phones();
$a_sipphones = &$config['phones']['sipphone'];

if ($_POST) {

	$pconfig = $_POST;

	if ($_POST['apply']) {
		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			// not quite sure what to do here...
			config_lock();
			$retval |= asterisk_sip_conf_generate();
			$retval |= asterisk_sip_reload();
			$retval |= asterisk_extensions_conf_generate();
			$retval |= asterisk_extensions_reload();
			config_unlock();
		}
		$savemsg = get_std_save_message($retval);
		if ($retval == 0) {
			if (file_exists($d_sipconfdirty_path))
				unlink($d_sipconfdirty_path);
		}
	}
}

if ($_GET['act'] == "del") {
	if ($a_sipphones[$_GET['id']]) {
		unset($a_sipphones[$_GET['id']]);
		write_config();
		touch($d_sipconfdirty_path);
		header("Location: phones_sip.php");
		exit;
	}
}

?>

<?php include("fbegin.inc"); ?>
<form action="phones_sip.php" method="post">
<?php if ($input_errors) print_input_errors($input_errors); ?>
<?php if (file_exists($d_sipconfdirty_path)): ?><p>
<?php print_info_box_np("The SIP phones list has been changed.<br>You must apply the changes in order for them to take effect.");?><br>
<input name="apply" type="submit" class="formbtn" id="apply" value="Apply changes"></p>
<?php endif; ?>

<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="15%" class="listhdrr">Extension</td>
		<td width="35%" class="listhdrr">Caller ID</td>
		<td width="40%" class="listhdr">Description</td>
		<td width="10%" class="list"></td>
	</tr>

	<?php $i = 0; foreach ($a_sipphones as $sipphone): ?>
	<tr>
		<td class="listlr">
			<?=htmlspecialchars($sipphone['extension']);?>
		</td>
		<td class="listr">
			<?=htmlspecialchars($sipphone['callerid']);?>
		</td>
		<td class="listbg">
			<?=htmlspecialchars($sipphone['descr']);?>&nbsp;
		</td>
		<td valign="middle" nowrap class="list"> <a href="phones_sip_edit.php?id=<?=$i;?>"><img src="e.gif" title="edit SIP phone" width="17" height="17" border="0"></a>
           &nbsp;<a href="phones_sip.php?act=del&id=<?=$i;?>" onclick="return confirm('Do you really want to delete this SIP phone?"><img src="x.gif" title="delete SIP phone" width="17" height="17" border="0"></a></td>
	</tr>
	<?php $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="3"></td>
		<td class="list"> <a href="phones_sip_edit.php"><img src="plus.gif" title="add SIP phone" width="17" height="17" border="0"></a></td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
