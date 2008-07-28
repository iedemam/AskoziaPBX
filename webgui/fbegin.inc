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

$navlevelsep = ": ";	/* navigation level separator string */
$mandfldhtml = "";		/* display this before mandatory input fields */
$mandfldhtmlspc = "";	/* same as above, but with spacing */

//locale stuff
//try to acquire the language 
$current_language =  $config['system']['webgui']['language'];
// default to English if nothing is still set
if (!$current_language) {
	$current_language = "en_US";
}

bindtextdomain("messages", "./locale");
bind_textdomain_codeset("messages","utf-8");
//setlocale(LC_ALL, $lang);
setlocale(LC_MESSAGES, $current_language);
putenv("LANG=$current_language");
textdomain("messages");
// end locale stuff

function gentitle($title) {
	global $navlevelsep;
	//return join($navlevelsep, $title);
	foreach ($title as $item) {
        	echo gettext($item);
		if(next($title)) {
			echo ": ";
		}
	}
}

function genhtmltitle($title) {
	global $config;
	//return $config['system']['hostname'] . "." . $config['system']['domain'] . " - " . gentitle($title);
	echo $config['system']['hostname'] . "." . $config['system']['domain'] . " - ";
	gentitle($title);
}

function dynamic_menu(&$menu, $menu_title) {
	global $allowed, $config;

	// Make sure the admin can see everything
	if ($_SERVER['REMOTE_USER'] === $config['system']['username']) {
		unset($allowed);
   		foreach ($menu as $item) {
        			$allowed[] = $item[0];
		}
	}
	
	foreach ($menu as $desc => $links) {
		if ($intersect = array_intersect($menu[$desc],$allowed)) {
			
			if (!isset($header_displayed) && ($menu_title == 'Dialplan')) {
				echo "\n" 
				?>
				<strong><?=gettext("Dialplan");?></strong>&nbsp;<a href="printable_dialplan.php" class="navlnks">(<?=gettext("print");?>)</a><br>
				<?
				$header_displayed = true;
			
			} else if (!isset($header_displayed) && !($menu_title == 'Diagnostics') && !($menu_title == 'Advanced')) {
				echo "\n			<strong>".$menu_title.'</strong><br>';
				$header_displayed = true;
			
			} else { 
				if (!isset($header_displayed)) {
					if ($menu_title == 'Diagnostics') {
						if (isset($config['system']['webgui']['expanddiags']) || strstr($_SERVER['SCRIPT_FILENAME'], "diag_")) :?>
							<a href="javascript:showhide('diag','tri_diag')"><img src="tri_o.gif" id="tri_diag" width="14" height="10" border="0"></a><strong><a href="javascript:showhide('diag','tri_diag')" class="navlnk"><?=gettext("Diagnostics");?></a></strong><br><span id="diag">
						<? else :?>
							<a href="javascript:showhide('diag','tri_diag')"><img src="tri_c.gif" id="tri_diag" width="14" height="10" border="0"></a><strong><a href="javascript:showhide('diag','tri_diag')" class="navlnk"><?=gettext("Diagnostics");?></a></strong><br><span id="diag" style="display: none">
						<? endif;
					}
					if ($menu_title == 'Advanced') {
						if (isset($config['system']['webgui']['expandadvanced']) || strstr($_SERVER['SCRIPT_FILENAME'], "advanced_")) :?>
							<a href="javascript:showhide('advanced','tri_advanced')"><img src="tri_o.gif" id="tri_advanced" width="14" height="10" border="0"></a><strong><a href="javascript:showhide('advanced','tri_advanced')" class="navlnk"><?=gettext("Advanced");?></a></strong><br><span id="advanced">
						<? else :?>
							<a href="javascript:showhide('advanced','tri_advanced')"><img src="tri_c.gif" id="tri_advanced" width="14" height="10" border="0"></a><strong><a href="javascript:showhide('advanced','tri_advanced')" class="navlnk"><?=gettext("Advanced");?></a></strong><br><span id="advanced" style="display: none">
						<? endif;
					}
					
					$header_displayed = true;			  
				}
				
			}
			// $intersect may contain one or more links, but we only need the first one
			// BUT, we are doing a foreach here because it might not be element [0]
			foreach ($intersect as $link) {
				if ((isset($config['system']['webgui']['hidesip']) && $desc == "SIP") ||
					(isset($config['system']['webgui']['hideiax']) && $desc == "IAX") ||
					(isset($config['system']['webgui']['hideisdn']) && $desc == "ISDN") ||
					(isset($config['system']['webgui']['hideanalog']) && $desc == "Analog")) {
					
					break;	
				}
				 
				// Below lines are spaced out oddly to help improve the look of resulting HTML code.
				?>

				&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href="<?=$link?>" class="navlnk"><?=$desc?></a><br><?
	   			// Only need the first one, so break out of foreach
	   			break;
			}
		}
	}
	if ($menu_title == 'Advanced' || $menu_title == 'Diagnostics') {
		?></span><?
	}
}
        
