#!/usr/bin/php
<?php 
/*
	$Id$
	part of m0n0wall (http://m0n0.ch/wall)
	
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
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

require("guiconfig.inc");

$pgtitle = array(gettext("System"), gettext("General Setup"));
$pghelp = gettext("This is the first page which needs configuring upon installing a new system. Most importantly, the default password should be changed. Then, ensure your host/domain name, time zone and country's indication tones are set correctly before continuing.");

$pconfig['language'] = $config['system']['webgui']['language'];
$pconfig['hostname'] = $config['system']['hostname'];
$pconfig['domain'] = $config['system']['domain'];
$pconfig['username'] = $config['system']['username'];
if (!$pconfig['username']) {
	$pconfig['username'] = "admin";
}
$pconfig['webguiproto'] = $config['system']['webgui']['protocol'];
if (!$pconfig['webguiproto']) {
	$pconfig['webguiproto'] = "http";
}
$pconfig['webguiport'] = $config['system']['webgui']['port'];
$pconfig['tonezone'] = $config['system']['tonezone'];
$pconfig['timezone'] = $config['system']['timezone'];
$pconfig['timeupdateinterval'] = $config['system']['time-update-interval'];
$pconfig['timeservers'] = $config['system']['timeservers'];

if (!isset($pconfig['timeupdateinterval'])) {
	$pconfig['timeupdateinterval'] = $defaults['system']['timeupdateinterval']
}
if (!$pconfig['timeservers']) {
	$pconfig['timeservers'] = $defaults['system']['timeservers']
}

if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;

	/* input validation */
	$reqdfields = split(" ", "hostname domain username");
	$reqdfieldsn = split(",", gettext("Hostname,Domain,Username"));
	
	verify_input($_POST, $reqdfields, $reqdfieldsn, &$input_errors);
	
	if ($_POST['hostname'] && !verify_is_hostname($_POST['hostname'])) {
		$input_errors[] = gettext("The hostname may only contain the characters a-z, 0-9 and '-'.");
	}
	if ($_POST['domain'] && !verify_is_domain($_POST['domain'])) {
		$input_errors[] = gettext("The domain may only contain the characters a-z, 0-9, '-' and '.'.");
	}
	if ($_POST['username'] && !preg_match("/^[a-zA-Z0-9]*$/", $_POST['username'])) {
		$input_errors[] = gettext("The username may only contain the characters a-z, A-Z and 0-9.");
	}
	if ($_POST['webguiport'] && (!verify_is_numericint($_POST['webguiport']) || 
			($_POST['webguiport'] < 1) || ($_POST['webguiport'] > 65535))) {
		$input_errors[] = gettext("A valid TCP/IP port must be specified for the webGUI port.");
	}
	if (($_POST['password']) && ($_POST['password'] != $_POST['password2'])) {
		$input_errors[] = gettext("The passwords do not match.");
	}
	
	$t = (int)$_POST['timeupdateinterval'];
	if (($t < 0) || (($t > 0) && ($t < 6)) || ($t > 1440)) {
		$input_errors[] = gettext("The time update interval must be either 0 (disabled) or between 6 and 1440.");
	}
	foreach (explode(' ', $_POST['timeservers']) as $ts) {
		if (!verify_is_domain($ts)) {
			$input_errors[] = gettext("A NTP Time Server name may only contain the characters a-z, 0-9, '-' and '.'.");
		}
	}

	if (!$input_errors) {
		$config['system']['hostname'] = strtolower($_POST['hostname']);
		$config['system']['domain'] = strtolower($_POST['domain']);
		$oldwebguiproto = $config['system']['webgui']['protocol'];
		$config['system']['username'] = $_POST['username'];
		$config['system']['webgui']['protocol'] = $pconfig['webguiproto'];
		$oldwebguiport = $config['system']['webgui']['port'];
		$config['system']['webgui']['port'] = $pconfig['webguiport'];
		$config['system']['tonezone'] = $_POST['tonezone'];
		if ($_POST['timezone'] != "0000") {
			$config['system']['timezone']
				= $_POST['timezone'] . "|" . $tz["{$_POST['timezone']}"][1];
		} else {
			$config['system']['timezone'] = false;
		}
		$config['system']['timeservers'] = strtolower($_POST['timeservers']);
		$config['system']['time-update-interval'] = $_POST['timeupdateinterval'];
		$config['system']['webgui']['language'] = $_POST['lang'];
				
		if ($_POST['password']) {
			$config['system']['password'] = $_POST['password'];
			touch($d_passworddirty_path);
		}
		
		write_config();
		
		if (($oldwebguiproto != $config['system']['webgui']['protocol']) ||
			($oldwebguiport != $config['system']['webgui']['port']))
			touch($d_sysrebootreqd_path);
		
		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval = system_hostname_configure();
			$retval |= system_hosts_generate();
			$retval |= system_timezone_configure();
 			$retval |= system_cron_configure();
			$retval |= indications_conf_generate();
			$retval |= indications_reload();
			config_unlock();
		}
		
		$savemsg = get_std_save_message($retval);
	}
}

include("fbegin.inc");

if ($input_errors) display_input_errors($input_errors);
if ($savemsg) display_info_box($savemsg); ?>

