<?php
/*
	$Id$
	originally part of m0n0wall (http://m0n0.ch/wall)
	continued modifications as part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
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
 * Configures the system's loopback device
 */
function network_loopback_configure() {
	global $config, $g;

	mwexec("/sbin/ifconfig lo0 127.0.0.1");
	
	return 0;
}

/**
 * Configures the system's main LAN interface, bridging additional interfaces
 * to it as selected in the GUI.
 */
function network_lan_configure() {
	global $config, $g;
	
	$lancfg =& $config['interfaces']['lan'];
	$interface_names = array_keys(network_get_interfaces());
	
	if ($g['booting'])
		echo "Configuring LAN interface... ";
	else {
		unlink_if_exists("{$g['varetc_path']}/nameservers.conf");
	}

	/* no interface found? uh-oh! assign an interface and bridge everything to it */
	if (!in_array($lancfg['if'], $interface_names)) {
		$lancfg['if'] = $interface_names[0];
		if (count($interface_names) > 1) {
			$lancfg['bridge'] = array_slice($interface_names, 1);
		} else {
			$lancfg['bridge'] = false;
		}
		write_config();
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
		/* ensure that hardware TX/RX checksumming is off for bridged interfaces */
		foreach ($lancfg['bridge'] as $b_if) {
			mwexec("/sbin/ifconfig " . escapeshellarg($b_if) . " -rxcsum -txcsum");
		}
		mwexec("/sbin/ifconfig " . escapeshellarg($lancfg['if']) . " -rxcsum -txcsum");
		/* build bridge configuration */
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
		
		/* restart ez-ipupdate */
		services_dyndns_configure();
		
		/* force DNS update *//*
		services_dnsupdate_process();*/
	}
	
	if ($g['booting'])
		echo "done\n";
	
	return 0;
}

/**
 * Returns the current public IP address.
 */
function network_get_current_public_ip_address() {
	global $config, $g;
	
	$lancfg = $config['interfaces']['lan'];
	
	if ($lancfg['topology'] == "public") {
	 	return $lancfg['ipaddr'];

	} else if ($lancfg['topology'] == "natstatic") {
		return $lancfg['extipaddr'];

	} else {
		return exec("/usr/bin/fetch -q -o - http://ip.askozia.com");
	}
}

/**
 * Returns the given interface's MAC address
 */
function network_get_interface_mac($interface) {
  
	/* build interface list with netstat */
	exec("/usr/bin/netstat -I $interface -nW -f link", $linkinfo);
	array_shift($linkinfo);
	$alink = preg_split("/\s+/", $linkinfo[0]);
	$mac = chop($alink[3]);

	return $mac;
}

/**
 * Returns a list of interfaces with MAC addresses
 * - skips VLAN and other virtual interfaces
 * - also skips WLAN interfaces
 */
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

		if (!preg_match("/^(ppp|sl|gif|faith|lo|ng|vlan|tun|enc|wi|ath|ral|ural|ipr)\d/", $ifname)) {
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

/**
 * Returns the subnet address given a host address and a subnet bit count
 */
function gen_subnet($ipaddr, $bits) {
	if (!verify_is_ipaddress($ipaddr) || !is_numeric($bits))
		return "";
	
	return long2ip(ip2long($ipaddr) & gen_subnet_mask_long($bits));
}

/**
 * return the highest (broadcast) address in the subnet given a host address 
 * and a subnet bit count
 */
function gen_subnet_max($ipaddr, $bits) {
	if (!verify_is_ipaddress($ipaddr) || !is_numeric($bits))
		return "";
	
	return long2ip(ip2long($ipaddr) | ~gen_subnet_mask_long($bits));
}

/**
 * Returns a subnet mask (long given a bit count)
 */
function gen_subnet_mask_long($bits) {
	$sm = 0;
	for ($i = 0; $i < $bits; $i++) {
		$sm >>= 1;
		$sm |= 0x80000000;
	}
	return $sm;
}

/**
 * Same as gen_subnet_mask_long but returns a string
 */
function gen_subnet_mask($bits) {
	return long2ip(gen_subnet_mask_long($bits));
}

/**
 * Find out whether two subnets overlap
 */
function check_subnets_overlap($subnet1, $bits1, $subnet2, $bits2) {

	if (!is_numeric($bits1))
		$bits1 = 32;
	if (!is_numeric($bits2))
		$bits2 = 32;

	if ($bits1 < $bits2)
		$relbits = $bits1;
	else
		$relbits = $bits2;
	
	$sn1 = gen_subnet_mask_long($relbits) & ip2long($subnet1);
	$sn2 = gen_subnet_mask_long($relbits) & ip2long($subnet2);
	
	if ($sn1 == $sn2)
		return true;
	else
		return false;
}

/**
 * Compares two IP addresses
 */
function ipcmp($a, $b) {
	if (ip2long($a) < ip2long($b))
		return -1;
	else if (ip2long($a) > ip2long($b))
		return 1;
	else
		return 0;
}

/**
 * Returns true if $addr is in $subnet, false if not
 */
function ip_in_subnet($addr,$subnet) {
	list($ip, $mask) = explode('/', $subnet);
	$mask = 0xffffffff << (32 - $mask);
	return ((ip2long($addr) & $mask) == (ip2long($ip) & $mask));
}

/**
 * Obtains MAC address given an IP address by looking at the ARP table
 */
function arp_get_mac_by_ip($ip) {
	exec("/usr/sbin/arp -n {$ip}", $arpoutput);
	
	if ($arpoutput[0]) {
		$arpi = explode(" ", $arpoutput[0]);
		$macaddr = $arpi[3];
		if (verify_is_macaddress($macaddr))
			return $macaddr;
		else
			return false;
	}
	
	return false;
}

/**
 * Returns MAC addresses in a variety of formats
 */
function mac_format($clientmac) {
    $mac =explode(":", $clientmac);

    global $config;

    $mac_format = $config['captiveportal']['radmac_format'] ? $config['captiveportal']['radmac_format'] : false;

    switch($mac_format) {

        case 'singledash':
        return "$mac[0]$mac[1]$mac[2]-$mac[3]$mac[4]$mac[5]";

        case 'ietf':
        return "$mac[0]-$mac[1]-$mac[2]-$mac[3]-$mac[4]-$mac[5]";

        case 'cisco':
        return "$mac[0]$mac[1].$mac[2]$mac[3].$mac[4]$mac[5]";

        case 'unformatted':
        return "$mac[0]$mac[1]$mac[2]$mac[3]$mac[4]$mac[5]";

        default:
        return $clientmac;
    }
}

?>
