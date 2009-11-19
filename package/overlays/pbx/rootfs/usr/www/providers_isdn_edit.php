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

$needs_scriptaculous = false;

require("guiconfig.inc");

$pgtitle = array(gettext("Providers"), gettext("Edit ISDN Line"));

if (!is_array($config['isdn']['provider']))
	$config['isdn']['provider'] = array();

isdn_sort_providers();
$a_isdnproviders = &$config['isdn']['provider'];

$a_isdnphones = isdn_get_phones();


$id = $_GET['id'];
if (isset($_POST['id']))
	$id = $_POST['id'];

/* pull current config into pconfig */
if (isset($id) && $a_isdnproviders[$id]) {
	$pconfig['name'] = $a_isdnproviders[$id]['name'];
	$pconfig['interface'] = $a_isdnproviders[$id]['interface'];
	$pconfig['msn'] = $a_isdnproviders[$id]['msn'];
	$pconfig['language'] = $a_isdnproviders[$id]['language'];
	$pconfig['dialpattern'] = $a_isdnproviders[$id]['dialpattern'];
	$pconfig['calleridsource'] = 
		isset($a_isdnproviders[$id]['calleridsource']) ? $a_isdnproviders[$id]['calleridsource'] : "phones";
	$pconfig['calleridstring'] = $a_isdnproviders[$id]['calleridstring'];
	$pconfig['incomingextensionmap'] = $a_isdnproviders[$id]['incomingextensionmap'];
	$pconfig['override'] = $a_isdnproviders[$id]['override'];
	$pconfig['overridestring'] = $a_isdnproviders[$id]['overridestring'];
}

