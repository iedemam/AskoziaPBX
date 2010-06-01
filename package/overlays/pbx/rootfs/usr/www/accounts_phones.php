#!/usr/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2010 tecema (a.k.a IKT) <http://www.tecema.de>.
	All rights reserved.
	
	Askozia®PBX is a registered trademark of tecema. Any unauthorized use of
	this trademark is prohibited by law and international treaties.
	
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
	
	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.
	
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	
	3. Redistribution in any form at a charge, that in whole or in part
	   contains or is derived from the software, including but not limited to
	   value added products, is prohibited without prior written consent of
	   tecema.
	
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

$pgtitle = array(gettext("Accounts"), gettext("Phones"));
$pghelp = gettext("Phone Accounts allow you to configure internal extensions and their access to Provider Accounts. Click an account type below to get started.");
$pglegend = array("add", "enabled", "disabled", "edit", "delete");

/* delete */
if ($_GET['action'] == "delete") {
	if(!($msg = pbx_delete_phone($_GET['uniqid']))) {
		$successful_action = true;
	} else {
		$savemsg = $msg;
	}
}

/* disable */
if ($_GET['action'] == "disable") {
	if(!($msg = pbx_disable_phone($_GET['uniqid']))) {
		$successful_action = true;
	} else {
		$savemsg = $msg;
	}
}

/* enable */
if ($_GET['action'] == "enable") {
	if(!($msg = pbx_enable_phone($_GET['uniqid']))) {
		$successful_action = true;
	} else {
		$savemsg = $msg;
	}
}

/* handle successful action */
if ($successful_action) {
	write_config();
	$pieces = explode("-", $_GET['uniqid']);
	$phone_type = strtolower($pieces[0]);
	switch ($phone_type) {
		case "analog":
			touch($g['analog_dirty_path']);
			break;
		case "external":
			touch($g['external_dirty_path']);
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
		case "skinny":
			touch($g['skinny_dirty_path']);	
			break;
	}
	header("Location: accounts_phones.php");
	exit;	
}

/* dirty sip config? */
if (file_exists($g['sip_dirty_path'])) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= sip_conf_generate();
		$retval |= extensions_conf_generate();
		$retval |= voicemail_conf_generate();
		$retval |= provisioning_configure();
		config_unlock();
		
		$retval |= pbx_exec("module reload chan_sip.so");
		$retval |= pbx_exec("dialplan reload");
		$retval |= pbx_exec("module reload app_voicemail.so");
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
		$retval |= voicemail_conf_generate();
		config_unlock();
		
		$retval |= pbx_exec("module reload chan_iax2.so");
		$retval |= pbx_exec("dialplan reload");
		$retval |= pbx_exec("module reload app_voicemail.so");
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
		$retval |= voicemail_conf_generate();
		config_unlock();
		
		$retval |= pbx_exec("module reload chan_dahdi.so");
		$retval |= pbx_exec("dahdi restart");
		$retval |= pbx_exec("dialplan reload");
		$retval |= pbx_exec("module reload app_voicemail.so");
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
		$retval |= voicemail_conf_generate();
		config_unlock();

		$retval |= pbx_exec("module reload chan_dahdi.so");
		$retval |= pbx_exec("dahdi restart");
		$retval |= pbx_exec("dialplan reload");
		$retval |= pbx_exec("module reload app_voicemail.so");
	}
	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($g['analog_dirty_path']);
	}
}

/* dirty external config? */
if (file_exists($g['external_dirty_path'])) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= extensions_conf_generate();
		$retval |= voicemail_conf_generate();
		config_unlock();
		
		$retval |= pbx_exec("dialplan reload");
		$retval |= pbx_exec("module reload app_voicemail.so");
	}
	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($g['external_dirty_path']);
	}
}

/* dirty skinny config? */
if (file_exists($g['skinny_dirty_path'])) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= skinny_conf_generate();
		$retval |= extensions_conf_generate();
		$retval |= voicemail_conf_generate();
		config_unlock();
		
		$retval |= pbx_exec("module reload chan_skinny.so");
		$retval |= pbx_exec("dialplan reload");
		$retval |= pbx_exec("module reload app_voicemail.so");
	}
	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($g['skinny_dirty_path']);
	}
}
?>

<? include("fbegin.inc"); ?>
<script type="text/javascript" charset="utf-8">

	<?=javascript_account_statuses("functions");?>

	jQuery(document).ready(function(){
	
		<?=javascript_account_statuses("ready");?>
	
	});

</script>
<form action="accounts_phones.php" method="post">
<? $status_info = pbx_get_peer_statuses(); ?>

