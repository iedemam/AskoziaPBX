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
		<br><?=gettext("Subject");?>
		<br><input type="text" name="user_subject" id="user_subject" size="40" value="<?=$subj?>">
		<br><br><?=gettext("Body");?>
		<br><textarea name="user_body" cols="60" rows="3" id="user_body" class="userdefined"><?=$body?></textarea><br>
		<br><strong><?=gettext("The following variables can be used in the e-mail notification");?>:</strong>
		<br>VM_MAILBOX = <?=gettext("extension number of voicemail box");?>
		<br>VM_NAME = <?=gettext("name of voicemail recipient");?>
		<br>VM_DURATION = <?=gettext("duration of the message (i.e. 0:58)");?>
		<br>VM_CALLERID = <?=gettext("caller id of person leaving the message");?>
		<br>VM_DATE = <?=gettext("full date / time (i.e. 2025-11-20 (15:45))");?>
	</span><?
}

function display_passwd_generation() {

	?><a href="javascript:{}"><img class='verticalalign' src='key_go.png' name='generate' id="generate" title="<?=gettext("generate password");?>"></a>
	<span name="generator_wrapper" id="generator_wrapper" style="display: none;">
	<br><input type="text" name="gen_passwd" id="gen_passwd" value="">
	<a href="javascript:{}"><img class='verticalalign' src='add.png' name='use' id="use" title="<?=gettext("use this password");?>"></a>
	</span><?	
}

//------------- pagination/filter logic -------------

function display_calculate_pages_sqlite($filter=null, $logpath, $nentries) {

	if($filter) {
		 $query = "SELECT * FROM cdr WHERE start LIKE '%$filter%' OR src LIKE '%$filter%' OR dst LIKE '%$filter%' OR channel LIKE'%$filter%' OR dstchannel LIKE '%$filter%' OR lastapp LIKE '%$filter%' OR lastdata LIKE '%$filter%' OR duration LIKE '%$filter%' OR billsec LIKE '%$filter%' OR disposition LIKE '%$filter%'";

		$db = sqlite_open($logpath, 0666, $err);
		$results = sqlite_query($query, $db);
		$rows = sqlite_num_rows($results);
		$pages = ceil($rows/$nentries);
	}
	else {
		$query = "select * from cdr";
		$db = sqlite_open($logpath, 0666, $err);
		$results = sqlite_query($query, $db);
		$rows = sqlite_num_rows($results);
		$pages = ceil($rows/$nentries);
	}

	return $pages;
}

function display_get_query($start, $stop, $nentries, $current_page, $filter=null, $sort=null) {		
	
	if($current_page == 0 || $current_page == 1) {
		$start = 0;
	}
	else {
		$start = ($nentries*($current_page-1));
	}		
	$stop = $nentries;	

	
	if($filter && $sort) {
		$query = "SELECT * FROM cdr WHERE start LIKE '%$filter%' OR src LIKE '%$filter%' OR dst LIKE '%$filter%' OR channel LIKE'%$filter%' OR dstchannel LIKE '%$filter%' OR lastapp LIKE '%$filter%' OR lastdata LIKE '%$filter%' OR duration LIKE '%$filter%' OR billsec LIKE '%$filter%' OR disposition LIKE '%$filter%' ORDER BY id DESC LIMIT $start,$stop";
	}
	else if($filter) {
		$query = "SELECT * FROM cdr WHERE start LIKE '%$filter%' OR src LIKE '%$filter%' OR dst LIKE '%$filter%' OR channel LIKE'%$filter%' OR dstchannel LIKE '%$filter%' OR lastapp LIKE '%$filter%' OR lastdata LIKE '%$filter%' OR duration LIKE '%$filter%' OR billsec LIKE '%$filter%' OR disposition LIKE '%$filter%' ORDER BY id ASC LIMIT $start,$stop";
	}
	else if($sort) {
		$query = "SELECT * FROM cdr ORDER BY id DESC LIMIT $start,$stop";
	}
	else {
		$query = "SELECT * FROM cdr ORDER BY id ASC LIMIT $start,$stop";
	}

	return $query;
}

function display_calculate_pages($filter=null, $logpath, $source, $nentries) {

	if($filter) {
		if($source == "internal") {
			$lines = exec("/usr/sbin/clog $logpath | /usr/bin/grep -i $filter | /usr/bin/wc -l");
		}
		else {
			$lines = exec("/usr/bin/grep -i $filter $logpath | /usr/bin/wc -l");
		}
		$pages = ceil($lines/$nentries);
	}
	else {
		$tmp = exec("/usr/bin/wc -l $logpath");
		$lines = preg_split("/\s+/", $tmp, -1, PREG_SPLIT_NO_EMPTY);
		$pages = ceil($lines[0]/$nentries);
	}
	return $pages;
}

