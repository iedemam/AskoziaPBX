#!/usr/bin/php
<?php 
/*
	$Id: accounts_faxes.php 1515 2010-04-30 11:38:34Z michael.iedema $
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2011 tecema (a.k.a IKT) <http://www.tecema.de>.
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

$pgtitle = array(gettext("Accounts"), gettext("Faxes"));

/* delete */
if ($_GET['action'] == "delete") {
	if(!($msg = pbx_delete_fax($_GET['uniqid']))) {
		$successful_action = true;
	} else {
		$savemsg = $msg;
	}
}

/* disable */
if ($_GET['action'] == "disable") {
	if(!($msg = pbx_disable_fax($_GET['uniqid']))) {
		$successful_action = true;
	} else {
		$savemsg = $msg;
	}
}

/* enable */
if ($_GET['action'] == "enable") {
	if(!($msg = pbx_enable_fax($_GET['uniqid']))) {
		$successful_action = true;
	} else {
		$savemsg = $msg;
	}
}

/* handle successful action */
if ($successful_action) {
	write_config();
	$pieces = explode("-", $_GET['uniqid']);
	$fax_type = strtolower($pieces[0]);
	switch ($fax_type) {
		case "virtual":
			touch($g['virtualfax_dirty_path']);
			break;
		case "analog":
			touch($g['analog_dirty_path']);
			break;
	}
	header("Location: accounts_faxes.php");
	exit;
}

/* dirty virtualfax config? */
if (file_exists($g['virtualfax_dirty_path'])) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= extensions_conf_generate();
		config_unlock();

		$retval |= pbx_exec("dialplan reload");
	}
	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($g['virtualfax_dirty_path']);
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

include("fbegin.inc");

?><form action="accounts_faxes.php" method="post">
<table border="0" cellspacing="0" cellpadding="6" width="100%">
	<tr>
		<td class="listhdradd"><img src="add.png">&nbsp;&nbsp;&nbsp;
			<a href="faxes_analog_edit.php"><?=gettext("Analog");?></a><img src="bullet_add.png">
			&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
			<a href="faxes_virtual_edit.php"><?=gettext("Virtual");?></a><img src="bullet_add.png">
		</td>
	</tr>
	<tr>
		<td class="list" height="12">&nbsp;</td>
	</tr>
</table><?

if ($analog_faxes = analog_get_faxes()) {
?><table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="5%" class="list"></td>
		<td colspan="4" class="listtopiclight"><?=gettext("Analog");?></td>
	</tr>
	<tr>
		<td width="5%" class="list"></td>
		<td width="15%" class="listhdrr"><?=gettext("Extension");?></td>
		<td width="35%" class="listhdrr"><?=gettext("Name");?></td>
		<td width="35%" class="listhdr"><?=gettext("Port");?></td>
		<td width="10%" class="list"></td>
	</tr><?

	if(count($analog_faxes) > 0)
	{
		foreach ($analog_faxes as $f) {
		?><tr>
			<td valign="middle" nowrap class="list"><?
			if (isset($f['disabled'])) {
				?><a href="?action=enable&uniqid=<?=$f['uniqid'];?>"><img src="disabled.png" title="<?=gettext("click to enable fax");?>" border="0"></a><?
			} else {
				?><a href="?action=disable&uniqid=<?=$f['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to disable this fax?");?>')"><img src="enabled.png" title="<?=gettext("click to disable fax");?>" border="0"></a><?
			}
			?></td>
			<td class="listbgl"><?
			if (!isset($f['disabled'])) {
				echo htmlspecialchars($f['extension']);
			} else {
				?><span class="gray"><?=htmlspecialchars($f['extension']);?></span><?
			}
			?></td>
			<td class="listr"><?=htmlspecialchars($f['callerid']);?>&nbsp;</td>
			<td class="listr"><?=htmlspecialchars(pbx_uniqid_to_name($f['port']));?>&nbsp;</td>
			<td valign="middle" nowrap class="list"><a href="faxes_analog_edit.php?uniqid=<?=$f['uniqid'];?>"><img src="edit.png" title="<?=gettext("edit fax");?>" border="0"></a>
				<a href="?action=delete&uniqid=<?=$f['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this fax?");?>')"><img src="delete.png" title="<?=gettext("delete fax");?>" border="0"></a></td>
		</tr><?
		}
	}

	?><tr>
		<td class="list" colspan="5" height="12">&nbsp;</td>
	</tr>
</table><?
}

if ($virtual_faxes = virtual_get_faxes()) {
?><table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="5%" class="list"></td>
		<td colspan="4" class="listtopiclight"><?=gettext("Virtual");?></td>
	</tr>
	<tr>
		<td width="5%" class="list"></td>
		<td width="15%" class="listhdrr"><?=gettext("Extension");?></td>
		<td width="35%" class="listhdrr"><?=gettext("Name");?></td>
		<td width="35%" class="listhdr"><?=gettext("E-Mail");?></td>
		<td width="10%" class="list"></td>
	</tr><?

	if(count($virtual_faxes) > 0)
	{
		foreach ($virtual_faxes as $f) {
		?><tr>
			<td valign="middle" nowrap class="list"><?
			if (isset($f['disabled'])) {
				?><a href="?action=enable&uniqid=<?=$f['uniqid'];?>"><img src="disabled.png" title="<?=gettext("click to enable fax");?>" border="0"></a><?
			} else {
				?><a href="?action=disable&uniqid=<?=$f['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to disable this fax?");?>')"><img src="enabled.png" title="<?=gettext("click to disable fax");?>" border="0"></a><?
			}
			?></td>
			<td class="listbgl"><?
			if (!isset($f['disabled'])) {
				echo htmlspecialchars($f['extension']);
			} else {
				?><span class="gray"><?=htmlspecialchars($f['extension']);?></span><?
			}
			?></td>
			<td class="listr"><?=htmlspecialchars($f['callerid']);?>&nbsp;</td>
			<td class="listr"><?=htmlspecialchars($f['email']);?>&nbsp;</td>
			<td valign="middle" nowrap class="list"><a href="faxes_virtual_edit.php?uniqid=<?=$f['uniqid'];?>"><img src="edit.png" title="<?=gettext("edit fax");?>" border="0"></a>
				<a href="?action=delete&uniqid=<?=$f['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this fax?");?>')"><img src="delete.png" title="<?=gettext("delete fax");?>" border="0"></a></td>
		</tr><?
		}
	}

	?><tr>
		<td class="list" colspan="5" height="12">&nbsp;</td>
	</tr>
</table><?
}


?></form><?

include("fend.inc");
