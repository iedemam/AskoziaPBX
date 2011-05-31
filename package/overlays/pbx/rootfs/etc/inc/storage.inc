<?php
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2011 tecema (a.k.a IKT) <http://www.tecema.de>.
	All rights reserved.
	
	Askozia®PBX is a registered trademark of tecema. Any unauthorized use of
	this trademark is prohibited by law and international treaties.
	
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
	
	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.
	
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	
	3. Redistribution in any form at a charge, that in whole or in part
	   contains or is derived from the software, including but not limited to
	   value added products, is prohibited without prior written consent of
	   tecema.
	
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


function storage_cdrom_get_devices() {
	return explode(" ", trim(preg_replace("/drive name: /", "", exec("/sbin/sysctl -n dev.cdrom.info |grep \"drive name\""))));
}

function storage_disk_get_devices() {
	return explode(" ", trim(exec("/bin/ls /dev | grep '[s|h]d[a-z]' | tr \"\n\" \" \"")));
}

function storage_get_disks() {
	$all_disks = $GLOBALS['config']['storage']['disk'];
	return is_array($all_disks) ? $all_disks : array();
}
function storage_get_previously_used_disks() {
	// find all of the disks available
	$devices = storage_get_unassigned_devices();

	// make a mount point, then loop through all relevant disks and look for the askoziapbx folder on them
	exec("mkdir -p /mnt/tmpprevdisk");
	openlog("storage_get_previously_used_disks()", LOG_INFO, LOG_LOCAL0);
	if(count($devices) > 0)
	{
		foreach ($devices as $device=>$description) {
			exec("mount -t vfat " . $device . "1 /mnt/tmpprevdisk -o noatime", $out, $retval);
			if ($retval != 0) {
				syslog(LOG_INFO, "Mounting " . $device . "1 on /mnt/tmpprevdisk returned " . $retval);
			} else {
				syslog(LOG_INFO, "Mounted " . $device . "1 on /mnt/tmpprevdisk");
			}
	
			if (file_exists("/mnt/tmpprevdisk/askoziapbx")) {
				$prev_disks[] = $device;
				syslog(LOG_INFO, "Previously used AskoziaPBX disk recognized on " . $device . "1");
			} else {
				syslog(LOG_INFO, "Disk does not appear to be an AskoziaPBX disk.");
			}
			exec("umount /mnt/tmpprevdisk");
			sleep(1);
		}
	}
	closelog();
	exec("rmdir /mnt/tmpprevdisk");

	return is_array($prev_disks) ? $prev_disks : array();
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
	v_checkbox('astlogs', &$disk, &$post);
	v_checkbox('faxarchive', &$disk, &$post);
	v_checkbox('voicemailarchive', &$disk, &$post);

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

	} else if ($service == 'astlogs') {
		exec("mkdir " . $servicepath . "/asterisk");
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
