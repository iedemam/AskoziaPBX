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

	mwexec("/sbin/ifconfig lo 127.0.0.1");
	
	return 0;
}

/**
 * Configures the system's main LAN interface, bridging additional interfaces
 * to it as selected in the GUI.
 */
function network_lan_configure() {
	global $config, $g;

	$syscfg = $config['system'];
	$lancfg =& $config['interfaces']['lan'];
	$interface_names = array_keys(network_get_interfaces());
	
	if ($g['booting'])
		echo "Configuring LAN interface... ";
	else {
		unlink_if_exists("{$g['varetc_path']}/nameservers.conf");
	}

	if (isset($lancfg['dhcp'])) {
		foreach ($interface_names as $i) {
			mwexec("/sbin/ifconfig $i up");
			mwexec("/sbin/udhcpc -S -q -i $i -h " . escapeshellarg("{$syscfg['hostname']}.{$syscfg['domain']}"));
		}
	} else {
		/* deconfigure */
		mwexec("/sbin/ifconfig " . escapeshellarg($lancfg['if']) . " 0.0.0.0");
		mwexec("/sbin/route del default gw 0.0.0.0 dev " . escapeshellarg($lancfg['if']));

		/* reconfigure */
		mwexec("/sbin/ifconfig " . escapeshellarg($lancfg['if']) . " " .
			escapeshellarg($lancfg['ipaddr']) . " " .
			"netmask " . escapeshellarg(gen_subnet_mask($lancfg['subnet'])));
		mwexec("/sbin/ifconfig " . escapeshellarg($lancfg['if']) . " up");
		mwexec("/sbin/route add default gw " . escapeshellarg($lancfg['gateway']) . " " .
			"dev " . escapeshellarg($lancfg['if']));
	}
	///* no interface found? uh-oh! assign an interface and bridge everything to it */
	//} else if (!in_array($lancfg['if'], $interface_names)) {
	//	$lancfg['if'] = $interface_names[0];
	//	if (count($interface_names) > 1) {
	//		$lancfg['bridge'] = array_slice($interface_names, 1);
	//	} else {
	//		$lancfg['bridge'] = false;
	//	}
	//	write_config();
	//}

	/* MAC spoofing? */
	//if ($lancfg['spoofmac']) {
	//	mwexec("/sbin/ifconfig " . escapeshellarg($lancfg['if']) . 
	//		" link " . escapeshellarg($lancfg['spoofmac']));
	//}

	/* bridging */
	//mwexec("/sbin/sysctl net.link.ether.bridge.enable=0");
	//if (is_array($lancfg['bridge'])) {
	//	/* ensure that hardware TX/RX checksumming is off for bridged interfaces */
	//	foreach ($lancfg['bridge'] as $b_if) {
	//		mwexec("/sbin/ifconfig " . escapeshellarg($b_if) . " -rxcsum -txcsum");
	//	}
	//	mwexec("/sbin/ifconfig " . escapeshellarg($lancfg['if']) . " -rxcsum -txcsum");
	//	/* build bridge configuration */
	//	$bridgeconfig = implode(":0,", $lancfg['bridge']);
	//	$bridgeconfig .= ":0,{$lancfg['if']}:0";
	//	mwexec("/sbin/sysctl net.link.ether.bridge.config=" . $bridgeconfig);		
	//	mwexec("/sbin/sysctl net.link.ether.bridge.enable=1");
	//}

	
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
		return exec("/usr/bin/wget -q -O - http://ip.askozia.com");
	}
}

function network_get_interface_names() {
	exec("ls /proc/sys/net/ipv4/conf/ | grep eth", $names);
	return $names;
}

/**
 * Returns a list of interfaces with MAC addresses
 */
function network_get_interfaces() {
	
	global $g;

	// get network interface names
	$i_names = network_get_interface_names();
	// interface information array
	$iflist = array();
	// go through each name and populate the interface information array
	foreach ($i_names as $i) {

		if (preg_match("/^(eth)\d/", $i)) {

			// get ifconfig's output for this interface
			exec("/sbin/ifconfig $i", $output);
			$output = implode(" ", $output);

			// grab the mac address
			preg_match("/HWaddr (\S+)/", $output, $matches);
			$iflist[$i]['mac'] = $matches[1];

			// grab the ip address
			preg_match("/inet addr:(\S+)/", $output, $matches);
			$iflist[$i]['ipaddr'] = $matches[1];

			// grab the subnet
			preg_match("/Mask:(\S+)/", $output, $matches);
			$iflist[$i]['subnet'] = netmask_to_cidr_bits($matches[1]);

			// is the interface up
			preg_match("/\s+(UP)\s+/", $output, $matches);
			if ($matches[1] == "UP") {
				$iflist[$i]['up'] = true;
			} else {
				$iflist[$i]['up'] = false;
			}

		}
	}
	return $iflist;
}

function netmask_to_cidr_bits($netmask) {
	$int = ip2long($netmask);
	$int = $int - (($int >> 1) & 0x55555555);
	$int = ($int & 0x33333333) + (($int >> 2) & 0x33333333);
	return (($int + ($int >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
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
