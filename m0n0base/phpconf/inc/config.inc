<?php
/*
	$Id: config.inc 174 2006-12-16 17:10:52Z mkasper $
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

/* include globals/utility/XML parser files */
require_once("globals.inc");
require_once("util.inc");
require_once("xmlparse.inc");

/* read platform */
if (file_exists("{$g['etc_path']}/platform")) {
	$g['fullplatform'] = chop(file_get_contents("{$g['etc_path']}/platform"));
	$pla = explode("_", $g['fullplatform']);
	$g['platform'] = $pla[0];
} else {
	$g['platform'] = $g['fullplatform'] = "unknown";
}

if ($g['booting']) {
	/* find the device where config.xml resides and write out an fstab */
	unset($cfgdevice);
	
	/* check if there's already an fstab (NFS booting?) */
	if (!file_exists("{$g['etc_path']}/fstab")) {
	
		if (strstr($g['platform'], "cdrom")) {
			/* config is either on floppy disk or USB stick for CD-ROM version */
			
			/* try the USB stick at da0 first, and if this fails, use the floppy disk  */
			$cfgdevice = "da0";
			$cfgpartition = "da0s1";
			$cfgfstype = "msdos";
			
			if (mwexec("/sbin/mount -t msdos -r /dev/da0s1 {$g['cf_path']}") == 0) {
				/* USB stick is OK */
				echo "Using USB memory stick for configuration storage.\n";
				mwexec("/sbin/umount -f {$g['cf_path']}");
			} else {
				$cfgdevice = $cfgpartition = "fd0";
				
				if (mwexec("/sbin/mount -t msdos -r /dev/fd0 {$g['cf_path']}") != 0) {
					echo <<<EOD


*******************************************************************************
* WARNING!                                                                    *
* No FAT formatted USB memory stick or floppy disk could be found.            *
* Your configuration changes will be lost on reboot.                          *
*******************************************************************************


EOD;
					sleep(5);
				} else {
					echo "Using floppy disk for configuration storage.\n";
					mwexec("/sbin/umount -f {$g['cf_path']}");
				}
			}
		} else {
			/* probe kernel known disks until we find one with config.xml */
			$disks = explode(" ", trim(preg_replace("/kern.disks: /", "", exec("/sbin/sysctl kern.disks"))));
			foreach ($disks as $mountdisk) {
				/* skip mfs mounted filesystems */
				if (strstr($mountdisk, "md"))
					continue;
				if (mwexec("/sbin/mount -r /dev/{$mountdisk}a {$g['cf_path']}") == 0) {
					if (file_exists("{$g['cf_conf_path']}/config.xml")) {
						/* found it */
						$cfgdevice = $mountdisk;
						$cfgpartition = $cfgdevice . "a";
						$cfgfstype = "ufs";
						echo "Found configuration on $cfgdevice.\n";
					}
					
					mwexec("/sbin/umount -f {$g['cf_path']}");
					
					if ($cfgdevice)
						break;
				}
			}
		}
		
		if (!$cfgdevice) {
			/* no device found, print an error and die */
			echo <<<EOD


*******************************************************************************
* FATAL ERROR                                                                 *
* The device that contains the configuration file (config.xml) could not be   *
* found. m0n0wall cannot continue booting.                                    *
*******************************************************************************


EOD;

			mwexec("/sbin/halt");
			exit;
		}
		
		/* write device name to a file for rc.firmware */
		$fd = fopen("{$g['varetc_path']}/cfdevice", "w");
		fwrite($fd, $cfgdevice . "\n");
		fclose($fd);
		
		/* write out an fstab */
		$fd = fopen("{$g['etc_path']}/fstab", "w");
		
		$fstab = "/dev/{$cfgpartition} {$g['cf_path']} {$cfgfstype} ro 1 1\n";
		$fstab .= "proc /proc procfs rw 0 0\n";
		
		fwrite($fd, $fstab);
		fclose($fd);
	}
	
	/* mount all filesystems */
	mwexec("/sbin/mount -a");
}

