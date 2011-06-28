<?php
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2010 tecema (a.k.a IKT) <http://www.tecema.de>.
	All rights reserved.
	
	Askozia®PBX is a registered trademark of tecema. Any unauthorized use of
	this trademark is prohibited by law and international treaties.
	
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
	
	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.
	
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	
	3. Redistribution in any form at a charge, that in whole or in part
	   contains or is derived from the software, including but not limited to
	   value added products, is prohibited without prior written consent of
	   tecema.
	
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

require_once("functions.inc");

/**
 * Returns all providers sorted by name.
 * If no providers are defined, the global array is initialized.
 * This should be changed to return by reference to eliminate a lot of repetitive code.
 */
function iax_get_providers() {

	if (!is_array($GLOBALS['config']['iax']['provider'])) {
		$GLOBALS['config']['iax']['provider'] = array();
	}

	$tmp = $GLOBALS['config']['iax']['provider'];
	usort($tmp, "pbx_sort_by_name");

	return $tmp;
}

/**
 * Returns a specific provider based on its uniqid
 */
function iax_get_provider($uniqid) {
	global $uniqid_map;

	return $GLOBALS['config']['iax']['provider'][$uniqid_map[$uniqid]];
}

/**
 * Returns all phones sorted by extension.
 * If no phones are defined, the global array is initialized.
 */
function iax_get_phones() {

	if (!is_array($GLOBALS['config']['iax']['phone'])) {
		$GLOBALS['config']['iax']['phone'] = array();
	}

	$tmp = $GLOBALS['config']['iax']['phone'];
	usort($tmp, "pbx_sort_by_extension");

	return $tmp;
}

/**
 * Returns a specific phone based on its uniqid
 */
function iax_get_phone($uniqid) {
	global $uniqid_map;

	return $GLOBALS['config']['iax']['phone'][$uniqid_map[$uniqid]];
}

/**
 * Returns an array of all assigned phone extensions
 */
function iax_get_extensions() {

	$extensions = array();

	if (is_array($GLOBALS['config']['iax']['phone'])) {
		foreach($GLOBALS['config']['iax']['phone'] as $phone) {
			$extensions[$phone['uniqid']] = $phone['extension'];
		}
	}

	return $extensions;
}

/**
 * Returns an array of all assigned provider dialpatterns
 */
function iax_get_dialpatterns() {

	$dialpatterns = array();

	if (is_array($GLOBALS['config']['iax']['provider'])) {
		foreach($GLOBALS['config']['iax']['provider'] as $provider) {
			$dialpatterns[$provider['uniqid']] = $provider['dialpattern'];
		}
	}

	return $dialpatterns;
}

/**
 * Generates iax.conf
 */
