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

$pgtitle = array(gettext("Diagnostics"), gettext("Editor"));

$mode = "select";

if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;
	
	if (isset($pconfig['open'])) {
		if (!isset($pconfig['file']) || !file_exists($pconfig['file'])) {
			$input_errors[] = sprintf(gettext("The specified file (%s) does not exist."), $pconfig['file']);
		} else {
			$mode = "edit";
		}
		
	} else if (isset($pconfig['save'])) {
		$needsremount = (substr($pconfig['file'], 0, 6) == "/conf/");
		if ($needsremount) {
			conf_mount_rw();
		}
		$f = fopen($pconfig['file'], "w");
		$pconfig['contents'] = str_replace("\r", "", $pconfig['contents']);
		fwrite($f, $pconfig['contents']);
		fclose($f);
		if ($needsremount) {
			conf_mount_ro();
		}
		$savemsg = sprintf(gettext("File contents (%s) saved."), $pconfig['file']);
	}

}

include("fbegin.inc");

if ($input_errors) display_input_errors($input_errors);
if ($savemsg) display_info_box($savemsg);

?><form action="diag_editor.php" method="post">
	<table width="100%" border="0" cellpadding="6" cellspacing="0">
		<?php if ($mode == "edit"): ?>
		<tr> 
			<td width="22%" valign="top" class="vncellreq"><?=gettext("File");?></td>
			<td width="78%" class="vtable"><?=$mandfldhtml;?><input name="file" type="text" class="formfld" id="file" size="60" value="<?=htmlspecialchars($pconfig['file']);?>"></td>
		</tr>
		<tr> 
			<td width="22%" class="vncellreq" valign="top"><?=gettext("Contents");?></td>
			<td width="78%" class="listr">
                <textarea name="contents" cols="80" rows="21" id="contents" class="pre"><?=file_get_contents($pconfig['file']);?></textarea>
			</td>
		</tr>
		<tr>
			<td width="22%" valign="top">&nbsp;</td>
			<td width="78%"> <input name="save" type="submit" class="formbtn" value="<?=gettext("Save");?>">
				<input name="cancel" type="submit" class="formbtn" value="<?=gettext("Cancel");?>"> 
 			</td>
		</tr>
		<?php else: ?>
		<tr> 
			<td width="22%" valign="top" class="vncellreq"><?=gettext("File");?></td>
			<td width="78%" class="vtable"><?=$mandfldhtml;?><input name="file" type="text" class="formfld" id="file" size="60" value="<?=htmlspecialchars($pconfig['file']);?>"></td>
    	</tr>
		<tr>
			<td width="22%" valign="top">&nbsp;</td>
			<td width="78%"> <input name="open" type="submit" class="formbtn" value="<?=gettext("Open");?>"> 
 			</td>
		</tr>
		<?php endif; ?>
	</table>
</form>
<?php include("fend.inc"); ?>