function display_calculate_current_page($pages, $sort=null) {
	
	if($_GET['page']) {
		$current_page = $_GET['page'];
	}
	else if($sort) {
		$current_page = 1;
	}
	else {
		$current_page = $pages;
	}

	return $current_page;
}

function display_get_command($current_page, $nentries, $source, $filter=null, $logpath, $sort=null) {

	if($current_page == 0 || $current_page == 1) {
		$start = 1;
	}
	else {
		$start = (($nentries*($current_page-1))+1);
	}

	$stop = (($start+$nentries)-1);

	if($source == "internal") {
		if($filter && $sort) {
			$command = "/usr/sbin/clog $logpath | /usr/bin/grep -i \"$filter\" | /usr/bin/tail -r | /usr/bin/sed '$start,$stop!d'";
		}
		else if($filter) {
			$command = "/usr/sbin/clog $logpath | /usr/bin/grep -i \"$filter\" | /usr/bin/sed '$start,$stop!d'";	
		}
		else if($sort) {
			$command = "/usr/sbin/clog $logpath | /usr/bin/tail -r | /usr/bin/sed '$start,$stop!d'";
		}
		else {
			$command = "/usr/sbin/clog $logpath | /usr/bin/sed '$start,$stop!d'";
		}
	}
	else {
		if($filter && $sort) {
			$command = "/usr/bin/grep -i \"$filter\" $logpath | /usr/bin/tail -r | /usr/bin/sed '$start,$stop!d'";
		}
		else if($filter) {
			$command = "/usr/bin/grep -i \"$filter\" $logpath | /usr/bin/sed '$start,$stop!d'";
		}
		else if($sort) {
			$command = "/usr/bin/tail -r $logpath | /usr/bin/sed '$start,$stop!d'";
		}
		else {
			$command = "/usr/bin/sed '$start,$stop!d' $logpath";
		}
	}
	
	return $command;
}

function display_page_selector($page, $totalitems, $limit, $filtering=null) {
		
	if($filtering) {
		$filtering = "&filter=".$filtering;
	}

	$prev = $page - 1;		//previous page is page - 1
	$next = $page + 1;		//next page is page + 1
	$lpm1 = $totalitems - 1;	//last page minus 1
	$pagestring = "?page=";
	
	$pagination = "";
	//if more than one page
	if($totalitems > 1)
	{	
		$pagination .= "<div class=\"pagination\">";

		//previous button
		if ($page > 1) {
			$pagination .= "<a class=\"leftmost\" href=\"$pagestring$prev$filtering\"> « </a>";
		}
		else {
			$pagination .= "<span class=\"disabled\"> « </span>";	
		}

		//pages	
		if ($totalitems < $limit)	//not enough pages to bother breaking it up
		{	
			for ($i = 1; $i <= $totalitems; $i++)
			{
				if ($i == $page) {
					$pagination .= "<span class=\"current\">$i</span>";
				}
				else {
					$pagination .= "<a href=\"" . $pagestring . $i . $filtering . "\">$i</a>";
				}
			}
		}
		elseif($totalitems >= $limit)	//enough pages to hide some
		{
			//close to beginning; only hide later pages
			if($page < ($limit - 5))		
			{
				for ($i = 1; $i < ($limit - 4); $i++)
				{
					if ($i == $page) {
						$pagination .= "<span class=\"current\">$i</span>";
					}
					else {
						$pagination .= "<a href=\"" . $pagestring . $i . $filtering . "\">$i</a>";
					}
				}
				$pagination .= "...";
				$pagination .= "<a href=\"" . $pagestring . $lpm1 . $filtering . "\">$lpm1</a>";
				$pagination .= "<a href=\"" . $pagestring . $totalitems . $filtering . "\">$totalitems</a>";
			}
			//close to end; only hide early pages
			else if($page > ($totalitems - ($limit - 6))) 
			{
				$pagination .= "<a href=\"" . $pagestring . "1" . $filtering . "\">1</a>";
				$pagination .= "<a href=\"" . $pagestring . "2" . $filtering . "\">2</a>";
				$pagination .= "...";
				for ($i = ($totalitems - ($limit - 6)); $i <= $totalitems; $i++)
				{
					if ($i == $page) {
						$pagination .= "<span class=\"current\">$i</span>";
					}
					else {
						$pagination .= "<a href=\"" . $pagestring . $i . $filtering . "\">$i</a>";
					}
				}
			}
			//in middle; hide some front and some back
			else
			{
				$pagination .= "<a href=\"" . $pagestring . "1" . $filtering . "\">1</a>";
				$pagination .= "<a href=\"" . $pagestring . "2" . $filtering . "\">2</a>";
				$pagination .= "...";
				for ($i = $page - 3; $i <= $page + 3; $i++)
				{
					if ($i == $page) {
						$pagination .= "<span class=\"current\">$i</span>";
					}
					else {
						$pagination .= "<a href=\"" . $pagestring . $i . $filtering . "\">$i</a>";
					}
				}
				$pagination .= "...";
				$pagination .= "<a href=\"" . $pagestring . $lpm1 . $filtering . "\">$lpm1</a>";
				$pagination .= "<a href=\"" . $pagestring . $totalitems . $filtering . "\">$totalitems</a>";	
			}

		}
		
		//next button
		if ($page < $i - 1) {
			$pagination .= "<a href=\"" . $pagestring . $next . $filtering . "\"> » </a>";
		}
		else {
			$pagination .= "<span class=\"disabled\"> » </span>";
		}
		$pagination .= "</div>\n";
	}
	
	return $pagination;

}

