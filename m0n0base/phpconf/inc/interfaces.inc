<?php
/*
	$Id: interfaces.inc 173 2006-12-09 18:44:57Z mkasper $
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

/* include all configuration functions */
require_once("functions.inc");

function interfaces_loopback_configure() {
	global $config, $g;

	mwexec("/sbin/ifconfig lo0 127.0.0.1");
	
	return 0;
}

function interfaces_lan_configure() {
	global $config, $g;
	
	$lancfg = $config['interfaces']['lan'];
	
	if ($g['booting'])
		echo "Configuring LAN interface... ";
	else {
		unlink_if_exists("{$g['varetc_path']}/nameservers.conf");
	}
	
	
	/* remove all addresses first */
	while (mwexec("/sbin/ifconfig " . escapeshellarg($lancfg['if']) . " -alias") == 0);
	mwexec("/sbin/ifconfig " . escapeshellarg($lancfg['if']) . " down");
		
		
	/* media */
	if ($lancfg['media'] || $lancfg['mediaopt']) {
		$cmd = "/sbin/ifconfig " . escapeshellarg($lancfg['if']);
		if ($lancfg['media'])
			$cmd .= " media " . escapeshellarg($lancfg['media']);
		if ($lancfg['mediaopt'])
			$cmd .= " mediaopt " . escapeshellarg($lancfg['mediaopt']);
		mwexec($cmd);
	}
	
	$addflags = "";
	if (strpos($lancfg['if'], "fxp") !== false)
		$addflags .= " link0";
	

	mwexec("/sbin/ifconfig " . escapeshellarg($lancfg['if']) . " " . 
			escapeshellarg($lancfg['ipaddr'] . "/" . $lancfg['subnet']) . $addflags);
			
	/* install default route */
	mwexec("/sbin/route delete default");
	mwexec("/sbin/route add default " . escapeshellarg($lancfg['gateway']));

	
	if (!$g['booting']) {
		/* reconfigure static routes (kernel may have deleted them) */
		system_routing_configure();
	}
	
	if ($g['booting'])
		echo "done\n";
	
	return 0;
}

function get_interface_mac($interface) {
  
        /* build interface list with netstat */
        exec("/usr/bin/netstat -I $interface -nW -f link", $linkinfo);
        array_shift($linkinfo);
        $alink = preg_split("/\s+/", $linkinfo[0]);
        $mac = chop($alink[3]);
        return $mac;
}

?>
