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

$pgtitle = array(gettext("Advanced"), gettext("GUI Options"));

$pconfig['cert'] = base64_decode($config['system']['webgui']['certificate']);
$pconfig['key'] = base64_decode($config['system']['webgui']['private-key']);
$pconfig['disableconsolemenu'] = isset($config['system']['disableconsolemenu']);
$pconfig['disablefirmwarecheck'] = isset($config['system']['disablefirmwarecheck']);
$pconfig['disablepackagechecks'] = isset($config['system']['disablepackagechecks']);
$pconfig['expanddiags'] = isset($config['system']['webgui']['expanddiags']);
$pconfig['expandadvanced'] = isset($config['system']['webgui']['expandadvanced']);

$pconfig['hidesip'] = isset($config['system']['webgui']['hidesip']);
$pconfig['hideiax'] = isset($config['system']['webgui']['hideiax']);
$pconfig['hideisdn'] = isset($config['system']['webgui']['hideisdn']);
$pconfig['hideanalog'] = isset($config['system']['webgui']['hideanalog']);

if ($g['platform'] == "Generic") {
	$pconfig['harddiskstandby'] = $config['system']['harddiskstandby'];
}
$pconfig['noantilockout'] = isset($config['system']['webgui']['noantilockout']);
$pconfig['polling_enable'] = isset($config['system']['polling']);

