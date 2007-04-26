#!/usr/local/bin/php
<?php
/*
	$Id$
	part of m0n0wall (http://m0n0.ch/wall)

	Copyright (C) 2005 Paul Taylor (paultaylor@winndixie.com) and Manuel Kasper <mk@neon1.net>.
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

$pgtitle = array("Diagnostics", "ARP table");
require("guiconfig.inc");

$id = $_GET['id'];
if (isset($_POST['id']))
        $id = $_POST['id'];

if ($_GET['act'] == "del") {
	if (isset($id)) {
		/* remove arp entry from arp table */
		mwexec("/usr/sbin/arp -d " . escapeshellarg($id));

		/* redirect to avoid reposting form data on refresh */
		header("Location: diag_arp.php");
		exit;
	} else {
		/* remove all entries from arp table */
		mwexec("/usr/sbin/arp -d -a");

		/* redirect to avoid reposting form data on refresh */
		header("Location: diag_arp.php");
		exit;
	}
}

$resolve = isset($config['syslog']['resolve']);
?>

<?php include("fbegin.inc"); ?>

<?php

exec("/usr/sbin/arp -an",$rawdata);

$i = 0; 
$ifdescrs = array('lan' => 'LAN');
						
foreach ($ifdescrs as $key =>$interface) {
	$hwif[$config['interfaces'][$key]['if']] = $interface;
}

$data = array();
foreach ($rawdata as $line) {
	$elements = explode(' ',$line);
	
	if ($elements[3] != "(incomplete)") {
		$arpent = array();
		$arpent['ip'] = trim(str_replace(array('(',')'),'',$elements[1]));
		$arpent['mac'] = trim($elements[3]);
		$arpent['interface'] = trim($elements[5]);
		$data[] = $arpent;
	}
}

function getHostName($mac,$ip)
{
	global $dhcpmac, $dhcpip, $resolve;
	
	if ($dhcpmac[$mac])
		return $dhcpmac[$mac];
	else if ($dhcpip[$ip])
		return $dhcpip[$ip];
	else if ($resolve) 
		return gethostbyaddr($ip);
	else
		return "&nbsp;";
}

?>

<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td class="listhdrr">IP address</td>
    <td class="listhdrr">MAC address</td>
    <td class="listhdrr">Hostname</td>
    <td class="listhdr">Interface</td>
    <td class="list"></td>
  </tr>
<?php $i = 0; foreach ($data as $entry): ?>
  <tr>
    <td class="listlr"><?=$entry['ip'];?></td>
    <td class="listr"><?=$entry['mac'];?></td>
    <td class="listr"><?=getHostName($entry['mac'], $entry['ip']);?></td>
    <td class="listr"><?=$hwif[$entry['interface']];?></td>
    <td valign="middle" nowrap class="list"><a href="diag_arp.php?act=del&id=<?=$entry['ip'];?>"><img src="x.gif" title="delete arp entry" width="17" height="17" border="0"></a></td>
  </tr>
<?php $i++; endforeach; ?>
  <tr> 
    <td></td>
  </tr> 
  <tr> 
    <td class="list" colspan="4"></td>
    <td class="list"><a href="diag_arp.php?act=del"><img src="x.gif" title="remove all entries from arp table" width="17" height="17" border="0"></a></td>
  </tr>
  <tr>
    <td colspan="4">
      <span class="vexpl"><span class="red"><strong>Hint:<br>
      </strong></span>IP addresses are resolved to hostnames if
      &quot;Resolve IP addresses to hostnames&quot; 
      is checked on the <a href="diag_logs_settings.php">
      Diagnostics: Logs</a> page.</span>
    </td>
  </tr>
</table>

<?php include("fend.inc"); ?>
