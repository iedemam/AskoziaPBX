#!/usr/local/bin/php
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

	$dialpatterns = array();
	$incomingextension_pairs = array();
	$phone_pairs = array();
	
	foreach ($g['technologies'] as $tech) {
		$dialpatterns[$tech] = array();
		$incomingextension_pairs[$tech] = array();
		$phone_pairs[$tech] = array();
	}
	
	// incoming extension maps
	$incomingextensionmaps = gather_incomingextensionmaps($_POST, "multi");
	
	if (is_array($incomingextensionmap)) {
		foreach($incomingextensionmap as $map) {
			if ($map['incomingpattern'] && !pbx_is_valid_dialpattern($map['incomingpattern'], &$internal_error, true)) {
				$input_errors[] = "The incoming extension pattern \"{$map['incomingpattern']}\" for \"" .
					pbx_uniqid_to_name($key_split[0]) . "\"is invalid. $internal_error";
			}
		}
	}

	foreach ($g['technologies'] as $tech) {
		if (strpos($key_split[0], strtoupper($tech) . "-PROVIDER") !== false) {
			$incomingextension_pairs[$tech] = $incomingextensionmap;
		}
	}

	foreach($post_keys as $post_key) {
		
		$key_split = explode("_", $post_key);

		// phone permission, phone -> provider pairs
		foreach ($g['technologies'] as $tech) {
			if (strpos($key_split[1], strtoupper($tech) . "-PHONE") !== false) {
				$phone_pairs[$tech] = array($key_split[1], $_POST[$post_key]);
			}
		}
			
		// save dialpatterns
		if (strpos($key_split[1], "dialpattern") !== false) {

			$dialpatterns = split_and_clean_lines($_POST[$post_key]);

			foreach($dialpatterns as $pattern) {
				if (!pbx_is_valid_dialpattern($pattern, &$internal_error)) {
					$input_errors[] = "An invalid dial-pattern ($pattern) was found for \"" .
					 	pbx_uniqid_to_name($key_split[0]) . "\". $internal_error";
				}
				if (pbx_dialpattern_exists($p, &$return_provider_name, $key_split[0])) {
					$input_errors[] = "The dial-pattern \"$p\" defined for \"".
						pbx_uniqid_to_name($key_split[0]) . "\" already exists for \"$return_provider_name\".";
				}
			}
			
			foreach ($g['technologies'] as $tech) {
				if (strpos($key_split[0], strtoupper($tech) . "-PROVIDER") !== false) {
					$dialpatterns[$tech][$key_split[0]] = $dialpatterns;
				}
			}
		}
	}

	if (!$input_errors) {
		
		// clear phone to provider mappings
		foreach ($g['technologies'] as $tech) {
			$n = count($config[$tech]['phone']);
			for ($i = 0; $i < $n; $i++) {
				if (isset($config[$tech]['phone'][$i]['provider']))
					unset($config[$tech]['phone'][$i]['provider']);
			}
		}
		
		// clear provider patterns
		foreach ($g['technologies'] as $tech) {
			$n = count($config[$tech]['provider']);
			for ($i = 0; $i < $n; $i++) {
				if (isset($config[$tech]['provider'][$i]['dialpattern']))
					unset($config[$tech]['provider'][$i]['dialpattern']);
			}	
		}
		
		// remap phones to providers
		foreach ($g['technologies'] as $tech) {
			foreach($phone_pairs[$tech] as $pair) {
				$config[$tech]['phone'][$uniqid_map[$pair[0]]]['provider'][] = $pair[1];
			}			
		}

		// remap incoming extensions
		foreach ($g['technologies'] as $tech) {
			foreach($incomingextension_pairs[$tech] as $pair) {
				$config[$tech]['provider'][$uniqid_map[$pair[0]]]['incomingextension'] = $pair[1];
			}
		}

		// remap dialpatterns
		foreach ($g['technologies'] as $tech) {
			foreach($dialpatterns[$tech] as $providerid => $patterns) {
				$config[$tech]['provider'][$uniqid_map[$providerid]]['dialpattern'] = $patterns;
			}
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

		$a_providers = pbx_get_providers();
		
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
						<? elseif (strpos($provider['uniqid'], "ANALOG") !== false): ?>
						(Analog)
						<? else: ?>
						(<?=$provider['username']?>@<?=$provider['host']?>)
						<? endif; ?>
					</td>
				</tr>
				<? display_provider_dialpattern_editor($provider['dialpattern'], 1, $provider['uniqid']); ?>
				<? display_incoming_extension_selector(1, $provider['uniqid']); ?>
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
<script language="JavaScript">
<!-- 

<?
foreach ($a_providers as $provider) {
	javascript_incoming_extension_selector($provider['incomingextensionmap'], $provider['uniqid']);	
}
?>

//-->
</script>
<?php include("fend.inc"); ?>
