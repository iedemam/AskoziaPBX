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

/**
 * Kills a process by pid file
 */
function killbypid($pidfile) {
	sigkillbypid($pidfile, "TERM");
	unlink_if_exists($pidfile);
}

/**
 * Sigkills a process by pid file 
 */
function sigkillbypid($pidfile, $sig) {
	if (file_exists($pidfile)) {
		mwexec("/bin/kill -s $sig `/bin/cat " . $pidfile . "`");
	}
}

/**
 * Kills a process by name
 */
function killbyname($procname) {
	return mwexec("/usr/bin/killall " . escapeshellarg($procname));
}

/**
 * Kills a process by name
 */
function sigkillbyname($procname, $sig) {
	return mwexec("/usr/bin/killall -{$sig} " . escapeshellarg($procname));
}

/**
 * Wrapper for exec()
 */
function mwexec($command) {

	global $g;
	
	if ($g['debug']) {
		if (!$_SERVER['REMOTE_ADDR'])
			echo "mwexec(): $command\n";
		passthru($command, $retval);
	} else {
		exec("$command > /dev/null 2>&1", $oarr, $retval);
	}
	
	return $retval; 
}

/**
 * Wrapper for exec() in background
 */
function mwexec_bg($command) {

	global $g;
	
	if ($g['debug']) {
		if (!$_SERVER['REMOTE_ADDR'])
			echo "mwexec(): $command\n";
	}
	
	exec("nohup $command > /dev/null 2>&1 &");
}

/**
 * Unlink a file, if it exists
 */
function unlink_if_exists($fn) {
	if (file_exists($fn))
		unlink($fn);
}

function format_bytes($bytes) {
	if ($bytes >= 1073741824) {
		return sprintf(gettext("%.2f GB"), $bytes/1073741824);
	} else if ($bytes >= 1048576) {
		return sprintf(gettext("%.2f MB"), $bytes/1048576);
	} else if ($bytes >= 1024) {
		return sprintf(gettext("%.0f KB"), $bytes/1024);
	} else {
		return sprintf(gettext("%d bytes"), $bytes);
	}
}

/**
 * Manual attributes are needed in a variety of places,
 * this generates an appropriate conf snippet.
 * XXX : util_gen_*() functions should be written for language, qualify, codecs...
 */
function util_gen_manual_attributes($ma_array) {

	$conf = "";
	if (is_array($ma_array)) {
		$conf .= "; manual attributes\n";
		foreach ($ma_array as $ma) {
			$conf .= base64_decode($ma) . "\n";
		}
	}

	return $conf;
}

function util_file_put_contents($contents, $filename) {

	$fd = fopen($filename, "w");
	if (!$fd) {
		printf("Error: cannot open $filename in util_file_put_contents().\n");
		return 1;
	}
	fwrite($fd, $contents);
	fclose($fd);

	return 0;
}

/* generate a self-signed SSL cert/key (1024-bit, valid for 2 years) and return it in PEM format */
function generate_self_signed_cert() {
	global $g, $config;

	$orgname = "AskoziaPBX";
	$cn = $config['system']['hostname'] . "." . $config['system']['domain'];

	/* check local clock first - must be set to something after the release date
	  (subtract a day to avoid TZ problems with early adopters) */
	$buildtime = trim(file_get_contents("{$g['etc_path']}/version.buildtime.unix")) - 86400;
	if (time() < $buildtime)
		return false;

	$dn = array(
	    "organizationName" => $orgname,
	    "commonName" => $cn
	);

	$conf = array(
		"digest_alg" => "sha1",
		"private_key_bits" => 1024
	);

	$privkey = openssl_pkey_new($conf);
	$csr = openssl_csr_new($dn, $privkey, $conf);
	$sscert = openssl_csr_sign($csr, null, $privkey, 730, $conf, time());

	openssl_x509_export($sscert, $certout);
	openssl_pkey_export($privkey, $pkeyout);

	/* sanity check */
	if (strpos($certout, "BEGIN CERTIFICATE") !== false && strpos($pkeyout, "BEGIN RSA PRIVATE KEY") !== false)
		return array('cert' => $certout, 'key' => $pkeyout);
	else
		return false;
}

?>