<table border="0" cellspacing="0" cellpadding="6" width="100%">
	<tr>
		<td class="listhdradd"><img src="add.png">&nbsp;&nbsp;&nbsp;<?
		if (!isset($config['system']['webgui']['hidesip'])) {
			?><a href="phones_sip_edit.php"><?=gettext("SIP");?></a><img src="bullet_add.png">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<?
		}
		if (!isset($config['system']['webgui']['hideiax'])) {
			?><a href="phones_iax_edit.php"><?=gettext("IAX");?></a><img src="bullet_add.png">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<?
		}
		if (!isset($config['system']['webgui']['hideisdn']) &&
			count(dahdi_get_ports("isdn", "nt"))) {
			?><a href="phones_isdn_edit.php"><?=gettext("ISDN");?></a><img src="bullet_add.png">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<?
		}
		if (!isset($config['system']['webgui']['hideanalog']) &&
			count(dahdi_get_ports("analog", "fxs"))) {
			?><a href="phones_analog_edit.php"><?=gettext("Analog");?></a><img src="bullet_add.png">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<?
		}
			?><a href="phones_external_edit.php"><?=gettext("External");?></a><img src="bullet_add.png">
		</td>
	</tr>
	<tr> 
		<td class="list" height="12">&nbsp;</td>
	</tr>
</table>

<? if ($sip_phones = sip_get_phones()) : ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="5%" class="list"></td>
		<td colspan="4" class="listtopiclight"><?=gettext("SIP");?></td>
	</tr>
	<tr>
		<td width="5%" class="list"></td>
		<td width="15%" class="listhdrr"><?=gettext("Extension");?></td>
		<td width="35%" class="listhdrr"><?=gettext("Caller ID");?></td>
		<td width="35%" class="listhdr"><?=gettext("Description");?></td>
		<td width="10%" class="list"></td>
	</tr>

	<? foreach ($sip_phones as $p): ?>
	<tr>
		<td valign="middle" nowrap class="list"><?
		if (isset($p['disabled'])) {
			?><a href="?action=enable&uniqid=<?=$p['uniqid'];?>"><img src="disabled.png" title="<?=gettext("click to enable phone");?>" border="0"></a><?
		} else {
			?><a href="?action=disable&uniqid=<?=$p['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to disable this phone?");?>')"><img src="enabled.png" title="<?=gettext("click to disable phone");?>" border="0"></a><?
		}
		?></td>
		<td class="listbgl"><?
		if (!isset($p['disabled'])) {
			echo display_peer_status_icon($status_info[$p['extension']], $p['extension']);
			echo htmlspecialchars($p['extension']);
		} else {
			?><span class="gray"><?=htmlspecialchars($p['extension']);?></span><?
		}
		?></td>
		<td class="listr"><?=htmlspecialchars($p['callerid']);?>&nbsp;</td>
		<td class="listr"><?=htmlspecialchars($p['descr']);?>&nbsp;</td>
		<td valign="middle" nowrap class="list"><a href="phones_sip_edit.php?uniqid=<?=$p['uniqid'];?>"><img src="edit.png" title="<?=gettext("edit phone");?>" border="0"></a>
			<a href="?action=delete&uniqid=<?=$p['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this phone?");?>')"><img src="delete.png" title="<?=gettext("delete phone");?>" border="0"></a></td>
	</tr>
	<? endforeach; ?>

	<tr> 
		<td class="list" colspan="5" height="12">&nbsp;</td>
	</tr>
</table>
<? endif; ?>

<? if ($iax_phones = iax_get_phones()) : ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="5%" class="list"></td>
		<td colspan="4" class="listtopiclight"><?=gettext("IAX");?></td>
	</tr>
	<tr>
		<td width="5%" class="list"></td>
		<td width="15%" class="listhdrr"><?=gettext("Extension");?></td>
		<td width="35%" class="listhdrr"><?=gettext("Caller ID");?></td>
		<td width="35%" class="listhdr"><?=gettext("Description");?></td>
		<td width="10%" class="list"></td>
	</tr>

	<? foreach ($iax_phones as $p): ?>
	<tr>
		<td valign="middle" nowrap class="list"><?
		if (isset($p['disabled'])) {
			?><a href="?action=enable&uniqid=<?=$p['uniqid'];?>"><img src="disabled.png" title="<?=gettext("click to enable phone");?>" border="0"></a><?
		} else {
			?><a href="?action=disable&uniqid=<?=$p['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to disable this phone?");?>')"><img src="enabled.png" title="<?=gettext("click to disable phone");?>" border="0"></a><?
		}
		?></td>
		<td class="listbgl"><?
		if (!isset($p['disabled'])) {
			echo display_peer_status_icon($status_info[$p['extension']], $p['extension']);
			echo htmlspecialchars($p['extension']);
		} else {
			?><span class="gray"><?=htmlspecialchars($p['extension']);?></span><?
		}
		?></td>
		<td class="listr"><?=htmlspecialchars($p['callerid']);?>&nbsp;</td>
		<td class="listr"><?=htmlspecialchars($p['descr']);?>&nbsp;</td>
		<td valign="middle" nowrap class="list"><a href="phones_iax_edit.php?uniqid=<?=$p['uniqid'];?>"><img src="edit.png" title="<?=gettext("edit phone");?>" border="0"></a>
			<a href="?action=delete&uniqid=<?=$p['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this phone?");?>')"><img src="delete.png" title="<?=gettext("delete phone");?>" border="0"></a></td>
	</tr>
	<? endforeach; ?>

	<tr> 
		<td class="list" colspan="5" height="12">&nbsp;</td>
	</tr>
