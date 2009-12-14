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

function display_userdefined_email_notification($subj, $body) {

	?><span id="email_notification_wrapper" style="display: none;">
		<br><?=spanify(gettext("Subject"));?>
		<br><input type="text" name="user_subject" id="user_subject" size="40" value="<?=htmlspecialchars($subj);?>">
		<br><br><?=spanify(gettext("Body"));?>
		<br><textarea name="user_body" cols="60" rows="3" id="user_body" class="userdefined"><?=htmlspecialchars($body);?></textarea><br>
		<br><strong><?=spanify(gettext("The following variables can be used in the e-mail notification"));?>:</strong>
		<br>VM_MAILBOX = <?=spanify(gettext("extension number of voicemail box"));?>
		<br>VM_NAME = <?=spanify(gettext("name of voicemail recipient"));?>
		<br>VM_DURATION = <?=spanify(gettext("duration of the message (i.e. 0:58)"));?>
		<br>VM_CALLERID = <?=spanify(gettext("caller id of person leaving the message"));?>
		<br>VM_DATE = <?=spanify(gettext("full date / time (i.e. 2025-11-20 (15:45))"));?>
	</span><?
}

function display_passwd_generation() {

	?><a href="javascript:{}"><img class='verticalalign' src='key_go.png' name='generate' id="generate" title="<?=gettext("generate password");?>"></a>
	<span name="generator_wrapper" id="generator_wrapper" style="display: none;">
	<br><input type="text" name="gen_passwd" id="gen_passwd" value="">
	<a href="javascript:{}"><img class='verticalalign' src='add.png' name='use' id="use" title="<?=gettext("use this password");?>"></a>
	</span><?	
}

function display_gui_language_selector($current_language) {

	$languages = array(
		"en_US" => "English",
		"bg_BG" => "Български",
		"es_ES" => "Castellano",
		"zh_CN" => "简体中文",
		"zh_TW" => "繁體中文",
		"cs_CS" => "Čeština",
		"da_DA" => "Dansk",
		"de_DE" => "Deutsch",
		"el_GR" => "Ελληνικά",
		"fr_FR" => "Français",
		"it_IT" => "Italiano",
		"pl_PL" => "Język polski",
		"nl_NL" => "Nederlands",
		"jp_JP" => "日本語",
//		"pt_BR" => "Português",
		"fi_FI" => "Suomi",
//		"se_SE" => "Svenska",
		"tr_TR" => "Türkçe"
//		"ru_RU" => "Русский язык"
	);

	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=spanify(gettext("webGUI language"));?></td>
		<td width="80%" class="vtable">
			<select name="lang" class="formfld" id="lang">
			<? foreach ($languages as $lang => $langdesc) : ?>
			<option value="<?=$lang;?>" <?
			if ($lang == $current_language)
				echo "selected"; ?>
			><?=$langdesc;?></option>
			<? endforeach; ?>
			</select>
			<br><span class="vexpl"><?=spanify(gettext("Select in which language you want the webGUI to be displayed.")); ?>
			</span>
		</td>
	</tr><?
}

function display_audio_codec_selector($enabled_codecs) {
	global $scripts;

	$scripts['javascript_codec_selector'] = true;

	$audio_codecs = array(
		"ulaw" => "G.711 u-law",
		"alaw" => "G.711 A-law",
		"gsm" => "GSM",
		"g729" => "G.729A",
		"ilbc" => "iLBC",
		"speex" => "SpeeX",
		"g723" => "G.723.1",
		"g726aal2" => "G.726 AAL2",
		"adpcm" => "ADPCM",
		"slin" => "16 bit Signed Linear PCM",
		"lpc10" => "LPC10",
		"g726" => "G.726 RFC3551",
		"g722" => "G722"
	);

	if (!is_array($enabled_codecs)) {
		$enabled_codecs = array();
	}

	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=spanify(gettext("Audio Codecs"));?></td>
		<td width="40%" class="vtable" valign="top"><strong><?=spanify(gettext("Enabled"));?></strong>&nbsp;<i>(<?=spanify(gettext("drag-and-drop"));?>)</i>
			<ul id="ace" class="ace" style="min-height:50px; cursor:pointer;">
			<? foreach ($enabled_codecs as $codec): ?>
				<? if (array_key_exists($codec, $audio_codecs)): ?>
				<li class="ace" id="ace_<?=$codec;?>"><?=$audio_codecs[$codec];?></li>
				<? endif; ?>
			<? endforeach; ?>
			</ul>
		</td>
		<td width="40%" class="vtable" valign="top"><strong><?=spanify(gettext("Disabled"));?></strong>
			<ul id="acd" class="acd" style="min-height:50px; cursor:pointer;">
			<? foreach ($audio_codecs as $codec=>$friendly): ?>
				<? if (!in_array($codec, $enabled_codecs)): ?>
				<li class="acd" id="acd_<?=$codec;?>"><?=$friendly;?></li>
				<? endif; ?>
			<? endforeach; ?>
			</ul><input id="a_codecs" name="a_codecs" type="hidden" value="">
		</td>
	</tr><?	
}

