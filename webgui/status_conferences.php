#!/usr/local/bin/php
<?php 
/*
	$Id$
	part of m0n0wall (http://m0n0.ch/wall)
	
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
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

$pgtitle = array("Status", "Conferences");
require("guiconfig.inc");


if ($_GET['action'] == "kick") {
	pbx_exec("meetme {$_GET['action']} {$_GET['conf']} {$_GET['member']}");
	touch($d_conferencing_kicked_path);
	header("Location: status_conferences.php?c={$_GET['conf']}&m={$_GET['member']}");
	exit;
}

$rooms = conferencing_get_rooms();
$n = count($rooms);
for($i = 0; $i < $n; $i++) {
	$members = conferencing_get_members($rooms[$i]['number']);
	if (count($members)) {
		$active_rooms[$rooms[$i]['number']] = $members;
	}
}

if (file_exists($d_conferencing_kicked_path)) {
	unset($active_rooms[$_GET['c']][$_GET['m']]);
	if (!count($active_rooms[$_GET['c']])) {
		unset($active_rooms[$_GET['c']]);
	}
	$savemsg = "Member {$_GET['m']} has been kicked from conference {$_GET['c']}.";
	unlink($d_conferencing_kicked_path);
}


?>
<?php include("fbegin.inc"); ?>
<?php if ($savemsg) print_info_box($savemsg); ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">

	<? if (!count($active_rooms)): ?>
		
		<tr> 
			<td><strong>There are currently no active conferences.</strong></td>
		</tr>
		
	<? else: ?>

		<? foreach($active_rooms as $room_no => $members): ?>
		<tr> 
			<td colspan="4" valign="top" class="listtopic">Conference <?=$room_no;?></td>
			<td class="list"></td>
		</tr>
		<tr>
			<td width="15%" class="listhdrr">Member</td>
			<td width="40%" class="listhdrr">Caller ID</td>
			<td width="20%" class="listhdrr">Channel</td>
			<td width="20%" class="listhdr">Time</td>
			<td width="5%" class="list"></td>
		</tr>
			<? foreach($members as $member_no => $member): ?>
			<tr>
				<td class="listlr">
					<?=htmlspecialchars($member_no);?>&nbsp;
				</td>
				<td class="listbg">
					<?=htmlspecialchars($member['callerid']);?>&nbsp;
				</td>
				<td class="listr">
					<?=htmlspecialchars($member['channel']);?>&nbsp;
				</td>
				<td class="listr">
					<?=htmlspecialchars($member['connecttime']);?>&nbsp;
				</td>
				<td valign="middle" nowrap class="list">
					<a href="?action=kick&conf=<?=$room_no;?>&member=<?=$member_no;?>" onclick="return confirm('Do you really want to kick this member?')"><img src="user_delete.png" title="kick conference member" border="0"></a>
				</td>
			</tr>
			<? endforeach; ?>
		<tr> 
			<td colspan="5" class="list" height="12">&nbsp;</td>
		</tr>
		<? endforeach; ?>

	<? endif; ?>

</table>
</form>
<?php include("fend.inc"); ?>
