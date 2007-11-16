<?php
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
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

require_once("functions.inc");

function storage_get_configured_disks() {
	global $config;
	
	if (!is_array($config['interfaces']['storage-disk'])) {
		$config['interfaces']['storage-disk'] = array();
	}

	return $config['interfaces']['storage-disk'];
}

function storage_get_configured_disk($disk) {
	
	$disks = storage_get_configured_disks();
	
	foreach ($disks as $d) {
		if ($d['device'] == $disk) {
			return $d;
		}
	}
}

function storage_get_recognized_disks() {

	$valid_disks = array();
	$system_disk = storage_get_system_disk();

	$all_disks = explode(" ", trim(preg_replace("/kern.disks: /", "", exec("/sbin/sysctl kern.disks"))));
	foreach ($all_disks as $disk) {
		/* skip mfs mounted filesystems */
		if (!strstr($mountdisk, "md")) {
			$valid_disks[$disk]['device'] = $disk;
			$valid_disks[$disk]['name'] = storage_get_disk_name($disk);
			$valid_disks[$disk]['bytes'] = storage_get_disk_bytes($disk);
			$valid_disks[$disk]['usage'] = storage_get_disk_usage($disk);
			if ($disk == $system_disk) {
				$valid_disks[$disk]['systemdisk'] = true;
			}
		}
	}
	
	return $valid_disks;
}

function storage_get_recognized_disk($device) {
	
	$disks = storage_get_recognized_disks();

	return $disks[$device];
}

function storage_get_disk_name($disk) {
	
	$dmesg = system_get_dmesg_as_string();
	$matches = array();
	if (preg_match("/$disk:.+<.+>/", $dmesg, &$matches)) {
		$start = strpos($matches[0], "<") + 1;
		$name = substr($matches[0], $start, strpos($matches[0], ">") - $start);
	} else {
		$name = "unavailable";
	}
	
	return $name;
}

function storage_get_disk_bytes($disk) {
	
	if (storage_get_disk_state($disk) == "active") {
		exec("/bin/df -k", $output);
		foreach ($output as $o) {
			if (strstr($o, "/dev/$disk")) {
				$o = preg_split("/\s+/", $o);
				$bytes += 1024 * $o[1];
			}
		}

	} else {
		exec("/usr/sbin/diskinfo $disk", $output);
		$output = preg_split("/\s+/", $output[0]);
		$bytes = $output[2];
	}
	
	return $bytes;
}

function storage_get_system_disk() {
	global $g;

	return chop(file_get_contents("{$g['varetc_path']}/cfdevice"));
}

function storage_get_disk_usage($disk) {

	$matches = array();

	exec("/bin/df", $output);
	foreach ($output as $o) {
		if (strstr($o, "/dev/$disk")) {
			$o = preg_split("/\s+/", $o);
			$matches[] = rtrim($o[4], "%");
		}
	}
	
	if (!count($matches)) {
		return false;
	}
	
	return (array_sum($matches) / count($matches));
}

function storage_get_disk_services($disk) {
	
	$services = array();
	
	if ($disk == storage_get_system_disk()) {
		$services[] = "system";

	} else if ($c_disk = storage_get_configured_disk($disk)) {
		if (is_array($c_disk['storage-service'])) {
			foreach ($c_disk['storage-service'] as $service) {
				$services[] = $service;
			}
		}
	}
	
	return $services;
}

function storage_get_disk_state($disk) {
	
	if (storage_get_configured_disk($disk) || $disk == storage_get_system_disk()) {
		$state = "active";
	} else {
		$state = "unconfigured";
	}
	
	return $state;
}

?>
