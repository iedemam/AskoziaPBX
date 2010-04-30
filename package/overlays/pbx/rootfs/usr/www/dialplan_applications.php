#!/usr/bin/php
<?php 
/*
	$Id: dialplan_callgroups.php 198 2007-09-17 14:11:18Z michael.iedema $
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2008 tecema (a.k.a IKT) <http://www.tecema.de>.
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
$pghelp = gettext("With some knowledge of how the Asterisk dialplan works, some basic applications can be added to the system. These applications can then be dialed via their defined extensions or used as a destination for incoming calls from Providers.");
$pglegend = array("add", "edit", "delete");

// XXX this is_array, sort, reference stuff is all over...
if (!is_array($config['dialplan']['application']))
	$config['dialplan']['application'] = array();

applications_sort_apps();
$a_applications = &$config['dialplan']['application'];


if ($_GET['action'] == "delete") {
	if ($a_applications[$_GET['id']]) {

		unset($a_applications[$_GET['id']]);
		
		write_config();
		touch($d_extensionsconfdirty_path);
		header("Location: dialplan_applications.php");
		exit;
	}
}

if (file_exists($d_extensionsconfdirty_path)) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= extensions_conf_generate();
		config_unlock();
		
		$retval |= pbx_exec("dialplan reload");
	}
	
	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($d_extensionsconfdirty_path);
	}
}

include("fbegin.inc");
?><form action="dialplan_applications.php" method="post">
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="15%" class="listhdrr"><?=gettext("Extension");?></td>
		<td width="25%" class="listhdrr"><?=gettext("Name");?></td>
		<td width="50%" class="listhdrr"><?=gettext("Description");?></td>
		<td width="10%" class="list"></td>
	</tr>
	

	<?php $i = 0; foreach ($a_applications as $app): ?>
	<tr>
		<td class="listlr"><?=htmlspecialchars($app['extension']);?></td>
		<td class="listbg"><?=htmlspecialchars($app['name']);?>&nbsp;</td>
		<td class="listr"><?=htmlspecialchars(gettext($app['descr']));?>&nbsp;</td>
		<td valign="middle" nowrap class="list"><a href="dialplan_applications_edit.php?id=<?=$i;?>"><img src="edit.png" title="<?=gettext("edit application mapping");?>" border="0"></a>
			<a href="?action=delete&id=<?=$i;?>" onclick="return confirm('<?=gettext("Do you really want to delete this application mapping?");?>')"><img src="delete.png" title="<?=gettext("delete application mapping");?>" border="0"></a></td>
	</tr>
	<?php $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="3"></td>
		<td class="list"><a href="dialplan_applications_edit.php"><img src="add.png" title="<?=gettext("add application mapping");?>" border="0"></a></td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
