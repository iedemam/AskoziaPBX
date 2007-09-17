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

$pgtitle = array("Debug", "ISDN");
require("guiconfig.inc");

$state = "stopped";

if ($_POST) {

	$pconfig = $_POST;
	
	if (isset($pconfig['start'])) {
		exec("/sbin/isdndecode -u {$pconfig['interface']} -i -o -x > /tmp/isdndecode.txt &");
		$state = "running";
		
	} else if (isset($pconfig['stop'])) {
		killbyname("isdndecode");
		$file_contents = file_get_contents("/tmp/isdndecode.txt");
		unlink("/tmp/isdndecode.txt");
		$state = "finished";
	}

}

?>

<?php include("fbegin.inc"); ?>
<form action="debug_isdn.php" method="post">
	<table width="100%" border="0" cellpadding="6" cellspacing="0">
		<?php if ($state == "stopped"): ?>
		<tr> 
			<td width="20%" valign="top" class="vncell">ISDN Interface</td>
			<td width="80%" class="vtable">
				<select name="interface" class="formfld" id="interface">
				<? $isdn_interfaces = isdn_get_interfaces(); ?>
				<? foreach ($isdn_interfaces as $interface) : ?>
				<option value="<?=$interface['unit'];?>"><?=$interface['name'];?></option>
				<? endforeach; ?>
				</select>
			</td>
		</tr>
		<tr>
			<td valign="top">&nbsp;</td>
			<td>
				<input name="start" type="submit" class="formbtn" value="Start">
 			</td>
		</tr>
		
		<?php elseif ($state == "running"): ?>
		<tr> 
			<td width="20%" valign="top" class="vncell">Debugging</td>
			<td width="80%" class="vtable">...</td>
		</tr>
		<tr>
			<td width="20%" valign="top">&nbsp;</td>
			<td width="80%">
				<input name="stop" type="submit" class="formbtn" value="Stop">
 			</td>
		</tr>
		
		<?php elseif ($state == "finished"): ?>
		<tr> 
			<td width="20%" valign="top" class="vncell">Debug Ouptut</td>
			<td width="80%" class="vtable">
				<textarea name="contents" cols="80" rows="40" id="contents" class="pre"><?=$file_contents?></textarea>
			</td>
		</tr>
		<tr>
			<td valign="top">&nbsp;</td>
			<td>
				<input name="restart" type="submit" class="formbtn" value="Restart">
 			</td>
		</tr>
		<?php endif; ?>
	</table>
</form>
<?php include("fend.inc"); ?>