if ($_POST) {

	unset($input_errors);
	$savemsgadd = "";

	if ($_POST['gencert']) {
		/* custom certificate generation requested */
		$ck = generate_self_signed_cert();
		
		if ($ck === false) {
			$input_errors[] = gettext("A self-signed certificate could not be generated because the system's clock is not set.");
		} else {
			$_POST['cert'] = $ck['cert'];
		 	$_POST['key'] = $ck['key'];
			$savemsgadd = "<br><br>" . gettext("A self-signed certificate and private key have been automatically generated.");
		}
	}

	$pconfig = $_POST;

	/* input validation */
	if (($_POST['cert'] && !$_POST['key']) || ($_POST['key'] && !$_POST['cert'])) {
		$input_errors[] = gettext("Certificate and key must always be specified together.");
	} else if ($_POST['cert'] && $_POST['key']) {
		if (!strstr($_POST['cert'], "BEGIN CERTIFICATE") || !strstr($_POST['cert'], "END CERTIFICATE"))
			$input_errors[] = gettext("This certificate does not appear to be valid.");
		if (!strstr($_POST['key'], "BEGIN RSA PRIVATE KEY") || !strstr($_POST['key'], "END RSA PRIVATE KEY"))
			$input_errors[] = gettext("This key does not appear to be valid.");
	}

	if (!$input_errors) {
		$oldcert = $config['system']['webgui']['certificate'];
		$oldkey = $config['system']['webgui']['private-key'];
		$config['system']['webgui']['certificate'] = base64_encode($_POST['cert']);
		$config['system']['webgui']['private-key'] = base64_encode($_POST['key']);
		$config['system']['disableconsolemenu'] = $_POST['disableconsolemenu'] ? true : false;
		$config['system']['disablefirmwarecheck'] = $_POST['disablefirmwarecheck'] ? true : false;
		$config['system']['disablepackagechecks'] = $_POST['disablepackagechecks'] ? true : false;
		$config['system']['webgui']['expanddiags'] = $_POST['expanddiags'] ? true : false;
		$config['system']['webgui']['expandadvanced'] = $_POST['expandadvanced'] ? true : false;
		
		$config['system']['webgui']['hidesip'] = $_POST['hidesip'] ? true : false;
		$config['system']['webgui']['hideiax'] = $_POST['hideiax'] ? true : false;
		$config['system']['webgui']['hideisdn'] = $_POST['hideisdn'] ? true : false;
		$config['system']['webgui']['hideanalog'] = $_POST['hideanalog'] ? true : false;
		
		if ($g['platform'] == "Generic") {
			$oldharddiskstandby = $config['system']['harddiskstandby'];
			$config['system']['harddiskstandby'] = $_POST['harddiskstandby'];
		}
		$config['system']['webgui']['noantilockout'] = $_POST['noantilockout'] ? true : false;
		$oldpolling = $config['system']['polling'];
		$config['system']['polling'] = $_POST['polling_enable'] ? true : false;

		write_config();
		
		if (($config['system']['webgui']['certificate'] != $oldcert)
				|| ($config['system']['webgui']['private-key'] != $oldkey)
				|| ($config['system']['polling'] != $oldpolling)) {
			touch($d_sysrebootreqd_path);
		}
		if (($g['platform'] == "Generic") && ($config['system']['harddiskstandby'] != $oldharddiskstandby)) {
			if (!$config['system']['harddiskstandby']) {
				// Reboot needed to deactivate standby due to a stupid ATA-protocol
				touch($d_sysrebootreqd_path);
				unset($config['system']['harddiskstandby']);
			} else {
				// No need to set the standby-time if a reboot is needed anyway
				system_set_harddisk_standby();
			}
		}
		
		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			//$retval |= system_set_termcap();
			config_unlock();
		}
		$savemsg = get_std_save_message($retval) . $savemsgadd;
	}
}
?>
<?php include("fbegin.inc"); ?>

            <?php if ($input_errors) display_input_errors($input_errors); ?>
            <?php if ($savemsg) display_info_box($savemsg); ?>
            <form action="advanced_guioptions.php" method="post" name="iform" id="iform">
              <table width="100%" border="0" cellpadding="6" cellspacing="0">
                <tr> 
                  <td colspan="2" valign="top" class="listtopic"><?=gettext("webGUI SSL certificate/key");?></td>
                </tr>
                <tr> 
                  <td width="22%" valign="top" class="vncell"><?=gettext("Certificate");?></td>
                  <td width="78%" class="vtable"> 
                    <textarea name="cert" cols="65" rows="7" id="cert" class="formpre"><?=htmlspecialchars($pconfig['cert']);?></textarea>
                    <br> 
                    <?=gettext("Paste a signed certificate in X.509 PEM format here.");?></td>
                </tr>
                <tr> 
                  <td width="22%" valign="top" class="vncell"><?=gettext("Key");?></td>
                  <td width="78%" class="vtable"> 
                    <textarea name="key" cols="65" rows="7" id="key" class="formpre"><?=htmlspecialchars($pconfig['key']);?></textarea>
                    <br> 
                    <?=gettext("Paste an RSA private key in PEM format here.");?></td>
                </tr>
                <tr> 
                  <td width="22%" valign="top">&nbsp;</td>
                  <td width="78%"> 
                    <input name="gencert" type="submit" class="formbtn" value="<?=gettext("Generate self-signed certificate");?>"> 
                    <input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>"> 
                  </td>
                </tr>
                <tr> 
                  <td colspan="2" class="list" height="12"></td>
                </tr>
                <tr> 
                  <td colspan="2" valign="top" class="listtopic"><?=gettext("Miscellaneous");?></td>
                </tr>
				<tr> 
                  <td width="22%" valign="top" class="vncell"><?=gettext("Console menu");?></td>
                  <td width="78%" class="vtable"> 
                    <input name="disableconsolemenu" type="checkbox" id="disableconsolemenu" value="yes" <?php if ($pconfig['disableconsolemenu']) echo "checked"; ?>>
                    <strong><?=gettext("Disable console menu");?></strong><span class="vexpl"><br>
                    <?=gettext("Changes to this option will take effect after a reboot.");?></span></td>
                </tr>
				<tr>
                  <td valign="top" class="vncell"><?=gettext("Firmware version check");?></td>
                  <td class="vtable">
                    <input name="disablefirmwarecheck" type="checkbox" id="disablefirmwarecheck" value="yes" <?php if ($pconfig['disablefirmwarecheck']) echo "checked"; ?>>
                    <strong><?=gettext("Disable firmware version check");?></strong><span class="vexpl"><br>
    <?=gettext("This will cause the system not to check for newer firmware versions when the <a href=\"system_firmware.php\">System: Firmware</a> page is viewed.");?></span></td>
			    </tr>
				<tr>
	              <td valign="top" class="vncell"><?=gettext("Package version checks");?></td>
	              <td class="vtable">
	                <input name="disablepackagechecks" type="checkbox" id="disablepackagechecks" value="yes" <?php if ($pconfig['disablepackagechecks']) echo "checked"; ?>>
	                <strong><?=gettext("Disable package version checks");?></strong><span class="vexpl"><br>
	    <?=gettext("This will cause the system not to check for package updates when the <a href=\"system_packages.php\">System: Packages</a> page is viewed.");?></span></td>
				</tr>
