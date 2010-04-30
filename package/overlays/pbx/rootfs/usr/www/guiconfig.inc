<?php
/*
	$Id$
	originally part of m0n0wall (http://m0n0.ch/wall)
	continued modifications as part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
	Copyright (C) 2007-2009 tecema (a.k.a IKT) <http://www.tecema.de>.
	
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
require_once("i18n.inc");
require_once("functions.inc");
require_once("display.inc");
require_once("javascript.inc");

$d_staticroutesdirty_path = $g['varrun_path'] . "/staticroutes.dirty";
$d_extensionsconfdirty_path = $g['varrun_path'] . "/extensionsconf.dirty";
$d_featuresconfdirty_path = $g['varrun_path'] . "/featuresconf.dirty";
$d_conferencingconfdirty_path = $g['varrun_path'] . "/conferencingconf.dirty";
$d_conferencing_kicked_path = $g['varrun_path'] . "/conferencing.kicked";
$d_managerconfdirty_path = $g['varrun_path'] . "/managerconf.dirty";
$d_rtpconfdirty_path = $g['varrun_path'] . "/rtpconf.dirty";
$d_packageconfdirty_path = $g['varrun_path'] . "/packageconf.dirty";
$d_ultmpmounted_path = $g['varrun_path'] . "/ultmp.mounted";
$d_sysrebootreqd_path = $g['varrun_path'] . "/sysreboot.reqd";
$d_passworddirty_path = $g['varrun_path'] . "/password.dirty";

$applications = array(
	"Echo" => gettext("Echos audio and video back to the caller as soon as it is received. Used to test connection delay."),
	"Milliwatt" => gettext("Generates a constant 1000Hz tone. Used to test connection quality."),
	"WakeMe" => gettext("Wake-Up call manager. Call to set and cancel wake-up calls on the current extension.")
);

$vm_email_languages = array(
	"en-informal" => gettext("English"),
	"en-formal" => gettext("English (formal)"),
	"bg-informal" => gettext("Bulgarian"),
	"bg-formal" => gettext("Bulgarian (formal)"),
	"cs-informal" => gettext("Czech"),
	"cs-formal" => gettext("Czech (formal)"),
	"da-informal" => gettext("Danish"),
	"da-formal" => gettext("Danish (formal)"),
	"nl-informal" => gettext("Dutch"),
	"nl-formal" => gettext("Dutch (formal)"),
	"fi-informal" => gettext("Finnish"),
	"fi-formal" => gettext("Finnish (formal)"),
	"fr-informal" => gettext("French"),
	"fr-formal" => gettext("French (formal)"),
	"de-informal" => gettext("German"),
	"de-formal" => gettext("German (formal)"),
	"gr-informal" => gettext("Greek"),
	"gr-formal" => gettext("Greek (formal)"),
	"it-informal" => gettext("Italian"),
	"it-formal" => gettext("Italian (formal)"),
	"jp-formal" => gettext("Japanese (formal)"),
	"pl-informal" => gettext("Polish"),
	"pl-formal" => gettext("Polish (formal)"),
	"pt-br-informal" => gettext("Portuguese, Brazil"),
	"pt-br-formal" => gettext("Portuguese, Brazil (formal)"),
	"ru-informal" => gettext("Russian"),
	"ru-formal" => gettext("Russian (formal)"),
	"es-formal" => gettext("Spanish (formal)"),
	"se-informal" => gettext("Swedish"),
	"tr-informal" => gettext("Turkish"),
	"tr-formal" => gettext("Turkish (formal)")
);

$isdn_dchannel_modes = array(
	0 => "",
	"DRVR_DSS1_TE" => gettext("point-to-multipoint, terminal equipment"),
	"DRVR_DSS1_NT" => gettext("multipoint-to-point, network termination"),
	"DRVR_DSS1_P2P_TE" => gettext("point-to-point, terminal equipment"),
	"DRVR_DSS1_P2P_NT" => gettext("point-to-point, network termination")
//	"DRVR_D_CHANNEL" => "default D-channel driver",
//	"DRVR_CAPI_TE" => "",
//	"DRVR_DIEHL_TE" => "",
//	"DRVR_TINA_DD_TE" => "",
//	"DRVR_AMV_B1_TE" => "",
//	"DRVR_D64S" => "This driver can be used on single B-channel leased lines.",
//	"DRVR_DUMMY" => "",
);

$system_tonezones = array(
	"us" => gettext("United States / North America"),
	"au" => gettext("Australia"),
	"at" => gettext("Austria"),
	"be" => gettext("Belgium"),
	"br" => gettext("Brazil"),
	"bg" => gettext("Bulgaria"),
	"cl" => gettext("Chile"),
	"cn" => gettext("China"),
	"cz" => gettext("Czech Republic"),
	"dk" => gettext("Denmark"),
	"ee" => gettext("Estonia"),
	"fi" => gettext("Finland"),
	"fr" => gettext("France"),
	"de" => gettext("Germany"),
	"gr" => gettext("Greece"),
	"hu" => gettext("Hungary"),
	"in" => gettext("India"),
	"il" => gettext("Israel"),
	"it" => gettext("Italy"),
	"jp" => gettext("Japan"),
	"lt" => gettext("Lithuania"),
	"my" => gettext("Malaysia"),
	"mx" => gettext("Mexico"),
	"nl" => gettext("Netherlands"),
	"nz" => gettext("New Zealand"),
	"no" => gettext("Norway"),
	"ph" => gettext("Philippines"),
	"pl" => gettext("Poland"),
	"pt" => gettext("Portugal"),
	"ru" => gettext("Russia / ex Soviet Union"),
	"sg" => gettext("Singapore"),
	"za" => gettext("South Africa"),
	"es" => gettext("Spain"),
	"se" => gettext("Sweden"),
	"ch" => gettext("Switzerland"),
	"tw" => gettext("Taiwan"),
	"th" => gettext("Thailand"),
	"uk" => gettext("United Kingdom"),
	"us-old" => gettext("United States Circa 1950/ North America"),
	"ve" => gettext("Venezuela / South America")
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

	$returnvar = mwexec("/bin/gunzip -t " . escapeshellarg($fname));
	if ($returnvar != 0)
		return 0;
	else
		return 1;
}

function get_std_save_message($ok) {
	global $d_sysrebootreqd_path;

	if ($ok == 0) {
		if (file_exists($d_sysrebootreqd_path))
			return gettext("The changes have been saved. You must <a href=\"/system_reboot.php\">reboot</a> your pbx for the changes to take effect.");
		else
			return gettext("The changes have been applied successfully.");
	} else {
		return sprintf(gettext("ERROR: the changes could not be applied (error code %s)."), $ok);
	}
}

function dynamic_tab_menu(&$tabs) {
	global $allowed, $config;

	if ($_SERVER['REMOTE_USER'] == $config['system']['username']) {
		$authorized = &$tabs;
	} else {
		$authorized = &$allowed;
	}

	foreach ($tabs as $desc => $link) {
		if (in_array($link,$authorized)) {
			if ($link == basename($_SERVER['SCRIPT_FILENAME'])) {
				echo '<li class="tabact">'.spanify($desc).'</li>';
			} else {
				echo '<li class="tabinact"><a href="'.$link.'">'.spanify($desc).'</a></li>';
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

function spanify($string) {
	return '<span class="spannit">' . $string . '</span>';
}

?>
