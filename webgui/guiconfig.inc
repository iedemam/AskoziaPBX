<?php
/*
	$Id$
	part of m0n0wall (http://m0n0.ch/wall)
	
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
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

/* make sure nothing is cached */
if (!$omit_nocacheheaders) {
	header("Expires: 0");
	header("Last-Modified: " . gmdate("D, d M Y H:i:s") . " GMT");
	header("Cache-Control: no-store, no-cache, must-revalidate");
	header("Cache-Control: post-check=0, pre-check=0", false);
	header("Pragma: no-cache");
}

/* parse the configuration and include all configuration functions */
require_once("config.inc");
require_once("functions.inc");
require_once("help.inc");

$d_staticroutesdirty_path = $g['varrun_path'] . "/staticroutes.dirty";
$d_sipconfdirty_path = $g['varrun_path'] . "/sipconf.dirty";
$d_iaxconfdirty_path = $g['varrun_path'] . "/iaxconf.dirty";
$d_extensionsconfdirty_path = $g['varrun_path'] . "/extensionsconf.dirty";
$d_featuresconfdirty_path = $g['varrun_path'] . "/featuresconf.dirty";
$d_conferencingconfdirty_path = $g['varrun_path'] . "/conferencingconf.dirty";
$d_conferencing_kicked_path = $g['varrun_path'] . "/conferencing.kicked";
$d_managerconfdirty_path = $g['varrun_path'] . "/managerconf.dirty";
$d_isdnconfdirty_path = $g['varrun_path'] . "/isdnconf.dirty";
$d_analogconfdirty_path = $g['varrun_path'] . "/analogconf.dirty";
$d_rtpconfdirty_path = $g['varrun_path'] . "/rtpconf.dirty";
$d_fwupenabled_path = $g['varrun_path'] . "/fwup.enabled";
$d_firmwarelock_path = $g['varrun_path'] . "/firmware.lock";
$d_sysrebootreqd_path = $g['varrun_path'] . "/sysreboot.reqd";

if (file_exists($d_firmwarelock_path)) {
	if (!$d_isfwfile) {
		header("Location: system_firmware.php");
		exit;
	} else {
		return;
	}
}

$specialnets = array("lan" => "LAN net");
	
$medias = array("auto" => "autoselect", "100full" => "100BASE-TX full-duplex",
	"100half" => "100BASE-TX half-duplex", "10full" => "10BASE-T full-duplex",
	"10half" => "10BASE-T half-duplex", "1000full" => "1000BASE-TX full-duplex");
	
/* platforms that support firmware updating */
$fwupplatforms = array('net45xx', 'net48xx', 'net55xx', 'generic-pc', 'wrap', 'alix1x', 'alix23x', 'hl4xx');


/*     1 (1 <<  0)      (0x1)  audio       g723   (G.723.1)
      2 (1 <<  1)      (0x2)  audio        gsm   (GSM)
      4 (1 <<  2)      (0x4)  audio       ulaw   (G.711 u-law)
      8 (1 <<  3)      (0x8)  audio       alaw   (G.711 A-law)
     16 (1 <<  4)     (0x10)  audio   g726aal2   (G.726 AAL2)
     32 (1 <<  5)     (0x20)  audio      adpcm   (ADPCM)
     64 (1 <<  6)     (0x40)  audio       slin   (16 bit Signed Linear PCM)
    128 (1 <<  7)     (0x80)  audio      lpc10   (LPC10)
    256 (1 <<  8)    (0x100)  audio       g729   (G.729A)
    512 (1 <<  9)    (0x200)  audio      speex   (SpeeX)
   1024 (1 << 10)    (0x400)  audio       ilbc   (iLBC)
   2048 (1 << 11)    (0x800)  audio       g726   (G.726 RFC3551)
   4096 (1 << 12)   (0x1000)  audio       g722   (G722)
  65536 (1 << 16)  (0x10000)  image       jpeg   (JPEG image)
 131072 (1 << 17)  (0x20000)  image        png   (PNG image)
 262144 (1 << 18)  (0x40000)  video       h261   (H.261 Video)
 524288 (1 << 19)  (0x80000)  video       h263   (H.263 Video)
1048576 (1 << 20) (0x100000)  video      h263p   (H.263+ Video)
2097152 (1 << 21) (0x200000)  video       h264   (H.264 Video)     */

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

