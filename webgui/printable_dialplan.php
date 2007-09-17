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
require("guiconfig.inc");

$a_sipphones = sip_get_phones();
$a_iaxphones = iax_get_phones();
$a_isdnphones = isdn_get_phones();
$a_analogphones = analog_get_phones();
$a_extphones = external_get_phones();
$a_callgroups = dialplan_get_callgroups();
$a_rooms = conferencing_get_rooms();

$all_extensions = printable_sort_extensions(
	array_merge(
		$a_sipphones,
		$a_iaxphones,
		$a_isdnphones,
		$a_analogphones,
		$a_extphones,
		$a_callgroups,
		$a_rooms
	));

function printable_sort_extensions($extensions) {
	usort($extensions, "_a_sortexts");
	return $extensions;
}
function _a_sortexts($a, $b) {
	$ext1 = isset($a['extension']) ? $a['extension'] : $a['number'];
	$ext2 = isset($b['extension']) ? $b['extension'] : $b['number'];

    if ($ext1 == $ext2) {
        return 0;
    }
    return ($ext1 < $ext2) ? -1 : 1;
}

?>
<html>
	<head>
		<title>Printable Dialplan</title>
		<link href="gui.css" rel="stylesheet" type="text/css">
	</head>
	<body>
		<table width="500" border="0" cellpadding="0" cellspacing="0">
			<tr>
				<td colspan="2" align="right"><img src="/logo.gif" width="150" height="47" border="0"></td>
			</tr>
			<tr>
				<td width="100" class="listhdr">Extension</td>
				<td width="400" class="listhdr">Caller ID / Name</td>
			</tr>
		
			<?php $i = 0; foreach ($all_extensions as $ext): ?>
			<tr>
				<td class="listlr"><?=htmlspecialchars(isset($ext['extension']) ? $ext['extension'] : $ext['number']);?></td>
				<td class="listr"><?=htmlspecialchars(isset($ext['callerid']) ? $ext['callerid'] : $ext['name']);?>&nbsp;</td>
			</tr>
			<?php $i++; endforeach; ?>
		</table>
		<br>&nbsp;<i>generated on: <?=htmlspecialchars(date("D M j G:i T Y"));?></i>
	</body>
</html>
