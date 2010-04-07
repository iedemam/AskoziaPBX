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

require_once("functions.inc");

function storage_syspart_initialize() {
	global $g, $defaults;

	/*printf("LINUX TODO: empty storage_syspart_initialize()\n");*/ return 0;

	if (strstr($g['platform'], "cdrom")) {
		return 0;
	}

	if (!storage_syspart_media_is_large_enough()) {
		echo "WARNING: System media not large enough for storage partition!\n";
		echo " - a minimum of " . $defaults['storage']['system-media-minimum-size'] . "MB is required\n";
		return 0;
	}

	// secure a copy of the original bsdlabel in the mfs
	copy("{$g['cf_path']}/original.bsdlabel", "{$g['tmp_path']}/original.bsdlabel");

	$device = storage_syspart_get_device();

	echo "Initializing system storage partition... \n";

	if (storage_syspart_has_default_bsdlabel()) {
		echo " - generating bsdlabel...";
		mwexec("/bin/umount -a");
		storage_syspart_update_bsdlabel();
		mwexec("/bin/mount -a");
		echo "done.\n";
	}

	if (!file_exists("/dev/$device" . "d")) {
		echo "failed! (/dev/$device" . "d not found)\n";
		return 1;
	}

	if (!storage_syspart_is_formatted()) {
		echo " - formatting storage partition...";
		storage_syspart_format();
		echo "done.\n";
	}

	if (storage_syspart_is_dirty()) {
		echo " - WARNING: file system errors detected, attempting to repair...";
		storage_syspart_repair();
		echo "done.\n";
	}

	storage_syspart_mount();
	echo "done.\n";

	return 0;
}

function storage_syspart_media_is_large_enough() {
	global $defaults;

	$capacity = storage_syspart_get_size() / 1024 / 1024;

	return $capacity > $defaults['storage']['system-media-minimum-size'];
}

function storage_syspart_has_default_bsdlabel() {

	$label = storage_read_bsdlabel(storage_syspart_get_device());

	return !isset($label['d']);
}

function storage_syspart_update_bsdlabel() {
	global $defaults, $g;

	$device = storage_syspart_get_device();
	$label = storage_read_bsdlabel($device);
	$standard_label = storage_read_bsdlabel($device, "standard");

	$a_size = $label['a']['size'];
	$b_size = $label['b']['size'];
	$c_size = $standard_label['c']['size'];
	$d_offset = $defaults['storage']['system-partition-offset-megabytes'] * 1024 * 2;
	$d_size = $c_size - $d_offset;

	$label  = "# /dev/$device:\n";
	$label .= "8 partitions:\n";
	$label .= "#       size         offset      fstype   [fsize bsize bps/cpg]\n";
	$label .= " a: " . $a_size . "      16     4.2BSD     0     0\n";
	$label .= " b: " . $b_size . "       *     4.2BSD     0     0\n";
	$label .= " c: " . $c_size . "       0     unused     0     0\n";
	$label .= " d: " . $d_size . "  $d_offset  4.2BSD     0     0\n\n";

	// write label to tmp
	$label_file = "{$g['tmp_path']}/tmp.bsdlabel";
	$fd = fopen("$label_file", "w");
	if (!$fd) {
		printf("Error: cannot open temporary bsdlabel output file!\n");
		exit(1);
	}
	fwrite($fd, $label);
	fclose($fd);

	// apply label
	mwexec("/sbin/bsdlabel -R $device $label_file");

	// remove temporary label file
	unlink($label_file);

	return 0;
}

function storage_syspart_is_formatted() {
	global $defaults;

	$device = storage_syspart_get_device();	

	// offset from /dev/ad0x : 64 KB of reserved space + 1372 bytes offset into struct fs (see /sys/ufs/ffs/fs.h).
	$magic_offset = 66908;
	$magic_number = "19 01 54 19";

	exec("/usr/bin/hexdump -C -s " . $magic_offset . " -n 4 /dev/" . $device . "d", $output);

	return (strpos($output[0], $magic_number) !== false) ? true : false;
}

function storage_syspart_format() {
	return mwexec("/sbin/newfs -b 8192 -f 1024 -o space /dev/" . storage_syspart_get_device() . "d");
}

function storage_syspart_is_dirty() {
	return mwexec("/sbin/fsck -p /dev/" . storage_syspart_get_device() . "d");
}

function storage_syspart_repair() {
	return mwexec("/sbin/fsck -y /dev/" . storage_syspart_get_device() . "d");
}

function storage_syspart_mount() {
	mwexec("/bin/mount -o noatime /dev/" . storage_syspart_get_device() . "d /storage");
}

function storage_syspart_unmount() {
	mwexec("/bin/sync");
	mwexec("/bin/umount -f /storage");
}

function storage_syspart_get_device() {
	global $g;

	return chop(file_get_contents("{$g['varetc_path']}/cfdevice"));
}