//------------------------ pagination/filter end ------------------------------

function display_gui_language_selector($current_language) {

	$languages = array(
		"en_US" => "English",
		"es_ES" => "Castellano (0%)",
		"zh_CN" => "简体中文",
		"zh_TW" => "繁體中文",
		"da_DA" => "Dansk (90%)",
		"de_DE" => "Deutsch (0%)",
		"fr_FR" => "Français (0%)",
		"it_IT" => "Italiano (70%)",
		"pl_PL" => "Język polski (90%)",
		"nl_NL" => "Nederlands (60%)",
		"pt_BR" => "Português (0%)",
		"fi_FI" => "Suomi",
		"se_SE" => "Svenska (0%)",
		"ru_RU" => "Русский язык (0%)"
	);

	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=gettext("webGUI language");?></td>
		<td width="80%" class="vtable">
			<select name="lang" class="formfld" id="lang">
			<? foreach ($languages as $lang => $langdesc) : ?>
			<option value="<?=$lang;?>" <?
			if ($lang == $current_language)
				echo "selected"; ?>
			><?=$langdesc;?></option>
			<? endforeach; ?>
			</select>
			<br><span class="vexpl"><?=gettext("Select in which language you want the webGUI to be displayed."); ?><br><?
			/* don't translate */
			?><em>Please <a href="mailto:michael@askozia.com">contact us</a>
			if you are able to provide a new translation.</em><?
			/* end */
			?></span>
		</td>
	</tr><?
}

function display_audio_codec_selector($enabled_codecs) {
	global $audio_codecs;
	
	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=gettext("Audio Codecs");?></td>
		<td width="40%" class="vtable" valign="top"><strong><?=gettext("Enabled");?></strong>&nbsp;<i>(<?=gettext("drag-and-drop");?>)</i>
			<ul id="ace" class="ace" style="min-height:50px">
			<? foreach ($enabled_codecs as $codec): ?>
				<? if (array_key_exists($codec, $audio_codecs)): ?>
				<li class="ace" id="ace_<?=$codec;?>"><?=$audio_codecs[$codec];?></li>
				<? endif; ?>
			<? endforeach; ?>
			</ul>
		</td>
		<td width="40%" class="vtable" valign="top"><strong><?=gettext("Disabled");?></strong>
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
		<td width="20%" valign="top" class="vncell"><?=gettext("Video Codecs");?></td>
		<td width="40%" class="vtable" valign="top"><strong><?=gettext("Enabled");?></strong>&nbsp;<i>(<?=gettext("drag-and-drop");?>)</i>
			<ul id="vce" class="vce" style="min-height:50px">
				<? foreach ($enabled_codecs as $codec): ?>
					<? if (array_key_exists($codec, $video_codecs)): ?>
					<li class="vce" id="vce_<?=$codec;?>"><?=$video_codecs[$codec];?></li>
					<? endif; ?>
				<? endforeach; ?>
			</ul>
		</td>
		<td width="40%" class="vtable" valign="top"><strong><?=gettext("Disabled");?></strong>
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
		<td width="20%" valign="top" class="vncell"><?=gettext("Group Members");?></td>
		<td width="40%" class="vtable" valign="top"><strong><?=gettext("Members");?></strong>&nbsp;<i>(<?=gettext("drag-and-drop");?>)</i>
			<ul id="gme" class="gme" style="min-height:50px"><? 
			
			foreach ($current_members as $member) {
				if (array_key_exists($member, $indexed_phones)) {
					?><li class="gme" id="gme_<?=$member;?>"><?=pbx_uniqid_to_name($member);?>&nbsp;&lt;<?=$indexed_phones[$member]['extension'];?>&gt;</li><?
				}
			}
			
			?></ul>
		</td>
		<td width="40%" class="vtable" valign="top"><strong><?=gettext("Non-Members");?></strong>
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
			<span class="vexpl"><strong><span class="red"><?=gettext("Note:");?></span></strong>
			<?=gettext("All group members will be called at the same time. <strong>The voicemail account associated with the first member will be used if the call is not answered.");?></strong></span>
		</td>
	</tr><?	
}

