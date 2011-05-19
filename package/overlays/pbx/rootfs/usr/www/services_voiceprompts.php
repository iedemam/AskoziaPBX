#!/usr/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2010-2011 tecema (a.k.a IKT) <http://www.tecema.de>.
	All rights reserved.
	
	Askozia®PBX is a registered trademark of tecema. Any unauthorized use of
	this trademark is prohibited by law and international treaties.
	
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
	
	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.
	
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	
	3. Redistribution in any form at a charge, that in whole or in part
	   contains or is derived from the software, including but not limited to
	   value added products, is prohibited without prior written consent of
	   tecema.
	
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

require("guiconfig.inc");
$pgtitle = array(gettext("Services"), gettext("Voice Prompts"));


$disk = storage_service_is_active("media");
$promptsdir = $disk['mountpoint'] . "/askoziapbx/media/sounds";
$tmpdir = $disk['mountpoint'] . "/askoziapbx/tmp";

if ($_GET['startdownload']) {
	$url = $_GET['startdownload'];
	$file = basename($url);
	exec("rm -rf " . $tmpdir . "/*");
	$size = util_get_remote_filesize($url);
	if (is_numeric($size) && $size > 0) {
		echo $size;
		if (file_exists($tmpdir . "/" . $file)) {
			unlink($tmpdir . "/" . $file);
		}
		exec("nohup wget -q -O " . $tmpdir . "/" . $file . " " . $url  . " > /dev/null 2>&1 &");
	} else {
		echo "error";
	}

	exit;
}

if ($_GET['startextract']) {
	$file = $_GET['startextract'];
	if (strpos($file, ".tar.gz") !== false) {
		$decompressionflag = "z";
	} else if (strpos($file, ".tar.bz2") !== false) {
		$decompressionflag = "j";
	}
	$count = exec("tar -t" . $decompressionflag . " -f " . $tmpdir . "/" . $file . " | wc -l");
	if (is_numeric($count)) {
		echo $count;
		if (file_exists($tmpdir . "/extract")) {
			exec("rm -rf " . $tmpdir . "/extract");
		}
		exec("mkdir " . $tmpdir . "/extract");
		exec("nohup tar -x" . $decompressionflag . " -f ". $tmpdir . "/" . $file . " -C " . $tmpdir . "/extract > /dev/null 2>&1 &");
	} else {
		echo "error";
	}

	exit;
}

if ($_GET['startinstall']) {
	$language_code = $_GET['startinstall'];
	$count = exec("find " . $tmpdir . "/extract | wc -l");

	if (is_numeric($count)) {
		echo $count;
		exec("touch " . $tmpdir . "/copystart");
		exec("nohup cp -fR " . $tmpdir . "/extract/* " . $promptsdir . "/" . $language_code . "/ > /dev/null 2>&1 &");
	} else {
		echo "error";
	}

	exit;
}


function _generate_digium_formats_array($language, $formats) {
	$url = "http://downloads.asterisk.org/pub/telephony/sounds/releases";
	$version = "1.4.21";

	$format_array = array();
	foreach ($formats as $format) {
		$format_array[$format] = array(
			"url" => $url . "/" . "asterisk-core-sounds-" . $language . "-" . $format . "-" . $version . ".tar.gz",
			"version" => $version,
			"install" => "install_digium_prompts.sh"
		);
	}

	return $format_array;
}

