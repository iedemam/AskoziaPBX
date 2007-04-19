#!/usr/local/bin/php
<?php 
/*
	$Id: diag_logs_asterisk.php 38 2007-03-22 22:47:23Z michael.iedema $
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
	$nentries = 50;

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
	
	foreach ($logarr as $logent) {
		$logent = preg_split("/\s+/", $logent, 6);
		$cdr = strstr($logent[5], "\"");
		$cdr = explode(",", $cdr);
		$cdr = str_replace("\"", "", $cdr);
		
		echo "<tr valign=\"top\">\n";
		
		echo "<td class=\"listlr\" nowrap>" . htmlspecialchars(join(" ", array_slice($logent, 0, 3))) . "</td>\n";
		echo "<td class=\"listr\">" . htmlspecialchars($cdr[0]) . 
			"&nbsp;-&gt;&nbsp;" . htmlspecialchars($cdr[2]) . "&nbsp;( " .
			htmlspecialchars($cdr[12]) . " sec. )</td>\n";
		
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
				  'Asterisk' => 'diag_logs_asterisk.php',
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