$video_codecs = array(
	"h261" => "H.261",
	"h263" => "H.263",
	"h263p" => "H.263+",
	"h264" => "H.264"
);

$topologies = array(
	"public" => "Public IP address",
	"natstatic" => "NAT + static public IP",
	"natdynamichost" => "NAT + dynamic public IP"
);

$dtmfmodes = array(
	"auto",
	"rfc2833",
	"inband",
	"info"
);

$analog_startsignals = array(
	"ks" => "Kewl Start",
	"gs" => "Ground Start",
	"ls" => "Loop Start"
);

$applications = array(
	"Echo" => "Echos audio and video back to the caller as soon as it is received. Used to test connection delay.",
	"Milliwatt" => "Generates a constant 1000Hz tone. Used to test connection quality.",
	"WakeMe" => "Wake-Up call manager. Call to set and cancel wake-up calls on the current extension."
);

$prompt_languages = array(
	"en" => "English (US)",
	"en-gb" => "English (UK)",
	"nl" => "Dutch",
	"fr" => "French (France)",
	"fr-ca" => "French (Canada)",
	"de" => "German",
	"it" => "Italian",
	"jp" => "Japanese",
	"ru" => "Russian",
	"es" => "Spanish",
	"se" => "Swedish"
);

$vm_email_languages = array(
	"en-informal" => "English",
	"en-formal" => "English (formal)",
	"nl-informal" => "Dutch",
	"nl-formal" => "Dutch (formal)",
	"fr-informal" => "French",
	"fr-formal" => "French (formal)",
	"de-informal" => "German",
	"de-formal" => "German (formal)",
	"it-informal" => "Italian",
	"it-formal" => "Italian (formal)",
	"pl-informal" => "Polish",
	"pl-formal" => "Polish (formal)",
	"es-formal" => "Spanish (formal)",
	"se-informal" => "Swedish"
);

$isdn_dchannel_modes = array(
	0 => "",
	"DRVR_DSS1_TE" => "euro-ISDN, point-to-multipoint, terminal equipment",
	"DRVR_DSS1_NT" => "euro-ISDN, multipoint-to-point, network termination",
	"DRVR_DSS1_P2P_TE" => "euro-ISDN, point-to-point, terminal equipment",
	"DRVR_DSS1_P2P_NT" => "euro-ISDN, point-to-point, network termination"
//	"DRVR_D_CHANNEL" => "default D-channel driver",
//	"DRVR_CAPI_TE" => "",
//	"DRVR_DIEHL_TE" => "",
//	"DRVR_TINA_DD_TE" => "",
//	"DRVR_AMV_B1_TE" => "",
//	"DRVR_D64S" => "This driver can be used on single B-channel leased lines.",
//	"DRVR_DUMMY" => "",
);

$zaptel_loadzones = array(
	"us" => "United States / North America",
	"au" => "Australia",
	"at" => "Austria",
	"be" => "Belgium",
	"cl" => "Chile",
	"cn" => "China",
	"fi" => "Finland",
	"fr" => "France",
	"gr" => "Greece",
	"it" => "Italy",
	"jp" => "Japan",
	"my" => "Malaysia",
	"nl" => "Netherlands",
	"nz" => "New Zealand",
	"no" => "Norway",
	"ru" => "Russia",
	"es" => "Spain",
	"se" => "Sweden",
	"tw" => "Taiwan",
	"uk" => "United Kingdom",
	"us-old" => "United States Circa 1950/ North America"
);