/* parse configuration */
if (!$noparseconfig) {
	
	config_lock();
	
	/* see if there's a newer cache file */
	if (file_exists("{$g['tmp_path']}/config.cache") && 
		(filemtime("{$g['tmp_path']}/config.cache") >= 
		 filemtime("{$g['conf_path']}/config.xml"))) {
		
		/* read cache */
		$config = unserialize(file_get_contents("{$g['tmp_path']}/config.cache"));
	} else {
	
		if (!file_exists("{$g['conf_path']}/config.xml")) {
			if ($g['booting']) {
				if (strstr($g['platform'], "cdrom")) {
					/* try copying the default config. to the floppy/USB stick */
					reset_factory_defaults();
					
					echo "No XML configuration file found - using factory defaults.\n";
				} else {
					echo "XML configuration file not found. m0n0wall cannot continue booting.\n"; 
					mwexec("/sbin/halt");
					exit;
				}
			} else {
				config_unlock();
				exit(0);
			}
		}
		
		$config = parse_xml_config("{$g['conf_path']}/config.xml", $g['xml_rootobj']);
		
		if ((float)$config['version'] > (float)$g['latest_config']) {
			if ($g['booting']) {
				echo <<<EOD


*******************************************************************************
* WARNING!                                                                    *
* The current configuration has been created with a newer version of m0n0wall *
* than this one! This can lead to serious misbehavior and even security       *
* holes! You are urged to either upgrade to a newer version of m0n0wall or    *
* revert to the default configuration immediately!                            *
*******************************************************************************


EOD;
			}
		}
		
		/* write config cache */
		$fd = @fopen("{$g['tmp_path']}/config.cache", "wb");
		if ($fd) {
			fwrite($fd, serialize($config));
			fclose($fd);
		}
	}
	
	config_unlock();
	
	/* make alias table (for faster lookups) */
	alias_make_table();
}

/* mount flash card read/write */
function conf_mount_rw() {
	global $g;

	/* don't use mount -u anymore
	   (doesn't sync the files properly and /bin/sync won't help either) */
	mwexec("/sbin/umount -f {$g['cf_path']}");
	mwexec("/sbin/mount -w -o noatime {$g['cf_path']}");
}

/* mount flash card read only */
function conf_mount_ro() {
	global $g;
	
	mwexec("/sbin/umount -f {$g['cf_path']}");
	mwexec("/sbin/mount -r {$g['cf_path']}");
}

