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

$pgtitle = array("Interfaces", "Edit ISDN Interface #$unit");
require("guiconfig.inc");


if (!is_array($config['interfaces']['isdn-unit']))
	$config['interfaces']['isdn-unit'] = array();

isdn_sort_interfaces();
$a_isdninterfaces = &$config['interfaces']['isdn-unit'];

$configured_units = array();
foreach ($a_isdninterfaces as $interface) {
	$configured_units[$interface['unit']]['name'] = $interface['name'];
	$configured_units[$interface['unit']]['mode'] = $interface['mode'];
	$configured_units[$interface['unit']]['echocancel'] = $interface['echocancel'];
	$configured_units[$interface['unit']]['pcmmaster'] = $interface['pcmmaster'];
	$configured_units[$interface['unit']]['nopwrsave'] = $interface['nopwrsave'];
	$configured_units[$interface['unit']]['pollmode'] = $interface['pollmode'];
	$configured_units[$interface['unit']]['manual-attribute'] = $interface['manual-attribute'];
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
		$merged_units[$i]['unit'] = $i;
		$merged_units[$i]['name'] = $configured_units[$i]['name'];
		$merged_units[$i]['mode'] = $configured_units[$i]['mode'];
		$merged_units[$i]['echocancel'] = $configured_units[$i]['echocancel'];
		$merged_units[$i]['pcmmaster'] = $configured_units[$i]['pcmmaster'];
		$merged_units[$i]['nopwrsave'] = $configured_units[$i]['nopwrsave'];
		$merged_units[$i]['pollmode'] = $configured_units[$i]['pollmode'];
		$merged_units[$i]['manual-attribute'] = $configured_units[$i]['manual-attribute'];
	} else {
		$merged_units[$i]['unit'] = $i;
		$merged_units[$i]['name'] = "(unconfigured)";
	}
}

/* pull current config into pconfig */
$pconfig['unit'] = $merged_units[$unit]['unit'];
$pconfig['name'] = $merged_units[$unit]['name'];
$pconfig['mode'] = $merged_units[$unit]['mode'];
$pconfig['echocancel'] = $merged_units[$unit]['echocancel'];
$pconfig['pcmmaster'] = $merged_units[$unit]['pcmmaster'];
$pconfig['nopwrsave'] = $merged_units[$unit]['nopwrsave'];
$pconfig['pollmode'] = $merged_units[$unit]['pollmode'];
$pconfig['manual-attribute'] = $merged_units[$unit]['manual-attribute'];


if ($_POST) {

	unset($input_errors);
	$_POST['manualattributes'] = split_and_clean_lines($_POST['manualattributes']);
	$pconfig = $_POST;
	
	if ($msg = verify_manual_attributes($_POST['manualattributes'])) {
		$input_errors[] = $msg;
	}

	if (!$_POST['mode']) {
		$input_errors[] = "An Operating Mode must be selected for this interface.";
	}

	// this is a messy fix for properly and encoding the content
	$pconfig['manual-attribute'] = array_map("base64_encode", $_POST['manualattributes']);
	
	if (!$input_errors) {
		
		$n = count($a_isdninterfaces);
		if (isset($configured_units[$unit])) {
			for ($i = 0; $i < $n; $i++) {
				if ($a_isdninterfaces[$i]['unit'] == $unit) {
					$a_isdninterfaces[$i]['name'] = $_POST['name'];
					$a_isdninterfaces[$i]['mode'] = $_POST['mode'];
					$a_isdninterfaces[$i]['echocancel'] = $_POST['echocancel'];
					$a_isdninterfaces[$i]['pcmmaster'] = $_POST['pcmmaster'];
					$a_isdninterfaces[$i]['nopwrsave'] = $_POST['nopwrsave'];
					$a_isdninterfaces[$i]['pollmode'] = $_POST['pollmode'];
					$a_isdninterfaces[$i]['manual-attribute'] = array_map("base64_encode", $_POST['manualattributes']);
				}
			}

		} else {
			$a_isdninterfaces[$n]['unit'] = $unit;
			$a_isdninterfaces[$n]['name'] = $_POST['name'];
			$a_isdninterfaces[$n]['mode'] = $_POST['mode'];
			$a_isdninterfaces[$n]['echocancel'] = $_POST['echocancel'];
			$a_isdninterfaces[$n]['pcmmaster'] = $_POST['pcmmaster'];
			$a_isdninterfaces[$n]['nopwrsave'] = $_POST['nopwrsave'];
			$a_isdninterfaces[$n]['pollmode'] = $_POST['pollmode'];
			$a_isdninterfaces[$n]['manual-attribute'] = array_map("base64_encode", $_POST['manualattributes']);
		}


		touch($d_isdnconfdirty_path);

		write_config();

		header("Location: interfaces_isdn.php");
		exit;
	}
}
?>
<?php include("fbegin.inc"); ?>
<script type="text/JavaScript">
<!--

	jQuery(document).ready(function(){

		<?=javascript_advanced_settings("ready");?>

	});

