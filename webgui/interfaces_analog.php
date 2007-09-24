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

$pgtitle = array("Interfaces", "Analog");
require("guiconfig.inc");


if (!is_array($config['interfaces']['ab-unit']))
	$config['interfaces']['ab-unit'] = array();

analog_sort_ab_interfaces();
$a_abinterfaces = &$config['interfaces']['ab-unit'];




$configured_units = array();
foreach ($a_abinterfaces as $interface) {
	$configured_units[$interface['unit']]['name'] = $interface['name'];
	$configured_units[$interface['unit']]['type'] = $interface['type'];
	$configured_units[$interface['unit']]['startsignal'] = $interface['startsignal'];
	$configured_units[$interface['unit']]['echocancel'] = $interface['echocancel'];
}

$recognized_units = analog_get_recognized_ab_unit_numbers();
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
		$merged_units[$i]['unit'] = $i;
		$merged_units[$i]['name'] = $configured_units[$i]['name'];
		$merged_units[$i]['type'] = $configured_units[$i]['type'];
		$merged_units[$i]['startsignal'] = $configured_units[$i]['startsignal'];
		$merged_units[$i]['echocancel'] = $configured_units[$i]['echocancel'];
	} else {
		$merged_units[$i]['unit'] = $i;
		$merged_units[$i]['name'] = "(unconfigured)";
		$merged_units[$i]['type'] = $recognized_units[$i];
		$merged_units[$i]['startsignal'] = "ks";
		$merged_units[$i]['echocancel'] = "yes";
	}
}


if (file_exists($d_analogconfdirty_path)) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= analog_conf_generate();
		$retval |= analog_configure();
		config_unlock();
		
		$retval |= asterisk_configure();
	}
	
	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($d_analogconfdirty_path);
	}
}

?>

<?php include("fbegin.inc"); ?>
<form action="interfaces_analog.php" method="post">
<?php if ($savemsg) print_info_box($savemsg); ?>

<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav"><?

			$tabs = array(
				'Network'	=> 'interfaces_network.php',
				//'Wireless'	=> 'interfaces_wireless.php',
				'ISDN'		=> 'interfaces_isdn.php',
				'Analog'	=> 'interfaces_analog.php',
				//'Storage'	=> 'interfaces_storage.php'
			);
			dynamic_tab_menu($tabs);
			
			?></ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<table width="100%" border="0" cellpadding="6" cellspacing="0"><?

			if (!count($recognized_units)) {
				
				?><tr> 
					<td><strong>No compatible analog interfaces detected.</strong>
					<br>
					<br>If an analog interface is present but was not detected, please send <a href="/exec_raw.php?cmd=pciconf%20-lv;echo;dmesg">this output</a> to <a href="mailto:michael@askozia.com">michael@askozia.com</a>.</td>
				</tr><?
				
			} else {
				/* XXX remove signaling, rearrange some bits, echo cancel & start don't seem to be working */
				?><tr>
					<td width="10%" class="listhdrr">Unit</td>
					<td width="30%" class="listhdrr">Name</td>
					<td width="15%" class="listhdrr">Type</td>
					<td width="20%" class="listhdrr">Start</td>
					<td width="20%" class="listhdrr">Echo Canceller</td>
					<td width="5%" class="list"></td>
				</tr><?	
			
				foreach ($merged_units as $mu) {
					if (isset($mu['startsignal'])) {
						$startsignal = $analog_startsignals[$mu['startsignal']];
						$startsignal = substr($startsignal, 0, strpos($startsignal, " "));	
					} else {
						$startsignal = "Kewl";
					}
			
				?><tr>
					<td class="listlr"><?=htmlspecialchars($mu['unit']);?></td>
					<td class="listbg"><?=htmlspecialchars($mu['name']);?>&nbsp;</td>
					<td class="listr"><?=htmlspecialchars($mu['type']);?>&nbsp;</td>
					<td class="listr"><?=htmlspecialchars($startsignal);?>&nbsp;</td>
					<td class="listr"><?=htmlspecialchars(isset($mu['echocancel']) ? "Enabled" : "");?>&nbsp;</td>
					<td valign="middle" nowrap class="list">
						<a href="interfaces_analog_edit.php?unit=<?=$mu['unit'];?>&type=<?=$mu['type'];?>"><img src="e.gif" title="edit analog interface" width="17" height="17" border="0"></a>
					</td>
				</tr><?
				
				}
			}

			?></table>
		</td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
