#!/usr/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2010 IKT <http://itison-ikt.de>.
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

$pgtitle = array(gettext("Accounts"), gettext("Providers"));
$pghelp = gettext("Provider Accounts allow you to configure routes to and from external sources (i.e. VoIP service providers or attached ISDN / Analog interface hardware). Click an account type below to get started.");
$pglegend = array("add", "enabled", "disabled", "edit", "delete");

/* delete */
if ($_GET['action'] == "delete") {
	if(!($msg = pbx_delete_provider($_GET['uniqid']))) {
		$successful_action = true;
	} else {
		$savemsg = $msg;
	}
}

/* disable */
if ($_GET['action'] == "disable") {
	if(!($msg = pbx_disable_provider($_GET['uniqid']))) {
		$successful_action = true;
	} else {
		$savemsg = $msg;
	}
}

/* enable */
if ($_GET['action'] == "enable") {
	if(!($msg = pbx_enable_provider($_GET['uniqid']))) {
		$successful_action = true;
	} else {
		$savemsg = $msg;
	}
}

/* handle successful action */
if ($successful_action) {
	write_config();
	$pieces = explode("-", $_GET['uniqid']);
	$provider_type = strtolower($pieces[0]);
	switch ($provider_type) {
		case "analog":
			touch($g['analog_dirty_path']);
			break;
		case "external":
			touch($d_extensionsconfdirty_path);
			break;
		case "iax":
			touch($g['iax_dirty_path']);
			break;
		case "sip":
			touch($g['sip_dirty_path']);
			break;
		case "isdn":
			touch($g['isdn_dirty_path']);	
			break;
	}
	header("Location: accounts_providers.php");
	exit;	
}

/* dirty sip config? */
if (file_exists($g['sip_dirty_path'])) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= sip_conf_generate();
		$retval |= extensions_conf_generate();
		config_unlock();
		
		$retval |= pbx_exec("module reload chan_sip.so");
		$retval |= pbx_exec("dialplan reload");
	}
	
	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($g['sip_dirty_path']);
	}
}

/* dirty iax config? */
if (file_exists($g['iax_dirty_path'])) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= iax_conf_generate();
		$retval |= extensions_conf_generate();
		config_unlock();
		
		$retval |= pbx_exec("module reload chan_iax2.so");
		$retval |= pbx_exec("dialplan reload");
	}

	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($g['iax_dirty_path']);
	}
}

/* dirty isdn config? */
if (file_exists($g['isdn_dirty_path'])) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= chan_dahdi_conf_generate();
		$retval |= extensions_conf_generate();
		config_unlock();
		
		$retval |= pbx_exec("module reload chan_dahdi.so");
		$retval |= pbx_exec("dahdi restart");
		$retval |= pbx_exec("dialplan reload");
	}

	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($g['isdn_dirty_path']);
	}
}

/* dirty analog config? */
if (file_exists($g['analog_dirty_path'])) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= chan_dahdi_conf_generate();
		$retval |= extensions_conf_generate();
		config_unlock();

		$retval |= pbx_exec("module reload chan_dahdi.so");
		$retval |= pbx_exec("dahdi restart");
		$retval |= pbx_exec("dialplan reload");
	}

	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($g['analog_dirty_path']);
	}
}
?>

<? include("fbegin.inc"); ?>
<script type="text/javascript" charset="utf-8">

	<?=javascript_account_statuses("functions");?>

	jQuery(document).ready(function(){
	
		<? /*=javascript_account_statuses("ready");*/ ?>
	
	});

</script>
<form action="accounts_providers.php" method="post">
<? /* $status_info = pbx_get_peer_statuses(); */ ?>

<table border="0" cellspacing="0" cellpadding="6" width="100%">
	<tr>
		<td class="listhdradd"><img src="add.png">&nbsp;&nbsp;&nbsp;<?
		if (!isset($config['system']['webgui']['hidesip'])) {
			?><a href="providers_sip_edit.php"><?=gettext("SIP");?></a><img src="bullet_add.png">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<?
		}
		if (!isset($config['system']['webgui']['hideiax'])) {
			?><a href="providers_iax_edit.php"><?=gettext("IAX");?></a><img src="bullet_add.png">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<?
		}
		if (!isset($config['system']['webgui']['hideisdn']) &&
			(count(dahdi_get_ports("isdn", "te")) || count(redfone_get_gateways()))) {
			?><a href="providers_isdn_edit.php"><?=gettext("ISDN");?></a><img src="bullet_add.png">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<?
		}
		if (!isset($config['system']['webgui']['hideanalog']) &&
			count(dahdi_get_ports("analog", "fxo"))) {
			?><a href="providers_analog_edit.php"><?=gettext("Analog");?></a><img src="bullet_add.png"><?
		}
		?></td>
	</tr>
	<tr> 
		<td class="list" height="12">&nbsp;</td>
	</tr>
</table>