function display_video_codec_selector($enabled_codecs) {
	global $scripts;

	$scripts['javascript_codec_selector'] = true;

	$video_codecs = array(
		"h261" => "H.261",
		"h263" => "H.263",
		"h263p" => "H.263+",
		"h264" => "H.264"
	);

	if (!is_array($enabled_codecs)) {
		$enabled_codecs = array();
	}

	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=spanify(gettext("Video Codecs"));?></td>
		<td width="40%" class="vtable" valign="top"><strong><?=spanify(gettext("Enabled"));?></strong>&nbsp;<i>(<?=spanify(gettext("drag-and-drop"));?>)</i>
			<ul id="vce" class="vce" style="min-height:50px; cursor:pointer;">
				<? foreach ($enabled_codecs as $codec): ?>
					<? if (array_key_exists($codec, $video_codecs)): ?>
					<li class="vce" id="vce_<?=$codec;?>"><?=$video_codecs[$codec];?></li>
					<? endif; ?>
				<? endforeach; ?>
			</ul>
		</td>
		<td width="40%" class="vtable" valign="top"><strong><?=spanify(gettext("Disabled"));?></strong>
			<ul id="vcd" class="vcd" style="min-height:50px; cursor:pointer;">
			<? foreach ($video_codecs as $codec=>$friendly): ?>
				<? if (!in_array($codec, $enabled_codecs)): ?>
				<li class="vcd" id="vcd_<?=$codec;?>"><?=$friendly;?></li>
				<? endif; ?>
			<? endforeach; ?>
			</ul><input id="v_codecs" name="v_codecs" type="hidden" value="">
		</td>
	</tr><?
}

function display_callgroup_member_selector($current_members) {
	
	$internal_phones = pbx_get_phones();
	$external_phones = external_get_phones();
	
	// XXX this should be handled at a lower level
	$indexed_phones = array();
	foreach($internal_phones as $phone) {
		$indexed_phones[$phone['uniqid']] = $phone;
	}
	foreach($external_phones as $phone) {
		$indexed_phones[$phone['uniqid']] = $phone;
	}
	
	if (!is_array($current_members)) {
		$current_members = array();
	}
	
	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=spanify(gettext("Group Members"));?></td>
		<td width="40%" class="vtable" valign="top"><strong><?=spanify(gettext("Members"));?></strong>&nbsp;<i>(<?=spanify(gettext("drag-and-drop"));?>)</i>
			<ul id="gme" class="gme" style="min-height:50px; cursor:pointer;"><? 
			
			foreach ($current_members as $member) {
				if (array_key_exists($member, $indexed_phones)) {
					?><li class="gme" id="gme_<?=$member;?>"><?=pbx_uniqid_to_name($member);?>&nbsp;&lt;<?=$indexed_phones[$member]['extension'];?>&gt;</li><?
				}
			}
			
			?></ul>
		</td>
		<td width="40%" class="vtable" valign="top"><strong><?=spanify(gettext("Non-Members"));?></strong>
			<ul id="gmd" class="gmd" style="min-height:50px; cursor:pointer;"><?
			

			foreach ($indexed_phones as $uniqid=>$phone) {
				if (!in_array($uniqid, $current_members)) {
					?><li class="gmd" id="gmd_<?=$uniqid;?>"><?=pbx_uniqid_to_name($phone['uniqid']);?>&nbsp;&lt;<?=$phone['extension'];?>&gt;</li><?
				}
			}
			
			?></ul>
		</td>
	</tr>
	<tr>
		<td width="20%" valign="top" class="vncell">&nbsp;</td>
		<td width="40%" class="vtable" valign="top" colspan="2">
			<span class="vexpl"><strong><span class="red"><?=spanify(gettext("Note:"));?></span></strong>
			<?=spanify(gettext("All group members will be called at the same time. <strong>The voicemail account associated with the first member will be used if the call is not answered."));?></strong></span>
		</td>
	</tr><?	
}

