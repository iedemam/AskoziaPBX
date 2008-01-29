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
	
	THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
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

$pgtitle = array("Phones", "Edit IAX Account");
require("guiconfig.inc");

/* grab and sort the iax phones in our config */
if (!is_array($config['iax']['phone']))
	$config['iax']['phone'] = array();

iax_sort_phones();
$a_iaxphones = &$config['iax']['phone'];

$pconfig['codec'] = array("ulaw");

$id = $_GET['id'];
if (isset($_POST['id']))
	$id = $_POST['id'];

/* pull current config into pconfig */
if (isset($id) && $a_iaxphones[$id]) {
	$pconfig['extension'] = $a_iaxphones[$id]['extension'];
	$pconfig['callerid'] = $a_iaxphones[$id]['callerid'];
	$pconfig['authentication'] = $a_iaxphones[$id]['authentication'];
	$pconfig['secret'] = $a_iaxphones[$id]['secret'];
	$pconfig['provider'] = $a_iaxphones[$id]['provider'];
	$pconfig['outbounduridial'] = isset($a_iaxphones[$id]['outbounduridial']);
	$pconfig['voicemailbox'] = $a_iaxphones[$id]['voicemailbox'];
	$pconfig['sendcallnotifications'] = isset($a_iaxphones[$id]['sendcallnotifications']);
	$pconfig['allowdirectdial'] = isset($a_iaxphones[$id]['allowdirectdial']);
	$pconfig['publicname'] = $a_iaxphones[$id]['publicname'];
	$pconfig['language'] = $a_iaxphones[$id]['language'];
	$pconfig['qualify'] = $a_iaxphones[$id]['qualify'];
	if(!is_array($pconfig['codec'] = $a_iaxphones[$id]['codec']))
		$pconfig['codec'] = array("ulaw");
	$pconfig['descr'] = $a_iaxphones[$id]['descr'];
}

if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;
	$pconfig['codec'] = array("ulaw");
	
	parse_str($_POST['a_codecs']);
	parse_str($_POST['v_codecs']);

	/* input validation */
	$reqdfields = explode(" ", "extension callerid");
	$reqdfieldsn = explode(",", "Extension,Caller ID");
	
	do_input_validation($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

	if (($_POST['extension'] && !pbx_is_valid_extension($_POST['extension']))) {
		$input_errors[] = "An extension must be a one to four digit number.";
	}
	if (($_POST['callerid'] && !pbx_is_valid_callerid($_POST['callerid']))) {
		$input_errors[] = "A valid Caller ID must be specified.";
	}
	if (($_POST['secret'] && !pbx_is_valid_secret($_POST['secret']))) {
		$input_errors[] = "A valid secret must be specified.";
	}
	if (!isset($id) && in_array($_POST['extension'], pbx_get_extensions())) {
		$input_errors[] = "A phone with this extension already exists.";
	}
	if (($_POST['port'] && !verify_is_port($_POST['port']))) {
		$input_errors[] = "A valid port must be specified.";
	}
	if (($_POST['voicemailbox'] && !verify_is_email_address($_POST['voicemailbox']))) {
		$input_errors[] = "A valid e-mail address must be specified.";
	}
	if (($_POST['qualify'] && !verify_is_numericint($_POST['qualify']))) {
		$input_errors[] = "A whole number of seconds must be entered for the \"qualify\" timeout.";
	}
	if ($_POST['publicname'] && ($msg = verify_is_public_name($_POST['publicname']))) {
		$input_errors[] = $msg;
	}

	if (!$input_errors) {
		$sp = array();
		$sp['extension'] = $_POST['extension'];
		$sp['callerid'] = $_POST['callerid'];
		$sp['authentication'] = $_POST['authentication'];
		$sp['secret'] = $_POST['secret'];
		$sp['voicemailbox'] = verify_non_default($_POST['voicemailbox']);
		$sp['sendcallnotifications'] = $_POST['sendcallnotifications'] ? true : false;
		$sp['allowdirectdial'] = $_POST['allowdirectdial'] ? true : false;
		$sp['publicname'] = verify_non_default($_POST['publicname']);
		$sp['language'] = $_POST['language'];
		$sp['descr'] = verify_non_default($_POST['descr']);
		$sp['qualify'] = $_POST['qualify'];

		$a_providers = pbx_get_providers();
		$sp['provider'] = array();
		foreach ($a_providers as $provider) {
			if($_POST[$provider['uniqid']] == true) {
				$sp['provider'][] = $provider['uniqid'];
			}
		}
		$sp['outbounduridial'] = $_POST['outbounduridial'] ? true : false;
		
		$sp['codec'] = array();
		$sp['codec'] = array_merge($ace, $vce);

		if (isset($id) && $a_iaxphones[$id]) {
			$sp['uniqid'] = $a_iaxphones[$id]['uniqid'];
			$a_iaxphones[$id] = $sp;
		 } else {
			$sp['uniqid'] = "IAX-PHONE-" . uniqid(rand());
			$a_iaxphones[] = $sp;
		}
		
		touch($d_iaxconfdirty_path);
		
		write_config();
		
		header("Location: accounts_phones.php");
		exit;
	}
}
?>
<?php include("fbegin.inc"); ?>
<script type="text/JavaScript">
<!--
	<?=javascript_public_direct_dial_editor("functions");?>

	jQuery(document).ready(function(){

		<?=javascript_public_direct_dial_editor("ready");?>
		<?=javascript_advanced_settings("ready");?>

	});