</table>
<? endif; ?>

<? if ($isdn_phones = isdn_get_phones()) : ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="5%" class="list"></td>
		<td colspan="4" class="listtopiclight"><?=gettext("ISDN");?></td>
	</tr>
	<tr>
		<td width="5%" class="list"></td>
		<td width="15%" class="listhdrr"><?=gettext("Extension");?></td>
		<td width="35%" class="listhdrr"><?=gettext("Caller ID");?></td>
		<td width="35%" class="listhdr"><?=gettext("Description");?></td>
		<td width="10%" class="list"></td>
	</tr>

	<? foreach ($isdn_phones as $p): ?>
	<tr>
		<td valign="middle" nowrap class="list"><?
		if (isset($p['disabled'])) {
			?><a href="?action=enable&uniqid=<?=$p['uniqid'];?>"><img src="disabled.png" title="<?=gettext("click to enable phone");?>" border="0"></a><?
		} else {
			?><a href="?action=disable&uniqid=<?=$p['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to disable this phone?");?>')"><img src="enabled.png" title="click to disable phone" border="0"></a><?
		}
		?></td>
		<td class="listbgl"><?
		if (!isset($p['disabled'])) {
			echo display_peer_status_icon($status_info[$p['extension']], $p['extension']);
			echo htmlspecialchars($p['extension']);
		} else {
			?><span class="gray"><?=htmlspecialchars($p['extension']);?></span><?
		}
		?></td>
		<td class="listr"><?=htmlspecialchars($p['callerid']);?>&nbsp;</td>
		<td class="listr"><?=htmlspecialchars($p['descr']);?>&nbsp;</td>
		<td valign="middle" nowrap class="list"><a href="phones_isdn_edit.php?uniqid=<?=$p['uniqid'];?>"><img src="edit.png" title="<?=gettext("edit phone");?>" border="0"></a>
			<a href="?action=delete&uniqid=<?=$p['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this phone?");?>')"><img src="delete.png" title="<?=gettext("delete phone");?>" border="0"></a></td>
	</tr>
	<? endforeach; ?>

	<tr> 
		<td class="list" colspan="5" height="12">&nbsp;</td>
	</tr>
</table>
<? endif; ?>

<? if ($analog_phones = analog_get_phones()) : ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="5%" class="list"></td>
		<td colspan="4" class="listtopiclight"><?=gettext("Analog");?></td>
	</tr>
	<tr>
		<td width="5%" class="list"></td>
		<td width="15%" class="listhdrr"><?=gettext("Extension");?></td>
		<td width="35%" class="listhdrr"><?=gettext("Caller ID");?></td>
		<td width="35%" class="listhdr"><?=gettext("Description");?></td>
		<td width="10%" class="list"></td>
	</tr>

	<? foreach ($analog_phones as $p): ?>
	<tr>
		<td valign="middle" nowrap class="list"><?
		if (isset($p['disabled'])) {
			?><a href="?action=enable&uniqid=<?=$p['uniqid'];?>"><img src="disabled.png" title="<?=gettext("click to enable phone");?>" border="0"></a><?
		} else {
			?><a href="?action=disable&uniqid=<?=$p['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to disable this phone?");?>')"><img src="enabled.png" title="<?=gettext("click to disable phone");?>" border="0"></a><?
		}
		?></td>
		<td class="listbgl"><?
		if (!isset($p['disabled'])) {
			echo display_peer_status_icon($status_info[$p['extension']], $p['extension']);
			echo htmlspecialchars($p['extension']);
		} else {
			?><span class="gray"><?=htmlspecialchars($p['extension']);?></span><?
		}
		?></td>
		<td class="listr"><?=htmlspecialchars($p['callerid']);?>&nbsp;</td>
		<td class="listr"><?=htmlspecialchars($p['descr']);?>&nbsp;</td>
		<td valign="middle" nowrap class="list"><a href="phones_analog_edit.php?uniqid=<?=$p['uniqid'];?>"><img src="edit.png" title="<?=gettext("edit phone");?>" border="0"></a>
			<a href="?action=delete&uniqid=<?=$p['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this phone?");?>')"><img src="delete.png" title="<?=gettext("delete phone");?>" border="0"></a></td>
	</tr>
	<? endforeach; ?>

	<tr> 
		<td class="list" colspan="5" height="12">&nbsp;</td>
	</tr>
