#!/usr/local/bin/php
<?php 
/*
	$Id: providers_iax.php 143 2007-07-03 14:34:07Z michael.iedema $
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

$pgtitle = array("Providers", "ISDN");
require("guiconfig.inc");

if (!is_array($config['isdn']['provider']))
	$config['isdn']['provider'] = array();

isdn_sort_providers();
$a_isdnproviders = &$config['isdn']['provider'];


if ($_GET['act'] == "del") {
	if ($a_isdnproviders[$_GET['id']]) {
		
		dialplan_remove_provider_reference_from_phones($a_isdnproviders[$_GET['id']]['uniqid']);
		unset($a_isdnproviders[$_GET['id']]);

		write_config();
		touch($d_isdnconfdirty_path);
		header("Location: providers_isdn.php");
		exit;
	}
}

if (file_exists($d_isdnconfdirty_path)) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= isdn_conf_generate();
		$retval |= extensions_conf_generate();
		config_unlock();
		
		$retval |= isdn_reload();
		$retval |= extensions_reload();
	}

	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($d_isdnconfdirty_path);
	}
}

?>

<?php include("fbegin.inc"); ?>
<form action="providers_isdn.php" method="post">
<?php if ($savemsg) print_info_box($savemsg); ?>

<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="20%" class="listhdrr">Pattern(s)</td>
		<td width="30%" class="listhdrr">Name</td>
		<td width="15%" class="listhdrr">Interface</td>
		<td width="25%" class="listhdrr">MSN</td>
		<td width="10%" class="list"></td>
	</tr>

	<?php $i = 0; foreach ($a_isdnproviders as $ip): ?>
	<? $interface = isdn_get_interface($ip['interface']); ?>
	<tr>
		<td class="listlr"><?
			$n = count($ip['dialpattern']);
			echo htmlspecialchars($ip['dialpattern'][0]);
			for($ii = 1; $ii < $n; $ii++) {
				echo "<br>" . htmlspecialchars($ip['dialpattern'][$ii]);
			}
		?>&nbsp;</td>
		<td class="listbg"><?=htmlspecialchars($ip['name']);?></td>
		<td class="listr"><?=htmlspecialchars($interface['name']);?></td>
		<td class="listr"><?=htmlspecialchars($ip['msn']);?></td>
		<td valign="middle" nowrap class="list"> <a href="providers_isdn_edit.php?id=<?=$i;?>"><img src="e.gif" title="edit ISDN provider" width="17" height="17" border="0"></a>
           &nbsp;<a href="providers_isdn.php?act=del&id=<?=$i;?>" onclick="return confirm('Do you really want to delete this ISDN provider?')"><img src="x.gif" title="delete ISDN provider" width="17" height="17" border="0"></a></td>
	</tr>
	<?php $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="4"></td>
		<td class="list"> <a href="providers_isdn_edit.php"><img src="plus.gif" title="add ISDN provider" width="17" height="17" border="0"></a></td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
