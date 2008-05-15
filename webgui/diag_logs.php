#!/usr/local/bin/php
<?php 
/*
	$Id$
	originally part of m0n0wall (http://m0n0.ch/wall)
	continued modifications as part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
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

$pgtitle = array("Diagnostics", "Logs");
require("guiconfig.inc");

if ($_POST['clear']) {
	exec("/usr/sbin/clog -i -s 262144 $logpath");
	header("Location: diag_logs.php");
	exit;
}

$packages = packages_get_packages();

$sort = isset($config['syslog']['reverse']) ? "-r" : "";
$nentries = $config['syslog']['nentries'];
if (!$nentries) {
	$nentries = 100;
}

$source = "internal";
$logpath = "/var/log/system.log";
$command = "/usr/sbin/clog $logpath | /usr/bin/tail $sort -n $nentries";

if (isset($packages['logging']['active'])) {
	$source = "package";
	$logpath = $packages['logging']['datapath'] . "/system/system.log";
	$command = "/usr/bin/tail $sort -n $nentries $logpath";
}

exec($command, $logarr);

?>
<?php include("fbegin.inc"); ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav"><?

			$tabs = array('System' => 'diag_logs.php',
							'PBX' => 'diag_logs_pbx.php',
							'Calls' => 'diag_logs_calls.php',
							'Settings' => 'diag_logs_settings.php');
			dynamic_tab_menu($tabs);

			?></ul>
		</td>
	</tr>
	<tr> 
		<td class="tabcont">
			<table width="100%" border="0" cellspacing="0" cellpadding="0">
				<tr> 
					<td colspan="2" class="listtopic">Last <?=$nentries;?> system log entries</td>
				</tr><?

			foreach ($logarr as $logent) {
				$logent = preg_split("/\s+/", $logent, 6);
				?><tr valign="top">
					<td class="listlr" nowrap><?=htmlspecialchars(join(" ", array_slice($logent, 0, 3)));?></td>
					<td class="listr"><?=htmlspecialchars($logent[4] . " " . $logent[5]);?></td>
				</tr><?
			}

			?></table><?

		if ($source == "internal") {
			?><br>
			<form action="diag_logs.php" method="post">
				<input name="clear" type="submit" class="formbtn" value="Clear Log">
			</form><?
		}

		?></td>
	</tr>
</table>
<?php include("fend.inc"); ?>
