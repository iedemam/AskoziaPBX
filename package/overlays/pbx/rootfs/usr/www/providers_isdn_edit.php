#!/usr/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2010 tecema (a.k.a IKT) <http://www.tecema.de>.
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

$pgtitle = array(gettext("Accounts"), gettext("Edit ISDN Provider"));


if ($_POST) {
	unset($input_errors);

	$provider = isdn_verify_provider(&$_POST, &$input_errors);
	if (!$input_errors) {
		isdn_save_provider($provider);
		header("Location: accounts_providers.php");
		exit;
	}
}


$colspan = 1;
$carryovers[] = "uniqid";

$uniqid = $_GET['uniqid'];
if (isset($_POST['uniqid'])) {
	$uniqid = $_POST['uniqid'];
}

if ($_POST) {
	$form = $_POST;
} else if ($uniqid) {
	$form = isdn_get_provider($uniqid);
} else {
	$form = isdn_generate_default_provider();
}

$ports = dahdi_get_ports("isdn", "te");
$groups = dahdi_get_portgroups("isdn");
$gateways = redfone_get_gateways();

include("fbegin.inc");
d_start("providers_isdn_edit.php");

	// General
	d_header(gettext("General Settings"));
	d_field(gettext("Name"), "name", 40,
		gettext("Descriptive name for this provider."), "required");
	display_channel_language_selector($form['language'], 1);
	?><tr>
		<td valign="top" class="vncell"><?=gettext("Hardware Port");?></td>
		<td colspan="<?=$colspan;?>" class="vtable">
			<select name="port" class="formfld" id="port"><?
			foreach ($ports as $port) {
				?><option value="<?=$port['uniqid'];?>" <?
				if ($port['uniqid'] == $form['port']) {
					echo "selected";
				}
				?>><?=gettext("Port");?> : <?=$port['name'];?></option><?
			}

			foreach ($groups as $group) {
				?><option value="<?=$group['uniqid'];?>" <?
				if ($group['uniqid'] == $form['port']) {
					echo "selected";
				}
				?>><?=gettext("Group");?> : <?=$group['name'];?></option><?
			}

			foreach ($gateways as $gateway) {
				for ($i = 1; $i < $gateway['spancount'] + 1; $i++) {
					$spanid = $gateway['uniqid'] . "-" . $i;
					?><option value="<?=$spanid;?>" <?
					if ($spanid == $form['port']) {
						echo "selected";
					}
					?>><?=gettext("Gateway");?> : <?=$gateway['gwname'];?> - <?=$gateway['span' . $i . 'name'];?></option><?
				}
			}

			?></select>
			<br><span class="vexpl"><?=$help;?></span>
		</td>
	</tr><?
	d_spacer();


	// Call Routing
	d_header(gettext("Call Routing"));
	display_provider_dialpattern_editor($form['dialpattern'], 1);
	display_incoming_extension_selector(1);
	d_incoming_fax_editor();
	d_failover_provider($form['failover']);
	d_spacer();


	// Caller ID Options
	d_collapsible(gettext("Caller ID Options"));
	display_outgoing_callerid_options($form['calleridsource'], $form['calleridstring'], 1);
	display_incoming_callerid_override_options($form['override'], $form['overridestring'], 1);
	d_collapsible_end();
	d_spacer();


	// Advanced Options
	d_collapsible(gettext("Advanced Options"));
	d_manualattributes_editor($form['manualattributes']);
	d_collapsible_end();
	d_spacer();


d_submit();

?><script type="text/javascript" charset="utf-8"><?
	javascript_incoming_extension_selector($form['incomingextensionmap']);
?></script><?

include("fend.inc");
