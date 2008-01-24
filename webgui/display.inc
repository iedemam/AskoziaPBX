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

function display_audio_codec_selector($enabled_codecs) {
	global $audio_codecs;
	
	?><tr> 
		<td width="20%" valign="top" class="vncell">Audio Codecs</td>
		<td width="40%" class="vtable" valign="top"><strong>Enabled</strong>&nbsp;<i>(drag-and-drop)</i>
			<ul id="ace" class="ace" style="min-height:50px">
			<? foreach ($enabled_codecs as $codec): ?>
				<? if (array_key_exists($codec, $audio_codecs)): ?>
				<li class="ace" id="ace_<?=$codec;?>"><?=$audio_codecs[$codec];?></li>
				<? endif; ?>
			<? endforeach; ?>
			</ul>
		</td>
		<td width="40%" class="vtable" valign="top"><strong>Disabled</strong>
			<ul id="acd" class="acd" style="min-height:50px">
			<? foreach ($audio_codecs as $codec=>$friendly): ?>
				<? if (!in_array($codec, $enabled_codecs)): ?>
				<li class="acd" id="acd_<?=$codec;?>"><?=$friendly;?></li>
				<? endif; ?>
			<? endforeach; ?>
			</ul>
		</td>
	</tr><?	
}

function display_video_codec_selector($enabled_codecs) {
	global $video_codecs;
	
	?><tr> 
		<td width="20%" valign="top" class="vncell">Video Codecs</td>
		<td width="40%" class="vtable" valign="top"><strong>Enabled</strong>&nbsp;<i>(drag-and-drop)</i>
			<ul id="vce" class="vce" style="min-height:50px">
				<? foreach ($enabled_codecs as $codec): ?>
					<? if (array_key_exists($codec, $video_codecs)): ?>
					<li class="vce" id="vce_<?=$codec;?>"><?=$video_codecs[$codec];?></li>
					<? endif; ?>
				<? endforeach; ?>
			</ul>
		</td>
		<td width="40%" class="vtable" valign="top"><strong>Disabled</strong>
			<ul id="vcd" class="vcd" style="min-height:50px">
			<? foreach ($video_codecs as $codec=>$friendly): ?>
				<? if (!in_array($codec, $enabled_codecs)): ?>
				<li class="vcd" id="vcd_<?=$codec;?>"><?=$friendly;?></li>
				<? endif; ?>
			<? endforeach; ?>
			</ul>
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
		<td width="20%" valign="top" class="vncell">Group Members</td>
		<td width="40%" class="vtable" valign="top"><strong>Members</strong>&nbsp;<i>(drag-and-drop)</i>
			<ul id="gme" class="gme" style="min-height:50px"><? 
			
			foreach ($current_members as $member) {
				if (array_key_exists($member, $indexed_phones)) {
					?><li class="gme" id="gme_<?=$member;?>"><?=pbx_uniqid_to_name($member);?>&nbsp;&lt;<?=$indexed_phones[$member]['extension'];?>&gt;</li><?
				}
			}
			
			?></ul>
		</td>
		<td width="40%" class="vtable" valign="top"><strong>Non-Members</strong>
			<ul id="gmd" class="gmd" style="min-height:50px"><?
			

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
			<span class="vexpl"><strong><span class="red">Note:</span></strong>
			All group members will be called at the same time. <strong>The voicemail account associated with the first member will be used if the call is not answered.</strong></span>
		</td>
	</tr><?	
}

function display_incoming_extension_selector($colspan, $idprefix=false) {
	global $help, $help_language;
	
	$table_id = $idprefix ? $idprefix . "_incoming_extension_selector" : "incoming_extension_selector";
	$row_id = $idprefix ? $idprefix . "_incoming_extension_selector_last" : "incoming_extension_selector_last";
	$add_params = $idprefix ? ",,\"$idprefix\"" : "";
	
	?><tr>
		<td width="20%" valign="top" class="vncell">Incoming Extension(s)</td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable">
			<table id="<?=$table_id;?>">
				<tr id="<?=$row_id;?>">
					<td>&nbsp;</td>
					<td><img src="plus.gif" title="add incoming extension mapping" width="17" height="17" border="0" style="cursor: pointer; cursor: hand;" onclick="add_incoming_extension_selector(<?=$add_params;?>)"></td>
				</tr>
			</table><br><?
			
			// multiple instances don't need help for every instance
			if (!$idprefix) {				
				?><span class="vexpl"><?=$help[$help_language]["display_incoming_extension_selector"]; ?></span><?
			}			

		?></td>
	</tr><?

}

