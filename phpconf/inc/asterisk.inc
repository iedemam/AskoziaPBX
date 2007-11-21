<?php
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007 IKT <http://itison-ikt.de>.
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

/* include all configuration functions */
require_once("functions.inc");

/**
 * Generates all Asterisk configuration files and (re)starts the Asterisk process
 */
function asterisk_configure() {
	global $config, $g;

	if ($g['booting']) {
		echo "Starting Asterisk...\n";
	} else {
		killbypid("{$g['varrun_path']}/asterisk.pid");
	}

	
	$res = 0;
	if ($g['booting']) echo " - generating indications configuration...";
	$res |= asterisk_indications_conf_generate();
	if ($g['booting']) echo "done\n";
	
	if ($g['booting']) echo " - generating manager configuration...";
	$res |= asterisk_manager_conf_generate();
	if ($g['booting']) echo "done\n";
	
	if ($g['booting']) echo " - generating rtp configuration...";
	$res |= asterisk_rtp_conf_generate();
	if ($g['booting']) echo "done\n";
	
	if ($g['booting']) echo " - generating sip configuration...";
	$res |= sip_conf_generate();
	if ($g['booting']) echo "done\n";
	
	if ($g['booting']) echo " - generating iax configuration...";
	$res |= iax_conf_generate();
	if ($g['booting']) echo "done\n";
	
	if ($g['booting']) echo " - generating conferencing configuration...";
	$res |= conferencing_conf_generate();
	if ($g['booting']) echo "done\n";
	
	if ($g['booting']) echo " - generating dialplan...";
	$res |= extensions_conf_generate();
	$res |= dialplan_features_conf_generate();
	if ($g['booting']) echo "done\n";
	
	if ($g['booting']) echo " - generating voicemail configuration...";
	$res |= voicemail_conf_generate();
	$res |= msmtp_conf_generate();
	if ($g['booting']) echo "done\n";
	
	if ($g['booting']) echo " - generating isdn configuration...";
	$res |= isdn_conf_generate();
	if ($g['booting']) echo "done\n";
	
	if ($g['booting']) echo " - applying isdn configuration...";
	$res |= isdn_configure();
	if ($g['booting']) echo "done\n";
	
	if ($g['booting']) echo " - generating analog configuration...";
	$res |= analog_conf_generate();
	if ($g['booting']) echo "done\n";
	
	if ($g['booting']) echo " - applying analog configuration...";
	$res |= analog_configure();
	if ($g['booting']) echo "done\n";
	
	if ($g['booting']) echo " - configuring timing source...";
	$res |= asterisk_configure_timing();
	if ($g['booting']) echo "done\n";
	
	if ($g['booting']) echo " - executing Asterisk...";
	$res |= mwexec("/usr/local/sbin/asterisk");
	if ($g['booting']) echo "done\n";

	if ($g['booting']) {
		if ($res == 0)
			echo "done\n";
		else
			echo "failed\n";
	}

	return $res;
}

/**
 * Loads an appropriate combination of kernel modules to provide a timing source
 * depending on what hardware is available.
 */
function asterisk_configure_timing() {

	/* zaptel modules are all loaded at boot, 
	if nothing was detected we fall back to ztdummy for timing */
	if(!count(analog_get_recognized_ab_unit_numbers())) {
		mwexec("/sbin/kldunload wcfxo");
		mwexec("/sbin/kldunload wcfxs");
		//mwexec("/sbin/kldunload wct1xxp");
		//mwexec("/sbin/kldunload wct4xxp");
		mwexec("/sbin/kldload ztdummy");
	}
	return 0;
}

/**
 * Generates rtp.conf which contains the RTP port range Asterisk uses
 */
function asterisk_rtp_conf_generate() {
	global $config;
	
	$fd = fopen("/usr/local/etc/asterisk/rtp.conf", "w");
	if (!$fd) {
		printf("Error: cannot open rtp.conf in asterisk_rtp_conf_generate().\n");
		return 1;
	}
	
	$rtpconfig = $config['services']['rtp'];

	$lowport = isset($rtpconfig['lowport']) ? $rtpconfig['lowport'] : "10000";
	$highport = isset($rtpconfig['highport']) ? $rtpconfig['highport'] : "20000";
	
	$conf = "[general]\n";
	$conf .= "rtpstart=$lowport\n";
	$conf .= "rtpend=$highport\n";
	$conf .= "\n";
	
	fwrite($fd, $conf);
	fclose($fd);

	return 0;
}

/**
 * Sorts the defined Asterisk manager users by username.
 */
function asterisk_sort_manager_users() {
	global $config;

	usort($config['services']['manager']['manager-user'], "_a_sortmanagerusers");
}

/* XXX : replace with "sort_by_username_field" */
function _a_sortmanagerusers($a, $b) {
	return strcmp($a['username'], $b['username']);
}

/**
 * Generates manager.conf which contains extra AMI users defined in the GUI
 */
function asterisk_manager_conf_generate() {
	global $config;
	
	$fd = fopen("/usr/local/etc/asterisk/manager.conf", "w");
	if (!$fd) {
		printf("Error: cannot open manager.conf in asterisk_manager_conf_generate().\n");
		return 1;
	}
	
	$managerconfig = $config['services']['manager'];

	$conf = <<<EOD

[general]
displaysystemname = yes
enabled = yes
port = 5038
bindaddr = 0.0.0.0

[admin]
secret = askozia
deny=0.0.0.0/0.0.0.0
permit=127.0.0.1/255.255.255.255
read = system,call,log,verbose,command,agent,user,config
write = system,call,log,verbose,command,agent,user,config



EOD;

	// XXX : replace with a function, no more direct $config access
	if (is_array($config['services']['manager']['manager-user'])) {
		foreach ($config['services']['manager']['manager-user'] as $user) {
			$conf .= "[" . $user['username'] . "]" . "\n";
			$conf .= "secret = " . $user['secret'] . "\n";
			$conf .= "deny=" . $user['denyip'] . "/" . $user['denynetmask'] . "\n";
			$conf .= "permit=" . $user['permitip'] . "/" . $user['permitnetmask'] . "\n";
			if (is_array($user['read-permission'])) {
				$conf .= "read = " . implode(",", $user['read-permission']) . "\n";
			}
			if (is_array($user['write-permission'])) {
				$conf .= "write = " . implode(",", $user['write-permission']) . "\n";
			}
			$conf .= "\n";
		}
	}
	$conf .= "\n";
	
	fwrite($fd, $conf);
	fclose($fd);

	return 0;
}