//-->
</script>
<?php if ($input_errors) print_input_errors($input_errors); ?>
	<form action="phones_iax_edit.php" method="post" name="iform" id="iform">
		<table width="100%" border="0" cellpadding="6" cellspacing="0">
			<tr> 
				<td width="20%" valign="top" class="vncellreq">Extension</td>
				<td width="80%" colspan="2" class="vtable">
					<input name="extension" type="text" class="formfld" id="extension" size="20" value="<?=htmlspecialchars($pconfig['extension']);?>"> 
					<br><span class="vexpl">This is also this account's username.</span>
				</td>
			</tr>
			<tr> 
				<td valign="top" class="vncellreq">Caller ID</td>
				<td colspan="2" class="vtable">
					<input name="callerid" type="text" class="formfld" id="callerid" size="40" value="<?=htmlspecialchars($pconfig['callerid']);?>"> 
					<br><span class="vexpl">Text to be displayed for Caller ID.</span>
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
			<? display_call_notifications_editor($pconfig['voicemailbox'], $pconfig['sendcallnotifications'], 2); ?>
			<? display_public_direct_dial_editor($pconfig['allowdirectdial'], $pconfig['publicname'], 2); ?>
			<? display_channel_language_selector($pconfig['language'], 2); ?>
			<? display_provider_access_selector($pconfig['provider'], $pconfig['outbounduridial'], 2); ?>
			<? display_audio_codec_selector($pconfig['codec']); ?>
			<? display_video_codec_selector($pconfig['codec']); ?>
			<tr> 
				<td valign="top" class="vncell">Description</td>
				<td colspan="2" class="vtable">
					<input name="descr" type="text" class="formfld" id="descr" size="40" value="<?=htmlspecialchars($pconfig['descr']);?>"> 
					<br><span class="vexpl">You may enter a description here 
					for your reference (not parsed).</span>
				</td>
			</tr>
			<? display_advanced_settings_begin(2); ?>
			<tr> 
				<td valign="top" class="vncell">Qualify</td>
				<td colspan="2" class="vtable">
					<input name="qualify" type="text" class="formfld" id="qualify" size="5" value="<?=htmlspecialchars($pconfig['qualify']);?>">&nbsp;seconds 
                    <br><span class="vexpl">Packets will be sent to this phone every <i>n</i> seconds to check its status.
					<br>Defaults to '2'. Set to '0' to disable.</span>
				</td>
			</tr>
			<? display_advanced_settings_end(); ?>
			<tr> 
				<td valign="top">&nbsp;</td>
				<td>
					<input name="Submit" type="submit" class="formbtn" value="Save" onclick="save_codec_states()">
					<input id="a_codecs" name="a_codecs" type="hidden" value="">
					<input id="v_codecs" name="v_codecs" type="hidden" value="">
					<?php if (isset($id) && $a_iaxphones[$id]): ?>
					<input name="id" type="hidden" value="<?=$id;?>"> 
					<?php endif; ?>
				</td>
			</tr>
		</table>
	</form>
<script type="text/javascript" charset="utf-8">
// <![CDATA[
	
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
