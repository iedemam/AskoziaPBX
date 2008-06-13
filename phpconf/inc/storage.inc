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
		mwexec("/sbin/umount -a");
		storage_syspart_update_bsdlabel();
		mwexec("/sbin/mount -a");
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
	mwexec("/sbin/mount -o noatime /dev/" . storage_syspart_get_device() . "d /storage");
}

function storage_syspart_unmount() {
	mwexec("/bin/sync");
	mwexec("/sbin/umount -f /storage");
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









function storage_disk_get_devices() {
	return explode(" ", trim(preg_replace("/kern.disks: /", "", exec("/sbin/sysctl kern.disks"))));
}

function storage_disk_get_name($device) {
	
	$dmesg = system_get_dmesg_as_string();
	$matches = array();
	if (preg_match("/$device:.+<.+>/", $dmesg, &$matches)) {
		$start = strpos($matches[0], "<") + 1;
		$name = substr($matches[0], $start, strpos($matches[0], ">") - $start);
	} else {
		$name = "unavailable";
	}
	
	return $name;
}

?>
