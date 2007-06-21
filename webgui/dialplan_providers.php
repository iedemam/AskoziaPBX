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

$pgtitle = array("Dialplan", "Providers");
require("guiconfig.inc");


if ($_POST) {

	unset($input_errors);
	
	$post_keys = array_keys($_POST);

	$sip_provider_incomingextension_pairs = array();
	$sip_provider_prefixes = array();
	$sip_phone_pairs = array();
	$iax_provider_incomingextension_pairs = array();
	$iax_provider_prefixes = array();
	$iax_phone_pairs = array();

	foreach($post_keys as $post_key) {
		
		$key_split = explode(":", $post_key);

		// phone permission, phone -> provider pairs
		if (strpos($key_split[1], "SIP-PHONE") !== false) {
			$sip_phone_pairs[] = array($key_split[1], $_POST[$post_key]);

		} else if (strpos($key_split[1], "IAX-PHONE") !== false) {
			$iax_phone_pairs[] = array($key_split[1], $_POST[$post_key]);
			
		// incoming extension, provider -> phone pairs
		} else if (strpos($key_split[1], "incomingextension") !== false) {
			
			if (strpos($key_split[0], "SIP-PROVIDER") !== false) {
				$sip_provider_incomingextension_pairs[] = array($key_split[0], $_POST[$post_key]);

			} else if (strpos($key_split[0], "IAX-PROVIDER") !== false) {
				$iax_provider_incomingextension_pairs[] = array($key_split[0], $_POST[$post_key]);
			}
		
		// prefix or pattern?
		} else if (strpos($key_split[1], "prefixorpattern") !== false) {
			
			$prefixorpattern = $_POST[$post_key];
			$prefixpatternkey = $key_split[0] . ":" . "prefixpattern";
			$prefixpattern = $_POST[$prefixpatternkey];
			
			if ($prefixorpattern == "prefix") {
				if (!asterisk_is_valid_prefix($prefixpattern)) {
					$input_errors[] = "A valid prefix must be specified for \"" .
					 	asterisk_uniqid_to_name($key_split[0]) . "\".";
				}
			} else if ($prefixorpattern == "pattern") {
				if (!asterisk_is_valid_pattern($prefixpattern)) {
					$input_errors[] = "A valid pattern must be specified for \"" .
					 	asterisk_uniqid_to_name($key_split[0]) . "\".";
				}
			}
			
			if (strpos($key_split[0], "SIP-PROVIDER") !== false) {
				$sip_provider_prefixes[$key_split[0]] = array($prefixorpattern, $prefixpattern);

			} else if (strpos($key_split[0], "IAX-PROVIDER") !== false) {
				$iax_provider_prefixes[$key_split[0]] = array($prefixorpattern, $prefixpattern);
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
		
		// clear provider prefixes and patterns
		$n = count($config['sip']['provider']);
		for ($i = 0; $i < $n; $i++) {
			if (isset($config['sip']['provider'][$i]['prefix']))
				unset($config['sip']['provider'][$i]['prefix']);
			if (isset($config['sip']['provider'][$i]['pattern']))
				unset($config['sip']['provider'][$i]['pattern']);
		}
		$n = count($config['iax']['provider']);
		for ($i = 0; $i < $n; $i++) {
			if (isset($config['iax']['provider'][$i]['prefix']))
				unset($config['iax']['provider'][$i]['prefix']);
			if (isset($config['iax']['provider'][$i]['pattern']))
				unset($config['iax']['provider'][$i]['pattern']);
		}
		
		// remap phones to providers
		foreach($sip_phone_pairs as $pair) {
			$config['sip']['phone'][$uniqid_map[$pair[0]]]['provider'][] = $pair[1];
		}
		foreach($iax_phone_pairs as $pair) {
			$config['iax']['phone'][$uniqid_map[$pair[0]]]['provider'][] = $pair[1];
		}
		
		// remap incoming extensions
		foreach($sip_provider_incomingextension_pairs as $pair) {
			$config['sip']['provider'][$uniqid_map[$pair[0]]]['incomingextension'] = $pair[1];
		}
		foreach($iax_provider_incomingextension_pairs as $pair) {
			$config['iax']['provider'][$uniqid_map[$pair[0]]]['incomingextension'] = $pair[1];
		}
		
		// remap prefixes and patterns
		foreach($sip_provider_prefixes as $providerid => $pair) {
			$config['sip']['provider'][$uniqid_map[$providerid]][$pair[0]] = $pair[1];
		}
		foreach($iax_provider_prefixes as $providerid => $pair) {
			$config['iax']['provider'][$uniqid_map[$providerid]][$pair[0]] = $pair[1];
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
?>
<?php include("fbegin.inc"); ?>
<?php if ($input_errors) print_input_errors($input_errors); ?>
<?php if ($savemsg) print_info_box($savemsg); ?>
	<form action="dialplan_providers.php" method="post" name="iform" id="iform">
		<table width="100%" border="0" cellpadding="6" cellspacing="0"><?

		$provider_count = 0;

		// sip
		$a_sipproviders = sip_get_providers();
		$provider_count = $n = count($a_sipproviders);

		for($i = 0; $i < $n; $i++) {
			$provider = $a_sipproviders[$i];
			?><tr> 
				<td colspan="2" valign="top" class="listtopic">
					<?=$provider['name']?>
					(SIP:&nbsp;<?=$provider['username']?>@<?=$provider['host']?>)
				</td>
			</tr>
			<? display_provider_prefix_pattern_editor($provider['prefix'], $provider['pattern'], 1, $provider['uniqid']); ?>
			<? display_incoming_extension_selector($provider['incomingextension'], 1, $provider['uniqid']); ?>
			<? display_phone_access_selector($provider['uniqid'], 1, $provider['uniqid']); ?>
			<tr> 
				<td colspan="2" class="list" height="12">&nbsp;</td>
			</tr><?
		}

		// iax
		$a_iaxproviders = iax_get_providers();
		$n = count($a_iaxproviders);
		$provider_count += $n;
		
		for($i = 0; $i < $n; $i++) {
			$provider = $a_iaxproviders[$i];
			?><tr> 
				<td colspan="2" valign="top" class="listtopic">
					<?=$provider['name']?>
					(IAX:&nbsp;<?=$provider['username']?>@<?=$provider['host']?>)
				</td>
			</tr>
			<? display_provider_prefix_pattern_editor($provider['prefix'], $provider['pattern'], 1, $provider['uniqid']); ?>
			<? display_incoming_extension_selector($provider['incomingextension'], 1, $provider['uniqid']); ?>
			<? display_phone_access_selector($provider['uniqid'], 1, $provider['uniqid']); ?>
			<tr> 
				<td colspan="2" class="list" height="12">&nbsp;</td>
			</tr><?
		}
			
		if ($provider_count == 0) {
			?><tr> 
				<td><i>There are currently no providers defined.</i></td>
			</tr><?
			
		} else {
			?><tr> 
				<td valign="top">&nbsp;</td>
				<td>
					<input name="Submit" type="submit" class="formbtn" value="Save">
				</td>
			</tr><?
		}
		
		?></table>
	</form>
<?php include("fend.inc"); ?>