// Check for issues when changing the web admin username
// $allowed is set in auth.inc *IF* the user is not the webGUI admin
// so, if it's not set here, they were the admin user when they submited the admin username change.
if ((!is_array($allowed)) and 
    ($_SERVER['PHP_SELF'] == '/system.php') and 
    ($_SERVER['REMOTE_USER'] != $config['system']['username'])) {
	// webGUI username was just changed, let us change what PHP sees the remote user as so
	// the menu will build properly.  When user hits next link, they will be prompted to reauthenticate.
	$_SERVER['REMOTE_USER'] = $config['system']['username'];	
   
}

/* System */
$menu['System'][gettext("General Setup")]		= array('system.php');
$menu['System'][gettext("Interfaces")] 			= array('interfaces_network.php',
												'interfaces_isdn.php',
												'interfaces_isdn_edit.php',
												'interfaces_analog.php',
												'interfaces_analog_edit.php',
												'interfaces_storage.php',
												'interfaces_storage_syspart_edit.php',
												'interfaces_storage_disk_edit.php');
$menu['System'][gettext("Packages")]			= array('system_packages.php');
$menu['System'][gettext("Firmware")]			= array('system_firmware.php');
$menu['System'][gettext("Backup/Restore")] 		= array('system_backup.php');
$menu['System'][gettext("Factory Defaults")]	= array('system_defaults.php');
$menu['System'][gettext("Reboot")] 				= array('system_reboot.php');

/* Accounts */
$menu['Accounts'][gettext("Providers")]			= array('accounts_providers.php',
												'providers_sip_edit.php',
												'providers_iax_edit.php',
												'providers_isdn_edit.php',
												'providers_analog_edit.php');
$menu['Accounts'][gettext("Phones")]			= array('accounts_phones.php',
												'phones_sip_edit.php',
												'phones_iax_edit.php',
												'phones_isdn_edit.php',
												'phones_analog_edit.php',
												'phones_external_edit.php');

/* Dialplan */
$menu['Dialplan'][gettext("Applications")]		= array('dialplan_applications.php',
												'dialplan_applications_edit.php');
$menu['Dialplan'][gettext("Call Groups")]		= array('dialplan_callgroups.php',
												'dialplan_callgroups_edit.php');
$menu['Dialplan'][gettext("Transfers")]			= array('dialplan_transfers.php');
											
/* Services */
$menu['Services'][gettext("Conferencing")]		= array('services_conferencing.php',
												'services_conferencing_edit.php');
$menu['Services'][gettext("Voicemail")]			= array('services_voicemail.php');

/* Packages */
if ($nav = packages_get_www_nav()) {
	$menu['Packages'] = $nav;
}

/* Status */
$menu['Status'][gettext("Summary")]				= array('index.php');
$menu['Status'][gettext("Interfaces")]			= array('status_interfaces.php');
$menu['Status'][gettext("Channels")]				= array('status_channels.php');
$menu['Status'][gettext("Conferences")]			= array('status_conferences.php');

/* Live Stats */
$menu['Live Stats'][gettext("Network Traffic")]	= array('status_graph_traffic.php');
$menu['Live Stats'][gettext("CPU Load")]			= array('status_graph_cpu.php');

/* Advanced */
$menu['Advanced'][gettext("RTP")] 				= array('advanced_rtp.php');
$menu['Advanced'][gettext("SIP")] 				= array('advanced_sip.php');
$menu['Advanced'][gettext("IAX")] 				= array('advanced_iax.php');
$menu['Advanced'][gettext("ISDN")] 				= array('advanced_isdn.php');
$menu['Advanced'][gettext("Analog")] 			= array('advanced_analog.php');
$menu['Advanced'][gettext("Manager")] 			= array('advanced_manager.php',
												'advanced_manager_edit.php');