$system_tonezones = array(
	"us" => "United States / North America",
	"au" => "Australia",
	"at" => "Austria",
	"be" => "Belgium",
	"br" => "Brazil",
	"bg" => "Bulgaria",
	"cl" => "Chile",
	"cn" => "China",
	"cz" => "Czech Republic",
	"dk" => "Denmark",
	"ee" => "Estonia",
	"fi" => "Finland",
	"fr" => "France",
	"de" => "Germany",
	"gr" => "Greece",
	"hu" => "Hungary",
	"in" => "India",
	"il" => "Israel",
	"it" => "Italy",
	"jp" => "Japan",
	"lt" => "Lithuania",
	"my" => "Malaysia",
	"mx" => "Mexico",
	"nl" => "Netherlands",
	"nz" => "New Zealand",
	"no" => "Norway",
	"ph" => "Philippines",
	"pl" => "Poland",
	"pt" => "Portugal",
	"ru" => "Russia / ex Soviet Union",
	"sg" => "Singapore",
	"za" => "South Africa",
	"es" => "Spain",
	"se" => "Sweden",
	"ch" => "Switzerland",
	"tw" => "Taiwan",
	"th" => "Thailand",
	"uk" => "United Kingdom",
	"us-old" => "United States Circa 1950/ North America",
	"ve" => "Venezuela / South America"
);

$analog_signaling = array(
	"fxs" => "fxo",
	"fxo" => "fxs"
);

$manager_permissions = array(
	"agent",
	"call",
	"command",
	"config",
	"log",
	"user",
	"verbose",
	"system"
);


function do_input_validation($postdata, $reqdfields, $reqdfieldsn, $input_errors) {
	
	/* check for bad control characters */	
	foreach ($postdata as $pn => $pd) {
		if (is_string($pd) && preg_match("/[\\x00-\\x08\\x0b\\x0c\\x0e-\\x1f]/", $pd)) {
			$input_errors[] = "The field '" . $pn . "' contains invalid characters.";
		}
	}
	
	for ($i = 0; $i < count($reqdfields); $i++) {
		if (!isset($_POST[$reqdfields[$i]]) || ($_POST[$reqdfields[$i]] === "")) {
			$input_errors[] = "The field '" . $reqdfieldsn[$i] . "' is required.";
		}
	}	
}

function print_input_errors($input_errors) {
	echo "<p><table border=\"0\" cellspacing=\"0\" cellpadding=\"4\" width=\"100%\">\n";
	echo "<tr><td bgcolor=\"#A12A2A\" width=\"36\" align=\"center\" valign=\"top\"><img src=\"/err.gif\" width=\"28\" height=\"32\"></td>\n";
	echo "<td bgcolor=\"#FFD9D1\" style=\"padding-left: 8px; padding-top: 6px\">";
	
	echo "<span class=\"errmsg\"><p>The following input errors were detected:<ul>\n";
	foreach ($input_errors as $ierr) {
		echo "<li>" . htmlspecialchars($ierr) . "</li>\n";
	}
	echo "</ul></span>";
	
	echo "</td></tr></table></p>";
}

function exec_rc_script($scriptname) {
	
	global $d_sysrebootreqd_path;
	
	if (file_exists($d_sysrebootreqd_path))
		return 0;
	
	exec($scriptname . " >/dev/null 2>&1", $execoutput, $retval);
	return $retval; 
}

function exec_rc_script_async($scriptname) {
	
	global $d_sysrebootreqd_path;
	
	if (file_exists($d_sysrebootreqd_path))
		return 0;
		
	exec("nohup " . $scriptname . " >/dev/null 2>&1 &", $execoutput, $retval);
	return $retval; 
}

function verify_gzip_file($fname) {

	$returnvar = mwexec("/usr/bin/gunzip -S \"\" -t " . escapeshellarg($fname));
	if ($returnvar != 0)
		return 0;
	else
		return 1;
}

