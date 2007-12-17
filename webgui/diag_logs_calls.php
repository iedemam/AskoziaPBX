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

	$db = sqlite_open("/var/log/asterisk/cdr.db", 0666, $err);
	$results = sqlite_query("delete from cdr", $db); // XXX sync problem here?

	header("Location: diag_logs_calls.php");
	exit;
}

function dump_clog($logfile, $max) {
	global $g, $config;

	$sor = isset($config['syslog']['reverse']) ? "desc" : "asc";
	$query = "select * from cdr order by id " . $sor . " limit $max";

	$db = sqlite_open($logfile, 0666, $err);
	$results = sqlite_query($query, $db);
	
	echo "<tr valign=\"top\">\n";
	echo "<td class=\"listhdrr\">start</td>\n";
	echo "<td class=\"listhdrr\">src</td>\n";
	echo "<td class=\"listhdrr\">dst</td>\n";
	echo "<td class=\"listhdrr\">channels</td>\n";
	echo "<td class=\"listhdrr\">last app</td>\n";
	echo "<td class=\"listhdrr\">sec.</td>\n";
	echo "<td class=\"listhdrr\">bill</td>\n";
	echo "<td class=\"listhdr\">disposition</td>\n";
	echo "</tr>\n";
	
	while ($cdr = sqlite_fetch_array($results, SQLITE_ASSOC)) {
		// filter out pseudo channels XXX to do
		//if (strpos($logent, "Zap/pseudo"))
		//	continue;
			
		//$logent = pbx_replace_uniqids_with_names($logent);
		
		echo "<tr valign=\"top\">\n";
		// start
		echo "<td class=\"listlr\" nowrap>".htmlspecialchars($cdr['start'])."&nbsp;</td>\n";
		// src
		if ($cdr['clid']) {
			echo "<td class=\"listr\">\n";
			echo "\t<span title=\"".
					htmlspecialchars($cdr['clid']).
					"\" style=\"cursor: help; border-bottom: 1px dashed #000000;\">".
					htmlspecialchars($cdr['src']).
				"</span>&nbsp;</td>\n";
		} else {
			echo "<td class=\"listr\">" . htmlspecialchars($cdr['src']) . "&nbsp;</td>\n";
		}
		
		// dst
		echo "<td class=\"listr\">".htmlspecialchars($cdr['dst'])."&nbsp;</td>\n";
		
		// channels
		echo "<td class=\"listr\">".htmlspecialchars($cdr['channel'])."&nbsp;";
		if($cdr['dstchannel']) {
			echo "-&gt;&nbsp;";
			echo htmlspecialchars($cdr['dstchannel'])."&nbsp;</td>\n";
		} else {
			echo "</td>\n";
		}
		
		// last app
		echo "<td class=\"listr\">";
		if ($cdr['lastapp']) {
			echo htmlspecialchars($cdr['lastapp'])."(";
			if ($cdr['lastdata']) {
				echo "&nbsp;".htmlspecialchars($cdr['lastdata'])."&nbsp;";
			}
			echo ")";
		}
		echo "&nbsp;</td>\n";
		
		// seconds
		echo "<td class=\"listr\">".htmlspecialchars($cdr['duration'])."&nbsp;</td>\n";
		// billable
		echo "<td class=\"listr\">".htmlspecialchars($cdr['billsec'])."&nbsp;</td>\n";
		// disposition
		echo "<td class=\"listr\">".htmlspecialchars($cdr['disposition'])."&nbsp;</td>\n";
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
			<td colspan="8" class="listtopic"> 
			  Last <?=$nentries;?> Call Records</td>
		  </tr>
		  <?php dump_clog("/var/log/asterisk/cdr.db", $nentries); ?>
		</table>
		<br><form action="diag_logs_calls.php" method="post">
<input name="clear" type="submit" class="formbtn" value="Clear log">
</form>
	</td>
  </tr>
</table>
<?php include("fend.inc"); ?>