/* convert configuration, if necessary */
function convert_config() {
	global $config, $g;
	
	if ($config['version'] == $g['latest_config'])
		return;		/* already at latest version */
	
	if ($g['booting'])
		echo "Converting configuration... ";
	
	/* convert 1.0 -> 1.1 */
	if ($config['version'] == "1.0") {
		$opti = 1;
		$ifmap = array('lan' => 'lan', 'wan' => 'wan', 'pptp' => 'pptp');
		
		/* convert DMZ to optional, if necessary */
		if (isset($config['interfaces']['dmz'])) {
			
			$dmzcfg = &$config['interfaces']['dmz'];
			
			if ($dmzcfg['if']) {
				$config['interfaces']['opt' . $opti] = array();
				$optcfg = &$config['interfaces']['opt' . $opti];
				
				$optcfg['enable'] = $dmzcfg['enable'];
				$optcfg['descr'] = "DMZ";
				$optcfg['if'] = $dmzcfg['if'];
				$optcfg['ipaddr'] = $dmzcfg['ipaddr'];
				$optcfg['subnet'] = $dmzcfg['subnet'];
				
				$ifmap['dmz'] = "opt" . $opti;
				$opti++;
			}
			
			unset($config['interfaces']['dmz']);
		}
		
		/* convert WLAN1/2 to optional, if necessary */
		for ($i = 1; isset($config['interfaces']['wlan' . $i]); $i++) {
			
			if (!$config['interfaces']['wlan' . $i]['if']) {
				unset($config['interfaces']['wlan' . $i]);
				continue;
			}
			
			$wlancfg = &$config['interfaces']['wlan' . $i];
			$config['interfaces']['opt' . $opti] = array();
			$optcfg = &$config['interfaces']['opt' . $opti];
			
			$optcfg['enable'] = $wlancfg['enable'];
			$optcfg['descr'] = "WLAN" . $i;
			$optcfg['if'] = $wlancfg['if'];
			$optcfg['ipaddr'] = $wlancfg['ipaddr'];
			$optcfg['subnet'] = $wlancfg['subnet'];
			$optcfg['bridge'] = $wlancfg['bridge'];
			
			$optcfg['wireless'] = array();
			$optcfg['wireless']['mode'] = $wlancfg['mode'];
			$optcfg['wireless']['ssid'] = $wlancfg['ssid'];
			$optcfg['wireless']['channel'] = $wlancfg['channel'];
			$optcfg['wireless']['wep'] = $wlancfg['wep'];
			
			$ifmap['wlan' . $i] = "opt" . $opti;
			
			unset($config['interfaces']['wlan' . $i]);
			$opti++;
		}
		
		/* convert filter rules */
		$n = count($config['filter']['rule']);
		for ($i = 0; $i < $n; $i++) {
		
			$fr = &$config['filter']['rule'][$i];
			
			/* remap interface */
			if (array_key_exists($fr['interface'], $ifmap))
				$fr['interface'] = $ifmap[$fr['interface']];
			else {
				/* remove the rule */
				echo "\nWarning: filter rule removed " . 
					"(interface '{$fr['interface']}' does not exist anymore).";
				unset($config['filter']['rule'][$i]);
				continue;
			}
			
			/* remap source network */
			if (isset($fr['source']['network'])) {
				if (array_key_exists($fr['source']['network'], $ifmap))
					$fr['source']['network'] = $ifmap[$fr['source']['network']];
				else {
					/* remove the rule */
					echo "\nWarning: filter rule removed " . 
						"(source network '{$fr['source']['network']}' does not exist anymore).";
					unset($config['filter']['rule'][$i]);
					continue;
				}
			}
					
			/* remap destination network */
			if (isset($fr['destination']['network'])) {
				if (array_key_exists($fr['destination']['network'], $ifmap))
					$fr['destination']['network'] = $ifmap[$fr['destination']['network']];
				else {
					/* remove the rule */
					echo "\nWarning: filter rule removed " . 
						"(destination network '{$fr['destination']['network']}' does not exist anymore).";
					unset($config['filter']['rule'][$i]);
					continue;
				}
			}
		}
		
		/* convert shaper rules */
		$n = count($config['shaper']['rule']);
		if (is_array($config['shaper']['rule']))
			for ($i = 0; $i < $n; $i++) {
		
			$fr = &$config['shaper']['rule'][$i];
			
			/* remap interface */
			if (array_key_exists($fr['interface'], $ifmap))
				$fr['interface'] = $ifmap[$fr['interface']];
			else {
				/* remove the rule */
				echo "\nWarning: traffic shaper rule removed " . 
					"(interface '{$fr['interface']}' does not exist anymore).";
				unset($config['shaper']['rule'][$i]);
				continue;
			}
			
			/* remap source network */
			if (isset($fr['source']['network'])) {
				if (array_key_exists($fr['source']['network'], $ifmap))
					$fr['source']['network'] = $ifmap[$fr['source']['network']];
				else {
					/* remove the rule */
					echo "\nWarning: traffic shaper rule removed " . 
						"(source network '{$fr['source']['network']}' does not exist anymore).";
					unset($config['shaper']['rule'][$i]);
					continue;
				}
			}
					
			/* remap destination network */
			if (isset($fr['destination']['network'])) {
				if (array_key_exists($fr['destination']['network'], $ifmap))
					$fr['destination']['network'] = $ifmap[$fr['destination']['network']];
				else {
					/* remove the rule */
					echo "\nWarning: traffic shaper rule removed " . 
						"(destination network '{$fr['destination']['network']}' does not exist anymore).";
					unset($config['shaper']['rule'][$i]);
					continue;
				}
			}
		}
				
		$config['version'] = "1.1";
	}
	
	/* convert 1.1 -> 1.2 */
	if ($config['version'] == "1.1") {
		/* move LAN DHCP server config */
		$tmp = $config['dhcpd'];
		$config['dhcpd'] = array();
		$config['dhcpd']['lan'] = $tmp;
		
		/* encrypt password */
		$config['system']['password'] = crypt($config['system']['password']);
		
		$config['version'] = "1.2";
	}
	
	/* convert 1.2 -> 1.3 */
	if ($config['version'] == "1.2") {
		/* convert advanced outbound NAT config */
		for ($i = 0; isset($config['nat']['advancedoutbound']['rule'][$i]); $i++) {
			$curent = &$config['nat']['advancedoutbound']['rule'][$i];
			$src = $curent['source'];
			$curent['source'] = array();
			$curent['source']['network'] = $src;
			$curent['destination'] = array();
			$curent['destination']['any'] = true;
		}
		
		/* add an explicit type="pass" to all filter rules to make things consistent */
		for ($i = 0; isset($config['filter']['rule'][$i]); $i++) {
			$config['filter']['rule'][$i]['type'] = "pass";
		}
		
		$config['version'] = "1.3";
	}
	
	/* convert 1.3 -> 1.4 */
	if ($config['version'] == "1.3") {
		/* convert shaper rules (make pipes) */
		if (is_array($config['shaper']['rule'])) {
			$config['shaper']['pipe'] = array();
			
			for ($i = 0; isset($config['shaper']['rule'][$i]); $i++) {
				$curent = &$config['shaper']['rule'][$i];
				
				/* make new pipe and associate with this rule */
				$newpipe = array();
				$newpipe['descr'] = $curent['descr'];
				$newpipe['bandwidth'] = $curent['bandwidth'];
				$newpipe['delay'] = $curent['delay'];
				$newpipe['mask'] = $curent['mask'];
				$config['shaper']['pipe'][$i] = $newpipe;
				
				$curent['targetpipe'] = $i;
				
				unset($curent['bandwidth']);
				unset($curent['delay']);
				unset($curent['mask']);
			}
		}
		
		$config['version'] = "1.4";
	}
	
	/* convert 1.4 -> 1.5 */
	if ($config['version'] == "1.4") {
		/* Remove old certs & keys */
		unlink_if_exists("{$g['vardb_path']}/ovpn_ca_cert.pem");
		unlink_if_exists("{$g['vardb_path']}/ovpn_srv_cert.pem");
		unlink_if_exists("{$g['vardb_path']}/ovpn_srv_key.pem");
		unlink_if_exists("{$g['vardb_path']}/ovpn_dh.pem");

		/* convert openvpn tunnels */
		if (isset($config['ovpn']['server'])) {
			$current = &$config['ovpn']['server'];

			/* construct new tunnel array */
			$config['ovpn']['server']['tunnel'] = array();

			/* make new pipe and associate with this rule */
			$newpipe = array();
			if (isset($current['enable']))
				$newpipe['enable'] = $current['enable'];
			$newpipe['descr'] = "";
			$newpipe['tun_iface'] = $current['tun_iface'];
			if (preg_match("/tun/", $current['tun_iface']))
				$newpipe['type'] = "tun";
			else
				$newpipe['type'] = "tap";
			$newpipe['psh_options'] = array();
			if (isset($current['psh_options']['redir']))
				$newpipe['psh_options']['redir'] = $current['psh_options']['redir'];
			if (isset($current['psh_options']['redir_loc']))
				$newpipe['psh_options']['redir_loc'] = $current['psh_options']['redir_loc'];

			if (isset($current['psh_options']['rtedelay'])) {
				$newpipe['psh_options']['rtedelay_int'] = $current['psh_options']['rtedelay'];
				$newpipe['psh_options']['rtedelay'] = true;
			}
			if (isset($current['psh_options']['inact'])) {
				$newpipe['psh_options']['inact_int'] = $current['psh_options']['inact'];
				$newpipe['psh_options']['inact'] = true;
			}
			if (isset($current['psh_options']['ping'])) {
				$newpipe['psh_options']['ping_int'] = $current['psh_options']['ping'];
				$newpipe['psh_options']['ping'] = true;
			}
			if (isset($current['psh_options']['pingexit'])) {
				$newpipe['psh_options']['pingexit_int'] = $current['psh_options']['pingexit'];
				$newpipe['psh_options']['pingexit'] = true;
			}
			if (isset($current['psh_options']['pingrst'])) {
				$newpipe['psh_options']['pingrst_int'] = $current['psh_options']['pingrst'];
				$newpipe['psh_options']['pingrst'] = true;
			}
			$newpipe['port'] = $current['port'];
			$newpipe['proto'] = strtolower($current['proto']);
			if (isset($current['maxcli']))
				$newpipe['maxcli'] = $current['maxcli'];
			$newpipe['crypto'] = $current['crypto'];
			if (isset($current['dupcn']))
				$newpipe['dupcn'] = $current['dupcn'];
			$newpipe['verb'] = $current['verb'];
			$newpipe['bind_iface'] = $current['bind_iface'];
			$newpipe['ipblock'] = $current['ipblock'];
			$newpipe['prefix'] = $current['prefix'];
			if (isset($current['cli2cli']))
				$newpipe['cli2cli'] = $current['cli2cli'];
			if (isset($current['dynip']))
				$newpipe['dynip'] = $current['dynip'];
			$newpipe['ca_cert'] = $current['ca_cert'];
			$newpipe['srv_key'] = $current['srv_key'];
			$newpipe['srv_cert'] = $current['srv_cert'];
			$newpipe['dh_param'] = $current['dh_param'];

			$config['ovpn']['server']['tunnel'][$i] = $newpipe;

			/* destroy old array */
			unset($current['enable']);
			unset($current['tun_iface']);
			unset($current['psh_options']);
			unset($current['port']);
			unset($current['proto']);
			unset($current['maxcli']);
			unset($current['crypto']);
			unset($current['dupcn']);
			unset($current['verb']);
			unset($current['bind_iface']);
			unset($current['ipblock']);
			unset($current['prefix']);
			unset($current['cli2cli']);
			unset($current['dynip']);
			unset($current['ca_cert']);
			unset($current['srv_key']);
			unset($current['srv_cert']);
			unset($current['dh_param']);
		}

		$config['version'] = "1.5";
	}
	
	/* convert 1.5 -> 1.6 */
	if ($config['version'] == "1.5") {
	
		/* Remove OpenVPN configuration */
		unset($config['ovpn']);
		
		/* Remove OpenVPN interfaces */
		for ($i = 1; isset($config['interfaces']['opt' . $i]); $i++) {
			if (isset($config['interfaces']['opt' . $i]['ovpn'])) {
			
				unset($config['interfaces']['opt' . $i]);

				/* shift down other OPTn interfaces to get rid of holes */
				$j = $i + 1;
				
				/* look at the following OPTn ports */
				while (is_array($config['interfaces']['opt' . $j])) {
					$config['interfaces']['opt' . ($j - 1)] =
						$config['interfaces']['opt' . $j];
					
					if ($config['interfaces']['opt' . ($j - 1)]['descr'] == "OPT" . $j)
						$config['interfaces']['opt' . ($j - 1)]['descr'] = "OPT" . ($j - 1);
					
					unset($config['interfaces']['opt' . $j]);
					$j++;
				}
			}
		}

		$config['version'] = "1.6";
	}
	
	write_config();
	
	if ($g['booting'])
		echo "done\n";
}