function display_dtmfmode_selector($current_dtmfmode, $colspan) {
	global $dtmfmodes;
	
	?><tr> 
		<td width="20%" valign="top" class="vncell">DTMF Mode</td>
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

function display_registration_options($current_noregister, $colspan) {
	
	?><tr> 
		<td valign="top" class="vncell">Registration</td>
		<td colspan="<?=$colspan?>" class="vtable">
			<input name="noregister" id="noregister" type="checkbox" value="yes" <? if ($current_noregister) echo "checked"; ?>>Do not register with this provider.
		</td>
	</tr><?
}

function display_qualify_options($current_qualify, $colspan) {
	
	?><tr> 
		<td valign="top" class="vncell">Qualify</td>
		<td colspan="<?=$colspan?>" class="vtable">
			<input name="qualify" type="text" class="formfld" id="qualify" size="5" value="<?=htmlspecialchars($current_qualify);?>">&nbsp;seconds 
            <br><span class="vexpl">Packets will be sent to this provider every <i>n</i> seconds to check its status.
			<br>Defaults to '2'. Set to '0' to disable.</span>
		</td>
	</tr><?
}

function display_incoming_callerid_override_options($current_override, $colspan) {
	
	?><tr> 
		<td width="20%" valign="top" class="vncell">Incoming Caller ID Overrides</td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable">
			<input name="override" type="checkbox" value="replacenamewithnum" <?php if ($current_override == "replacenamewithnum") echo "checked"; ?>> Replace calling name with calling number.
		</td>
	</tr><?
}

function display_outgoing_callerid_options($current_callerid_source, $current_callerid_string, $colspan) {
	
	?><tr> 
		<td width="20%" valign="top" class="vncell">Outgoing Caller ID</td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable">
			<input name="calleridstring" type="text" class="formfld" id="calleridstring" size="40" value="<?=htmlspecialchars($current_callerid_string);?>">&nbsp;<i>"Your Name" &lt;number&gt;</i><br>
			<input name="calleridsource" type="radio" value="string" <?php if ($current_callerid_source == "string") echo "checked"; ?>> Send string defined above&nbsp;<input name="calleridsource" type="radio" value="phones" <?php if ($current_callerid_source == "phones") echo "checked"; ?>> Send phones' Caller ID
		</td>
	</tr><?
}

function display_provider_access_selector($current_providers, $current_outbounduridial, $colspan) {
	
	$a_providers = pbx_get_providers();

	?><tr> 
		<td width="20%" valign="top" class="vncell">Providers</td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable"><?
		
		$n = count($a_providers);
		if ($n == 0) {
			?><span class="vexpl"><i>There are currently no providers defined.</i></span><br><?

		} else {
			for($i = 0; $i < $n; $i++) {
				$provider = $a_providers[$i];
			
				?><input name="<?=$provider['uniqid']?>" id="<?=$provider['uniqid']?>" type="checkbox" value="yes" <? 
				if (in_array($provider['uniqid'], $current_providers)) echo "checked"; 
				?>><?=$provider['name']?><br><?
			}
		}

		?><input name="outbounduridial" id="outbounduridial" type="checkbox" value="yes" <? 
		if ($current_outbounduridial) echo "checked"; 
		?>>direct outbound dialing via SIP uris<br><?

		?><br>
		<span class="vexpl">Checked providers will be accessible from this phone.</span>
		</td>
	</tr><?
}

function display_phone_access_selector($provider_uniqid, $colspan, $idprefix=false) {
	global $help, $help_language;
	
	$a_phones = pbx_get_phones();

	?><tr> 
		<td width="20%" valign="top" class="vncell">Phones</td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable"><?
		
		$n = count($a_phones);
		if ($n == 0) {
			?><span class="vexpl"><i>There are currently no phones defined.</i></span><br><?

		} else {
			for($i = 0; $i < $n; $i++) {
				$phone = $a_phones[$i];
				
				if($idprefix) {
					?><input name="<?=$idprefix."_".$phone['uniqid'];?>" id="<?=$idprefix."_".$phone['uniqid'];?>" type="checkbox" value="<?=$idprefix;?>"<?
				
				} else {
					?><input name="<?=$phone['uniqid'];?>" id="<?=$phone['uniqid'];?>" type="checkbox" value="yes"<?
				}
				
				if (in_array($provider_uniqid, $phone['provider']))
					echo "checked"; 

				?>><?=$phone['callerid'];?> &lt;<?=$phone['extension'];?>&gt;<br><?
			}
		}
		
		// multiple instances don't need help for every instance
		if (!$idprefix) {				
			?><br><span class="vexpl"><?=$help[$help_language]["display_phone_access_selector"]; ?></span><?
		}
		
		?></td>
	</tr><?
}

function display_provider_dialpattern_editor($current_patterns, $colspan, $idprefix=false) {
	global $help, $help_language;
	
	$n = count($current_patterns);
	if ($n == 0) {
		$n = 1 ;	
	}
	
	?><tr> 
		<td width="20%" valign="top" class="vncell">Dialing Pattern(s)</td>
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
			?><br><span class="vexpl"><?=$help[$help_language]["display_provider_dialpattern_editor"]; ?></span><?
		}
		
		?></td>
	</tr><?
}

