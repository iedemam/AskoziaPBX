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
					echo "XML configuration file not found. AskoziaPBX cannot continue booting.\n"; 
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