function display_incoming_extension_selector($colspan, $idprefix=false) {
	
	$table_id = $idprefix ? $idprefix . "_incoming_extension_selector" : "incoming_extension_selector";
	$row_id = $idprefix ? $idprefix . "_incoming_extension_selector_last" : "incoming_extension_selector_last";
	$add_params = $idprefix ? ",,\"$idprefix\"" : "";
	
	?><tr>
		<td width="20%" valign="top" class="vncell"><?=gettext("Incoming Extension(s)");?></td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable">
			<table id="<?=$table_id;?>">
				<tr id="<?=$row_id;?>">
					<td>&nbsp;</td>
					<td><img src="add.png" title="<?=gettext("add incoming extension mapping");?>" border="0" style="cursor: pointer; cursor: hand;" onclick="add_incoming_extension_selector(<?=$add_params;?>)"></td>
				</tr>
			</table><br><?
			
			// multiple instances don't need help for every instance
			if (!$idprefix) {				
				?><span class="vexpl"><?=gettext("Enter a pattern or a phone number and select a destination extension. To ring multiple phones, simply define them with the same pattern. If a phone number is entered, it will used as the \"external\" number on unanswered calls. If nothing is entered, all calls will be directed to the destination extension.<br>\n <ul>\n \t<li><strong>X</strong> - matches digits 0-9</li>\n \t<li><strong>Z</strong> - matches digits 1-9</li>\n \t<li><strong>N</strong> - matches digits 2-9</li>\n \t<li><strong>[13-5]</strong> - matches any digit in the brackets (here, 1,3,4,5)</li>\n \t<li><strong>.</strong> - matches one or more characters</li>\n \t<li><strong>!</strong> - matches zero or more characters</li>\n </ul>\n");?></span><?
			}

		?></td>
	</tr><?

}

function display_dtmfmode_selector($current_dtmfmode, $colspan) {
	global $dtmfmodes;
	
	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=gettext("DTMF Mode");?></td>
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
		<td valign="top" class="vncell"><?=gettext("Registration");?></td>
		<td colspan="<?=$colspan?>" class="vtable">
			<input name="noregister" id="noregister" type="checkbox" value="yes" <? if ($current_noregister) echo "checked"; ?>><?=gettext("Do not register with this provider.");?>
		</td>
	</tr><?
}

function display_qualify_options($current_qualify, $colspan) {
	
	?><tr> 
		<td valign="top" class="vncell"><?=gettext("Qualify");?></td>
		<td colspan="<?=$colspan?>" class="vtable">
			<input name="qualify" type="text" class="formfld" id="qualify" size="5" value="<?=htmlspecialchars($current_qualify);?>">&nbsp;<?=gettext("seconds");?> 
            <br><span class="vexpl"><?=gettext("Packets will be sent to this account every <i>n</i> seconds to check its status.<br>Defaults to '2'. Set to '0' to disable.");?></span>
		</td>
	</tr><?
}

