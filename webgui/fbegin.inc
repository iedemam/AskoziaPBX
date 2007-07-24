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

function gentitle($title) {
	global $navlevelsep;
	return join($navlevelsep, $title);
}

function genhtmltitle($title) {
	global $config;
	return $config['system']['hostname'] . "." . $config['system']['domain'] . " - " . gentitle($title);
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
			if (!isset($header_displayed) && !($menu_title=='Diagnostics')) {
				echo "\n			<strong>".$menu_title.'</strong><br>';
				$header_displayed = true;
			}
			else { 
				if (!isset($header_displayed)) {
					if (isset($config['system']['webgui']['expanddiags']) || strstr($_SERVER['SCRIPT_FILENAME'], "diag_")): ?>
					<a href="javascript:showhide('diag','tri_diag')"><img src="/tri_o.gif" id="tri_diag" width="14" height="10" border="0"></a><strong><a href="javascript:showhide('diag','tri_diag')" class="navlnk">Diagnostics</a></strong><br>
					<span id="diag">
					<?php else: ?>
					<a href="javascript:showhide('diag','tri_diag')"><img src="/tri_c.gif" id="tri_diag" width="14" height="10" border="0"></a><strong><a href="javascript:showhide('diag','tri_diag')" class="navlnk">Diagnostics</a></strong><br>
					<span id="diag" style="display: none">
					<?php endif;
					$header_displayed = true;			  
				}
				
			}
			// $intersect may contain one or more links, but we only need the first one
			// BUT, we are doing a foreach here because it might not be element [0]
			foreach ($intersect as $link) { 
				// Below lines are spaced out oddly to help improve the look of resulting HTML code.
				?>

				&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href="/<?=$link?>" class="navlnk"><?=$desc?></a><br><?
	   			// Only need the first one, so break out of foreach
	   			break;
			}
		}
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
$menu['System']['General Setup']		= array('system.php');
$menu['System']['Firmware']				= array('system_firmware.php');
$menu['System']['Backup/Restore'] 		= array('system_backup.php');
$menu['System']['Factory Defaults']		= array('system_defaults.php');
$menu['System']['Reboot'] 				= array('system_reboot.php');

/* Interfaces */
$menu['Interfaces']['Network'] 			= array('interfaces_network.php',
												'interfaces_network_assign.php');
$menu['Interfaces']['ISDN'] 			= array('interfaces_isdn.php');

/* Providers */
$menu['Providers']['SIP'] 				= array('providers_sip.php');
$menu['Providers']['IAX'] 				= array('providers_iax.php');
$menu['Providers']['ISDN'] 				= array('providers_isdn.php');

/* Phones */
$menu['Phones']['SIP'] 					= array('phones_sip.php');
$menu['Phones']['IAX'] 					= array('phones_iax.php');

/* Dialplan */
$menu['Dialplan']['Providers']			= array('dialplan_providers.php');
$menu['Dialplan']['Call Groups']		= array('dialplan_callgroups.php');
$menu['Dialplan']['Call Parking']		= array('dialplan_callparking.php');
											
/* Services */
$menu['Services']['Conferencing']		= array('services_conferencing.php');
$menu['Services']['Voicemail']			= array('services_voicemail.php');

/* Status */
$menu['Status']['Summary']				= array('index.php');
$menu['Status']['Interfaces']			= array('status_interfaces.php');
$menu['Status']['Channels']				= array('status_channels.php');
//$menu['Status']['Providers']				= array('status_providers.php');
//$menu['Status']['Phones']					= array('status_phones.php');
$menu['Status']['Conferences']			= array('status_conferences.php');
//$menu['Status']['Voicemail']				= array('status_voicemail.php');

/* Live Stats */
$menu['Live Stats']['Network Traffic']	= array('status_graph_traffic.php');
$menu['Live Stats']['CPU Load']			= array('status_graph_cpu.php');

/* Advanced */
$menu['Advanced']['SIP'] 				= array('advanced_sip.php');
$menu['Advanced']['IAX'] 				= array('advanced_iax.php');
$menu['Advanced']['ISDN'] 				= array('advanced_isdn.php');
$menu['Advanced']['GUI Options']		= array('advanced_guioptions.php');
//$menu['Advanced']['Quality of Service'] 					= array('advanced_qos.php');

/* Diagnostics */
$menu['Diagnostics']['Logs']			= array('diag_logs.php',
												'diag_logs_pbx.php',
												'diag_logs_calls.php',
							  					'diag_logs_settings.php');
$menu['Diagnostics']['Ping/Traceroute'] = array('diag_ping.php',
												'diag_traceroute.php');
$menu['Diagnostics']['ARP Table'] 		= array('diag_arp.php');
$menu['Diagnostics']['Manager Interface']= array('diag_manager.php');

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
<link href="gui.css" rel="stylesheet" type="text/css">
</head>

<body link="#0000CC" vlink="#0000CC" alink="#0000CC">
<script language="javascript">
<!--
var tri_open = "";
var tri_closed = "";

window.onload = preload;

function preload() {
	if (document.images) {
		tri_open = new Image(14,10);
		tri_closed = new Image(14,10);
		tri_open.src = "/tri_o.gif";
		tri_closed.src = "/tri_c.gif";
	}
}

function showhide(tspan, tri) {
	tspanel = document.getElementById(tspan);
	triel = document.getElementById(tri);
	if (tspanel.style.display == 'none') {
		tspanel.style.display = '';
		triel.src = "/tri_o.gif";
	} else {
		tspanel.style.display = 'none';
		triel.src = "/tri_c.gif";
	}
}
-->
</script>
<table width="750" border="0" cellspacing="0" cellpadding="2">
  <tr valign="bottom"> 
    <td width="150" height="65" align="center" valign="middle"> <strong><a href="http://askozia.com/pbx" target="_blank"><img src="/logo.gif" width="150" height="47" border="0"></a></strong></td>
    <td height="65" bgcolor="#435370">
	<table border="0" cellspacing="0" cellpadding="0" width="100%">
	<tr><td align="left" valign="bottom"><span class="tfrtitle">&nbsp;webGUI 
      Configuration</span></td>
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
                   
dynamic_menu($menu['System'], 'System');
dynamic_menu($menu['Interfaces'], 'Interfaces');
dynamic_menu($menu['Providers'], 'Providers');
dynamic_menu($menu['Phones'], 'Phones');
dynamic_menu($menu['Dialplan'], 'Dialplan');
dynamic_menu($menu['Services'], 'Services');
dynamic_menu($menu['Status'], 'Status');
dynamic_menu($menu['Live Stats'], 'Live Stats');
dynamic_menu($menu['Advanced'], 'Advanced');
dynamic_menu($menu['Diagnostics'], 'Diagnostics');

?>
			  </span>
			  </font></span>
            </td>
        </tr></table></td>
    <td width="600"><table width="100%" border="0" cellpadding="10" cellspacing="0">
        <tr><td>
<?php if (!$pgtitle_omit): ?>
      <p class="pgtitle"><?=gentitle($pgtitle);?></p>
<?php endif; ?>