<? if ($sip_providers = sip_get_providers()) : ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="5%" class="list"></td>
		<td colspan="5" class="listtopiclight"><?=gettext("SIP");?></td>
	</tr>
	<tr>
		<td width="5%" class="list"></td>
		<td width="25%" class="listhdrr"><?=gettext("Name");?></td>
		<td width="20%" class="listhdrr"><?=gettext("Pattern(s)");?></td>
		<td width="20%" class="listhdrr"><?=gettext("Username");?></td>
		<td width="20%" class="listhdr"><?=gettext("Host");?></td>
		<td width="10%" class="list"></td>
	</tr>

	<? $i = 0; foreach ($sip_providers as $p): ?>
	<tr>
		<td valign="middle" nowrap class="list"><?
		if (isset($p['disabled'])) {
			?><a href="?action=enable&uniqid=<?=$p['uniqid'];?>"><img src="disabled.png" title="<?=gettext("click to enable provider");?>" border="0"></a><?
		} else {
			?><a href="?action=disable&uniqid=<?=$p['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to disable this provider?");?>')"><img src="enabled.png" title="<?=gettext("click to disable provider");?>" border="0"></a><?
		}
		?></td>
		<td class="listbgl"><?
		if (!isset($p['disabled'])) {
			echo display_peer_status_icon($status_info[$p['uniqid']], $p['uniqid']);
			echo htmlspecialchars($p['name']);
		} else {
			?><span class="gray"><?=htmlspecialchars($p['name']);?></span><?
		}
		?></td>
		<td class="listr"><?=@implode("<br>", $p['dialpattern']);?>&nbsp;</td>
		<td class="listr"><?=htmlspecialchars($p['username']);?></td>
		<td class="listr"><?=htmlspecialchars($p['host']);?>&nbsp;</td>
		<td valign="middle" nowrap class="list"><a href="providers_sip_edit.php?uniqid=<?=$p['uniqid'];?>"><img src="edit.png" title="<?=gettext("edit provider");?>" border="0"></a>
			<a href="?action=delete&uniqid=<?=$p['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this provider?");?>')"><img src="delete.png" title="<?=gettext("delete provider");?>" border="0"></a></td>
	</tr>
	<? $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="6" height="12">&nbsp;</td>
	</tr>
</table>
<? endif; ?>

<? if ($iax_providers = iax_get_providers()) : ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="5%" class="list"></td>
		<td colspan="5" class="listtopiclight"><?=gettext("IAX");?></td>
	</tr>
	<tr>
		<td width="5%" class="list"></td>
		<td width="25%" class="listhdrr"><?=gettext("Name");?></td>
		<td width="20%" class="listhdrr"><?=gettext("Pattern(s)");?></td>
		<td width="20%" class="listhdrr"><?=gettext("Username");?></td>
		<td width="20%" class="listhdr"><?=gettext("Host");?></td>
		<td width="10%" class="list"></td>
	</tr>

	<? $i = 0; foreach ($iax_providers as $p): ?>
	<tr>
		<td valign="middle" nowrap class="list"><?
		if (isset($p['disabled'])) {
			?><a href="?action=enable&uniqid=<?=$p['uniqid'];?>"><img src="disabled.png" title="<?=gettext("click to enable provider");?>" border="0"></a><?
		} else {
			?><a href="?action=disable&uniqid=<?=$p['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to disable this provider?");?>')"><img src="enabled.png" title="<?=gettext("click to disable provider");?>" border="0"></a><?
		}
		?></td>
		<td class="listbgl"><?
		if (!isset($p['disabled'])) {
			echo display_peer_status_icon($status_info[$p['uniqid']], $p['uniqid']);
			echo htmlspecialchars($p['name']);
		} else {
			?><span class="gray"><?=htmlspecialchars($p['name']);?></span><?
		}
		?></td>
		<td class="listr"><?=@implode("<br>", $p['dialpattern']);?>&nbsp;</td>
		<td class="listr"><?=htmlspecialchars($p['username']);?></td>
		<td class="listr"><?=htmlspecialchars($p['host']);?>&nbsp;</td>
		<td valign="middle" nowrap class="list"><a href="providers_iax_edit.php?uniqid=<?=$p['uniqid'];?>"><img src="edit.png" title="<?=gettext("edit provider");?>" border="0"></a>
			<a href="?action=delete&uniqid=<?=$p['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this provider?");?>')"><img src="delete.png" title="<?=gettext("delete provider");?>" border="0"></a></td>
	</tr>
	<? $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="6" height="12">&nbsp;</td>
	</tr>
</table>
<? endif; ?>

