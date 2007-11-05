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

$pgtitle = array("Providers", "IAX", "Edit Account");
require("guiconfig.inc");

if (!is_array($config['iax']['provider']))
	$config['iax']['provider'] = array();

iax_sort_providers();
$a_iaxproviders = &$config['iax']['provider'];

$a_iaxphones = iax_get_phones();

$pconfig['codec'] = array("ulaw");


$id = $_GET['id'];
if (isset($_POST['id']))
	$id = $_POST['id'];

/* pull current config into pconfig */
if (isset($id) && $a_iaxproviders[$id]) {
	$pconfig['name'] = $a_iaxproviders[$id]['name'];
	$pconfig['username'] = $a_iaxproviders[$id]['username'];
	$pconfig['authentication'] = $a_iaxproviders[$id]['authentication'];
	$pconfig['secret'] = $a_iaxproviders[$id]['secret'];
	$pconfig['host'] = $a_iaxproviders[$id]['host'];
	$pconfig['port'] = $a_iaxproviders[$id]['port'];
	$pconfig['prefix'] = $a_iaxproviders[$id]['prefix'];
	$pconfig['dialpattern'] = $a_iaxproviders[$id]['dialpattern'];
	$pconfig['language'] = $a_iaxproviders[$id]['language'];
	$pconfig['noregister'] = $a_iaxproviders[$id]['noregister'];
	$pconfig['qualify'] = $a_iaxproviders[$id]['qualify'];
	$pconfig['calleridsource'] = 
		isset($a_iaxproviders[$id]['calleridsource']) ? $a_iaxproviders[$id]['calleridsource'] : "phones";
	$pconfig['calleridstring'] = $a_iaxproviders[$id]['calleridstring'];
	$pconfig['incomingextensionmap'] = $a_iaxproviders[$id]['incomingextensionmap'];
	$pconfig['override'] = $a_iaxproviders[$id]['override'];
	if(!is_array($pconfig['codec'] = $a_iaxproviders[$id]['codec']))
		$pconfig['codec'] = array("ulaw", "gsm");
}

