#!/usr/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2009 IKT <http://itison-ikt.de>.
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
$pgtitle = array(gettext("System"), gettext("Interfaces"), gettext("Analog"));

if (file_exists($g['dahdi_dirty_path'])) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= dahdi_configure();
		config_unlock();
	}

	$savemsg = get_std_save_message($retval);
}

$analog_ports = dahdi_get_ports("analog");

include("fbegin.inc");

?><form action="interfaces_analog.php" method="post"><?
if ($savemsg) {
	display_info_box($savemsg);
}

?><table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav"><?

			$tabs = array(
				gettext('Network')	=> 'interfaces_network.php',
				//gettext('Wireless')	=> 'interfaces_wireless.php',
				gettext('ISDN')		=> 'interfaces_isdn.php',
				gettext('Analog')	=> 'interfaces_analog.php',
				gettext('Storage')	=> 'interfaces_storage.php'
			);
			dynamic_tab_menu($tabs);
			
			?></ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<table width="100%" border="0" cellpadding="6" cellspacing="0"><?

		if (!count($analog_ports)) {

				?><tr> 
					<td><strong><?=gettext("No compatible analog ports were detected.");?></strong></td>
				</tr>
			</table><?

		} else {

				?><tr>
					<td width="5%" class="listhdrr">#</td>
					<td width="40%" class="listhdrr"><?=gettext("Name");?></td>
					<td width="35%" class="listhdrr"><?=gettext("Card");?></td>
					<td width="15%" class="listhdrr"><?=gettext("Type");?></td>
					<td width="5%" class="list"></td>
				</tr><?	
			
				foreach ($analog_ports as $port) {
					$type = ($port['type'] == "fxs") ? gettext("Phone") : gettext("Provider");

				?><tr>
					<td class="listlr"><?=htmlspecialchars($port['basechannel']);?></td>
					<td class="listbg"><?=htmlspecialchars($port['name']);?></td>
					<td class="listr"><?=htmlspecialchars($port['card']);?></td>
					<td class="listr"><?=htmlspecialchars($type);?></td>
					<td valign="middle" nowrap class="list"><a href="interfaces_analog_edit.php?uniqid=<?=$port['uniqid'];?>"><img src="edit.png" title="<?=gettext("edit analog port");?>" border="0"></a></td>
				</tr><?
			}

			?></table><?

		}

		?></td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
