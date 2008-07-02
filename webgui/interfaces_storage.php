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

$pgtitle = array("Interfaces", "Storage");
require("guiconfig.inc");


if (storage_syspart_get_state() == "active") {
	$syspart = storage_syspart_get_info();
	$packages = packages_get_packages();
}

?>

<?php include("fbegin.inc"); ?>
<?php if ($savemsg) print_info_box($savemsg); ?>
<?php if ($input_errors) print_input_errors($input_errors); ?>
<form action="interfaces_storage.php" method="post" enctype="multipart/form-data">
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
		<td class="tabcont"><?

		if (!$syspart) {

			?><strong>The system storage media is not large enough to install packages.</strong> A minimum of <?=$defaults['storage']['system-media-minimum-size'];?>MB is required. In the future, external media will be able to be used, but currently packages must be stored on the internal system media.<?

		} else {

			?><table width="100%" border="0" cellpadding="6" cellspacing="0">
				<tr>
					<td width="20%" class="listhdrr">Name</td>
					<td width="25%" class="listhdrr">Capacity</td>
					<td width="15%" class="listhdrr">State</td>
					<td width="30%" class="listhdr">Installed Packages</td>
					<td width="10%" class="list"></td>
				</tr>
				<tr>
					<td class="listbgl">system storage partition</td>
					<td class="listr"><? display_capacity_bar(
						($syspart['size'] - ($defaults['storage']['system-partition-offset-megabytes']*1024*1024)),
						$syspart['usage']);?>&nbsp;</td>
					<td class="listr"><?=htmlspecialchars($syspart['state']);?>&nbsp;</td>
					<td class="listr"><?=htmlspecialchars(
						implode(", ", array_keys($syspart['packages'])));?>&nbsp;</td>
					<td valign="middle" nowrap class="list"><? /*
						<a href="interfaces_storage_syspart_edit.php"><img src="e.gif" title="edit system storage partition" width="17" height="17" border="0"></a> */ ?>&nbsp;
					</td>
				</tr>
			</table><?

		}

		?></td>
	</tr>
</table>
</form><?

include("fend.inc"); ?>