function print_info_box_np($msg) {
	echo "<table border=\"0\" cellspacing=\"0\" cellpadding=\"4\" width=\"100%\">\n";
	echo "<tr><td bgcolor=\"#687BA4\" align=\"center\" valign=\"top\" width=\"36\"><img src=\"/exclam.gif\" width=\"28\" height=\"32\"></td>\n";
	echo "<td bgcolor=\"#D9DEE8\" style=\"padding-left: 8px\">";
	echo $msg;
	echo "</td></tr></table>";
}

function print_info_box($msg) {
	echo "<p>";
	print_info_box_np($msg);
	echo "</p>";
}

function format_bytes($bytes) {
	if ($bytes >= 1073741824) {
		return sprintf("%.2f GB", $bytes/1073741824);
	} else if ($bytes >= 1048576) {
		return sprintf("%.2f MB", $bytes/1048576);
	} else if ($bytes >= 1024) {
		return sprintf("%.0f KB", $bytes/1024);
	} else {
		return sprintf("%d bytes", $bytes);
	}
}

function get_std_save_message($ok) {
	global $d_sysrebootreqd_path;

	if ($ok == 0) {
		if (file_exists($d_sysrebootreqd_path))
			return "The changes have been saved. You must <a href=\"/system_reboot.php\">reboot</a> your pbx for the changes to take effect.";
		else
			return "The changes have been applied successfully.";
	} else {
		return "ERROR: the changes could not be applied (error code $ok).";
	}
}

function pprint_address($adr) {
	global $specialnets;

	if (isset($adr['any'])) {
		$padr = "*";
	} else if ($adr['network']) {
		$padr = $specialnets[$adr['network']];
	} else {
		$padr = $adr['address'];
	}
	
	if (isset($adr['not']))
		$padr = "! " . $padr;
	
	return $padr;
}

function pprint_port($port) {
	global $wkports;

	$pport = "";
		
	if (!$port)
		echo "*";
	else {
		$srcport = explode("-", $port);
		if ((!$srcport[1]) || ($srcport[0] == $srcport[1])) {
			$pport = $srcport[0];
			if ($wkports[$srcport[0]]) {
				$pport .= " (" . $wkports[$srcport[0]] . ")";
			}
		} else
			$pport .= $srcport[0] . " - " . $srcport[1];
	}
	
	return $pport;
}

function staticroutes_sort() {
	global $g, $config;

	function staticroutecmp($a, $b) {
		return strcmp($a['network'], $b['network']);
	}

	usort($config['staticroutes']['route'], "staticroutecmp");
}


function dynamic_tab_menu(&$tabs) {
	global $allowed, $config;
	
	if ($_SERVER['REMOTE_USER'] == $config['system']['username']) {
		$authorized = &$tabs;
	} else {
		$authorized = &$allowed;
	}
	
	// Below needed to add the "1" onto the end of the class name for the first tab, if it is inactive.
    $linkStyle = '1';
   	foreach ($tabs as $desc => $link) {
   		if (in_array($link,$authorized)) {
            if ($link == basename($_SERVER['PHP_SELF'])) {
   				// special handling for diagnostic Logs tabs.
   				if ((strpos($link,'diag_logs') > -1) && ($link != 'diag_logs_settings.php')) {
	   				if ($desc == "Firewall") {
	       				echo '<li class="tabact"><a href="diag_logs_filter.php" style="color:black" title="reset filter and reload firewall logs page">Firewall</a></li>';
	   				}
	   				else {
	   					echo '<li class="tabact"><a href="'.$link.'" style="color:black" title="reload page">'.$desc.'</a></li>';
	   				}
	   			}
       			else {
       				echo '<li class="tabact">'.$desc.'</li>';
       			}
   				$linkStyle = '';
   			}
   			else {
   				echo '<li class="tabinact'.$linkStyle.'"><a href="'.$link.'">'.$desc.'</a></li>';
   				$linkStyle = '';
   			}
   		}
    }	
}

function print_r_html($data, $return_data=true) {
    $data = print_r($data, true);
    $data = str_replace( " ", "&nbsp;", $data);
    $data = str_replace( "\r\n", "<br>\r\n", $data);
    $data = str_replace( "\r", "<br>\r", $data);
    $data = str_replace( "\n", "<br>\n", $data);

    if (!$return_data)
        echo $data;   
    else
        return $data;
}

