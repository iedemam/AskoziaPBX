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

$needs_scriptaculous = true;

require("guiconfig.inc");

$pgtitle = array(gettext("Dialplan"), gettext("Call Groups"), gettext("Edit"));

if (!is_array($config['dialplan']['callgroup']))
	$config['dialplan']['callgroup'] = array();

callgroups_sort_groups();
$a_callgroups = &$config['dialplan']['callgroup'];


$id = $_GET['id'];
if (isset($_POST['id']))
	$id = $_POST['id'];

/* pull current config into pconfig */
if (isset($id) && $a_callgroups[$id]) {
	$pconfig['name'] = $a_callgroups[$id]['name'];
	$pconfig['extension'] = $a_callgroups[$id]['extension'];
	$pconfig['descr'] = $a_callgroups[$id]['descr'];
	$pconfig['groupmember'] = $a_callgroups[$id]['groupmember'];
	$pconfig['allowdirectdial'] = isset($a_callgroups[$id]['allowdirectdial']);
	$pconfig['publicname'] = $a_callgroups[$id]['publicname'];
	$pconfig['ringlength'] = $a_callgroups[$id]['ringlength'];
}

if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;
	
	parse_str($_POST['groupmembers']);
	$pconfig['groupmember'] = $gme;
	
	/* input validation */
	$reqdfields = explode(" ", "name");
	$reqdfieldsn = explode(",", "Name");
	
	verify_input($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

	if ($_POST['publicname'] && ($msg = verify_is_public_name($_POST['publicname']))) {
		$input_errors[] = $msg;
	}

	if (!$input_errors) {
		$gm = array();		
		$gm['name'] = $_POST['name'];
		$gm['extension'] = verify_non_default($_POST['extension']);
		$gm['descr'] = verify_non_default($_POST['descr']);
		$gm['groupmember'] = $gme;
		$gm['allowdirectdial'] = $_POST['allowdirectdial'] ? true : false;
		$gm['publicname'] = verify_non_default($_POST['publicname']);
		$gm['ringlength'] = verify_non_default($_POST['ringlength'], $defaults['accounts']['phones']['ringlength']);

		if (isset($id) && $a_callgroups[$id]) {
			$gm['uniqid'] = $a_callgroups[$id]['uniqid'];
			$a_callgroups[$id] = $gm;
		} else {
			$gm['uniqid'] = "CALLGROUP-PARALLEL-" . uniqid(rand());
			$a_callgroups[] = $gm;
		}
		
		touch($d_extensionsconfdirty_path);
		
		write_config();
		
		header("Location: dialplan_callgroups.php");
		exit;
	}
}

include("fbegin.inc");
?><script type="text/JavaScript">
<!--
	<?=javascript_public_direct_dial_editor("functions");?>

	jQuery(document).ready(function(){

		<?=javascript_public_direct_dial_editor("ready");?>

	});

//-->
</script>
<?php if ($input_errors) display_input_errors($input_errors); ?>
	<form action="dialplan_callgroups_edit.php" method="post" name="iform" id="iform">
		<table width="100%" border="0" cellpadding="6" cellspacing="0">
			<tr> 
				<td width="20%" valign="top" class="vncellreq"><?=gettext("Name");?></td>
				<td width="80%" colspan="2" class="vtable">
					<input name="name" type="text" class="formfld" id="name" size="40" value="<?=htmlspecialchars($pconfig['name']);?>"> 
					<br><span class="vexpl"><?=gettext("Group name");?></span>
				</td>
			</tr>
			<tr> 
				<td valign="top" class="vncell"><?=gettext("Extension");?></td>
				<td colspan="2" class="vtable">
					<input name="extension" type="text" class="formfld" id="extension" size="20" value="<?=htmlspecialchars($pconfig['extension']);?>"> 
					<br><span class="vexpl"><?=gettext("Internal extension used to reach this call group.");?></span>
				</td>
			</tr>
			<? display_public_direct_dial_editor($pconfig['allowdirectdial'], $pconfig['publicname'], 2); ?>
			<? display_description_field($pconfig['descr'], 2); ?>
			<? display_phone_ringlength_selector($pconfig['ringlength'], 2); ?>
			<? display_callgroup_member_selector($pconfig['groupmember']); ?>
			<tr> 
				<td valign="top">&nbsp;</td>
				<td colspan="2">
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>" onclick="save_groupmember_states()">
					<input id="groupmembers" name="groupmembers" type="hidden" value="">
					<?php if (isset($id) && $a_callgroups[$id]): ?>
					<input name="id" type="hidden" value="<?=$id;?>"> 
					<?php endif; ?>
				</td>
			</tr>
		</table>
	</form>
<script type="text/javascript" charset="utf-8">
// <![CDATA[

	Sortable.create("gme",
		{dropOnEmpty:true,containment:["gme","gmd"],constraint:false});
	Sortable.create("gmd",
		{dropOnEmpty:true,containment:["gme","gmd"],constraint:false});

	function save_groupmember_states() {
		var gms = document.getElementById('groupmembers');
		gms.value = Sortable.serialize('gme');
	}
// ]]>			
</script>
<?php include("fend.inc"); ?>
