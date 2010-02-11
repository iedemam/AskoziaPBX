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

		if (file_exists("/offload/livecd")) {
			$livecd = true;
			/* config is either on floppy disk or USB stick for CD-ROM version */

			/* try the USB stick at da0 first, and if this fails, use the floppy disk  */
			//$cfgdevice = "da0";
			//$cfgpartition = "da0s1";
			//$cfgfstype = "msdos";
            //
			//if (mwexec("/bin/mount -t msdos -r /dev/da0s1 {$g['cf_path']}") == 0) {
			//	/* USB stick is OK */
			//	echo "Using USB memory stick for configuration storage.\n";
			//	mwexec("/bin/umount -f {$g['cf_path']}");
			//} else {
			//	$cfgdevice = $cfgpartition = "fd0";
            //
			//	if (mwexec("/bin/mount -t msdos -r /dev/fd0 {$g['cf_path']}") != 0) {
					echo <<<EOD


*******************************************************************************
* WARNING!                                                                    *
* No FAT formatted USB memory stick or floppy disk could be found.            *
* Your configuration changes will be lost on reboot.                          *
*******************************************************************************


EOD;
					sleep(5);

			//	} else {
			//		echo "Using floppy disk for configuration storage.\n";
			//		mwexec("/bin/umount -f {$g['cf_path']}");
			//	}
			//}
		} else {
			/* iterate through the disks fdisk finds until one contains a config.xml */
			$attempts = 2;
			while ($attempts--) {
				if ($g['platform'] == "COMpact 3000") {
					$disks[] = "mmcblk0";
				} else {
					exec('fdisk -l | grep "^/dev/" | cut -c 6-8 | uniq', $disks);
				}
				echo "Found disks: " . implode(", ", $disks) . "\n";
				foreach ($disks as $disk) {
					echo "Looking for configuration on: $disk\n";
					if (strpos($disk, "mmcblk") !== false) {
						$part = "p1";
					} else {
						$part = "1";
					}
					if (mwexec("/bin/mount -r /dev/{$disk}{$part} {$g['cf_path']}") == 0) {
						if (file_exists("{$g['cf_conf_path']}/config.xml")) {
							$cfgdevice = $disk;
							$cfgpartition = $cfgdevice . $part;
							echo "Found configuration on $cfgdevice.\n";
						}

						mwexec("/bin/umount -f {$g['cf_path']}");

						if ($cfgdevice) {
							break;
						}
					}
				}

				if ($cfgdevice) {
					break;
				}

				if ($attempts) {
					echo "Configuration not found.\n";
					echo "Waiting for devices to settle and attempting again.\n";
					sleep(5);
				}
			}
		}

		if (!$cfgdevice && !$livecd) {
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
		$fd = fopen("/etc/fstab", "w");

		$fstab = "/dev/{$cfgpartition} {$g['cf_path']} msdos ro 1 1\n";
		if (strpos($disk, "mmcblk") !== false) {
			$part = "p2";
		} else {
			$part = "2";
		}
		$fstab .= "/dev/{$cfgdevice}{$part} /offload ext2 ro 0 0\n";

		fwrite($fd, $fstab);
		fclose($fd);

		/* create a copy for the fstab generator to read in */
		copy("/etc/fstab", "/etc/fstab.boot");
	}

	/* mount all filesystems */
	mwexec("/bin/mount -a");
}

/* parse configuration */
if (!$noparseconfig) {

	config_lock();

	/* see if there's a newer cache file */
	if (file_exists("{$g['tmp_path']}/configca.che") && 
		(filemtime("{$g['tmp_path']}/configca.che") >= 
		 filemtime("{$g['conf_path']}/config.xml"))) {

		/* read cache */
		$config = unserialize(file_get_contents("{$g['tmp_path']}/configca.che"));
		$uniqid_map = unserialize(file_get_contents("{$g['tmp_path']}/uniqidca.che"));
	} else {

		if (!file_exists("{$g['conf_path']}/config.xml")) {
			if ($g['booting']) {
				if ($livecd) {
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
		$fd = @fopen("{$g['tmp_path']}/configca.che", "wb");
		if ($fd) {
			fwrite($fd, serialize($config));
			fclose($fd);
		}

		/* write uniqid_map cache */
		$uniqid_map = config_build_uniqid_map($config);
		$fd = @fopen("{$g['tmp_path']}/uniqidca.che", "wb");
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

	/* already at latest version */
	if ($config['version'] == $g['latest_config']) {
		return;
	}

	/* 
	convert 1.5 -> 1.6
		- upgrade to new ntp interval method
	*/
	if ($config['version'] == "1.5") {
		$config['system']['time-update-interval'] = "4-hours";
		$config['version'] = "1.6";
	}

	write_config();

}

/**
 * Saves the system configuration and uniqid mapping
 */
function write_config() {

	global $config, $g, $uniqid_map;

	config_lock();

	conf_mount_rw();

	/* make sure the clock settings are plausible */
	if (time() > mktime(0, 0, 0, 9, 1, 2004)) {
		$config['lastchange'] = time();
	}

	/* generate configuration XML */
	$xmlconfig = array_to_xml($config, $g['xml_rootobj']);

	/* write configuration */
	$fd = fopen("{$g['cf_conf_path']}/config.xml", "w");

	if (!$fd) {
		die("Unable to open config.xml for writing in write_config()\n");
	}

	fwrite($fd, $xmlconfig);
	fclose($fd);

	conf_mount_ro();

	/* re-read configuration */
	$config = xml_to_array("{$g['conf_path']}/config.xml", $g['xml_rootobj']);
	/* build uniqid_map */
	$uniqid_map = config_build_uniqid_map($config);

	/* write config cache */
	$fd = @fopen("{$g['tmp_path']}/configca.che", "wb");
	if ($fd) {
		fwrite($fd, serialize($config));
		fclose($fd);
	}

	/* write uniqid_map cache */
	$fd = @fopen("{$g['tmp_path']}/uniqidca.che", "wb");
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

	// dahdi ports
	$n = count($config['interfaces']['dahdi-port']);
	for ($i = 0; $i < $n; $i++) {
		$uniqid_map[$config['interfaces']['dahdi-port'][$i]['uniqid']] = $i;
	}

	// redfone gateways
	$n = count($config['interfaces']['redfone-gateway']);
	for ($i = 0; $i < $n; $i++) {
		$uniqid_map[$config['interfaces']['redfone-gateway'][$i]['uniqid']] = $i;
	}

	// storage disks
	$n = count($config['storage']['disk']);
	for ($i = 0; $i < $n; $i++) {
		$uniqid_map[$config['storage']['disk'][$i]['uniqid']] = $i;
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
	if (!file_exists("{$g['cf_conf_path']}")) {
		@mkdir("{$g['cf_conf_path']}");
	}

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

/**
 * Installs the specified configuration file into the global "conf path"
 */
function config_install($conffile) {

	global $config, $g;

	if (!file_exists($conffile)) {
		return 1;
	}

	if (!config_validate($conffile)) {
		return 1;
	}

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

	if (file_exists($lockfile)) {
		unlink($lockfile);
	}
}

?>
