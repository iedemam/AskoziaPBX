#!/usr/bin/php
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

$pgtitle_omit = true;

require("guiconfig.inc");

$product_name = system_get_product_name();
$pgtitle = array(gettext("$product_name webGUI"));

if ($_POST) {
	$config['system']['notes'] = base64_encode($_POST['notes']);
	write_config();
	header("Location: index.php");
	exit;
}

?>
<?php include("fbegin.inc"); ?>
<form action="" method="POST">
<table width="100%" border="0" cellspacing="0" cellpadding="0">
	<tr align="center" valign="top"> 
		<td height="10" colspan="2">&nbsp;</td>
	</tr>
	<tr align="center" valign="top"> 
		<td height="170" colspan="2"><img src="logobig.png"></td>
	</tr>
	<tr>
		<td colspan="2" class="listtopic"><?=gettext("System Information");?></td>
	</tr>
	<tr>
		<td width="30%" class="vncellt"><?=gettext("Name");?></td>
		<td width="70%" class="listr">
			<? echo $config['system']['hostname'] . "." . $config['system']['domain']; ?>
		</td>
	</tr>
	<tr>
		<td valign="top" class="vncellt"><?=gettext("Version");?></td>
		<td class="listr"><strong><?
			readfile("/etc/version");
			?></strong><br><?
			echo gettext("built on") . " ";
			readfile("/etc/version.buildtime");
		?></td>
	</tr>
	<tr>
		<td class="vncellt"><?=gettext("Platform");?></td>
		<td class="listr"><?=htmlspecialchars($g['platform']);?></td>
	</tr>
	<tr>
		<td class="vncellt"><?=gettext("Uptime");?></td>
		<td class="listr"><?
			exec("/usr/bin/uptime", $ut);
			$start = strpos($ut[0], "up") + 2;
			$end = strpos($ut[0], ",");
			$ut = substr($ut[0], $start, $end-$start);
			echo htmlspecialchars($ut);
		?></td>
	</tr><?
if ($config['lastchange']) {
	?><tr>
		<td class="vncellt"><?=gettext("Last Config Change");?></td>
		<td class="listr"> 
			<?=htmlspecialchars(date("D M j G:i:s T Y", $config['lastchange']));?>
		</td>
	</tr><?
}
	pbx_get_active_calls(&$active_calls, &$active_channels, &$channel_list);
	?><tr> 
		<td class="vncellt"><?=gettext("Active Calls");?></td>
		<td class="listr"><?=$active_calls;?>&nbsp;</td>
	</tr>
	<tr> 
		<td class="vncellt"><?=gettext("Active Channels");?></td>
		<td class="listr"><?=$active_channels;?>&nbsp;</td>
	</tr>
	<tr> 
		<td class="vncellt"><?=gettext("Memory Usage");?></td>
		<td class="listr"><?
			exec("/usr/bin/free", $memory);
			$memory = preg_split("/\s+/", $memory[1]);
			$totalMem = $memory[2];
			$freeMem = $memory[4];
			$usedMem = $totalMem - $freeMem;
			$memUsage = round(($usedMem * 100) / $totalMem, 0);
			$usageTitle = $usedMem . " / " . $totalMem . " kBytes";
			echo "<img src='bar_left.gif' height='15' width='4' border='0' align='absmiddle' title='$usageTitle'>";
			echo "<img src='bar_blue.gif' height='15' width='" . $memUsage . "' border='0' align='absmiddle' title='$usageTitle'>";
			echo "<img src='bar_gray.gif' height='15' width='" . (100 - $memUsage) . "' border='0' align='absmiddle' title='$usageTitle'>";
			echo "<img src='bar_right.gif' height='15' width='5' border='0' align='absmiddle' title='$usageTitle'> ";
			echo $memUsage . "%";
		?></td>
	</tr>
	<tr> 
		<td class="vncellt" valign="top"><?=gettext("Notes");?></td>
		<td class="listr">
			<textarea name="notes" cols="65" rows="7" id="notes" class="notes"><?=htmlspecialchars(base64_decode($config['system']['notes']));?></textarea><br>
			<input name="Submit" type="submit" class="formbtns" value="<?=gettext("Save");?>">
		</td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