<form action="system.php" method="post">
<table width="100%" border="0" cellpadding="6" cellspacing="0">
	<tr> 
		<td width="22%" valign="top" class="vncellreq"><?=gettext("Hostname");?></td>
		<td width="78%" class="vtable">
			<input name="hostname" type="text" class="formfld" id="hostname" size="40" value="<?=htmlspecialchars($pconfig['hostname']);?>">
	    	<br>
			<span class="vexpl"><?=gettext("name of the pbx host, without domain part");?>
			<br>
			<?=gettext("e.g. <em>pbx");?></em></span>
		</td>
	</tr>
	<tr> 
		<td valign="top" class="vncellreq"><?=gettext("Domain");?></td>
		<td class="vtable">
			<input name="domain" type="text" class="formfld" id="domain" size="40" value="<?=htmlspecialchars($pconfig['domain']);?>">
			<br>
			<span class="vexpl"><?=gettext("e.g. <em>mycorp.com");?></em></span>
		</td>
	</tr>
	<tr> 
		<td valign="top" class="vncell"><?=gettext("Username");?></td>
		<td class="vtable">
			<input name="username" type="text" class="formfld" id="username" size="20" value="<?=$pconfig['username'];?>">
			<br>
			<span class="vexpl"><?=gettext("If you want to change the username for accessing the webGUI, enter it here.");?></span>
		</td>
	</tr>
	<tr> 
		<td valign="top" class="vncell"><?=gettext("Password");?></td>
		<td class="vtable">
			<input name="password" type="password" class="formfld" id="password" size="20">
			<br>
			<input name="password2" type="password" class="formfld" id="password2" size="20">
			&nbsp;(<?=gettext("confirmation");?>) <br> <span class="vexpl"><?=gettext("If you want to change the password for accessing the webGUI, enter it here twice.");?></span>
		</td>
	</tr>
	<? display_gui_language_selector($config['system']['webgui']['language']); ?>
	<tr> 
		<td valign="top" class="vncell"><?=gettext("webGUI protocol");?></td>
		<td class="vtable">
			<input name="webguiproto" type="radio" value="http" <? if ($pconfig['webguiproto'] == "http") echo "checked"; ?>>
			<?=gettext("HTTP");?> &nbsp;&nbsp;&nbsp; <input type="radio" name="webguiproto" value="https" <? if ($pconfig['webguiproto'] == "https") echo "checked"; ?>><?=gettext("HTTPS");?>
		</td>
	</tr>
	<tr> 
		<td valign="top" class="vncell"><?=gettext("webGUI port");?></td>
		<td class="vtable">
			<input name="webguiport" type="text" class="formfld" id="webguiport" size="5" value="<?=htmlspecialchars($pconfig['webguiport']);?>">
			<br>
			<span class="vexpl"><?=gettext("Enter a custom port number for the webGUI above if you want to override the default (80 for HTTP, 443 for HTTPS).");?></span>
		</td>
	</tr>
	<tr> 
		<td valign="top" class="vncell"><?=gettext("Indications Tonezone");?></td>
		<td class="vtable">
			<select name="tonezone" id="tonezone"><?
			foreach ($system_tonezones as $abbreviation => $friendly) {
				?><option value="<?=htmlspecialchars($abbreviation);?>" <? if ($abbreviation == $pconfig['tonezone']) echo "selected"; ?>>
				<?=htmlspecialchars($friendly);?></option><?
			}
			?></select>
			<br>
			<span class="vexpl"><?=gettext("Select which country's indication tones are to be used.");?></span>
		</td>
	</tr>
	<tr> 
		<td valign="top" class="vncell"><?=gettext("Time zone");?></td>
		<td class="vtable">
			<select name="timezone" id="timezone">
				<option value="0000">UTC</option><?
			if ($pconfig['timezone']) {
				$stored_id = explode("|", $pconfig['timezone']);
				$stored_id = $stored_id[0];
			}
			foreach ($tz as $id => $t) {
				?><option value="<?=$id;?>" <? if ($id == $stored_id) echo "selected"; ?>>
				<?=htmlspecialchars($t[0]);?></option><?
			}
			?></select>
			<br>
			<span class="vexpl"><?=gettext("Select the location closest to you");?></span>
		</td>
	</tr>
	<tr> 
		<td valign="top" class="vncell"><?=gettext("Time update interval");?></td>
		<td class="vtable">
			<input name="timeupdateinterval" type="text" class="formfld" id="timeupdateinterval" size="4" value="<?=htmlspecialchars($pconfig['timeupdateinterval']);?>">
			<br>
			<span class="vexpl"><?=gettext("Minutes between network time sync.; 300 recommended, or 0 to disable");?></span>
		</td>
	</tr>
	<tr> 
		<td valign="top" class="vncell"><?=gettext("NTP time server");?></td>
		<td class="vtable">
			<input name="timeservers" type="text" class="formfld" id="timeservers" size="40" value="<?=htmlspecialchars($pconfig['timeservers']);?>">
			<br>
			<span class="vexpl"><?=gettext("Enter a NTP server to synchronize with. Remember to set up at least one DNS server if you enter a host name here!");?></span>
		</td>
	</tr>
	<tr> 
		<td valign="top">&nbsp;</td>
		<td>
			<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>"> 
		</td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