// XXX : first option to fully implement the $defaults array, time to start updating the others
function display_natmode_selector($current_natmode, $colspan) {
	global $defaults;

	$natmodes = array(
		"yes" => gettext("always use NAT mode."),
		"no" => gettext("only use NAT mode according to RFC3581 (rport)"),
		"never" => gettext("never attempt NAT mode or RFC3581 support"),
		"route" => gettext("assume NAT mode but don't send rport ")
	);

	if (!$current_natmode) {
		$current_natmode = $defaults['sip']['natmode'];
	}

	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=gettext("NAT");?></td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable">
			<select name="natmode" class="formfld" id="natmode">
			<? foreach ($natmodes as $natmode => $natdesc) : ?>
			<option value="<?=$natmode;?>" <?
			if ($natmode == $current_natmode)
				echo "selected"; ?>
			><?=$natmode;?>&nbsp;-&nbsp;<?=$natdesc;?></option>
			<? endforeach; ?>
			</select>
			<br><span class="vexpl"><?=gettext("Is there a NATing device between AskoziaPBX and this account? Setting this to 'yes' will work in most cases. The options 'never' and 'route' are only rarely used for certain buggy SIP phones.");?></span>
		</td>
	</tr><?
}

function display_phone_ringlength_selector($current_ringlength, $colspan) {
	global $defaults;

	$ringlengths = array(
		"5", "10", "15", "20", "25", "30", "35", "40", "45", "50", "55", "60",
		"70", "80", "90", "100", "110", "120", "150", "180",
		gettext("indefinitely")
	);

	if (!$current_ringlength) {
		$current_ringlength = $defaults['accounts']['phones']['ringlength'];
	}

	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=gettext("Ring Length");?></td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable">
			<select name="ringlength" class="formfld" id="ringlength">
			<? foreach ($ringlengths as $ringlength) : ?>
			<option value="<?=$ringlength;?>" <?
			if ($ringlength == $current_ringlength)
				echo "selected"; ?>
			><?=$ringlength;?></option>
			<? endforeach; ?>
			</select>
			<br><span class="vexpl"><?=gettext("The number of seconds this phone will ring before being reported as unanswered.");?></span>
		</td>
	</tr><?
}

function display_analog_gain_selector($current_rxgain, $current_txgain, $colspan) {
	global $defaults;

	$mingain = -10;
	$maxgain = 10;
	$stepping = 0.1;

	if (!$current_rxgain) {
		$current_rxgain = $defaults['analog']['interface']['rxgain'];
	}
	if (!$current_txgain) {
		$current_txgain = $defaults['analog']['interface']['txgain'];
	}

	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=gettext("Line Gain");?></td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable">
			<select name="rxgain" class="formfld" id="rxgain">
			<? for ($i = $mingain; $i <= $maxgain; $i += $stepping) : ?>
			<? $i = round($i, 1); ?>
			<option value="<?=$i;?>" <?
			if ($i == $current_rxgain)
				echo "selected"; ?>
			><?=sprintf("%0.1f\n", $i);?></option>
			<? endfor; ?>
			</select>&nbsp;<?=gettext("receive");?>
			&nbsp;&nbsp;
			<select name="txgain" class="formfld" id="txgain">
			<? for ($i = $mingain; $i <= $maxgain; $i += $stepping) : ?>
			<? $i = round($i, 1); ?>
			<option value="<?=$i;?>" <?
			if ($i == $current_txgain)
				echo "selected"; ?>
			><?=sprintf("%0.1f\n", $i);?></option>
			<? endfor; ?>
			</select>&nbsp;<?=gettext("transmit");?>
			<br><span class="vexpl"><?=gettext("Some analog connections may need adjustment to achieve an acceptable audio level. Adjust the receive and transmit gain in decibels to amplify or quiet the line.");?></span>
		</td>
	</tr><?
}

function display_incoming_callerid_override_options($current_override, $current_overridestring, $colspan) {
	
	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=gettext("Incoming Caller ID Overrides");?></td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable">
			<input name="overridestring" type="text" class="formfld" id="overridestring" size="40" value="<?=htmlspecialchars($current_overridestring);?>">&nbsp;<i><?=gettext("Source Name &lt;number&gt;");?></i><br>
			<input name="override" type="radio" value="prepend" <?php if ($current_override == "prepend") echo "checked"; ?>> <?=gettext("Prepend with string");?>&nbsp;<input name="override" type="radio" value="replace" <?php if ($current_override == "replace") echo "checked"; ?>> <?=gettext("Replace with string");?><br>
			<input name="override" type="radio" value="replacenamewithnum" <?php if ($current_override == "replacenamewithnum") echo "checked"; ?>> <?=gettext("Replace calling name with calling number.");?><br>
			<input name="override" type="radio" value="disable" <?php if (!$current_override) echo "checked"; ?>> <?=gettext("Disable overrides");?>
		</td>
	</tr><?
}

