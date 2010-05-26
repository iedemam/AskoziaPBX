<?php
/*
	$Id$
	originally part of m0n0wall (http://m0n0.ch/wall)
	continued modifications as part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
	Copyright (C) 2007-2010 tecema (a.k.a IKT) <http://www.tecema.de>.
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
 * Generates resolv.conf
 */
function system_resolvconf_generate($dynupdate = false) {
	global $config, $g;

	$syscfg = $config['system'];

	$fd = fopen("{$g['etc_path']}/resolv.conf", "w");
	if (!$fd) {
		printf("Error: cannot open resolv.conf in system_resolvconf_generate().\n");
		return 1;
	}

	$resolvconf = "domain {$syscfg['domain']}\n";
	if (is_array($syscfg['dnsserver'])) {
		foreach ($syscfg['dnsserver'] as $ns) {
			if ($ns) {
				$resolvconf .= "nameserver $ns\n";
			}
		}
	}

	fwrite($fd, $resolvconf);
	fclose($fd);

	return 0;
}

/**
 * Generates a hosts file containing localhost and hostname entries
 */
function system_hosts_generate() {
	global $config, $g;
	
	$syscfg = $config['system'];
	$lancfg = $config['interfaces']['lan'];

	$fd = fopen("{$g['etc_path']}/hosts", "w");
	if (!$fd) {
		printf("Error: cannot open hosts file in system_hosts_generate().\n");
		return 1;
	}
		
	$hosts = "127.0.0.1	localhost localhost.{$syscfg['domain']}\n";
	if ($lancfg['ipaddr']) {
		$hosts .= "{$lancfg['ipaddr']}	{$syscfg['hostname']}.{$syscfg['domain']} {$syscfg['hostname']}";
	}
	$hosts .= "\n";
	
	fwrite($fd, $hosts);
	fclose($fd);
	
	return 0;
}

/**
 * Applies system hostname
 */
function system_hostname_configure() {
	global $config, $g;

	$syscfg = $config['system'];

	/* set hostname */
	return mwexec("/bin/hostname " .
		escapeshellarg("{$syscfg['hostname']}.{$syscfg['domain']}"));
}

/**
 * Configures and (re)starts the busybox httpd process
 */
function system_update_httpd_settings() {
	global $config, $g;

	// non-default username?
	if ($config['system']['username']) {
		$username = $config['system']['username'];
	} else {
		$username = "admin";
	}

	$pid = exec('ps -w | grep "/etc/httpd.conf" | grep -v "grep" | cut -d " " -f 2');
	if ($pid) {
		killbypidnumber($pid);
	}
	killbyname("sslwrap");

	// setup httpd with password protection and a php interpreter
	$fd = fopen("{$g['etc_path']}/httpd.conf", "w");
	if (!$fd) {
		printf("Error: cannot open httpd.conf in system_update_httpd_settings().\n");
		return 1;
	}
	fwrite($fd,
		"/:" . $username . ":" . $config['system']['password'] . "\n".
		"*.php:/usr/bin/php\n".
		"I:index.php\n".
		"\n"
	);
	fclose($fd);
	chmod("{$g['etc_path']}/httpd.conf", 0600);

	if ($config['system']['webgui']['protocol'] == "https") {
		// non-default https certificate?
		if ($config['system']['webgui']['certificate'] 
			&& $config['system']['webgui']['private-key']) {
    	
			$cert = base64_decode($config['system']['webgui']['certificate']);
			$key = base64_decode($config['system']['webgui']['private-key']);
    	
			$fd = fopen("{$g['etc_path']}/httpscert.pem", "w");
			if (!$fd) {
				printf("Error: cannot open httpscert.pem in system_update_httpd_settings().\n");
				return 1;
			}
			chmod("{$g['etc_path']}/httpscert.pem", 0600);
			fwrite($fd, $cert . "\n" . $key);
			fclose($fd);
		}

		if ($config['system']['webgui']['port']) {
			$portarg = "-accept " . $config['system']['webgui']['port'];
		} else {
			$portarg = "-accept 443";
		}

		$res = mwexec("httpd -c /etc/httpd.conf -u root -p 127.0.0.1:80 -h {$g['www_path']}");
		$res |= mwexec_bg("sslwrap -cert {$g['etc_path']}/httpscert.pem -port 80 $portarg");

	} else {

		if ($config['system']['webgui']['port']) {
			$portarg = "-p " . $config['system']['webgui']['port'];
		}

		$res = mwexec("httpd -c /etc/httpd.conf -u root $portarg -h {$g['www_path']}");
	}

	return $res;
}

