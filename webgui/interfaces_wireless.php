#!/usr/local/bin/php
<?php 
/*
	$Id: interfaces_opt.php 220 2007-08-25 20:09:05Z mkasper $
	part of m0n0wall (http://m0n0.ch/wall)
	
	Copyright (C) 2003-2007 Manuel Kasper <mk@neon1.net>.
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

$pgtitle = array(gettext("Interfaces"), gettext("Wireless"));
$pghelp = gettext("If a compatible wireless card is present in the system, it can be configured to act as an Access Point. It will be bridged to the wired network that AskoziaPBX is connected to.");
require("guiconfig.inc");
require("interfaces_wlan.inc");

/* XXX : what happens if the wireless interface changes */
if (!is_array($config['interfaces']['wireless'])) {
	$networkinterfaces = wireless_get_interfaces();
	foreach ($networkinterfaces as $ifname => $ifinfo) {
		$newwlan = array();
		$newwlan['if'] = $ifname;
		$config['interfaces']['wireless'] = $newwlan;
		break;
	}
}

$wlstandards = wireless_get_standards($config['interfaces']['wireless']['if']);
$wlchannels = wireless_get_channellist($config['interfaces']['wireless']['if']);

$wlancfg = &$config['interfaces']['wireless'];

$pconfig['enable'] = isset($wlancfg['enable']);
$pconfig['standard'] = $wlancfg['standard'];
$pconfig['ssid'] = $wlancfg['ssid'];
$pconfig['channel'] = $wlancfg['channel'];
$pconfig['wep_enable'] = isset($wlancfg['wep']['enable']);
$pconfig['hidessid'] = isset($wlancfg['hidessid']);

$pconfig['wpamode'] = $wlancfg['wpa']['mode'];
$pconfig['wpaversion'] = $wlancfg['wpa']['version'];
$pconfig['wpacipher'] = $wlancfg['wpa']['cipher'];
$pconfig['wpapsk'] = $wlancfg['wpa']['psk'];
/*$pconfig['radiusip'] = $wlancfg['wpa']['radius']['server'];
$pconfig['radiusauthport'] = $wlancfg['wpa']['radius']['authport'];
$pconfig['radiusacctport'] = $wlancfg['wpa']['radius']['acctport'];
$pconfig['radiussecret'] = $wlancfg['wpa']['radius']['secret'];*/

if (is_array($wlancfg['wep']['key'])) {
	$i = 1;
	foreach ($wlancfg['wep']['key'] as $wepkey) {
		$pconfig['key' . $i] = $wepkey['value'];
		if (isset($wepkey['txkey']))
			$pconfig['txkey'] = $i;
		$i++;
	}
	if (!isset($wepkey['txkey']))
		$pconfig['txkey'] = 1;
}


