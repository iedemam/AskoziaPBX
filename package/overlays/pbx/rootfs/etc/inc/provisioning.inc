<?php
/*
	$Id: pbx.inc 1325 2010-01-22 09:09:35Z michael.iedema $
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2010 IKT <http://itison-ikt.de>.
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


function provisioning_configure() {
	global $config, $g;

	$laninfo = network_get_interface($config['interfaces']['lan']['if']);
	$port = 56080;

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
	mwexec("httpd -c /etc/httpd_provisioning.conf -u root -p " . $port . " -h /usr/www_provisioning/");
	mwexec_bg("/usr/sbin/snomd -l -v -u \"http://" . $laninfo['ipaddr'] . ":" . $port . "/snom.php?mac={mac}\"");
}

?>
