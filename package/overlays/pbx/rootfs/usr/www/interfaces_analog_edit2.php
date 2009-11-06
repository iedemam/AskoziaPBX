#!/usr/bin/php
<?php 
/*
	$Id: interfaces_analog_edit.php 906 2009-05-20 13:41:18Z michael.iedema $
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

$pgtitle = array(
	gettext("Interfaces"), 
	gettext("Edit Analog Port")
);

$uniqid = $_GET['uniqid'];
if (isset($_POST['uniqid'])) {
	$uniqid = $_POST['uniqid'];
}

$pconfig = dahdi_get_port($uniqid);


if ($_POST) {

	unset($input_errors);
	$_POST['manualattributes'] = split_and_clean_lines($_POST['manualattributes']);
	$pconfig = $_POST;
	
	if ($msg = verify_manual_attributes($_POST['manualattributes'])) {
		$input_errors[] = $msg;
	}

	// this is a messy fix for properly and encoding the content
	$pconfig['manual-attribute'] = array_map("base64_encode", $_POST['manualattributes']);
	
	if (!$input_errors) {
		// XXX : these merging and sorting bits in isdn and analog interfaces need a rewrite
		$n = count($a_abinterfaces);
		if (isset($configured_units[$unit])) {
			for ($i = 0; $i < $n; $i++) {
				if ($a_abinterfaces[$i]['unit'] == $unit) {
					$a_abinterfaces[$i]['name'] = $_POST['name'];
					$a_abinterfaces[$i]['type'] = $_POST['type'];
					$a_abinterfaces[$i]['startsignal'] = verify_non_default($_POST['startsignal'], $defaults['analog']['interface']['startsignal']);
					$a_abinterfaces[$i]['echocancel'] = verify_non_default($_POST['echocancel'], $defaults['analog']['interface']['echocancel']);
					$a_abinterfaces[$i]['rxgain'] = verify_non_default($_POST['rxgain'], $defaults['analog']['interface']['rxgain']);
					$a_abinterfaces[$i]['txgain'] = verify_non_default($_POST['txgain'], $defaults['analog']['interface']['txgain']);
					$a_abinterfaces[$i]['manual-attribute'] = array_map("base64_encode", $_POST['manualattributes']);
				}
			}

		} else {
			$a_abinterfaces[$n]['unit'] = $unit;
			$a_abinterfaces[$n]['name'] = $_POST['name'];
			$a_abinterfaces[$n]['type'] = $_POST['type'];
			$a_abinterfaces[$n]['startsignal'] = verify_non_default($_POST['startsignal'], $defaults['analog']['interface']['startsignal']);
			$a_abinterfaces[$n]['echocancel'] = verify_non_default($_POST['echocancel'], $defaults['analog']['interface']['echocancel']);
			$a_abinterfaces[$n]['rxgain'] = verify_non_default($_POST['rxgain'], $defaults['analog']['interface']['rxgain']);
			$a_abinterfaces[$n]['txgain'] = verify_non_default($_POST['txgain'], $defaults['analog']['interface']['txgain']);
			$a_abinterfaces[$n]['manual-attribute'] = array_map("base64_encode", $_POST['manualattributes']);
		}


		touch($d_analogconfdirty_path);

		write_config();

		header("Location: interfaces_analog.php");
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
<?php if ($input_errors) display_input_errors($input_errors); ?>
<form action="interfaces_analog_edit.php" method="post" name="iform" id="iform">
<table width="100%" border="0" cellpadding="6" cellspacing="0">
	<tr> 
		<td width="20%" valign="top" class="vncellreq"><?=gettext("Name");?></td>
		<td width="80%" class="vtable">
			<input name="name" type="text" class="formfld" id="name" size="40" value="<?=htmlspecialchars($pconfig['name']);?>">
			<br><span class="vexpl"><?=gettext("Descriptive name for this port");?></span>
		</td>
	</tr>
	<tr> 
		<td valign="top" class="vncell"><?=gettext("Echo Canceller");?></td>
		<td class="vtable">
			<select name="echocancel" class="formfld" id="echocancel">
				<option value="no" <?
				if (isset($pconfig['echo-enabled'])) {
					echo "selected";
				}
				?>><?=gettext("Disabled");?></option><?
				
				$tapvals = array(32, 64, 128, 256);
				foreach ($tapvals as $tapval) {
					?><option value="<?=$tapval;?>" <?
					if ($pconfig['echo-typs'] == $tapval) {
						echo "selected";
					}
					?>><?=$tapval;?></option><?
				}

			?></select>
			<br><span class="vexpl"><?=gettext("The echo canceller 'tap' size. Larger sizes more effectively cancel echo but require more processing power.");?></span>
		</td>
	</tr>
	<? display_advanced_settings_begin(1); ?>
	<? display_analog_gain_selector($pconfig['rxgain'], $pconfig['txgain'], 1); ?>
	<tr> 
		<td valign="top" class="vncell"><?=gettext("Start Signaling");?></td>
		<td class="vtable">
			<select name="startsignal" class="formfld" id="startsignal"><?

			foreach ($analog_startsignals as $signalabb => $signalname) {
				?><option value="<?=$signalabb;?>" <?
				if ($signalabb == $pconfig['startsignal']) {
					echo "selected";
				}
				?>><?=$signalname;?></option><?
			}

			?></select>
			<br><span class="vexpl"><?=gettext("In nearly all cases, 'Kewl Start' is the appropriate choice here.");?></span>
		</td>
	</tr>
	<? display_manual_attributes_editor($pconfig['manual-attribute'], 1); ?>
	<? display_advanced_settings_end(); ?>
	<tr> 
		<td width="20%" valign="top">&nbsp;</td>
		<td width="80%">
			<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>">
			<input name="unit" type="hidden" value="<?=$unit;?>">
			<input name="type" type="hidden" value="<?=$type;?>">
		</td>
	</tr>
	<tr> 
		<td valign="top">&nbsp;</td>
		<td>
			<span class="vexpl"><span class="red"><strong><?=gettext("Warning:");?><br>
			</strong></span><?=gettext("clicking &quot;Save&quot; will drop all current calls.");?></span>
		</td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
