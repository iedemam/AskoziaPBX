#!/usr/bin/php
<?php 
/*
	$Id$
	part of m0n0wall (http://m0n0.ch/wall)
	continued modifications as part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
	Copyright (C) 2007-2009 IKT <http://itison-ikt.de>.
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

$d_isfwfile = 1;

openlog("GUIDEBUG", LOG_NDELAY, LOG_LOCAL0);
syslog(LOG_INFO, "Syslog Connection Opened and Working");

require("guiconfig.inc");

$pgtitle = array(gettext("System"), gettext("Firmware"));
$pghelp = gettext("AskoziaPBX's firmware can be kept up to date here. The system's configuration will be maintained through the upgrade. The system will reboot automatically after installing the new firmware. Firmware images of formal releases are digitally signed before distribution and checked upon installation.");

/* checks with downloads.askozia.com to see if a newer firmware version is available;
   returns any HTML message it gets from the server */
function check_firmware_version() {
	global $g;
	$post = "&check=pbxfirmware&platform=" . rawurlencode($g['platform']) . 
		"&version=" . rawurlencode(trim(file_get_contents("/etc/version")));
		
	$rfd = @fsockopen("downloads.askozia.com", 80, $errno, $errstr, 3);
	if ($rfd) {
		$hdr = "POST /index.php HTTP/1.0\r\n";
		$hdr .= "Content-Type: application/x-www-form-urlencoded\r\n";
		$hdr .= "User-Agent: AskoziaPBX-webGUI/1.0\r\n";
		$hdr .= "Host: downloads.askozia.com\r\n";
		$hdr .= "Content-Length: " . strlen($post) . "\r\n\r\n";
		
		fwrite($rfd, $hdr);
		fwrite($rfd, $post);
		
		$inhdr = true;
		$resp = "";
		while (!feof($rfd)) {
			$line = fgets($rfd);
			if ($inhdr) {
				if (trim($line) == "")
					$inhdr = false;
			} else {
				$resp .= $line;
			}
		}
		
		fclose($rfd);
		
		return $resp;
	}
	
	return null;
}

if ($_POST && !file_exists($d_fwlock_path)) {

	unset($input_errors);
	unset($sig_warning);
	
	if ($_POST['Enable']) {
		$mode = "enable";
	} else if ($_POST['Disable']) {
		syslog(LOG_INFO, "Firmware Upgrading Disabled");
		$mode = "disable";
	} else if ($_POST['Upgrade'] || $_POST['sig_override']) {
		$mode = "upgrade";
	} else if ($_POST['sig_no']) {
		unlink("/ultmp/firmware.img.gz");
	}
		
	if ($mode) {
		if ($mode == "enable") {
			syslog(LOG_INFO, "Firmware Enable - pre-exec");
			exec_rc_script("/etc/rc.firmware enable");
			syslog(LOG_INFO, "Firmware Enable - post-exec");
			touch($d_fwupenabled_path);
		} else if ($mode == "disable") {
			syslog(LOG_INFO, "Firmware Disable - pre-exec");
			exec_rc_script("/etc/rc.firmware disable");
			syslog(LOG_INFO, "Firmware Disable - post-exec");
			if (file_exists($d_fwupenabled_path)) {
				unlink($d_fwupenabled_path);
			}
		} else if ($mode == "upgrade") {
			/* XXX : system reboots if no file was uploaded...some checks are failing here */
			if (is_uploaded_file($_FILES['ulfile']['tmp_name'])) {
				/* verify firmware image(s) */
				syslog(LOG_INFO, "Firmware Upgrade - OK - is_uploaded_file()");
				if (!stristr($_FILES['ulfile']['name'], chop(file_get_contents("{$g['etc_path']}/firmwarepattern"))) && !$_POST['sig_override']) {
					syslog(LOG_INFO, "Firmware Upgrade - FAIL - firmware is not for platform");
					$input_errors[] = sprintf(gettext("The uploaded image file is not for this platform (%s)."), $g['platform']);
				} else if (!file_exists($_FILES['ulfile']['tmp_name'])) {
					/* probably out of memory for the MFS */
					syslog(LOG_INFO, "Firmware Upgrade - FAIL - firmware file does not exists");
					$input_errors[] = gettext("Image upload failed (out of memory?)");
					exec_rc_script("/etc/rc.firmware disable");
					if (file_exists($d_fwupenabled_path)) {
						unlink($d_fwupenabled_path);
					}
				} else {
					/* move the image so PHP won't delete it */
					syslog(LOG_INFO, "Firmware Upgrade - OK - moving file");
					rename($_FILES['ulfile']['tmp_name'], "/ultmp/firmware.img.gz");

					/* check digital signature */
					$sigchk = verify_digital_signature("/ultmp/firmware.img.gz");
					syslog(LOG_INFO, "Firmware Upgrade - INFO - verify_digital_signature() returned $sigchk");

					if ($sigchk == 1) {
						$sig_warning = gettext("The digital signature on this image is invalid.");
						syslog(LOG_INFO, "Firmware Upgrade - FAIL - signature on this image is invalid");
					} else if ($sigchk == 2) {
						$sig_warning = gettext("This image is not digitally signed.");
						syslog(LOG_INFO, "Firmware Upgrade - FAIL - image is not digitally signed");
					} else if (($sigchk == 3) || ($sigchk == 4)) {
						$sig_warning = gettext("There has been an error verifying the signature on this image.");
						syslog(LOG_INFO, "Firmware Upgrade - FAIL - error verifying the signature on this image");
					}

					if (!verify_gzip_file("/ultmp/firmware.img.gz")) {
						$input_errors[] = gettext("The image file is corrupt.");
						syslog(LOG_INFO, "Firmware Upgrade - FAIL - image file is corrupt");
						unlink("/ultmp/firmware.img.gz");
					}
				}
			}

			if (!$input_errors && !file_exists($d_fwlock_path) && (!$sig_warning || $_POST['sig_override'])) {			
				/* stop any disk activity Asterisk may be causing */
				syslog(LOG_INFO, "Firmware Upgrade - INFO - executing pbx_stop()");
				pbx_stop();
				/* fire up the update script in the background */
				touch($d_fwlock_path);
				syslog(LOG_INFO, "Firmware Upgrade - INFO - executing /etc/rc.firmware upgrade...");
				exec_rc_script_async("/etc/rc.firmware upgrade /ultmp/firmware.img.gz");
				syslog(LOG_INFO, "Firmware Upgrade - INFO - executing /etc/rc.firmware upgrade COMPLETE");

				$keepmsg = gettext("The firmware is now being installed. The PBX will reboot automatically.");
			}
		}
	}
} else {
	if (!isset($config['system']['disablefirmwarecheck'])) {
		$fwstatus = check_firmware_version();
	}
}