function display_incoming_extension_selector($colspan, $idprefix=false) {
	
	$table_id = $idprefix ? $idprefix . "_incoming_extension_selector" : "incoming_extension_selector";
	$row_id = $idprefix ? $idprefix . "_incoming_extension_selector_last" : "incoming_extension_selector_last";
	$add_params = $idprefix ? ",,\"$idprefix\"" : "";
	
	?><tr>
		<td width="20%" valign="top" class="vncell"><?=spanify(gettext("Incoming Extension(s)"));?></td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable">
			<table id="<?=$table_id;?>">
				<tr id="<?=$row_id;?>">
					<td>&nbsp;</td>
					<td><img src="add.png" title="<?=gettext("add incoming extension mapping");?>" border="0" style="cursor:pointer;" onclick="add_incoming_extension_selector(<?=$add_params;?>)"></td>
				</tr>
			</table><br><?
			
			// multiple instances don't need help for every instance
			if (!$idprefix) {				
				?><span class="vexpl"><?=spanify(gettext("Enter a pattern or a phone number and select a destination extension. To ring multiple phones, simply define them with the same pattern. If a phone number is entered, it will used as the \"external\" number on unanswered calls. If nothing is entered, all calls will be directed to the destination extension.<br>\n <ul>\n \t<li><strong>X</strong> - matches digits 0-9</li>\n \t<li><strong>Z</strong> - matches digits 1-9</li>\n \t<li><strong>N</strong> - matches digits 2-9</li>\n \t<li><strong>[13-5]</strong> - matches any digit in the brackets (here, 1,3,4,5)</li>\n \t<li><strong>.</strong> - matches one or more characters</li>\n \t<li><strong>!</strong> - matches zero or more characters</li>\n </ul>\n"));?></span><?
			}

		?></td>
	</tr><?

}

function display_dtmfmode_selector($current_dtmfmode, $colspan) {

	$dtmfmodes = array(
		"auto",
		"rfc2833",
		"inband",
		"info"
	);

	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=spanify(gettext("DTMF Mode"));?></td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable">
			<select name="dtmfmode" class="formfld" id="dtmfmode">
			<? foreach ($dtmfmodes as $dtmfmode) : ?>
			<option value="<?=$dtmfmode;?>" <?
			if ($dtmfmode == $current_dtmfmode)
				echo "selected"; ?>
			><?=$dtmfmode;?></option>
			<? endforeach; ?>
			</select>
		</td>
	</tr><?
}

function display_registration_options($current_noregister, $current_manualregister, $colspan) {
	
	?><tr> 
		<td valign="top" class="vncell"><?=spanify(gettext("Registration"));?></td>
		<td colspan="<?=$colspan?>" class="vtable">
			<input name="noregister" id="noregister" type="checkbox" value="yes" <? if ($current_noregister) echo "checked"; ?>><?=spanify(gettext("Do not register with this provider."));?>
			<br>
			<input name="manualregister" type="text" class="formfld" id="manualregister" size="50" value="<?=htmlspecialchars($current_manualregister);?>"><br><?=spanify(gettext("Completely override the generated registration string with a manually defined one."));?> 
		</td>
	</tr><?
}

function display_qualify_options($current_qualify, $colspan) {
	
	?><tr> 
		<td valign="top" class="vncell"><?=spanify(gettext("Qualify"));?></td>
		<td colspan="<?=$colspan?>" class="vtable">
			<input name="qualify" type="text" class="formfld" id="qualify" size="5" value="<?=htmlspecialchars($current_qualify);?>">&nbsp;<?=spanify(gettext("seconds"));?> 
            <br><span class="vexpl"><?=spanify(gettext("Packets will be sent to this account every <i>n</i> seconds to check its status.<br>Defaults to '2'. Set to '0' to disable."));?></span>
		</td>
	</tr><?
}

function display_natmode_selector($current_natmode, $colspan) {
	global $defaults;

	$natmodes = array(
		"yes" => gettext("always use NAT mode."),
		"no" => gettext("only use NAT mode according to RFC3581 (rport)"),
		"never" => gettext("never attempt NAT mode or RFC3581 support"),
		"route" => gettext("assume NAT mode but don't send rport ")
	);

	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=spanify(gettext("NAT"));?></td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable">
			<select name="natmode" class="formfld" id="natmode">
			<? foreach ($natmodes as $natmode => $natdesc) : ?>
			<option value="<?=$natmode;?>" <?
			if ($natmode == $current_natmode)
				echo "selected"; ?>
			><?=$natdesc;?></option>
			<? endforeach; ?>
			</select>
			<br><span class="vexpl"><?=spanify(gettext("Is there a NATing device between the pbx and this account? Setting this to 'yes' will work in most cases. The options 'never' and 'route' are only rarely used for certain buggy SIP phones."));?></span>
		</td>
	</tr><?
}

function display_phone_ringlength_selector($current_ringlength, $colspan=1) {
	global $defaults;

	$ringlengths = array(
		5 => "5",
		10 => "10",
		15 => "15",
		20 => "20",
		25 => "25",
		30 => "30",
		35 => "35",
		40 => "40",
		45 => "45",
		50 => "50",
		55 => "55",
		60 => "60",
		"indefinitely" => gettext("indefinitely")
	);

	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=spanify(gettext("Ring Length"));?></td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable">
			<select name="ringlength" class="formfld" id="ringlength">
			<? foreach ($ringlengths as $v => $s) : ?>
			<option value="<?=$v;?>" <?
			if ($v == $current_ringlength)
				echo "selected"; ?>
			><?=$s;?></option>
			<? endforeach; ?>
			</select>
			<br><span class="vexpl"><?=spanify(gettext("The number of seconds this phone will ring before giving up or going to voicemail."));?></span>
		</td>
	</tr><?
}

