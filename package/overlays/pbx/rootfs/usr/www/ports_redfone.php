#!/usr/bin/php
<?php 
/*
	$Id: ports_isdn.php 1323 2010-01-20 16:35:29Z michael.iedema $
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
$pgtitle = array(gettext("Telephony Ports"), gettext("RedFone Gateways"));

if ($_GET['action'] == "forget") { 
	redfone_forget_gateway($_GET['uniqid']);
	header("Location: ports_redfone.php"); 
	exit;
}

if (file_exists($g['redfone_dirty_path'])) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= redfone_configure();
		$retval |= dahdi_configure();
		$retval |= pbx_restart();
		config_unlock();
	}

	$savemsg = get_std_save_message($retval);
}

$redfone_gateways = redfone_get_gateways();

include("fbegin.inc");

?><form action="ports_redfone.php" method="post">
<table width="100%" border="0" cellpadding="0" cellspacing="0"><?

	display_ports_tab_menu();

	?><tr>
		<td class="tabcont">
			<table width="100%" border="0" cellpadding="6" cellspacing="0">
				<tr>
					<td width="15%" class="listhdrr"><?=gettext("Interface");?></td>
					<td width="20%" class="listhdrr"><?=gettext("MAC");?></td>
					<td width="15%" class="listhdrr"><?=gettext("IP");?></td>
					<td width="10%" class="listhdrr"><?=gettext("Spans");?></td>
					<td width="15%" class="listhdrr"><?=gettext("Firmware");?></td>
					<td width="15%" class="listhdrr"><?=gettext("Hardware");?></td>
					<td width="10%" class="list"></td>
				</tr><?	

			foreach ($redfone_gateways as $gw) {

				?><tr>
					<td class="listbgl"><?=htmlspecialchars($gw['ethif']);?></td>
					<td class="listr"><?=htmlspecialchars($gw['mac']);?></td>
					<td class="listr"><?=htmlspecialchars($gw['ip']);?></td>
					<td class="listr"><?=htmlspecialchars($gw['spancount']);?></td>
					<td class="listr">v<?=htmlspecialchars($gw['firmwareversion']);?></td>
					<td class="listr">v<?=htmlspecialchars($gw['hardwareversion']);?></td>
					<td valign="middle" nowrap class="list"><a href="ports_redfone_edit.php?uniqid=<?=$gw['uniqid'];?>"><img src="edit.png" title="<?=gettext("edit gateway");?>" border="0"></a>
					<a href="?action=forget&uniqid=<?=$gw['uniqid'];?>" onclick="return confirm('<?=gettext("Do you really want to forget this gateway's settings?");?>')"><img src="delete.png" title="<?=gettext("forget gateway settings");?>" border="0"></a></td>
				</tr><?

			}

			?></table>
		</td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
