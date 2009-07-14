<?php
/*
	$Id$
	originally part of m0n0wall (http://m0n0.ch/wall)
	continued modifications as part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
	Copyright (C) 2007-2009 IKT <http://itison-ikt.de>.
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
	$g['platform'] = chop(file_get_contents("{$g['etc_path']}/platform"));
} else {
	$g['platform'] = "unknown";
}

if (file_exists("{$g['etc_path']}/brand.product")) {
	$product_name = chop(file_get_contents("{$g['etc_path']}/brand.product"));
} else {
	$product_name = "AskoziaPBX";
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
			
			if (mwexec("/bin/mount -t msdos -r /dev/da0s1 {$g['cf_path']}") == 0) {
				/* USB stick is OK */
				echo "Using USB memory stick for configuration storage.\n";
				mwexec("/bin/umount -f {$g['cf_path']}");
			} else {
				$cfgdevice = $cfgpartition = "fd0";
				
				if (mwexec("/bin/mount -t msdos -r /dev/fd0 {$g['cf_path']}") != 0) {
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
					mwexec("/bin/umount -f {$g['cf_path']}");
				}
			}
		} else {
			/* iterate through the disks fdisk finds until one contains a config.xml */
			exec('fdisk -l | grep "^/dev/" | cut -c 6-8 | uniq', $disks);
			foreach ($disks as $disk) {
				if (mwexec("/bin/mount -r /dev/{$disk}1 {$g['cf_path']}") == 0) {
					if (file_exists("{$g['cf_conf_path']}/config.xml")) {
						/* found it */
						$cfgdevice = $disk;
						$cfgpartition = $cfgdevice . "1";
						$cfgfstype = "ext2";
						echo "Found configuration on $cfgdevice.\n";
					}
					
					mwexec("/bin/umount -f {$g['cf_path']}");
					
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
* found. $product_name cannot continue booting.                                  *
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
		$fstab .= "/dev/{$cfgdevice}2 /offload ext2 ro 0 0\n";
		
		fwrite($fd, $fstab);
		fclose($fd);
	}
	
	/* mount all filesystems */
	mwexec("/bin/mount -a");
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
		$uniqid_map = unserialize(file_get_contents("{$g['tmp_path']}/uniqid_map.cache"));
	} else {
	
		if (!file_exists("{$g['conf_path']}/config.xml")) {
			if ($g['booting']) {
				if (strstr($g['platform'], "cdrom")) {
					/* try copying the default config. to the floppy/USB stick */
					reset_factory_defaults();
					
					echo "No XML configuration file found - using factory defaults.\n";
				} else {
					echo "XML configuration file not found. $product_name cannot continue booting.\n"; 
					mwexec("/sbin/halt");
					exit;
				}
			} else {
				config_unlock();
				exit(0);
			}
		}
		
		$config = xml_to_array("{$g['conf_path']}/config.xml", $g['xml_rootobj']);
		
		if ((float)$config['version'] > (float)$g['latest_config']) {
			if ($g['booting']) {
				echo <<<EOD


*******************************************************************************
* WARNING!                                                                    *
* The current configuration has been created with a newer version of          *
* $product_name than this one! This can lead to serious misbehavior and even     *
* security holes! You are urged to either upgrade to a newer version of       *
* $product_name or revert to the default configuration immediately!              *
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
		
		/* write uniqid_map cache */
		$uniqid_map = config_build_uniqid_map($config);
		$fd = @fopen("{$g['tmp_path']}/uniqid_map.cache", "wb");
		if ($fd) {
			fwrite($fd, serialize($uniqid_map));
			fclose($fd);
		}
	}
	
	config_unlock();

}

/**
 * Mounts the flash card read/write
 */
function conf_mount_rw() {
	global $g;

	/* don't use mount -u anymore
	   (doesn't sync the files properly and /bin/sync won't help either) */
	mwexec("/bin/umount -f {$g['cf_path']}");
	mwexec("/bin/mount -w -o noatime {$g['cf_path']}");
}

/**
 * Mounts the flash card read only
 */
function conf_mount_ro() {
	global $g;
	
	mwexec("/bin/umount -f {$g['cf_path']}");
	mwexec("/bin/mount -r {$g['cf_path']}");
}

/**
 * Converts the configuration structure if updates require.
 */
function convert_config() {
	global $config, $g;
	
	if ($config['version'] == $g['latest_config'])
		return;		/* already at latest version */
	
	if ($g['booting'])
		echo "Converting configuration... ";
	
	/* convert 1.0 -> 1.1 */
	if ($config['version'] == "1.0") {

		// update sip provider prefixes and patterns
		$n = count($config['sip']['provider']);
		for($i = 0; $i < $n; $i++) {
			$p = &$config['sip']['provider'][$i];
			
			$p['dialpattern'] = array();
			if ($p['prefix']) {
				$p['dialpattern'][] = $p['prefix'] . "|.";
				unset($p['prefix']);
			}
			if ($p['pattern']) {
				$p['dialpattern'][] = $p['pattern'];
				unset($p['pattern']);
			}
		}

		// update iax provider prefixes and patterns
		$n = count($config['iax']['provider']);
		for($i = 0; $i < $n; $i++) {
			$p = &$config['iax']['provider'][$i];
			
			$p['dialpattern'] = array();
			if ($p['prefix']) {
				$p['dialpattern'][] = $p['prefix'] . "|.";
				unset($p['prefix']);
			}
			if ($p['pattern']) {
				$p['dialpattern'][] = $p['pattern'];
				unset($p['pattern']);
			}
		}
		
		$config['version'] = "1.1";
	}
	
	/* convert 1.1 -> 1.2 */
	if ($config['version'] == "1.1") {
		
		// add default applications to new mapper
		$apps[] = array(
				"name" => "Echo",
				"extension" => "00003246",
				"uniqid" => "APPLICATION-MAPPING-83606076846f835d8828a4");

		$apps[] = array(
				"name" => "Milliwatt",
				"extension" => "000064554",
				"uniqid" => "APPLICATION-MAPPING-48856682646f8381701993");

		$apps[] = array(
				"name" => "WakeMe",
				"extension" => "00009253",
				"uniqid" => "APPLICATION-MAPPING-202074715246f8385a7a820");
		
		$config['dialplan']['application'] = $apps;
		
		$config['version'] = "1.2";
	}
	
	/* convert 1.2 -> 1.3 */
	if ($config['version'] == "1.2") {

		// convert incomingextensions to new representation
		$techs = array("sip", "iax", "isdn", "analog");
		foreach ($techs as $tech) {
			if (!isset($config[$tech]['provider'])) continue; // missed this the first time...
			$n = count($config[$tech]['provider']);
			for($i = 0; $i < $n; $i++) {
				$p = &$config[$tech]['provider'][$i];
				
				if ($p['incomingextension']) {
					$destinationid = $p['incomingextension'];
					$p['incomingextensionmap'] = array();
					$p['incomingextensionmap'][0]['destinationid'] = $destinationid;
				}
				unset($p['incomingextension']);
			}
		}
		
		$config['version'] = "1.3";
	}
	
	/* convert 1.3 -> 1.4 */
	if ($config['version'] == "1.3") {

		// clean up bug from 1.2 -> 1.3 upgrade
		$techs = array("sip", "iax", "isdn", "analog");
		foreach ($techs as $tech) {
			if (isset($config[$tech]['provider']) && (count($config[$tech]['provider'], COUNT_RECURSIVE) == 1)) {
				unset($config[$tech]['provider']);
				if ($g['booting']) {
					echo "- fixed $tech config\n";
				}
			}
		}
		
		$config['version'] = "1.4";
	}

	/* convert 1.4 -> 1.5 */
	if ($config['version'] == "1.4") {

		// convert applications to new representation
		if (is_array($config['dialplan']['application'])) {
			$n = count($config['dialplan']['application']);
			for($i = 0; $i < $n; $i++) {
				$a = &$config['dialplan']['application'][$i];
				
				if ($a['name'] == "Echo") {
					$a['app-command'] = array(
						base64_encode("1,Answer()"),
						base64_encode("2,Echo()"),
						base64_encode("3,Hangup()")
					);
					$a['descr'] = "Echos audio and video back to the caller as soon as it is received. Used to test connection delay.";

				} else if ($a['name'] == "Milliwatt") {
					$a['app-command'] = array(
						base64_encode("1,Answer()"),
						base64_encode("2,Milliwatt()"),
						base64_encode("3,Hangup()")
					);
					$a['descr'] = "Generates a constant 1000Hz tone. Used to test connection quality.";

				} else if ($a['name'] == "WakeMe") {
					$a['app-command'] = array(
						base64_encode("1,Answer()"),
						base64_encode("2,Set(CHANNEL(language)=en)"),
						base64_encode("3,WakeMe()"),
						base64_encode("4,Hangup()")
					);
					$a['descr'] = "Wake-Up call manager. Call to set and cancel wake-up calls on the current extension.";
				}
			}
		}

		$config['version'] = "1.5";
	}

	write_config();
	
	if ($g['booting'])
		echo "done\n";
}

/**
 * Saves the system configuration and uniqid mapping
 */
function write_config() {

	global $config, $g;

	config_lock();

	conf_mount_rw();
	
	if (time() > mktime(0, 0, 0, 9, 1, 2004))	/* make sure the clock settings are plausible */
		$config['lastchange'] = time();
	
	/* generate configuration XML */
	$xmlconfig = array_to_xml($config, $g['xml_rootobj']);
	
	/* write configuration */
	$fd = fopen("{$g['cf_conf_path']}/config.xml", "w");
	
	if (!$fd)
		die("Unable to open config.xml for writing in write_config()\n");
		
	fwrite($fd, $xmlconfig);
	fclose($fd);
	
	conf_mount_ro();
	
	/* re-read configuration */
	$config = xml_to_array("{$g['conf_path']}/config.xml", $g['xml_rootobj']);
	/* build uniqid_map */
	$uniqid_map = config_build_uniqid_map($config);
	
	
	/* write config cache */
	$fd = @fopen("{$g['tmp_path']}/config.cache", "wb");
	if ($fd) {
		fwrite($fd, serialize($config));
		fclose($fd);
	}
	
	/* write uniqid_map cache */
	$fd = @fopen("{$g['tmp_path']}/uniqid_map.cache", "wb");
	if ($fd) {
		fwrite($fd, serialize($uniqid_map));
		fclose($fd);
	}
	
	
	config_unlock();
}

/**
 * Builds a mapping of uniqids to their array offsets to speed up element lookups
 */
function config_build_uniqid_map($config) {
	global $g;

	// providers and internal phones
	foreach ($g['technologies'] as $tech) {
		$n = count($config[$tech]['provider']);
		for ($i = 0; $i < $n; $i++) {
			$uniqid_map[$config[$tech]['provider'][$i]['uniqid']] = $i;
		}
		$n = count($config[$tech]['phone']);
		for ($i = 0; $i < $n; $i++) {
			$uniqid_map[$config[$tech]['phone'][$i]['uniqid']] = $i;
		}
	}

	// external phones
	$n = count($config['external']['phone']);
	for ($i = 0; $i < $n; $i++) {
		$uniqid_map[$config['external']['phone'][$i]['uniqid']] = $i;
	}
	
	// conferencing
	$n = count($config['conferencing']['room']);
	for ($i = 0; $i < $n; $i++) {
		$uniqid_map[$config['conferencing']['room'][$i]['uniqid']] = $i;
	}
	
	// callgroups
	$n = count($config['dialplan']['callgroup']);
	for ($i = 0; $i < $n; $i++) {
		$uniqid_map[$config['dialplan']['callgroup'][$i]['uniqid']] = $i;
	}
	
	// applications
	$n = count($config['dialplan']['application']);
	for ($i = 0; $i < $n; $i++) {
		$uniqid_map[$config['dialplan']['application'][$i]['uniqid']] = $i;
	}
	
	return $uniqid_map;
}

/**
 * Overwrites the existing config.xml file with factory default values.
 */
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
	if (strstr($g['platform'], "askozia-pbx-generic-pc")) {
		$plname = "generic-pc";
	} else {
		$plname = $g['platform'];
	}
	@copy("{$g['conf_default_path']}/config.{$plname}.xml", "{$g['conf_path']}/config.xml");
	
	conf_mount_ro();
	
	config_unlock();
	
	return 0;
}

/**
 * Installs the specified configuration file into the global "conf path"
 */
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

/**
 * Performs an XML validation of the configuration file.
 */
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

/**
 * Locks access to the configuration file.
 * Lock is invalidated after 10 seconds.
 */
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

/**
 * Unlocks access to the configuration file
 */
function config_unlock() {
	
	global $g;
	
	$lockfile = "{$g['varrun_path']}/config.lock";
	
	if (file_exists($lockfile))
		unlink($lockfile);
}

?>
