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
require_once("display.inc");
require_once("javascript.inc");
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
$d_packageconfdirty_path = $g['varrun_path'] . "/packageconf.dirty";
$d_fwupenabled_path = $g['varrun_path'] . "/fwup.enabled";
$d_ultmpmounted_path = $g['varrun_path'] . "/ultmp.mounted";
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
	
/* platforms that do not support firmware updating */
$no_firmware_update_platforms = array();


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
	"da" => "Danish",
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
	"da-informal" => "Danish",
	"da-formal" => "Danish (formal)",
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
	"pt-br-informal" => "Portuguese, Brazil",
	"pt-br-formal" => "Portuguese, Brazil (formal)",
	"ru-informal" => "Russian",
	"ru-formal" => "Russian (formal)",
	"es-formal" => "Spanish (formal)",
	"se-informal" => "Swedish"
);

$isdn_dchannel_modes = array(
	0 => "",
	"DRVR_DSS1_TE" => "point-to-multipoint, terminal equipment",
	"DRVR_DSS1_NT" => "multipoint-to-point, network termination",
	"DRVR_DSS1_P2P_TE" => "point-to-point, terminal equipment",
	"DRVR_DSS1_P2P_NT" => "point-to-point, network termination"
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

function split_and_clean_lines($s) {
	$lines = preg_split("/[\x0D\x0A]+/", $s, -1, PREG_SPLIT_NO_EMPTY);
	$n = count($lines);
	for ($i = 0; $i < $n; $i++) {
		$lines[$i] = trim($lines[$i], " ");
	}
	return $lines;
}


function gather_incomingextensionmaps($post, $multi=false) {
	
	$maps = array();
	$keys = array_keys($post);
	
	if ($multi) {
		// XXX : the entire multiple vs single display mechanism needs to be redesigned...this will get / is horrible
	} else {
		foreach ($keys as $key) {
			if ((strpos($key, "destinationid_") !== false && $post[$key]) ||
				(strpos($key, "incomingpattern_") !== false && strlen($post[$key]))) {

				$match = explode("_", $key);
				$maps[$match[1]][$match[0]] = $post[$key];
			}
		}	
	}

	return $maps;
}

?>
