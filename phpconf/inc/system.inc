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
 * XXX : this needs to be cleaned up, leftovers still here from m0n0wall trim down
 * XXX : is $havedns being set correctly?
 */
function system_resolvconf_generate($dynupdate = false) {
	global $config, $g;
	
	$syscfg = $config['system'];
	
	$fd = fopen("{$g['varetc_path']}/resolv.conf", "w");
	if (!$fd) {
		printf("Error: cannot open resolv.conf in system_resolvconf_generate().\n");
		return 1;
	}
		
	$resolvconf = "domain {$syscfg['domain']}\n";
	
	$havedns = false;
	
	if (!$havedns && is_array($syscfg['dnsserver'])) {
		foreach ($syscfg['dnsserver'] as $ns) {
			if ($ns)
				$resolvconf .= "nameserver $ns\n";
			$havedns = true;
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

	$fd = fopen("{$g['varetc_path']}/hosts", "w");
	if (!$fd) {
		printf("Error: cannot open hosts file in system_hosts_generate().\n");
		return 1;
	}
		
	$hosts = <<<EOD
127.0.0.1	localhost localhost.{$syscfg['domain']}
{$lancfg['ipaddr']}	{$syscfg['hostname']}.{$syscfg['domain']} {$syscfg['hostname']}

EOD;
	
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
	
	return mwexec("/sbin/sysctl net.inet.ip.forwarding=1");
}

/**
 * Generates syslog.conf and (re)starts the syslog daemon
 * XXX : this generator should replace the static syslog.conf file still
 *       present in the svn tree
 */
function system_syslogd_start() {
	global $config, $g;
	
	$syslogcfg = $config['syslog'];

	if ($g['booting']) 
		echo "Starting syslog service... ";
	else
		killbypid("{$g['varrun_path']}/syslog.pid");
			
	if (isset($syslogcfg['enable'])) {

		/* write syslog.conf */
		$fd = fopen("{$g['varetc_path']}/syslog.conf", "w");
		if (!$fd) {
			printf("Error: cannot open syslog.conf in system_syslogd_start().\n");
			return 1;
		}
		
		$syslogconf = <<<EOD
local0.*;mail.*							%/var/log/pbx.log
*.notice;kern.debug;lpr.info;mail.none;news.err;local0.none;local1.none; %/var/log/system.log
security.*								%/var/log/system.log
auth.info;authpriv.info;daemon.info		%/var/log/system.log
*.emerg									*

EOD;

		if (isset($syslogcfg['asterisk'])) {
			$syslogconf .= <<<EOD
local0.*								@{$syslogcfg['remoteserver']}

EOD;
		}
		if (isset($syslogcfg['cdr'])) {
			$syslogconf .= <<<EOD
local1.*								@{$syslogcfg['remoteserver']}

EOD;
		}

		fwrite($fd, $syslogconf);
		fclose($fd);
		
		$retval = mwexec("/usr/sbin/syslogd -s -f {$g['varetc_path']}/syslog.conf");

	} else {
		$retval = mwexec("/usr/sbin/syslogd -ss");
	}
	
	if ($g['booting'])
		echo "done\n";
		
	return $retval;
}

/**
 * Configures and (re)starts the mini_httpd process
 */
function system_webgui_start() {
	global $config, $g;
	
	if ($g['booting'])
		echo "Starting webGUI... ";
	
	/* kill any running mini_httpd */
	killbypid("{$g['varrun_path']}/mini_httpd.pid");
	
	/* generate password file */
	system_password_configure();
	
	chdir($g['www_path']);
	
	/* non-standard port? */
	if ($config['system']['webgui']['port'])
		$portarg = "-p {$config['system']['webgui']['port']}";
	else
		$portarg = "";
	
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
	
		$res = mwexec("/usr/local/sbin/mini_httpd -S -E {$g['varetc_path']}/cert.pem" .
			" -c \"**.php|**.cgi\" -u root -maxproc 16 $portarg" .
			" -i {$g['varrun_path']}/mini_httpd.pid");
	} else {
		$res = mwexec("/usr/local/sbin/mini_httpd -c \"**.php|**.cgi\" -u root" .
			" -maxproc 16 $portarg -i {$g['varrun_path']}/mini_httpd.pid");
	}
	
	if ($g['booting']) {
		if ($res == 0)
			echo "done\n";
		else
			echo "failed\n";
	}
	
	return $res;
}

/**
 * Generates htpasswd
 * XXX : still contains "local user manager" which should still be reimplemented
 */
function system_password_configure() {
	global $config, $g;
	
	$fd = fopen("{$g['varrun_path']}/htpasswd", "w");
	if (!$fd) {
		printf("Error: cannot open htpasswd in system_password_configure().\n");
		return 1;
	}
	
	if ($config['system']['username'])
		$username = $config['system']['username'];
	else
		$username = "admin";
	
	fwrite($fd, $username . ":" . $config['system']['password'] . "\n");

	if (is_array($config['system']['user'])) {
		foreach ($config['system']['user'] as $userent) {
			    fwrite($fd, $userent['name'] . ":" 
		              . $userent['password'] . "\n");
		}
	}
	
	fclose($fd);
	chmod("{$g['varrun_path']}/htpasswd", 0600);
	
	return 0;
}

/**
 * Decompresses timezone entries and stores them in /etc/localtime
 */
function system_timezone_configure() {
	global $config, $g;

	$syscfg = $config['system'];

	if ($g['booting'])
		echo "Initializing timezone... ";

	/* extract appropriate timezone file */
	$timezone = $syscfg['timezone'];
	if (!$timezone)
		$timezone = "Etc/UTC";
		
	exec("/usr/bin/tar xzfO /usr/share/zoneinfo.tgz " . 
		escapeshellarg($timezone) . " > /etc/localtime");

	if ($g['booting'])
		echo "done\n";
}

/**
 * Configures and (re)starts msntp
 */
function system_ntp_configure() {
	global $config, $g;

	$syscfg = $config['system'];

	if ($g['booting'])
		echo "Starting NTP client... ";
	else {
		killbypid("{$g['varrun_path']}/runmsntp.pid");
		killbypid("{$g['varrun_path']}/msntp.pid");
	}

	/* start ntp client if needed - needs to be forced into background */
	$updateinterval = $syscfg['time-update-interval'];
	
	if ($updateinterval > 0) {
		if ($updateinterval < 6)
			$updateinterval = 6;
		
		$timeservers = "";
		foreach (explode(' ', $syscfg['timeservers']) as $ts)
			$timeservers .= " " . $ts;
		
		mwexec_bg("/usr/local/bin/runmsntp.sh " .
			escapeshellarg("{$g['varrun_path']}/runmsntp.pid") . " " .
			escapeshellarg("{$g['varrun_path']}/msntp.pid") . " " .
			escapeshellarg($updateinterval) . " " .
			escapeshellarg($timeservers));
	}
		
	if ($g['booting'])
		echo "done\n";
}

/**
 * Reboots the system after calling system_reboot_cleanup()
 * XXX : difference between system_reboot_sync() and system_reboot() ?
 */
function system_reboot() {
	global $g;
	
	system_reboot_cleanup();
	
	mwexec("nohup /etc/rc.reboot > /dev/null 2>&1 &");
}

/**
 * Reboots the system after calling system_reboot_cleanup()
 * XXX : difference between system_reboot_sync() and system_reboot() ?
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

	if ($g['platform'] != "generic-pc")
		return;

	if (isset($config['system']['harddiskstandby']) && 
			($config['system']['harddiskstandby'] > 0)) {
		if ($g['booting']) {
			echo 'Setting harddisk standby time... ';
		}

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
		} else if ($g['booting']) {
			echo "failed\n";
		}
	}
}

/**
 * Sets termcap
 * XXX : doesn't seem to be working / understood when dealing with zttool
 */
function system_set_termcap() {
	global $config;
	
	$lines = 80;
	
	$termcap = <<<EOD
cons25w|ansiw|ansi80x25-raw:\
	:am:bs:NP:ms:pt:AX:eo:bw:ut:km:\
	:co#80:li#25:pa#64:Co#8:it#8:\
	:al=\E[L:cd=\E[J:ce=\E[K:cl=\E[H\E[J:cm=\E[%i%d;%dH:\
	:dc=\E[P:dl=\E[M:do=\E[B:bt=\E[Z:ho=\E[H:ic=\E[@:cb=\E[1K:\
	:nd=\E[C:rs=\Ec:so=\E[7m:se=\E[27m:up=\E[A:cr=^M:ta=^I:\
	:AF=\E[3%dm:AB=\E[4%dm:op=\E[39;49m:sc=\E7:rc=\E8:\
	:k1=\E[M:k2=\E[N:k3=\E[O:k4=\E[P:k5=\E[Q:k6=\E[R:k7=\E[S:k8=\E[T:\
	:k9=\E[U:k;=\E[V:F1=\E[W:F2=\E[X:K2=\E[E:nw=\E[E:ec=\E[%dX:\
	:kb=^H:kh=\E[H:ku=\E[A:kd=\E[B:kl=\E[D:kr=\E[C:le=^H:sf=\E[S:sr=\E[T:\
	:kN=\E[G:kP=\E[I:@7=\E[F:kI=\E[L:kD=\\177:kB=\E[Z:\
	:IC=\E[%d@:DC=\E[%dP:SF=\E[%dS:SR=\E[%dT:AL=\E[%dL:DL=\E[%dM:\
	:DO=\E[%dB:LE=\E[%dD:RI=\E[%dC:UP=\E[%dA:cv=\E[%i%dd:ch=\E[%i%d`:\
	:mb=\E[5m:md=\E[1m:mr=\E[7m:me=\E[m:bl=^G:\
	:ve=\E[=S:vi=\E[=1S:vs=\E[=2S:
cons25|ansis|ansi80x25:\
	:ac=l\\332m\\300k\\277j\\331u\\264t\\303v\\301w\\302q\\304x\\263n\\305`^Da\\260f\\370g\\361~\\371.^Y-^Xh\\261i^U0\\333y\\363z\\362:\
	:tc=cons25w:
dumb|su|unknown:\
	:am:co#132:li#$lines:do=^J:
xterm-noapp|xterm with cursor keys in normal mode:\
	:kl=\E[D:kd=\E[B:kr=\E[C:ku=\E[A:ks=\E=:ke=\E>:ti@:te@:tc=xterm:
xterm|xterm-color|X11 terminal emulator:\
	:ti@:te@:tc=xterm-xfree86:
xterm-xfree86|XFree86 xterm:\
	:k1=\EOP:k2=\EOQ:k3=\EOR:k4=\EOS:\
	:k5=\E[15~:k6=\E[17~:k7=\E[18~:k8=\E[19~:\
	:k9=\E[20~:k;=\E[21~:F1=\E[23~:F2=\E[24~:\
	:kH=\EOF:@7=\EOF:kI=\E[2~:\
	:kh=\EOH:*6=\EOF:kP=\E[5~:kN=\E[6~:\
	:ku=\EOA:kd=\EOB:kr=\EOC:kl=\EOD:Km=\E[M:tc=xterm-basic:
xterm-basic|xterm common (XFree86):\
	:li#24:co#80:am:kn#12:km:mi:ms:xn:bl=^G:\
	:is=\E[!p\E[?3;4l\E[4l\E>:rs=\E[!p\E[?3;4l\E[4l\E>:le=^H:\
	:AL=\E[%dL:DL=\E[%dM:DC=\E[%dP:al=\E[L:dc=\E[P:dl=\E[M:\
	:UP=\E[%dA:DO=\E[%dB:LE=\E[%dD:RI=\E[%dC:\
	:ho=\E[H:cd=\E[J:ce=\E[K:cl=\E[H\E[2J:cm=\E[%i%d;%dH:cs=\E[%i%d;%dr:\
	:im=\E[4h:ei=\E[4l:ks=\E[?1h\E=:ke=\E[?1l\E>:kD=\E[3~:kb=^H:\
	:sf=\n:sr=\EM:st=\EH:ct=\E[3g:sc=\E7:rc=\E8:\
	:eA=\E(B\E)0:as=^N:ae=^O:ml=\El:mu=\Em:up=\E[A:nd=\E[C:\
	:md=\E[1m:me=\E[m^O:mr=\E[7m:so=\E[7m:se=\E[27m:us=\E[4m:ue=\E[24m:\
	:ti=\E[?1049h:te=\E[?1049l:vi=\E[?25l:ve=\E[?25h:\
	:ut:Co#8:pa#64:op=\E[39;49m:AB=\E[4%dm:AF=\E[3%dm:\

EOD;

	if (!file_exists("/usr/share/misc"))
		mkdir("/usr/share/misc");

	$fd = @fopen("/usr/share/misc/termcap", "w");
	if (!$fd) {
		printf("Error: cannot open termcap in system_set_termcap().\n");
		return 1;
	}
	chmod("/usr/share/misc/termcap", 0644);
	fwrite($fd, $termcap);
	fclose($fd);
	
	return 0;
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

	$retval = mwexec("/usr/local/sbin/" . $platform . "resetbtn");

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
	
	exec("/sbin/dmesg", $dmesg_lines);
	$n = count($dmesg_lines);
	
	/* find last copyright line (output from previous boots may be present) */
	$lastcpline = 0;
	for ($i = 0; $i < $n; $i++) {
		if (strstr($dmesg_lines[$i], "Copyright (c) 1992-")) {
			$lastcpline = $i;
		}
	}
	
	$dmesg = array();
	for ($i = $lastcpline; $i < $n; $i++) {
		$dmesg[] = $dmesg_lines[$i];
	}
	
	return $dmesg;
}

/**
 * Parses dmesg output to determine if this system supports MMX extensions
 */
function system_supports_mmx() {
	global $g;

	// Features=0x88a93d<FPU,DE,PSE,TSC,MSR,CX8,SEP,PGE,CMOV,CLFLUSH,MMX>
	$dmesg = system_get_dmesg_as_string();
	if (preg_match("/Features.*?<.*MMX.*>/", $dmesg)) {
		return true;
	} else {
		return false;
	}
}

/**
 * Loads required kernel modules
 */
function system_load_kernel_modules() {
	
	/*if (system_supports_mmx()) {
		mwexec("/sbin/kldload mmx_oslec");
	} else {
		mwexec("/sbin/kldload oslec");
	}*/
	mwexec("/sbin/kldload oslec");
	mwexec("/sbin/kldload zaptel");
	mwexec("/sbin/kldload wcfxo");
	mwexec("/sbin/kldload wcfxs");
	//mwexec("/sbin/kldload wct1xxp");
	//mwexec("/sbin/kldload wct4xxp");
}

?>