</table>
<? endif; ?>

<? if ($skinny_phones = skinny_get_phones()) : ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="5%" class="list"></td>
		<td colspan="4" class="listtopiclight"><?=gettext("Skinny");?></td>
	</tr>
	<tr>
		<td width="5%" class="list"></td>
		<td width="15%" class="listhdrr"><?=gettext("Extension");?></td>
		<td width="35%" class="listhdrr"><?=gettext("Caller ID");?></td>
		<td width="35%" class="listhdr"><?=gettext("Description");?></td>
		<td width="10%" class="list"></td>
	</tr>

	<? foreach ($skinny_phones as $p): ?>
	<tr>
		<td valign="middle" nowrap class="list"><?
		if (isset($p['disabled'])) {
			?><a href="?action=enable&uniqid=<?=$p['uniqid'];?>"><img src="disabled.png" title="<?=gettext("click to enable phone");?>" border="0"></a><?
		} else {
			?><a href="?action=disable&uniqid=<?=$p['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to disable this phone?");?>')"><img src="enabled.png" title="<?=gettext("click to disable phone");?>" border="0"></a><?
		}
		?></td>
		<td class="listbgl"><?
		if (!isset($p['disabled'])) {
			echo display_peer_status_icon($status_info[$p['extension']], $p['extension']);
			echo htmlspecialchars($p['extension']);
		} else {
			?><span class="gray"><?=htmlspecialchars($p['extension']);?></span><?
		}
		?></td>
		<td class="listr"><?=htmlspecialchars($p['callerid']);?>&nbsp;</td>
		<td class="listr"><?=htmlspecialchars($p['descr']);?>&nbsp;</td>
		<td valign="middle" nowrap class="list"><a href="phones_skinny_edit.php?uniqid=<?=$p['uniqid'];?>"><img src="edit.png" title="<?=gettext("edit phone");?>" border="0"></a>
			<a href="?action=delete&uniqid=<?=$p['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this phone?");?>')"><img src="delete.png" title="<?=gettext("delete phone");?>" border="0"></a></td>
	</tr>
	<? endforeach; ?>

	<tr> 
		<td class="list" colspan="5" height="12">&nbsp;</td>
	</tr>
</table>
<? endif; ?>

<? if ($external_phones = external_get_phones()) : ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="5%" class="list"></td>
		<td colspan="4" class="listtopiclight"><?=gettext("External");?></td>
	</tr>
	<tr>
		<td width="5%" class="list"></td>
		<td width="15%" class="listhdrr"><?=gettext("Extension");?></td>
		<td width="35%" class="listhdrr"><?=gettext("Caller ID");?></td>
		<td width="35%" class="listhdr"><?=gettext("Dialstring");?></td>
		<td width="10%" class="list"></td>
	</tr>

	<? foreach ($external_phones as $p): ?>
	<tr>
		<td valign="middle" nowrap class="list"><?
		if (isset($p['disabled'])) {
			?><a href="?action=enable&uniqid=<?=$p['uniqid'];?>"><img src="disabled.png" title="<?=gettext("click to enable phone");?>" border="0"></a><?
		} else {
			?><a href="?action=disable&uniqid=<?=$p['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to disable this phone?");?>')"><img src="enabled.png" title="<?=gettext("click to disable phone");?>" border="0"></a><?
		}
		?></td>
		<td class="listbgl"><?
		if (!isset($p['disabled'])) {
			echo display_peer_status_icon($status_info[$p['uniqid']], $p['uniqid']);
			echo htmlspecialchars($p['extension']);
		} else {
			?><span class="gray"><?=htmlspecialchars($p['extension']);?></span><?
		}
		?></td>
		<td class="listr"><?=htmlspecialchars($p['callerid']);?>&nbsp;</td><?

		if ($p['dialprovider'] == "sipuri" || $p['dialprovider'] == "iaxuri") {
			?><td class="listr"><?=htmlspecialchars($p['dialstring']);?></td><?
		} else {
			?><td class="listr"><?=htmlspecialchars($p['dialstring'] . " via " . pbx_uniqid_to_name($p['dialprovider']));?></td><?
		}

		?><td valign="middle" nowrap class="list"><a href="phones_external_edit.php?uniqid=<?=$p['uniqid'];?>"><img src="edit.png" title="<?=gettext("edit phone");?>" border="0"></a>
			<a href="?action=delete&uniqid=<?=$p['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this phone?");?>')"><img src="delete.png" title="<?=gettext("delete phone");?>" border="0"></a></td>
	</tr>
	<? endforeach; ?>

	<tr> 
		<td class="list" colspan="5" height="12">&nbsp;</td>
	</tr>
</table>
<? endif; ?>

</form>

<? include("fend.inc"); ?>