if ($_POST) {

	unset($input_errors);
	$_POST['dialpattern'] = split_and_clean_lines($_POST['dialpattern']);
	$_POST['incomingextensionmap'] = gather_incomingextensionmaps($_POST);
	$pconfig = $_POST;

	/* input validation */
	$reqdfields = explode(" ", "name");
	$reqdfieldsn = explode(",", "Name");
	
	verify_input($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

	if ($_POST['calleridsource'] == "string" && !pbx_is_valid_callerid_string($_POST['calleridstring'])) {
		$input_errors[] = gettext("A valid Caller ID string must be specified.");
	}
	if (($_POST['override'] == "prepend" || $_POST['override'] == "replace") && !$_POST['overridestring']) {
		$input_errors[] = gettext("An incoming Caller ID override string must be specified.");
	}

	// pattern validation
	if (isset($id)) {
		$current_provider_id = $a_isdnproviders[$id]['uniqid'];
	}
	if (is_array($_POST['dialpattern'])) {
		foreach($_POST['dialpattern'] as $p) {
			/*if (pbx_dialpattern_exists($p, &$return_provider_name, $current_provider_id)) {
				$input_errors[] = "The dial-pattern \"$p\" already exists for \"$return_provider_name\".";
			}*/
			if (!pbx_is_valid_dialpattern($p, &$internal_error)) {
				$input_errors[] = sprintf(gettext("The dial-pattern \"%s\" is invalid. %s"), $p, $internal_error);
			}
		}
	}
	if (is_array($_POST['incomingextensionmap'])) {
		foreach($_POST['incomingextensionmap'] as $map) {
			/* XXX : check for duplicates */
			if ($map['incomingpattern'] && !pbx_is_valid_dialpattern($map['incomingpattern'], &$internal_error, true)) {
				$input_errors[] = sprintf(gettext("The incoming extension pattern \"%s\" is invalid. %s"), $map['incomingpattern'], $internal_error);
			}
		}
	}
	if ($_POST['msn'] && !verify_is_numericint($_POST['msn'])) {
		$input_errors[] = gettext("A valid default number (MSN) must be specified.");
	}
	

	if (!$input_errors) {
		$ip = array();		
		$ip['name'] = $_POST['name'];
		$ip['interface'] = $_POST['interface'];
		$ip['msn'] = $_POST['msn'];
		$ip['language'] = $_POST['language'];

		$ip['calleridsource'] = $_POST['calleridsource'];
		$ip['calleridstring'] = $_POST['calleridstring'];

		$ip['dialpattern'] = $_POST['dialpattern'];
		$ip['incomingextensionmap'] = $_POST['incomingextensionmap'];
		$ip['override'] = ($_POST['override'] != "disable") ? $_POST['override'] : false;
		$ip['overridestring'] = verify_non_default($_POST['overridestring']);
		
		if (isset($id) && $a_isdnproviders[$id]) {
			$ip['uniqid'] = $a_isdnproviders[$id]['uniqid'];
			$a_isdnproviders[$id] = $ip;
		 } else {
			$ip['uniqid'] = "ISDN-PROVIDER-" . uniqid(rand());
			$a_isdnproviders[] = $ip;
		}
		
		touch($d_isdnconfdirty_path);
		
		write_config();
		
		header("Location: accounts_providers.php");
		exit;
	}
}

include("fbegin.inc");

?><script type="text/JavaScript">
<!--

	jQuery(document).ready(function(){

		<?=javascript_advanced_settings("ready");?>

	});

//-->
</script><?

if ($input_errors) display_input_errors($input_errors);

$isdn_interfaces = isdn_get_te_interfaces();

if (count($isdn_interfaces) == 0) {

	$page_link = '<a href="ports_isdn.php">' . gettext("Ports") . ": " . gettext("ISDN") . '</a>';
	$interfaces_warning = sprintf(gettext("<strong>No compatible interfaces found!</strong><br><br> To configure this type of account, make sure an appropriately configured interface is present on the %s page"), $page_link);
	display_info_box($interfaces_warning, "keep");
	
} else {

	?><form action="providers_isdn_edit.php" method="post" name="iform" id="iform">
		<table width="100%" border="0" cellpadding="6" cellspacing="0">
			<tr> 
				<td width="20%" valign="top" class="vncellreq"><?=gettext("Name");?></td>
				<td width="80%" colspan="1" class="vtable">
					<input name="name" type="text" class="formfld" id="name" size="40" value="<?=htmlspecialchars($pconfig['name']);?>"> 
					<br><span class="vexpl"><?=gettext("Descriptive name for this provider.");?></span>
				</td>
			</tr>
			<tr> 
				<td width="20%" valign="top" class="vncellreq"><?=gettext("Number");?></td>
				<td width="80%" colspan="1" class="vtable">
					<input name="msn" type="text" class="formfld" id="msn" size="40" value="<?=htmlspecialchars($pconfig['msn']);?>"> 
					<br><span class="vexpl"><?=gettext("The default telephone number (MSN) assigned to this line. Individual MSN mappings can be defined below.");?></span>
				</td>
			</tr>
			<? display_provider_dialpattern_editor($pconfig['dialpattern'], 1); ?>
			<tr> 
				<td valign="top" class="vncell"><?=gettext("ISDN Interface");?></td>
				<td class="vtable">
					<select name="interface" class="formfld" id="interface"><?

					foreach ($isdn_interfaces as $interface) {
						?><option value="<?=$interface['unit'];?>" <?
						if ($interface['unit'] == $pconfig['interface']) {
							echo "selected";
						}
						?>><?=$interface['name'];?></option><?
					}

					?></select>
				</td>
			</tr>
			<? display_outgoing_callerid_options($pconfig['calleridsource'], $pconfig['calleridstring'], 1); ?>
			<? display_channel_language_selector($pconfig['language']); ?>
			<? display_incoming_extension_selector(1); ?>
			<? display_advanced_settings_begin(1); ?>
			<? display_incoming_callerid_override_options($pconfig['override'], $pconfig['overridestring'], 1); ?>
			<? display_advanced_settings_end(); ?>
			<tr> 
				<td valign="top">&nbsp;</td>
				<td>
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>">
					<?php if (isset($id) && $a_isdnproviders[$id]): ?>
					<input name="id" type="hidden" value="<?=$id;?>"> 
					<?php endif; ?>
				</td>
			</tr>
		</table>
	</form><?

}

?><script language="JavaScript">
<!-- 

<? javascript_incoming_extension_selector($pconfig['incomingextensionmap']); ?>

//-->
</script><?

include("fend.inc");
