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
	exec("/usr/sbin/clog -i -s 262144 /var/log/cdr.log");
	/* redirect to avoid reposting form data on refresh */
	header("Location: diag_logs_calls.php");
	exit;
}

function dump_clog($logfile, $tail) {
	global $g, $config;

	$sor = isset($config['syslog']['reverse']) ? "-r" : "";

	exec("/usr/sbin/clog " . $logfile . " | tail {$sor} -n " . $tail, $logarr);	
	
	/*
	0	clid		""Grandstream" <2424>",
	1	src			"2424",
	2	dst			"4242",
	3	dcontext	"SIP-PHONE-15751265024607fa93812e3",
	4	channel		"SIP/2424-086be000",
	5	dstchannel	"",
	6	lastapp		"Dial",
	7	lastdata	"SIP/4242|30",
	8	start		"2007-03-27 10:18:04",
	9	answer		"",
	10	end			"2007-03-27 10:18:04",
	11	duration	"0",
	12	billsec		"0",
	13	disposition	"FAILED",
	14	amaflags	"DOCUMENTATION",
	15	accountcode	"",
	16	uniqueid	"1174990684.0",
	17	userfield	""
	*/
	
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
	
	foreach ($logarr as $logent) {
		// filter out pseudo channels
		if (strpos($logent, "Zap/pseudo"))
			continue;

		$logent = preg_split("/\s+/", $logent, 6);
		$cdr = strstr($logent[5], "\"");
		$cdr = explode(",", $cdr);
		$cdr = str_replace("\"", "", $cdr);
		
		$timestamp = join(" ", array_slice($logent, 0, 2));
		$tstime = explode(" ", $cdr[8]);
		$tstime = $tstime[1];
		$timestamp .= " " . $tstime;
		
		echo "<tr valign=\"top\">\n";
		// start
		echo "<td class=\"listlr\" nowrap>".htmlspecialchars($timestamp)."&nbsp;</td>\n";
		// src
		echo "<td class=\"listr\">\n";
		echo "\t<span title=\"".
				htmlspecialchars($cdr[0]).
				"\" style=\"cursor: help; border-bottom: 1px dashed #000000;\">".
				htmlspecialchars($cdr[1]).
			"</span>&nbsp;</td>\n";
		
		// dst
		echo "<td class=\"listr\">".htmlspecialchars($cdr[2])."&nbsp;</td>\n";
		
		// channels
		if (strpos($cdr[4], "ISDN-PROVIDER") !== false) {
			$chan = explode("/", $cdr[4]);
			$chan[1] = asterisk_uniqid_to_name($chan[1]);
			$chan = implode("/", $chan);
		} else if (strpos($cdr[4], "PROVIDER") !== false) {
			$chan = explode("/", $cdr[4]);
			$chan[1] = asterisk_uniqid_to_name(substr($chan[1], 0, strrpos($chan[1], "-")));
			$chan = implode("/", $chan);
		} else {
			$chan = substr($cdr[4], 0, strpos($cdr[4], "-"));
		}
		echo "<td class=\"listr\">".htmlspecialchars($chan)."&nbsp;";
		if($cdr[5]) {
			echo "-&gt;&nbsp;";
			if (strpos($cdr[5], "ISDN-PROVIDER") !== false) {
				$chan = explode("/", $cdr[5]);
				$chan[1] = asterisk_uniqid_to_name($chan[1]);
				$chan = implode("/", $chan);
			} else if (strpos($cdr[5], "PROVIDER") !== false) {
				$chan = explode("/", $cdr[5]);
				$chan[1] = asterisk_uniqid_to_name(substr($chan[1], 0, strrpos($chan[1], "-")));
				$chan = implode("/", $chan);
			} else {
				$chan = substr($cdr[5], 0, strpos($cdr[5], "-"));
			}
			echo htmlspecialchars($chan)."&nbsp;</td>\n";
		} else {
			echo "</td>\n";
		}
		
		// last app
		echo "<td class=\"listr\">";
		if ($cdr[6]) {
			echo htmlspecialchars($cdr[6])."(";
			// need to replace dial provider uniqid strings with names
			if ($cdr[7]) {
				if (strpos($cdr[7], "IAX-PROVIDER") !== false) {
					$appdata = explode("/", $cdr[7]);
					$appdata[1] = asterisk_uniqid_to_name($appdata[1]);
					$appdata = implode("/", $appdata);

				} else if (strpos($cdr[7], "SIP-PROVIDER") !== false) {
					$infront = strpos($cdr[7], "@") + 1;
					$behind = strrpos($cdr[7], "|");
					$appdata = substr($cdr[7], 0, $infront) . 
						asterisk_uniqid_to_name(substr($cdr[7], $infront, $behind - $infront)) .
						substr($cdr[7], $behind);

				} else if (strpos($cdr[7], "ISDN-PROVIDER") !== false) {
					$appdata = explode("/", $cdr[7]);
					$appdata[1] = asterisk_uniqid_to_name($appdata[1]);
					$appdata = implode("/", $appdata);

				} else {
					$appdata = $cdr[7];
				}
				echo "&nbsp;".htmlspecialchars($appdata)."&nbsp;";
			}
			echo ")";
		}
		echo "&nbsp;</td>\n";
		
		// seconds
		echo "<td class=\"listr\">".htmlspecialchars($cdr[11])."&nbsp;</td>\n";
		// billable
		echo "<td class=\"listr\">".htmlspecialchars($cdr[12])."&nbsp;</td>\n";
		// disposition
		echo "<td class=\"listr\">".htmlspecialchars($cdr[13])."&nbsp;</td>\n";
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
		  <?php dump_clog("/var/log/cdr.log", $nentries); ?>
		</table>
		<br><form action="diag_logs_calls.php" method="post">
<input name="clear" type="submit" class="formbtn" value="Clear log">
</form>
	</td>
  </tr>
</table>
<?php include("fend.inc"); ?>
