#!/usr/local/bin/php
<?php 
/*
	$Id$
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

$pgtitle = array("Diagnostics", "Manager Interface");
require("guiconfig.inc");

if ($_POST) {

	 pbx_exec($_POST['command'], &$output);

}

?>

<?php include("fbegin.inc"); ?>
<?php if ($input_errors) print_input_errors($input_errors); ?>
<?php if ($savemsg) print_info_box($savemsg); ?>
<form action="diag_manager.php" method="post">
	<table width="100%" border="0" cellpadding="6" cellspacing="0">
		<tr> 
			<td width="22%" valign="top" class="vncellreq">Command</td>
			<td width="78%" class="vtable"><input name="command" type="text" class="formfld" id="command" size="60" value="<?=htmlspecialchars($_POST['command']);?>"></td>
		</tr>
		<tr> 
			<td width="22%" class="vncellreq" valign="top">Output</td>
			<td width="78%" class="listr">
                <textarea wrap="nowrap" name="contents" cols="80" rows="21" id="contents" class="pre"><?=$output;?></textarea>
			</td>
		</tr>
		<tr>
			<td width="22%" valign="top">&nbsp;</td>
			<td width="78%"> <input name="execute" type="submit" class="formbtn" value="Execute">
 			</td>
		</tr>
	</table>
</form>
<?php include("fend.inc"); ?>
