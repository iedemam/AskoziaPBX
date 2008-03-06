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

$unit = $_GET['unit'];
if (isset($_POST['unit']))
	$unit = $_POST['unit'];
	
$type = $_GET['type'];
if (isset($_POST['type']))
	$type = $_POST['type'];

$pgtitle = array("Interfaces", "Edit Analog ". strtoupper($type) ." Interface #$unit");
require("guiconfig.inc");


if (!is_array($config['interfaces']['ab-unit']))
	$config['interfaces']['ab-unit'] = array();

analog_sort_ab_interfaces();
$a_abinterfaces = &$config['interfaces']['ab-unit'];

// XXX : these merging and sorting bits in isdn and analog interfaces need a rewrite
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
	}
}

/* pull current config into pconfig */
$pconfig['unit'] = $merged_units[$unit]['unit'];
$pconfig['name'] = $merged_units[$unit]['name'];
$pconfig['type'] = $merged_units[$unit]['type'];
$pconfig['startsignal'] = $merged_units[$unit]['startsignal'];
$pconfig['echocancel'] = $merged_units[$unit]['echocancel'] ? $merged_units[$unit]['echocancel'] : "128";
$pconfig['rxgain'] = $merged_units[$unit]['rxgain'];
$pconfig['txgain'] = $merged_units[$unit]['txgain'];


if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;
	
	if (!$input_errors) {
		// XXX : these merging and sorting bits in isdn and analog interfaces need a rewrite
		$n = count($a_abinterfaces);
		if (isset($configured_units[$unit])) {
			for ($i = 0; $i < $n; $i++) {
				if ($a_abinterfaces[$i]['unit'] == $unit) {
					$a_abinterfaces[$i]['name'] = $_POST['name'];
					$a_abinterfaces[$i]['type'] = $_POST['type'];
					$a_abinterfaces[$i]['startsignal'] = ($_POST['startsignal'] != "ks") ? $_POST['startsignal'] : false;
					$a_abinterfaces[$i]['echocancel'] = ($_POST['echocancel'] != "128") ? $_POST['echocancel'] : false;
					$a_abinterfaces[$i]['rxgain'] = verify_non_default($_POST['rxgain'], $defaults['analog']['interface']['rxgain']);
					$a_abinterfaces[$i]['txgain'] = verify_non_default($_POST['txgain'], $defaults['analog']['interface']['txgain']);
				}
			}

		} else {
			$a_abinterfaces[$n]['unit'] = $unit;
			$a_abinterfaces[$n]['name'] = $_POST['name'];
			$a_abinterfaces[$n]['type'] = $_POST['type'];
			$a_abinterfaces[$n]['startsignal'] = ($_POST['startsignal'] != "ks") ? $_POST['startsignal'] : false;
			$a_abinterfaces[$n]['echocancel'] = ($_POST['echocancel'] != "128") ? $_POST['echocancel'] : false;
			$a_abinterfaces[$n]['rxgain'] = verify_non_default($_POST['rxgain'], $defaults['analog']['interface']['rxgain']);
			$a_abinterfaces[$n]['txgain'] = verify_non_default($_POST['txgain'], $defaults['analog']['interface']['txgain']);
		}


		touch($d_analogconfdirty_path);

		write_config();

		header("Location: interfaces_analog.php");
		exit;
	}
}
?>
<?php include("fbegin.inc"); ?>
<?php if ($input_errors) print_input_errors($input_errors); ?>
<form action="interfaces_analog_edit.php" method="post" name="iform" id="iform">
<table width="100%" border="0" cellpadding="6" cellspacing="0">
	<tr> 
		<td width="20%" valign="top" class="vncellreq">Name</td>
		<td width="80%" class="vtable">
			<input name="name" type="text" class="formfld" id="name" size="40" value="<?=htmlspecialchars($pconfig['name']);?>"> 
			<br><span class="vexpl">Descriptive name for this interface</span>
		</td>
	</tr>
	<tr> 
		<td valign="top" class="vncell">Start Signaling</td>
		<td class="vtable">
			<select name="startsignal" class="formfld" id="startsignal">
			<? foreach ($analog_startsignals as $signalabb => $signalname) : ?>
			<option value="<?=$signalabb;?>" <?
			if ($signalabb == $pconfig['startsignal'])
				echo "selected"; ?>
			><?=$signalname;?></option>
			<? endforeach; ?>
			</select>
			<br><span class="vexpl">In nearly all cases, "Kewl Start" is the appropriate choice here.</span>
		</td>
	</tr>
	<tr> 
		<td valign="top" class="vncell">Echo Canceller</td>
		<td class="vtable">
			<select name="echocancel" class="formfld" id="echocancel">
				<option value="no" <? if ($pconfig['echocancel'] == "no") echo "selected"; ?>>Disabled</option>
				<option value="32" <? if ($pconfig['echocancel'] == "32") echo "selected"; ?>>32</option>
				<option value="64" <? if ($pconfig['echocancel'] == "64") echo "selected"; ?>>64</option>
				<option value="128" <? if ($pconfig['echocancel'] == "128") echo "selected"; ?>>128</option>
				<option value="256" <? if ($pconfig['echocancel'] == "256") echo "selected"; ?>>256</option>
			</select>
			<br><span class="vexpl">The echo canceller "tap" size. Larger sizes more effectively cancel echo but require more processing power.</span>
		</td>
	</tr>
	<? display_analog_gain_selector($pconfig['rxgain'], $pconfig['txgain'], 1); ?>
	<tr> 
		<td valign="top">&nbsp;</td>
		<td>
			<input name="Submit" type="submit" class="formbtn" value="Save">
			<input name="unit" type="hidden" value="<?=$unit;?>">
			<input name="type" type="hidden" value="<?=$type;?>">
		</td>
	</tr>
	<tr> 
		<td valign="top">&nbsp;</td>
		<td>
			<span class="vexpl"><span class="red"><strong>Warning:<br>
			</strong></span>clicking &quot;Save&quot; will drop all current
			calls.</span>
		</td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
