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

$pgtitle = array("Accounts", "Phones");
require("guiconfig.inc");

/* delete phone? */
if ($_GET['act'] == "del") {
	pbx_delete_phone($_GET['id']);		
	write_config();
	touch($d_analogconfdirty_path);
	header("Location: accounts_phones.php");
	exit;
}

/* dirty sip config */
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

/* dirty iax config? */
if (file_exists($d_iaxconfdirty_path)) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= iax_conf_generate();
		$retval |= extensions_conf_generate();
		$retval |= voicemail_conf_generate();
		config_unlock();
		
		$retval |= iax_reload();
		$retval |= extensions_reload();
		$retval |= voicemail_reload();
	}
	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($d_iaxconfdirty_path);
	}
}

/* dirty isdn config */
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

/* dirty analog config? */
if (file_exists($d_analogconfdirty_path)) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= analog_zapata_conf_generate();
		$retval |= extensions_conf_generate();
		$retval |= voicemail_conf_generate();
		config_unlock();
		
		$retval |= analog_reload();
		$retval |= extensions_reload();
		$retval |= voicemail_reload();
	}
	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($d_analogconfdirty_path);
	}
}

/* dirty external config? */
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

<? include("fbegin.inc"); ?>
<form action="accounts_phones.php" method="post">
<? if ($savemsg) print_info_box($savemsg); ?>

<? if (!isset($config['system']['webgui']['hidesip'])) : ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td colspan="3" class="listtopiclight">SIP</td>
		<td class="list"></td>
	</tr>
	<tr>
		<td width="15%" class="listhdrr">Extension</td>
		<td width="35%" class="listhdrr">Caller ID</td>
		<td width="40%" class="listhdr">Description</td>
		<td width="10%" class="list"></td>
	</tr>
	<? $sip_phones = sip_get_phones(); ?>
	<? $i = 0; foreach ($sip_phones as $p): ?>
	<tr>
		<td class="listlr">
			<?=htmlspecialchars($p['extension']);?>
		</td>
		<td class="listbg">
			<?=htmlspecialchars($p['callerid']);?>
		</td>
		<td class="listr">
			<?=htmlspecialchars($p['descr']);?>&nbsp;
		</td>
		<td valign="middle" nowrap class="list"> <a href="phones_sip_edit.php?id=<?=$i;?>"><img src="e.gif" title="edit SIP phone" width="17" height="17" border="0"></a>
           &nbsp;<a href="accounts_phones.php?act=del&id=<?=$p['uniqid'];?>" onclick="return confirm('Do you really want to delete this SIP phone?')"><img src="x.gif" title="delete SIP phone" width="17" height="17" border="0"></a></td>
	</tr>
	<? $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="3"></td>
		<td class="list"> <a href="phones_sip_edit.php"><img src="plus.gif" title="add SIP phone" width="17" height="17" border="0"></a></td>
	</tr>
	<tr> 
		<td class="list" colspan="4" height="12">&nbsp;</td>
	</tr>
</table>
<? endif; ?>


<? if (!isset($config['system']['webgui']['hideiax'])) : ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td colspan="3" class="listtopiclight">IAX</td>
		<td class="list"></td>
	</tr>
	<tr>
		<td width="15%" class="listhdrr">Extension</td>
		<td width="35%" class="listhdrr">Caller ID</td>
		<td width="40%" class="listhdr">Description</td>
		<td width="10%" class="list"></td>
	</tr>
	<? $iax_phones = iax_get_phones(); ?>
	<? $i = 0; foreach ($iax_phones as $p): ?>
	<tr>
		<td class="listlr">
			<?=htmlspecialchars($p['extension']);?>
		</td>
		<td class="listbg">
			<?=htmlspecialchars($p['callerid']);?>
		</td>
		<td class="listr">
			<?=htmlspecialchars($p['descr']);?>&nbsp;
		</td>
		<td valign="middle" nowrap class="list"> <a href="phones_iax_edit.php?id=<?=$i;?>"><img src="e.gif" title="edit IAX phone" width="17" height="17" border="0"></a>
           &nbsp;<a href="accounts_phones.php?act=del&id=<?=$p['uniqid'];?>" onclick="return confirm('Do you really want to delete this IAX phone?')"><img src="x.gif" title="delete IAX phone" width="17" height="17" border="0"></a></td>
	</tr>
	<? $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="3"></td>
		<td class="list"> <a href="phones_iax_edit.php"><img src="plus.gif" title="add IAX phone" width="17" height="17" border="0"></a></td>
	</tr>
	<tr> 
		<td class="list" colspan="4" height="12">&nbsp;</td>
	</tr>
</table>
<? endif; ?>