function system_upgrade_https_certificate() {
	global $config;

	if (!$config['system']['webgui']['certificate']) {
		echo "Upgrading HTTPS to a more secure, machine-specific certificate...";

		$ck = generate_self_signed_cert();
		
		if ($ck === false) {
			echo "failed!\n";
			echo " (A certificate could not be automatically generated because your system's clock is not set. " .
				 "Please configure a custom certificate on the Advanced -> GUI Options page for security; " .
				 "until then, the system will use an insecure default certificate shared by all installations)\n\n";
		} else {
			$config['system']['webgui']['certificate'] = base64_encode($ck['cert']);
			$config['system']['webgui']['private-key'] = base64_encode($ck['key']);
			echo "done\n";

			write_config();
		}
	}
}

/**
 * Generates htpasswd and sets the root password
 */
function system_update_shell_password() {
	global $config;

	mwexec("echo \"root:" . $config['system']['password'] . "\" | /usr/sbin/chpasswd");

	return 0;
}

/**
 * Populates /etc/TZ with an appropriate time zone
 */
function system_timezone_configure() {
	global $config, $g;

	$timezone = $config['system']['timezone'];

	include("timezones.inc");

	unlink_if_exists("/etc/TZ");
	unlink_if_exists("/etc/localtime");

	if ($timezone) {
		$zonestring = $tz[$timezone][1];
		exec("echo \"$zonestring\" > /etc/TZ");
		$zonefile = $tz[$timezone][2];
		exec("cp /usr/share/zoneinfo/" . $zonefile . " /etc/localtime");
	}
}

/**
 * Initial NTP time sync
 */
function system_ntp_initial_sync() {
	global $config;

	if ($config['system']['time-update-interval'] != "disable") {
		mwexec("ntpclient -i 10 -L -s -h " . escapeshellarg($config['system']['timeservers']));
	}
}

/**
 * Configures and (re)starts dropbear ssh daemon
 */
function system_sshd_configure() {
	global $config, $g;

	$dirtyconfig = false;

	$keytypes = array("rsa", "dss");
	foreach($keytypes as $keytype) {

		$keyfieldname = "ssh-" . $keytype . "-key";
		$keyfilepath = "/etc/dropbear/dropbear_" . $keytype . "_host_key";

		// do not disturb if the file is already present
		if (file_exists($keyfilepath)) {
			continue;
		}

		// prepare the dropbear configuration directory if necessary
		if (!file_exists("/etc/dropbear")) {
			mkdir("/etc/dropbear");
		}

		// if a key is already stored in config.xml, write it out to the system...
		if (isset($config['system'][$keyfieldname])) {
			echo("Restoring dropbear " . $keytype . " key ... ");
			util_file_write($keyfilepath, base64_decode($config['system'][$keyfieldname]));
			chmod($keyfilepath, 0600);
			echo ("done.\n");
		// ...otherwise generate a new key and store it in config.xml
		} else {
			echo("Generating dropbear " . $keytype . " key ... ");
			exec("/usr/bin/dropbearkey -t " . $keytype . " -f " . $keyfilepath);
			$config['system'][$keyfieldname] = base64_encode(file_get_contents($keyfilepath));
			$dirtyconfig = true;
			echo ("done.\n");
		}
	}

	if ($dirtyconfig) {
		write_config();
	}

	killbyname("dropbear");
	mwexec("dropbear");

	return 0;
}