if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;

	/* input validation */
	if ($_POST['enable']) {
		$reqdfields = "ssid channel";
		$reqdfieldsn = gettext("SSID,Channel");
		
		if ($_POST['wpamode'] != "none") {
			$reqdfields .= " wpaversion wpacipher";
			$reqdfieldsn .= gettext(",WPA version,WPA cipher");
			
			if ($_POST['wpamode'] == "psk") {
				$reqdfields .= " wpapsk";
				$reqdfieldsn .= gettext(",WPA PSK");
			} /*else if ($_POST['wpamode'] == "enterprise") {
				$reqdfields .= " radiusip radiussecret";
				$reqdfieldsn .= ",RADIUS IP,RADIUS Shared secret";
			}*/
		}
		
		do_input_validation($_POST, explode(" ", $reqdfields), explode(",", $reqdfieldsn), &$input_errors);
		
		/*if ($_POST['radiusip'] && !verify_is_ipaddress($_POST['radiusip']))
			$input_errors[] = "A valid RADIUS IP address must be specified.";
		if ($_POST['radiusauthport'] && !verify_is_port($_POST['radiusauthport']))
			$input_errors[] = "A valid RADIUS authentication port number must be specified.";
		if ($_POST['radiusacctport'] && !verify_is_port($_POST['radiusacctport']))
			$input_errors[] = "A valid RADIUS accounting port number must be specified.";*/
	
		if ($_POST['wpapsk'] && !(strlen($_POST['wpapsk']) >= 8 && strlen($_POST['wpapsk']) <= 63))
			$input_errors[] = gettext("The WPA PSK must be between 8 and 63 characters long.");
	
		if (!$input_errors) {
				
			/* if an 11a channel is selected, the mode must be 11a too, and vice versa */
			$is_chan_11a = (strpos($wlchannels[$_POST['channel']]['mode'], "11a") !== false);
			$is_std_11a = ($_POST['standard'] == "11a");
			if ($is_chan_11a && !$is_std_11a)
				$input_errors[] = gettext("802.11a channels can only be selected if the standard is set to 802.11a too.");
			else if (!$is_chan_11a && $is_std_11a)
				$input_errors[] = gettext("802.11a can only be selected if an 802.11a channel is selected too.");

		}
	}

	if (!$input_errors) {
	
		$wlancfg['standard'] = $_POST['standard'];
		$wlancfg['mode'] = "hostap";
		$wlancfg['ssid'] = $_POST['ssid'];
		$wlancfg['channel'] = $_POST['channel'];
		$wlancfg['wep']['enable'] = $_POST['wep_enable'] ? true : false;
		$wlancfg['hidessid'] = $_POST['hidessid'] ? true : false;
		
		$wlancfg['wep']['key'] = array();
		for ($i = 1; $i <= 4; $i++) {
			if ($_POST['key' . $i]) {
				$newkey = array();
				$newkey['value'] = $_POST['key' . $i];
				if ($_POST['txkey'] == $i)
					$newkey['txkey'] = true;
				$wlancfg['wep']['key'][] = $newkey;
			}
		}
		
		$wlancfg['wpa'] = array();
		$wlancfg['wpa']['mode'] = $_POST['wpamode'];
		$wlancfg['wpa']['version'] = $_POST['wpaversion'];
		$wlancfg['wpa']['cipher'] = $_POST['wpacipher'];
		$wlancfg['wpa']['psk'] = $_POST['wpapsk'];
		/*$wlancfg['wpa']['radius'] = array();
		$wlancfg['wpa']['radius']['server'] = $_POST['radiusip'];
		$wlancfg['wpa']['radius']['authport'] = $_POST['radiusauthport'];
		$wlancfg['wpa']['radius']['acctport'] = $_POST['radiusacctport'];
		$wlancfg['wpa']['radius']['secret'] = $_POST['radiussecret'];*/

		$wlancfg['enable'] = $_POST['enable'] ? true : false;

		write_config();
		
		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval = wireless_configure();
			$retval = network_lan_configure();
			config_unlock();
		}
		$savemsg = get_std_save_message($retval);
	}
}

?>

<?php include("fbegin.inc"); ?>
<script language="javascript" src="interfaces_wlan.js"></script>
<?php if ($input_errors) print_input_errors($input_errors); ?>
<?php if ($savemsg) print_info_box($savemsg); ?>

