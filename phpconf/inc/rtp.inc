<?php
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2008 IKT <http://itison-ikt.de>.
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
 * Generates rtp.conf which contains the RTP port range Asterisk uses
 */
function rtp_conf_generate() {
	global $config;
	
	$fd = fopen("/usr/local/etc/asterisk/rtp.conf", "w");
	if (!$fd) {
		printf("Error: cannot open rtp.conf in rtp_conf_generate().\n");
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

?>