if ($_POST) {

	unset($input_errors);
	$_POST['dialpattern'] = split_and_clean_patterns($_POST['dialpattern']);
	$_POST['incomingextensionmap'] = gather_incomingextensionmaps($_POST);
	$pconfig = $_POST;
	$pconfig['codec'] = array("ulaw", "gsm");
	
	parse_str($_POST['a_codecs']);
	parse_str($_POST['v_codecs']);

	/* input validation */
	$reqdfields = explode(" ", "name username host");
	$reqdfieldsn = explode(",", "Name,Username,Host");
	
	do_input_validation($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

	if (($_POST['username'] && !asterisk_is_valid_username($_POST['username']))) {
		$input_errors[] = "A valid username must be specified.";
	}
	if (($_POST['secret'] && !asterisk_is_valid_secret($_POST['secret']))) {
		$input_errors[] = "A valid secret must be specified.";
	}
/*	if (($_POST['host'] && !is_hostname($_POST['host']))) {
		$input_errors[] = "A valid host must be specified.";
	}*/
	if (($_POST['port'] && !is_port($_POST['port']))) {
		$input_errors[] = "A valid port must be specified.";
	}	
	if (($_POST['qualify'] && !is_numericint($_POST['qualify']))) {
		$input_errors[] = "A whole number of seconds must be entered for the \"qualify\" timeout.";
	}
	if ($_POST['calleridsource'] == "string" && !asterisk_is_valid_callerid_string($_POST['calleridstring'])) {
		$input_errors[] = "A valid Caller ID string must be specified.";
	}

	// pattern validation
	if (isset($id)) {
		$current_provider_id = $a_iaxproviders[$id]['uniqid'];
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
			if ($map['incomingpattern'] && !asterisk_is_valid_dialpattern($map, &$internal_error)) {
				$input_errors[] = "The incoming extension pattern \"{$map['incomingpattern']}\" is invalid. $internal_error";
			}
		}
	}
	

	if (!$input_errors) {
		$sp = array();		
		$sp['name'] = $_POST['name'];
		$sp['username'] = $_POST['username'];
		$sp['authentication'] = $_POST['authentication'];
		$sp['secret'] = $_POST['secret'];
		$sp['host'] = $_POST['host'];
		$sp['port'] = $_POST['port'];
		
		$sp['dialpattern'] = $_POST['dialpattern'];
		
		$sp['language'] = $_POST['language'];
		$sp['noregister'] = $_POST['noregister'];
		$sp['qualify'] = $_POST['qualify'];
		
		$sp['calleridsource'] = $_POST['calleridsource'];
		$sp['calleridstring'] = $_POST['calleridstring'];
		
		$sp['incomingextensionmap'] = $_POST['incomingextensionmap'];
		$sp['override'] = $_POST['override'];
		
		$sp['codec'] = array();
		$sp['codec'] = array_merge($ace, $vce);

		if (isset($id) && $a_iaxproviders[$id]) {
			$sp['uniqid'] = $a_iaxproviders[$id]['uniqid'];
			$a_iaxproviders[$id] = $sp;
		 } else {
			$sp['uniqid'] = "IAX-PROVIDER-" . uniqid(rand());
			$a_iaxproviders[] = $sp;
		}
		
		touch($d_iaxconfdirty_path);
		
		write_config();
		
		header("Location: providers_iax.php");
		exit;
	}
}
?>
<?php include("fbegin.inc"); ?>
<?php if ($input_errors) print_input_errors($input_errors); ?>
	<form action="providers_iax_edit.php" method="post" name="iform" id="iform">
		<table width="100%" border="0" cellpadding="6" cellspacing="0">
			<tr> 
				<td width="20%" valign="top" class="vncellreq">Name</td>
				<td width="80%" colspan="2" class="vtable">
					<input name="name" type="text" class="formfld" id="name" size="40" value="<?=htmlspecialchars($pconfig['name']);?>"> 
					<br><span class="vexpl">Descriptive name of this provider.</span>
				</td>
			</tr>
			<? display_provider_dialpattern_editor($pconfig['dialpattern'], 2); ?>
			<tr> 
				<td valign="top" class="vncellreq">Username</td>
				<td colspan="2" class="vtable">
					<input name="username" type="text" class="formfld" id="username" size="40" value="<?=htmlspecialchars($pconfig['username']);?>">
				</td>
			</tr>
			<tr>
				<td valign="top" class="vncell">Secret</td>
				<td colspan="2" class="vtable">
					<select name="authentication" class="formfld" id="authentication">
						<option value="plaintext" <? 
							if ($pconfig['authentication'] == "plaintext") 
								echo "selected"; 
							?>>plaintext</option>
						<option value="md5" <? 
							if ($pconfig['authentication'] == "md5") 
								echo "selected"; 
							?>>md5</option>
					</select>&nbsp;
					<input name="secret" type="password" class="formfld" id="secret" size="40" value="<?=htmlspecialchars($pconfig['secret']);?>"> 
                    <br><span class="vexpl">This account's password and authentication scheme.</span>
				</td>
			</tr>
			<tr> 
				<td valign="top" class="vncellreq">Host</td>
				<td colspan="2" class="vtable">
					<input name="host" type="text" class="formfld" id="host" size="40" value="<?=htmlspecialchars($pconfig['host']);?>">
					:
					<input name="port" type="text" class="formfld" id="port" size="10" maxlength="5" value="<?=htmlspecialchars($pconfig['port']);?>"> 
					<br><span class="vexpl">IAX proxy host URL or IP address and optional port.</span>
				</td>
			</tr>
			<? display_outgoing_callerid_options($pconfig['calleridsource'], $pconfig['calleridstring'], 2); ?>
			<? display_channel_language_selector($pconfig['language'], 2); ?>
			<? display_registration_options($pconfig['noregister'], 2); ?>
			<? display_qualify_options($pconfig['qualify'], 2); ?>
			<? display_incoming_extension_selector(2); ?>
			<? display_incoming_callerid_override_options($pconfig['override'], 2); ?>
			<? display_audio_codec_selector($pconfig['codec']); ?>
			<? display_video_codec_selector($pconfig['codec']); ?>
			<tr> 
				<td valign="top">&nbsp;</td>
				<td colspan="2">
					<input name="Submit" type="submit" class="formbtn" value="Save" onclick="save_codec_states()">
					<input id="a_codecs" name="a_codecs" type="hidden" value="">
					<input id="v_codecs" name="v_codecs" type="hidden" value="">					 
					<?php if (isset($id) && $a_iaxproviders[$id]): ?>
					<input name="id" type="hidden" value="<?=$id;?>"> 
					<?php endif; ?>
				</td>
			</tr>
		</table>
	</form>
<script type="text/javascript" charset="utf-8">
// <![CDATA[

	<? js_incoming_extension_selector($pconfig['incomingextensionmap']); ?>

	Sortable.create("ace",
		{dropOnEmpty:true,containment:["ace","acd"],constraint:false});
	Sortable.create("acd",
		{dropOnEmpty:true,containment:["ace","acd"],constraint:false});
	Sortable.create("vce",
		{dropOnEmpty:true,containment:["vce","vcd"],constraint:false});
	Sortable.create("vcd",
		{dropOnEmpty:true,containment:["vce","vcd"],constraint:false});	

	function save_codec_states() {
		var acs = document.getElementById('a_codecs');
		acs.value = Sortable.serialize('ace');
		var vcs = document.getElementById('v_codecs');
		vcs.value = Sortable.serialize('vce');
	}
// ]]>			
</script>
<?php include("fend.inc"); ?>
