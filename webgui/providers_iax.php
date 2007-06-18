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

$pgtitle = array("Providers", "IAX");
require("guiconfig.inc");

if (!is_array($config['iax']['provider']))
	$config['iax']['provider'] = array();

iax_sort_providers();
$a_iaxproviders = &$config['iax']['provider'];


if ($_GET['act'] == "del") {
	if ($a_iaxproviders[$_GET['id']]) {
		
		// get the provider's unique id before removal
		$removed_id = $a_iaxproviders[$_GET['id']]['uniqid'];
		unset($a_iaxproviders[$_GET['id']]);
				
		// remove references to this provider from sip phones
		if (is_array($config['sip']['phone'])) {
			$a_sipphones = &$config['sip']['phone'];
			$n = count($a_sipphones);
			for ($i = 0; $i < $n; $i++) {
				if (is_array($a_sipphones[$i]['provider'])) {
					$nn = count($a_sipphones[$i]['provider']);
					for ($j = 0; $j < $nn; $j++) {
						if ($a_sipphones[$i]['provider'][$j] == $removed_id) {
							unset($a_sipphones[$i]['provider'][$j]);
						}
					}
				}
			}
		}
		
		// remove references to this provider from iax phones
		if (is_array($config['iax']['phone'])) {
			$a_iaxphones = &$config['iax']['phone'];
			$n = count($a_iaxphones);
			for ($i = 0; $i < $n; $i++) {
				if (is_array($a_iaxphones[$i]['provider'])) {
					$nn = count($a_iaxphones[$i]['provider']);
					for ($j = 0; $j < $nn; $j++) {
						if ($a_iaxphones[$i]['provider'][$j] == $removed_id) {
							unset($a_iaxphones[$i]['provider'][$j]);
						}
					}

				}
			}
		}


		write_config();
		touch($d_iaxconfdirty_path);
		header("Location: providers_iax.php");
		exit;
	}
}

if (file_exists($d_iaxconfdirty_path)) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= iax_conf_generate();
		$retval |= extensions_conf_generate();
		config_unlock();
		
		$retval |= iax_reload();
		$retval |= extensions_reload();
	}

	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($d_iaxconfdirty_path);
	}
}

?>

<?php include("fbegin.inc"); ?>
<form action="providers_iax.php" method="post">
<?php if ($savemsg) print_info_box($savemsg); ?>

<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="20%" class="listhdrr">Prefix / Pattern</td>
		<td width="25%" class="listhdrr">Name</td>		
		<td width="20%" class="listhdrr">Username</td>
		<td width="25%" class="listhdr">Host</td>
		<td width="10%" class="list"></td>
	</tr>

	<?php $i = 0; foreach ($a_iaxproviders as $sp): ?>
	<tr>
		<td class="listlr"><?
			if ($sp['prefix']) 
				echo htmlspecialchars($sp['prefix']);
			else if ($sp['pattern'])
				echo htmlspecialchars($sp['pattern']);?></td>
		<td class="listbg"><?=htmlspecialchars($sp['name']);?></td>
		<td class="listr"><?=htmlspecialchars($sp['username']);?></td>
		<td class="listr"><?=htmlspecialchars($sp['host']);?>&nbsp;</td>
		<td valign="middle" nowrap class="list"> <a href="providers_iax_edit.php?id=<?=$i;?>"><img src="e.gif" title="edit IAX phone" width="17" height="17" border="0"></a>
           &nbsp;<a href="providers_iax.php?act=del&id=<?=$i;?>" onclick="return confirm('Do you really want to delete this IAX provider?')"><img src="x.gif" title="delete IAX provider" width="17" height="17" border="0"></a></td>
	</tr>
	<?php $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="4"></td>
		<td class="list"> <a href="providers_iax_edit.php"><img src="plus.gif" title="add IAX provider" width="17" height="17" border="0"></a></td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
