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

/* include all configuration functions */
require_once("functions.inc");

function network_loopback_configure() {
	global $config, $g;

	mwexec("/sbin/ifconfig lo0 127.0.0.1");
	
	return 0;
}

function network_lan_configure() {
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
	
	/* MAC spoofing? */
	if ($lancfg['spoofmac']) {
		mwexec("/sbin/ifconfig " . escapeshellarg($lancfg['if']) . 
			" link " . escapeshellarg($lancfg['spoofmac']));
	}
		
	/* media */
	if ($lancfg['media'] || $lancfg['mediaopt']) {
		$cmd = "/sbin/ifconfig " . escapeshellarg($lancfg['if']);
		if ($lancfg['media'])
			$cmd .= " media " . escapeshellarg($lancfg['media']);
		if ($lancfg['mediaopt'])
			$cmd .= " mediaopt " . escapeshellarg($lancfg['mediaopt']);
		mwexec($cmd);
	}
	
	
	/* bridging */
	mwexec("/sbin/sysctl net.link.ether.bridge.enable=0");
	if (is_array($lancfg['bridge'])) {
		$bridgeconfig = implode(":0,", $lancfg['bridge']);
		$bridgeconfig .= ":0,{$lancfg['if']}:0";
		mwexec("/sbin/sysctl net.link.ether.bridge.config=" . $bridgeconfig);		
		mwexec("/sbin/sysctl net.link.ether.bridge.enable=1");
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
		
		/* restart ez-ipupdate *//*
		services_dyndns_configure();*/
		
		/* force DNS update *//*
		services_dnsupdate_process();*/
	}
	
	if ($g['booting'])
		echo "done\n";
	
	return 0;
}

function get_current_public_address() {
	global $config, $g;
	
	$lancfg = $config['interfaces']['lan'];
	
	if ($lancfg['topology'] == "public")
	 	return $lancfg['ipaddr'];
	else if ($lancfg['topology'] == "natstatic")
		return $lancfg['extipaddr'];
		
	/* else.. TODO: get STUN public info */
}

function get_interface_mac($interface) {
  
	/* build interface list with netstat */
	exec("/usr/bin/netstat -I $interface -nW -f link", $linkinfo);
	array_shift($linkinfo);
	$alink = preg_split("/\s+/", $linkinfo[0]);
	$mac = chop($alink[3]);
	return $mac;
}

/* returns a list of interfaces with MAC addresses
   (skips VLAN and other virtual interfaces) */
function network_get_interfaces() {
	
	global $g;
	
	/* build interface list with netstat */
	exec("/usr/bin/netstat -inW -f link", $linkinfo);
	array_shift($linkinfo);
	
	$iflist = array();
	
	foreach ($linkinfo as $link) {
		$alink = preg_split("/\s+/", $link);
		$ifname = chop($alink[0]);
		
		if (substr($ifname, -1) == "*")
			$ifname = substr($ifname, 0, strlen($ifname) - 1);
		
		if (!preg_match("/^(ppp|sl|gif|faith|lo|ng|vlan|tun)/", $ifname)) {
			$iflist[$ifname] = array();
			
			$iflist[$ifname]['mac'] = chop($alink[3]);
			$iflist[$ifname]['up'] = false;
			
			/* find out if the link on this interface is up */
			unset($ifinfo);
			exec("/sbin/ifconfig {$ifname}", $ifinfo);
			
			foreach ($ifinfo as $ifil) {
				if (preg_match("/status: (.*)$/", $ifil, $matches)) {
					if ($matches[1] == "active")
						$iflist[$ifname]['up'] = true;
					break;
				}
			}
		}
	}
	
	return $iflist;
}

?>