<? if ($isdn_providers = isdn_get_providers()) : ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="5%" class="list"></td>
		<td colspan="5" class="listtopiclight"><?=gettext("ISDN");?></td>
	</tr>
	<tr>
		<td width="5%" class="list"></td>
		<td width="25%" class="listhdrr"><?=gettext("Name");?></td>
		<td width="20%" class="listhdrr"><?=gettext("Pattern(s)");?></td>
		<td width="20%" class="listhdrr"><?=gettext("Main Number");?></td>
		<td width="20%" class="listhdr"><?=gettext("Port");?></td>
		<td width="10%" class="list"></td>
	</tr>

	<? $i = 0; foreach ($isdn_providers as $p): ?>
	<tr>
		<td valign="middle" nowrap class="list"><?
		if (isset($p['disabled'])) {
			?><a href="?action=enable&uniqid=<?=$p['uniqid'];?>"><img src="disabled.png" title="<?=gettext("click to enable provider");?>" border="0"></a><?
		} else {
			?><a href="?action=disable&uniqid=<?=$p['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to disable this provider?");?>')"><img src="enabled.png" title="<?=gettext("click to disable provider");?>" border="0"></a><?
		}
		?></td>
		<td class="listbgl"><?
		if (!isset($p['disabled'])) {
			echo display_peer_status_icon($status_info[$p['uniqid']], $p['uniqid']);
			echo htmlspecialchars($p['name']);
		} else {
			?><span class="gray"><?=htmlspecialchars($p['name']);?></span><?
		}
		?></td>
		<td class="listr"><?=@implode("<br>", $p['dialpattern']);?>&nbsp;</td>
		<td class="listr"><?=htmlspecialchars($p['mainnumber']);?></td><?

		if (strpos($p['port'], "REDFONE") !== false) {
			// in case redfone spans start going into the double-digits, watch out here
			$gwid = substr($p['port'], 0, -2);
			$spannum = substr($p['port'], -1);
			$gateway = redfone_get_gateway($gwid);
			$portname = $gateway['span' . $spannum . 'name'];

		} else if (strpos($p['port'], "DAHDI") !== false) {
			$port = dahdi_get_port($p['port']);
			$portname = $port['name'];
		}

		?><td class="listr"><?=htmlspecialchars($portname);?></td>
		<td valign="middle" nowrap class="list"><a href="providers_isdn_edit.php?uniqid=<?=$p['uniqid'];?>"><img src="edit.png" title="<?=gettext("edit provider");?>" border="0"></a>
			<a href="?action=delete&uniqid=<?=$p['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this provider?");?>')"><img src="delete.png" title="<?=gettext("delete provider");?>" border="0"></a></td>
	</tr>
	<? $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="6" height="12">&nbsp;</td>
	</tr>
</table>
<? endif; ?>

<? if ($analog_providers = analog_get_providers()) : ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="5%" class="list"></td>
		<td colspan="5" class="listtopiclight"><?=gettext("Analog");?></td>
	</tr>
	<tr>
		<td width="5%" class="list"></td>
		<td width="25%" class="listhdrr"><?=gettext("Name");?></td>
		<td width="20%" class="listhdrr"><?=gettext("Pattern(s)");?></td>
		<td width="20%" class="listhdrr"><?=gettext("Number");?></td>
		<td width="20%" class="listhdr"><?=gettext("Port");?></td>
		<td width="10%" class="list"></td>
	</tr>

	<? $i = 0; foreach ($analog_providers as $p): ?>
	<? $port = dahdi_get_port($p['port']); ?>
	<tr>
		<td valign="middle" nowrap class="list"><?
		if (isset($p['disabled'])) {
			?><a href="?action=enable&uniqid=<?=$p['uniqid'];?>"><img src="disabled.png" title="<?=gettext("click to enable provider");?>" border="0"></a><?
		} else {
			?><a href="?action=disable&uniqid=<?=$p['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to disable this provider?");?>')"><img src="enabled.png" title="<?=gettext("click to disable provider");?>" border="0"></a><?
		}
		?></td>
		<td class="listbgl"><?
		if (!isset($p['disabled'])) {
			echo display_peer_status_icon($status_info[$p['uniqid']], $p['uniqid']);
			echo htmlspecialchars($p['name']);
		} else {
			?><span class="gray"><?=htmlspecialchars($p['name']);?></span><?
		}
		?></td>
		<td class="listr"><?=@implode("<br>", $p['dialpattern']);?>&nbsp;</td>
		<td class="listr"><?=htmlspecialchars($p['number']);?></td>
		<td class="listr"><?=htmlspecialchars($port['name']);?></td>
		<td valign="middle" nowrap class="list"><a href="providers_analog_edit.php?uniqid=<?=$p['uniqid'];?>"><img src="edit.png" title="<?=gettext("edit provider");?>" border="0"></a>
			<a href="?action=delete&uniqid=<?=$p['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this provider?");?>')"><img src="delete.png" title="<?=gettext("delete provider");?>" border="0"></a></td>
	</tr>
	<? $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="6" height="12">&nbsp;</td>
	</tr>
</table>
<? endif; ?>

</form>
<? include("fend.inc"); ?>
