#!/usr/bin/php
<?php 
/*
	$Id: index.php 1104 2009-09-03 14:54:33Z michael.iedema $
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

$pgtitle_omit = true;

//require("guiconfig.inc");

$menu = array(
	"System" => array(
		"General Setup",
		"Packages",
		"Firmware",
		"Backup/Restore",
		"Reboot"
	),
	"Interfaces" => array(
		"Network",
		"ISDN",
		"Analog",
		"Storage"
	),
	"Providers" => array(
		"SIP",
		"IAX",
		"ISDN",
		"Analog"
	),
	"Phones" => array(
		"SIP",
		"IAX",
		"ISDN",
		"Analog",
		"External"
	),
	"Dialplan" => array(
		"Applications",
		"Call Groups",
		"Transfers"
	),
	"Services" => array(
		"Conferencing",
		"Voicemail"
	),
	"Status" => array(
		"Summary",
		"Interfaces",
		"Channels",
		"Conferences"
	),
	"Live Stats" => array(
		"Network Traffic",
		"CPU Load"
	),
	"Diagnostics" => array(
		"Logs",
		"Ping/Traceroute",
		"ARP Table",
		"Manager Interface"
	),
	"Advanced" => array(
		"RTP",
		"SIP",
		"IAX",
		"ISDN",
		"Analog",
		"Manager",
		"GUI Options"
	)
);

function generate_screens($menu) {
	$depth = 0;
	foreach ($menu as $title => $entries) {
		$mainscreen[] = $title;
		generate_screen($title, $entries);
	}
	generate_screen("home", $mainscreen);
}

function generate_screen($name, $list) {

	if ($name == "home") {
		print("<ul id=\"home\" title=\"AskoziaPBX\" selected=\"true\">\n");
	} else {
		print("<ul id=\"" . $name . "\" title=\"" . $name . "\">\n");
	}

	foreach ($list as $l) {
		print("\t<li><a href=\"#" . $l . "\">" . $l . "</a></li>\n");
	}

	print("</ul>\n");
}

?><!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
         "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">

<html xmlns="http://www.w3.org/1999/xhtml">
	<head>
		<title>AskoziaPBX</title>
		<meta name="viewport" content="width=device-width; initial-scale=1.0; maximum-scale=1.0; user-scalable=0;"/>
		<link rel="apple-touch-icon" href="iui/iui-logo-touch-icon.png" />
		<meta name="apple-touch-fullscreen" content="YES" />
		<style type="text/css" media="screen">@import "iui/iui.css";</style>
		<script type="application/x-javascript" src="iui/iui.js"></script>
		<script type="text/javascript">
			iui.animOn = true;
		</script>
</head>

<body>
	<div class="toolbar">
		<h1 id="pageTitle"></h1>
		<a id="backButton" class="button" href="#"></a>
		<a class="button" href="#searchForm">Search</a>
	</div>

	<?=generate_screens($menu);?>

</body>
</html>
