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

$pgtitle = array("Phones", "SIP", "Edit Account");
require("guiconfig.inc");

/* grab and sort the sip phones in our config */
if (!is_array($config['sip']['phone']))
	$config['sip']['phone'] = array();

sip_sort_phones();
$a_sipphones = &$config['sip']['phone'];

$pconfig['codec'] = array("ulaw");

$id = $_GET['id'];
if (isset($_POST['id']))
	$id = $_POST['id'];

/* pull current config into pconfig */
if (isset($id) && $a_sipphones[$id]) {
	$pconfig['extension'] = $a_sipphones[$id]['extension'];
	$pconfig['callerid'] = $a_sipphones[$id]['callerid'];
	$pconfig['secret'] = $a_sipphones[$id]['secret'];
	$pconfig['provider'] = $a_sipphones[$id]['provider'];
	$pconfig['voicemailbox'] = $a_sipphones[$id]['voicemailbox'];
	$pconfig['sendcallnotifications'] = isset($a_sipphones[$id]['sendcallnotifications']);
	$pconfig['language'] = $a_sipphones[$id]['language'];
	$pconfig['dtmfmode'] = $a_sipphones[$id]['dtmfmode'];
	$pconfig['qualify'] = $a_sipphones[$id]['qualify'];
	
	$pconfig['calllimit'] = isset($a_sipphones[$id]['calllimit']) ? $a_sipphones[$id]['calllimit'] : "2";
	$pconfig['busylimit'] = isset($a_sipphones[$id]['busylimit']) ? $a_sipphones[$id]['busylimit'] : "1";
	
	if(!is_array($pconfig['codec'] = $a_sipphones[$id]['codec']))
		$pconfig['codec'] = array("ulaw");
	$pconfig['descr'] = $a_sipphones[$id]['descr'];
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

	if (($_POST['extension'] && !asterisk_is_valid_extension($_POST['extension']))) {
		$input_errors[] = "An extension must be a one to four digit number.";
	}
	if (($_POST['callerid'] && !asterisk_is_valid_callerid($_POST['callerid']))) {
		$input_errors[] = "A valid Caller ID must be specified.";
	}
	if (($_POST['secret'] && !asterisk_is_valid_secret($_POST['secret']))) {
		$input_errors[] = "A valid secret must be specified.";
	}
	if (!isset($id) && in_array($_POST['extension'], asterisk_get_extensions())) {
		$input_errors[] = "A phone with this extension already exists.";
	}
	if (($_POST['port'] && !is_port($_POST['port']))) {
		$input_errors[] = "A valid port must be specified.";
	}
	if (($_POST['voicemailbox'] && !is_email_address($_POST['voicemailbox']))) {
		$input_errors[] = "A valid e-mail address must be specified.";
	}
	if (($_POST['qualify'] && !is_numericint($_POST['qualify']))) {
		$input_errors[] = "A whole number of seconds must be entered for the \"qualify\" timeout.";
	}
	
	if (($_POST['calllimit'] && !is_numericint($_POST['calllimit']))) {
		$input_errors[] = "A whole number of calls must be entered for the \"call limit.\"";
	}
	if (($_POST['busylimit'] && !is_numericint($_POST['busylimit']))) {
		$input_errors[] = "A whole number of calls must be entered for the \"busy limit.\"";
	}

	if (!$input_errors) {
		$sp = array();
		$sp['extension'] = $_POST['extension'];
		$sp['callerid'] = $_POST['callerid'];
		$sp['secret'] = $_POST['secret'];
		$sp['voicemailbox'] = $_POST['voicemailbox'];
		$sp['sendcallnotifications'] = $_POST['sendcallnotifications'] ? true : false;
		$sp['language'] = $_POST['language'];
		$sp['dtmfmode'] = $_POST['dtmfmode'];
		$sp['qualify'] = $_POST['qualify'];
		
		// XXX the proper way of setting posted options with default values...
		$sp['calllimit'] = ($_POST['calllimit'] != "2") ? $_POST['calllimit'] : false;
		$sp['busylimit'] = ($_POST['busylimit'] != "1") ? $_POST['busylimit'] : false;
		
		$sp['descr'] = $_POST['descr'];

		$a_providers = asterisk_get_providers();
		$sp['provider'] = array();
		foreach ($a_providers as $provider)
			if($_POST[$provider['uniqid']] == true)
				$sp['provider'][] = $provider['uniqid'];
		
		$sp['codec'] = array();
		$sp['codec'] = array_merge($ace, $vce);

		if (isset($id) && $a_sipphones[$id]) {
			$sp['uniqid'] = $a_sipphones[$id]['uniqid'];
			$a_sipphones[$id] = $sp;
		 } else {
			$sp['uniqid'] = "SIP-PHONE-" . uniqid(rand());
			$a_sipphones[] = $sp;
		}
		
		touch($d_sipconfdirty_path);
		
		write_config();
		
		header("Location: phones_sip.php");
		exit;
	}
}
?>
<?php include("fbegin.inc"); ?>
<?php if ($input_errors) print_input_errors($input_errors); ?>
	<form action="phones_sip_edit.php" method="post" name="iform" id="iform">
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
					<input name="secret" type="password" class="formfld" id="secret" size="40" value="<?=htmlspecialchars($pconfig['secret']);?>"> 
                    <br><span class="vexpl">This account's password.</span>
				</td>
			</tr>
			<? display_call_notifications_editor($pconfig['voicemailbox'], $pconfig['sendcallnotifications'], 2); ?>
			<? display_channel_language_selector($pconfig['language'], 2); ?>
			<? display_dtmfmode_selector($pconfig['dtmfmode'], 2); ?>
			<tr> 
				<td valign="top" class="vncell">Qualify</td>
				<td colspan="2" class="vtable">
					<input name="qualify" type="text" class="formfld" id="qualify" size="5" value="<?=htmlspecialchars($pconfig['qualify']);?>">&nbsp;seconds 
                    <br><span class="vexpl">Packets will be sent to this phone every <i>n</i> seconds to check its status.
					<br>Defaults to '2'. Set to '0' to disable.</span>
				</td>
			</tr>
			<? display_call_and_busy_limit_selector($pconfig['calllimit'], $pconfig['busylimit'], 2); ?>
			<? display_provider_access_selector($pconfig['provider'], 2); ?>
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
			<tr> 
				<td valign="top">&nbsp;</td>
				<td colspan="2">
					<input name="Submit" type="submit" class="formbtn" value="Save" onclick="save_codec_states()">
					<input id="a_codecs" name="a_codecs" type="hidden" value="">
					<input id="v_codecs" name="v_codecs" type="hidden" value="">
					<?php if (isset($id) && $a_sipphones[$id]): ?>
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
