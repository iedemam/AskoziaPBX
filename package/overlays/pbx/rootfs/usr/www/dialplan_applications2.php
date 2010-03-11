#!/usr/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2010 IKT <http://itison-ikt.de>.
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

$pgtitle = array(gettext("Dialplan"), gettext("Applications"));

$applications = applications2_get_applications();

if ($_GET['action'] == "delete") {
	applications2_delete_application($_GET['uniqid']);
	header("Location: dialplan_applications2.php");
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
?><form action="dialplan_applications2.php" method="post">
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="15%" class="listhdrr"><?=gettext("Extension");?></td>
		<td width="25%" class="listhdrr"><?=gettext("Name");?></td>
		<td width="50%" class="listhdrr"><?=gettext("Description");?></td>
		<td width="10%" class="list"></td>
	</tr>
	

	<?php $i = 0; foreach ($applications as $app): ?>
	<tr>
		<td class="listlr"><?=htmlspecialchars($app['extension']);?></td>
		<td class="listbg"><?=htmlspecialchars($app['name']);?>&nbsp;</td>
		<td class="listr"><?=htmlspecialchars(gettext($app['descr']));?>&nbsp;</td>
		<td valign="middle" nowrap class="list"><a href="dialplan_applications_edit2.php?uniqid=<?=$app['uniqid'];?>"><img src="edit.png" title="<?=gettext("edit application");?>" border="0"></a>
			<a href="?action=delete&uniqid=<?=$app['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this application?");?>')"><img src="delete.png" title="<?=gettext("delete application");?>" border="0"></a></td>
	</tr>
	<?php $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="3"></td>
		<td class="list"><a href="dialplan_applications_edit2.php"><img src="add.png" title="<?=gettext("add application mapping");?>" border="0"></a></td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
