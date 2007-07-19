#!/usr/local/bin/php
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
	
	THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
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

$pgtitle = array("Dialplan", "Providers");
require("guiconfig.inc");


if ($_POST) {

	unset($input_errors);
	
	$post_keys = array_keys($_POST);

	$sip_provider_incomingextension_pairs = array();
	$sip_provider_dialpatterns = array();
	$sip_phone_pairs = array();

	$iax_provider_incomingextension_pairs = array();
	$iax_provider_dialpatterns = array();
	$iax_phone_pairs = array();

	$isdn_provider_incomingextension_pairs = array();
	$isdn_provider_dialpatterns = array();
	$isdn_phone_pairs = array();

	foreach($post_keys as $post_key) {
		
		$key_split = explode(":", $post_key);

		// phone permission, phone -> provider pairs
		if (strpos($key_split[1], "SIP-PHONE") !== false) {
			$sip_phone_pairs[] = array($key_split[1], $_POST[$post_key]);

		} else if (strpos($key_split[1], "IAX-PHONE") !== false) {
			$iax_phone_pairs[] = array($key_split[1], $_POST[$post_key]);
			
		} else if (strpos($key_split[1], "ISDN-PHONE") !== false) {
			$isdn_phone_pairs[] = array($key_split[1], $_POST[$post_key]);
			
		// incoming extension, provider -> phone pairs
		} else if (strpos($key_split[1], "incomingextension") !== false) {
			
			if (strpos($key_split[0], "SIP-PROVIDER") !== false) {
				$sip_provider_incomingextension_pairs[] = array($key_split[0], $_POST[$post_key]);

			} else if (strpos($key_split[0], "IAX-PROVIDER") !== false) {
				$iax_provider_incomingextension_pairs[] = array($key_split[0], $_POST[$post_key]);

			} else if (strpos($key_split[0], "ISDN-PROVIDER") !== false) {
					$isdn_provider_incomingextension_pairs[] = array($key_split[0], $_POST[$post_key]);
			}
		
		// save dialpatterns
		} else if (strpos($key_split[1], "dialpattern") !== false) {

			$dialpatterns = split_and_clean_patterns($_POST[$post_key]);
			
			foreach($dialpatterns as $pattern) {
				if (!asterisk_is_valid_dialpattern($pattern, &$internal_error)) {
					$input_errors[] = "An invalid dial-pattern ($pattern) was found for \"" .
					 	asterisk_uniqid_to_name($key_split[0]) . "\". $internal_error";
				}
				if (asterisk_dialpattern_exists($p, &$return_provider_name, $key_split[0])) {
					$input_errors[] = "The dial-pattern \"$p\" defined for \"".
						asterisk_uniqid_to_name($key_split[0]) . "\" already exists for \"$return_provider_name\".";
				}
			}
			
			if (strpos($key_split[0], "SIP-PROVIDER") !== false) {
				$sip_provider_dialpatterns[$key_split[0]] = $dialpatterns;

			} else if (strpos($key_split[0], "IAX-PROVIDER") !== false) {
				$iax_provider_dialpatterns[$key_split[0]] = $dialpatterns;
			
			} else if (strpos($key_split[0], "ISDN-PROVIDER") !== false) {
				$isdn_provider_dialpatterns[$key_split[0]] = $dialpatterns;
			}
		}
	}

	if (!$input_errors) {
		
		// clear phone to provider mappings
		$n = count($config['sip']['phone']);
		for ($i = 0; $i < $n; $i++) {
			if (isset($config['sip']['phone'][$i]['provider']))
				unset($config['sip']['phone'][$i]['provider']);
		}
		$n = count($config['iax']['phone']);
		for ($i = 0; $i < $n; $i++) {
			if (isset($config['iax']['phone'][$i]['provider']))
				unset($config['iax']['phone'][$i]['provider']);
		}
		$n = count($config['isdn']['phone']);
		for ($i = 0; $i < $n; $i++) {
			if (isset($config['isdn']['phone'][$i]['provider']))
				unset($config['isdn']['phone'][$i]['provider']);
		}
		
		// clear provider patterns
		$n = count($config['sip']['provider']);
		for ($i = 0; $i < $n; $i++) {
			if (isset($config['sip']['provider'][$i]['dialpattern']))
				unset($config['sip']['provider'][$i]['dialpattern']);
		}
		$n = count($config['iax']['provider']);
		for ($i = 0; $i < $n; $i++) {
			if (isset($config['iax']['provider'][$i]['dialpattern']))
				unset($config['iax']['provider'][$i]['dialpattern']);
		}
		$n = count($config['isdn']['provider']);
		for ($i = 0; $i < $n; $i++) {
			if (isset($config['isdn']['provider'][$i]['dialpattern']))
				unset($config['isdn']['provider'][$i]['dialpattern']);
		}
		
		// remap phones to providers
		foreach($sip_phone_pairs as $pair) {
			$config['sip']['phone'][$uniqid_map[$pair[0]]]['provider'][] = $pair[1];
		}
		foreach($iax_phone_pairs as $pair) {
			$config['iax']['phone'][$uniqid_map[$pair[0]]]['provider'][] = $pair[1];
		}
		foreach($isdn_phone_pairs as $pair) {
			$config['isdn']['phone'][$uniqid_map[$pair[0]]]['provider'][] = $pair[1];
		}
		
		// remap incoming extensions
		foreach($sip_provider_incomingextension_pairs as $pair) {
			$config['sip']['provider'][$uniqid_map[$pair[0]]]['incomingextension'] = $pair[1];
		}
		foreach($iax_provider_incomingextension_pairs as $pair) {
			$config['iax']['provider'][$uniqid_map[$pair[0]]]['incomingextension'] = $pair[1];
		}
		foreach($isdn_provider_incomingextension_pairs as $pair) {
			$config['isdn']['provider'][$uniqid_map[$pair[0]]]['incomingextension'] = $pair[1];
		}
		
		// remap dialpatterns
		foreach($sip_provider_dialpatterns as $providerid => $patterns) {
			$config['sip']['provider'][$uniqid_map[$providerid]]['dialpattern'] = $patterns;
		}
		foreach($iax_provider_dialpatterns as $providerid => $patterns) {
			$config['iax']['provider'][$uniqid_map[$providerid]]['dialpattern'] = $patterns;
		}
		foreach($isdn_provider_dialpatterns as $providerid => $patterns) {
			$config['isdn']['provider'][$uniqid_map[$providerid]]['dialpattern'] = $patterns;
		}
		
		write_config();
		touch($d_extensionsconfdirty_path);
		header("Location: dialplan_providers.php");
		exit;
	}
}

