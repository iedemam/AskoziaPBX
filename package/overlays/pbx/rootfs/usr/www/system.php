#!/usr/bin/php
<?php 
/*
	$Id$
	part of m0n0wall (http://m0n0.ch/wall)
	continued modifications as part of AskoziaPBX (http://askozia.com/pbx)

	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
	Copyright (C) 2007-2009 tecema (a.k.a IKT) <http://www.tecema.de>.
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
include("timezones.inc");

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
	$pconfig['timeupdateinterval'] = $defaults['system']['timeupdateinterval'];
}
if (!$pconfig['timeservers']) {
	$pconfig['timeservers'] = $defaults['system']['timeservers'];
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
		$config['system']['timezone'] = $_POST['timezone'];
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
			$retval |= pbx_exec("module reload res_indications.so");
			config_unlock();
		}
		
		$savemsg = get_std_save_message($retval);
	}
}


include("fbegin.inc");
?><form action="system.php" method="post">
<table width="100%" border="0" cellpadding="6" cellspacing="0">
	<tr> 
		<td colspan="2" valign="top" class="listtopic"><?=gettext("Security");?></td>
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
	</tr><? /*
	<tr> 
		<td valign="top" class="vncell"><?=gettext("webGUI protocol");?></td>
		<td class="vtable">
			<input name="webguiproto" type="radio" value="http" <? if ($pconfig['webguiproto'] == "http") echo "checked"; ?>>
			<?=gettext("HTTP");?> &nbsp;&nbsp;&nbsp; <input type="radio" name="webguiproto" value="https" <? if ($pconfig['webguiproto'] == "https") echo "checked"; ?>><?=gettext("HTTPS");?>
		</td>
	</tr>
	*/ ?><tr> 
		<td valign="top" class="vncell"><?=gettext("webGUI port");?></td>
		<td class="vtable">
			<input name="webguiport" type="text" class="formfld" id="webguiport" size="5" value="<?=htmlspecialchars($pconfig['webguiport']);?>">
			<br>
			<span class="vexpl"><?=gettext("Enter a custom port number for the webGUI above if you want to override the default (80 for HTTP, 443 for HTTPS).");?></span>
		</td>
	</tr>
	<tr> 
		<td colspan="2" class="list" height="12"></td>
	</tr>
	<tr> 
		<td colspan="2" valign="top" class="listtopic"><?=gettext("Hostname");?></td>
	</tr>
	<tr> 
		<td width="22%" valign="top" class="vncellreq"><?=gettext("Hostname");?></td>
		<td width="78%" class="vtable">
			<input name="hostname" type="text" class="formfld" id="hostname" size="25" value="<?=htmlspecialchars($pconfig['hostname']);?>"> . <input name="domain" type="text" class="formfld" id="domain" size="25" value="<?=htmlspecialchars($pconfig['domain']);?>">
	    	<br>
			<span class="vexpl"><?=gettext("Hostname of the PBX.");?>
			<br>
			<?=gettext("e.g. <em>pbx . mydomain.com");?></em></span>
		</td>
	</tr>
	<tr> 
		<td colspan="2" class="list" height="12"></td>
	</tr>
	<tr> 
		<td colspan="2" valign="top" class="listtopic"><?=gettext("Regional Settings");?></td>
	</tr>
	<? display_gui_language_selector($config['system']['webgui']['language']); ?>
	<tr> 
		<td valign="top" class="vncell"><?=gettext("Indication Tones");?></td>
		<td class="vtable">
			<select name="tonezone" id="tonezone"><?
			foreach ($system_tonezones as $abbreviation => $friendly) {
				?><option value="<?=htmlspecialchars($abbreviation);?>" <? if ($abbreviation == $pconfig['tonezone']) echo "selected"; ?>>
				<?=htmlspecialchars($friendly);?></option><?
			}
			?></select>
			<br>
			<span class="vexpl"><?=gettext("Select which country's indication tones (the ringing, busy and error tones) to be used.");?></span>
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
			<span class="vexpl"><?=gettext("Select the location closest to you.");?></span>
		</td>
	</tr>
	<tr> 
		<td colspan="2" class="list" height="12"></td>
	</tr>
	<tr> 
		<td colspan="2" valign="top" class="listtopic"><?=gettext("Time Synchronization");?></td>
	</tr>
	<tr> 
		<td valign="top" class="vncell"><?=gettext("Update Interval");?></td>
		<td class="vtable">
			<select name="timeupdateinterval" id="timeupdateinterval">
				<option value="disable" <? if ($pconfig['timeupdateinterval'] == "disable") echo "selected";?>><?=gettext("disable time synchronization");?></option>
				<option value="10-minutes" <? if ($pconfig['timeupdateinterval'] == "10-minutes") echo "selected";?>><?=gettext("every 10 minutes");?></option>
				<option value="30-minutes" <? if ($pconfig['timeupdateinterval'] == "30-minutes") echo "selected";?>><?=gettext("every 30 minutes");?></option>
				<option value="1-hour" <? if ($pconfig['timeupdateinterval'] == "1-hour") echo "selected";?>><?=gettext("every hour");?></option>
				<option value="4-hours" <? if ($pconfig['timeupdateinterval'] == "4-hours") echo "selected";?>><?=gettext("every 4 hours");?></option>
				<option value="12-hours" <? if ($pconfig['timeupdateinterval'] == "12-hours") echo "selected";?>><?=gettext("every 12 hours");?></option>
				<option value="1-day" <? if ($pconfig['timeupdateinterval'] == "1-day") echo "selected";?>><?=gettext("every day");?></option>
			</select>
			<br>
			<span class="vexpl"><?=gettext("Select how often the time should be synchronized.");?></span>
		</td>
	</tr>
	<tr> 
		<td valign="top" class="vncell"><?=gettext("NTP Server");?></td>
		<td class="vtable">
			<input name="timeservers" type="text" class="formfld" id="timeservers" size="40" value="<?=htmlspecialchars($pconfig['timeservers']);?>">
			<br>
			<span class="vexpl"><?=gettext("Enter a server to synchronize with.");?></span>
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
