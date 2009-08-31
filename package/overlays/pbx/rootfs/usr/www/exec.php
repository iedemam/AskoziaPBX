#!/usr/bin/php
<?php
/*
	$Id$
	part of m0n0wall (http://m0n0.ch/wall)
	
	Copyright (C) 2003-2006 technologEase (http://www.technologEase.com) and Manuel Kasper <mk@neon1.net>.
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

/* omit no-cache headers because it confuses IE with file downloads */
$omit_nocacheheaders = true;

require("guiconfig.inc");

/* handle download request */
if (($_POST['submit'] == "Download") && file_exists($_POST['dlPath'])) {
	session_cache_limiter('public');
	$fd = fopen($_POST['dlPath'], "rb");
	header("Content-Type: application/octet-stream");
	header("Content-Length: " . filesize($_POST['dlPath']));
	header("Content-Disposition: attachment; filename=\"" . 
		trim(htmlentities(basename($_POST['dlPath']))) . "\"");
	
	fpassthru($fd);
	exit;

/* handle upload request */
} else if (($_POST['submit'] == "Upload") && is_uploaded_file($_FILES['ulfile']['tmp_name'])) {
	move_uploaded_file($_FILES['ulfile']['tmp_name'], "/ultmp/" . $_FILES['ulfile']['name']);
	$ulmsg = "Uploaded file to /ultmp/" . htmlentities($_FILES['ulfile']['name']);
	unset($_POST['txtCommand']);
}
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
<head>
<?php

/*
	Exec+ v1.02-000 - Copyright 2001-2003, All rights reserved
	Created by technologEase (http://www.technologEase.com).
	
	(modified for m0n0wall by Manuel Kasper <mk@neon1.net>)
*/

// Function: is Blank
// Returns true or false depending on blankness of argument.
function isBlank( $arg ) { return ereg( "^\s*$", $arg ); }


// Function: Puts
// Put string, Ruby-style.
function puts( $arg ) { echo "$arg\n"; }



?>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
<title><?=gettext("AskoziaPBX: execute command");?></title>
<script src="jquery.js" type="text/javascript"></script>
<script src="jquery.blockUI.js" type="text/javascript"></script>
<script src="jquery.preloadImage.js" type="text/javascript"></script>
<script language="javascript">
<!--

	jQuery(document).ready(function(){
	
		jQuery.preloadImages(['ajax_busy_round.gif']);

		jQuery("#contents_wrapper").ajaxStart(function(){
			jQuery(this).text("executing...");
		});

		jQuery("#txtCommand").keyup(function(e){
			if (e.keyCode == 13) {
				execute();
			}
		});

		jQuery("#Execute").click(function(){
			execute();
		});

	});

	function execute() {

		var command = jQuery("#txtCommand").val();

		if (!isBlank(command)) {
			// If this command is not a repeat of last command, then not store command.
			if (command != arrRecallBuffer[arrRecallBuffer.length-1]) {
				arrRecallBuffer[arrRecallBuffer.length] = command;
			}
		}
		jQuery.get("cgi-bin/ajax.cgi", { exec_shell: command }, function(data){
			jQuery("#contents_wrapper").text(data);
		});
	}

	// Create recall buffer array (of encoded strings).
	var arrRecallBuffer = new Array;

	// Set pointer to end of recall buffer.
	var intRecallPtr = arrRecallBuffer.length;

	// Function: is Blank
	// Returns boolean true or false if argument is blank.
	function isBlank( strArg ) { return strArg.match( /^\s*$/ ) }

	// Recalls command buffer going either up or down.
	function btnRecall_onClick(n) {

		// If nothing in recall buffer, then error.
		if (!arrRecallBuffer.length) {
			alert( 'Nothing to recall!' );
			jQuery("#txtCommand").focus();
			return;
		}

		// Increment recall buffer pointer in positive or negative direction
		// according to <n>.
		intRecallPtr += n;

		// Make sure the buffer stays circular.
		if (intRecallPtr < 0) { intRecallPtr = arrRecallBuffer.length - 1 }
		if (intRecallPtr > (arrRecallBuffer.length - 1)) { intRecallPtr = 0 }

		// Recall the command.
		jQuery("#txtCommand").val(arrRecallBuffer[intRecallPtr]);
	}

	// Resets form on reset button click event.
	function Reset_onClick() {

		// Reset recall buffer pointer.
		intRecallPtr = arrRecallBuffer.length;

		// Clear form (could have spaces in it) and return focus ready for cmd.
		jQuery("#txtCommand").val('');
		jQuery("#txtCommand").focus();

		return true;
	}

	// hansmi, 2005-01-13
	function txtCommand_onKey(e) {
		if(!e) var e = window.event; // IE-Fix
		var code = (e.keyCode?e.keyCode:(e.which?e.which:0));
		if(!code) return;
		var f = document.getElementsByName('frmExecPlus')[0];
		if(!f) return;
		switch(code) {
		case 38: // up
			btnRecall_onClick(f, -1);
			break;
		case 40: // down
			btnRecall_onClick(f, 1);
			break;
		}
	}
//-->
</script>
<link href="gui.css" rel="stylesheet" type="text/css">
<style>
<!--

input {
	font-family: courier new, courier;
	font-weight: normal;
	font-size: 9pt;
}

pre {
	border: 2px solid #435370;
	background: #F0F0F0;
	padding: 1em;
	font-family: courier new, courier;
	white-space: pre;
	line-height: 10pt;
	font-size: 10pt;
}

.label {
	font-family: tahoma, verdana, arial, helvetica;
	font-size: 11px;
	font-weight: bold;
}

.button {
	font-family: tahoma, verdana, arial, helvetica;
	font-weight: bold;
	font-size: 11px;
}

-->
</style>
</head>
<body>
<p><span class="pgtitle"><?=gettext("AskoziaPBX: execute command");?></span>
<?php if ($ulmsg) echo "<p><strong>" . $ulmsg . "</strong></p>\n"; ?>
<pre id="contents_wrapper"><?=gettext("enter a command below...");?></pre>
	<table>
		<tr>
			<td class="label" align="right"><?=gettext("Command:");?></td>
			<td class="type"><input name="txtCommand" id="txtCommand" type="text" size="80" value=""></td>
		</tr>
		<tr>
			<td valign="top">&nbsp;</td>
			<td valign="top" class="label">
				<input type="hidden" name="txtRecallBuffer" value="">
				<input type="button" class="button" name="btnRecallPrev" value="<" onClick="btnRecall_onClick(-1);">
				<input type="button" class="button" name="Execute" id="Execute" value="<?=gettext("Execute");?>">
				<input type="button" class="button" name="btnRecallNext" value=">" onClick="btnRecall_onClick(1);">
				<input type="button"  class="button" value="<?=gettext("Clear");?>" onClick="return Reset_onClick();">
			</td>
		</tr>
	</table>
<form action="exec.php" method="post" enctype="multipart/form-data" name="frmExecPlus">
	<table>
		<tr>
			<td align="right"><?=gettext("Download:");?></td>
			<td>
				<input name="dlPath" type="text" id="dlPath" size="50">
				<input name="submit" type="submit"  class="button" id="download" value="<?=gettext("Download");?>">
			</td>
		</tr>
		<tr>
			<td align="right"><?=gettext("Upload:");?></td>
			<td valign="top" class="label">
				<input name="ulfile" type="file" class="button" id="ulfile">
				<input name="submit" type="submit"  class="button" id="upload" value="<?=gettext("Upload");?>">
			</td>
		</tr>
	</table>
</form>
</body>
</html>