/**
 * Generates indications.conf which contains regional tone information and sets this
 * installation's tonezone country
 */
function asterisk_indications_conf_generate() {
	global $config;
	
	$fd = fopen("/usr/local/etc/asterisk/indications.conf", "w");
	if (!$fd) {
		printf("Error: cannot open indications.conf in asterisk_indications_conf_generate().\n");
		return 1;
	}
	
	$conf = "[general]\n";
	
	if ($config['system']['tonezone']) {
		$conf .= "country=". $config['system']['tonezone'] ."\n\n";
	} else {
		$conf .= "country=us\n\n";
	}
	
	$conf .= <<<EOD
	
[at]
description = Austria
ringcadence = 1000,5000
; Reference: http://www.itu.int/ITU-T/inr/forms/files/tones-0203.pdf
dial = 420
busy = 420/400,0/400
ring = 420/1000,0/5000
congestion = 420/200,0/200
callwaiting = 420/40,0/1960
dialrecall = 420
; RECORDTONE - not specified
record = 1400/80,0/14920
info = 950/330,1450/330,1850/330,0/1000
stutter = 380+420

[au]
description = Australia
; Reference http://www.acif.org.au/__data/page/3303/S002_2001.pdf
; Normal Ring
ringcadence = 400,200,400,2000
; Distinctive Ring 1 - Forwarded Calls
; 400,400,200,200,400,1400
; Distinctive Ring 2 - Selective Ring 2 + Operator + Recall
; 400,400,200,2000
; Distinctive Ring 3 - Multiple Subscriber Number 1
; 200,200,400,2200
; Distinctive Ring 4 - Selective Ring 1 + Centrex
; 400,2600
; Distinctive Ring 5 - Selective Ring 3
; 400,400,200,400,200,1400
; Distinctive Ring 6 - Multiple Subscriber Number 2
; 200,400,200,200,400,1600
; Distinctive Ring 7 - Multiple Subscriber Number 3 + Data Privacy
; 200,400,200,400,200,1600
; Tones
dial = 413+438
busy = 425/375,0/375
ring = 413+438/400,0/200,413+438/400,0/2000
; XXX Congestion: Should reduce by 10 db every other cadence XXX
congestion = 425/375,0/375,420/375,0/375
callwaiting = 425/200,0/200,425/200,0/4400
dialrecall = 413+438
; Record tone used for Call Intrusion/Recording or Conference
record = !425/1000,!0/15000,425/360,0/15000
info = 425/2500,0/500
; Other Australian Tones
; The STD "pips" indicate the call is not an untimed local call
std = !525/100,!0/100,!525/100,!0/100,!525/100,!0/100,!525/100,!0/100,!525/100
; Facility confirmation tone (eg. Call Forward Activated)
facility = 425
; Message Waiting "stutter" dialtone
stutter = 413+438/100,0/40
; Ringtone for calls to Telstra mobiles
ringmobile = 400+450/400,0/200,400+450/400,0/2000

[bg]
; Reference: http://www.itu.int/ITU-T/inr/forms/files/tones-0203.pdf
description = Bulgaria
ringdance = 1000,4000
;
dial = 425
busy = 425/500,0/500
ring = 425/1000,0/4000
congestion = 425/250,0/250
callwaiting = 425/150,0/150,425/150,0/4000
dialrecall = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425
record = 1400/425,0/15000
info = 950/330,1400/330,1800/330,0/1000
stutter = 425/1500,0/100

[br]
description = Brazil
ringcadence = 1000,4000
dial = 425
busy = 425/250,0/250
ring = 425/1000,0/4000
congestion = 425/250,0/250,425/750,0/250
callwaiting = 425/50,0/1000
; Dialrecall not used in Brazil standard (using UK standard)
dialrecall = 350+440
; Record tone is not used in Brazil, use busy tone
record = 425/250,0/250
; Info not used in Brazil standard (using UK standard)
info = 950/330,1400/330,1800/330
stutter = 350+440

[be]
description = Belgium
; Reference: http://www.itu.int/ITU-T/inr/forms/files/tones-0203.pdf
ringcadence = 1000,3000
dial = 425
busy = 425/500,0/500
ring = 425/1000,0/3000
congestion = 425/167,0/167
callwaiting = 1400/175,0/175,1400/175,0/3500
; DIALRECALL - not specified
dialrecall = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440"
; RECORDTONE - not specified
record = 1400/500,0/15000
info = 900/330,1400/330,1800/330,0/1000
stutter = 425/1000,0/250

[ch]
description = Switzerland
; Reference: http://www.itu.int/ITU-T/inr/forms/files/tones-0203.pdf
ringcadence = 1000,4000
dial = 425
busy = 425/500,0/500
ring = 425/1000,0/4000
congestion = 425/200,0/200
callwaiting = 425/200,0/200,425/200,0/4000
; DIALRECALL - not specified
dialrecall = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425
; RECORDTONE - not specified
record = 1400/80,0/15000
info = 950/330,1400/330,1800/330,0/1000
stutter = 425+340/1100,0/1100

[cl]
description = Chile
; According to specs from Telefonica CTC Chile
ringcadence = 1000,3000
dial = 400
busy = 400/500,0/500
ring = 400/1000,0/3000
congestion = 400/200,0/200
callwaiting = 400/250,0/8750
dialrecall = !400/100,!0/100,!400/100,!0/100,!400/100,!0/100,400
record = 1400/500,0/15000
info = 950/333,1400/333,1800/333,0/1000
stutter = !400/100,!0/100,!400/100,!0/100,!400/100,!0/100,!400/100,!0/100,!400/100,!0/100,!400/100,!0/100,400

[cn]
description = China
; Reference: http://www.itu.int/ITU-T/inr/forms/files/tones-0203.pdf
ringcadence = 1000,4000
dial = 450
busy = 450/350,0/350
ring = 450/1000,0/4000
congestion = 450/700,0/700
callwaiting = 450/400,0/4000
dialrecall = 450
record = 950/400,0/10000
info = 450/100,0/100,450/100,0/100,450/100,0/100,450/400,0/400
; STUTTER - not specified
stutter = 450+425

[cz]
description = Czech Republic
; Reference: http://www.itu.int/ITU-T/inr/forms/files/tones-0203.pdf
ringcadence = 1000,4000
dial = 425/330,0/330,425/660,0/660
busy = 425/330,0/330
ring = 425/1000,0/4000
congestion = 425/165,0/165
callwaiting = 425/330,0/9000
; DIALRECALL - not specified
dialrecall = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425/330,0/330,425/660,0/660
; RECORDTONE - not specified
record = 1400/500,0/14000
info = 950/330,0/30,1400/330,0/30,1800/330,0/1000
; STUTTER - not specified
stutter = 425/450,0/50

[de]
description = Germany
; Reference: http://www.itu.int/ITU-T/inr/forms/files/tones-0203.pdf
ringcadence = 1000,4000
dial = 425
busy = 425/480,0/480
ring = 425/1000,0/4000
congestion = 425/240,0/240
callwaiting = !425/200,!0/200,!425/200,!0/5000,!425/200,!0/200,!425/200,!0/5000,!425/200,!0/200,!425/200,!0/5000,!425/200,!0/200,!425/200,!0/5000,!425/200,!0/200,!425/200,0
; DIALRECALL - not specified
dialrecall = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425
; RECORDTONE - not specified
record = 1400/80,0/15000
info = 950/330,1400/330,1800/330,0/1000
stutter = 425+400

[dk]
description = Denmark
; Reference: http://www.itu.int/ITU-T/inr/forms/files/tones-0203.pdf
ringcadence = 1000,4000
dial = 425
busy = 425/500,0/500
ring = 425/1000,0/4000
congestion = 425/200,0/200
callwaiting = !425/200,!0/600,!425/200,!0/3000,!425/200,!0/200,!425/200,0
; DIALRECALL - not specified
dialrecall = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425
; RECORDTONE - not specified
record = 1400/80,0/15000
info = 950/330,1400/330,1800/330,0/1000
; STUTTER - not specified
stutter = 425/450,0/50

[ee]
description = Estonia
; Reference: http://www.itu.int/ITU-T/inr/forms/files/tones-0203.pdf
ringcadence = 1000,4000
dial = 425
busy = 425/300,0/300
ring = 425/1000,0/4000
congestion = 425/200,0/200
; CALLWAIT not in accordance to ITU
callwaiting = 950/650,0/325,950/325,0/30,1400/1300,0/2600
; DIALRECALL - not specified
dialrecall = 425/650,0/25
; RECORDTONE - not specified
record = 1400/500,0/15000
; INFO not in accordance to ITU
info = 950/650,0/325,950/325,0/30,1400/1300,0/2600
; STUTTER not specified
stutter = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425

[es]
description = Spain
ringcadence = 1500,3000
dial = 425
busy = 425/200,0/200
ring = 425/1500,0/3000
congestion = 425/200,0/200,425/200,0/200,425/200,0/600
callwaiting = 425/175,0/175,425/175,0/3500
dialrecall = !425/200,!0/200,!425/200,!0/200,!425/200,!0/200,425
record = 1400/500,0/15000
info = 950/330,0/1000
dialout = 500


[fi]
description = Finland
ringcadence = 1000,4000
dial = 425
busy = 425/300,0/300
ring = 425/1000,0/4000
congestion = 425/200,0/200
callwaiting = 425/150,0/150,425/150,0/8000
dialrecall = 425/650,0/25
record = 1400/500,0/15000
info = 950/650,0/325,950/325,0/30,1400/1300,0/2600
stutter = 425/650,0/25

[fr]
description = France
; Reference: http://www.itu.int/ITU-T/inr/forms/files/tones-0203.pdf
ringcadence = 1500,3500
; Dialtone can also be 440+330
dial = 440
busy = 440/500,0/500
ring = 440/1500,0/3500
; CONGESTION - not specified
congestion = 440/250,0/250
callwait = 440/300,0/10000
; DIALRECALL - not specified
dialrecall = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440
; RECORDTONE - not specified
record = 1400/500,0/15000
info = !950/330,!1400/330,!1800/330
stutter = !440/100,!0/100,!440/100,!0/100,!440/100,!0/100,!440/100,!0/100,!440/100,!0/100,!440/100,!0/100,440

[gr]
description = Greece
ringcadence = 1000,4000
dial = 425/200,0/300,425/700,0/800
busy = 425/300,0/300
ring = 425/1000,0/4000
congestion = 425/200,0/200
callwaiting = 425/150,0/150,425/150,0/8000
dialrecall = 425/650,0/25
record = 1400/400,0/15000
info = !950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,0
stutter = 425/650,0/25

[hu]
description = Hungary
; Reference: http://www.itu.int/ITU-T/inr/forms/files/tones-0203.pdf
ringcadence = 1250,3750
dial = 425
busy = 425/300,0/300
ring = 425/1250,0/3750
congestion = 425/300,0/300
callwaiting = 425/40,0/1960
dialrecall = 425+450
; RECORDTONE - not specified
record = 1400/400,0/15000
info = !950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,0
stutter = 350+375+400

[il]
description = Israel
ringcadence = 1000,3000
dial = 414
busy = 414/500,0/500
ring = 414/1000,0/3000
congestion = 414/250,0/250
callwaiting = 414/100,0/100,414/100,0/100,414/600,0/3000
dialrecall = !414/100,!0/100,!414/100,!0/100,!414/100,!0/100,414
record = 1400/500,0/15000
info = 1000/330,1400/330,1800/330,0/1000
stutter = !414/160,!0/160,!414/160,!0/160,!414/160,!0/160,!414/160,!0/160,!414/160,!0/160,!414/160,!0/160,!414/160,!0/160,!414/160,!0/160,!414/160,!0/160,!414/160,!0/160,414


[in]
description = India
ringcadence = 400,200,400,2000
dial = 400*25
busy = 400/750,0/750
ring = 400*25/400,0/200,400*25/400,0/2000
congestion = 400/250,0/250
callwaiting = 400/200,0/100,400/200,0/7500
dialrecall = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440
record = 1400/500,0/15000
info = !950/330,!1400/330,!1800/330,0/1000
stutter = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440

[it]
description = Italy
; Reference: http://www.itu.int/ITU-T/inr/forms/files/tones-0203.pdf
ringcadence = 1000,4000
dial = 425/200,0/200,425/600,0/1000
busy = 425/500,0/500
ring = 425/1000,0/4000
congestion = 425/200,0/200
callwaiting = 425/400,0/100,425/250,0/100,425/150,0/14000
dialrecall = 470/400,425/400
record = 1400/400,0/15000
info = !950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,0
stutter = 470/400,425/400

[lt]
description = Lithuania
ringcadence = 1000,4000
dial = 425
busy = 425/350,0/350
ring = 425/1000,0/4000
congestion = 425/200,0/200
callwaiting = 425/150,0/150,425/150,0/4000
; DIALRECALL - not specified
dialrecall = 425/500,0/50
; RECORDTONE - not specified
record = 1400/500,0/15000
info = !950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,0
; STUTTER - not specified
stutter = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425

[jp]
description = Japan
ringcadence = 1000,2000
dial = 400
busy = 400/500,0/500
ring = 400+15/1000,0/2000
congestion = 400/500,0/500
callwaiting = 400+16/500,0/8000
dialrecall = !400/200,!0/200,!400/200,!0/200,!400/200,!0/200,400
record = 1400/500,0/15000
info = !950/330,!1400/330,!1800/330,0
stutter = !400/100,!0/100,!400/100,!0/100,!400/100,!0/100,!400/100,!0/100,!400/100,!0/100,!400/100,!0/100,400

[mx]
description = Mexico
ringcadence = 2000,4000
dial = 425
busy = 425/250,0/250
ring = 425/1000,0/4000
congestion = 425/250,0/250
callwaiting = 425/200,0/600,425/200,0/10000
dialrecall = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440
record = 1400/500,0/15000
info = 950/330,0/30,1400/330,0/30,1800/330,0/1000
stutter = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440

[my]
description = Malaysia
ringcadence = 2000,4000
dial = 425
busy = 425/500,0/500
ring = 425/400,0/200
congestion = 425/500,0/500

[nl]
description = Netherlands
; Reference: http://www.itu.int/ITU-T/inr/forms/files/tones-0203.pdf
ringcadence = 1000,4000
; Most of these 425's can also be 450's
dial = 425
busy = 425/500,0/500
ring = 425/1000,0/4000
congestion = 425/250,0/250
callwaiting = 425/500,0/9500
; DIALRECALL - not specified
dialrecall = 425/500,0/50
; RECORDTONE - not specified
record = 1400/500,0/15000
info = 950/330,1400/330,1800/330,0/1000
stutter = 425/500,0/50

[no]
description = Norway
ringcadence = 1000,4000
dial = 425
busy = 425/500,0/500
ring = 425/1000,0/4000
congestion = 425/200,0/200
callwaiting = 425/200,0/600,425/200,0/10000
dialrecall = 470/400,425/400
record = 1400/400,0/15000
info = !950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,0
stutter = 470/400,425/400

[nz]
description = New Zealand
;NOTE - the ITU has different tonesets for NZ, but according to some residents there,
;      this is, indeed, the correct way to do it.
ringcadence = 400,200,400,2000
dial = 400
busy = 400/250,0/250
ring = 400+450/400,0/200,400+450/400,0/2000
congestion = 400/375,0/375
callwaiting = !400/200,!0/3000,!400/200,!0/3000,!400/200,!0/3000,!400/200
dialrecall = !400/100!0/100,!400/100,!0/100,!400/100,!0/100,400
record = 1400/425,0/15000
info = 400/750,0/100,400/750,0/100,400/750,0/100,400/750,0/400
stutter = !400/100!0/100,!400/100,!0/100,!400/100,!0/100,!400/100!0/100,!400/100,!0/100,!400/100,!0/100,400
unobtainable = 400/75,0/100,400/75,0/100,400/75,0/100,400/75,0/400

[ph]

; reference http://www.itu.int/ITU-T/inr/forms/files/tones-0203.pdf

description = Philippines
ringcadence = 1000,4000
dial = 425
busy = 480+620/500,0/500
ring = 425+480/1000,0/4000
congestion = 480+620/250,0/250
callwaiting = 440/300,0/10000
; DIALRECALL - not specified
dialrecall = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440
; RECORDTONE - not specified
record = 1400/500,0/15000
; INFO - not specified
info = !950/330,!1400/330,!1800/330,0
; STUTTER - not specified
stutter = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440


[pl]
description = Poland
ringcadence = 1000,4000
dial = 425
busy = 425/500,0/500
ring = 425/1000,0/4000
congestion = 425/500,0/500
callwaiting = 425/150,0/150,425/150,0/4000
; DIALRECALL - not specified
dialrecall = 425/500,0/50
; RECORDTONE - not specified
record = 1400/500,0/15000
; 950/1400/1800 3x0.33 on 1.0 off  repeated 3 times 
info = !950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000
; STUTTER - not specified
stutter = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425

[pt]
description = Portugal
ringcadence = 1000,5000
dial = 425
busy = 425/500,0/500
ring = 425/1000,0/5000
congestion = 425/200,0/200
callwaiting = 440/300,0/10000
dialrecall = 425/1000,0/200
record = 1400/500,0/15000
info = 950/330,1400/330,1800/330,0/1000
stutter = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425

[ru]
; References:
;	http://www.minsvyaz.ru/site.shtml?id=1806
;	http://www.aboutphone.info/lib/gost/45-223-2001.html
description = Russian Federation / ex Soviet Union
ringcadence = 1000,4000
dial = 425
busy = 425/350,0/350
ring = 425/1000,0/4000
congestion = 425/175,0/175
callwaiting = 425/200,0/5000
record = 1400/400,0/15000
info = 950/330,1400/330,1800/330,0/1000
dialrecall = 425/400,0/40
stutter = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425

[se]
description = Sweden
ringcadence = 1000,5000
dial = 425
busy = 425/250,0/250
ring = 425/1000,0/5000
congestion = 425/250,0/750
callwaiting = 425/200,0/500,425/200,0/9100
dialrecall = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425
record = 1400/500,0/15000
info = !950/332,!0/24,!1400/332,!0/24,!1800/332,!0/2024,!950/332,!0/24,!1400/332,!0/24,!1800/332,!0/2024,!950/332,!0/24,!1400/332,!0/24,!1800/332,!0/2024,!950/332,!0/24,!1400/332,!0/24,!1800/332,!0/2024,!950/332,!0/24,!1400/332,!0/24,!1800/332,0
stutter = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425
; stutter = 425/320,0/20        ; Real swedish standard, not used for now

[sg]
description = Singapore
; Singapore
; Reference: http://www.ida.gov.sg/idaweb/doc/download/I397/ida_ts_pstn1_i4r2.pdf 
; Frequency specs are:   425 Hz +/- 20Hz; 24 Hz +/- 2Hz; modulation depth 100%; SIT +/- 50Hz
ringcadence = 400,200,400,2000
dial        = 425
ring        = 425*24/400,0/200,425*24/400,0/2000     ; modulation should be 100%, not 90%
busy        = 425/750,0/750
congestion  = 425/250,0/250
callwaiting = 425*24/300,0/200,425*24/300,0/3200
stutter     = !425/200,!0/200,!425/600,!0/200,!425/200,!0/200,!425/600,!0/200,!425/200,!0/200,!425/600,!0/200,!425/200,!0/200,!425/600,!0/200,425
info        = 950/330,1400/330,1800/330,0/1000       ; not currently in use acc. to reference
dialrecall  = 425*24/500,0/500,425/500,0/2500        ; unspecified in IDA reference, use repeating Holding Tone A,B
record      = 1400/500,0/15000                       ; unspecified in IDA reference, use 0.5s tone every 15s
; additionally defined in reference
nutone      = 425/2500,0/500
intrusion   = 425/250,0/2000
warning     = 425/624,0/4376                         ; end of period tone, warning
acceptance  = 425/125,0/125
holdinga    = !425*24/500,!0/500                     ; followed by holdingb
holdingb    = !425/500,!0/2500

[th]
description = Thailand
ringcadence = 1000,4000
; Reference: http://www.itu.int/ITU-T/inr/forms/files/tones-0203.pdf
dial = 400*50
busy = 400/500,0/500
ring = 420/1000,0/5000
congestion = 400/300,0/300
callwaiting = 1000/400,10000/400,1000/400
; DIALRECALL - not specified - use special dial tone instead.
dialrecall = 400*50/400,0/100,400*50/400,0/100
; RECORDTONE - not specified
record = 1400/500,0/15000
; INFO - specified as an announcement - use special information tones instead
info = 950/330,1400/330,1800/330
; STUTTER - not specified
stutter = !400/200,!0/200,!400/600,!0/200,!400/200,!0/200,!400/600,!0/200,!400/200,!0/200,!400/600,!0/200,!400/200,!0/200,!400/600,!0/200,400

[uk]
description = United Kingdom
ringcadence = 400,200,400,2000
; These are the official tones taken from BT SIN350. The actual tones
; used by BT include some volume differences so sound slightly different
; from Asterisk-generated ones.
dial = 350+440
; Special dial is the intermittent dial tone heard when, for example,
; you have a divert active on the line
specialdial = 350+440/750,440/750
; Busy is also called "Engaged"
busy = 400/375,0/375
; "Congestion" is the Beep-bip engaged tone
congestion = 400/400,0/350,400/225,0/525
; "Special Congestion" is not used by BT very often if at all
specialcongestion = 400/200,1004/300
unobtainable = 400
ring = 400+450/400,0/200,400+450/400,0/2000
callwaiting = 400/100,0/4000
; BT seem to use "Special Call Waiting" rather than just "Call Waiting" tones
specialcallwaiting = 400/250,0/250,400/250,0/250,400/250,0/5000
; "Pips" used by BT on payphones. (Sounds wrong, but this is what BT claim it
; is and I've not used a payphone for years)
creditexpired = 400/125,0/125
; These two are used to confirm/reject service requests on exchanges that
; don't do voice announcements.
confirm = 1400
switching = 400/200,0/400,400/2000,0/400
; This is the three rising tones Doo-dah-dee "Special Information Tone",
; usually followed by the BT woman saying an appropriate message.
info = 950/330,0/15,1400/330,0/15,1800/330,0/1000
; Not listed in SIN350
record = 1400/500,0/60000
stutter = 350+440/750,440/750

[us]
description = United States / North America
ringcadence = 2000,4000
dial = 350+440
busy = 480+620/500,0/500
ring = 440+480/2000,0/4000
congestion = 480+620/250,0/250
callwaiting = 440/300,0/10000
dialrecall = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440
record = 1400/500,0/15000
info = !950/330,!1400/330,!1800/330,0
stutter = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440

[us-old]
description = United States Circa 1950/ North America
ringcadence = 2000,4000
dial = 600*120
busy = 500*100/500,0/500
ring = 420*40/2000,0/4000
congestion = 500*100/250,0/250
callwaiting = 440/300,0/10000
dialrecall = !600*120/100,!0/100,!600*120/100,!0/100,!600*120/100,!0/100,600*120
record = 1400/500,0/15000
info = !950/330,!1400/330,!1800/330,0
stutter = !600*120/100,!0/100,!600*120/100,!0/100,!600*120/100,!0/100,!600*120/100,!0/100,!600*120/100,!0/100,!600*120/100,!0/100,600*120

[tw]
description = Taiwan
; http://nemesis.lonestar.org/reference/telecom/signaling/dialtone.html
; http://nemesis.lonestar.org/reference/telecom/signaling/busy.html
; http://www.iproducts.com.tw/ee/kylink/06ky-1000a.htm
; http://www.pbx-manufacturer.com/ky120dx.htm
; http://www.nettwerked.net/tones.txt
; http://www.cisco.com/univercd/cc/td/doc/product/tel_pswt/vco_prod/taiw_sup/taiw2.htm
;
; busy tone 480+620Hz 0.5 sec. on ,0.5 sec. off
; reorder tone 480+620Hz 0.25 sec. on,0.25 sec. off
; ringing tone 440+480Hz 1 sec. on ,2 sec. off
;
ringcadence = 1000,4000
dial = 350+440
busy = 480+620/500,0/500
ring = 440+480/1000,0/2000
congestion = 480+620/250,0/250
callwaiting = 350+440/250,0/250,350+440/250,0/3250
dialrecall = 300/1500,0/500
record = 1400/500,0/15000
info = !950/330,!1400/330,!1800/330,0
stutter = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440

[ve]
; Tone definition source for ve found on 
; Reference: http://www.itu.int/ITU-T/inr/forms/files/tones-0203.pdf
description = Venezuela / South America
ringcadence = 1000,4000
dial = 425
busy = 425/500,0/500
ring = 425/1000,0/4000
congestion = 425/250,0/250
callwaiting = 400+450/300,0/6000
dialrecall = 425
record = 1400/500,0/15000
info = !950/330,!1440/330,!1800/330,0/1000


[za]
description = South Africa
; http://www.cisco.com/univercd/cc/td/doc/product/tel_pswt/vco_prod/safr_sup/saf02.htm
; (definitions for other countries can also be found there)
; Note, though, that South Africa uses two switch types in their network --
; Alcatel switches -- mainly in the Western Cape, and Siemens elsewhere.
; The former use 383+417 in dial, ringback etc.  The latter use 400*33
; I've provided both, uncomment the ones you prefer
ringcadence = 400,200,400,2000
; dial/ring/callwaiting for the Siemens switches:
dial = 400*33
ring = 400*33/400,0/200,400*33/400,0/2000
callwaiting = 400*33/250,0/250,400*33/250,0/250,400*33/250,0/250,400*33/250,0/250
; dial/ring/callwaiting for the Alcatel switches:
; dial = 383+417
; ring = 383+417/400,0/200,383+417/400,0/2000
; callwaiting = 383+417/250,0/250,383+417/250,0/250,383+417/250,0/250,383+417/250,0/250
congestion = 400/250,0/250
busy = 400/500,0/500
dialrecall = 350+440
; XXX Not sure about the RECORDTONE
record = 1400/500,0/10000
info = 950/330,1400/330,1800/330,0/330
stutter = !400*33/100,!0/100,!400*33/100,!0/100,!400*33/100,!0/100,!400*33/100,!0/100,!400*33/100,!0/100,!400*33/100,!0/100,400*33

EOD;

	fwrite($fd, $conf);
	fclose($fd);

	return 0;

}

/**
 * Reloads res_indications to apply newly generated configuration changes
 */
function asterisk_indications_reload() {
	return asterisk_exec("module reload res_indications.so");
}

/**
 * Executes a command using the AMI
 */
function asterisk_exec($cmd, $output=NULL) {

	$token = md5(uniqid(rand()));
	$errno = 0;
	$errstr = 0;
	$fp = fsockopen("localhost", 5038, &$errno, &$errstr, 20);
	if (!$fp) {
	  return 1;
	}
	
	fputs($fp, "Action: login\r\n");
	fputs($fp, "Username: admin\r\n");
	fputs($fp, "Secret: askozia\r\n");
	fputs($fp, "Events: off\r\n\r\n");
	usleep(500);
	
	fputs($fp, "Action: COMMAND\r\n");
	fputs($fp, "command: $cmd\r\n");
	fputs($fp, "ActionID: $token\r\n\r\n");
	usleep(500);
	
	$out = fread($fp, 38000); 
	while(strpos($out,"--END COMMAND--")==0) {
		$out .= fread($fp, 38000); 	  
	}
	fclose ($fp);
	
	$out = substr($out, strpos($out, "ActionID"));
	$out = substr($out, strpos($out, "\n") + 1);
	$out = substr($out, 0, strpos($out, "--END COMMAND--") - 1);

	// XXX : better command failure checking needed
	$output = $out;

	return 0; // XXX : this needs to return a value for SUCCESS/FAILURE
}

/**
 * Determines the validity of a given internal extension
 */
function asterisk_is_valid_extension($extension) {
	if(!is_numericint($extension) || (strlen($extension) < 1))
		return false;
	
	return true;
}

/**
 * Determines the validity of a given Caller ID
 * XXX : unimplemented
 */
function asterisk_is_valid_callerid($id) {
	return true;
}

/**
 * Determines the validity of a given Caller ID string
 * XXX : unimplemented
 */
function asterisk_is_valid_callerid_string($string) {
	return true;
}

/**
 * Determines the validity of a given secret
 * XXX : unimplemented
 */
function asterisk_is_valid_secret($secret) {
	return true;
}

/**
 * Determines the validity of a given username
 * XXX : unimplemented
 */
function asterisk_is_valid_username($username) {
	return true;
}

/**
 * Checks is the supplied dialpattern already exists.
 */
function asterisk_dialpattern_exists($dialpattern, &$return_provider_name, $sending_provider_id=false) {
	$allpatterns = asterisk_get_dialpatterns();

	if (is_array($allpatterns)) {
		foreach ($allpatterns as $provider_id => $patterns) {
			if ($provider_id == $sending_provider_id) {
				continue;
			}
			if (is_array($patterns) && in_array($dialpattern, $patterns)) {
				$return_provider_name = asterisk_uniqid_to_name($provider_id);
				return true;
			}
		}
	}
	
	return false;
}

/**
 * Determines the validity of a given dialpattern
 */
function asterisk_is_valid_dialpattern($dialpattern, &$error, $incoming=false) {
	
	$incoming_regex = "/^[XNZ0-9\-\[\]\.]+$/";
	$outgoing_regex = "/^[XNZ0-9\-\[\]\.\|\+\*\#]+$/";
	
	$invalid_regex = $incoming ? $incoming_regex : $outgoing_regex;
	
	// invalid character
	if (!preg_match($invalid_regex, $dialpattern)) {
		$error = "It contains an invalid character";
		return false;
	}
	
	// doubled character
	if (substr_count($dialpattern, "|") > 1) {
		$error = "It contains an doubled \"|\" character, only one is allowed.";
		return false;
	}
	if (substr_count($dialpattern, "+") > 1) {
		$error = "It contains an doubled \"+\" character, only one is allowed.";
		return false;
	}
	if (substr_count($dialpattern, "-") > 1) {
		$error = "It contains an doubled \"-\" character, only one is allowed.";
		return false;
	}
	
	// bracketing
	$len = strlen($dialpattern);
	$open = false;
	$open_index = 0;
	for($i = 0; $i < $len; $i++) {
		if ($dialpattern[$i] == "[") {
			if ($open) {
				$error = "Nested square brackets are not allowed.";
				return false;
			} else {
				$open = true;
				$open_index = $i;
			}
		} else if ($dialpattern[$i] == "]") {
			if (!$open) {
				$error = "Unmatched square bracket.";
				return false;
			} else if (($i - $open_index) < 3) {
				$error = "Brackets are not needed for less than 2 characters.";
				return false;
			} else {
				$open = false;
			}
		}
	}
	
	// invalid characters in brackets
	/* XXX : not working...
	if (preg_match("/\[[XNZ\.\|\+]+\]/", $dialpattern)) {
		$error = "An invalid character is contained within square brackets, only digits and hyphens are allowed.";
		return false;
	}
	*/
	
	// invalid appended prefix
	if (($plus_offset = strpos($dialpattern, "+")) !== false) {
		if (!preg_match("/^[0-9\*\#]+\+/", $dialpattern)) {
			$error = "Appended prefixes may only contain digits, '*' and '#'.";
			return false;
		}
		$dialpattern = substr($dialpattern, $plus_offset+1);
	}
	
	// invalid chopped prefix
	if (($pipe_offset = strpos($dialpattern, "|")) !== false) {
		if (strpos(substr($dialpattern, 0, $pipe_offset), ".") !== false) {
			$error = "The wildcard character \".\" is not allowed before a prefix which is to be removed.";
			return false;
		}
	}

	return true;
}

/**
 * Merges all *get_extensions() into a single array
 * XXX : should be reimplemented to have an optional "technology" parameter
 * so each of these sub _get_ functions can be generally implemented here
 */
function asterisk_get_extensions() {
	global $config;

	return array_merge(
		sip_get_extensions(), 
		iax_get_extensions(),
		isdn_get_extensions(),
		analog_get_extensions(),
		conferencing_get_extensions(),
		external_get_extensions(),
		dialplan_get_callgroup_extensions(),
		dialplan_get_application_extensions()
	);
}

/**
 * Merges all *get_providers() into a single array
 * XXX : should be reimplemented to have an optional "technology" parameter
 * so each of these sub _get_ functions can be generally implemented here
 */
function asterisk_get_providers() {
	global $config;
	
	return asterisk_sort_providers(
				array_merge(
					sip_get_providers(),
					iax_get_providers(),
					isdn_get_providers(),
					analog_get_providers()
				));
}

/**
 * Sorts providers by name
 */
function asterisk_sort_providers($providers) {
	usort($providers, "_a_sortproviders");

	return $providers;
}

/* XXX : replace with "sort_by_name_field" */
function _a_sortproviders($a, $b) {
	return strcmp($a['name'], $b['name']);
}

/**
 * Merges all *get_phones() into a single array
 * XXX : should be reimplemented to have an optional "technology" parameter
 * so each of these sub _get_ functions can be generally implemented here
 */
function asterisk_get_phones() {
	global $config;
	
	return asterisk_sort_phones(
				array_merge(
					sip_get_phones(),
					iax_get_phones(),
					isdn_get_phones(),
					analog_get_phones()
				));
}

/**
 * Sorts phones by Caller ID
 * XXX : this sort differs in behavior from the individual _sort_phones() functions
 */
function asterisk_sort_phones($phones) {
	usort($phones, "_a_sortphones");

	return $phones;
}

/* XXX : replace with "sort_by_callerid_field" */
function _a_sortphones($a, $b) {
	return strcmp($a['callerid'], $b['callerid']);
}

/**
 * Merges all *get_dialpatterns() into a single array
 * XXX : should be reimplemented to have an optional "technology" parameter
 * so each of these sub _get_ functions can be generally implemented here
 */
function asterisk_get_dialpatterns() {
	global $config;

	return array_merge(
		sip_get_dialpatterns(),
		iax_get_dialpatterns(),
		isdn_get_dialpatterns(),
		analog_get_dialpatterns()
	);
}

/**
 * Parses "core show channels" output to return active calls, channels and a channel list
 */
function asterisk_get_active_calls(&$active_calls, &$active_channels, &$channel_list) {
	
	asterisk_exec("core show channels", &$output);
	$lines = explode("\n", $output);

	$n = count($lines) - 3;
	for ($i = 1; $i <= $n; $i++) {
		$channel_list[] = $lines[$i];
	}
	$active_channels = substr($lines[$n+1], 0, strpos($lines[$n+1], " "));
	$active_calls = substr($lines[$n+2], 0, strpos($lines[$n+2], " "));

	return 0;
}

/**
 * Returns a dialstring based on the supplied uniqid and misc. options
 * XXX : needs to be made more granular
 * XXX : very clunky / hacky / junky
 */
function asterisk_uniqid_to_dial($exten, $uniqid, $wait=false, $override=false, $readbacknum=false) {
	global $config;
	
	$id = split("-", $uniqid);
	
	$dial = "exten => $exten,1,NoOp()\n";

	if ($wait) {
		$dial .= "exten => $exten,n,Ringing()\n";
		$dial .= "exten => $exten,n,Wait($wait)\n";
	}
	if ($override == "replacenamewithnum") {
		$dial .= "exten => $exten,n,Set(CALLERID(name)=$\{CALLERID(number)})\n";
	}
	
	if ($id[0] == "SIP" && $id[1] == "PHONE") {
		$phone = sip_get_phone($uniqid);
		$readbacknum = ($readbacknum) ? $readbacknum : $phone['extension'];
		$dial .= _extensions_add_sendnotifications_line($exten, $phone);
		$dial .= "exten => $exten,n,".
			"Macro(vm|SIP/{$phone['extension']}|{$phone['extension']}|$readbacknum|to)\n";
			
	} else if ($id[0] == "IAX" && $id[1] == "PHONE") {
		$phone = iax_get_phone($uniqid);
		$readbacknum = ($readbacknum) ? $readbacknum : $phone['extension'];
		$dial .= _extensions_add_sendnotifications_line($exten, $phone);
		$dial .= "exten => $exten,n,".
			"Macro(vm|IAX2/{$phone['extension']}|{$phone['extension']}|$readbacknum|to)\n";

	} else if ($id[0] == "ISDN" && $id[1] == "PHONE") {
		$phone = isdn_get_phone($uniqid);
		$readbacknum = ($readbacknum) ? $readbacknum : $phone['extension'];
		$dial .= _extensions_add_sendnotifications_line($exten, $phone);
		$dial .= "exten => $exten,n,".
			"Macro(vm|CAPI/{$phone['interface']}/{$phone['extension']}|{$phone['extension']}|$readbacknum|to)\n";

	} else if ($id[0] == "ANALOG" && $id[1] == "PHONE") {
		$phone = analog_get_phone($uniqid);
		$readbacknum = ($readbacknum) ? $readbacknum : $phone['extension'];
		$dial .= _extensions_add_sendnotifications_line($exten, $phone);
		$dial .= "exten => $exten,n,".
			"Macro(vm|ZAP/{$phone['interface']}/{$phone['extension']}|{$phone['extension']}|$readbacknum|to)\n";

	} else if ($id[0] == "CONFERENCE" && $id[1] == "ROOM") {
		$room = conferencing_get_room($uniqid);
		$dial .= "exten => $exten,n,Answer()\n";
		$dial .= "exten => $exten,n,MeetMe({$room['number']},M)\n";
		$dial .= "exten => $exten,n,Hangup()\n";
		
	} else if ($id[0] == "EXTERNAL" && $id[1] == "PHONE") {
		$phone = external_get_phone($uniqid);
		$readbacknum = ($readbacknum) ? $readbacknum : $phone['extension'];
		$technology = asterisk_uniqid_to_technology($phone['dialprovider']);
		$dial .= _extensions_add_sendnotifications_line($exten, $phone);
		if ($technology == "ZAP") {
			$zprov = analog_get_provider($phone['dialprovider']);
			$dial .= "exten => $exten,n,".
				"Macro(vm|$technology/{$zprov['interface']}/{$phone['dialstring']}|{$phone['extension']}|$readbacknum|to)\n";
		} else {
			$dial .= "exten => $exten,n,".
				"Macro(vm|$technology/{$phone['dialprovider']}/{$phone['dialstring']}|{$phone['extension']}|$readbacknum|to)\n";
		}

	} else if ($id[0] == "CALLGROUP" && $id[1] == "PARALLEL") {
		$group = dialplan_get_callgroup($uniqid);
		$dial_strings = array();
		$n = count($group['groupmember']);
		for ($i = 0; $i < $n; $i++) {
			$subid = split("-", $group['groupmember'][$i]);
			if ($subid[0] == "SIP" && $subid[1] == "PHONE") {
				$phone = sip_get_phone($group['groupmember'][$i]);
				if ($i == 0) {
					$vm_phone = $phone;
				}
				$dial_strings[] = "SIP/{$phone['extension']}";
        	
			} else if ($subid[0] == "IAX" && $subid[1] == "PHONE") {
				$phone = iax_get_phone($group['groupmember'][$i]);
				if ($i == 0) {
					$vm_phone = $phone;
				}
				$dial_strings[] = "IAX2/{$phone['extension']}";        	
			
			} else if ($subid[0] == "ISDN" && $subid[1] == "PHONE") {
				$phone = isdn_get_phone($group['groupmember'][$i]);
				if ($i == 0) {
					$vm_phone = $phone;
				}
				$dial_strings[] = "CAPI/{$phone['interface']}/{$phone['extension']}";
				
			} else if ($subid[0] == "ANALOG" && $subid[1] == "PHONE") {
				$phone = analog_get_phone($group['groupmember'][$i]);
				if ($i == 0) {
					$vm_phone = $phone;
				}
				$dial_strings[] = "ZAP/{$phone['interface']}/{$phone['extension']}";

			} else if ($subid[0] == "EXTERNAL" && $subid[1] == "PHONE") {
				$phone = external_get_phone($group['groupmember'][$i]);
				if ($i == 0) {
					$vm_phone = $phone;
				}
				$technology = asterisk_uniqid_to_technology($phone['dialprovider']);
				if ($technology == "ZAP") {
					$zprov = analog_get_provider($phone['dialprovider']);
					$dial_strings[] = "$technology/{$zprov['interface']}/{$phone['dialstring']}";
				} else {
					$dial_strings[] = "$technology/{$phone['dialprovider']}/{$phone['dialstring']}";	
				}
			}
			
		}
		$readbacknum = ($readbacknum) ? $readbacknum : $vm_account;
		$n = count($dial_strings);
		$dstring = $dial_strings[0];
		for ($i = 1; $i < $n; $i++) {
			$dstring .= "&" . $dial_strings[$i];
		}

		$dial .= _extensions_add_sendnotifications_line($exten, $vm_phone);
		$dial .= "exten => $exten,n,".
			"Macro(vm|$dstring|{$vm_phone['extension']}|$readbacknum|to)\n";

	} else if ($id[0] == "APPLICATION" && $id[1] == "MAPPING") {
		$app = dialplan_get_application($uniqid);
		
		$dial .= "exten => $exten,n,Answer()\n";
		if ($app['name'] == "WakeMe") {
			$conf .= "exten => $exten,n,Set(CHANNEL(language)=en)\n";
		}
		$dial .= "exten => $exten,n,{$app['name']}()\n";
		$dial .= "exten => $exten,n,Hangup()\n";
	}
	
	return $dial;
}

/**
 * Replace a string's contents with the name equivalents of the uniqids contained within.
 */
function asterisk_replace_uniqids_with_names($string) {
	
	$uniqid_regex = "/(SIP|IAX|ISDN|ANALOG|CONFERENCE|CALLGROUP|EXTERNAL){1}-(PROVIDER|PHONE|ROOM|PARALLEL){1}-[0-9a-zA-Z]+/";

	preg_match_all($uniqid_regex, $string, $uniqids);
	if (!is_array($uniqids)) {
		return;
	}

	$uniqids = array_keys(array_flip($uniqids[0]));

	$names = array();
	$n = count($uniqids);
	for ($i = 0; $i < $n; $i++) {
		$names[$i] = asterisk_uniqid_to_name($uniqids[$i]);
	}

	return str_replace($uniqids, $names, $string);
}

/**
 * Returns an appropriate "name" for the supplied uniqid.
 */
function asterisk_uniqid_to_name($uniqid) {
	global $config, $uniqid_map;
	
	$pair = explode("-", substr(strtolower($uniqid), 0 , strrpos($uniqid, "-")));

	if ($pair[1] == "phone" && $pair[0] == "external") {
		return $config[$pair[0]][$pair[1]][$uniqid_map[$uniqid]]['name'];
	} else if ($pair[1] == "phone") {
		return $config[$pair[0]][$pair[1]][$uniqid_map[$uniqid]]['callerid'];
	}
	if ($pair[1] == "provider") {
		return $config[$pair[0]][$pair[1]][$uniqid_map[$uniqid]]['name'];
	}
	if ($pair[1] == "room") {
		return $config[$pair[0]][$pair[1]][$uniqid_map[$uniqid]]['name'];
	}
	if ($pair[1] == "callgroup") {
		return $config[$pair[0]][$pair[1]][$uniqid_map[$uniqid]]['name'];
	}

	return "ERROR:UNIQID_TO_NAME_FAILED($uniqid)";
}

/**
 * Returns the extension associated with the supplied uniqid.
 */
function asterisk_uniqid_to_extension($uniqid) {
	global $config, $uniqid_map;
	
	$pair = explode("-", substr(strtolower($uniqid), 0 , strrpos($uniqid, "-")));

	if ($pair[1] == "phone") {
		return $config[$pair[0]][$pair[1]][$uniqid_map[$uniqid]]['extension'];
	}
	if ($pair[1] == "room") {
		return $config[$pair[0]][$pair[1]][$uniqid_map[$uniqid]]['extension'];
	}

	return "ERROR:UNIQID_TO_EXTENSION_FAILED($uniqid)";
}

/**
 * Returns the channel technology associated with the supplied uniqid.
 */
function asterisk_uniqid_to_technology($uniqid) {
	
	$parts = explode("-", $uniqid);
	switch($parts[0]) {
		case "SIP":
			return "SIP";
		case "IAX":
			return "IAX2";
		case "ISDN":
			return "CAPI";
		case "ANALOG":
			return "ZAP";
	}

	return "ERROR:UNIQID_TO_TECHNOLOGY_FAILED($uniqid)";
}

?>