if (file_exists($d_extensionsconfdirty_path)) {
	$retval = 0;
	config_lock();
	$retval |= extensions_conf_generate();
	config_unlock();

	$retval |= extensions_reload();

	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($d_extensionsconfdirty_path);
	}
}

include("fbegin.inc");
if ($input_errors)
	print_input_errors($input_errors);
if ($savemsg)
	print_info_box($savemsg);
	
	?><form action="dialplan_providers.php" method="post" name="iform" id="iform">
		<table width="100%" border="0" cellpadding="6" cellspacing="0"><?

		$a_providers = asterisk_get_providers();
		
		if (!is_array($a_providers)) {
			
			?><tr> 
				<td><strong>There are currently no providers defined.</strong></td>
			</tr><?
			
		} else {
			foreach($a_providers as $provider) {
				
				?><tr> 
					<td colspan="2" valign="top" class="listtopic">
						<?=$provider['name']?>
						<? if (strpos($provider['uniqid'], "ISDN") !== false): ?>
						(ISDN)
						<? else: ?>
						(<?=$provider['username']?>@<?=$provider['host']?>)
						<? endif; ?>
					</td>
				</tr>
				<? display_provider_dialpattern_editor($provider['dialpattern'], 1, $provider['uniqid']); ?>
				<? display_incoming_extension_selector($provider['incomingextension'], 1, $provider['uniqid']); ?>
				<? display_phone_access_selector($provider['uniqid'], 1, $provider['uniqid']); ?>
				<tr> 
					<td colspan="2" class="list" height="12">&nbsp;</td>
				</tr><?
					
			}

			?><tr>
				<td colspan="2" valign="top" class="listhelptopic">Help</td>
			</tr>
			<tr>
				<td width="20%" valign="top" class="vncell">Dialing Pattern(s)</td>
				<td width="80%" class="vtable">
					<span class="vexpl"><?=$help[$help_language]["display_provider_dialpattern_editor"]; ?></span>
				</td>
			</tr>
			<tr>
				<td width="20%" valign="top" class="vncell">Incoming Extension</td>
				<td width="80%" class="vtable">
					<span class="vexpl"><?=$help[$help_language]["display_incoming_extension_selector"]; ?></span>
				</td>
			</tr>
			<tr>
				<td width="20%" valign="top" class="vncell">Phones</td>
				<td width="80%" class="vtable">
					<span class="vexpl"><?=$help[$help_language]["display_phone_access_selector"]; ?></span>
				</td>
			</tr>
			<tr> 
				<td valign="top">&nbsp;</td>
				<td>
					<input name="Submit" type="submit" class="formbtn" value="Save">
				</td>
			</tr><?
		}
		
		?></table>
	</form>
<?php include("fend.inc"); ?>