function display_port_gain_selector($current_rxgain, $current_txgain, $colspan) {
	global $defaults;

	$mingain = -10;
	$maxgain = 10;
	$stepping = 0.1;

	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=spanify(gettext("Volume"));?></td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable">
			<select name="rxgain" class="formfld" id="rxgain">
			<? for ($i = $mingain; $i <= $maxgain; $i += $stepping) : ?>
			<? $i = round($i, 1); ?>
			<option value="<?=$i;?>" <?
			if ($i == $current_rxgain)
				echo "selected"; ?>
			><?=sprintf("%0.1f\n", $i);?></option>
			<? endfor; ?>
			</select>&nbsp;<?=spanify(gettext("receive"));?>
			&nbsp;&nbsp;
			<select name="txgain" class="formfld" id="txgain">
			<? for ($i = $mingain; $i <= $maxgain; $i += $stepping) : ?>
			<? $i = round($i, 1); ?>
			<option value="<?=$i;?>" <?
			if ($i == $current_txgain)
				echo "selected"; ?>
			><?=sprintf("%0.1f\n", $i);?></option>
			<? endfor; ?>
			</select>&nbsp;<?=spanify(gettext("transmit"));?>
			<br><span class="vexpl"><?=spanify(gettext("If calls coming through this port are too loud or too quiet, make adjustments here. Receive and transmit levels (in decibels) can be adjusted independently."));?></span>
		</td>
	</tr><?
}

function display_incoming_callerid_override_options($current_override, $current_overridestring, $colspan) {
	
	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=spanify(gettext("Incoming Caller ID Overrides"));?></td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable">
			<input name="overridestring" type="text" class="formfld" id="overridestring" size="40" value="<?=htmlspecialchars($current_overridestring);?>">&nbsp;<i><?=spanify(gettext("Source Name &lt;number&gt;"));?></i><br>
			<input name="override" type="radio" value="prepend" <?php if ($current_override == "prepend") echo "checked"; ?>> <?=spanify(gettext("Prepend with string"));?>&nbsp;<input name="override" type="radio" value="replace" <?php if ($current_override == "replace") echo "checked"; ?>> <?=spanify(gettext("Replace with string"));?><br>
			<input name="override" type="radio" value="replacenamewithnum" <?php if ($current_override == "replacenamewithnum") echo "checked"; ?>> <?=spanify(gettext("Replace calling name with calling number."));?><br>
			<input name="override" type="radio" value="disable" <?php if (!$current_override) echo "checked"; ?>> <?=spanify(gettext("Disable overrides"));?>
		</td>
	</tr><?
}

function display_outgoing_callerid_options($current_callerid_source, $current_callerid_string, $colspan) {
	
	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=spanify(gettext("Outgoing Caller ID"));?></td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable">
			<input name="calleridstring" type="text" class="formfld" id="calleridstring" size="40" value="<?=htmlspecialchars($current_callerid_string);?>">&nbsp;<i><?=spanify(gettext("Your Name &lt;number&gt;"));?></i><br>
			<input name="calleridsource" type="radio" value="string" <?php if ($current_callerid_source == "string") echo "checked"; ?>> <?=spanify(gettext("Send string defined above&nbsp;"));?><input name="calleridsource" type="radio" value="phones" <?php if ($current_callerid_source == "phones" || !$current_callerid_source) echo "checked"; ?>> <?=spanify(gettext("Send phones' Caller ID"));?>
		</td>
	</tr><?
}

function display_provider_dialpattern_editor($current_patterns, $colspan, $idprefix=false) {
	
	$n = count($current_patterns);
	if ($n == 0) {
		$n = 1 ;	
	}
	
	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=spanify(gettext("Dialing Pattern(s)"));?></td>
		<td width="80%" colspan="<?=$colspan;?>" class="vtable"><?

		// multiple instances need a prefixing identifier
		if ($idprefix) {
			$idstring = $idprefix . "_dialpattern";
		} else {
			$idstring = "dialpattern";
		}

			?><textarea name="<?=$idstring;?>" cols="40" rows="<?=$n;?>" id="<?=$idstring;?>" class="notes"><?

			// print the pattern(s)
			if (is_array($current_patterns)) {
				echo htmlspecialchars($current_patterns[0]);
				for ($i = 1; $i < $n; $i++) {
					echo "\n" . htmlspecialchars($current_patterns[$i]);
				}
			}
			?></textarea><?
			
		// multiple instances don't need help for every instance
		if (!$idprefix) {				
			?><br><span class="vexpl"><?=spanify(gettext("Enter patterns, one per line, to define this provider's routing. Enter \"X!\" to use this Provider for all outgoing calls. To only use this Provider for calls starting with the number \"9\" enter \"9|.\". If no patterns are entered, only incoming calls will be accepted.<br>\n <ul>\n \t<li><strong>+</strong> - adds a prefix (i.e. \"1+555\" matches \"555\" and passes \"1555\" to the provider)</li>\n \t<li><strong>|</strong> - removes a prefix (i.e. \"1|555\" matches \"1555\" but only passes \"555\" to the provider)</li>\n \t<li><strong>X</strong> - matches digits 0-9</li>\n \t<li><strong>Z</strong> - matches digits 1-9</li>\n \t<li><strong>N</strong> - matches digits 2-9</li>\n \t<li><strong>[13-5]</strong> - matches any digit in the brackets (here, 1,3,4,5)</li>\n \t<li><strong>.</strong> - matches one or more characters (not allowed before | or +)</li>\n \t<li><strong>!</strong> - matches zero or more characters (not allowed before | or +)</li>\n </ul>\n"));?></span><?
		}
		
		?></td>
	</tr><?
}