function split_and_clean_patterns($patterns_string) {
	$patterns_string = str_replace(" ", "", $patterns_string);
	$patterns = preg_split("/[\x0D\x0A]+/", $patterns_string, -1, PREG_SPLIT_NO_EMPTY);
	
	return $patterns;
}

// XXX : multiple still needs to be implemented for dialplan_providers page
function gather_incomingextensionmaps($post, $multiple=false) {
	
	$maps = array();
	$keys = array_keys($post);
	
	foreach ($keys as $key) {
		if ((strpos($key, "destinationid:") !== false && $post[$key]) ||
			(strpos($key, "incomingpattern:") !== false && strlen($post[$key]))) {
				
			$match = explode(":", $key);
			$maps[$match[1]][$match[0]] = $post[$key];
		}
	}

	return $maps;
}


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
	
	$internal_phones = asterisk_get_phones();
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
					?><li class="gme" id="gme_<?=$member;?>"><?=asterisk_uniqid_to_name($member);?></li><?
				}
			}
			
			?></ul>
		</td>
		<td width="40%" class="vtable" valign="top"><strong>Non-Members</strong>
			<ul id="gmd" class="gmd" style="min-height:50px"><?
			

			foreach ($indexed_phones as $uniqid=>$phone) {
				if (!in_array($uniqid, $current_members)) {
					?><li class="gmd" id="gmd_<?=$uniqid;?>"><?=asterisk_uniqid_to_name($phone['uniqid']);?></li><?
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

function display_incoming_extension_selector($current_incoming_extensions, $colspan, $idprefix=false) {
	global $help, $help_language;
	
	?><tr>
		<td width="20%" valign="top" class="vncell">Incoming Extension(s)</td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable"><?
			
			$n = count($current_incoming_extensions);
			if ($n == 0) {
				display_incoming_extension_selector_sub(0, false, $idprefix);
				echo "<br>";
			} else {
				for ($i = 0; $i < $n; $i++) {
					display_incoming_extension_selector_sub($i, $current_incoming_extensions[$i], $idprefix);
					echo "<br>";
				}
			}

		// multiple instances don't need help for every instance
		if (!$idprefix) {				
			?><span class="vexpl"><?=$help[$help_language]["display_incoming_extension_selector"]; ?></span><?
		}

		?></td>
	</tr><?

}

function display_incoming_extension_selector_sub($id, $current_incoming_extension, $idprefix=false) {
	
	$destinationid = $current_incoming_extension['destinationid'];
	$pattern = $current_incoming_extension['pattern'];
	
	if ($idprefix) {
		?><input name="<?=$idprefix;?>:incomingpattern:<?=$id;?>" type="text" class="formfld" id="<?=$idprefix;?>:incomingpattern:<?=$id;?>" size="20" value="<?=htmlspecialchars($pattern);?>"><?
	} else {
		?><input name="incomingpattern:<?=$id;?>" type="text" class="formfld" id="incomingpattern:<?=$id;?>" size="20" value="<?=htmlspecialchars($pattern);?>"><?
	}
	
	if ($idprefix) {
		?><select name="<?=$idprefix;?>:destinationid:<?=$id;?>" class="formfld" id="<?=$idprefix;?>:destinationid:<?=$id;?>"><?
	} else {
		?><select name="destinationid:<?=$id;?>" class="formfld" id="destinationid:<?=$id;?>"><?
	}

			?><option value="">Outgoing Calls Only</option>
			<option></option><?
			$have_extension = false;
			
		if (count($a_items = dialplan_get_callgroups())) {
			$have_extension = true;
			echo "<option value=\"\">Call Groups</option>";
			foreach ($a_items as $item) {
				echo "<option value=\"{$item['uniqid']}\"";
				if ($item['uniqid'] == $destinationid) {
					echo " selected";
				}
			 	echo ">&nbsp;&nbsp;{$item['name']}</option>";
			}
		}
		
		if (count($a_items = sip_get_phones())) {
			$have_extension = true;
			echo "<option value=\"\">SIP Phones</option>";
			foreach ($a_items as $item) {
				echo "<option value=\"{$item['uniqid']}\"";
				if ($item['uniqid'] == $destinationid) {
					echo " selected";
				}
			 	echo ">&nbsp;&nbsp;{$item['callerid']} &lt;{$item['extension']}&gt</option>";
			}
		}
		
		if (count($a_items = iax_get_phones())) {
			$have_extension = true;
			echo "<option value=\"\">IAX Phones</option>";
			foreach ($a_items as $item) {
				echo "<option value=\"{$item['uniqid']}\"";
				if ($item['uniqid'] == $destinationid) {
					echo " selected";
				}
			 	echo ">&nbsp;&nbsp;{$item['callerid']} &lt;{$item['extension']}&gt</option>";
			}
		}
		
		if (count($a_items = isdn_get_phones())) {
			$have_extension = true;
			echo "<option value=\"\">ISDN Phones</option>";
			foreach ($a_items as $item) {
				echo "<option value=\"{$item['uniqid']}\"";
				if ($item['uniqid'] == $destinationid) {
					echo " selected";
				}
			 	echo ">&nbsp;&nbsp;{$item['callerid']} &lt;{$item['extension']}&gt</option>";
			}
		}
		
		if (count($a_items = analog_get_phones())) {
			$have_extension = true;
			echo "<option value=\"\">Analog Phones</option>";
			foreach ($a_items as $item) {
				echo "<option value=\"{$item['uniqid']}\"";
				if ($item['uniqid'] == $destinationid) {
					echo " selected";
				}
			 	echo ">&nbsp;&nbsp;{$item['callerid']} &lt;{$item['extension']}&gt</option>";
			}
		}

		if (count($a_items = external_get_phones())) {
			$have_extension = true;
			echo "<option value=\"\">External Phones</option>";
			foreach ($a_items as $item) {
				echo "<option value=\"{$item['uniqid']}\"";
				if ($item['uniqid'] == $destinationid) {
					echo " selected";
				}
			 	echo ">&nbsp;&nbsp;{$item['name']} &lt;{$item['dialstring']}&gt</option>";
			}
		}
		
		if (count($a_items = conferencing_get_rooms())) {
			$have_extension = true;
			echo "<option value=\"\">Conference Rooms</option>";
			foreach ($a_items as $item) {
				echo "<option value=\"{$item['uniqid']}\"";
				if ($item['uniqid'] == $destinationid) {
					echo " selected";
				}
			 	echo ">&nbsp;&nbsp;{$item['name']} &lt;{$item['number']}&gt</option>";
			}
		}

		if (count($a_items = dialplan_get_applications())) {
			$have_extension = true;
			echo "<option value=\"\">Applications</option>";
			foreach ($a_items as $item) {
				echo "<option value=\"{$item['uniqid']}\"";
				if ($item['uniqid'] == $destinationid) {
					echo " selected";
				}
			 	echo ">&nbsp;&nbsp;{$item['name']}</option>";
			}
		}

		if (!$have_extension) {
			?><option value="">no incoming extensions available</option><?
		}
		
        ?></select><?

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

function display_provider_access_selector($current_providers, $colspan) {
	
	$a_providers = asterisk_get_providers();

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
		
		?><br>
		<span class="vexpl">Checked providers will be accessible from this phone.</span>
		</td>
	</tr><?
}

function display_phone_access_selector($provider_uniqid, $colspan, $idprefix=false) {
	global $help, $help_language;
	
	$a_phones = asterisk_get_phones();

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
					?><input name="<?=$idprefix.":".$phone['uniqid'];?>" id="<?=$idprefix.":".$phone['uniqid'];?>" type="checkbox" value="<?=$idprefix;?>"<?
				
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
			$idstring = $idprefix . ":dialpattern";
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

?>
