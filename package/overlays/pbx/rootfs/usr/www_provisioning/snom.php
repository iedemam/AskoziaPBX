#!/usr/bin/php
<?php 
/*
	$Id: index.php 1316 2010-01-08 04:24:39Z michael.iedema $
	part of AskoziaPBX (http://askozia.com/pbx)

	Copyright (C) 2010 tecema (a.k.a IKT) <http://www.tecema.de>.
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

require_once("functions.inc");


openlog("snom.php", LOG_INFO, LOG_LOCAL0);

$for_mac = $_GET['mac'];
$from_mac = exec("arp -n -D " . $_SERVER['REMOTE_ADDR'] . " | cut -d \" \" -f4 | sed -e \"s/://g\"");

/* security check on incoming mac against arp table */
if (strcasecmp($from_mac, $for_mac) != 0) {
	syslog(LOG_WARNING, "Configuration for MAC (" . $for_mac . ") requested by MAC (" . $from_mac . ")! " .
		"Configuration not sent.");
	closelog();
	exit();
}

$laninfo = network_get_interface($config['interfaces']['lan']['if']);

$phones = sip_get_phones();
foreach ($phones as $p) {
	if (isset($p['disabled'])) {
		continue;
	}
	if ($p['snom-mac'] == $for_mac) {
		$conf  = "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
		$conf .= "<settings>\n";
		$conf .= "  <phone-settings e=\"2\">\n";
		$conf .= "    <update_policy perm=\"\">auto_update</update_policy>\n";
		$conf .= "    <http_user perm=\"\">" . $p['extension'] . "</http_user>\n";
		$conf .= "    <http_pass perm=\"\">" . $p['secret'] . "</http_pass>\n";
		$conf .= "    <admin_mode_password perm=\"\">" . $p['extension'] . "</admin_mode_password>\n";
		$conf .= "    <admin_mode perm=\"\">on</admin_mode>\n";
		$conf .= "    <active_line perm=\"\">1</active_line>\n";
		$conf .= "    <outgoing_identity perm=\"\">1</outgoing_identity>\n";
		$conf .= "    <user_name idx=\"1\" perm=\"\">" . $p['extension'] . "</user_name>\n";
		$conf .= "    <user_host idx=\"1\" perm=\"\">" . $laninfo['ipaddr'] . "</user_host>\n";
		$conf .= "    <user_pass idx=\"1\" perm=\"\">" . $p['secret'] . "</user_pass>\n";
		$conf .= "  </phone-settings>\n";
		$conf .= "</settings>\n";
	}
}

if ($conf) {
	header ("content-type: text/xml");
	echo $conf;
	syslog(LOG_INFO, "Requested MAC (" . $for_mac . ") is present in system. Configuration sent.");
} else {
	syslog(LOG_INFO, "Requested MAC (" . $for_mac . ") not found in system. Configuration not sent.");
}

closelog();
