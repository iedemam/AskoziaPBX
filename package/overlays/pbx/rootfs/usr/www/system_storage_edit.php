#!/usr/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)

	Copyright (C) 2010 IKT <http://itison-ikt.de>.
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.

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
		$_POST['uniqid'] = "STORAGE-DISK-" . uniqid(rand());
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
$carryovers[] = "unmounted";


$uniqid = $_GET['uniqid'];
if (isset($_POST['uniqid'])) {
	$uniqid = $_POST['uniqid'];
}

if ($_POST) {
	$form = $_POST;
} else if ($uniqid) {
	$form = storage_get_disk($uniqid);
	$initialformat = true;
} else {
	$form = storage_generate_default_disk();
}


include("fbegin.inc");
d_start("system_storage_edit.php");


if (!$initialformat) {
	d_header(gettext("Format & Initialize"));
	d_field(gettext("Disk Device"), "device", 20,
		gettext("Enter the full path of the device to be used. i.e. <strong>/dev/sdb</strong>. Find available devices by executing <strong>fdisk -l</strong> in the Integrator Panel."), "required");
	?><tr>
		<td valign="top">&nbsp;</td>
		<td colspan="<?=$colspan;?>">
			<span class="vexpl"><strong><span class="red"><?=strtoupper(gettext("warning"));?>:</span>
			<?=gettext("All information on this disk will be lost after clicking \"Format\"!!!");?></strong></span>
			<br>
			<br>
			<input name="format" id="format" type="submit" class="formbtn" value="<?=gettext("Format");?>...">
		</td>
	</tr>
</table>
</form><?


} else {
	d_header(gettext("General Settings"));
	d_field(gettext("Name"), "name", 40,
		gettext("Enter a descriptive name for this disk."), "required");
	d_field(gettext("Mount Point"), "mountpoint", 20,
		gettext("Enter the full path where this device should be mounted. i.e. <em>/storage/usbstick1</em>."), "required");
	d_label(gettext("Disk Device"), "device");
	d_spacer();


	d_submit();
}


include("fend.inc");
