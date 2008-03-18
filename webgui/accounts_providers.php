#!/usr/local/bin/php
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

$pgtitle = array("Accounts", "Providers");
require("guiconfig.inc");

/* delete provider? */
if ($_GET['act'] == "del") {
	if(!($msg = pbx_delete_provider($_GET['id']))) {
		write_config();
		$pieces = explode("-", $_GET['id']);
		$provider_type = strtolower($pieces[0]);
		switch ($provider_type) {
			case "analog":
				touch($d_analogconfdirty_path);
				break;
			case "external":
				touch($d_extensionsconfdirty_path);
				break;
			case "iax":
				touch($d_iaxconfdirty_path);
				break;
			case "sip":
				touch($d_sipconfdirty_path);
				break;
			case "isdn":
				touch($d_isdnconfdirty_path);	
				break;
		}
		header("Location: accounts_providers.php");
		exit;
	} else {
		$savemsg = $msg;	
	}
}

/* dirty sip config? */
if (file_exists($d_sipconfdirty_path)) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= sip_conf_generate();
		$retval |= extensions_conf_generate();
		config_unlock();
		
		$retval |= sip_reload();
		$retval |= extensions_reload();
	}
	
	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($d_sipconfdirty_path);
	}
}

/* dirty iax config? */
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

/* dirty isdn config? */
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

/* dirty analog config? */
if (file_exists($d_analogconfdirty_path)) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= analog_zapata_conf_generate();
		$retval |= extensions_conf_generate();
		config_unlock();
		
		$retval |= analog_reload();
		$retval |= extensions_reload();
	}

	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($d_analogconfdirty_path);
	}
}
?>

<? include("fbegin.inc"); ?>
<form action="accounts_providers.php" method="post">
<? if ($savemsg) print_info_box($savemsg); ?>

<? if (!isset($config['system']['webgui']['hidesip'])) : ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td colspan="5" class="listtopiclight">SIP</td>
	</tr>
	<tr>
		<td width="20%" class="listhdrr">Pattern(s)</td>
		<td width="25%" class="listhdrr">Name</td>		
		<td width="20%" class="listhdrr">Username</td>
		<td width="25%" class="listhdr">Host</td>
		<td width="10%" class="list"></td>
	</tr>
	
	<? $sip_providers = sip_get_providers(); ?>
	<? $i = 0; foreach ($sip_providers as $p): ?>
	<tr>
		<td class="listlr"><?
			$n = count($p['dialpattern']);
			echo htmlspecialchars($p['dialpattern'][0]);
			for($ii = 1; $ii < $n; $ii++) {
				echo "<br>" . htmlspecialchars($p['dialpattern'][$ii]);
			}
		?>&nbsp;</td>
		<td class="listbg"><?=htmlspecialchars($p['name']);?></td>
		<td class="listr"><?=htmlspecialchars($p['username']);?></td>
		<td class="listr"><?=htmlspecialchars($p['host']);?>&nbsp;</td>
		<td valign="middle" nowrap class="list"> <a href="providers_sip_edit.php?id=<?=$i;?>"><img src="e.gif" title="edit SIP phone" width="17" height="17" border="0"></a>
           &nbsp;<a href="accounts_providers.php?act=del&id=<?=$p['uniqid'];?>" onclick="return confirm('Do you really want to delete this SIP provider?')"><img src="x.gif" title="delete SIP provider" width="17" height="17" border="0"></a></td>
	</tr>
	<? $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="4"></td>
		<td class="list"> <a href="providers_sip_edit.php"><img src="plus.gif" title="add SIP provider" width="17" height="17" border="0"></a></td>
	</tr>
	<tr> 
		<td class="list" colspan="5" height="12">&nbsp;</td>
	</tr>
</table>
<? endif; ?>


<? if (!isset($config['system']['webgui']['hideiax'])) : ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td colspan="5" class="listtopiclight">IAX</td>
	</tr>
	<tr>
		<td width="20%" class="listhdrr">Pattern(s)</td>
		<td width="25%" class="listhdrr">Name</td>		
		<td width="20%" class="listhdrr">Username</td>
		<td width="25%" class="listhdr">Host</td>
		<td width="10%" class="list"></td>
	</tr>
	<? $iax_providers = iax_get_providers(); ?>
	<? $i = 0; foreach ($iax_providers as $p): ?>
	<tr>
		<td class="listlr"><?
			$n = count($p['dialpattern']);
			echo htmlspecialchars($p['dialpattern'][0]);
			for($ii = 1; $ii < $n; $ii++) {
				echo "<br>" . htmlspecialchars($p['dialpattern'][$ii]);
			}
		?>&nbsp;</td>
		<td class="listbg"><?=htmlspecialchars($p['name']);?></td>
		<td class="listr"><?=htmlspecialchars($p['username']);?></td>
		<td class="listr"><?=htmlspecialchars($p['host']);?>&nbsp;</td>
		<td valign="middle" nowrap class="list"> <a href="providers_iax_edit.php?id=<?=$i;?>"><img src="e.gif" title="edit IAX provider" width="17" height="17" border="0"></a>
           &nbsp;<a href="accounts_providers.php?act=del&id=<?=$p['uniqid'];?>" onclick="return confirm('Do you really want to delete this IAX provider?')"><img src="x.gif" title="delete IAX provider" width="17" height="17" border="0"></a></td>
	</tr>
	<? $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="4"></td>
		<td class="list"> <a href="providers_iax_edit.php"><img src="plus.gif" title="add IAX provider" width="17" height="17" border="0"></a></td>
	</tr>
	<tr> 
		<td class="list" colspan="5" height="12">&nbsp;</td>
	</tr>