function storage_syspart_get_usage() {

	$device = storage_syspart_get_device();
	exec("/bin/df", $output);
	foreach ($output as $o) {
		if (strstr($o, "/dev/" . $device . "d")) {
			$o = preg_split("/\s+/", $o);
			return rtrim($o[4], "%");
		}
	}
}

function storage_syspart_get_size() {

	$diskinfo = storage_read_diskinfo(storage_syspart_get_device());

	return $diskinfo['sizeinbytes'];
}

function storage_syspart_get_state() {
	return storage_syspart_has_default_bsdlabel() ? "inactive" : "active";
}

/* XXX : some of these functions refer to the entire disk and others to the partition */
function storage_syspart_get_info() {

	$info['device'] = storage_syspart_get_device();
	$info['usage'] = storage_syspart_get_usage();
	$info['size'] = storage_syspart_get_size();
	$info['state'] = storage_syspart_get_state();
	$info['packages'] = packages_get_packages_assigned_to_media("syspart");

	return $info;
}

function storage_get_media_paths() {

	$paths = array();
	$mediums[] = "syspart";

	foreach ($mediums as $media) {
		if ($path = storage_get_media_path($media)) {
			$paths[] = storage_get_media_path($media);
		}
	}

	return $paths;
}

function storage_get_media_path($media) {

	if ($media == "syspart") {
		return "/storage";
	}

	return false;
}

function storage_read_bsdlabel($device, $standard=false) {

	$command = "/sbin/bsdlabel ";
	if ($standard) {
		$command .= "-wn ";
	}
	exec("$command $device 2>/dev/null", $output);

	foreach ($output as $line) {
		if (!preg_match("/^\s+[a-z]:/", $line)) {
			continue;
		}

		$elements = preg_split("/\s+/", $line);
		$partition = $elements[1][0];
		$info[$partition]['size'] = $elements[2];
		$info[$partition]['offset'] = $elements[3];
	}

	return $info;
}

function storage_read_diskinfo($device) {

	$output = preg_split("/\s+/", exec("/usr/sbin/diskinfo $device"));

	$info['sectorsize'] = $output[1];
	$info['sizeinbytes'] = $output[2];
	$info['sizeinsectors'] = $output[3];

	return $info;
}







function storage_cdrom_get_devices() {
	return explode(" ", trim(preg_replace("/drive name: /", "", exec("/sbin/sysctl -n dev.cdrom.info |grep \"drive name\""))));
}

function storage_disk_get_devices() {
	return explode(" ", trim(exec("/bin/ls /dev | grep '[s|h]d[a-z]' | tr \"\n\" \" \"")));
}

function storage_disk_get_name($device) {
	
	$dmesg = system_get_dmesg_as_string();

	if (preg_match("/^$device:.*<(.+)>/m", $dmesg, $matches)) {
		$name = $matches[1];
	} else {
		$name = "unavailable";
	}
	
	return $name;
}











function storage_get_disks() {
	$all_disks = $GLOBALS['config']['storage']['disk'];
	return is_array($all_disks) ? $all_disks : array();
}

function storage_get_disk($uniqid) {
	global $uniqid_map;

	return $GLOBALS['config']['storage']['disk'][$uniqid_map[$uniqid]];
}

function storage_get_assigned_services($ignorethisdiskuniqid=false) {
	global $g;

	$disks = storage_get_disks();

	$assignedservices = array();
	foreach ($disks as $disk) {
		if ($disk['uniqid'] == $ignorethisdiskuniqid) {
			continue;
		}
		foreach ($g['storage_services'] as $servicetype) {
			if (isset($disk[$servicetype])) {
				$assignedservices[$servicetype] = $disk['name'];
			}
		}
	}

	return $assignedservices;
}

function storage_service_is_active($servicename) {

	$disks = storage_get_disks();

	foreach ($disks as $disk) {
		if (isset($disk[$servicename])) {
			return $disk;
		}
	}

	return false;
}

function storage_get_unassigned_devices() {

	$disks = storage_get_disks();

	/* store all recognized devices */
	exec("fdisk -l | grep \"Disk /dev/\"", $fdiskoutput);
	foreach ($fdiskoutput as $fdiskline) {
		$unassigned[substr($fdiskline, 5, 8)] = $fdiskline;
	}

	/* mark configured disks as assigned */
	foreach ($disks as $disk) {
		unset($unassigned[$disk['device']]);
	}

	/* mark system disk as assigned */
	unset($unassigned["/dev/" . chop(file_get_contents("/var/etc/cfdevice"))]);

	return $unassigned;
}

function storage_verify_disk($post, $errors) {

	$disk = array();

	v_cleanliness(&$post, &$errors);
	v_required('name', gettext('Name'), &$disk, &$post, &$errors);
	v_required('device', gettext('Disk Device'), &$disk, &$post, &$errors);
	v_required('mountpoint', gettext('Mount Point'), &$disk, &$post, &$errors);
	v_checkbox('media', &$disk, &$post);
	v_checkbox('persistence', &$disk, &$post);

	$trusted = array(
		'uniqid'
	);
	foreach ($trusted as $t) {
		$disk[$t] = $post[$t];
	}

	return $disk;
}

