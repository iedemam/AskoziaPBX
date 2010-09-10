#!/usr/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2010 tecema (a.k.a IKT) <http://www.tecema.de>.
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

$pgtitle = array(gettext("Dialplan"), gettext("Applications"));

/* delete */
if ($_GET['action'] == "delete") {
	applications_delete_application($_GET['uniqid']);
	header("Location: dialplan_applications.php");
	exit;
}

/* disable */
if ($_GET['action'] == "disable") {
	applications_disable_application($_GET['uniqid']);
	header("Location: dialplan_applications.php");
	exit;
}

/* enable */
if ($_GET['action'] == "enable") {
	applications_enable_application($_GET['uniqid']);
	header("Location: dialplan_applications.php");
	exit;
}


if (file_exists($g['dialplan_dirty_path'])) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= extensions_conf_generate();
		config_unlock();

		$retval |= pbx_exec("dialplan reload");
	}

	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($g['dialplan_dirty_path']);
	}
}


include("fbegin.inc");


?><form action="dialplan_applications.php" method="post">
<table border="0" cellspacing="0" cellpadding="6" width="100%">
	<tr>
		<td class="listhdradd"><img src="add.png">&nbsp;&nbsp;&nbsp;
			<a href="dialplan_applications_edit.php?type=plaintext"><?=gettext("Plaintext");?></a><img src="bullet_add.png">
			&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
			<a href="dialplan_applications_edit.php?type=php"><?=gettext("PHP");?></a><img src="bullet_add.png">
		</td>
	</tr>
	<tr>
		<td class="list" height="12">&nbsp;</td>
	</tr>
</table><?


if ($plaintext_applications = applications_get_applications("plaintext")) {
?><table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="5%" class="list"></td>
		<td colspan="4" class="listtopiclight"><?=gettext("Plaintext");?></td>
	</tr>
	<tr>
		<td width="5%" class="list"></td>
		<td width="20%" class="listhdrr"><?=gettext("Number");?></td>
		<td width="25%" class="listhdrr"><?=gettext("Name");?></td>
		<td width="40%" class="listhdrr"><?=gettext("Description");?></td>
		<td width="10%" class="list"></td>
	</tr><?

	foreach ($plaintext_applications as $a) {
	?><tr>
		<td valign="middle" nowrap class="list"><?
		if (isset($a['disabled'])) {
			?><a href="?action=enable&uniqid=<?=$a['uniqid'];?>"><img src="disabled.png" title="<?=gettext("click to enable application");?>" border="0"></a><?
		} else {
			?><a href="?action=disable&uniqid=<?=$a['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to disable this application?");?>')"><img src="enabled.png" title="<?=gettext("click to disable application");?>" border="0"></a><?
		}
		?></td>
		<td class="listbgl"><?
		if (!isset($a['disabled'])) {
			echo htmlspecialchars($a['extension']);
		} else {
			?><span class="gray"><?=htmlspecialchars($a['extension']);?></span><?
		}
		?></td>
		<td class="listr"><?=htmlspecialchars($a['name']);?>&nbsp;</td>
		<td class="listr"><?=htmlspecialchars($a['descr']);?>&nbsp;</td>
		<td valign="middle" nowrap class="list"><a href="dialplan_applications_edit.php?uniqid=<?=$a['uniqid'];?>"><img src="edit.png" title="<?=gettext("edit application");?>" border="0"></a>
			<a href="?action=delete&uniqid=<?=$a['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this application?");?>')"><img src="delete.png" title="<?=gettext("delete application");?>" border="0"></a></td>
	</tr><?
	}

	?><tr>
		<td class="list" colspan="5" height="12">&nbsp;</td>
	</tr>
</table><?
}

if ($php_applications = applications_get_applications("php")) {
?><table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="5%" class="list"></td>
		<td colspan="4" class="listtopiclight"><?=gettext("PHP");?></td>
	</tr>
	<tr>
		<td width="5%" class="list"></td>
		<td width="20%" class="listhdrr"><?=gettext("Number");?></td>
		<td width="25%" class="listhdrr"><?=gettext("Name");?></td>
		<td width="40%" class="listhdrr"><?=gettext("Description");?></td>
		<td width="10%" class="list"></td>
	</tr><?

	foreach ($php_applications as $a) {
	?><tr>
		<td valign="middle" nowrap class="list"><?
		if (isset($a['disabled'])) {
			?><a href="?action=enable&uniqid=<?=$a['uniqid'];?>"><img src="disabled.png" title="<?=gettext("click to enable application");?>" border="0"></a><?
		} else {
			?><a href="?action=disable&uniqid=<?=$a['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to disable this application?");?>')"><img src="enabled.png" title="<?=gettext("click to disable application");?>" border="0"></a><?
		}
		?></td>
		<td class="listbgl"><?
		if (!isset($a['disabled'])) {
			echo htmlspecialchars($a['extension']);
		} else {
			?><span class="gray"><?=htmlspecialchars($a['extension']);?></span><?
		}
		?></td>
		<td class="listr"><?=htmlspecialchars($a['name']);?>&nbsp;</td>
		<td class="listr"><?=htmlspecialchars($a['descr']);?>&nbsp;</td>
		<td valign="middle" nowrap class="list"><a href="dialplan_applications_edit.php?uniqid=<?=$a['uniqid'];?>"><img src="edit.png" title="<?=gettext("edit application");?>" border="0"></a>
			<a href="?action=delete&uniqid=<?=$a['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this application?");?>')"><img src="delete.png" title="<?=gettext("delete application");?>" border="0"></a></td>
	</tr><?
	}

	?><tr>
		<td class="list" colspan="5" height="12">&nbsp;</td>
	</tr>
</table><?
}

include("fend.inc");
