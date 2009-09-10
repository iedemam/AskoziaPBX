<?php
/*
	$Id$
	originally part of m0n0wall (http://m0n0.ch/wall)
	continued modifications as part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
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
 * Configures systems static routes
 * XXX : static routes would still be nice to have
 */
function system_routing_configure() {
	global $config, $g;
	
	/*printf("LINUX TODO: empty system_routing_configure()\n");*/ return 0;
	
	/* clear out old routes, if necessary */
	if (file_exists("{$g['vardb_path']}/routes.db")) {
		$fd = fopen("{$g['vardb_path']}/routes.db", "r");
		if (!$fd) {
			printf("Error: cannot open routes DB file in system_routing_configure().\n");
			return 1;		
		}
		while (!feof($fd)) {
			$oldrt = fgets($fd);
			if ($oldrt)
				mwexec("/sbin/route delete " . escapeshellarg($oldrt));
		}
		fclose($fd);
		unlink("{$g['vardb_path']}/routes.db");
	}
	
	if (is_array($config['staticroutes']['route'])) {
		
		$fd = fopen("{$g['vardb_path']}/routes.db", "w");
		if (!$fd) {
			printf("Error: cannot open routes DB file in system_routing_configure().\n");
			return 1;		
		}
		
		foreach ($config['staticroutes']['route'] as $rtent) {
			mwexec("/sbin/route add " . escapeshellarg($rtent['network']) . 
				" " . escapeshellarg($rtent['gateway']));
			
			/* record route so it can be easily removed later (if necessary) */
			fwrite($fd, $rtent['network'] . "\n");
		}
		
		fclose($fd); 
	}
	
	return 0;
}

/**
 * Turns ip forwarding on
 */
function system_routing_enable() {
	global $config, $g;

	/*printf("LINUX TODO: empty system_routing_enable()\n");*/ return 0;

	return mwexec("/sbin/sysctl net.inet.ip.forwarding=1");
}

/**
 * Generates syslog.conf and (re)starts the syslog daemon
 */
function system_syslogd_start() {
	global $config, $g;

	if ($g['platform'] == "COMpact 3000") {
		echo "skipping... ";
		return 0;
	}

	/*
	LINUX TODO : Logging currently runs everything into
	a single destination. This is OK but the GUI should
	be extended to only show a single summary page (maybe
	with filter buttons "All, PBX, CDR, etc...")
	if no 'logging service' is enabled. If it is enabled,
	the page should display all tabs again. CDR to Syslog
	needs to be replaced with something more robust if
	that service is enabled.
	For now, let's not kill and restart the daemon for
	each configuration change since that deletes our
	in-memory buffer!
	Long story short...logs need a rewrite!
	*/

	$retval = 0;

	if ($g['booting']) {
		$retval = mwexec("/sbin/syslogd -C1024");
	}

	return $retval;


//	$syslogcfg = $config['syslog'];
//	$logging_pkg = packages_get_package("logging");
//
//	if ($g['booting']) {
//		echo "Starting syslog service... ";
//	} else {
//		killbypid("{$g['varrun_path']}/syslog.pid");
//	}
//
//	/* write syslog.conf */
//	$fd = fopen("{$g['etc_path']}/syslog.conf", "w");
//	if (!$fd) {
//		printf("Error: cannot open syslog.conf in system_syslogd_start().\n");
//		return 1;
//	}
//
//	$syslogconf = <<<EOD
//local0.*;mail.*							%/var/log/pbx.log
//local1.*								%/var/log/cdr.log
//*.notice;kern.debug;lpr.info;mail.none;news.err;local0.none;local1.none; %/var/log/system.log
//security.*								%/var/log/system.log
//auth.info;authpriv.info;daemon.info		%/var/log/system.log
//*.emerg									*
//
//EOD;
//
//	if (isset($logging_pkg['active'])) {
//		$logpath = $logging_pkg['datapath'] . "/system";
//		$syslogconf .= <<<EOD
//local0.*;mail.*							$logpath/pbx.log
//*.notice;kern.debug;lpr.info;mail.none;news.err;local0.none;local1.none; $logpath/system.log
//security.*								$logpath/system.log
//auth.info;authpriv.info;daemon.info		$logpath/system.log
//
//EOD;
//	}
//
//	if (isset($syslogcfg['enable'])) {
//
//		if (isset($syslogcfg['system'])) {
//			$syslogconf .= <<<EOD
//*.notice;kern.debug;lpr.info;mail.none;news.err;local0.none;local1.none; @{$syslogcfg['remoteserver']}
//security.*								@{$syslogcfg['remoteserver']}
//auth.info;authpriv.info;daemon.info		@{$syslogcfg['remoteserver']}
//
//EOD;
//		}
//		if (isset($syslogcfg['asterisk'])) {
//			$syslogconf .= <<<EOD
//local0.*								@{$syslogcfg['remoteserver']}
//
//EOD;
//		}
//		if (isset($syslogcfg['cdr'])) {
//			$syslogconf .= <<<EOD
//local1.*								@{$syslogcfg['remoteserver']}
//
//EOD;
//		}
//	}
//
//	fwrite($fd, $syslogconf);
//	fclose($fd);
//	
//	$retval = mwexec("/sbin/syslogd -C1024");
//
//	if ($g['booting']) {
//		echo "done\n";
//	}
//	
//	return $retval;
}

