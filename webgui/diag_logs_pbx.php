#!/usr/local/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
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

$pgtitle = array("Diagnostics", "Logs");
require("guiconfig.inc");

$nentries = $config['syslog']['nentries'];
if (!$nentries)
	$nentries = 100;

if ($_POST['clear']) {
	exec("/usr/sbin/clog -i -s 262144 /var/log/pbx.log");
	/* redirect to avoid reposting form data on refresh */
	header("Location: diag_logs_pbx.php");
	exit;
}

function dump_clog($logfile, $tail) {
	global $g, $config;

	$sor = isset($config['syslog']['reverse']) ? "-r" : "";

	exec("/usr/sbin/clog " . $logfile . " | tail {$sor} -n " . $tail, $logarr);
	
	foreach ($logarr as $logent) {
		$logent = asterisk_replace_uniqids_with_names($logent);
		$logent = preg_split("/\s+/", $logent, 7);
		
		// filter some unimportant messages
		if (($logent[6] == "Found") || 
			(strpos($logent[6], "Manager 'admin' logged") !== false) ||
			(strpos($logent[6], "Remote UNIX connection") !== false)) {
			continue;
		}
		
		echo "<tr valign=\"top\">\n";
		
		echo "<td class=\"listlr\" nowrap>" . htmlspecialchars(join(" ", array_slice($logent, 0, 3))) . "</td>\n";
		echo "<td class=\"listr\">" . str_replace("^M", "<br>", htmlspecialchars($logent[6])) . "</td>\n";

		echo "</tr>\n";
	}
}

?>
<?php include("fbegin.inc"); ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr><td class="tabnavtbl">
  <ul id="tabnav">
<?php 
	$tabs = array('System' => 'diag_logs.php',
				  'PBX' => 'diag_logs_pbx.php',
				  'Calls' => 'diag_logs_calls.php',
	       		  'Settings' => 'diag_logs_settings.php');
	dynamic_tab_menu($tabs);
?> 
  </ul>
  </td></tr>
  <tr> 
    <td class="tabcont">
		<table width="100%" border="0" cellspacing="0" cellpadding="0">
		  <tr> 
			<td colspan="2" class="listtopic"> 
			  Last <?=$nentries;?> Asterisk log entries</td>
		  </tr>
		  <?php dump_clog("/var/log/pbx.log", $nentries); ?>
		</table>
		<br><form action="diag_logs_pbx.php" method="post">
<input name="clear" type="submit" class="formbtn" value="Clear log">
</form>
	</td>
  </tr>
</table>
<?php include("fend.inc"); ?>