function display_outgoing_callerid_options($current_callerid_source, $current_callerid_string, $colspan) {
	
	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=gettext("Outgoing Caller ID");?></td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable">
			<input name="calleridstring" type="text" class="formfld" id="calleridstring" size="40" value="<?=htmlspecialchars($current_callerid_string);?>">&nbsp;<i><?=gettext("Your Name &lt;number&gt;");?></i><br>
			<input name="calleridsource" type="radio" value="string" <?php if ($current_callerid_source == "string") echo "checked"; ?>> <?=gettext("Send string defined above&nbsp;");?><input name="calleridsource" type="radio" value="phones" <?php if ($current_callerid_source == "phones" || !$current_callerid_source) echo "checked"; ?>> <?=gettext("Send phones' Caller ID");?>
		</td>
	</tr><?
}

function display_provider_access_selector($current_providers, $current_outbounduridial, $colspan) {
	
	$a_providers = pbx_get_providers();

	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=gettext("Providers");?></td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable"><?
		
		$n = count($a_providers);
		if ($n == 0) {
			?><span class="vexpl"><i><?=gettext("There are currently no providers defined.");?></i></span><br><?

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
		?>><?=gettext("direct outbound dialing via SIP uris");?><br><?

		?><br>
		<span class="vexpl"><?=gettext("Checked providers will be accessible from this phone.");?></span>
		</td>
	</tr><?
}

function display_phone_access_selector($provider_uniqid, $colspan, $idprefix=false) {
	
	$a_phones = pbx_get_phones();

	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=gettext("Phones");?></td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable"><?
		
		$n = count($a_phones);
		if ($n == 0) {
			?><span class="vexpl"><i><?=gettext("There are currently no phones defined.");?></i></span><br><?

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
			?><br><span class="vexpl"><?=gettext("Checked phones will have access to this provider."); ?></span><?
		}
		
		?></td>
	</tr><?
}

function display_provider_dialpattern_editor($current_patterns, $colspan, $idprefix=false) {
	
	$n = count($current_patterns);
	if ($n == 0) {
		$n = 1 ;	
	}
	
	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=gettext("Dialing Pattern(s)");?></td>
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
			?><br><span class="vexpl"><?=gettext("Enter patterns, one per line, to define this provider's routing. Enter \"X!\" to use this Provider for all outgoing calls. To only use this Provider for calls starting with the number \"9\" enter \"9|.\". If no patterns are entered, only incoming calls will be accepted.<br>\n <ul>\n \t<li><strong>+</strong> - adds a prefix (i.e. \"1+555\" matches \"555\" and passes \"1555\" to the provider)</li>\n \t<li><strong>|</strong> - removes a prefix (i.e. \"1|555\" matches \"1555\" but only passes \"555\" to the provider)</li>\n \t<li><strong>X</strong> - matches digits 0-9</li>\n \t<li><strong>Z</strong> - matches digits 1-9</li>\n \t<li><strong>N</strong> - matches digits 2-9</li>\n \t<li><strong>[13-5]</strong> - matches any digit in the brackets (here, 1,3,4,5)</li>\n \t<li><strong>.</strong> - matches one or more characters (not allowed before | or +)</li>\n \t<li><strong>!</strong> - matches zero or more characters (not allowed before | or +)</li>\n </ul>\n");?></span><?
		}
		
		?></td>
	</tr><?
}

function display_channel_language_selector($current_language, $colspan) {
	global $prompt_languages;

	// XXX : needs to be changed from always being English and instead
	//			to recognize the webgui language

	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=gettext("Language");?></td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable">
			<select name="language" class="formfld" id="language">
			<? foreach($prompt_languages as $prompt_language=>$friendly) : ?>
			<option value="<?=$prompt_language;?>" <?
			if ($prompt_language == $current_language)
				echo "selected"; ?>
			><?=$friendly;?></option>
			<? endforeach; ?>
			</select>
			<br><span class="vexpl"><?=gettext("Audio prompts will be played back in the selected language for this account.");?></span>
		</td>
	</tr><?
}