/**
 * Configures and (re)starts the mini_httpd process
 */
function system_webgui_start() {
	global $config, $g;

	/* kill any running mini_httpd */
	killbypid("{$g['varrun_path']}/mini_httpd.pid");

	/* generate password file */
	system_update_shell_password();

	chdir($g['www_path']);

	/* non-standard port? */
	if ($config['system']['webgui']['port']) {
		$portarg = "-p {$config['system']['webgui']['port']}";
	} else {
		$portarg = "";
	}

	if ($config['system']['webgui']['protocol'] == "https") {

		if ($config['system']['webgui']['certificate'] && $config['system']['webgui']['private-key']) {
			$cert = base64_decode($config['system']['webgui']['certificate']);
			$key = base64_decode($config['system']['webgui']['private-key']);
		} else {
			/* default certificate/key */
			$cert = <<<EOD
-----BEGIN CERTIFICATE-----
MIIBtzCCASACCQCfbBuIBnO/HDANBgkqhkiG9w0BAQQFADAgMRAwDgYDVQQKEwdB
c2tvemlhMQwwCgYDVQQLEwNQQlgwHhcNMDcwNTI0MDQxMTAzWhcNMDgwNTIzMDQx
MTAzWjAgMRAwDgYDVQQKEwdBc2tvemlhMQwwCgYDVQQLEwNQQlgwgZ8wDQYJKoZI
hvcNAQEBBQADgY0AMIGJAoGBALnUNDOIXFAVTdanBSPTEidJRAHGUTESaC+bgQl5
XskMsHZ4b7OViAqVg/AcCJ2CoMXz13xmSJqvbbdr1kEVxeFabpfIkKZ139yJsFe8
HIzjuqoMiYTYDhH2MHIK3sTZmX6NnpsfSbRWhMNUepq/tFu6QoPewRqW2/F9B8ne
fbEFAgMBAAEwDQYJKoZIhvcNAQEEBQADgYEAT0wAzz3m5rpItKFBigZPRS8aqVAi
ffwFDMUcAo72KX7i8gpVe22rNUUfDkNiPubegfNKj0gWG+8d/h3l/zALvGqfC4aa
VdoqS2VEspLQBiDm3BZ6YyS+ZNohXt/s3qJdBWClj0KZT4UouIZhMuEOX7/fEDmC
Ni0EbQa+fsuwYgA=
-----END CERTIFICATE-----

EOD;

			$key = <<<EOD
-----BEGIN RSA PRIVATE KEY-----
MIICWwIBAAKBgQC51DQziFxQFU3WpwUj0xInSUQBxlExEmgvm4EJeV7JDLB2eG+z
lYgKlYPwHAidgqDF89d8Zkiar223a9ZBFcXhWm6XyJCmdd/cibBXvByM47qqDImE
2A4R9jByCt7E2Zl+jZ6bH0m0VoTDVHqav7RbukKD3sEaltvxfQfJ3n2xBQIDAQAB
AoGADWRYmroiBPd0E5sXyLwmW8Rw7TvQkBvGfF/Vjcw8Y3rRoMgPLL8C/dhGmOA8
UrWeRoXLKsDoTJWF76kK2Mfr6bE6j7Trkf3gXImXl0P1ge5h+uEBsfJj3TCJwnxF
w6ZRsVEKQiLQNZAX+YVDYIydYOg8zw6ou36Y5D1l6E13EmECQQDgW1gH0in8fW0x
mwfEJy0d2pbsYvRzEks2SActKQhfzoXO81kSWJM9GZbkiL2DCZdOS8iqYuKAl0X+
zNduVUGNAkEA1AnEXHQtzTeXnoo8UT4sqmY2aZB6ma3NH6/h8qxBRKfQUu/Qp4J7
HSEnRm5S90qVSTw0DTDXzgyUK5Adq/pDWQJAJTD/NsywCRsBrud0MiAMapaBzk3Z
F/pAXY8zIVlb+gXSSKwuZ8wnHEnRqbDBOaetXV1StmwOACMTh1mNnMylQQJAPFPD
EGLlnRuSAcNmwd6kqp6GIKbAmvS+BaPu7zJfJ39vYAlnHiGbkYAbQoilBkKsErgU
PySEYdVmWraYGpQLkQJARzMpatUv43+SXGPCI6syywO+efB/pz/HAdBc4syMLUUi
1sy5CWZE63NhtedF332qYOt+8dmT8tO/DzN4rqUEqg==
-----END RSA PRIVATE KEY-----

EOD;
		}

		$fd = fopen("{$g['varetc_path']}/cert.pem", "w");
		if (!$fd) {
			printf("Error: cannot open cert.pem in system_webgui_start().\n");
			return 1;
		}
		chmod("{$g['varetc_path']}/cert.pem", 0600);
		fwrite($fd, $cert);
		fwrite($fd, "\n");
		fwrite($fd, $key);
		fclose($fd);

		$res = mwexec("/usr/sbin/mini_httpd -S -E {$g['varetc_path']}/cert.pem" .
			" -c \"**.php|**.cgi\" -u root -maxproc 16 $portarg" .
			" -i {$g['varrun_path']}/mini_httpd.pid");
	} else {
		$res = mwexec("/usr/sbin/mini_httpd -c \"**.php|**.cgi\" -u root" .
			" -maxproc 16 $portarg -i {$g['varrun_path']}/mini_httpd.pid");
	}

	return $res;
}

