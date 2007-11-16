#!/usr/local/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007 IKT <http://itison-ikt.de>.
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

$pgtitle = array("Interfaces", "Storage");
require("guiconfig.inc");

$recognized_disks = storage_get_recognized_disks();

/* sort storage_get* ? */
/* merge units ? */

/*
if (file_exists($d_isdnconfdirty_path)) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= isdn_configure();
		$retval |= isdn_conf_generate();
		config_unlock();
		
		$retval |= isdn_reload();
	}
	
	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($d_isdnconfdirty_path);
	}
}
*/
?>

<?php include("fbegin.inc"); ?>
<form action="interfaces_storage.php" method="post">
<?php if ($savemsg) print_info_box($savemsg); ?>

<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav"><?

			$tabs = array(
				'Network'	=> 'interfaces_network.php',
				'Wireless'	=> 'interfaces_wireless.php',
				'ISDN'		=> 'interfaces_isdn.php',
				'Analog'	=> 'interfaces_analog.php',
				'Storage'	=> 'interfaces_storage.php'
			);
			dynamic_tab_menu($tabs);
			
			?></ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<table width="100%" border="0" cellpadding="6" cellspacing="0">
				<tr>
					<td colspan="4" class="listtopic">Disks</td>
					<td class="list">&nbsp;</td>
				</tr>
				<tr>
					<td width="25%" class="listhdrr">Name</td>
					<td width="25%" class="listhdrr">Capacity</td>
					<td width="15%" class="listhdrr">State</td>
					<td width="30%" class="listhdr">Services</td>
					<td width="5%" class="list"></td>
				</tr><?	

				foreach ($recognized_disks as $disk) {

					$capacity = "";
					if ($disk['usage'] !== false) {
						$capacity .= "<img src='bar_left.gif' height='15' width='4' border='0' align='absmiddle'>";
						$capacity .= "<img src='bar_blue.gif' height='15' width='" . $disk['usage'] . "' border='0' align='absmiddle'>";
						$capacity .= "<img src='bar_gray.gif' height='15' width='" . (100 - $disk['usage']) . "' border='0' align='absmiddle'>";
						$capacity .= "<img src='bar_right.gif' height='15' width='5' border='0' align='absmiddle'> ";
						$capacity .= "<br>" . $disk['usage'] . "% of ";
					}
					$capacity .= format_bytes($disk['bytes']);

					$state = storage_get_disk_state($disk['device']);
					
					if ($state == "active") {
						$services = implode(", ", storage_get_disk_services($disk['device']));
					}

				?><tr>
					<td class="listbgl"><?=htmlspecialchars($disk['name']);?>&nbsp;</td>
					<td class="listr"><?=$capacity;?>&nbsp;</td>
					<td class="listr"><?=htmlspecialchars($state);?>&nbsp;</td>
					<td class="listr"><?=htmlspecialchars($services);?>&nbsp;</td>
					<td valign="middle" nowrap class="list">
					<? if (!$disk['systemdisk']): ?>
						<a href="interfaces_storage_disk_edit.php?dev=<?=$disk['device'];?>"><img src="e.gif" title="edit storage disk" width="17" height="17" border="0"></a>
					<? else: ?>
						&nbsp;
					<? endif; ?>
					</td>
				</tr><?
				
				}

			?></table>
		</td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
