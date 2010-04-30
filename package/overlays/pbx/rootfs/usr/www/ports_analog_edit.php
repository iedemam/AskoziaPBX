#!/usr/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2009 tecema (a.k.a IKT) <http://www.tecema.de>.
	All rights reserved.
	
	Askozia®PBX is a registered trademark of tecema. Any unauthorized use of
	this trademark is prohibited by law and international treaties.
	
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
	
	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.
	
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	
	3. Redistribution in any form at a charge, that in whole or in part
	   contains or is derived from the software, including but not limited to
	   value added products, is prohibited without prior written consent of
	   tecema.
	
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

$uniqid = $_GET['uniqid'];
if (isset($_POST['uniqid'])) {
	$uniqid = $_POST['uniqid'];
}

$carryovers = array(
	"location", "card", "technology", "type", "basechannel", "uniqid"
);


if ($_POST) {

	$pconfig = $_POST;

	$port['name'] = $_POST['name'] ? $_POST['name'] : gettext("Port") . " " . $_POST['basechannel'];
	$port['startsignaling'] = $_POST['startsignaling'];
	$port['echo-taps'] = $_POST['echo-taps'];
	$port['rxgain'] = $_POST['rxgain'];
	$port['txgain'] = $_POST['txgain'];

	foreach ($carryovers as $co) {
		$port[$co] = $_POST[$co];
	}

	dahdi_save_port($port);

	header("Location: ports_analog.php");
	exit;
}

$pconfig = dahdi_get_port($uniqid);

$porttype = ($pconfig['type'] == "fxs") ? gettext("Phone") : gettext("Provider");
$pgtitle = array(gettext("Ports"), sprintf(gettext("Edit Analog %s Port"), $porttype));

include("fbegin.inc");
if ($input_errors) {
	display_input_errors($input_errors);
}
?><form action="ports_analog_edit.php" method="post" name="iform" id="iform">
<table width="100%" border="0" cellpadding="6" cellspacing="0">
	<tr> 
		<td width="20%" valign="top" class="vncell"><?=gettext("Name");?></td>
		<td width="80%" class="vtable">
			<input name="name" type="text" class="formfld" id="name" size="40" value="<?=htmlspecialchars($pconfig['name']);?>">
			<br><span class="vexpl"><?=gettext("Enter a descriptive name for this port.");?></span>
		</td>
	</tr>
	<tr>
		<td valign="top" class="vncell"><?=gettext("Echo Cancellation");?></td>
		<td class="vtable">
			<select name="echo-taps" class="formfld" id="echo-taps">
				<option value="disable" <?
				if ($pconfig['echo-taps'] == "disable") {
					echo "selected";
				}
				?>><?=gettext("disable echo cancellation");?></option><?
				
				$tapvals = array(32, 64, 128, 256);
				foreach ($tapvals as $tapval) {
					?><option value="<?=$tapval;?>" <?
					if ($pconfig['echo-taps'] == $tapval) {
						echo "selected";
					}
					?>><?=$tapval/8 . " " . gettext("milliseconds");?></option><?
				}

			?></select>
			<br><span class="vexpl"><?=gettext("The echo canceller window size. If your calls have echo, try increasing this window.");?></span>
		</td>
	</tr>
	<? display_port_gain_selector($pconfig['rxgain'], $pconfig['txgain'], 1); ?>
	<tr>
		<td valign="top" class="vncell"><?=gettext("Start Signaling");?></td>
		<td class="vtable">
			<select name="startsignaling" class="formfld" id="startsignaling"><?

			$startsignals = array(
				"ks" => gettext("Kewl Start"),
				"gs" => gettext("Ground Start"),
				"ls" => gettext("Loop Start")
			);
			foreach ($startsignals as $signalabb => $signalname) {
				?><option value="<?=$signalabb;?>" <?
				if ($signalabb == $pconfig['startsignaling']) {
					echo "selected";
				}
				?>><?=$signalname;?></option><?
			}

			?></select>
			<br><span class="vexpl"><?=gettext("This is the how your system determines if a phone has been hung-up or picked-up. Usually \"Kewl Start\" is the best choice but some providers work better with one of the other options. If your calls are not starting or ending reliably, try adjusting this setting.");?></span>
		</td>
	</tr>
	<tr>
		<td width="20%" valign="top">&nbsp;</td>
		<td width="80%">
			<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>"><?

			foreach ($carryovers as $co) {
				?>
				<input name="<?=$co;?>" id="<?=$co;?>" type="hidden" value="<?=$pconfig[$co];?>">
				<?
			}

		?></td>
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