//-->
</script>
<?php if ($input_errors) print_input_errors($input_errors); ?>
<form action="interfaces_isdn_edit.php" method="post" name="iform" id="iform">
<table width="100%" border="0" cellpadding="6" cellspacing="0">
	<tr> 
		<td valign="top" class="vncellreq">Name</td>
		<td class="vtable">
			<input name="name" type="text" class="formfld" id="name" size="40" value="<?=htmlspecialchars(($pconfig['name'] != "(unconfigured)") ? $pconfig['name'] : "isdn #$unit");?>"> 
			<br><span class="vexpl">Descriptive name for this interface</span>
		</td>
	</tr>
	<tr> 
		<td valign="top" class="vncell">Operating Mode</td>
		<td class="vtable">
			<select name="mode" class="formfld" id="mode">
			<? foreach ($isdn_dchannel_modes as $mode => $friendly) : ?>
			<option value="<?=$mode;?>" <?
			if ($mode == $pconfig['mode'])
				echo "selected"; ?>
			><?=$friendly;?></option>
			<? endforeach; ?>
			</select>
			<br><span class="vexpl">
				<ul>
					<li>point-to-multipoint, terminal equipment: this port accepts MSNs to route calls and is attached to the public ISDN network or another PBX system</li>
					<li>multipoint-to-point, network termination: this port provides MSNs to route calls and is attached to one or more telephones</li>
					<li>point-to-point, terminal equipment: this port accepts DID to route calls and is connected directly to another PBX system</li>
					<li>point-to-point, network termination: this port provides DID to route calls and is connected directly to another PBX system</li>
				</ul>
			</span>
			<br>
		</td>
	</tr>
	<tr> 
		<td valign="top" class="vncell">Echo Canceller</td>
		<td class="vtable">
			<input name="echocancel" id="echocancel" type="checkbox" value="yes" <? if ($pconfig['echocancel']) echo "checked"; ?>>
			Enable echo cancellation.
		</td>
	</tr>
	<tr> 
		<td valign="top" class="vncell">PCM Timing Master</td>
		<td class="vtable">
			<input name="pcmmaster" id="pcmmaster" type="checkbox" value="yes" <? if ($pconfig['pcmmaster']) echo "checked"; ?>>
			This card provides the timing source (needed if this is the only card).
		</td>
	</tr>
	<tr> 
		<td valign="top" class="vncell">Disable Power Save</td>
		<td class="vtable">
			<input name="nopwrsave" id="nopwrsave" type="checkbox" value="yes" <? if ($pconfig['nopwrsave']) echo "checked"; ?>>
			Disable power save mode. (sometimes needed for older cards)
		</td>
	</tr>
	<tr> 
		<td valign="top" class="vncell">Enable Polling Mode</td>
		<td class="vtable">
			<input name="pollmode" id="pollmode" type="checkbox" value="yes" <? if ($pconfig['pollmode']) echo "checked"; ?>>
			Enable polling mode. (sometimes needed for older cards)
		</td>
	</tr>
	<? display_advanced_settings_begin(1); ?>
	<? display_manual_attributes_editor($pconfig['manual-attribute'], 1); ?>
	<? display_advanced_settings_end(); ?>
	<tr> 
		<td width="20%" valign="top">&nbsp;</td>
		<td width="80%">
			<input name="Submit" type="submit" class="formbtn" value="Save">
			<input name="unit" type="hidden" value="<?=$unit;?>"> 
		</td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