$languages = array(
	"en-us" => array(
		"name" => gettext("English (US)"),
		"formats" => _generate_digium_formats_array("en",
			array("alaw", "g722", "g729", "gsm", "siren7", "siren14", "sln16", "ulaw", "wav")
		)
	),
	//"en-gb" => array(
	//	"name" => gettext("English (UK)"),
	//	"formats" => array(
	//		"ulaw" => array(
	//			"url" => "http://www.enicomms.com/cutglassivr/audiofiles/Alison_Keenan-British-English-ulaw.tar.gz",
	//			"version" => "unknown",
	//			"install" => "install_cutglass_prompts.sh"
	//		),
	//		"alaw" => array(
	//			"url" => "http://www.enicomms.com/cutglassivr/audiofiles/Alison_Keenan-British-English-alaw.tar.gz",
	//			"version" => "unknown",
	//			"install" => "install_cutglass_prompts.sh"
	//		),
	//		"g729" => array(
	//			"url" => "http://www.enicomms.com/cutglassivr/audiofiles/Alison_Keenan-British-English-g729.tar.gz",
	//			"version" => "unknown",
	//			"install" => "install_cutglass_prompts.sh"
	//		),
	//		"g723" => array(
	//			"url" => "http://www.enicomms.com/cutglassivr/audiofiles/Alison_Keenan-British-English-g723.tar.gz",
	//			"version" => "unknown",
	//			"install" => "install_cutglass_prompts.sh"
	//		),
	//		"wav" => array(
	//			"url" => "http://www.enicomms.com/cutglassivr/audiofiles/Alison_Keenan-British-English-44kwav.tar.gz",
	//			"version" => "unknown",
	//			"install" => "install_cutglass_prompts.sh"
	//		)
	//	)
	//),
	"fr-ca" => array(
		"name" => gettext("French (Canada)"),
		"formats" => _generate_digium_formats_array("fr",
			array("alaw", "g722", "g729", "gsm", "siren7", "siren14", "sln16", "ulaw", "wav")
		)
	),
	//),
	//"de-de" => array(
	//	"name" => gettext("German"),
	//	"formats" => array(
	//		"wav" => array(
	//			"url" => "http://downloads.askozia.com/t2/amooma-wav-r25.tar.bz2",
	//			"version" => "r25",
	//			"install" => "install_amooma_prompts.sh"
	//		)
	//	)
	//),
	//"it-it" => array(
	//	"name" => gettext("Italian"),
	//	"formats" => array(
	//		"gsm" => array(
	//			"url" => "http://mirror.tomato.it/ftp/pub/asterisk/suoni_ita/it_mm_sounds_20060510.tar.gz",
	//			"version" => "20060510",
	//			"install" => "install_tomato_prompts.sh"
	//		)
	//	)
	//),
	//"es-es" => array(
	//	"name" => gettext("Spanish (Spain)"),
	//	"formats" => array(
	//		"ulaw" => array(
	//			"url" => "http://www.voipnovatos.es/voces/voipnovatos-core-sounds-es-ulaw-1.4.tar.gz",
	//			"version" => "unknown",
	//			"install" => "install_voipnovatos_prompts.sh"
	//		)
	//	)
	//)
	"ja_JP" => array(
		"name" => gettext("Japanese"),
		"formats" => _generate_digium_formats_array("ja",
			array("ulaw", "g722", "g729", "gsm", "siren7", "siren14", "sln16", "alaw", "wav")
		)
    ),
	"ru-ru" => array(
		"name" => gettext("Russia"),
		"formats" => _generate_digium_formats_array("ru",
			array("alaw", "g722", "g729", "gsm", "siren7", "siren14", "sln16", "ulaw", "wav")
		)
	)

);


function _install_set($languagecode, $format) {
	global $promptsdir;


}

function _uninstall_set($languagecode, $format) {
	global $promptsdir;

	unlink($promptsdir . "/" . $languagecode . "/" . $format . ".installed");
}

function _get_installed_sets($languagecode) {
	global $promptsdir;

	$formats = array();
	$globber = glob($promptsdir . "/" . $languagecode . "/*.installed");
	$matches = array();
	foreach ($globber as $match) {
		$matches[] = basename($match, ".installed");
	}
	return $matches;
}

function _installed($languagecode) {
	global $languages;

	$installed_sets = _get_installed_sets($languagecode);
	$list = array();
	foreach ($installed_sets as $set) {
		//$list[] = '<a href="?action=uninstall' .
		//			'&language=' . $languagecode .
		//			'&format=' . $set . '">' . $set . '</a>';
		$list[] = $set;
	}

	return implode(", ", $list);
}

function _installable($languagecode) {
	global $languages;

	$possible_sets = array_keys($languages[$languagecode]['formats']);
	$installed_sets = _get_installed_sets($languagecode);

	$list = array();
	foreach ($possible_sets as $set) {
		if (in_array($set, $installed_sets)) {
			continue;
		}
		$list[] = '<a href="?action=add' .
					'&language=' . $languagecode .
					'&format=' . $set . '">' . $set . '</a>';
	}

	return implode(", ", $list);
}


$colspan=1;

if (isset($_GET['installed'])) {
	$savemsg = sprintf(gettext("Successfully installed %s formatted prompts for %s."), $_GET['format'], $_GET['language']);
}

