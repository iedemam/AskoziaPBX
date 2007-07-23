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
	
	THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
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


function sip_get_providers() {
	global $config;
	
	if (!is_array($config['sip']['provider']))
		$config['sip']['provider'] = array();
		
	sip_sort_providers();
		
	return $config['sip']['provider'];
}

function sip_get_phones() {
	global $config;
	
	if (!is_array($config['sip']['phone']))
		$config['sip']['phone'] = array();
		
	sip_sort_phones();
		
	return $config['sip']['phone'];
}

function sip_get_phone($uniqid) {
	global $config, $uniqid_map;
	return $config['sip']['phone'][$uniqid_map[$uniqid]];
}

function sip_get_extensions() {
	global $config;
	
	$extensions = array();
	if (is_array($config['sip']['phones']))
		foreach($config['sip']['phones'] as $phone)
			$extensions[] = $phone['extension'];

	return $extensions;
}

function sip_get_dialpatterns() {
	global $config;
	
	$dialpatterns = array();
	if (is_array($config['sip']['provider']))
		foreach($config['sip']['provider'] as $provider)
			$dialpatterns[$provider['uniqid']] = $provider['dialpattern'];

	return $dialpatterns;	
}

function sip_conf_generate() {
	global $config, $g;
	
	$fd = fopen("/usr/local/etc/asterisk/sip.conf", "w");
	if (!$fd) {
		printf("Error: cannot open sip.conf in sip_conf_generate().\n");
		return 1;
	}
	
	$sipconfig = $config['services']['sip'];
	
	$port = "5060";
	if (isset($sipconfig['port']))
		$port = $sipconfig['port'];

	$conf = <<<EOD
	
[general]
context=default
allowoverlap=no
bindport=$port
bindaddr=0.0.0.0
srvlookup=yes
useragent=AskoziaPBX
relaxdtmf=yes
alwaysauthreject=yes
videosupport=yes

EOD;

	// registration options
	if (isset($sipconfig['defaultexpiry']))
		$conf .= "defaultexpiry={$sipconfig['defaultexpiry']}\n";
	if (isset($sipconfig['minexpiry']))
		$conf .= "minexpiry={$sipconfig['minexpiry']}\n";
	if (isset($sipconfig['maxexpiry']))
		$conf .= "maxexpiry={$sipconfig['maxexpiry']}\n";
	$conf .= "\n";
	
	// nat config
	$lancfg = $config['interfaces']['lan'];

	if ($lancfg['topology'] == "natstatic") {
		$conf .= "externip=" . $lancfg['extipaddr'] . "\n";
		$conf .= "localnet=" . $lancfg['ipaddr'] . "/" . $lancfg['subnet'] . "\n";
	} else if ($lancfg['topology'] == "natdynamic") {
		$conf .= "externhost=" . $lancfg['externhost'] . "\n";
		$conf .= "externrefresh=10\n";
		$conf .= "localnet=" . $lancfg['ipaddr'] . "/" . $lancfg['subnet'] . "\n";
	}
	
	$conf .= "\n\n";
	
	// providers
	if (!is_array($config['sip']['provider']))
		$config['sip']['provider'] = array();
		
	$sipproviders = $config['sip']['provider'];

	// provider registrations
	foreach($sipproviders as $p) {
		if ($p['noregister'])
			continue;
		$conf .= "; {$p['name']}\n";
		$conf .= "register => {$p['username']}";
		if($p['secret']) {
			$conf .= ":{$p['secret']}";
			if($p['authuser']) {
				$conf.= ":{$p['authuser']}";
			}
		}
		$conf .= "@{$p['host']}";
		if($p['port']) {
			$conf .= ":{$p['port']}";	
		}
		$conf .= "/{$p['username']}";	
		$conf .= "\n\n";
	}
	
	// provider definitions
	foreach($sipproviders as $p) {
		$conf .= "; {$p['name']}\n";
		$conf .= "[{$p['uniqid']}]\n";
		$conf .= "type=friend\n";
		$conf .= "username={$p['username']}\n";
		$conf .= "secret={$p['secret']}\n";
		
		if($p['fromuser']) {
			$conf .= "fromuser={$p['fromuser']}\n";
		} else {
			$conf .= "fromuser={$p['username']}\n";
		}
		
		$conf .= "context={$p['uniqid']}\n";
		$conf .= "host={$p['host']}\n";
		
		if ($p['port'])
			$conf .= "port={$p['port']}\n";
		
		if($p['fromdomain']) {
			$conf .= "fromdomain={$p['fromdomain']}\n";
		} else {
			$conf .= "fromdomain={$p['host']}\n";
		}
		
		$conf .= "nat=no\n";
		
		// language selection
		if ($p['language']) {
			$conf .= "language=" . $p['language'] . "\n";
		} else {
			$conf .= "language=en\n";
		}
		
		// qualify options
		if ($p['qualify'] == "0") {
			$conf .= "qualify=no\n";
		} else if ($p['qualify']) {
			$conf .= "qualify=". $p['qualify']*1000 ."\n";
		} else {
			$conf .= "qualify=yes\n";
		}
		
		$conf .= "canreinvite=no\n";
		$conf .= "insecure=port,invite\n";
		
		// dtmf options
		if ($p['dtmfmode']) {
			$conf .= "dtmfmode={$p['dtmfmode']}\n";
		} else {
			$conf .= "dtmfmode=auto\n";
		}
		
		// codecs	
		$conf .= "disallow=all\n";
		if(is_array($p['codec'])) {
			foreach ($p['codec'] as $codec)
				$conf .= "allow=$codec\n";
		} else {
			$conf .= "allow={$p['codec']}\n";
		}
		
		$conf .= "\n\n";
	}

	// phones
	if (!is_array($config['sip']['phone']))
		$config['sip']['phone'] = array();
		
	$sipphones = $config['sip']['phone'];
	
	foreach($sipphones as $p) {
		$conf .= "; {$p['callerid']}\n";
		$conf .= "[{$p['extension']}]\n";
		$conf .= "type=friend\n";
		$conf .= "secret={$p['secret']}\n";
		$conf .= "context={$p['uniqid']}\n";
		$conf .= "callerid=\"{$p['callerid']}\" <{$p['extension']}>\n";
		$conf .= "host=dynamic\n";
		
		if ($lancfg['topology'] != "public")
			$conf .= "nat=yes\n";
			
		// language selection
		if ($p['language']) {
			$conf .= "language=" . $p['language'] . "\n";
		} else {
			$conf .= "language=en\n";
		}
		
		if ($p['qualify'] == "0") {
			$conf .= "qualify=no\n";
		} else if ($p['qualify']) {
			$conf .= "qualify=". $p['qualify']*1000 ."\n";
		} else {
			$conf .= "qualify=yes\n";
		}
		
		$conf .= "canreinvite=no\n";
		
		if ($p['dtmfmode']) {
			$conf .= "dtmfmode={$p['dtmfmode']}\n";
		} else {
			$conf .= "dtmfmode=auto\n";
		}		
		
		$conf .= "disallow=all\n";
	
		if(is_array($p['codec'])) {
			foreach ($p['codec'] as $codec)
				$conf .= "allow=$codec\n";
		} else {
			$conf .= "allow={$p['codec']}\n";
		}
		
		$conf .= "\n\n";
	}

	fwrite($fd, $conf);
	fclose($fd);
	
	return 0;
}


function sip_reload() {
	
	return asterisk_exec("module reload chan_sip.so");
}


function sip_sort_phones() {
	global $g, $config;

	usort($config['sip']['phone'], "_sipphonecmp");	
}
function _sipphonecmp($a, $b) {
    if ($a['extension'] == $b['extension']) {
        return 0;
    }
    return ($a['extension'] < $b['extension']) ? -1 : 1;
}


function sip_sort_providers() {
	global $g, $config;

	usort($config['sip']['provider'], "_sipprovidercmp");	
}
function _sipprovidercmp($a, $b) {
	return strcmp($a['name'], $b['name']);
}

?>