function iax_conf_generate($return=false) {
	global $config, $g;

	$iaxconfig = $config['services']['iax'];
	
	$port = "4569";
	if (isset($iaxconfig['port']))
		$port = $iaxconfig['port'];

	$conf = <<<EOD
	
[general]
context=public-direct-dial
bindport=$port
bindaddr=0.0.0.0
delayreject=yes

EOD;

	if (isset($iaxconfig['jbenable'])) {
		$conf .= "jitterbuffer=yes\n";
	} else {
		$conf .= "jitterbuffer=no\n";
	}

	if (isset($iaxconfig['jbforce'])) {
		$conf .= "forcejitterbuffer=yes\n";
	} else {
		$conf .= "forcejitterbuffer=no\n";
	}

	$conf .= util_gen_manual_attributes($iaxconfig['manual-attribute']);
	
	$conf .= "\n\n";

	
	// providers
	$iaxproviders = iax_get_providers();

	// provider registrations
	foreach($iaxproviders as $p) {
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
			if (isset($p['useoutkey'])) {
				$conf .= ":[{$p['uniqid']}]";
			} else if ($p['secret']) {
				$conf .= ":{$p['secret']}";
			}
			$conf .= "@{$p['host']}";
			if ($p['port']) {
				$conf .= ":{$p['port']}";
			}
		}

		$conf .= "\n\n";
	}
	
	// provider definitions
	foreach($iaxproviders as $p) {
		if (isset($p['disabled'])) {
			continue;
		}
		// outgoing (peer)
		$conf .= "; {$p['name']}\n";
		$conf .= "[{$p['uniqid']}]\n";
		$conf .= "type=peer\n";
		$conf .= "username={$p['username']}\n";
		
		if (isset($p['useoutkey'])) {
			$conf .= "auth=rsa\n";
			$conf .= "outkey={$p['uniqid']}\n";
			util_file_write("/etc/asterisk/keys/" . $p['uniqid'] . ".key", base64_decode($p['outkey']));
		} else if ($p['authentication'] == "md5") {
			$conf .= "auth=md5\n";
			$conf .= "secret={$p['secret']}\n";
		} else {
			$conf .= "auth=plaintext\n";
			$conf .= "secret={$p['secret']}\n";
		}

		$conf .= "host={$p['host']}\n";
		
		if ($p['port'])
			$conf .= "port={$p['port']}\n";
			
		// language selection
		if ($p['language']) {
			$conf .= "language=" . $p['language'] . "\n";
		} else {
			$conf .= "language=en-us\n";
		}
		
		// qualify options
		if ($p['qualify'] == "0") {
			$conf .= "qualify=no\n";
		} else if ($p['qualify']) {
			$conf .= "qualify=". $p['qualify']*1000 ."\n";
		} else {
			$conf .= "qualify=yes\n";
		}

		$conf .= "transfer=mediaonly\n";			
		$conf .= "disallow=all\n";
	
		if(is_array($p['codec'])) {
			foreach ($p['codec'] as $codec)
			{
				if(strlen($codec) > 0)
				{
					$conf .= "allow=$codec\n";
				}
			}
		} else {
			if(strlen($p['codec']) > 0)
			{
				$conf .= "allow={$p['codec']}\n";
			}
		}
		$conf .= "permit=0.0.0.0/0.0.0.0\n";

		$conf .= util_gen_manual_attributes($p['manual-attribute']);

		$conf .= "\n\n";
		
		// incoming (user)
		$conf .= "; {$p['name']} - incoming\n";
		$conf .= "[{$p['username']}]\n";
		$conf .= "type=user\n";
		
		if (isset($p['useinkey'])) {
			$conf .= "auth=rsa\n";
			$conf .= "inkeys={$p['uniqid']}\n";
			util_file_write("/etc/asterisk/keys/" . $p['uniqid'] . ".pub", base64_decode($p['inkey']));
		} else if ($p['authentication'] == "md5") {
			$conf .= "auth=md5\n";
			$conf .= "secret={$p['secret']}\n";
		} else {
			$conf .= "auth=plaintext\n";
			$conf .= "secret={$p['secret']}\n";
		}
		
		$conf .= "context={$p['uniqid']}-incoming\n";
		$conf .= "host={$p['host']}\n";
		
		if ($p['port'])
			$conf .= "port={$p['port']}\n";
			
		// language selection
		if ($p['language']) {
			$conf .= "language=" . $p['language'] . "\n";
		} else {
			$conf .= "language=en-us\n";
		}
		
		// qualify options
		if ($p['qualify'] == "0") {
			$conf .= "qualify=no\n";
		} else if ($p['qualify']) {
			$conf .= "qualify=". $p['qualify']*1000 ."\n";
		} else {
			$conf .= "qualify=yes\n";
		}

		$conf .= "transfer=mediaonly\n";			
		$conf .= "disallow=all\n";
	
		if(is_array($p['codec'])) {
			foreach ($p['codec'] as $codec)
			{
				if(strlen($codec) > 0)
				{
					$conf .= "allow=$codec\n";
				}
			}
		} else {
			if(strlen($p['codec']) > 0)
			{
				$conf .= "allow={$p['codec']}\n";
			}
		}
		$conf .= "permit=0.0.0.0/0.0.0.0\n";

		if ($p['manualattributes']) {
			$conf .= "; manual attributes\n";
			$conf .= base64_decode($p['manualattributes']) . "\n";
			$conf .= "; end manual attributes\n";
		}

		$conf .= "\n\n";
	}

	// phones
	$phones = iax_get_phones();
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
		$conf .= "auth=" . $p['authentication'] . "\n";
		$conf .= "secret=" . $p['secret'] . "\n";
		$conf .= "context=" . $p['uniqid'] . "\n";
		$conf .= "callerid=" . $p['callerid'] . " <" . $p['extension'] . ">\n";
		$conf .= "host=dynamic\n";
		$conf .= "language=" . $p['language'] . "\n";
		$conf .= "qualify=yes\n";
		$conf .= "transfer=mediaonly\n";

		$conf .= "disallow=all\n";
		if(is_array($p['codec'])) {
			foreach ($p['codec'] as $codec) {
				if(strlen($codec) > 0)
				{
					$conf .= "allow=" . $codec . "\n";
				}
			}
		}

		if ($p['manualattributes']) {
			$conf .= "; manual attributes\n";
			$conf .= base64_decode($p['manualattributes']) . "\n";
			$conf .= "; end manual attributes\n";
		}

		$conf .= "\n\n";
	}

	return ($return) ? $conf : util_file_write("/etc/asterisk/iax.conf", $conf);
}