/**
 * Reboots the system after calling system_reboot_cleanup()
 */
function system_reboot() {
	global $g;
	
	system_reboot_cleanup();
	
	mwexec("nohup /etc/rc.reboot > /dev/null 2>&1 &");
}

/**
 * Reboots the system after calling system_reboot_cleanup()
 */
function system_reboot_sync() {
	global $g;
	
	system_reboot_cleanup();
	
	mwexec("/etc/rc.reboot > /dev/null 2>&1");
}

/**
 * Placeholder function for future cleanup() functions to be executed
 * before restarting
 */
function system_reboot_cleanup() {
	packages_shutdown_all();
}

/**
 * Executes commands from config.xml defined as shellcmd or earlyshellcmd 
 * entries upon boot
 */
function system_do_shell_commands($early = 0) {
	global $config, $g;
	
	if ($early)
		$cmdn = "earlyshellcmd";
	else
		$cmdn = "shellcmd";
	
	if (is_array($config['system'][$cmdn])) {
		
		foreach ($config['system'][$cmdn] as $cmd) {
			exec($cmd);
		}
	}
}

/**
 * (En|Dis)able the system console
 */
function system_console_configure() {
	global $config, $g;

	if (isset($config['system']['disableconsolemenu'])) {
		touch("{$g['varetc_path']}/disableconsole");
	} else {
		unlink_if_exists("{$g['varetc_path']}/disableconsole");
	}
}

/**
 * Set hard disk standby time
 * XXX : should perhaps be replaced with newer bits in m0n0wall from r213
 *       http://svn.m0n0.ch/wsvn/wall/branches/?rev=213
 */
function system_set_harddisk_standby() {
	global $g, $config;

	if ($g['platform'] != "Generic") {
		return;
	}

	if (isset($config['system']['harddiskstandby']) && 
			($config['system']['harddiskstandby'] > 0)) {

		$standby = $config['system']['harddiskstandby'];
		// Check for a numeric value
		if (is_numeric($standby)) {
			// Sync the disk(s)
			mwexec('/bin/sync');
			// Reinitialize ATA-drives
			mwexec('/sbin/atacontrol reinit 0');
			if ($g['booting']) {
				echo "done\n";
			}
		}
	}
}

/**
 * Checks to see if the wrap reset button was pressed and held for 2 seconds
 * on boot. If so, factory defaults will be restored and the system restarted.
 */
function system_check_reset_button() {
	global $config, $g;

	$platform = $g['platform'];
	if ($platform != "wrap" && $platform != "alix23x") {
		return 0;
	}

	$retval = mwexec("/usr/sbin/" . $platform . "resetbtn");

	if ($retval == 99) {
		/* user has pressed reset button for 2 seconds - 
		   reset to factory defaults */
		echo <<<EOD

***********************************************************************
* Reset button pressed - resetting configuration to factory defaults. *
* The system will reboot after this completes.                        *
***********************************************************************


EOD;
		
		reset_factory_defaults();
		system_reboot_sync();
		exit(0);
	}

	return 0;
}

/**
 * Returns most recent dmesg contents as a single string
 */
function system_get_dmesg_as_string() {

	return implode("\n", system_get_dmesg_as_array());
}

/**
 * Returns most recent dmesg contents as an array of its lines
 */
function system_get_dmesg_as_array() {

	exec("/bin/dmesg", $dmesg_lines);
	return $dmesg_lines;

	//$n = count($dmesg_lines);

	///* find last copyright line (output from previous boots may be present) */
	//$lastcpline = 0;
	//for ($i = 0; $i < $n; $i++) {
	//	if (strstr($dmesg_lines[$i], "Copyright (c) 1992-")) {
	//		$lastcpline = $i;
	//	}
	//}

	//$dmesg = array();
	//for ($i = $lastcpline; $i < $n; $i++) {
	//	$dmesg[] = $dmesg_lines[$i];
	//}

	//return $dmesg;
}

