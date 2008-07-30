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

$pgtitle = array(gettext("Interfaces"), gettext("Analog"));
$pghelp = gettext("Detected ports on this page must be edited and saved before the system can utilize them.");
$pglegend = array("edit", "delete");
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
	$configured_units[$interface['unit']]['rxgain'] = $interface['rxgain'];
	$configured_units[$interface['unit']]['txgain'] = $interface['txgain'];
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
		$merged_units[$i]['rxgain'] = $configured_units[$i]['rxgain'];
		$merged_units[$i]['txgain'] = $configured_units[$i]['txgain'];
	} else {
		$merged_units[$i]['unit'] = $i;
		$merged_units[$i]['name'] = "(unconfigured)";
		$merged_units[$i]['type'] = $recognized_units[$i];
		$merged_units[$i]['startsignal'] = "ks";
		$merged_units[$i]['echocancel'] = "128";
	}
}

if ($_GET['action'] == "forget") {
	if(!($msg = analog_forget_interface($_GET['unit']))) {
		write_config();
		touch($d_analogconfdirty_path);
		header("Location: interfaces_analog.php");
		exit;
	} else {
		$input_errors[] = $msg;	
	}
}

if (file_exists($d_analogconfdirty_path)) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= analog_conf_generate();
		$retval |= analog_configure();
		config_unlock();
		
		$retval |= pbx_configure();
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
				gettext('Network')	=> 'interfaces_network.php',
				gettext('Wireless')	=> 'interfaces_wireless.php',
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

		if (!count($recognized_units)) {

				?><tr> 
					<td><strong><?=gettext("No compatible analog interfaces detected.");?></strong>
					<br>
					<br><?=gettext("If an analog interface is present but was not detected, please send <a href=\"ajax.cgi?exec_shell=/usr/sbin/pciconf%20-lv;/bin/echo;/sbin/dmesg\">this output</a> to <a href=\"mailto:michael@askozia.com\">michael@askozia.com</a>.");?></td>
				</tr>
			</table><?

		} else {

				?><tr>
					<td width="10%" class="listhdrr"><?=gettext("Unit");?></td>
					<td width="20%" class="listhdrr"><?=gettext("Name");?></td>
					<td width="10%" class="listhdrr"><?=gettext("Type");?></td>
					<td width="12%" class="listhdrr"><?=gettext("Start");?></td>
					<td width="18%" class="listhdrr"><?=gettext("Gain (rx/tx)");?></td>
					<td width="20%" class="listhdrr"><?=gettext("Echo Canceller");?></td>
					<td width="10%" class="list"></td>
				</tr><?	
			
				foreach ($merged_units as $mu) {
					if ($mu['name'] != "(unconfigured)") {
						// set start signal text
						if (isset($mu['startsignal'])) {
							$startsignal = $analog_startsignals[$mu['startsignal']];
							$startsignal = substr($startsignal, 0, strpos($startsignal, " "));	
						} else {
							$startsignal = "Kewl";
						}

						// set gain text
						if (isset($mu['rxgain'])) {
							$rx = $mu['rxgain'];
						} else {
							$rx = $defaults['analog']['interface']['rxgain'];
						}
						if (isset($mu['txgain'])) {
							$tx = $mu['txgain'];
						} else {
							$tx = $defaults['analog']['interface']['txgain'];
						}
						$gain = $rx . "/" . $tx;

						// set echo cancel text
						if (!isset($mu['echocancel'])) {
							$ecfield = "128";
						} else if ($mu['echocancel'] == "no") {
							$ecfield = "Disabled";
						} else {
							$ecfield = $mu['echocancel'];
						}

					} else {
						$startsignal = "";
						$gain = "";
						$ecfield = "";
					}

				?><tr>
					<td class="listlr"><?=htmlspecialchars($mu['unit']);?></td>
					<td class="listbg"><?=htmlspecialchars($mu['name']);?>&nbsp;</td>
					<td class="listr"><?=htmlspecialchars($mu['type']);?></td>
					<td class="listr"><?=htmlspecialchars($startsignal);?>&nbsp;</td>
					<td class="listr"><?=htmlspecialchars($gain);?>&nbsp;</td>
					<td class="listr"><?=htmlspecialchars($ecfield);?>&nbsp;</td>
					<td valign="middle" nowrap class="list"><a href="interfaces_analog_edit.php?unit=<?=$mu['unit'];?>&type=<?=$mu['type'];?>"><img src="edit.png" title="<?=gettext("edit analog interface");?>" border="0"></a>
					<? if ($mu['name'] != "(unconfigured)") : ?>
						<a href="?action=forget&unit=<?=$mu['unit'];?>" onclick="return confirm('<?=gettext("Do you really want to forget this interface\'s settings?");?>')"><img src="delete.png" title="<?=gettext("forget interface settings");?>" border="0"></a>
					<? endif; ?>
					</td>
				</tr><?
			}

			?></table>
			<br>
			<span class="vexpl"><strong><span class="red"><?=gettext("Note:");?></span></strong>
			<?=gettext("FXS interfaces connect to telephones and FXO interfaces connect to provider lines. FXS interfaces produce electricity to power the telephone and FXO interfaces accept the electricity generated by the provider.");?> <strong><?=gettext("Connecting a provider line to an FXS port can damage your hardware! If you are unsure which ports are FXO or FXS on your card, configure and connect your phone accounts before connecting any provider lines.");?></strong></span><?

		}

		?></td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