<?php if ($g['platform'] == "Generic"): ?>
				<tr> 
                  <td width="22%" valign="top" class="vncell"><?=gettext("Hard disk standby time");?></td>
                  <td width="78%" class="vtable"> 
                    <select name="harddiskstandby" class="formfld">
					<?php $sbvals = array(1,2,3,4,5,10,15,20,30,60); ?>
                      <option value="" <?php if(!$pconfig['harddiskstandby']) echo('selected');?>><?=gettext("Always on");?></option>
					<?php foreach ($sbvals as $sbval): ?>
                      <option value="<?=$sbval;?>" <?php if($pconfig['harddiskstandby'] == $sbval) echo 'selected';?>><?=$sbval;?>&nbsp;<?=gettext("minutes");?></option>
					<?php endforeach; ?>
                    </select>
                    <br>
                    <?=gettext("Puts the hard disk into standby mode when the selected amount of time after the last access has elapsed. <em>Do not set this for CF cards.");?></em></td>
				</tr>
<?php endif; ?>
				<tr> 
                  <td width="22%" valign="top" class="vncell"><?=gettext("Navigation");?></td>
                  <td width="78%" class="vtable"> 
                    <input name="expanddiags" type="checkbox" id="expanddiags" value="yes" <?php if ($pconfig['expanddiags']) echo "checked"; ?>>
                    <strong><?=gettext("Keep diagnostics in navigation expanded");?></strong>
					<br>
					<input name="expandadvanced" type="checkbox" id="expandadvanced" value="yes" <?php if ($pconfig['expandadvanced']) echo "checked"; ?>>
					<strong><?=gettext("Keep advanced options in navigation expanded");?></strong>
					<br>
					<input name="hidesip" type="checkbox" id="hidesip" value="yes" <?php if ($pconfig['hidesip']) echo "checked"; ?>>
					<strong><?=gettext("Hide SIP related options");?></strong>
					<br>
					<input name="hideiax" type="checkbox" id="hideiax" value="yes" <?php if ($pconfig['hideiax']) echo "checked"; ?>>
					<strong><?=gettext("Hide IAX related options");?></strong>
					<br>
					<input name="hideisdn" type="checkbox" id="hideisdn" value="yes" <?php if ($pconfig['hideisdn']) echo "checked"; ?>>
					<strong><?=gettext("Hide ISDN related options");?></strong>
					<br>
					<input name="hideanalog" type="checkbox" id="hideanalog" value="yes" <?php if ($pconfig['hideanalog']) echo "checked"; ?>>
					<strong><?=gettext("Hide Analog related options");?></strong></td>
                </tr>
                <tr> 
                  <td width="22%" valign="top">&nbsp;</td>
                  <td width="78%"> 
                    <input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>">
                  </td>
                </tr>
              </table>
</form>
<script language="JavaScript">
<!--
enable_change(false);
//-->
</script>
<?php include("fend.inc"); ?>
