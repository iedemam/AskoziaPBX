<?php
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2010 IKT <http://itison-ikt.de>.
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

	if (!is_array($GLOBALS['config']['sip']['provider'])) {
		$GLOBALS['config']['sip']['provider'] = array();
	}

	$tmp = $GLOBALS['config']['sip']['provider'];
	usort($tmp, "pbx_sort_by_name");

	return $tmp;
}

/**
 * Returns a specific provider based on its uniqid
 */
function sip_get_provider($uniqid) {
	global $uniqid_map;

	return $GLOBALS['config']['sip']['provider'][$uniqid_map[$uniqid]];
}

/**
 * Returns all phones sorted by extension.
 * If no phones are defined, the global array is initialized.
 */
function sip_get_phones() {

	if (!is_array($GLOBALS['config']['sip']['phone'])) {
		$GLOBALS['config']['sip']['phone'] = array();
	}

	$tmp = $GLOBALS['config']['sip']['phone'];
	usort($tmp, "pbx_sort_by_extension");

	return $tmp;
}

/**
 * Returns a specific phone based on its uniqid
 */
function sip_get_phone($uniqid) {
	global $uniqid_map;

	return $GLOBALS['config']['sip']['phone'][$uniqid_map[$uniqid]];
}

/**
 * Returns an array of all assigned phone extensions
 */
function sip_get_extensions() {

	$extensions = array();

	if (is_array($GLOBALS['config']['sip']['phone'])) {
		foreach($GLOBALS['config']['sip']['phone'] as $phone) {
			$extensions[] = $phone['extension'];
		}
	}

	return $extensions;
}

/**
 * Returns an array of all assigned provider dialpatterns
 */
