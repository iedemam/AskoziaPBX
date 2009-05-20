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

/* XXX : sorting is handled in so many different ways...really time to normalize this */
$a_intphones = pbx_get_phones();			usort($a_intphones, "pbx_sort_by_extension");
$a_extphones = external_get_phones();		// already sorted by extension
$a_callgroups = callgroups_get_groups();	usort($a_callgroups, "pbx_sort_by_extension");
$a_rooms = conferencing_get_rooms();		usort($a_rooms, "pbx_sort_by_extension");
$a_apps = applications_get_apps();			// already sorted by extension
$a_providers = pbx_get_providers();			// already sorted by name

?>
<html>
	<head>
		<title><?=gettext("Printable Dialplan");?></title>
		<link href="gui.css" rel="stylesheet" type="text/css">
	</head>
	<body>
		<table width="100%" border="0" cellpadding="0" cellspacing="0">
			<tr>
				<td colspan="3" align="right"><img src="logo.png" width="150" height="47" border="0"></td>
			</tr>


			<?php if (count($a_intphones) > 0): ?>
			<tr>
				<td colspan="3" class="listtopiclight"><?=gettext("Internal Phones");?></td>
			</tr>
			<tr>
				<td width="15%" class="listhdr"><?=gettext("Extension");?></td>
				<td width="30%" class="listhdr"><?=gettext("Caller ID");?></td>
				<td width="55%" class="listhdr"><?=gettext("Description");?></td>
			</tr>		
			<?php foreach ($a_intphones as $p):
				if (isset($p['disabled'])) {
					continue;
				}
			?><tr>
				<td class="listlr"><?=htmlspecialchars($p['extension']);?>&nbsp;</td>
				<td class="listr"><?=htmlspecialchars($p['callerid']);?>&nbsp;</td>
				<td class="listr"><?=htmlspecialchars($p['descr']);?>&nbsp;</td>
			</tr>
			<?php endforeach; ?>
			<tr> 
				<td colspan="3" class="list" height="12">&nbsp;</td>
			</tr>
			<?php endif; ?>


			<?php if (count($a_extphones) > 0): ?>
			<tr>
				<td colspan="3" class="listtopiclight"><?=gettext("External Phones");?></td>
			</tr>
			<tr>
				<td width="15%" class="listhdr"><?=gettext("Extension");?></td>
				<td width="30%" class="listhdr"><?=gettext("Name");?></td>
				<td width="55%" class="listhdr"><?=gettext("Description");?></td>
			</tr>
			<?php foreach ($a_extphones as $p):
				if (isset($p['disabled'])) {
					continue;
				}
			?><tr>
				<td class="listlr"><?=htmlspecialchars($p['extension']);?>&nbsp;</td>
				<td class="listr"><?=htmlspecialchars($p['name']);?>&nbsp;</td>
				<td class="listr"><?=htmlspecialchars($p['descr']);?>&nbsp;</td>
			</tr>
			<?php endforeach; ?>
			<tr> 
				<td colspan="3" class="list" height="12">&nbsp;</td>
			</tr>
			<?php endif; ?>


			<?php if (count($a_callgroups) > 0): ?>
			<tr>
				<td colspan="3" class="listtopiclight"><?=gettext("Call Groups");?></td>
			</tr>
			<tr>
				<td width="15%" class="listhdr"><?=gettext("Extension");?></td>
				<td width="30%" class="listhdr"><?=gettext("Name");?></td>
				<td width="55%" class="listhdr"><?=gettext("Members");?></td>
			</tr>
			<?php foreach ($a_callgroups as $cg): ?>
			<tr>
				<td class="listlr"><?=htmlspecialchars($cg['extension']);?>&nbsp;</td>
				<td class="listr"><?=htmlspecialchars($cg['name']);?>&nbsp;</td>
				<td class="listr"><?
					$n = count($cg['groupmember']);
					echo htmlspecialchars(pbx_uniqid_to_name($cg['groupmember'][0]));
					for($i = 1; $i < $n; $i++) {
						echo ", " . htmlspecialchars(pbx_uniqid_to_name($cg['groupmember'][$i]));
					}
					?>&nbsp;</td>
			</tr>
			<?php endforeach; ?>
			<tr> 
				<td colspan="3" class="list" height="12">&nbsp;</td>
			</tr>
			<?php endif; ?>


			<?php if (count($a_rooms) > 0): ?>
			<tr>
				<td colspan="3" class="listtopiclight"><?=gettext("Conference Rooms");?></td>
			</tr>
			<tr>
				<td width="15%" class="listhdr"><?=gettext("Extension");?></td>
				<td width="30%" class="listhdr"><?=gettext("Name");?></td>
				<td width="55%" class="listhdr"><?=gettext("Pin");?></td>
			</tr>
			<?php foreach ($a_rooms as $r): ?>
			<tr>
				<td class="listlr"><?=htmlspecialchars($r['number']);?>&nbsp;</td>
				<td class="listr"><?=htmlspecialchars($r['name']);?>&nbsp;</td>
				<td class="listr">
					<?php if ($r['pin']): ?>
					<img src="lock.gif" width="7" height="9" border="0">
					<?php endif; ?>
					&nbsp;</td>
			</tr>
			<?php endforeach; ?>
			<tr> 
				<td colspan="3" class="list" height="12">&nbsp;</td>
			</tr>
			<?php endif; ?>


			<?php if (count($a_apps) > 0): ?>
			<tr>
				<td colspan="3" class="listtopiclight"><?=gettext("Custom Applications");?></td>
			</tr>
			<tr>
				<td width="15%" class="listhdr"><?=gettext("Extension");?></td>
				<td width="30%" class="listhdr"><?=gettext("Name");?></td>
				<td width="55%" class="listhdr"><?=gettext("Description");?></td>
			</tr>
			<?php foreach ($a_apps as $app): ?>
			<tr>
				<td class="listlr"><?=htmlspecialchars($app['extension']);?>&nbsp;</td>
				<td class="listr"><?=htmlspecialchars($app['name']);?>&nbsp;</td>
				<td class="listr"><?=htmlspecialchars($app['descr']);?>&nbsp;</td>
			</tr>
			<?php endforeach; ?>
			<tr> 
				<td colspan="3" class="list" height="12">&nbsp;</td>
			</tr>
			<?php endif; ?>

			<?php if (count($a_providers) > 0): ?>
			<tr>
				<td colspan="3" class="listtopiclight"><?=gettext("Providers");?></td>
			</tr>
			<tr>
				<td width="15%" class="listhdr"><?=gettext("Name");?></td>
				<td width="30%" class="listhdr"><?=gettext("Dialing Pattern(s)");?></td>
				<td width="55%" class="list">&nbsp;</td>
			</tr>
			<?php foreach ($a_providers as $provider): ?>
			<?php if (!is_array($provider['dialpattern'])) continue; ?>
			<tr>
				<td class="listlr"><?=htmlspecialchars($provider['name']);?>&nbsp;</td>
				<td class="listr"><?=@implode("<br>", $provider['dialpattern']);?>&nbsp;</td>
				<td class="list">&nbsp;</td>
			</tr>
			<?php endforeach; ?>
			<tr> 
				<td colspan="3" class="list" height="12">&nbsp;</td>
			</tr>
			<?php endif; ?>

		</table>
		<br>&nbsp;<i>generated on: <?=htmlspecialchars(date("D M j G:i T Y"));?></i>
	</body>
</html>