function display_call_notifications_editor($current_voicemailbox, $current_sendcallnotifications, $current_novmwhenbusy, $colspan) {
	
	?><tr> 
		<td valign="top" class="vncell"><?=gettext("Voicemail");?></td>
		<td colspan="<?=$colspan?>" class="vtable">
			<input name="voicemailbox" type="text" class="formfld" id="voicemailbox" size="40" value="<?=htmlspecialchars($current_voicemailbox);?>">
			<br><input name="sendcallnotifications" id="sendcallnotifications" type="checkbox" value="yes"<?
			
			if ($current_sendcallnotifications) {
				echo "checked";
			}

			?>>&nbsp;<?=gettext("send missed call notifications");?><br>
			<input name="novmwhenbusy" id="novmwhenbusy" type="checkbox" value="yes"<?
            
			if ($current_novmwhenbusy) {
				echo "checked";
			}
            
			?>>&nbsp;<?=gettext("don't send to voicemail when busy");?><br>
			<br><span class="vexpl"><?=gettext("An e-mail address. If entered, voicemail will be enabled on this extension. Incoming messages will be sent to the given address.");?>
			<br><span class="red"><strong><?=gettext("Note:");?></strong></span> <?=gettext("Voicemail Services");?> <a href="services_voicemail.php"><?=gettext("must be configured</a> before this feature will work.");?></span>
		</td>
	</tr><?
}

function display_public_direct_dial_editor($current_allowdirectdial, $current_publicname, $colspan, $getjs=false) {
	global $config;

	$fullhostname = $config['system']['hostname'] . "." . $config['system']['domain'];

	?><tr> 
		<td valign="top" class="vncell"><?=gettext("Public Direct Dial");?></td>
		<td colspan="<?=$colspan?>" class="vtable">
			<input name="allowdirectdial" id="allowdirectdial" type="checkbox" value="yes" <?
			
			if ($current_allowdirectdial) {
				echo "checked";
			}

			?>>&nbsp;<?=gettext("allow extension to be directly dialed over public networks");?>
			<br><span name="publicname_wrapper" id="publicname_wrapper" style="display: none;">
			<br><input name="publicname" type="text" class="formfld" id="publicname" size="20" value="<?=htmlspecialchars($current_publicname);?>">@<?=$fullhostname;?>
			<br><span class="vexpl"><?=gettext("Public name. Enter a string to map a friendly name to this extension.");?></span>
			<br>
			</span>
			<br><span class="vexpl"><?=sprintf(gettext("Enabling this option will allow this extension to be reachable via SIP and IAX2 uris (extension@%s)"), $fullhostname);?></span>
		</td>
	</tr><?
}

function display_call_and_busy_limit_selector($current_calllimit, $current_busylimit, $colspan) {
	
	?><tr> 
		<td valign="top" class="vncell"><?=gettext("Call Limit");?></td>
		<td colspan="<?=$colspan?>" class="vtable">
			<input name="calllimit" type="text" class="formfld" id="calllimit" size="5" value="<?=htmlspecialchars($current_calllimit);?>">&nbsp;<?=gettext("calls");?> 
            <br><span class="vexpl"><?=gettext("Permit only 'n' outgoing call(s) and 'n' incoming call(s) (default: 2)");?></span>
		</td>
	</tr>
	<tr> 
		<td valign="top" class="vncell"><?=gettext("Busy Limit");?></td>
		<td colspan="<?=$colspan?>" class="vtable">
			<input name="busylimit" type="text" class="formfld" id="busylimit" size="5" value="<?=htmlspecialchars($current_busylimit);?>">&nbsp;<?=gettext("calls");?> 
            <br><span class="vexpl"><?=gettext("Phone status will be switched to 'busy' with 'n' concurrent calls (default: 1)");?></span>
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
			?><br><span class="vexpl"><?=gettext("Enter application logic, one command per line as shown: <br> <pre> 1,Answer()\n 2,Echo()\n 3,Hangup()\n </pre> <p>Programming applications for AskoziaPBX requires knowledge of the Asterisk dialplan. An effort is made to verify this input but the possibility remains that an AskoziaPBX install may be rendered useless by faulty application logic. A reference of applications, functions and phones is available in the <strong><a href=\"popup_dialplan_element_library.php\" target=\"_blank\">Element Library</a></strong>.</p>"); ?></span><?
		}

		?></td>
	</tr><?
}

function display_advanced_settings_begin($colspan) {

		?><tr>
			<td class="list" colspan="<?=$colspan + 1;?>" height="12">&nbsp;</td>
		</tr>
		<tr>
			<td colspan="<?=$colspan + 1;?>" class="listtopiclight" id="advanced-settings-header"><?=gettext("Advanced Settings");?> (<a href="javascript:{}" id="advanced-settings-opener" class="link_color_black"><?=gettext("show");?></a>)</td>
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
		<td width="20%" valign="top" class="vncell"><?=gettext("Manual Attributes");?></td>
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
			?><br><span class="vexpl"><?=gettext("Manual key-value pairs can be entered in addition to the generated configuration (i.e. <em>configoption=value</em>). These settings will be appended to this item's Asterisk configuration file or context. Unless you have a very specific need to do this, nothing should be entered in this field. Improperly formed values can break system functionality and/or open security holes."); ?></span><?
		}

		?></td>
	</tr><?
}

