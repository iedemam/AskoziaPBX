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
$d_conferencingconfdirty_path = $g['varrun_path'] . "/conferencingconf.dirty";
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
$fwupplatforms = array('net45xx', 'net48xx', 'generic-pc', 'generic-pc-smp', 'wrap');


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

$prompt_languages = array(
	"en" => "English",
	"nl" => "Dutch",
	"de" => "German",
	"es" => "Spanish",
	"fr" => "French",
	"it" => "Italian",
	"jp" => "Japanese",
	"se" => "Swedish"
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

function display_incoming_extension_selector($current_incoming_extension, $colspan, $idprefix=false) {
	global $help, $help_language;
	
	?><tr>
		<td width="20%" valign="top" class="vncell">Incoming Extension</td>
		<td width="80%" colspan="<?=$colspan?>" class="vtable">
			<? if ($idprefix): ?>
			<select name="<?=$idprefix;?>:incomingextension" class="formfld" id="<?=$idprefix;?>:incomingextension">
			<? else: ?>
			<select name="incomingextension" class="formfld" id="incomingextension">
			<? endif; ?>
				<option>Outgoing Calls Only</option>
				<option></option>
				<?php $have_extension = false; ?>

				<?php if (count($a_sipphones = sip_get_phones()) > 0): ?>
				<?php $have_extension = true; ?>
				<option>SIP Phones</option>
				<?php foreach ($a_sipphones as $phone): ?>
				<option value="<?=$phone['uniqid'];?>" <?php 
				if ($phone['uniqid'] == $current_incoming_extension) 
					echo "selected"; ?>> 
				<? echo "&nbsp;&nbsp;{$phone['callerid']} &lt;{$phone['extension']}&gt;"; ?></option>
				<?php endforeach; ?>
				<?php endif; ?>

				<?php if (count($a_iaxphones = iax_get_phones()) > 0): ?>
				<?php $have_extension = true; ?>
				<option>IAX Phones</option>
				<?php foreach ($a_iaxphones as $phone): ?>
				<option value="<?=$phone['uniqid'];?>" <?php 
				if ($phone['uniqid'] == $current_incoming_extension) 
					echo "selected"; ?>> 
				<? echo "&nbsp;&nbsp;{$phone['callerid']} &lt;{$phone['extension']}&gt;"; ?></option>
				<?php endforeach; ?>
				<?php endif; ?>
			
				<?php if (count($a_rooms = conferencing_get_rooms()) > 0): ?>
				<?php $have_extension = true; ?>
				<option>Conference Rooms</option>
				<?php foreach ($a_rooms as $room): ?>
				<option value="<?=$room['uniqid'];?>" <?php 
				if ($room['uniqid'] == $current_incoming_extension) 
					echo "selected"; ?>> 
				<? echo "&nbsp;&nbsp;{$room['name']} &lt;{$room['number']}&gt;"; ?></option>
				<?php endforeach; ?>
				<?php endif; ?>
			
				<?php if (!$have_extension): ?>
				<option>no incoming extensions available</option>
				<?php endif; ?>

            </select><?

		// multiple instances don't need help for every instance
		if (!$idprefix) {				
			?><br><span class="vexpl"><?=$help[$help_language]["display_incoming_extension_selector"]; ?></span><?
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
?>
