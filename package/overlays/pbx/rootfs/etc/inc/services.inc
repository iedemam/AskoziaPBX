<?php
/*
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

/* include all configuration functions */
require_once("functions.inc");

function services_dyndns_reset() {
	global $config, $g;

	if (file_exists("{$g['vardb_path']}/ez-ipupdate.cache")) {
		unlink("{$g['vardb_path']}/ez-ipupdate.cache");
	}	
	
	if (file_exists("{$g['conf_path']}/ez-ipupdate.cache")) {
		conf_mount_rw();
		unlink("{$g['conf_path']}/ez-ipupdate.cache");
		conf_mount_ro();
	}
	
	return 0;
}

function services_dyndns_configure() {
	global $config, $g;

	/* kill any running ez-ipupdate */
	/* ez-ipupdate needs SIGQUIT instead of SIGTERM */
	sigkillbypid("{$g['varrun_path']}/ez-ipupdate.pid", "QUIT");

	$dyndnscfg = $config['dyndns'];
	$lancfg = $config['interfaces']['lan'];

	if ($config['interfaces']['lan']['hostnameupdatesrc'] == "pbx") {

		if ($g['booting'])
			echo "Starting DynDNS client... ";
		else
			sleep(1);

		/* write ez-ipupdate.conf */
		$fd = fopen("{$g['etc_path']}/ez-ipupdate.conf", "w");
		if (!$fd) {
			printf("Error: cannot open ez-ipupdate.conf in services_dyndns_configure().\n");
			return 1;
		}

		$ezipupdateconf = <<<EOD
service-type={$dyndnscfg['type']}
user={$dyndnscfg['username']}:{$dyndnscfg['password']}
host={$lancfg['exthostname']}
interface=web-detect
period=600
max-interval=2073600
pid-file={$g['varrun_path']}/ez-ipupdate.pid
cache-file={$g['vardb_path']}/ez-ipupdate.cache
execute=/etc/rc.dyndns.storecache
daemon

EOD;

		/* enable server[:port]? */
		if ($dyndnscfg['server']) {
			if ($dyndnscfg['port'])
				$ezipupdateconf .= "server={$dyndnscfg['server']}:{$dyndnscfg['port']}\n";
			else
				$ezipupdateconf .= "server={$dyndnscfg['server']}\n";
		}

		/* enable MX? */
		if ($dyndnscfg['mx']) {
			$ezipupdateconf .= "mx={$dyndnscfg['mx']}\n";
		}

		/* enable wildcards? */
		if (isset($dyndnscfg['wildcard'])) {
			$ezipupdateconf .= "wildcard\n";
		}

		fwrite($fd, $ezipupdateconf);
		fclose($fd);


		/* if we're booting, copy the cache file from /conf */
		if ($g['booting']) {
			if (file_exists("{$g['conf_path']}/ez-ipupdate.cache")) {
				copy("{$g['conf_path']}/ez-ipupdate.cache", "{$g['vardb_path']}/ez-ipupdate.cache");
			}
		}

		/* run ez-ipupdate */
		mwexec("/usr/bin/ez-ipupdate -c {$g['etc_path']}/ez-ipupdate.conf");

		if ($g['booting'])
			echo "done\n";
	}

	return 0;
}

function services_dnsupdate_process() {
	global $config, $g;
	
	/* Dynamic DNS updating active? */
	if (isset($config['dnsupdate']['enable'])) {
		
		$wanip = network_get_current_public_ip_address();
		
		if ($wanip) {
				
			$keyname = $config['dnsupdate']['keyname'];
			/* trailing dot */
			if (substr($keyname, -1) != ".")
				$keyname .= ".";
			
			$hostname = $config['dnsupdate']['host'];
			/* trailing dot */
			if (substr($hostname, -1) != ".")
				$hostname .= ".";
			
			/* write private key file
			   this is dumb - public and private keys are the same for HMAC-MD5,
			   but nsupdate insists on having both */
			$fd = fopen("{$g['varetc_path']}/K{$keyname}+157+00000.private", "w");
			$privkey .= <<<EOD
Private-key-format: v1.2
Algorithm: 157 (HMAC)
Key: {$config['dnsupdate']['keydata']}

EOD;
			fwrite($fd, $privkey);
			fclose($fd);
			
			/* write public key file */
			if ($config['dnsupdate']['keytype'] == "host") {
				$flags = 513;
				$proto = 3;
			} else if ($config['dnsupdate']['keytype'] == "user") {
				$flags = 0;
				$proto = 2;
			} else {
				$flags = 257;
				$proto = 3;
			}
			
			$fd = fopen("{$g['varetc_path']}/K{$keyname}+157+00000.key", "w");
			fwrite($fd, "{$keyname} IN KEY {$flags} {$proto} 157 {$config['dnsupdate']['keydata']}\n");
			fclose($fd);
			
			/* generate update instructions */
			$upinst =  "update delete {$hostname} A\n";
			$upinst .= "update add {$hostname} {$config['dnsupdate']['ttl']} A {$wanip}\n";
			$upinst .= "\n";	/* mind that trailing newline! */
			
			$fd = fopen("{$g['varetc_path']}/nsupdatecmds", "w");
			fwrite($fd, $upinst);
			fclose($fd);
			
			/* invoke nsupdate */
			$cmd = "/usr/bin/nsupdate -k {$g['varetc_path']}/K{$keyname}+157+00000.private";
			if (isset($config['dnsupdate']['usetcp']))
				$cmd .= " -v";
			$cmd .= " {$g['varetc_path']}/nsupdatecmds";
			
			mwexec_bg($cmd);
		}
	}
	
	return 0;
}

?>
