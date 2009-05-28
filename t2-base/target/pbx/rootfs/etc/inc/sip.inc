<?php
/*
	$Id$
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

/**
 * Returns all providers sorted by name.
 * If no providers are defined, the global array is initialized.
 * This should be changed to return by reference to eliminate a lot of repetitive code.
 */
function sip_get_providers() {
	global $config;
	
	if (!is_array($config['sip']['provider']))
		$config['sip']['provider'] = array();
		
	sip_sort_providers();
		
	return $config['sip']['provider'];
}

/**
 * Returns a specific provider based on its uniqid
 */
function sip_get_provider($uniqid) {
	global $config, $uniqid_map;

	return $config['sip']['provider'][$uniqid_map[$uniqid]];
}

/**
 * Sorts providers by name
 */
function sip_sort_providers() {
	global $g, $config;

	usort($config['sip']['provider'], "pbx_sort_by_name");	
}

/**
 * Returns all phones sorted by extension.
 * If no phones are defined, the global array is initialized.
 */
function sip_get_phones() {
	global $config;
	
	if (!is_array($config['sip']['phone']))
		$config['sip']['phone'] = array();
		
	sip_sort_phones();
		
	return $config['sip']['phone'];
}

/**
 * Sorts phones by extension
 */
function sip_sort_phones() {
	global $g, $config;

	usort($config['sip']['phone'], "pbx_sort_by_extension");	
}

/**
 * Returns a specific phone based on its uniqid
 */
function sip_get_phone($uniqid) {
	global $config, $uniqid_map;

	return $config['sip']['phone'][$uniqid_map[$uniqid]];
}

/**
 * Returns an array of all assigned phone extensions
 */
function sip_get_extensions() {
	global $config;
	
	$extensions = array();

	if (is_array($config['sip']['phone'])) {
		foreach($config['sip']['phone'] as $phone) {
			$extensions[] = $phone['extension'];
		}
	}

	return $extensions;
}

/**
 * Returns an array of all assigned provider dialpatterns
 */
function sip_get_dialpatterns() {
	global $config;
	
	$dialpatterns = array();

	if (is_array($config['sip']['provider'])) {
		foreach($config['sip']['provider'] as $provider) {
			$dialpatterns[$provider['uniqid']] = $provider['dialpattern'];
		}
	}

	return $dialpatterns;	
}

/**
 * Returns a host indexed array of sip providers which have multiple accounts
 * defined for the same host
 */
function sip_get_hosts_with_multiple_accounts() {

	$multi = array();

	$providers = sip_get_providers();
	$tmp = array();
	foreach ($providers as $p) {
		if (isset($p['disabled'])) {
			continue;
		}
		$tmp[$p['host']][] = $p['uniqid'];
	}
	foreach ($tmp as $host => $ids) {
		if (count($ids) > 1) {
			$multi[$host] = $ids;
		}
	}

	return $multi;
}

/**
 * Generates sip.conf
 * XXX : the way in which defaults are used must be normalized
 */