function storage_save_disk($disk) {
	global $g, $uniqid_map;

	if (storage_get_disk($disk['uniqid'])) {
		$GLOBALS['config']['storage']['disk'][$uniqid_map[$disk['uniqid']]] = $disk;
	} else {
		$GLOBALS['config']['storage']['disk'][] = $disk;
	}

	write_config();
	touch($g['storage_dirty_path']);
}

function storage_forget_disk($uniqid) {
	global $g, $uniqid_map;

	unset($GLOBALS['config']['storage']['disk'][$uniqid_map[$uniqid]]);

	write_config();
	touch($g['storage_dirty_path']);
}

function storage_format_disk($device) {

	openlog("storage_format_disk()", LOG_INFO, LOG_LOCAL0);

	// overwrite with fresh DOS partition table
	exec("echo \"o\n" .
	// create new
	"n\n" .
	// primary partition
	"p\n" .
	// number 1
	"1\n" .
	// from the beginning
	"\n" .
	// to the end
	"\n" .
	// change type
	"t\n" .
	// to FAT32
	"b\n" .
	// set active
	"a\n" .
	// partition 1
	"1\n" .
	// and write changes
	"w\n" .
	"\" | fdisk " . $device, $out, $retval);
	syslog(LOG_INFO, "fdisk returned " . $retval);

	sleep(1);

	exec("mkdosfs -n ASKOZIA " . $device . "1", $out, $retval);
	syslog(LOG_INFO, "mkdosfs returned " . $retval);
	closelog();

	return $retval;
}

function storage_mount_disk($uniqid) {
	$disk = storage_get_disk($uniqid);
	exec("mount " . $disk['device'] . "1 -o noatime", $out, $retval);
	return $retval;
}

function storage_unmount_disk($uniqid) {
	$disk = storage_get_disk($uniqid);
	exec("umount " . $disk['device'] . "1", $out, $retval);
	return $retval;
}

function storage_generate_default_disk() {
	$disk = array();

	$disk['name'] = gettext("Storage Disk");
	$disk['mountpoint'] = "/storage/usbdisk1";
	$disk['uniqid'] = "STORAGE-DISK-" . uniqid(rand());

	return $disk;
}

function storage_initialize_service($service, $disk) {
	global $g, $d_sysrebootreqd_path;

	$servicepath = $disk['mountpoint'] . "/askoziapbx/" . $service;
	exec("mkdir -p " . $servicepath);

	if ($service == 'media') {
		exec("mkdir " . $servicepath . "/moh");
		exec("cp -Rp /offload/asterisk/moh/* " . $servicepath . "/moh/");
		exec("mkdir " . $servicepath . "/sounds");
		exec("cp -Rp /offload/asterisk/sounds/* " . $servicepath . "/sounds/");
		/* operation not permitted on vfat system...workaround needed */
		//exec("ln -s " . $servicepath . "/sounds/en-us " . $servicepath . "/sounds/en");

	} else if ($service == 'persistence') {
		exec("cp -p /etc/asterisk/db/astdb " . $servicepath . "/");
	}


	if (!$g['booting']) {
		touch($d_sysrebootreqd_path);
	}
}

function storage_configure() {
	global $g;

	$conf = file_get_contents("/etc/fstab.boot");

	$disks = storage_get_disks();
	foreach ($disks as $disk) {
		$conf .= $disk['device'] . "1 " . $disk['mountpoint'] . " vfat rw 0 0\n";
		if (!file_exists($disk['mountpoint'])) {
			exec("mkdir -p " . $disk['mountpoint']);
		}
	}
	$conf .= "\n";
	util_file_write("/etc/fstab", $conf);
	sleep(1);

	openlog("storage_configure()", LOG_INFO, LOG_LOCAL0);
	foreach ($disks as $disk) {
		for ($i = 0; $i < 10; $i++) {
			if (file_exists($disk['device'] . "1")) {
				break;
			}
			syslog(LOG_INFO, "Device node " . $disk['device'] . "1 does not exist yet...sleeping (attempt " . $i . ")");
			sleep(1);
		}

		exec("mount -t vfat " . $disk['device'] . "1 -o noatime", $out, $retval);
		if ($retval != 0) {
			syslog(LOG_INFO, "Mounting " . $disk['device'] . "1 on " . $disk['mountpoint'] . " returned " . $retval);
		} else {
			syslog(LOG_INFO, "Mounted " . $disk['device'] . "1 on " . $disk['mountpoint']);
		}

		if (!file_exists($disk['mountpoint'] . "/askoziapbx/tmp")) {
			exec("mkdir -p " . $disk['mountpoint'] . "/askoziapbx/tmp");
		}
		foreach ($g['storage_services'] as $service) {
			if (isset($disk[$service])) {
				$servicepath = $disk['mountpoint'] . "/askoziapbx/" . $service;
				if (!file_exists($servicepath)) {
					storage_initialize_service($service, $disk);
				}
			}
		}
	}
	closelog();

	unlink_if_exists($g['storage_dirty_path']);
}

?>