function display_channel_language_selector($current_language, $colspan=1) {

	$prompt_languages = array(
		"en-us" => gettext("English (US)"),
		"en-gb" => gettext("English (UK)"),
		//"da" => gettext("Danish"),
		//"nl" => gettext("Dutch"),
		//"fr" => gettext("French (France)"),
		"fr-ca" => gettext("French (Canada)"),
		"de-de" => gettext("German"),
		"it-it" => gettext("Italian"),
		//"jp" => gettext("Japanese"),
		//"pt-br" => gettext("Portuguese (Brazil)"),
		"ru" => gettext("Russian"),
		"es-es" => gettext("Spanish (Spain)")
		//"se" => gettext("Swedish")
	);

	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=spanify(gettext("Language"));?></td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable">
			<select name="language" class="formfld" id="language">
			<? foreach($prompt_languages as $prompt_language=>$friendly) : ?>
			<option value="<?=$prompt_language;?>" <?
			if ($prompt_language == $current_language)
				echo "selected"; ?>
			><?=$friendly;?></option>
			<? endforeach; ?>
			</select>
			<br><span class="vexpl"><?=spanify(gettext("Audio prompts will be played back in the selected language for this account."));?></span>
		</td>
	</tr><?
}

function display_public_access_editor($current_publicaccess, $current_publicname, $colspan=1) {
	global $scripts;

	$scripts['javascript_public_access_editor'] = true;

	?><tr> 
		<td valign="top" class="vncell"><?=spanify(gettext("Public Access"));?></td>
		<td colspan="<?=$colspan?>" class="vtable">
			<input name="publicaccess" id="publicaccess" type="checkbox" value="enabled" <?
			if ($current_publicaccess == "enabled") {
				echo "checked";
			}
			?>>&nbsp;<?=spanify(gettext("allow this number to be reachable over the Internet"));?>
			<span name="publicaccess_wrapper" id="publicaccess_wrapper">
				<br><?=gettext("alias");?>:&nbsp;<input name="publicname" type="text" class="formfld" id="publicname" size="20" value="<?=htmlspecialchars($current_publicname);?>">&nbsp(<?=spanify(gettext("optional"));?>)
				<br><br><span class="vexpl"><?=spanify(gettext("Set a friendlier alias above if you would like to use a word or name instead of the extension number."));?></span>
			</span>
		</td>
	</tr><?

}

function display_application_logic_editor($current_appcommands, $colspan, $idprefix=false) {

	$n = count($current_appcommands);
	if ($n < 20) {
		$n = 20;	
	}

	?><tr> 
		<td width="20%" valign="top" class="vncellreq"><?=gettext("Logic");?></td>
		<td width="80%" colspan="<?=$colspan;?>" class="vtable"><?

		// multiple instances need a prefixing identifier
		if ($idprefix) {
			$idstring = $idprefix . "_applogic";
		} else {
			$idstring = "applogic";
		}

		?><textarea name="<?=$idstring;?>" cols="80" rows="<?=$n;?>" id="<?=$idstring;?>" class="code"><?

			// print the pattern(s)
			if (is_array($current_appcommands)) {
				echo htmlspecialchars(base64_decode($current_appcommands[0]));
				for ($i = 1; $i < $n; $i++) {
					echo "\n" . htmlspecialchars(base64_decode($current_appcommands[$i]));
				}
			}
			?></textarea><?

		// multiple instances don't need help for every instance
		if (!$idprefix) {				
			?><br><span class="vexpl"><?=spanify(gettext("Enter application logic, one command per line as shown: <br> <pre> 1,Answer()\n 2,Echo()\n 3,Hangup()\n </pre> <p>Programming applications for the PBX requires knowledge of the Asterisk dialplan. An effort is made to verify this input but the possibility remains that an install may be rendered useless by faulty application logic. A reference of applications, functions and phones is available in the <strong><a href=\"popup_dialplan_element_library.php\" target=\"_blank\">Element Library</a></strong>.</p>")); ?></span><?
		}

		?></td>
	</tr><?
}

