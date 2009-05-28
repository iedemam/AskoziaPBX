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

$pgtitle = array(gettext("Status"), gettext("Active Channels"));

pbx_get_active_calls(&$active_calls, &$active_channels, &$channel_strings);

$channels = array();
$n = count($channel_strings);
for($i = 0; $i < $n; $i++) {
	$channels[$i] = preg_split("/\s+/", $channel_strings[$i]);
}

?>
<?php include("fbegin.inc"); ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">

	<? if ($n == 0): ?>
		
		<tr> 
			<td><strong><?=gettext("There are currently no active channels.");?></strong></td>
		</tr>
		
	<? else: ?>

		<tr>
			<td width="25%" class="listhdrr"><?=gettext("Channel");?></td>
			<td width="25%" class="listhdrr"><?=gettext("Location");?></td>
			<td width="25%" class="listhdrr"><?=gettext("State");?></td>
			<td width="25%" class="listhdr"><?=gettext("Application");?></td>
		</tr>
		
		<? $n = count($channels); ?>
		<? for($i = 0; $i < $n; $i++): ?>
		<tr>
			<td class="listbgl">
				<?=htmlspecialchars($channels[$i][0]);?>&nbsp;
			</td>
			<td class="listr">
				<?=htmlspecialchars($channels[$i][1]);?>&nbsp;
			</td>
			<td class="listr">
				<?=htmlspecialchars($channels[$i][2]);?>&nbsp;
			</td>
			<td class="listr">
				<?=htmlspecialchars($channels[$i][3]);?>&nbsp;
			</td>
		</tr>
		<? endfor; ?>
		
	<? endif; ?>

</table>
</form>
<?php include("fend.inc"); ?>
