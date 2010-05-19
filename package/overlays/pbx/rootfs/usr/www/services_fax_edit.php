#!/usr/bin/php
<?php 
/*
	$Id: services_fax_edit.php 1515 2010-04-30 11:38:34Z michael.iedema $
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2008 tecema (a.k.a IKT) <http://www.tecema.de>.
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

require("guiconfig.inc");

$pgtitle = array(gettext("Services"), gettext("FAX"), gettext("Edit Fax"));

if (!is_array($config['fax']['machine']))
	$config['fax']['machine'] = array();

fax_sort_machines();
$a_faxs = &$config['fax']['machine'];

$id = $_GET['id'];
if (isset($_POST['id']))
	$id = $_POST['id'];

/* pull current config into pconfig */
if (isset($id) && $a_faxs[$id]) {
	$pconfig['number'] = $a_faxs[$id]['number'];
	$pconfig['name'] = $a_faxs[$id]['name'];
	$pconfig['email'] = $a_faxs[$id]['email'];
	$pconfig['publicaccess'] = $a_faxs[$id]['publicaccess'];
	$pconfig['publicname'] = $a_faxs[$id]['publicname'];
}


if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;

	/* input validation */
	$reqdfields = explode(" ", "fax name");
	$reqdfieldsn = explode(",", "Fax Number,Name");
	
	// verify_input($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

	if (!verify_is_numericint($_POST['number'])) {
		$input_errors[] = gettext("Fax numbers must be numeric.");
	}
	if (!isset($id) && in_array($_POST['number'], pbx_get_extensions())) {  //fax_get_extensions
		$input_errors[] = gettext("Fax number already exists.");
	} else if (!isset($id) && in_array($_POST['number'], pbx_get_extensions())) {
		$input_errors[] = gettext("Fax number matches an existing extension.");
	}


	if ($_POST['publicname'] && ($msg = verify_is_public_name($_POST['publicname']))) {
		$input_errors[] = $msg;
	}

	if (!$input_errors) {
		$fax = array();
		$fax['number'] = $_POST['number'];
		$fax['email'] = verify_non_default($_POST['email']);
		$fax['name'] = $_POST['name'];
		$fax['publicaccess'] = $_POST['publicaccess'];
		$fax['publicname'] = verify_non_default($_POST['publicname']);

		if (isset($id) && $a_faxs[$id]) {
			$fax['uniqid'] = $a_faxs[$id]['uniqid'];
			$a_faxs[$id] = $fax;
		} else {
			$fax['uniqid'] = "FAX-MACHINE-" . uniqid(rand());
			$a_faxs[] = $fax;
		}
						
		touch($d_faxconfdirty_path);

		write_config();
		
		header("Location: services_fax.php");
		exit;
	}
}
?>
<?php include("fbegin.inc"); ?>
<script type="text/JavaScript">
<!--
	<?=javascript_public_access_editor("functions");?>

	jQuery(document).ready(function(){

		<?=javascript_public_access_editor("ready");?>

	});

//-->
</script>
<?php if ($input_errors) display_input_errors($input_errors); ?>
	<form action="services_fax_edit.php" method="post" name="iform" id="iform">
		<table width="100%" border="0" cellpadding="6" cellspacing="0">		
			<tr> 
				<td width="20%" valign="top" class="vncellreq"><?=gettext("Fax Number");?></td>
				<td width="80%" class="vtable">
					<input name="number" type="text" class="formfld" id="number" size="20" value="<?=htmlspecialchars($pconfig['number']);?>"> 
					<br><span class="vexpl"><?=gettext("The Fax's number, also the extension that this fax is reachable at.");?></span>
				</td>
			</tr>
			<tr> 
				<td valign="top" class="vncellreq"><?=gettext("Name");?></td>
				<td class="vtable">
					<input name="name" type="text" class="formfld" id="name" size="40" value="<?=htmlspecialchars($pconfig['name']);?>">
				</td>
			</tr>			
			<tr> 
				<td valign="top" class="vncell"><?=gettext("Receipient email");?></td>
				<td class="vtable">
					<input name="email" type="text" class="formfld" id="email" size="40" value="<?=htmlspecialchars($pconfig['email']);?>"> 
				</td>
			</tr>
			<? display_public_access_editor($pconfig['publicaccess'], $pconfig['publicname']); ?>
			<tr> 
				<td valign="top">&nbsp;</td>
				<td>
					<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>">
					<?php if (isset($id) && $a_faxs[$id]): ?>
					<input name="id" type="hidden" value="<?=$id;?>"> 
					<?php endif; ?>
				</td>
			</tr>
		</table>
	</form>
<?php include("fend.inc"); ?>