function display_advanced_settings_begin($colspan) {

		?><tr>
			<td class="list" colspan="<?=$colspan + 1;?>" height="12">&nbsp;</td>
		</tr>
		<tr>
			<td colspan="<?=$colspan + 1;?>" class="listtopiclight" id="advanced-settings-header"><?=spanify(gettext("Advanced Settings"));?> (<a href="javascript:{}" id="advanced-settings-opener" class="link_color_black"><?=spanify(gettext("show"));?></a>)</td>
		</tr>
	</table>
	<div id="advanced-settings-container" style="display: none;">
	<table width="100%" border="0" cellpadding="6" cellspacing="0"><?
}

function display_advanced_settings_end() {

	?></table>
	</div>
	<table width="100%" border="0" cellpadding="6" cellspacing="0">
		<tr>
			<td class="list" colspan="2" height="12">&nbsp;</td>
		</tr><?
}

function display_manual_attributes_editor($current_manual, $colspan, $idprefix=false) {

	$n = count($current_manual);
	if ($n < 5) {
		$n = 5;
	}

	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=spanify(gettext("Manual Attributes"));?></td>
		<td width="80%" colspan="<?=$colspan;?>" class="vtable"><?

		// multiple instances need a prefixing identifier
		if ($idprefix) {
			$idstring = $idprefix . "_manualattributes";
		} else {
			$idstring = "manualattributes";
		}

			?><textarea name="<?=$idstring;?>" cols="40" rows="<?=$n;?>" id="<?=$idstring;?>" class="notes"><?

			// print the attributes
			if (is_array($current_manual)) {
				echo htmlspecialchars(base64_decode($current_manual[0]));
				for ($i = 1; $i < $n; $i++) {
					echo "\n" . htmlspecialchars(base64_decode($current_manual[$i]));
				}
			}
			?></textarea><?

		// multiple instances don't need help for every instance
		if (!$idprefix) {
			?><br><span class="vexpl"><?=spanify(gettext("Manual key-value pairs can be entered in addition to the generated configuration (i.e. <em>configoption=value</em>). These settings will be appended to this item's Asterisk configuration file or context. Unless you have a very specific need to do this, nothing should be entered in this field. Improperly formed values can break system functionality and/or open security holes.")); ?></span><?
		}

		?></td>
	</tr><?
}

function display_capacity_bar($sizeinbytes, $usage) {

	?><img src='bar_left.gif' height='15' width='4' border='0' align='absmiddle'><img src='bar_blue.gif' height='15' width='<?=$usage;?>' border='0' align='absmiddle'><img src='bar_gray.gif' height='15' width='<?=100-$usage;?>' border='0' align='absmiddle'><img src='bar_right.gif' height='15' width='5' border='0' align='absmiddle'>
	<br><?=$usage;?>% <?=spanify(gettext("of"));?> <?=format_bytes($sizeinbytes);?><?
}

function display_peer_status_icon($status, $id) { 
	switch ($status) {
		case "OK":
		return "<img id=\"" . $id . "\" src=\"bullet_green.png\" title=\"" . gettext("connected") . "\">";
		case "LAGGED":
		return "<img id=\"" . $id . "\" src=\"bullet_yellow.png\" title=\"" . gettext("connected but lagged") . "\">";
		case "UNREACHABLE":
		return "<img id=\"" . $id . "\" src=\"bullet_red.png\" title=\"" . gettext("unreachable") . "\">";
		case "UNKNOWN":
		return "<img id=\"" . $id . "\" src=\"bullet_white.png\" title=\"" . gettext("status unknown") . "\">";
		default:
		return "";
	}
}

function display_firmware_update_info($status) {

	if ($status == "current release") {

		$image = "check.gif";
		$message = gettext("You are using the most recent version of AskoziaPBX.");

	} else if ($status == "current beta") {

		$image = "check.gif";
		$message = gettext("You are using the most recent <strong>beta</strong> of AskoziaPBX.");

	} else if ($status == "release update available") { 
	
		$image = "exclam.gif";
		$message = gettext("A newer version of AskoziaPBX is available!") .
					"<br><a href=\"http://askozia.com/pbx\" target=\"_blank\">" .
					gettext("Update") . "</a>";

	} else if ($status == "beta update available") { 

		$image = "exclam.gif";
		$message = gettext("A newer <strong>beta</strong> version of AskoziaPBX is available!") .
					"<br><a href=\"http://askozia.com/pbx\" target=\"_blank\">" .
					gettext("Update") . "</a>";

	}

	if ($image && $message) {

		?><p>
			<table border="0" cellspacing="0" cellpadding="4" width="100%">
				<tr>
					<td class="firmware_update_info_image" align="center" valign="top" width="36">
					<img src="<?=$image;?>" width="28" height="32"></td>
					<td class="firmware_update_info_message"><?=spanify($message);?></td>
				</tr>
			</table>
		</p><?

	}
}