function sip_conf_generate() {
	global $config, $g, $defaults;
	
	$fd = fopen("/etc/asterisk/sip.conf", "w");
	if (!$fd) {
		printf("Error: cannot open sip.conf in sip_conf_generate().\n");
		return 1;
	}
	
	$sipconfig = $config['services']['sip'];
	
	$port = "5060";
	if (isset($sipconfig['port']))
		$port = $sipconfig['port'];

	$srvlookup = "yes";
	if (isset($sipconfig['disablesrv'])) {
		$srvlookup = "no";
	}

	$conf = <<<EOD
	
[general]
context=public-direct-dial
allowoverlap=no
bindport=$port
bindaddr=0.0.0.0
srvlookup=$srvlookup
useragent=AskoziaPBX
relaxdtmf=yes
alwaysauthreject=yes
videosupport=yes
notifybusy=yes
limitonpeer=yes
notifyhold=no
pedantic=yes

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
		$baseaddr = long2ip(ip2long($lancfg['ipaddr']) & gen_subnet_mask_long($lancfg['subnet']));
		$conf .= "localnet=" . $baseaddr . "/" . $lancfg['subnet'] . "\n";
	} else if ($lancfg['topology'] == "natdynamichost") {
		$conf .= "externhost=" . $lancfg['exthostname'] . "\n";
		$conf .= "externrefresh=10\n";
		$baseaddr = long2ip(ip2long($lancfg['ipaddr']) & gen_subnet_mask_long($lancfg['subnet']));
		$conf .= "localnet=" . $baseaddr . "/" . $lancfg['subnet'] . "\n";
	}

	$conf .= util_gen_manual_attributes($sipconfig['manual-attribute']);

	$conf .= "\n\n";
	
	// providers
	$sipproviders = sip_get_providers();
	$sipproviders_multi = sip_get_hosts_with_multiple_accounts();

	// provider registrations
	foreach($sipproviders as $p) {
		if (isset($p['disabled'])) {
			continue;
		}
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
		if (isset($p['disabled'])) {
			continue;
		}
		// outgoing (peer)
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

		$conf .= "host={$p['host']}\n";
		if ($p['port']) {
			$conf .= "port={$p['port']}\n";
		}

		// incoming context must be specially handled if this provider is
		// one of many accounts associated with a specific host
		$tmp_incomingcontext = $p['uniqid'] . "-incoming";
		foreach ($sipproviders_multi as $spm_host => $spm_ids) {
			if (in_array($p['uniqid'], $spm_ids)) {
				$tmp_incomingcontext = "SIP-PROVIDER-$spm_host-incoming";
			}
		}
		$conf .= "context=$tmp_incomingcontext\n";

		if($p['fromdomain']) {
			$conf .= "fromdomain={$p['fromdomain']}\n";
		} else {
			$conf .= "fromdomain={$p['host']}\n";
		}

		// language selection
		if ($p['language']) {
			$conf .= "language=" . $p['language'] . "\n";
		} else {
			$conf .= "language=en\n";
		}

		// nat settings
		if ($p['natmode']) {
			$conf .= "nat=" . $p['natmode'] . "\n";
		} else {
			$conf .= "nat=" . $defaults['sip']['natmode'] . "\n";
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
			foreach ($p['codec'] as $codec) {
				$conf .= "allow=$codec\n";
			}
		}

		$conf .= util_gen_manual_attributes($p['manual-attribute']);

		$conf .= "\n\n";
	}

	// phones
	$sipphones = sip_get_phones();
	
	foreach($sipphones as $p) {
		$conf .= "; {$p['callerid']}\n";
		$conf .= "[{$p['extension']}]\n";
		$conf .= "type=friend\n";
		$conf .= "secret={$p['secret']}\n";
		$conf .= "context={$p['uniqid']}\n";
		$conf .= "callerid={$p['callerid']} <{$p['extension']}>\n";
		$conf .= "host=dynamic\n";

		// language selection
		if ($p['language']) {
			$conf .= "language=" . $p['language'] . "\n";
		} else {
			$conf .= "language=en\n";
		}

		// nat settings
		if ($p['natmode']) {
			$conf .= "nat=" . $p['natmode'] . "\n";
		} else {
			$conf .= "nat=" . $defaults['sip']['natmode'] . "\n";
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
		
		if ($p['calllimit']) {
			$conf .= "call-limit={$p['calllimit']}\n";
		} else {
			$conf .= "call-limit=2\n";
		}
		if ($p['busylimit']) {
			$conf .= "busy-limit={$p['busylimit']}\n";
		} else {
			$conf .= "busy-limit=1\n";
		}
		
		$conf .= "disallow=all\n";
	
		if(is_array($p['codec'])) {
			foreach ($p['codec'] as $codec)
				$conf .= "allow=$codec\n";
		} else {
			$conf .= "allow={$p['codec']}\n";
		}

		$conf .= util_gen_manual_attributes($p['manual-attribute']);
		
		$conf .= "\n\n";
	}

	fwrite($fd, $conf);
	fclose($fd);
	
	return 0;
}

/**
 * Reloads chan_sip to apply newly generated configuration changes
 */
function sip_reload() {
	return pbx_exec("module reload chan_sip.so");
}

?>
