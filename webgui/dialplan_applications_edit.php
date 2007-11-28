#!/usr/local/bin/php
<?php 
/*
	$Id: dialplan_callgroups_edit.php 198 2007-09-17 14:11:18Z michael.iedema $
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007 IKT <http://itison-ikt.de>.
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

require_once("functions.inc");

$pgtitle = array("Dialplan", "Applications", "Edit");
require("guiconfig.inc");

// XXX this is_array, sort, reference stuff is all over...
if (!is_array($config['dialplan']['application']))
	$config['dialplan']['application'] = array();

dialplan_sort_applications();
$a_applications = &$config['dialplan']['application'];

$id = $_GET['id'];
if (isset($_POST['id']))
	$id = $_POST['id'];

/* pull current config into pconfig */
if (isset($id) && $a_applications[$id]) {
	$pconfig['name'] = $a_applications[$id]['name'];
	$pconfig['extension'] = $a_applications[$id]['extension'];
}

if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;
	
	/* input validation */
	$reqdfields = explode(" ", "extension");
	$reqdfieldsn = explode(",", "Extension");
	
	do_input_validation($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

	if (($_POST['extension'] && !pbx_is_valid_extension($_POST['extension']))) {
		$input_errors[] = "A valid extension must be entered.";
	}

	if (!$input_errors) {
		$app = array();		
		$app['name'] = $_POST['name'];
		$app['extension'] = $_POST['extension'];	

		if (isset($id) && $a_applications[$id]) {
			$gm['uniqid'] = $a_applications[$id]['uniqid'];
			$a_applications[$id] = $app;
		 } else {
			$app['uniqid'] = "APPLICATION-MAPPING-" . uniqid(rand());
			$a_applications[] = $app;
		}
		
		touch($d_extensionsconfdirty_path);
		
		write_config();
		
		header("Location: dialplan_applications.php");
		exit;
	}
}
?>
<?php include("fbegin.inc"); ?>
<?php if ($input_errors) print_input_errors($input_errors); ?>
	<form action="dialplan_applications_edit.php" method="post" name="iform" id="iform">
		<table width="100%" border="0" cellpadding="6" cellspacing="0">
			<tr> 
				<td width="20%" valign="top" class="vncellreq">Name</td>
				<td width="80%" colspan="2" class="vtable">
					<select name="name" class="formfld" id="name">
					<? foreach ($applications as $appname => $appdesc) : ?>
					<option value="<?=$appname;?>" <?
					if ($appname == $pconfig['name'])
						echo "selected"; ?>
					><?=$appname;?></option>
					<? endforeach; ?>
					</select>
					<br>
					<span class="vexpl">
						<ul>
						<? foreach ($applications as $appname => $appdesc) : ?>
							<li><strong><?=$appname;?></strong>&nbsp;<?=$appdesc;?></li>
						<? endforeach; ?>
						</ul>
					</span>
				</td>
			</tr>
			<tr> 
				<td valign="top" class="vncell">Extension</td>
				<td colspan="2" class="vtable">
					<input name="extension" type="text" class="formfldreq" id="extension" size="40" value="<?=htmlspecialchars($pconfig['extension']);?>"> 
					<br><span class="vexpl">Internal extension used to reach this call group.</span>
				</td>
			</tr>
			<tr> 
				<td valign="top">&nbsp;</td>
				<td colspan="2">
					<input name="Submit" type="submit" class="formbtn" value="Save">
					<?php if (isset($id) && $a_applications[$id]): ?>
					<input name="id" type="hidden" value="<?=$id;?>"> 
					<?php endif; ?>
				</td>
			</tr>
		</table>
	</form>
<?php include("fend.inc"); ?>