include("fbegin.inc");
if ($_GET['action'] == "add") {
?><script type="text/JavaScript">

	var total_ticks;
	var current_ticks;
	var percentage;
	var oldpercentage;
	var stalled;
	var tmpdir = "<?=$tmpdir;?>";
	var promptsdir = "<?=$promptsdir;?>";
	var language_code = "<?=$_GET['language'];?>";
	var format = "<?=$_GET['format'];?>";
	var url = "<?=$languages[$_GET['language']]['formats'][$_GET['format']]['url'];?>";
	var file = url.replace(/.*\//, "");

	function start_download() {
		percentage = 0;
		oldpercentage = 0;
		total_ticks = 0;
		current_ticks = 0;
		stalled = 0;

		jQuery("#downloadprogress").progressBar({ boxImage: 'progressbar.gif', barImage: 'progressbg_black.gif', showText: false} );
		jQuery("#extractprogress").progressBar({ boxImage: 'progressbar.gif', barImage: 'progressbg_black.gif', showText: false} );
		jQuery("#installprogress").progressBar({ boxImage: 'progressbar.gif', barImage: 'progressbg_black.gif', showText: false} );

		jQuery.get("?startdownload=" + url, function(data) {
			if (data == "error") {
				alert("download start failed!");
			} else {
				total_ticks = parseInt(data);
				setTimeout("update_download_progress()", 1000);
			}
		});
	}

	function update_download_progress() {
		jQuery.get("cgi-bin/ajax.cgi?exec_shell=ls%20-l%20" + tmpdir + "/" + file, function(data) {
			var pieces = data.split(/\s+/);
			if (!pieces[4]) {
				alert("download update failed!");
			} else {
				current_ticks = parseInt(pieces[4]);
				oldpercentage = percentage;
				percentage = Math.floor(100 * current_ticks / total_ticks);
				if (percentage == oldpercentage) {
					stalled++;
				}
				jQuery("#downloadprogress").progressBar(percentage);
				if (percentage == 100 || stalled == 5) {
					start_extract();
				} else {
					setTimeout("update_download_progress()", 1000);
				}
			}
		});
	}

	function start_extract() {
		percentage = 0;
		oldpercentage = 0;
		total_ticks = 0;
		current_ticks = 0;
		stalled = 0;

		jQuery.get("?startextract=" + file, function(data) {
			if (data == "error") {
				alert("extract start failed!");
			} else {
				total_ticks = parseInt(data);
				setTimeout("update_extract_progress()", 1000);
			}
		});
	}

	function update_extract_progress() {
		jQuery.get("cgi-bin/ajax.cgi?exec_shell=find%20" + tmpdir + "/extract%20|%20wc%20-l", function(data) {
			if (!data) {
				alert("extract update failed!");
			} else {
				current_ticks = parseInt(data);
				oldpercentage = percentage;
				percentage = Math.floor(100 * current_ticks / total_ticks);
				if (percentage == oldpercentage) {
					stalled++;
				}
				jQuery("#extractprogress").progressBar(percentage);
				if (percentage == 100 || stalled == 5) {
					start_install();
				} else {
					setTimeout("update_extract_progress()", 1000);
				}
			}
		});
	}

	function start_install() {
		percentage = 0;
		oldpercentage = 0;
		total_ticks = 0;
		current_ticks = 0;
		stalled = 0;

		jQuery.get("?startinstall=" + language_code, function(data) {
			if (data == "error") {
				alert("install start failed!");
			} else {
				total_ticks = parseInt(data);
				setTimeout("update_install_progress()", 1000);
			}
		});
	}

	function update_install_progress() {
		jQuery.get("cgi-bin/ajax.cgi?exec_shell=find%20" + promptsdir + "/" + language_code + "%20-newer%20" + tmpdir + "/copystart%20|%20wc%20-l", function(data) {
			if (!data) {
				alert("extract update failed!");
			} else {
				current_ticks = parseInt(data);
				oldpercentage = percentage;
				percentage = Math.floor(100 * current_ticks / total_ticks);
				if (percentage == oldpercentage) {
					stalled++;
				}
				jQuery("#installprogress").progressBar(percentage);
				if (percentage == 100 || stalled == 5) {
					mark_installed()
				} else {
					setTimeout("update_install_progress()", 1000);
				}
			}
		});
	}

	function mark_installed() {
		jQuery.get("cgi-bin/ajax.cgi?exec_shell=touch%20" + promptsdir + "/" + language_code + "/" + format + ".installed", function(data) {
			window.location = "?installed&language=" + language_code + "&format=" + format;
		});
	}


	jQuery(document).ready(function(){
		start_download();
	});


</script><?
} // end conditional 'add' javascript functionality
d_start();

if ($_GET['action'] == "add") {
	d_header(gettext("Adding") . ": " . $languages[$_GET['language']]['name'] . ", " . $_GET['format']);
	d_blanklabel(
		"1. " . gettext("downloading"),
		'<span id="downloadprogress"></span>&nbsp;'
	);
	d_blanklabel(
		"2. " . gettext("extracting"),
		'<span id="extractprogress"></span>&nbsp;'
	);
	d_blanklabel(
		"3. " . gettext("installing"),
		'<span id="installprogress"></span>&nbsp;'
	);

} else {
	
	?> <table width="100%" border="0" cellpadding="0" cellspacing="0"> <?
	
	display_media_tab_menu();
	
	?><tr>
		<td class="tabcont">
			<table width="100%" border="0" cellpadding="6" cellspacing="0"><?
	
	foreach ($languages as $code => $info) {
		d_header($info["name"]);
		d_blanklabel(
			"<strong>" . gettext("installed sets") . "</strong>",
			_installed($code) . "&nbsp;"
		);
		d_blanklabel(
			gettext("add set"),
			_installable($code) . "&nbsp;"
		);
		d_spacer();
	}
	
		?></table>
		</td>
	</tr>
</table><?
}

d_scripts();
include("fend.inc");