function display_input_errors($input_errors) {

	?><p>
		<table border="0" cellspacing="0" cellpadding="4" width="100%">
			<tr>
				<td class="print_input_errors_img" width="36"><img src="err.gif" width="28" height="32"></td>
				<td class="print_input_errors">

				<span class="errmsg"><p>
					<?=spanify(gettext("The following input errors were detected:"));?>
					<ul><?

					foreach ($input_errors as $ierr) {
							echo "<li>" . spanify(htmlspecialchars($ierr)) . "</li>\n";
					}

					?></ul>
				</span>
	
				</td>
			</tr>
		</table>
	</p><?

}

function display_info_box($msg, $keep=false) {

	if (!$keep) {
		?><script type="text/javascript" charset="utf-8">
			jQuery(document).ready(function() {
    	    	jQuery('#info-box').animate({opacity: 1.0}, 7000).slideUp('slow');
    	    });
		</script><?
	}

	?><span id="info-box">
		<p>
			<table border="0" cellspacing="0" cellpadding="4" width="100%">
				<tr>
					<td class="print_info_box_img" width="36"><img src="exclam.gif" width="28" height="32"></td>
					<td class="print_info_box_msg"><?=spanify($msg);?></td>
				</tr>
			</table>
		</p>
	</span><?

}

function display_ports_tab_menu() {

	?><tr>
		<td class="tabnavtbl">
			<ul id="tabnav"><?

			$tabs = array(
				gettext('Analog')	=> 'ports_analog.php',
				gettext('ISDN')		=> 'ports_isdn.php'
			);
			dynamic_tab_menu($tabs);

			?></ul>
		</td>
	</tr><?

}

function display_networking_tab_menu() {

	?><tr>
		<td class="tabnavtbl">
			<ul id="tabnav"><?

			$tabs = array(
				gettext('Local Area Network')	=> 'network_lan.php',
				gettext('Phones')				=> 'network_phones.php'
			);
			dynamic_tab_menu($tabs);

			?></ul>
		</td>
	</tr><?

}

function display_notifications_tab_menu() {

	?><tr>
		<td class="tabnavtbl">
			<ul id="tabnav"><?

			$tabs = array(
				gettext('E-Mail')	=> 'notifications_email.php'
			);
			dynamic_tab_menu($tabs);

			?></ul>
		</td>
	</tr><?

}

function display_description_field($current_description, $colspan) {

	?><tr> 
		<td valign="top" class="vncell"><?=spanify(gettext("Description"));?></td>
		<td colspan="<?=$colspan;?>" class="vtable">
			<input name="descr" type="text" class="formfld" id="descr" size="40" value="<?=htmlspecialchars($current_description);?>"> 
			<br><span class="vexpl"><?=spanify(gettext("You may enter a description here for your reference (not parsed)."));?></span>
		</td>
	</tr><?
}

function display_readback_number_field($current_readbacknumber, $colspan) {

	?><tr> 
		<td valign="top" class="vncell"><?=spanify(gettext("Public Number"));?></td>
		<td colspan="<?=$colspan;?>" class="vtable">
			<input name="readbacknumber" type="text" class="formfld" id="readbacknumber" size="40" value="<?=htmlspecialchars($current_readbacknumber);?>"> 
			<br><span class="vexpl"><?=spanify(gettext("This 'external' number will be read back to the caller when reaching voicemail; defaults to account's username if it is numeric. If it is not, the internal extension this call was routed to will be read back."));?></span>
		</td>
	</tr><?
}





function d_start($formname=false) {

	if ($formname) {
?>
<form action="<?=$formname;?>" method="post" name="iform" id="iform">
<?
	}
?>
<table width="100%" border="0" cellpadding="6" cellspacing="0">
<?
}

function d_header($title, $light=false) {
	global $colspan;

	$class = ($light) ? "listtopiclight" : "listtopic";
?>
	<tr>
		<td colspan="<?=($colspan+1);?>" valign="top" class="<?=$class;?>"><?=spanify($title);?></td>
	</tr>
<?
}

function d_field($title, $name, $width, $help=false, $required=false) {
	global $form, $colspan;

	$class = ($required) ? "vncellreq" : "vncell";
?>
	<tr> 
		<td width="20%" valign="top" class="<?=$class;?>"><?=spanify($title);?></td>
		<td width="80%" colspan="<?=$colspan;?>" class="vtable">
			<input name="<?=$name;?>" type="text" class="formfld" id="<?=$name;?>" size="<?=$width;?>" value="<?=htmlspecialchars($form[$name]);?>"> 
			<br><span class="vexpl"><?=spanify($help);?></span>
		</td>
	</tr>
<?
}

function d_spacer() {
	global $colspan;
?>
	<tr>
		<td colspan="<?=($colspan+1);?>" class="list" height="12"></td>
	</tr>
<?
}