include("fbegin.inc");
if ($fwstatus) echo display_firmware_update_info($fwstatus);

if (file_exists($d_fwupunsupported_path)) {

	?><p><strong><?=gettext("Firmware uploading is not supported on this platform.");?></strong></p><?

} else if ($sig_warning && !$input_errors) {

	?><form action="system_firmware.php" method="post"><?

	$sig_warning = sprintf(gettext("<strong>%s</strong><br>This means that the image you uploaded is not an official/supported image and may lead to unexpected behavior or security compromises. Only install images that come from sources that you trust, and make sure that the image has not been tampered with.<br><br> Do you want to install this image anyway (at your own risk)?"), $sig_warning);
	display_info_box($sig_warning, "keep");

		?><input name="sig_override" type="submit" class="formbtn" id="sig_override" value=" <?=gettext("Yes");?> ">
		<input name="sig_no" type="submit" class="formbtn" id="sig_no" value=" <?=gettext("No");?> ">
	</form><?

} else {

	if (!file_exists($d_fwlock_path)) {

		?><p><?

		if (!file_exists($d_ultmpmounted_path)) {

		?><?=gettext("Click &quot;Enable firmware upload&quot; below.");?> <?

		}

		?><?=sprintf(gettext("Choose the new firmware image file (%s-*.img) to be installed."), chop(file_get_contents("{$g['etc_path']}/firmwarepattern")));?><br>
		<?=gettext("Click &quot;Upgrade firmware&quot; to start the upgrade process.");?></p>
		<form action="system_firmware.php" method="post" enctype="multipart/form-data">
			<table width="100%" border="0" cellpadding="6" cellspacing="0">
				<tr> 
					<td width="22%" valign="top">&nbsp;</td>
					<td width="78%"><?

			if (!file_exists($d_sysrebootreqd_path)) {

				if (!file_exists($d_fwupenabled_path) && !file_exists($d_ultmpmounted_path)) {

					?><input name="Enable" type="submit" class="formbtn" value="<?=gettext("Enable firmware upload");?>"><?

				} else {

					if (!file_exists($d_ultmpmounted_path)) {

					?><input name="Disable" type="submit" class="formbtn" value="<?=gettext("Disable firmware upload");?>"><br>
					<br><?

					}

					?><strong><?=gettext("Firmware image file:");?></strong>&nbsp;<input name="ulfile" type="file" class="formfld"><br>
					<br>
					<input name="Upgrade" type="submit" class="formbtn" value="<?=gettext("Upgrade firmware");?>"><?

				}

			} else {

				?><strong><?=gettext("You must reboot the system before you can upgrade the firmware.");?></strong><?

			}

					?></td>
				</tr>
				<tr> 
					<td width="22%" valign="top">&nbsp;</td>
					<td width="78%">
						<span class="vexpl"><span class="red"><strong><?=gettext("Warning:");?></strong></span><br>
						<?=gettext("DO NOT abort the firmware upgrade once it has started.");?></span>
					</td>
				</tr>
			</table>
		</form><?

	}
}

include("fend.inc");