function iax_generate_default_phone() {
	global $defaults;

	$phone = array();

	$phone['extension'] = pbx_get_next_default_phone_extension();
	$phone['callerid'] = $defaults['accounts']['phones']['callerid'];
	$phone['language'] = $defaults['accounts']['language'];
	$phone['ringlength'] = $defaults['accounts']['phones']['ringlength'];
	$phone['publicaccess'] = $defaults['accounts']['phones']['publicaccess'];
	$phone['uniqid'] = "IAX-PHONE-" . uniqid(rand());

	$phone['secret'] = util_pwgen();
	$phone['codec'] = $defaults['accounts']['codec'];
	$phone['authentication'] = $defaults['iax']['authentication'];
	$phone['vmpin'] = rand(1000,9999);
	
	return $phone;
}

function iax_verify_phone($post, $errors) {

	$phone = array();

	v_cleanliness(&$post, &$errors);
	v_extension(&$phone, &$post, &$errors);
	v_callerid(&$phone, &$post, &$errors);
	v_optional('descr', &$phone, &$post);
	v_public_access_editor(&$phone, &$post, &$errors);
	v_notifications_editor(&$phone, &$post, &$errors);
	v_voicemail_editor(&$phone, &$post, &$errors);
	v_provider_access_selector(&$phone, &$post);
	v_outgoing_callerid_map(&$phone, &$post, &$errors);
	v_codecs(&$phone, &$post);
	v_password(&$phone, &$post);
	v_manualattributes(&$phone, &$post, &$errors);

	$trusted = array(
		'uniqid',
		'language',
		'ringlength',
		'authentication'
	);
	foreach ($trusted as $t) {
		$phone[$t] = $post[$t];
	}

	return $phone;
}

function iax_save_phone($phone) {
	global $g, $uniqid_map;

	if (iax_get_phone($phone['uniqid'])) {
		$GLOBALS['config']['iax']['phone'][$uniqid_map[$phone['uniqid']]] = $phone;
	} else {
		$GLOBALS['config']['iax']['phone'][] = $phone;
	}

	write_config();
	touch($g['iax_dirty_path']);
}

function iax_generate_default_provider() {
	global $defaults;

	$provider = array();

	$provider['language'] = $defaults['accounts']['language'];
	$provider['codec'] = $defaults['accounts']['codec'];
	$provider['qualify'] = $defaults['accounts']['qualify'];
	$provider['calleridsource'] = $defaults['accounts']['providers']['calleridsource'];
	$provider['authentication'] = $defaults['iax']['authentication'];
	$provider['uniqid'] = 'IAX-PROVIDER-' . uniqid(rand());

	return $provider;
}

function iax_verify_provider($post, $errors) {

	$provider = array();

	v_cleanliness(&$post, &$errors);
	v_required('name', gettext('Name'), &$provider, &$post, &$errors);
	v_required('host', gettext('Host'), &$provider, &$post, &$errors);
	v_port(&$provider, &$post, &$errors);
	v_required('username', gettext('Username'), &$provider, &$post, &$errors);
	v_checkbox('useinkey', &$provider, &$post);
	v_optional('inkey', &$provider, &$post, "base64");
	v_checkbox('useoutkey', &$provider, &$post);
	v_optional('outkey', &$provider, &$post, "base64");
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
		'noregister',
		'manualregister'
	);
	foreach ($optional as $o) {
		$provider[$o] = v_strip($post[$o]);
	}

	$trusted = array(
		'uniqid',
		'language',
		'authentication',
		'qualify'
	);
	foreach ($trusted as $t) {
		$provider[$t] = $post[$t];
	}

	return $provider;
}

function iax_save_provider($provider) {
	global $g, $uniqid_map;

	if (iax_get_provider($provider['uniqid'])) {
		$GLOBALS['config']['iax']['provider'][$uniqid_map[$provider['uniqid']]] = $provider;
	} else {
		$GLOBALS['config']['iax']['provider'][] = $provider;
	}

	write_config();
	touch($g['iax_dirty_path']);
}

?>
