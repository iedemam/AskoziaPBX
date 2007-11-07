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

require_once("functions.inc");

$needs_scriptaculous = true;
$needs_jquery = true;

$pgtitle = array("Providers", "ISDN", "Edit Line");
require("guiconfig.inc");

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
	$pconfig['incomingextensionmap'] = $a_isdnproviders[$id]['incomingextensionmap'];
	$pconfig['override'] = $a_isdnproviders[$id]['override'];
}

if ($_POST) {

	unset($input_errors);
	$_POST['dialpattern'] = split_and_clean_patterns($_POST['dialpattern']);
	$_POST['incomingextensionmap'] = gather_incomingextensionmaps($_POST);
	$pconfig = $_POST;

	/* input validation */
	$reqdfields = explode(" ", "name");
	$reqdfieldsn = explode(",", "Name");
	
	do_input_validation($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

	// pattern validation
	if (isset($id)) {
		$current_provider_id = $a_isdnproviders[$id]['uniqid'];
	}
	if (is_array($_POST['dialpattern'])) {
		foreach($_POST['dialpattern'] as $p) {
			if (asterisk_dialpattern_exists($p, &$return_provider_name, $current_provider_id)) {
				$input_errors[] = "The dial-pattern \"$p\" already exists for \"$return_provider_name\".";
			}
			if (!asterisk_is_valid_dialpattern($p, &$internal_error)) {
				$input_errors[] = "The dial-pattern \"$p\" is invalid. $internal_error";
			}
		}
	}
	if (is_array($_POST['incomingextensionmap'])) {
		foreach($_POST['incomingextensionmap'] as $map) {
			/* XXX : check for duplicates
			if (asterisk_dialpattern_exists($p, &$return_provider_name, $current_provider_id)) {
				$input_errors[] = "The dial-pattern \"$p\" already exists for \"$return_provider_name\".";
			}*/
			if ($map['incomingpattern'] && !asterisk_is_valid_dialpattern($map, &$internal_error, true)) {
				$input_errors[] = "The incoming extension pattern \"{$map['incomingpattern']}\" is invalid. $internal_error";
			}
		}
	}
	

	if (!$input_errors) {
		$ip = array();		
		$ip['name'] = $_POST['name'];
		$ip['interface'] = $_POST['interface'];
		$ip['msn'] = $_POST['msn'];
		$ip['language'] = $_POST['language'];
		
		$ip['dialpattern'] = $_POST['dialpattern'];
		$ip['incomingextensionmap'] = $_POST['incomingextensionmap'];
		$ip['override'] = $_POST['override'];
		
		if (isset($id) && $a_isdnproviders[$id]) {
			$ip['uniqid'] = $a_isdnproviders[$id]['uniqid'];
			$a_isdnproviders[$id] = $ip;
		 } else {
			$ip['uniqid'] = "ISDN-PROVIDER-" . uniqid(rand());
			$a_isdnproviders[] = $ip;
		}
		
		touch($d_isdnconfdirty_path);
		
		write_config();
		
		header("Location: providers_isdn.php");
		exit;
	}
}
?>
<?php include("fbegin.inc"); ?>
<?php if ($input_errors) print_input_errors($input_errors); ?>
	<form action="providers_isdn_edit.php" method="post" name="iform" id="iform">
		<table width="100%" border="0" cellpadding="6" cellspacing="0">
			<tr> 
				<td width="20%" valign="top" class="vncellreq">Name</td>
				<td width="80%" colspan="1" class="vtable">
					<input name="name" type="text" class="formfld" id="name" size="40" value="<?=htmlspecialchars($pconfig['name']);?>"> 
					<br><span class="vexpl">Descriptive name of this provider.</span>
				</td>
			</tr>
			<tr> 
				<td width="20%" valign="top" class="vncellreq">MSN</td>
				<td width="80%" colspan="1" class="vtable">
					<input name="msn" type="text" class="formfld" id="msn" size="40" value="<?=htmlspecialchars($pconfig['msn']);?>"> 
					<br><span class="vexpl">Telephone number assigned to this line.</span>
				</td>
			</tr>
			<? display_provider_dialpattern_editor($pconfig['dialpattern'], 1); ?>
			<tr> 
				<td valign="top" class="vncell">ISDN Interface</td>
				<td class="vtable">
					<select name="interface" class="formfld" id="interface"><?
					
					$isdn_interfaces = isdn_get_te_interfaces();
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
			<? display_channel_language_selector($pconfig['language'], 1); ?>
			<? display_incoming_extension_selector(1); ?>
			<? display_incoming_callerid_override_options($pconfig['override'], 1); ?>
			<tr> 
				<td valign="top">&nbsp;</td>
				<td>
					<input name="Submit" type="submit" class="formbtn" value="Save">
					<?php if (isset($id) && $a_isdnproviders[$id]): ?>
					<input name="id" type="hidden" value="<?=$id;?>"> 
					<?php endif; ?>
				</td>
			</tr>
		</table>
	</form>
<script language="JavaScript">
<!-- 

<? js_incoming_extension_selector($pconfig['incomingextensionmap']); ?>

//-->
</script>
<?php include("fend.inc"); ?>
