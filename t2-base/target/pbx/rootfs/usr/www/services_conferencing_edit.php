#!/usr/bin/php
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

$pgtitle = array(gettext("Services"), gettext("Conferencing"), gettext("Edit Room"));

if (!is_array($config['conferencing']['room']))
	$config['conferencing']['room'] = array();

conferencing_sort_rooms();
$a_rooms = &$config['conferencing']['room'];

$id = $_GET['id'];
if (isset($_POST['id']))
	$id = $_POST['id'];

/* pull current config into pconfig */
if (isset($id) && $a_rooms[$id]) {
	$pconfig['number'] = $a_rooms[$id]['number'];
	$pconfig['name'] = $a_rooms[$id]['name'];
	$pconfig['pin'] = $a_rooms[$id]['pin'];
	//$pconfig['adminpin'] = $a_rooms[$id]['adminpin'];
	$pconfig['allowdirectdial'] = isset($a_rooms[$id]['allowdirectdial']);
	$pconfig['publicname'] = $a_rooms[$id]['publicname'];
}


if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;

	/* input validation */
	$reqdfields = explode(" ", "number name");
	$reqdfieldsn = explode(",", "Room Number,Name");
	
	verify_input($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

	if (!verify_is_numericint($_POST['number'])) {
		$input_errors[] = gettext("Conference room numbers must be numeric.");
	}
	if (!isset($id) && in_array($_POST['number'], conferencing_get_extensions())) {
		$input_errors[] = gettext("Conference number already exists.");
	} else if (!isset($id) && in_array($_POST['number'], pbx_get_extensions())) {
		$input_errors[] = gettext("Conference number matches an existing extension.");
	}
	if (($_POST['pin'] && !verify_is_numericint($_POST['pin']))) {
		$input_errors[] = gettext("Conference pins must be numeric.");
	}
	/*if (($_POST['adminpin'] && !verify_is_numericint($_POST['adminpin']))) {
		$input_errors[] = "Conference administration pins must be numeric.";
	}*/
	if ($_POST['publicname'] && ($msg = verify_is_public_name($_POST['publicname']))) {
		$input_errors[] = $msg;
	}

	if (!$input_errors) {
		$room = array();
		$room['number'] = $_POST['number'];
		$room['pin'] = $_POST['pin'];
		//$room['adminpin'] = $_POST['adminpin'];
		$room['name'] = $_POST['name'];
		$room['allowdirectdial'] = $_POST['allowdirectdial'] ? true : false;
		$room['publicname'] = verify_non_default($_POST['publicname']);

		if (isset($id) && $a_rooms[$id]) {
			$room['uniqid'] = $a_rooms[$id]['uniqid'];
			$a_rooms[$id] = $room;
		} else {
			$room['uniqid'] = "CONFERENCE-ROOM-" . uniqid(rand());
			$a_rooms[] = $room;
		}
						
		touch($d_conferencingconfdirty_path);

		write_config();
		
		header("Location: services_conferencing.php");
		exit;
	}
}
?>
<?php include("fbegin.inc"); ?>
<script type="text/JavaScript">
<!--
	<?=javascript_public_direct_dial_editor("functions");?>

	jQuery(document).ready(function(){

		<?=javascript_public_direct_dial_editor("ready");?>

	});

//-->
</script>
<?php if ($input_errors) display_input_errors($input_errors); ?>
	<form action="services_conferencing_edit.php" method="post" name="iform" id="iform">
		<table width="100%" border="0" cellpadding="6" cellspacing="0">		
			<tr> 
				<td width="20%" valign="top" class="vncellreq"><?=gettext("Room Number");?></td>
				<td width="80%" class="vtable">
					<input name="number" type="text" class="formfld" id="number" size="20" value="<?=htmlspecialchars($pconfig['number']);?>"> 
					<br><span class="vexpl"><?=gettext("The conference room's number, also the extension that this conference is reachable at.");?></span>
				</td>
			</tr>
			<tr> 
				<td valign="top" class="vncellreq"><?=gettext("Name");?></td>
				<td class="vtable">
					<input name="name" type="text" class="formfld" id="name" size="40" value="<?=htmlspecialchars($pconfig['name']);?>">
				</td>
			</tr>			
			<tr> 
				<td valign="top" class="vncell"><?=gettext("Access PIN");?></td>
				<td class="vtable">
					<input name="pin" type="text" class="formfld" id="pin" size="40" value="<?=htmlspecialchars($pconfig['pin']);?>"> 
					<br><span class="vexpl"><?=gettext("Optional PIN needed to access this conference room.");?></span>
				</td>
			</tr>
			<? display_public_direct_dial_editor($pconfig['allowdirectdial'], $pconfig['publicname'], 1); ?>
			<tr> 
				<td valign="top">&nbsp;</td>
				<td>
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>" onclick="save_codec_states()">
					<?php if (isset($id) && $a_rooms[$id]): ?>
					<input name="id" type="hidden" value="<?=$id;?>"> 
					<?php endif; ?>
				</td>
			</tr>
		</table>
	</form>
<?php include("fend.inc"); ?>
