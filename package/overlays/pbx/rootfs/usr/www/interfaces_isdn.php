#!/usr/bin/php
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

require("guiconfig.inc");

$pgtitle = array(gettext("System"), gettext("Interfaces"), gettext("ISDN"));
$pghelp = gettext("Detected ports on this page must be edited and saved before the system can utilize them. Be careful when configuring ISDN interfaces. Having a single port on a card incorrectly configured can result in the entire card being unusable by the system.");
$pglegend = array("edit", "delete");

if (!is_array($config['interfaces']['isdn-unit']))
	$config['interfaces']['isdn-unit'] = array();

isdn_sort_interfaces();
$a_isdninterfaces = &$config['interfaces']['isdn-unit'];

$configured_units = array();
foreach ($a_isdninterfaces as $interface) {
	$configured_units[$interface['unit']] = $interface;
}

$recognized_units = isdn_get_recognized_unit_numbers();
if (!count($recognized_units)) {
	$n = 0;
} else {
	$n = max(array_keys($recognized_units));
	$n = ($n == 0) ? 1 : $n;
}
$merged_units = array();
for ($i = 0; $i <= $n; $i++) {
	if (!isset($recognized_units[$i])) {
		continue;
	}
	if (isset($configured_units[$i])) {
		$merged_units[$i] = $configured_units[$i];
		$merged_units[$i]['unit'] = $i;
	} else {
		$merged_units[$i]['unit'] = $i;
		$merged_units[$i]['name'] = $defaults['isdn']['interface']['name'];
	}
}

if ($_GET['action'] == "forget") {
	if(!($msg = isdn_forget_interface($_GET['unit']))) {
		write_config();
		touch($d_isdnconfdirty_path);
		header("Location: interfaces_isdn.php");
		exit;
	} else {
		$input_errors[] = $msg;	
	}
}

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

?>

<?php include("fbegin.inc"); ?>
<?php if ($input_errors) display_input_errors($input_errors); ?>
<?php if ($savemsg) display_info_box($savemsg); ?>
<form action="interfaces_isdn.php" method="post">
<table width="100%" border="0" cellpadding="0" cellspacing="0">
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
		<td class="tabcont"><?

		if (!count($recognized_units)) {

			?><table width="100%" border="0" cellpadding="6" cellspacing="0">
				<tr> 
					<td><strong><?=gettext("No compatible ISDN interfaces detected.");?></strong></td>
				</tr>
			</table><?

		} else {

			?><table width="100%" border="0" cellpadding="6" cellspacing="0">
				<tr>
					<td width="10%" class="listhdrr"><?=gettext("Unit");?></td>
					<td width="25%" class="listhdrr"><?=gettext("Name");?></td>		
					<td width="30%" class="listhdrr"><?=gettext("Operating Mode");?></td>
					<td width="25%" class="listhdr"><?=gettext("Echo Canceller");?></td>
					<td width="10%" class="list"></td>
				</tr><?	

			foreach ($merged_units as $mu) {
				if ($mu['name'] != $defaults['isdn']['interface']['name']) {
					$echocancel = $mu['echocancel'] ? gettext("Enabled") : gettext("Disabled");
				} else {
					$echocancel = "";
				}

				?><tr>
					<td class="listlr"><?=htmlspecialchars($mu['unit']);?></td>
					<td class="listbg"><?=htmlspecialchars($mu['name']);?>&nbsp;</td>
					<td class="listr"><?=htmlspecialchars($isdn_dchannel_modes[$mu['mode']]);?>&nbsp;</td>
					<td class="listr"><?=htmlspecialchars($echocancel);?>&nbsp;</td>
					<td valign="middle" nowrap class="list"><a href="interfaces_isdn_edit.php?unit=<?=$mu['unit'];?>"><img src="edit.png" title="<?=gettext("edit ISDN interface");?>" border="0"></a>
					<? if ($mu['name'] != $defaults['isdn']['interface']['name']) : ?>
						<a href="?action=forget&unit=<?=$mu['unit'];?>" onclick="return confirm('<?=gettext("Do you really want to forget this interface\'s settings?");?>')"><img src="delete.png" title="<?=gettext("forget interface settings");?>" border="0"></a>
					<? endif; ?>
					</td>
				</tr><?
			}

			?></table>
			<br>
			<span class="vexpl"><strong><?=gettext("Operating Modes");?></strong>
				<ul>
					<li><?=gettext("point-to-multipoint, terminal equipment: this port accepts MSNs to route calls and is attached to the public ISDN network or another PBX system");?></li>
					<li><?=gettext("multipoint-to-point, network termination: this port provides MSNs to route calls and is attached to one or more telephones");?></li>
					<li><?=gettext("point-to-point, terminal equipment: this port accepts DID to route calls and is connected directly to another PBX system")?></li>
					<li><?=gettext("point-to-point, network termination: this port provides DID to route calls and is connected directly to another PBX system");?></li>
				</ul>
			</span><?
		}

		?></td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