/* save the system configuration */
function write_config() {

	global $config, $g;

	config_lock();

	conf_mount_rw();
	
	if (time() > mktime(0, 0, 0, 9, 1, 2004))	/* make sure the clock settings is plausible */
		$config['lastchange'] = time();
	
	/* generate configuration XML */
	$xmlconfig = dump_xml_config($config, $g['xml_rootobj']);
	
	/* write configuration */
	$fd = fopen("{$g['cf_conf_path']}/config.xml", "w");
	
	if (!$fd)
		die("Unable to open config.xml for writing in write_config()\n");
		
	fwrite($fd, $xmlconfig);
	fclose($fd);
	
	conf_mount_ro();
	
	/* re-read configuration */
	$config = parse_xml_config("{$g['conf_path']}/config.xml", $g['xml_rootobj']);
	
	/* write config cache */
	$fd = @fopen("{$g['tmp_path']}/config.cache", "wb");
	if ($fd) {
		fwrite($fd, serialize($config));
		fclose($fd);
	}
	
	config_unlock();
}

function reset_factory_defaults() {
	
	global $g;
	
	config_lock();
	
	conf_mount_rw();
	
	/* create conf directory, if necessary */
	if (!file_exists("{$g['cf_conf_path']}"))
		@mkdir("{$g['cf_conf_path']}");
	
	/* clear out /conf */
	$dh = opendir($g['conf_path']); 
	while ($filename = readdir($dh)) {
		if (($filename != ".") && ($filename != "..")) { 
			unlink($g['conf_path'] . "/" . $filename); 
		} 
	}
	closedir($dh);
	
	/* copy default configuration */
	@copy("{$g['conf_default_path']}/config.xml", "{$g['conf_path']}/config.xml");
	
	conf_mount_ro();
	
	config_unlock();
	
	return 0;
}

