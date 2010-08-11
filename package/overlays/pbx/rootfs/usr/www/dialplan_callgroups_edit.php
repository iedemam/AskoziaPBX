#!/usr/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2010 tecema (a.k.a IKT) <http://www.tecema.de>.
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

$needs_scriptaculous = false; 

require("guiconfig.inc");

$pgtitle = array(gettext("Dialplan"), gettext("Call Groups"), gettext("Edit"));

if (!is_array($config['dialplan']['callgroup']))
	$config['dialplan']['callgroup'] = array();

callgroups_sort_groups();
$a_callgroups = &$config['dialplan']['callgroup'];


$id = $_GET['id'];
if (isset($_POST['id']))
	$id = $_POST['id'];

/* pull current config into form */
if (isset($id) && $a_callgroups[$id]) {
	$form['name'] = $a_callgroups[$id]['name'];
	$form['extension'] = $a_callgroups[$id]['extension'];
	$form['descr'] = $a_callgroups[$id]['descr'];
	$form['groupmember'] = $a_callgroups[$id]['groupmember'];
	$form['publicaccess'] = $a_callgroups[$id]['publicaccess'];
	$form['publicname'] = $a_callgroups[$id]['publicname'];
	$form['ringlength'] = $a_callgroups[$id]['ringlength'];
}

if ($_POST) {

	unset($input_errors);
	$form = $_POST;
	
	parse_str($_POST['groupmembers']);
	$form['groupmember'] = $gme;

	/* input validation */
	$reqdfields = explode(" ", "name extension");
	$reqdfieldsn = explode(",", "Name,Extension");
	
	verify_input($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

	if ($_POST['publicname'] && ($msg = verify_is_public_name($_POST['publicname']))) {
		$input_errors[] = $msg;
	}

	if (!$input_errors) {
		$gm = array();		
		$gm['name'] = $_POST['name'];
		$gm['extension'] = $_POST['extension'];
		$gm['descr'] = verify_non_default($_POST['descr']);
		$gm['groupmember'] = ($gme) ? $gme : false;
		$gm['publicaccess'] = $_POST['publicaccess'];
		$gm['publicname'] = verify_non_default($_POST['publicname']);
		$gm['ringlength'] = $_POST['ringlength'];

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
	<?=javascript_public_access_editor("functions");?>

	jQuery(document).ready(function(){

		<?=javascript_public_access_editor("ready");?>

	});

//-->
</script>
<?php if ($input_errors) display_input_errors($input_errors); ?>
	<form action="dialplan_callgroups_edit.php" method="post" name="iform" id="iform">
		<table width="100%" border="0" cellpadding="6" cellspacing="0">
			<tr> 
				<td width="20%" valign="top" class="vncellreq"><?=gettext("Name");?></td>
				<td width="80%" colspan="2" class="vtable">
					<input name="name" type="text" class="formfld" id="name" size="40" value="<?=htmlspecialchars($form['name']);?>"> 
					<br><span class="vexpl"><?=gettext("Group name");?></span>
				</td>
			</tr>
			<tr> 
				<td valign="top" class="vncellreq"><?=gettext("Extension");?></td>
				<td colspan="2" class="vtable">
					<input name="extension" type="text" class="formfld" id="extension" size="20" value="<?=htmlspecialchars($form['extension']);?>"> 
					<br><span class="vexpl"><?=gettext("Internal extension used to reach this call group.");?></span>
				</td>
			</tr><?

			display_public_access_editor($form['publicaccess'], $form['publicname'], 2);
			display_description_field($form['descr'], 2);
			// temporary fix until callgroup page rewrite
			if (!isset($form['ringlength'])) {
				$form['ringlength'] = $defaults['accounts']['phones']['ringlength'];
			}
			display_phone_ringlength_selector($form['ringlength'], 2);
			display_callgroup_member_selector($form['groupmember']);

			?><tr>
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

	jQuery(document).ready(function() {
		jQuery('ul.gme').sortable({ connectWith: ['ul.gmd'], revert: true });
		jQuery('ul.gmd').sortable({ connectWith: ['ul.gme'], revert: true });
	});

	function save_groupmember_states() {
		var gms = document.getElementById('groupmembers');
		gms.value = jQuery('ul.gme').sortable('serialize', {key: 'gme[]'});
	}
// ]]>			
</script>
<?php include("fend.inc"); ?>