<? if (!isset($config['system']['webgui']['hideisdn'])) : ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td colspan="3" class="listtopiclight">ISDN</td>
		<td class="list"></td>
	</tr>
	<tr>
		<td width="15%" class="listhdrr">Extension</td>
		<td width="35%" class="listhdrr">Caller ID</td>
		<td width="40%" class="listhdr">Description</td>
		<td width="10%" class="list"></td>
	</tr>
	<? $isdn_phones = isdn_get_phones(); ?>
	<? $i = 0; foreach ($isdn_phones as $p): ?>
	<tr>
		<td class="listlr">
			<?=htmlspecialchars($p['extension']);?>
		</td>
		<td class="listbg">
			<?=htmlspecialchars($p['callerid']);?>
		</td>
		<td class="listr">
			<?=htmlspecialchars($p['descr']);?>&nbsp;
		</td>
		<td valign="middle" nowrap class="list"> <a href="phones_isdn_edit.php?id=<?=$i;?>"><img src="e.gif" title="edit ISDN phone" width="17" height="17" border="0"></a>
           &nbsp;<a href="accounts_phones.php?act=del&id=<?=$p['uniqid'];?>" onclick="return confirm('Do you really want to delete this ISDN phone?')"><img src="x.gif" title="delete ISDN phone" width="17" height="17" border="0"></a></td>
	</tr>
	<? $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="3"></td>
		<td class="list"> <a href="phones_isdn_edit.php"><img src="plus.gif" title="add ISDN phone" width="17" height="17" border="0"></a></td>
	</tr>
	<tr> 
		<td class="list" colspan="4" height="12">&nbsp;</td>
	</tr>
</table>
<? endif; ?>


<? if (!isset($config['system']['webgui']['hideanalog'])) : ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td colspan="3" class="listtopiclight">Analog</td>
		<td class="list"></td>
	</tr>
	<tr>
		<td width="15%" class="listhdrr">Extension</td>
		<td width="35%" class="listhdrr">Caller ID</td>
		<td width="40%" class="listhdr">Description</td>
		<td width="10%" class="list"></td>
	</tr>
	<? $analog_phones = analog_get_phones(); ?>
	<? $i = 0; foreach ($analog_phones as $p): ?>
	<tr>
		<td class="listlr">
			<?=htmlspecialchars($p['extension']);?>
		</td>
		<td class="listbg">
			<?=htmlspecialchars($p['callerid']);?>
		</td>
		<td class="listr">
			<?=htmlspecialchars($p['descr']);?>&nbsp;
		</td>
		<td valign="middle" nowrap class="list"> <a href="phones_analog_edit.php?id=<?=$i;?>"><img src="e.gif" title="edit analog phone" width="17" height="17" border="0"></a>
           &nbsp;<a href="accounts_phones.php?act=del&id=<?=$p['uniqid'];?>" onclick="return confirm('Do you really want to delete this analog phone?')"><img src="x.gif" title="delete analog phone" width="17" height="17" border="0"></a></td>
	</tr>
	<? $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="3"></td>
		<td class="list"> <a href="phones_analog_edit.php"><img src="plus.gif" title="add analog phone" width="17" height="17" border="0"></a></td>
	</tr>
	<tr> 
		<td class="list" colspan="4" height="12">&nbsp;</td>
	</tr>
</table>
<? endif; ?>

<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td colspan="4" class="listtopiclight">External</td>
		<td class="list"></td>
	</tr>
	<tr>
		<td width="15%" class="listhdrr">Extension</td>
		<td width="25%" class="listhdrr">Name</td>
		<td width="20%" class="listhdrr">Dialstring</td>
		<td width="30%" class="listhdr">Provider</td>
		<td width="10%" class="list"></td>
	</tr>
	<? $external_phones = external_get_phones(); ?>
	<? $i = 0; foreach ($external_phones as $p): ?>
	<tr>
		<td class="listlr">
			<?=htmlspecialchars($p['extension']);?>
		</td>
		<td class="listbg">
			<?=htmlspecialchars($p['name']);?>
		</td>
		<td class="listr">
			<?=htmlspecialchars($p['dialstring']);?>
		</td>
		<td class="listr">
			<?=htmlspecialchars(pbx_uniqid_to_name($p['dialprovider']));?>&nbsp;
		</td>
		<td valign="middle" nowrap class="list"> <a href="phones_external_edit.php?id=<?=$i;?>"><img src="e.gif" title="edit external phone" width="17" height="17" border="0"></a>
           &nbsp;<a href="accounts_phones.php?act=del&id=<?=$p['uniqid'];?>" onclick="return confirm('Do you really want to delete this external phone?')"><img src="x.gif" title="delete external phone" width="17" height="17" border="0"></a></td>
	</tr>
	<? $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="4"></td>
		<td class="list"> <a href="phones_external_edit.php"><img src="plus.gif" title="add external phone" width="17" height="17" border="0"></a></td>
	</tr>
	<tr> 
		<td class="list" colspan="5" height="24">&nbsp;</td>
	</tr>
</table>

</form>
<? include("fend.inc"); ?>
