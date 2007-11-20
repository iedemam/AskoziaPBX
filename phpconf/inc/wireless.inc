<?php
/*
	$Id: network.inc 200 2007-09-24 12:26:27Z michael.iedema $
	originally part of m0n0wall (http://m0n0.ch/wall)
	continued modifications as part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
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
 * Returns a list of wireless interfaces with MAC addresses
 * - skips VLAN and other virtual interfaces
 */
function wireless_get_interfaces() {
	
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
		
		if (preg_match("/(wi|ath|ral|ural)/", $ifname)) {
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
 * Configures the wireless interface and adds it to the LAN bridge.
 */
function wireless_configure() {
	global $config, $g;
	
	if (!is_array($config['interfaces']['wireless'])) {
		return 0;
	}
	
	if ($g['booting'])
		echo "Configuring Wireless interface... ";


	$if = $config['interfaces']['wireless']['if'];
	$wlcfg = $config['interfaces']['wireless'];
	
	if (isset($wlcfg['enable'])) {
	
		/* remove all addresses first */
		while (mwexec("/sbin/ifconfig " . escapeshellarg($if) . " -alias") == 0);
		mwexec("/sbin/ifconfig " . escapeshellarg($if) . " down");
		
		/* the mode/standard (11a/b/g) needs to be set separately */
		if ($wlcfg['standard']) {
			mwexec("/sbin/ifconfig " . escapeshellarg($if) . " mode " .
				escapeshellarg($wlcfg['standard']));
		}
		
		$ifcargs = escapeshellarg($if) . 
			" ssid " . escapeshellarg($wlcfg['ssid']) . " channel " . 
			escapeshellarg($wlcfg['channel']) . " ";
		
		switch ($wlcfg['mode']) {
			case 'hostap':
				$ifcargs .= "-mediaopt adhoc mediaopt hostap ";
				break;
			case 'ibss':
			case 'IBSS':
				if (strstr($if, "an"))
					$ifcargs .= "mediaopt adhoc ";
				else
					$ifcargs .= "-mediaopt hostap mediaopt adhoc ";
				break;
			case 'bss':
			case 'BSS':
				if (strstr($if, "an"))
					$ifcargs .= "-mediaopt adhoc ";
				else
					$ifcargs .= "-mediaopt hostap -mediaopt adhoc ";
				break;
		}
		
		$ifcargs .= "up";
		
		mwexec("/sbin/ifconfig " . $ifcargs);
		
		/* need to set AP-specific parameters in a separate ifconfig call */
		if ($wlcfg['mode'] == "hostap") {
			$ifcargs = escapeshellarg($if);
			
			if (isset($wlcfg['hidessid']))
				$ifcargs .= " hidessid";
			else
				$ifcargs .= " -hidessid";
				
			mwexec("/sbin/ifconfig " . $ifcargs);
		}
		
		/* configure encryption (if needed) */
		wireless_configure_encryption();
		
		/* add interface to LAN bridge */
		if (!is_array($config['interfaces']['lan']['bridge'])) {
			$config['interfaces']['lan']['bridge'] = array();
		}
		if (!in_array($if, $config['interfaces']['lan']['bridge'])) {
			$config['interfaces']['lan']['bridge'][] = $if;
		}
	
	} else {
		mwexec("/sbin/ifconfig " . escapeshellarg($wlcfg['if']) . 
			" delete down");
		
		if (is_array($config['interfaces']['lan']['bridge']) &&
			in_array($if, $config['interfaces']['lan']['bridge'])) {
			$n = count($config['interfaces']['lan']['bridge']);
			for ($i = 0; $i < $n; $i++) {
				if ($config['interfaces']['lan']['bridge'][$i] == $if) {
					unset($config['interfaces']['lan']['bridge'][$i]);
				}
			}
		}
	}
	
	if ($g['booting'])
		echo "done\n";
	
	return 0;
}

/**
 * Applies wireless encryption settings. Currently, AskoziaPBX only
 * uses hostap mode.
 */
function wireless_configure_encryption() {
	global $config, $g;
	
	$if = $config['interfaces']['wireless']['if'];
	$wlcfg = $config['interfaces']['wireless'];
	
	/* kill any running hostapd */
	killbypid("{$g['varrun_path']}/hostapd-$if.pid");
	unlink_if_exists("{$g['varetc_path']}/hostapd-$if.conf");
	
	/* killing hostapd leaves the interface in down state - make sure it's up again */
	mwexec("/sbin/ifconfig " . escapeshellarg($if) . " up");

	if ($wlcfg['wpa']['mode'] && $wlcfg['wpa']['mode'] != "none") {

		if ($wlcfg['mode'] == "hostap") {
			/* use WPA in hostap mode */
			$hostapdcfg = wireless_generate_hostapd_config($if, $wlcfg);
			
			/* write config to temporary file */
			$fd = fopen("{$g['varetc_path']}/hostapd-$if.conf", "w");
			if (!$fd) {
				echo("Error: cannot open hostapd-$if.conf in wireless_configure_encryption().\n");
				return 1;
			}
	
			fwrite($fd, $hostapdcfg);
			fclose($fd);
			
			/* start hostapd */
			mwexec("/usr/sbin/hostapd -B -P {$g['varrun_path']}/hostapd-$if.pid {$g['varetc_path']}/hostapd-$if.conf");
		}
		
	} else {
		/* use WEP (or no encryption) */
	
		if (isset($wlcfg['wep']['enable']) && is_array($wlcfg['wep']['key'])) {
			$ifcargs = escapeshellarg($if) . " wepmode on";
			
			$i = 1;
			foreach ($wlcfg['wep']['key'] as $wepkey) {
				$ifcargs .= " wepkey " . escapeshellarg("{$i}:{$wepkey['value']}");
				if (isset($wepkey['txkey'])) {
					$ifcargs .= " weptxkey {$i}";
				}
				$i++;
			}
			mwexec("/sbin/ifconfig " . $ifcargs);
		} else {
			/* no encryption - make sure WEP is disabled */
			mwexec("/sbin/ifconfig " . escapeshellarg($if) . " wepmode off");
		}
	}
}

/**
 * Generates and returns a hostapd configuration file
 */
function wireless_generate_hostapd_config($if, $wlcfg) {
	$config = <<<EOD
interface=$if
driver=bsd
logger_syslog=-1
logger_syslog_level=2
logger_stdout=0
ssid={$wlcfg['ssid']}
wpa={$wlcfg['wpa']['version']}

EOD;
	
	if ($wlcfg['wpa']['mode'] == "psk") {
		$config .= <<<EOD
wpa_key_mgmt=WPA-PSK
wpa_passphrase={$wlcfg['wpa']['psk']}

EOD;
	} else if ($wlcfg['wpa']['mode'] == "enterprise") {
	
		$authport = 1812;
		$acctport = 1813;
		if ($wlcfg['wpa']['radius']['authport'])
			$authport = $wlcfg['wpa']['radius']['authport'];
		if ($wlcfg['wpa']['radius']['acctport'])
			$acctport = $wlcfg['wpa']['radius']['acctport'];
	
		$config .= <<<EOD
ieee8021x=1
wpa_key_mgmt=WPA-EAP
auth_server_addr={$wlcfg['wpa']['radius']['server']}
auth_server_port={$authport}
auth_server_shared_secret={$wlcfg['wpa']['radius']['secret']}
acct_server_addr={$wlcfg['wpa']['radius']['server']}
acct_server_port={$acctport}
acct_server_shared_secret={$wlcfg['wpa']['radius']['secret']}

EOD;
	}
	
	if ($wlcfg['wpa']['cipher'] == "tkip")
		$config .= "wpa_pairwise=TKIP\n";
	else if ($wlcfg['wpa']['cipher'] == "ccmp")
		$config .= "wpa_pairwise=CCMP\n";
	else
		$config .= "wpa_pairwise=TKIP CCMP\n";
	
	return $config;
}

?>