$menu['Advanced'][gettext("GUI Options")]		= array('advanced_guioptions.php');

/* Diagnostics */
$menu['Diagnostics'][gettext("Logs")]			= array('diag_logs.php',
												'diag_logs_pbx.php',
												'diag_logs_calls.php',
							  					'diag_logs_settings.php');
$menu['Diagnostics'][gettext('Ping/Traceroute')] = array('diag_ping.php',
												'diag_traceroute.php');
$menu['Diagnostics'][gettext("ARP Table")] 		= array('diag_arp.php');
$menu['Diagnostics'][gettext("Manager Interface")]= array('diag_manager.php');

?>

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<title><?=genhtmltitle($pgtitle);?></title>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
<?php if ($needs_scriptaculous) :?>
<script src="prototype.js" type="text/javascript"></script>
<script src="scriptaculous.js?load=effects,dragdrop" type="text/javascript"></script>
<?php endif; ?>
<script src="jquery.js" type="text/javascript"></script>
<script src="jquery.blockUI.js" type="text/javascript"></script>
<script src="jquery.selectboxes.js" type="text/javascript"></script>
<script src="jquery.preloadImage.js" type="text/javascript"></script>
<script src="jquery.progressbar.js" type="text/javascript"></script>
<link href="gui.css" rel="stylesheet" type="text/css">
</head>

<body link="#0000CC" vlink="#0000CC" alink="#0000CC">
<script type="text/JavaScript">
<!--
	jQuery.noConflict();

	jQuery(document).ready(function(){

		jQuery.preloadImages(['/tri_o.gif', '/tri_c.gif']);

	});

function showhide(tspan, tri) {
	tspanel = document.getElementById(tspan);
	triel = document.getElementById(tri);
	if (tspanel.style.display == 'none') {
		jQuery("#" + tspan).slideDown("slow");
		triel.src = "tri_o.gif";
	} else {
		jQuery("#" + tspan).slideUp("slow");
		triel.src = "tri_c.gif";
	}
}
-->
</script>
<table width="750" border="0" cellspacing="0" cellpadding="2">
  <tr valign="bottom"> 
    <td width="150" height="65" align="center" valign="middle"> <strong><a href="http://askozia.com/pbx" target="_blank"><img src="logo.gif" width="150" height="47" border="0"></a></strong></td>
    <td height="65" bgcolor="#435370">
	<table border="0" cellspacing="0" cellpadding="0" width="100%">
	<tr><td align="left" valign="bottom"><span class="tfrtitle">&nbsp;<?=gettext("webGUI Configuration");?></span></td>
	  <td align="right" valign="bottom">
	  <span class="hostname"><?=$config['system']['hostname'] . "." . $config['system']['domain'];?>&nbsp;</span>
	  </td></tr></table>
	</td>
  </tr>
  <tr valign="top"> 
    <td width="150" bgcolor="#9D9D9D">
	<table width="100%" border="0" cellpadding="6" cellspacing="0">
        <tr>
          <td><span class="navlnk"><font color="#FFFFFF"> 
            <?php 
                   
dynamic_menu($menu['System'], gettext('System'));
dynamic_menu($menu['Accounts'], gettext('Accounts'));
dynamic_menu($menu['Dialplan'], 'Dialplan');
dynamic_menu($menu['Services'], gettext('Services'));
if (isset($menu['Packages'])) {
	dynamic_menu($menu['Packages'], gettext('Packages'));
}
dynamic_menu($menu['Status'], gettext('Status'));
dynamic_menu($menu['Live Stats'], gettext('Live Stats'));
dynamic_menu($menu['Advanced'], 'Advanced');
dynamic_menu($menu['Diagnostics'], 'Diagnostics');

?>
			  </font></span>
            </td>
        </tr></table></td>
    <td width="600"><table width="100%" border="0" cellpadding="10" cellspacing="0">
        <tr><td>
<?php if (!$pgtitle_omit): ?>
      <p class="pgtitle"><?=gentitle($pgtitle);?></p>
<?php endif; ?>

