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

$pgtitle = array(gettext("Diagnostics"), gettext("Logs"));

if ($_POST['clear']) {
	exec("/usr/sbin/clog -i -s 262144 /var/log/cdr.log");
	header("Location: diag_logs_calls.php");
	exit;
}

$logging_pkg = packages_get_package("logging");

$nentries = $config['syslog']['nentries'];
if (!$nentries) {
	$nentries = 100;
}

//-------------pagination/filter logic start----------------------

if(isset($config['syslog']['reverse'])) {
	$sort = true;
}

if (isset($logging_pkg['active'])) {
	$source = "package";
	$logpath = $logging_pkg['datapath'] . "/asterisk/cdr.db";
	
	if($_GET['filter']) {
		$filter = addslashes($_GET['filter']);
	}

	$pages = display_calculate_pages_sqlite($filter, $logpath, $nentries);

	if(!$pages) {
		$message = gettext("No matches found.");
	}

	$current_page = display_calculate_current_page($pages, $sort);
	$query = display_get_query($start, $stop, $nentries, $current_page, $filter, $sort);
}
else {
	$source = "internal";
	$logpath = "/var/log/cdr.log";

	if($_GET['filter']) {
		$filter = $_GET['filter'];
	}

	$pages = display_calculate_pages($filter, $logpath, $source, $nentries);

	if(!$pages) {
		$message = gettext("No matches found.");
	}

	$current_page = display_calculate_current_page($pages, $sort);
	$command = display_get_command($current_page, $nentries, $source, $filter, $logpath, $sort);
}

$print_pageselector = display_page_selector($current_page, $pages, 12, $filter);

//---------------pagination/filter logic end----------------------------

function print_entry($cdr) {

	echo "<tr valign=\"top\">\n";
	// start
	echo "<td class=\"listlr\" nowrap>".htmlspecialchars($cdr['start'])."&nbsp;</td>\n";
	// src
	if ($cdr['src']) {
		echo "<td class=\"listr\">\n";
		echo "\t<span title=\"".
				htmlspecialchars($cdr['clid']).
				"\" class=\"print_entry\">".
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

function dump_clog($command) {
	global $g, $config;

	exec($command, $logarr);

	foreach ($logarr as $logent) {
		$logent = preg_split("/\s+/", $logent, 6);
		$cdr = strstr($logent[5], "\"");
		$cdr = explode(",", $cdr);
		$cdr = str_replace("\"", "", $cdr);
    
		$timestamp = join(" ", array_slice($logent, 0, 2));
		$tstime = explode(" ", $cdr[8]);
		$tstime = $tstime[1];
		$timestamp .= " " . $tstime;

		$clog_cdr['start'] = $timestamp;
		$clog_cdr['src'] = $cdr[1];
		$clog_cdr['clid'] = $cdr[0];
		$clog_cdr['dst'] = $cdr[2];
		$clog_cdr['channel'] = $cdr[4];
		$clog_cdr['dstchannel'] = $cdr[5];
		$clog_cdr['lastapp'] = $cdr[6];
		$clog_cdr['lastdata'] = $cdr[7];
		$clog_cdr['duration'] = $cdr[11];
		$clog_cdr['billsec'] = $cdr[12];
		$clog_cdr['disposition'] = $cdr[13];

		print_entry($clog_cdr);
	}
}

function dump_sqlite($logfile, $query) {
	global $g, $config;
	
	$db = sqlite_open($logfile, 0666, $err);
	$results = sqlite_query($query, $db);

	while ($db_cdr = sqlite_fetch_array($results, SQLITE_ASSOC)) {
		print_entry($db_cdr);
	}
}

include("fbegin.inc");

?><script type="text/JavaScript">
<!--
	<?=javascript_filter_textbox("functions");?>

	jQuery(document).ready(function(){

		<?=javascript_filter_textbox("ready");?>

	});

//-->
</script>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav"><?

			$tabs = array(gettext('System') => 'diag_logs.php',
					gettext('PBX') => 'diag_logs_pbx.php',
					gettext('Calls') => 'diag_logs_calls.php',
					gettext('Settings') => 'diag_logs_settings.php');
			dynamic_tab_menu($tabs);

			?></ul>
		</td>
	</tr>
	<tr> 
		<td class="tabcont">

			<?
			echo $print_pageselector;
			?>
	
			<table width="100%" border="0" cellspacing="0" cellpadding="0">
				<tr> 
					<td colspan="8" class="listtopic">
					<form action="diag_logs_calls.php" method="get" id="filtering" class="display">
						<div class="align_right">
							<label for="filter" style="display: none;"> <?=gettext("filter");?></label>
							<input name="filter" id="filter" type="text" width="20" class="filterbox" value="<?=$filter;?>">
							<?
							if(!$filter)
								echo "<input type='image' src='set_filter.png' name='set' class='verticalalign'>";
							else
								echo "<a href='?'><img class='verticalalign' src='remove_filter.png' name='erase'></a>";
							?>
						</div>
					</form>
					<div class="padding_top"><?=gettext("Call Records");?></div>
					</td>
				</tr>
				<tr valign="top">
					<td class="listhdrr"><?=gettext("start");?></td>
					<td class="listhdrr"><?=gettext("src");?></td>
					<td class="listhdrr"><?=gettext("dst");?></td>
					<td class="listhdrr"><?=gettext("channels");?></td>
					<td class="listhdrr"><?=gettext("last app");?></td>
					<td class="listhdrr"><?=gettext("sec.");?></td>
					<td class="listhdrr"><?=gettext("bill");?></td>
					<td class="listhdr"><?=gettext("disposition");?></td>
				</tr><?

				if ($source == "internal") {
					dump_clog($command);

				} else if ($source == "package") {
					dump_sqlite($logpath, $query);
				}

				?><tr> 
					<?
					if($message)
						echo "<td class='filter_info_message' colspan='8' height='12'>$message</td>";
					else
						echo "<td class='list' colspan='8' height='12'>&nbsp;</td>";
					?>
				</tr>
			</table><?

			echo $print_pageselector;

		if ($source == "internal") {
			?><br>
			<form action="diag_logs_calls.php" method="post">
				<input name="clear" type="submit" class="formbtn" value="<?=gettext("Clear log");?>">
			</form><?
		}

		?></td>
	</tr>
</table>
<?php include("fend.inc"); ?>