function sip_get_dialpatterns() {

	$dialpatterns = array();

	if (is_array($GLOBALS['config']['sip']['provider'])) {
		foreach($GLOBALS['config']['sip']['provider'] as $provider) {
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
 */
function sip_conf_generate($return=false) {
	global $g, $config, $defaults;

	$sipconfig = $config['services']['sip'];

	$port = "5060";
	if (isset($sipconfig['port'])) {
		$port = $sipconfig['port'];
	}

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
counteronpeer=yes
notifyhold=no
pedantic=yes
callcounter=yes

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
	} else if ($lancfg['topology'] == "natdynamichost") {
		$conf .= "externhost=" . $lancfg['exthostname'] . "\n";
		$conf .= "externrefresh=10\n";
	}
	if ($lancfg['topology'] && $lancfg['topology'] != "public") {
		if (isset($lancfg['dhcp'])) {
			$laninfo = network_get_interface($lancfg['if']);
			$baseaddr = long2ip(ip2long($laninfo['ipaddr']) & gen_subnet_mask_long($laninfo['subnet']));
			$conf .= "localnet=" . $baseaddr . "/" . $laninfo['subnet'] . "\n";
		} else {
			$baseaddr = long2ip(ip2long($lancfg['ipaddr']) & gen_subnet_mask_long($lancfg['subnet']));
			$conf .= "localnet=" . $baseaddr . "/" . $lancfg['subnet'] . "\n";
		}
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
		if ($p['noregister']) {
			continue;
		}

		$conf .= "; {$p['name']}\n";

		if ($p['manualregister']) {
			$conf .= $p['manualregister'];
		} else {
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
		}

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
		$conf .= "defaultuser={$p['username']}\n";
		$conf .= "secret={$p['secret']}\n";
		
		if($p['fromuser']) {
			$conf .= "fromuser={$p['fromuser']}\n";
		} else {
			// omit the fromuser config key if an explicit caller id name/number defined
			if ($p['calleridsource'] != 'string') {
				$conf .= "fromuser={$p['username']}\n";
			}
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

		if ($p['manualattributes']) {
			$conf .= "; manual attributes\n";
			$conf .= base64_decode($p['manualattributes']) . "\n";
			$conf .= "; end manual attributes\n";
		}

		$conf .= "\n\n";
	}

	// phones
	$phones = sip_get_phones();
	foreach($phones as $p) {
		if (isset($p['disabled'])) {
			continue;
		}
		$conf .= "; Caller ID: " . $p['callerid'] . "\n";
		if ($p['descr']) {
			$conf .= "; Description: " . $p['descr'] . "\n";
		}
		$conf .= "[" . $p['extension'] . "]\n";
		$conf .= "type=friend\n";
		$conf .= "secret=" . $p['secret'] . "\n";
		$conf .= "context=" . $p['uniqid'] . "\n";
		$conf .= "callerid=" . $p['callerid'] . " <" . $p['extension'] . ">\n";
		$conf .= "host=dynamic\n";
		$conf .= "language=" . $p['language'] . "\n";
		$conf .= "nat=" . $p['natmode'] . "\n";
		$conf .= "dtmfmode=" . $p['dtmfmode'] . "\n";
		$conf .= "qualify=yes\n";
		$conf .= "canreinvite=no\n";
		$conf .= "busy-limit=1\n";		

		$conf .= "disallow=all\n";
		if(is_array($p['codec'])) {
			foreach ($p['codec'] as $codec) {
				$conf .= "allow=" . $codec . "\n";
			}
		}

		if ($p['manualattributes']) {
			$conf .= "; manual attributes\n";
			$conf .= base64_decode($p['manualattributes']) . "\n";
			$conf .= "; end manual attributes\n";
		}

		$conf .= "\n\n";
	}

	return ($return) ? $conf : util_file_write("/etc/asterisk/sip.conf", $conf);
}

function sip_generate_default_phone() {
	global $defaults;

	$phone = array();

	$phone['extension'] = pbx_get_next_default_phone_extension();
	$phone['callerid'] = $defaults['accounts']['phones']['callerid'];
	$phone['language'] = $defaults['accounts']['language'];
	$phone['ringlength'] = $defaults['accounts']['phones']['ringlength'];
	$phone['publicaccess'] = $defaults['accounts']['phones']['publicaccess'];
	$phone['uniqid'] = "SIP-PHONE-" . uniqid(rand());

	$phone['secret'] = util_pwgen();
	$phone['codec'] = $defaults['accounts']['codec'];
	$phone['dtmfmode'] = $defaults['sip']['dtmfmode'];
	$phone['natmode'] = $defaults['sip']['natmode'];

	return $phone;
}

function sip_verify_phone($post, $errors) {

	$phone = array();

	v_cleanliness(&$post, &$errors);
	v_extension(&$phone, &$post, &$errors);
	v_callerid(&$phone, &$post, &$errors);
	v_optional('descr', &$phone, &$post);
	v_optional('snom-mac', &$phone, &$post);
	v_public_access_editor(&$phone, &$post, &$errors);
	v_notifications_editor(&$phone, &$post, &$errors);
	v_voicemail_editor(&$phone, &$post, &$errors);
	v_provider_access_selector(&$phone, &$post);
	v_codecs(&$phone, &$post);
	v_password(&$phone, &$post);
	v_manualattributes(&$phone, &$post, &$errors);

	$trusted = array(
		'uniqid',
		'language',
		'ringlength',
		'natmode',
		'dtmfmode'
	);
	foreach ($trusted as $t) {
		$phone[$t] = $post[$t];
	}

	return $phone;
}

function sip_save_phone($phone) {
	global $g, $uniqid_map;

	if (sip_get_phone($phone['uniqid'])) {
		$GLOBALS['config']['sip']['phone'][$uniqid_map[$phone['uniqid']]] = $phone;
	} else {
		$GLOBALS['config']['sip']['phone'][] = $phone;
	}

	write_config();
	touch($g['sip_dirty_path']);
}

function sip_generate_default_provider() {
	global $defaults;

	$provider = array();

	$provider['language'] = $defaults['accounts']['language'];
	$provider['codec'] = $defaults['accounts']['codec'];
	$provider['qualify'] = $defaults['accounts']['qualify'];
	$provider['calleridsource'] = $defaults['accounts']['providers']['calleridsource'];
	$provider['dtmfmode'] = $defaults['sip']['dtmfmode'];
	$provider['natmode'] = $defaults['sip']['natmode'];
	$provider['uniqid'] = 'SIP-PROVIDER-' . uniqid(rand());

	return $provider;
}

function sip_verify_provider($post, $errors) {

	$provider = array();

	v_cleanliness(&$post, &$errors);
	v_required('name', gettext('Name'), &$provider, &$post, &$errors);
	v_required('host', gettext('Host'), &$provider, &$post, &$errors);
	v_port(&$provider, &$post, &$errors);
	v_required('username', gettext('Username'), &$provider, &$post, &$errors);
	v_numeric('readbacknumber', gettext('Public Number'), &$provider, &$post, &$errors);
	v_incoming_prov_routing(&$provider, &$post, &$errors);
	v_incoming_prov_callerid(&$provider, &$post, &$errors);
	v_outgoing_prov_routing(&$provider, &$post, &$errors);
	v_outgoing_prov_callerid(&$provider, &$post, &$errors);
	v_codecs(&$provider, &$post);
	v_manualattributes(&$provider, &$post, &$errors);

	$optional = array(
		'failover',
		'secret',
		'fromuser',
		'authuser',
		'fromdomain',
		'noregister',
		'manualregister'
	);
	foreach ($optional as $o) {
		$provider[$o] = v_strip($post[$o]);
	}

	$trusted = array(
		'uniqid',
		'language',
		'natmode',
		'dtmfmode',
		'qualify'
	);
	foreach ($trusted as $t) {
		$provider[$t] = $post[$t];
	}

	return $provider;
}

function sip_save_provider($provider) {
	global $g, $uniqid_map;

	if (sip_get_provider($provider['uniqid'])) {
		$GLOBALS['config']['sip']['provider'][$uniqid_map[$provider['uniqid']]] = $provider;
	} else {
		$GLOBALS['config']['sip']['provider'][] = $provider;
	}

	write_config();
	touch($g['sip_dirty_path']);
}

?>