/**
 * Loads required kernel modules
 */
function system_load_kernel_modules() {
	global $g, $config;

	mwexec("/sbin/modprobe -q dahdi");
	mwexec("/sbin/modprobe -q dahdi_transcode");
	mwexec("/sbin/modprobe -q echo");
	mwexec("/sbin/modprobe -q dahdi_echocan_oslec");


	if ($g['platform'] == "COMpact 3000") {
		$basemodules = array("auerask_cp3k_mb", "auerask_cp3k_fxs", "auerask_cp3k_bri", "bfin_sport_tdm");
	} else if ($g['platform'] == "Warp") {
		$basemodules = array("warp_shared", "warp_analog", "pika-brie");
	} else {
		$basemodules = array("wcfxo", "wctdm", "wcb4xxp", "zaphfc");
	}
	foreach ($basemodules as $m) {
		$module_options = '';
		if (isset($config['system']['regional']['analog']['fxo']['daa'])) {
			switch ($m) {
				case 'wcfxo':
					/* opermode support in wcfxo still incomplete and some cards
						doesn't support some modes so here we use a very small subset (CTR21=1/FCC=0).
						Also fail safe changing the daa mode only if TBR21/CTR21 required
						because all the cards defaults to FCC.
					*/
					$int_daa_opermode = dahdi_get_daa_fallback($config['system']['regional']['analog']['fxo']['daa']);
					if ($int_daa_opermode > 0) {
						$module_options = " opermode={$int_daa_opermode}";
					}
					break;
				case 'wctdm':
					$module_options = " opermode={$config['system']['regional']['analog']['fxo']['daa']} fxshonormode=1";
					break;
			}
		}
		
		mwexec("/sbin/modprobe -q " . $m . $module_options);
	}

	// is xorcom stuff compiled for this platform?
	if (file_exists("/etc/supports.xorcom")) {
		mwexec("/sbin/modprobe -q xpp");
		mwexec("/sbin/modprobe -q xpp_usb");
		mwexec("/sbin/modprobe -q xpd_fxo");
		mwexec("/sbin/modprobe -q xpd_fxs");
		mwexec("/sbin/modprobe -q xpd_pri");
		mwexec("/sbin/modprobe -q xpd_bri");
	}

	// is redfone stuff compiled for this platform?
	if (file_exists("/etc/supports.redfone")) {
		mwexec("/sbin/modprobe -q dahdi_dynamic");
		mwexec("/sbin/modprobe -q dahdi_dynamic_ethmf");
	}
}

function system_get_product_name() {
	global $g;

	if (file_exists("{$g['etc_path']}/brand.product")) {
		return chop(file_get_contents("{$g['etc_path']}/brand.product"));
	} else {
		return "AskoziaPBX";
	}
}

function system_cron_generate($return=false) {

	$interval = $GLOBALS['config']['system']['time-update-interval'];
	if ($interval != "disable") {
		switch ($interval) {
			case "10-minutes":
				$conf .= "*/10 * * * *";
				break;
			case "30-minutes":
				$conf .= "*/30 * * * *";
				break;
			case "1-hour":
				$conf .= "0 * * * *";
				break;
			case "4-hours":
				$conf .= "0 */4 * * *";
				break;
			case "12-hours":
				$conf .= "0 */12 * * *";
				break;
			case "1-day":
				$conf .= "0 0 * * *";
				break;
		}

		$conf .= " ntpclient -i 10 -L -s -h " . $GLOBALS['config']['system']['timeservers'] . "\n";
	}

	return ($return) ? $conf : util_file_write("/var/spool/cron/crontabs/root", $conf);
}

function system_cron_configure() {

	killbyname("crond");
	system_cron_generate();
	mwexec("crond -l 0");

	return 0;
}

?>
