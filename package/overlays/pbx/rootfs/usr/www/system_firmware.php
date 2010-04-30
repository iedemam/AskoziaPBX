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

$d_isfwfile = 1;

require("guiconfig.inc");

$pgtitle = array(gettext("System"), gettext("Firmware"));
$pghelp = gettext("AskoziaPBX's firmware can be kept up to date here. The system's configuration will be maintained through the upgrade. The system will reboot automatically after installing the new firmware. Firmware images of formal releases are digitally signed before distribution and checked upon installation.");

if ($_POST) {

	unset($input_errors);
	unset($sig_warning);
	
	if ($_POST['Upgrade'] || $_POST['sig_override']) {
		$upgrading = true;
	} else if ($_POST['sig_no']) {
		unlink("/ultmp/firmware.img.gz");
	}

	// we're upgrading the firmware, start sanity checking
	if ($upgrading) {

		// verify that this file was uploaded through HTTP POST and not via another means
		if (is_uploaded_file($_FILES['ulfile']['tmp_name'])) {

			// check to see if the uploaded firmware is compatible with the platform based on its name
			if (!stristr($_FILES['ulfile']['name'], chop(file_get_contents("{$g['etc_path']}/firmwarepattern"))) && 
				!$_POST['sig_override']) {
				$input_errors[] = sprintf(gettext("The uploaded image file is not for this platform (%s)."), $g['platform']);

			// does the file still exist? this usually means we've exhausted the memory
			} else if (!file_exists($_FILES['ulfile']['tmp_name'])) {
				$input_errors[] = gettext("Image upload failed (out of memory?)");
				mwexec("rm -rf /ultmp/*");

			// everything seems OK...continue
			} else {
				// move the image so PHP won't delete it
				rename($_FILES['ulfile']['tmp_name'], "/ultmp/firmware.img.gz");

				// check digital signature
				$sigchk = verify_digital_signature("/ultmp/firmware.img.gz");
				if ($sigchk == 1) {
					$sig_warning = gettext("The digital signature on this image is invalid.");
				} else if ($sigchk == 2) {
					$sig_warning = gettext("This image is not digitally signed.");
				} else if (($sigchk == 3) || ($sigchk == 4)) {
					$sig_warning = gettext("There has been an error verifying the signature on this image.");
				}

				// check the integrity of gzip file
				if (!verify_gzip_file("/ultmp/firmware.img.gz")) {
					$input_errors[] = gettext("The image file is corrupt.");
					unlink("/ultmp/firmware.img.gz");
				}
			}
		}

		if (!$input_errors && (!$sig_warning || $_POST['sig_override'])) {
			// fire up the update script in the background
			exec("busybox nohup /etc/rc.firmware upgrade /ultmp/firmware.img.gz >/dev/null 2>&1 &");
			$keepmsg = gettext("The firmware is now being installed. The PBX will reboot automatically.");
		}
	}
}

include("fbegin.inc");

// firmware upgrades not supported
if (file_exists($g['varrun_path'] . "/firmware.upgrade.unsupported")) {

	?><p><strong><?=gettext("Firmware uploading is not supported on this platform.");?></strong></p><?

// signature warning needs confirmation
} else if ($sig_warning && !$input_errors) {

	?><form action="system_firmware.php" method="post"><?

	$sig_warning = sprintf(gettext("<strong>%s</strong><br>This means that the image you uploaded is not an official/supported image and may lead to unexpected behavior or security compromises. Only install images that come from sources that you trust, and make sure that the image has not been tampered with.<br><br> Do you want to install this image anyway (at your own risk)?"), $sig_warning);
	display_info_box($sig_warning, "keep");

		?><input name="sig_override" type="submit" class="formbtn" id="sig_override" value=" <?=gettext("Yes");?> ">
		<input name="sig_no" type="submit" class="formbtn" id="sig_no" value=" <?=gettext("No");?> ">
	</form><?

// default firmware upgrade screen
} else if (!$keepmsg) {

	?><p><?=sprintf(gettext("Choose the new firmware image file (%s-*.img) to be installed."),
		chop(file_get_contents("{$g['etc_path']}/firmwarepattern")));?><br>
	<?=gettext("Click &quot;Upgrade firmware&quot; to start the upgrade process.");?></p>

	<form action="system_firmware.php" method="post" enctype="multipart/form-data">
		<table width="100%" border="0" cellpadding="6" cellspacing="0">
			<tr> 
				<td width="22%" valign="top">&nbsp;</td>
				<td width="78%"><?

				if (!file_exists($d_sysrebootreqd_path)) {

					?><strong><?=gettext("Firmware image file:");?></strong>
					<input name="ulfile" type="file" class="formfld"><br>
					<br>
					<input name="Upgrade" type="submit" class="formbtn" value="<?=gettext("Upgrade firmware");?>"><?

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

include("fend.inc");