/**
 * Configures and (re)starts the busybox httpd process
 */
function system_update_httpd_settings() {
	global $config, $g;

	if ($config['system']['username']) {
		$username = $config['system']['username'];
	} else {
		$username = "admin";
	}

	killbyname("httpd");

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

	if ($config['system']['webgui']['port']) {
		$portarg = "-p {$config['system']['webgui']['port']}";
	}

	return mwexec("httpd -u root $portarg -h {$g['www_path']}");
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

	mwexec("echo \"root:" . crypt($config['system']['password']) . "\" | /usr/sbin/chpasswd -e");

	return 0;
}

/**
 * Populates /etc/TZ with an appropriate time zone
 */
function system_timezone_configure() {
	global $config, $g;

	$timezone = $config['system']['timezone'];

	unlink_if_exists("/etc/TZ");
	if ($timezone) {
		$timezone = explode("|", $timezone);
		$timezone = $timezone[1];
		exec("echo \"$timezone\" > /etc/TZ");
	}
}

/**
 * Configures and (re)starts msntp
 */
function system_ntp_configure() {
	global $config, $g;

	$syscfg = $config['system'];

	if (!$g['booting']) {
		killbypid("{$g['varrun_path']}/runmsntp.pid");
		killbypid("{$g['varrun_path']}/msntp.pid");
	}

	/* start ntp client if needed - needs to be forced into background */
	$updateinterval = $syscfg['time-update-interval'];

	if ($updateinterval > 0) {
		if ($updateinterval < 6) {
			$updateinterval = 6;
		}
		$timeservers = "";
		foreach (explode(' ', $syscfg['timeservers']) as $ts) {
			$timeservers .= " " . $ts;
		}

		mwexec_bg("/usr/bin/runmsntp.sh " .
			escapeshellarg("{$g['varrun_path']}/runmsntp.pid") . " " .
			escapeshellarg("{$g['varrun_path']}/msntp.pid") . " " .
			escapeshellarg($updateinterval) . " " .
			escapeshellarg($timeservers));
	}
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

	mwexec("/sbin/modprobe dahdi");
	mwexec("/sbin/modprobe wctdm");
	mwexec("/sbin/modprobe wcb4xxp");
	//mwexec("/sbin/modprobe wcfxo");
	//mwexec("/sbin/modprobe wcfxs");
}

function system_setup_ultmp() {
	global $g;

	//$d_ultmpmounted_path = $g['varrun_path'] . "/ultmp.mounted";

	//if (storage_syspart_get_state() == "active") {
	//	if (!file_exists("/storage/ultmp")) {
	//		mkdir("/storage/ultmp");
	//	} else {
	//		mwexec("rm -rf /ultmp");
	//	}
	//	symlink("/storage/ultmp", "/ultmp");
	//	touch($d_ultmpmounted_path);
	//} else {
		mkdir("/ultmp");
	//}

	// clean up uploaded / downloaded files
	mwexec("rm -rf /ultmp/*");
}

function system_get_product_name() {
	global $g;

	if (file_exists("{$g['etc_path']}/brand.product")) {
		return chop(file_get_contents("{$g['etc_path']}/brand.product"));
	} else {
		return "AskoziaPBX";
	}
}

?>