<form action="interfaces_wireless.php" method="post" name="iform" id="iform">
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav"><?

			$tabs = array(
				gettext('Network')	=> 'interfaces_network.php',
				gettext('Wireless')	=> 'interfaces_wireless.php',
				gettext('ISDN')		=> 'interfaces_isdn.php',
				gettext('Analog')	=> 'interfaces_analog.php',
				gettext('Storage')	=> 'interfaces_storage.php'
			);
			dynamic_tab_menu($tabs);
			
			?></ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<table width="100%" border="0" cellpadding="6" cellspacing="0"><?

			if (!count(wireless_get_interfaces())) {
				
				?><tr> 
					<td><strong><?=gettext("No compatible wireless interfaces detected.");?></strong></td>
				</tr><?
	
			} else {

				?><tr> 
					<td width="20%" valign="top" class="vtable">&nbsp;</td>
					<td width="80%" class="vtable">
						<input name="enable" type="checkbox" value="yes" <?php if ($pconfig['enable']) echo "checked"; ?> onClick="wlan_enable_change(false)"><strong><?=gettext("Enable Wireless Interface");?></strong>
					</td>
				</tr>
				<tr>
					<td valign="top" class="vncellreq"><?=gettext("Standard");?></td>
					<td class="vtable">
						<select name="standard" class="formfld" id="standard">
						<? foreach ($wlstandards as $sn): ?>
							<option value="<?=$sn;?>" <?php if ($sn == $pconfig['standard']) echo "selected";?>>
							<?="802.$sn";?>
							</option>
						<?php endforeach; ?>
						</select>
					</td>
				</tr>
				<tr> 
					<td valign="top" class="vncellreq"><?=gettext("SSID");?></td>
					<td class="vtable"><?=$mandfldhtml;?>
						<input name="ssid" type="text" class="formfld" id="ssid" size="20" value="<?=htmlspecialchars($pconfig['ssid']);?>">
                    	<br>
						<br>
						<input type="checkbox" name="hidessid" id="hidessid" value="1" <?php if ($pconfig['hidessid']) echo "checked";?>><strong><?=gettext("Hide SSID");?></strong>
						<br>
						<?=gettext("If this option is selected, the SSID will not be broadcast in hostap mode, and onlyclients that know the exact SSID will be able to connect. Note that this option should never be used as a substitute for proper security/encryption settings.");?>
					</td>
				</tr>
				<tr> 
					<td valign="top" class="vncellreq"><?=gettext("Channel");?></td>
					<td class="vtable">
						<select name="channel" class="formfld" id="channel">
							<option <?php if ($pconfig['channel'] == 0) echo "selected";?> value="0"><?=gettext("Auto");?></option>
							<? foreach ($wlchannels as $channel => $chaninfo):
								if ($chaninfo['mode'] == "11g")
									$mode = "11b/g";
								else
									$mode = $chaninfo['mode'];

								$chandescr = "{$chaninfo['chan']} ({$chaninfo['freq']} MHz, {$mode})";
	  						?><option <?php if ($channel == $pconfig['channel']) echo "selected";?> value="<?=$channel;?>">
							<?=$chandescr;?>
							</option>
							<? endforeach; ?>
						</select>
					</td>
				</tr>
				<tr>
					<td valign="top" class="vncell"><?=gettext("WPA");?></td>
					<td class="vtable">
						<table width="100%" border="0" cellpadding="6" cellspacing="0">
							<tr>
								<td colspan="2" valign="top" class="optsect_t2"><?=gettext("WPA settings");?></td>
							</tr>
							<tr>
								<td class="vncell" valign="top"><?=gettext("Mode");?></td>
								<td class="vtable">
									<select name="wpamode" id="wpamode" onChange="wlan_enable_change(false)">
										<option value="none" <?php if (!$pconfig['wpamode'] || $pconfig['wpamode'] == "none") echo "selected";?>><?=gettext("none");?></option>
										<option value="psk" <?php if ($pconfig['wpamode'] == "psk") echo "selected";?>><?=gettext("PSK");?></option><? /*
										<option value="enterprise" <?php if ($pconfig['wpamode'] == "enterprise") echo "selected";?>>Enterprise</option> */ ?>
									</select>
								</td>
							</tr>
							<tr>
								<td class="vncell" valign="top"><?=gettext("Version");?></td>
								<td class="vtable">
									<select name="wpaversion" id="wpaversion">
										<option value="1" <?php if ($pconfig['wpaversion'] == "1") echo "selected";?>><?=gettext("WPA only");?></option>
										<option value="2" <?php if ($pconfig['wpaversion'] == "2") echo "selected";?>><?=gettext("WPA2 only");?></option>
										<option value="3" <?php if ($pconfig['wpaversion'] == "3") echo "selected";?>><?=gettext("WPA + WPA2");?></option>
									</select>
									<br>
									<?=gettext("In most cases, you should select &quot;WPA + WPA2&quot; here.");?>
								</td>
							</tr>
							<tr>
								<td class="vncell" valign="top"><?=gettext("Cipher");?></td>
								<td class="vtable">
									<select name="wpacipher" id="wpacipher">
										<option value="tkip" <?php if ($pconfig['wpacipher'] == "tkip") echo "selected";?>><?=gettext("TKIP");?></option>
										<option value="ccmp" <?php if ($pconfig['wpacipher'] == "ccmp") echo "selected";?>><?=gettext("AES/CCMP");?></option>
										<option value="both" <?php if ($pconfig['wpacipher'] == "both") echo "selected";?>><?=gettext("TKIP + AES/CCMP");?></option>
									</select>
									<br>
									<?=gettext("AES/CCMP provides better security than TKIP, but TKIP is more compatible with older hardware.");?>
								</td>
							</tr>
							<tr>
								<td class="vncell" valign="top"><?=gettext("PSK");?></td>
								<td class="vtable">
									<input name="wpapsk" type="text" class="formfld" id="wpapsk" size="30" value="<?=htmlspecialchars($pconfig['wpapsk']);?>"><br>
										<?=gettext("Enter the passphrase that will be used in WPA-PSK mode. This must be between 8 and 63 characters long.");?>
								</td>
							</tr><? /*
							<tr> 
							  <td colspan="2" class="list" height="12"></td>
							</tr>
							<tr> 
								<td colspan="2" valign="top" class="optsect_t2">RADIUS server</td>
							</tr>
							<tr>
								<td class="vncell" valign="top">IP address</td>
								<td class="vtable"><input name="radiusip" type="text" class="formfld" id="radiusip" size="20" value="<?=htmlspecialchars($pconfig['radiusip']);?>"><br>
								Enter the IP address of the RADIUS server that will be used in WPA-Enterprise mode.</td>
							</tr>
							<tr>
								<td class="vncell" valign="top">Authentication port</td>
								<td class="vtable"><input name="radiusauthport" type="text" class="formfld" id="radiusauthport" size="5" value="<?=htmlspecialchars($pconfig['radiusauthport']);?>"><br>
								 Leave this field blank to use the default port (1812).</td>
							</tr>
							<tr>
								<td class="vncell" valign="top">Accounting port</td>
								<td class="vtable"><input name="radiusacctport" type="text" class="formfld" id="radiusacctport" size="5" value="<?=htmlspecialchars($pconfig['radiusacctport']);?>"><br>
								 Leave this field blank to use the default port (1813).</td>
							</tr>
							<tr>
								<td class="vncell" valign="top">Shared secret&nbsp;&nbsp;</td>
								<td class="vtable"><input name="radiussecret" type="text" class="formfld" id="radiussecret" size="16" value="<?=htmlspecialchars($pconfig['radiussecret']);?>"></td>
							</tr> */ ?>
						</table>
					</td>
				</tr>
				<tr>
					<td valign="top" class="vncell"><?=gettext("WEP");?></td>
					<td class="vtable">
						<input name="wep_enable" type="checkbox" id="wep_enable" value="yes" <?php if ($pconfig['wep_enable']) echo "checked"; ?>> 
						<strong><?=gettext("Enable WEP");?></strong>
						<table border="0" cellspacing="0" cellpadding="0">
							<tr> 
								<td>&nbsp;</td>
								<td>&nbsp;</td>
								<td>&nbsp;<?=gettext("TX key");?>&nbsp;</td>
							</tr>
							<tr> 
								<td><?=gettext("Key 1:");?>&nbsp;&nbsp;</td>
								<td>
									<input name="key1" type="text" class="formfld" id="key1" size="30" value="<?=htmlspecialchars($pconfig['key1']);?>">
								</td>
								<td align="center">
									<input name="txkey" type="radio" value="1" <?php if ($pconfig['txkey'] == 1) echo "checked";?>>
								</td>
							</tr>
							<tr> 
								<td><?=gettext("Key 2:");?>&nbsp;&nbsp;</td>
								<td>
									<input name="key2" type="text" class="formfld" id="key2" size="30" value="<?=htmlspecialchars($pconfig['key2']);?>">
								</td>
								<td align="center">
									<input name="txkey" type="radio" value="2" <?php if ($pconfig['txkey'] == 2) echo "checked";?>>
								</td>
							</tr>
							<tr>
								<td><?=gettext("Key 3:");?>&nbsp;&nbsp;</td>
								<td>
									<input name="key3" type="text" class="formfld" id="key3" size="30" value="<?=htmlspecialchars($pconfig['key3']);?>">
								</td>
								<td align="center">
									<input name="txkey" type="radio" value="3" <?php if ($pconfig['txkey'] == 3) echo "checked";?>>
								</td>
							</tr>
							<tr>
								<td><?=gettext("Key 4:");?>&nbsp;&nbsp;</td>
								<td>
									<input name="key4" type="text" class="formfld" id="key4" size="30" value="<?=htmlspecialchars($pconfig['key4']);?>">
								</td>
								<td align="center">
									<input name="txkey" type="radio" value="4" <?php if ($pconfig['txkey'] == 4) echo "checked";?>>
								</td>
							</tr>
						</table>
						<br>
						<?=gettext("40 (64) bit keys may be entered as 5 ASCII characters or 10 hex digits preceded by '0x'.");?>
						<br><?=gettext("104 (128) bit keys may be entered as 13 ASCII characters or 26 hex digits preceded by '0x'.");?>
					</td>
				</tr>
				<tr> 
					<td width="20%" valign="top">&nbsp;</td>
					<td width="80%"> 
						<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>"> 
					</td>
				</tr><?

			}

			?></table>
		</td>
	</tr>
</table>
</form>
<script language="JavaScript">
<!--
wlan_enable_change(false);
//-->
</script>
<?php include("fend.inc"); ?>