function config_install($conffile) {

	global $config, $g;
	
	if (!file_exists($conffile))
		return 1;

	if (!config_validate($conffile))
		return 1;

	config_lock();
	conf_mount_rw();
	
	copy($conffile, "{$g['conf_path']}/config.xml");
	
	conf_mount_ro();
	config_unlock();
	
	return 0;
}

function config_validate($conffile) {

	global $g, $xmlerr;

	$xml_parser = xml_parser_create();
	
	if (!($fp = fopen($conffile, "r"))) {
		$xmlerr = "XML error: unable to open file";
		return false;
	}
	
	while ($data = fread($fp, 4096)) {
		if (!xml_parse($xml_parser, $data, feof($fp))) {
			$xmlerr = sprintf("%s at line %d",
						xml_error_string(xml_get_error_code($xml_parser)),
						xml_get_current_line_number($xml_parser));
			return false;
		}
	}
	xml_parser_free($xml_parser);
	
	fclose($fp);
	
	return true;
}

/* lock configuration file, decide that the lock file is stale after
   10 seconds */
function config_lock() {
	
	global $g;
	
	$lockfile = "{$g['varrun_path']}/config.lock";
	
	$n = 0;
	while ($n < 10) {
		/* open the lock file in append mode to avoid race condition */
		if ($fd = @fopen($lockfile, "x")) {
			/* succeeded */
			fclose($fd);
			return;
		} else {
			/* file locked, wait and try again */
			sleep(1);
			$n++;
		}
	}
}

/* unlock configuration file */
function config_unlock() {
	
	global $g;
	
	$lockfile = "{$g['varrun_path']}/config.lock";
	
	if (file_exists($lockfile))
		unlink($lockfile);
}

?>