function d_submit() {
	global $form, $carryovers, $colspan;
?>
	<tr> 
		<td valign="top">&nbsp;</td>
		<td colspan="<?=$colspan;?>">
<?
		if (is_array($carryovers)) {
			foreach ($carryovers as $co) {
?>
			<input name="<?=$co;?>" id="<?=$co;?>" type="hidden" value="<?=$form[$co];?>">
<?
			}
		}
?>
			<input name="Submit" id="submit" type="submit" class="formbtn" value="<?=gettext("Save");?>">
		</td>
	</tr>
</table>
</form>
<?
d_scripts();
}

function d_scripts() {
	global $scripts;

	$scripts = array_keys($scripts);
?>
<script type="text/JavaScript">
<!--
<?
	foreach ($scripts as $script) {
		$script("functions");
	}
?>
	jQuery(document).ready(function(){
<?
		foreach ($scripts as $script) {
			$script("ready");
		}
?>
	});
//-->
</script>
<?
}

function d_hwport_selector($techfilter=null, $typefilter=null) {
	global $colspan;

	$ports = dahdi_get_ports($techfilter, $typefilter);

	if ($techfilter == "analog" && $typefilter == "fxs") {
		$help = gettext("The hardware port this phone is connected to.");
	}
?>
	<tr>
		<td valign="top" class="vncell"><?=spanify(gettext("Hardware Port"));?></td>
		<td colspan="<?=$colspan;?>" class="vtable">
			<select name="port" class="formfld" id="port"><?

			
			foreach ($ports as $port) {
				?><option value="<?=$port['uniqid'];?>" <?
				if ($port['uniqid'] == $form['port']) {
					echo "selected";
				}
				?>><?=$port['name'];?></option><?
			}
			
			?></select>
			<br><span class="vexpl"><?=spanify($help);?></span>
		</td>
	</tr>
<?
}

function d_notifications_editor($current_emailcallnotify, $current_emailcallnotifyaddress) {
	global $scripts, $colspan;

	$scripts['javascript_notifications_editor'] = true;
?>
	<tr>
		<td valign="top" class="vncell"><?=spanify(gettext("Notifications"));?></td>
		<td colspan="<?=$colspan;?>" class="vtable">
			<input name="emailcallnotify" id="emailcallnotify" type="checkbox" value="enabled" <?
			if ($current_emailcallnotify == "enabled") {
				echo "checked";
			}?>>&nbsp;<?=spanify(gettext("notify me of missed calls via e-mail"));?>
			<span name="emailnotify_wrapper" id="emailnotify_wrapper">
			<br><?=spanify(gettext("address"));?>:&nbsp;<input name="emailcallnotifyaddress" id="emailcallnotifyaddress" type="text" class="formfld" size="40" value="<?=htmlspecialchars($current_emailcallnotifyaddress);?>">
			<br>
			<br><span class="vexpl"><?=spanify(gettext("Note:")) . " " .
				spanify(gettext("<a href=\"notifications_email.php\">E-Mail Notifications</a> must be configured correctly before this will work."));?></span>
			</span>
		</td>
	</tr>
<?
}

function d_provider_access_selector($current_providers) {
	global $colspan;

	$providers = pbx_get_providers();
	if (!count($providers)) {
		return;
	}

	?><tr>
		<td width="20%" valign="top" class="vncell"><?=spanify(gettext("Block Providers"));?></td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable"><?

		foreach ($providers as $provider) {
			?><input name="<?=$provider['uniqid']?>" id="<?=$provider['uniqid']?>" type="checkbox" value="yes" <? 
			if (in_array($provider['uniqid'], $current_providers)) echo "checked"; 
			?>><?=$provider['name']?><br><?
		}

		?><br>
		<span class="vexpl"><?=spanify(gettext("Block access to the providers selected above."));?></span>
		</td>
	</tr><?
}

function d_voicemail_editor($current_vmtoemail, $current_vmtoemailaddress) {
	global $scripts, $colspan;

	$scripts['javascript_voicemail_editor'] = true;

	?><tr>
		<td valign="top" class="vncell"><?=spanify(gettext("Voicemail"));?></td>
		<td colspan="<?=$colspan;?>" class="vtable">
			<input name="vmtoemail" id="vmtoemail" type="checkbox" value="enabled" <?
			if ($current_vmtoemail == "enabled") {
				echo "checked";
			}
			?>>&nbsp;<?=spanify(gettext("enable voicemail to e-mail"));?>
			<span name="vmtoemail_wrapper" id="vmtoemail_wrapper">
				<br><?=spanify(gettext("address"));?>:&nbsp;<input name="vmtoemailaddress" id="vmtoemailaddress" type="text" class="formfld" size="40" value="<?=htmlspecialchars($current_vmtoemailaddress);?>">
				<br>
				<br><span class="vexpl"><?=spanify(gettext("Note:")) . " " .
					spanify(gettext("<a href=\"notifications_email.php\">E-Mail Notifications</a> must be configured correctly before this will work."));?></span>
			</span>
		</td>
	</tr><?
}

?>
