#!/usr/local/bin/php
<?php 
/*
	pbx_edit.php
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

$pgtitle = array("PBX", "Editor");
require("guiconfig.inc");

if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;
	
	if (!file_exists($_POST['file'])) {
		$input_errors[] = "The specified file does not exist.";
	}
	
}

?>

<?php include("fbegin.inc"); ?>

<form action="diag_edit.php" method="post">
	<table width="100%" border="0" cellpadding="6" cellspacing="0">
		<tr> 
			<td width="22%" valign="top" class="vncellreq">File</td>
			<td width="78%" class="vtable"><?=$mandfldhtml;?><input name="file" type="text" class="formfld" id="file" size="60" value="<?=htmlspecialchars($pconfig['file']);?>"></td>
        </tr>
		<tr> 
			<td width="22%" class="vncellreq" valign="top">Contents</td>
			<td width="78%" class="listr">
                <textarea name="notes" cols="75" rows="21" id="contents" class="notes"><?=file_get_contents($pconfig['file']);?></textarea>
			</td>
		</tr>
		<tr>
			<td width="22%" valign="top">&nbsp;</td>
			<td width="78%"> <input name="Submit" type="submit" class="formbtn" value="Save"> 
 			</td>
		</tr>
	</table>
</form>
<?php include("fend.inc"); ?>