</table>
<? endif; ?>


<? if (!isset($config['system']['webgui']['hideisdn'])) : ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td colspan="5" class="listtopiclight">ISDN</td>
	</tr>
	<tr>
		<td width="20%" class="listhdrr">Pattern(s)</td>
		<td width="25%" class="listhdrr">Name</td>
		<td width="20%" class="listhdrr">Main MSN</td>
		<td width="25%" class="listhdr">Interface</td>
		<td width="10%" class="list"></td>
	</tr>
	<? $isdn_providers = isdn_get_providers(); ?>
	<? $i = 0; foreach ($isdn_providers as $p): ?>
	<? $interface = isdn_get_interface($p['interface']); ?>
	<tr>
		<td class="listlr"><?
			$n = count($p['dialpattern']);
			echo htmlspecialchars($p['dialpattern'][0]);
			for($ii = 1; $ii < $n; $ii++) {
				echo "<br>" . htmlspecialchars($p['dialpattern'][$ii]);
			}
		?>&nbsp;</td>
		<td class="listbg"><?=htmlspecialchars($p['name']);?></td>
		<td class="listr"><?=htmlspecialchars($p['msn']);?></td>
		<td class="listr"><?=htmlspecialchars($interface['name']);?></td>
		<td valign="middle" nowrap class="list"> <a href="providers_isdn_edit.php?id=<?=$i;?>"><img src="e.gif" title="edit ISDN provider" width="17" height="17" border="0"></a>
           &nbsp;<a href="accounts_providers.php?act=del&id=<?=$p['uniqid'];?>" onclick="return confirm('Do you really want to delete this ISDN provider?')"><img src="x.gif" title="delete ISDN provider" width="17" height="17" border="0"></a></td>
	</tr>
	<? $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="4"></td>
		<td class="list"> <a href="providers_isdn_edit.php"><img src="plus.gif" title="add ISDN provider" width="17" height="17" border="0"></a></td>
	</tr>
	<tr> 
		<td class="list" colspan="5" height="12">&nbsp;</td>
	</tr>
</table>
<? endif; ?>


<? if (!isset($config['system']['webgui']['hideanalog'])) : ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td colspan="5" class="listtopiclight">Analog</td>
	</tr>
	<tr>
		<td width="20%" class="listhdrr">Pattern(s)</td>
		<td width="25%" class="listhdrr">Name</td>
		<td width="20%" class="listhdrr">Number</td>
		<td width="25%" class="listhdr">Interface</td>
		<td width="10%" class="list"></td>
	</tr>
	<? $analog_providers = analog_get_providers(); ?>
	<? $i = 0; foreach ($analog_providers as $p): ?>
	<? $interface = analog_get_ab_interface($p['interface']); ?>
	<tr>
		<td class="listlr"><?
			$n = count($p['dialpattern']);
			echo htmlspecialchars($p['dialpattern'][0]);
			for($ii = 1; $ii < $n; $ii++) {
				echo "<br>" . htmlspecialchars($p['dialpattern'][$ii]);
			}
		?>&nbsp;</td>
		<td class="listbg"><?=htmlspecialchars($p['name']);?></td>
		<td class="listr"><?=htmlspecialchars($p['number']);?></td>
		<td class="listr"><?=htmlspecialchars($interface['name']);?></td>
		<td valign="middle" nowrap class="list"> <a href="providers_analog_edit.php?id=<?=$i;?>"><img src="e.gif" title="edit analog provider" width="17" height="17" border="0"></a>
           &nbsp;<a href="accounts_providers.php?act=del&id=<?=$p['uniqid'];?>" onclick="return confirm('Do you really want to delete this analog provider?')"><img src="x.gif" title="delete analog provider" width="17" height="17" border="0"></a></td>
	</tr>
	<? $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="4"></td>
		<td class="list"> <a href="providers_analog_edit.php"><img src="plus.gif" title="add analog provider" width="17" height="17" border="0"></a></td>
	</tr>
	<tr> 
		<td class="list" colspan="5" height="12">&nbsp;</td>
	</tr>
</table>
<? endif; ?>

</form>
<? include("fend.inc"); ?>
