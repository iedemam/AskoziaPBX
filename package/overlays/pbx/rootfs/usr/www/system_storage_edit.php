#!/usr/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)

	Copyright (C) 2010-2011 tecema (a.k.a IKT) <http://www.tecema.de>.
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
	
	THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
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
$pgtitle = array(gettext("System"), gettext("Edit Storage Disk"));

$initialformat = false;
if ($_POST['format']) {
	$ret = storage_format_disk($_POST['device']);
	if ($ret == 0) {
		$savemsg = gettext("Formatting successful!");
		$initialformat = true;
	} else {
		$input_errors[] = gettext("Formatting failed! (returned " . $ret . ")");
	}

} else if ($_POST) {
	$initialformat = true;
	unset($input_errors);

	$disk = storage_verify_disk(&$_POST, &$input_errors);
	if (!$input_errors) {
		storage_save_disk($disk);
		header("Location: system_storage.php");
		exit;
	}
}


$colspan = 1;
$carryovers[] = "uniqid";


$uniqid = $_GET['uniqid'];
if (isset($_POST['uniqid'])) {
	$uniqid = $_POST['uniqid'];
}

if ($_POST['format']) {
	$form = storage_generate_default_disk();
	$form['device'] = $_POST['device'];
} else if ($_POST) {
	$form = $_POST;
} else if ($uniqid) {
	$form = storage_get_disk($uniqid);
	$initialformat = true;
} else {
	$form = storage_generate_default_disk();
}


/* get available devices */
$unassigneddisks = storage_get_unassigned_devices();

/* get assigned services */
$assignedservices = storage_get_assigned_services($form['uniqid']);



/* if we're dealing with a previously used disk, setup its defaults appropriately */
if ($_GET['previous']) {
	$form = storage_generate_default_disk();
	$form['device'] = $_GET['previous'];
	$initialformat = true;

	exec("mkdir -p /mnt/tmpprevdisk");
	openlog("system_storage_edit()", LOG_INFO, LOG_LOCAL0);
	exec("mount -w -t vfat " . $form['device'] . "1 /mnt/tmpprevdisk -o noatime", $out, $retval);

	foreach ($g['storage_services'] as $service) {
		if (file_exists("/mnt/tmpprevdisk/askoziapbx/" . $service)) {
			$form[$service] = true;
		}
	}

	exec("umount /mnt/tmpprevdisk");
	closelog();
	exec("rmdir /mnt/tmpprevdisk");
}


include("fbegin.inc");
d_start("system_storage_edit.php");


if (!$initialformat && !count($unassigneddisks)) {

	?><tr>
		<td><strong><?=gettext("No additional storage devices were detected.");?></strong></td>
	</tr>
</table>
</form><?


} else if (!$initialformat) {
	d_header(gettext("Format & Initialize"));

	?><tr>
		<td valign="top" class="vncell"><?=gettext("Disk Device");?></td>
		<td class="vtable">
			<select name="device" class="formfld" id="device"><?
			foreach ($unassigneddisks as $devicename => $devicedescription) {
				?><option value="<?=$devicename;?>" <? if ($devicename == $form['device']) echo "selected";?>> <?
				echo htmlspecialchars($devicedescription); ?></option><?
			}
			?></select>
			<br><span class="vexpl"><?=gettext("Select which disk you would like to format.");?></span>
		</td>
	</tr>
	<tr>
		<td valign="top">&nbsp;</td>
		<td colspan="<?=$colspan;?>">
			<span class="vexpl"><strong><span class="red"><?=strtoupper(gettext("warning"));?>:</span>
			<?=gettext("All information on this disk will be lost after clicking \"Format\"!");?></strong></span>
			<br>
			<input type="checkbox"><?=gettext("I know, thanks for the warning.");?>&nbsp;&nbsp;<input name="format" id="format" type="submit" class="formbtn" value="<?=gettext("Format");?>...">
		</td>
	</tr>
</table>
</form><?


} else {
	d_header(gettext("General Settings"));
	d_field(gettext("Name"), "name", 40,
		gettext("Enter a descriptive name for this disk."), "required");
	d_field(gettext("Mount Point"), "mountpoint", 20,
		gettext("Enter the full path where this device should be mounted."), "required");
	d_label(gettext("Disk Device"), "device");
	d_spacer();


	d_header(gettext("Storage Services"));
	// media
	if (!$assignedservices['media']) {
		d_checkbox(gettext("Media"), gettext("store voice prompts and music-on-hold on disk"), "media");
	} else {
		d_blanklabel(gettext("Media"),
			sprintf(gettext("service already assigned to \"%s\""), $assignedservices['media']));
	}
	// astdb
	if (!$assignedservices['persistence']) {
		d_checkbox(gettext("Persistence"), gettext("store Asterisk's database on disk"), "persistence");
	} else {
		d_blanklabel(gettext("Persistence"),
			sprintf(gettext("service already assigned to \"%s\""), $assignedservices['persistence']));
	}
	// logs
	if (!$assignedservices['astlogs']) {
		d_checkbox(gettext("Logs"), gettext("store logs on disk"), "astlogs");
	} else {
		d_blanklabel(gettext("Asterisk logs"),
			sprintf(gettext("service already assigned to \"%s\""), $assignedservices['astlogs']));
	}
	// Fax Archive
	if (!$assignedservices['faxarchive']) {
		d_checkbox(gettext("Fax Archive"), gettext("store virtual faxes on disk"), "faxarchive");
	} else {
		d_blanklabel(gettext("Fax Archive"),
			sprintf(gettext("service already assigned to \"%s\""), $assignedservices['faxarchive']));
	}
	// Voicemail Archive
	if (!$assignedservices['voicemailarchive']) {
		d_checkbox(gettext("Voicemail"), gettext("store voicemail on disk"), "voicemailarchive");
	} else {
		d_blanklabel(gettext("Voicemail"),
			sprintf(gettext("service already assigned to \"%s\""), $assignedservices['voicemailarchive']));
	}
	d_spacer();


	d_submit();
}


include("fend.inc");
