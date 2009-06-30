<?php
/*
	$Id: services.inc 356 2008-01-02 15:43:21Z michael.iedema $
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

/* include all configuration functions */
require_once("functions.inc");

function packages_get_packages() {

	$packages = array();
	$paths = packages_get_paths();

	/* for each path where packagese can reside */
	foreach ($paths as $media => $path) {
		$dh = opendir($path);
		while ($dirname = readdir($dh)) {
			/* if the directory is a pkg */
			if (strpos($dirname, ".pkg") !== false) { 
				/* read in the xml configuration */
				$pkg = packages_read_config($path . "/" . $dirname);
				/* store that information in the packages array */
				$packages[$pkg['name']] = $pkg;
				$packages[$pkg['name']]['media'] = $media;
				$packages[$pkg['name']]['parentpath'] = $path;
				$packages[$pkg['name']]['path'] = $path . "/" . $dirname;
				$packages[$pkg['name']]['datapath'] = $path . "/" . $dirname . "/data";
				/* is the package active? */
				if (file_exists($path . "/" . $dirname . "/pkg.active")) {
					$packages[$pkg['name']]['active'] = true;
				}
				/* does the package have its own conf pages? */
				if (file_exists($path . "/" . $dirname . "/www/index.php")) {
					$packages[$pkg['name']]['haspages'] = true;
				}
			} 
		}
		closedir($dh);
	}

	return $packages;
}

function packages_get_package($name) {

	$paths = packages_get_paths();

	/* for each path where packagese can reside */
	foreach ($paths as $media => $path) {
		$dh = opendir($path);
		while ($dirname = readdir($dh)) {
			/* if the directory is a pkg */
			if ($dirname == $name . ".pkg") { 
				/* read in the xml configuration */
				$pkg = packages_read_config($path . "/" . $dirname);
				/* store that information in the packages array */
				$pkg['media'] = $media;
				$pkg['parentpath'] = $path;
				$pkg['path'] = $path . "/" . $dirname;
				$pkg['datapath'] = $path . "/" . $dirname . "/data";
				/* is the package active? */
				if (file_exists($path . "/" . $dirname . "/pkg.active")) {
					$pkg['active'] = true;
				}
				/* does the package have its own conf pages? */
				if (file_exists($path . "/" . $dirname . "/www/index.php")) {
					$pkg['haspages'] = true;
				}
				break;
			} 
		}
		closedir($dh);
	}

	return $pkg;
}

function packages_get_paths() {

	$paths = array();

	if (storage_syspart_get_state() == "active") {
		$paths['syspart'] = storage_get_media_path("syspart");
	}

	return $paths;
}

function packages_get_packages_assigned_to_media($media) {

	$packages = packages_get_packages();

	$assigned_packages = array();
	foreach($packages as $package) {
		if ($package['media'] == $media) {
			$assigned_packages[$package['name']] = $package;
		}
	}

	return $assigned_packages;
}

function packages_read_config($path) {
	$tmp = xml_to_array($path . "/conf.xml", "package");
	return $tmp[0];
}

function packages_is_active($name) {

	$pkg = packages_get_package($name);

	return isset($package['active']);
}

function packages_exec_rc($name, $command) {

	$pkg = packages_get_package($name);
	return mwexec("/etc/rc.pkgexec " . $pkg['path'] . "/rc $command");
}

function packages_create_command_file($file, $package_name, $package_command) {

	util_file_put_contents($package_name . ":" . $package_command, $file);

	return 0;
}

function packages_read_command_file($file) {

	$tmp = explode(":", file_get_contents($file));
	$command['name'] = $tmp[0];
	$command['command'] = $tmp[1];

	return $command;
}

function packages_get_size($name) {

	$pkg = packages_get_package($name);

	exec("/usr/bin/du -k -d 0 " . $pkg['path'], $out);
	$out = preg_split("/\s+/", $out[0]);

	return $out[0] * 1024;
}

function packages_generate_save_message($name, $command) {

	switch($command) {
		case "activate":
			return gettext("Package activated successfully.");
		case "deactivate":
			return gettext("Package deactivated successfully.");
		case "delete":
			return gettext("Package deleted successfully.");
		case "restore":
			return gettext("Package restored successfully.");
		case "install":
			return gettext("Package installed successfully.");
		default:
			return "packages_generate_save_message...no message found";
	}
}

function packages_get_www_nav() {

	$packages = packages_get_packages();

	foreach ($packages as $package) {
		if ($package['haspages'] && $package['active']) {
			$symfile = "/usr/www/packages_" . $package['name'] . ".php";
			if (!file_exists($symfile)) {
				symlink($package['path'] . "/www/index.php", $symfile);
			}
			$nav[ucfirst($package['name'])] = array("packages_" . $package['name'] . ".php");
		}
	}

	return $nav;
}

function packages_boot_all() {

	printf("LINUX TODO: empty packages_boot_all()\n"); return 0;

	$packages = packages_get_packages();

	echo "Executing package boot routines...";

	foreach ($packages as $package) {
		if ($package['active']) {
			packages_exec_rc($package['name'], "boot");
		}
	}

	echo "done.\n";
}

function packages_shutdown_all() {

	printf("LINUX TODO: empty packages_shutdown_all()\n"); return 0;

	$packages = packages_get_packages();

	foreach ($packages as $package) {
		if ($package['active']) {
			packages_exec_rc($package['name'], "shutdown");
		}
	}

}

function packages_check_server_versions() {
	global $g;
	$post = "&check=pbxpackage&platform=" . rawurlencode($g['platform']) . 
		"&version=" . rawurlencode(trim(file_get_contents("/etc/version"))) .
		"&packagesystemversion=" . rawurlencode($g['package_system']);
		
	$rfd = @fsockopen("downloads.askozia.com", 80, $errno, $errstr, 3);
	if ($rfd) {
		$hdr = "POST /index.php HTTP/1.0\r\n";
		$hdr .= "Content-Type: application/x-www-form-urlencoded\r\n";
		$hdr .= "User-Agent: AskoziaPBX-webGUI/1.0\r\n";
		$hdr .= "Host: downloads.askozia.com\r\n";
		$hdr .= "Content-Length: " . strlen($post) . "\r\n\r\n";
		
		fwrite($rfd, $hdr);
		fwrite($rfd, $post);
		
		$inhdr = true;
		$resp = "";
		while (!feof($rfd)) {
			$line = fgets($rfd);
			if ($inhdr) {
				if (trim($line) == "")
					$inhdr = false;
			} else {
				$resp .= $line;
			}
		}
		
		fclose($rfd);

		$lines = explode("\n", $resp);
		foreach ($lines as $line) {
			$line = preg_split("/\s+/", $line, -1, PREG_SPLIT_NO_EMPTY);
			if ($line[0] && $line[1] && $line[2]) {
				$packages[$line[0]] = array(
					"version" => $line[1],
					"url" => $line[2],
					"filename" => basename($line[2])
				);
			}
		}
		return $packages;
	}
	
	return null;
}

?>
