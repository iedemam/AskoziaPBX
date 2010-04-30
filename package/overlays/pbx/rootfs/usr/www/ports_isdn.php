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
$pgtitle = array(gettext("Telephony Ports"), gettext("ISDN"));

if ($_GET['action'] == "forget") { 
	dahdi_forget_port($_GET['uniqid']);
	header("Location: ports_isdn.php"); 
	exit;
}

if ($_GET['action'] == "delete") { 
	dahdi_delete_portgroup($_GET['uniqid']);
	header("Location: ports_isdn.php"); 
	exit;
}

if (file_exists($g['dahdi_dirty_path'])) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= dahdi_configure();
		$retval |= pbx_restart();
		config_unlock();
	}

	$savemsg = get_std_save_message($retval);
}

$isdn_ports = dahdi_get_ports("isdn");
$isdn_groups = dahdi_get_portgroups("isdn");

include("fbegin.inc");

?><form action="ports_isdn.php" method="post">
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<? display_ports_tab_menu(); ?>
	<tr>
		<td class="tabcont">
			<table width="100%" border="0" cellpadding="6" cellspacing="0"><?

		if (!count($isdn_ports)) {

				?><tr> 
					<td><strong><?=gettext("No compatible isdn ports were detected.");?></strong></td>
				</tr>
			</table><?

		} else {

				?><tr>
					<td colspan="6" valign="top" class="d_header_nounderline"><?=gettext("Individual Ports");?></td>
				</tr>
				<tr>
					<td width="5%" class="listhdrr">#</td>
					<td width="35%" class="listhdrr"><?=gettext("Name");?></td>
					<td width="25%" class="listhdrr"><?=gettext("Card");?></td>
					<td width="15%" class="listhdrr"><?=gettext("Type");?></td>
					<td width="10%" class="listhdrr"><?=gettext("Channels");?></td>
					<td width="10%" class="list"></td>
				</tr><?	

			foreach ($isdn_ports as $port) {
				$type = ($port['type'] == "nt") ? gettext("Phone") : gettext("Provider");

				?><tr>
					<td class="listlr"><?=htmlspecialchars($port['span']);?></td>
					<td class="listbg"><?=htmlspecialchars($port['name']);?></td>
					<td class="listr"><?=htmlspecialchars($port['card']);?></td>
					<td class="listr"><?=htmlspecialchars($type);?></td>
					<td class="listr"><?=htmlspecialchars($port['totalchannels']);?></td>
					<td valign="middle" nowrap class="list"><a href="ports_isdn_edit.php?uniqid=<?=$port['uniqid'];?>"><img src="edit.png" title="<?=gettext("edit port");?>" border="0"></a>
					<a href="?action=forget&uniqid=<?=$port['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to forget this port\'s settings?");?>')"><img src="delete.png" title="<?=gettext("forget port settings");?>" border="0"></a></td>
				</tr><?

			}

				?><tr>
					<td colspan="6" class="list" height="22"></td>
				</tr>
			</table>
			<table width="100%" border="0" cellpadding="6" cellspacing="0">
				<tr>
					<td colspan="3" valign="top" class="d_header_nounderline"><?=gettext("Provider Port Groups");?></td>
				</tr>
				<tr>
					<td width="35%" class="listhdrr"><?=gettext("Name");?></td>
					<td width="55%" class="listhdrr"><?=gettext("Members");?></td>
					<td width="10%" class="list"></td>
				</tr><?

			foreach ($isdn_groups as $group) {

				?><tr>
					<td class="listlr"><?=htmlspecialchars($group['name']);?></td>
					<td class="listbg"><?
					if (is_array($group['groupmember'])) {
						echo @implode(", ", pbx_uniqid_to_name($group['groupmember']));
					}
					?></td>
					<td valign="middle" nowrap class="list"><a href="ports_groups_edit.php?uniqid=<?=$group['uniqid'];?>"><img src="edit.png" title="<?=gettext("edit port group");?>" border="0"></a>
					<a href="?action=delete&uniqid=<?=$group['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to delete this port group's settings?");?>')"><img src="delete.png" title="<?=gettext("delete port group");?>" border="0"></a></td>
				</tr><?

			}

				?><tr>
					<td class="list" colspan="2"></td>
					<td class="list"><a href="ports_groups_edit.php?technology=isdn"><img src="add.png" title="<?=gettext("add port group");?>" border="0"></a></td>
				</tr>
			</table><?

		}

		?></td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
