<?php
/*
	$Id: pbx.inc 1325 2010-01-22 09:09:35Z michael.iedema $
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

/* include all configuration functions */
require_once("functions.inc");


function provisioning_configure() {
	global $config, $g;

	openlog("provisioning_configure()", LOG_INFO, LOG_LOCAL0);

	$laninfo = network_get_interface($config['interfaces']['lan']['if']);
	$port = 56080;

	// generate the httpd configuration file
	$fd = fopen("{$g['etc_path']}/httpd_provisioning.conf", "w");
	if (!$fd) {
		printf("Error: cannot open httpd_provisioning.conf in provisioning_configure().\n");
		return 1;
	}
	fwrite($fd,
		"*.php:/usr/bin/php\n".
		"I:index.php\n".
		"\n"
	);
	fclose($fd);
	chmod("{$g['etc_path']}/httpd_provisioning.conf", 0600);

	// kill any existing instances of the provisioning http server and snom daemon
	$pid = exec('ps | grep "/etc/httpd_provisioning.conf" | grep -v "grep" | cut -d " " -f 2', $output);
	syslog(LOG_INFO, "pid finder returned: " . implode("\n", $output));
	if ($pid) {
		syslog(LOG_INFO, "Existing httpd provisioning server running (pid: " . $pid . "), stopping.");
		killbypidnumber($pid);
	} else {
		syslog(LOG_INFO, "No existing httpd provisioning server running, not killing.");
	}
	killbyname("snomd");

	// see if any phones are setup with auto-provisioning...
	$needs_provisioning = false;
	$phones = sip_get_phones();
	foreach ($phones as $phone) {
		if (isset($phone['disabled'])) {
			continue;
		}
		if ($phone['snom-mac']) {
			$needs_provisioning = true;
			break;
		}
	}

	// ...and launch the appropriate daemons
	if ($needs_provisioning) {
		syslog(LOG_INFO, "Phones needing provisioning found, starting services.");
		mwexec("httpd -c /etc/httpd_provisioning.conf -u root -p " . $port . " -h /usr/www_provisioning/");
		mwexec_bg("/usr/sbin/snomd -l -v -u \"http://" . $laninfo['ipaddr'] . ":" . $port . "/snom.php?mac={mac}\"");
	} else {
		syslog(LOG_INFO, "No phones needing provisioning found, not starting services.");
	}

	closelog();

	return 0;
}

?>
