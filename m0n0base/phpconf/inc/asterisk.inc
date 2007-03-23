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

/* include all configuration functions */
require_once("functions.inc");
require_once("asterisk_sip.inc");
require_once("asterisk_extensions.inc");

function asterisk_start() {
	global $config, $g;

	if ($g['booting'])
		echo "Starting Asterisk... ";
	else
		killbypid("{$g['varrun_path']}/asterisk.pid");

	$astcfg = $config['asterisk'];
	
	$res = 0;
	$res |= asterisk_sip_conf_generate();
	$res |= mwexec("/usr/local/sbin/asterisk");

	if ($g['booting']) {
		if ($res == 0)
			echo "done\n";
		else
			echo "failed\n";
	}

	return $res;
}


function asterisk_exec($cmd, $output=NULL) {

	$token = md5(uniqid(rand()));
	$errno = 0;
	$errstr = 0;
	$fp = fsockopen("localhost", 5038, &$errno, &$errstr, 20);
	if (!$fp) {
	  return 1;
	}
	
	fputs($fp, "Action: login\r\n");
	fputs($fp, "Username: admin\r\n");
	fputs($fp, "Secret: askozia\r\n");
	fputs($fp, "Events: off\r\n\r\n");
	usleep(500);
	fputs($fp, "Action: COMMAND\r\n");
	fputs($fp, "command: $cmd\r\n");
	fputs($fp, "ActionID: $token\r\n\r\n");
	usleep(500);
	$out = fread($fp, 38000); 
	while(strpos($out,"--END COMMAND--")==0) {
		$out .= fread($fp, 38000); 	  
	}
	fclose ($fp);

	// better command failure checking needed
	$output = $out;

	return 0;	
}


function asterisk_is_valid_extension($extension) {
	return true;
}


function asterisk_is_valid_callerid($id) {
	return true;
}


function asterisk_is_valid_secret($secret) {
	return true;
}

function asterisk_is_valid_username($username) {
	return true;
}

function asterisk_is_valid_host($host) {
	return true;
}

function asterisk_is_valid_prefix($prefix) {
	return true;
}

function asterisk_get_extensions() {
	global $config;
	
	$extensions = array();
	if (is_array($config['phones']['sipphone']))
		foreach($config['phones']['sipphone'] as $phone)
			$extensions[] = $phone['extension'];

	return $extensions;
}

function asterisk_get_providers() {
	global $config;
	
	if (!is_array($config['providers']['sipprovider']))
		$config['providers']['sipprovider'] = array();
		
	return array_merge($config['providers']['sipprovider']);
}

function asterisk_get_prefixes() {
	global $config;
	
	$prefixes = array();
	if (is_array($config['providers']['sipprovider']))
		foreach($config['providers']['sipprovider'] as $provider)
			$prefixes[] = $provider['prefix'];

	return $prefixes;	
}


	
?>