function display_caller_id_field($current_callerid, $colspan) {

	?><tr> 
		<td valign="top" class="vncellreq"><?=gettext("Caller ID");?></td>
		<td colspan="<?=$colspan;?>" class="vtable">
			<input name="callerid" type="text" class="formfld" id="callerid" size="40" value="<?=htmlspecialchars($current_callerid);?>"> 
			<br><span class="vexpl"><?=gettext("Text to be displayed for Caller ID.");?></span>
		</td>
	</tr><?
}

function display_description_field($current_description, $colspan) {

	?><tr> 
		<td valign="top" class="vncell"><?=gettext("Description");?></td>
		<td colspan="<?=$colspan;?>" class="vtable">
			<input name="descr" type="text" class="formfld" id="descr" size="40" value="<?=htmlspecialchars($current_description);?>"> 
			<br><span class="vexpl"><?=gettext("You may enter a description here for your reference (not parsed).");?></span>
		</td>
	</tr><?
}

function display_readback_number_field($current_readbacknumber, $colspan) {

	?><tr> 
		<td valign="top" class="vncell"><?=gettext("Public Number");?></td>
		<td colspan="<?=$colspan;?>" class="vtable">
			<input name="readbacknumber" type="text" class="formfld" id="readbacknumber" size="40" value="<?=htmlspecialchars($current_readbacknumber);?>"> 
			<br><span class="vexpl"><?=gettext("This 'external' number will be read back to the caller when reaching voicemail; defaults to account's username if it is numeric. If it is not, the internal extension this call was routed to will be read back.");?></span>
		</td>
	</tr><?
}

function display_voicemail_authtype_selector($current_authtype, $colspan) {
	global $defaults;

	$authtypes = array(
		"auto",
		"plain",
		"login",
		"cram-md5",
		//"digest-md5", (needs gnu sasl)
		//"gssapi", (needs gnu sasl)
		"external"
		//"ntlm" (needs gnu sasl)
	);

	if (!$current_authtype) {
		$current_authtype = $defaults['voicemail']['authmode'];
	}

	?><tr> 
		<td width="20%" valign="top" class="vncell"><?=gettext("Authentication Method");?></td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable">
			<select name="authtype" class="formfld" id="authtype">
			<? foreach ($authtypes as $authtype) : ?>
			<option value="<?=$authtype;?>" <?
			if ($authtype == $current_authtype)
				echo "selected"; ?>
			><?=$authtype;?></option>
			<? endforeach; ?>
			</select>
			<br><span class="vexpl"><?=gettext("Most accounts will work using 'auto'. If this fails, select an authentication method appropriate for your server.");?></span>
		</td>
	</tr><?
}

function display_capacity_bar($sizeinbytes, $usage) {

	?><img src='bar_left.gif' height='15' width='4' border='0' align='absmiddle'><img src='bar_blue.gif' height='15' width='<?=$usage;?>' border='0' align='absmiddle'><img src='bar_gray.gif' height='15' width='<?=100-$usage;?>' border='0' align='absmiddle'><img src='bar_right.gif' height='15' width='5' border='0' align='absmiddle'>
	<br><?=$usage;?>% <?=gettext("of");?> <?=format_bytes($sizeinbytes);?><?
}

function display_peer_status_icon($status, $id) {
	switch ($status) {
		case "OK":
		return "<img id=\"" . $id . "\" src=\"bullet_green.png\" title=\"connected\">";
		case "LAGGED":
		return "<img id=\"" . $id . "\" src=\"bullet_yellow.png\" title=\"connected but lagged\">";
		case "UNREACHABLE":
		return "<img id=\"" . $id . "\" src=\"bullet_red.png\" title=\"unreachable\">";
		case "UNKNOWN":
		return "<img id=\"" . $id . "\" src=\"bullet_white.png\" title=\"status unknown\">";
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
					<td class="firmware_update_info_message"><?=$message;?></td>
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
					<?=gettext("The following input errors were detected:");?>
					<ul><?

					foreach ($input_errors as $ierr) {
							echo "<li>" . htmlspecialchars($ierr) . "</li>\n";
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
					<td class="print_info_box_msg"><?=$msg;?></td>
				</tr>
			</table>
		</p>
	</span><?

}

?>