function display_channel_language_selector($current_language, $colspan) {
	global $prompt_languages;

	?><tr> 
		<td width="20%" valign="top" class="vncell">Language</td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable">
			<select name="language" class="formfld" id="language">
			<? foreach($prompt_languages as $prompt_language=>$friendly) : ?>
			<option value="<?=$prompt_language;?>" <?
			if ($prompt_language == $current_language)
				echo "selected"; ?>
			><?=$friendly;?></option>
			<? endforeach; ?>
			</select>
			<br><span class="vexpl">Audio prompts will be played back in the selected language for this account.</span>
		</td>
	</tr><?
}

function display_call_notifications_editor($current_voicemailbox, $current_sendcallnotifications, $colspan) {
	
	?><tr> 
		<td valign="top" class="vncell">Voicemail</td>
		<td colspan="<?=$colspan?>" class="vtable">
			<input name="voicemailbox" type="text" class="formfld" id="voicemailbox" size="40" value="<?=htmlspecialchars($current_voicemailbox);?>">
			<br><input name="sendcallnotifications" id="sendcallnotifications" type="checkbox" value="yes"<?
			
			if ($current_sendcallnotifications) {
				echo "checked";
			}

			?>>&nbsp;also send missed call notifications<br>
			<br><span class="vexpl">An e-mail address. If entered, voicemail will be enabled on this extension. Incoming messages will be sent to the given address.</span>
		</td>
	</tr><?
}

function display_public_direct_dial_editor($current_allowdirectdial, $current_publicname, $colspan, $getjs=false) {
	global $config;

	$fullhostname = $config['system']['hostname'] . "." . $config['system']['domain'];

	?><tr> 
		<td valign="top" class="vncell">Public Direct Dial</td>
		<td colspan="<?=$colspan?>" class="vtable">
			<input name="allowdirectdial" id="allowdirectdial" type="checkbox" value="yes" <?
			
			if ($current_allowdirectdial) {
				echo "checked";
			}

			?>>&nbsp;allow extension to be directly dialed over public networks
			<br><span name="publicname_wrapper" id="publicname_wrapper">
			<br><input name="publicname" type="text" class="formfld" id="publicname" size="20" value="<?=htmlspecialchars($current_publicname);?>">@<?=$fullhostname;?>
			<br><span class="vexpl">Public name. Enter a string to map a friendly name to this extension.</span>
			<br>
			</span>
			<br><span class="vexpl">Enabling this option will allow this extension to be reachable via SIP and IAX2 uris (extension@<?=$fullhostname;?>)</span>
		</td>
	</tr><?
}

function display_call_and_busy_limit_selector($current_calllimit, $current_busylimit, $colspan) {
	
	?><tr> 
		<td valign="top" class="vncell">Call Limit</td>
		<td colspan="<?=$colspan?>" class="vtable">
			<input name="calllimit" type="text" class="formfld" id="calllimit" size="5" value="<?=htmlspecialchars($current_calllimit);?>">&nbsp;calls 
            <br><span class="vexpl">Permit only 'n' outgoing call(s) and 'n' incoming call(s) (default: 2)</span>
		</td>
	</tr>
	<tr> 
		<td valign="top" class="vncell">Busy Limit</td>
		<td colspan="<?=$colspan?>" class="vtable">
			<input name="busylimit" type="text" class="formfld" id="busylimit" size="5" value="<?=htmlspecialchars($current_busylimit);?>">&nbsp;calls 
            <br><span class="vexpl">Phone status will be switched to 'busy' with 'n' concurrent calls (default: 1)</span>
		</td>
	</tr><?
}

function display_application_logic_editor($current_appcommands, $colspan, $idprefix=false) {
	global $help, $help_language;

	$n = count($current_appcommands);
	if ($n < 20) {
		$n = 20;	
	}

	?><tr> 
		<td width="20%" valign="top" class="vncellreq">Logic</td>
		<td width="80%" colspan="<?=$colspan;?>" class="vtable"><?

		// multiple instances need a prefixing identifier
		if ($idprefix) {
			$idstring = $idprefix . "_applogic";
		} else {
			$idstring = "applogic";
		}

			?><textarea name="<?=$idstring;?>" cols="65" rows="<?=$n;?>" id="<?=$idstring;?>" class="notes"><?

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
			?><br><span class="vexpl"><?=$help[$help_language]["display_application_logic_editor"]; ?></span><?
		}

		?></td>
	</tr><?
}

?>
