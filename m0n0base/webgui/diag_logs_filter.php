#!/usr/local/bin/php
<?php 
/*
	$Id: diag_logs_filter.php 72 2006-02-10 11:13:01Z jdegraeve $
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

$pgtitle = array("Diagnostics", "Logs");
require("guiconfig.inc");

$protocols = explode(" ", "TCP UDP TCP/UDP ICMP ESP AH GRE IPv6 IGMP any");

$nentries = $config['syslog']['nentries'];
$resolve = isset($config['syslog']['resolve']);

if (!$nentries)
	$nentries = 50;

if ($_POST['clear']) {
	exec("/usr/sbin/clog -i -s 262144 /var/log/filter.log");
	/* redirect to avoid reposting form data on refresh */
	header("Location: diag_logs_filter.php");
	exit;
}


if (isset($_GET['act']) && preg_match("/^[pb]+$/", $_GET['act'])) {
	$action = $_GET['act'];
	$ifstring  .= "&act=$action";
	$srcstring .= "&act=$action";
	$dststring .= "&act=$action";
	$prstring  .= "&act=$action";
}

if (isset($_GET['if']) && ($_GET['if'] != "")) {
	$iface = $_GET['if'];
	$actstring .= "&if=$iface";
	$srcstring .= "&if=$iface";
	$dststring .= "&if=$iface";
	$prstring  .= "&if=$iface";
}

if (isset($_GET['pr']) && in_array($_GET['pr'], $protocols)) {
	$proto = $_GET['pr'];
	$actstring .= "&pr=$proto";
	$ifstring  .= "&pr=$proto";
	$srcstring .= "&pr=$proto";
	$dststring .= "&pr=$proto";
}

if (isset($_GET['sp']) && (is_numeric($_GET['sp']))) {
	$srcport = $_GET['sp'];
	$actstring .= "&sp=$srcport";
	$ifstring  .= "&sp=$srcport";
	$dststring .= "&sp=$srcport";
	$prstring  .= "&sp=$srcport";
}

if (isset($_GET['dp']) && (is_numeric($_GET['dp']))) {
	$dstport = $_GET['dp'];
	$actstring .= "&dp=$dstport";
	$ifstring  .= "&dp=$dstport";
	$srcstring .= "&dp=$dstport";
	$prstring  .= "&dp=$dstport";
}


function dump_clog($logfile, $tail, $withorig = true) {
	global $g, $config;

	$sor = isset($config['syslog']['reverse']) ? "-r" : "";

	exec("/usr/sbin/clog " . $logfile . " | tail {$sor} -n " . $tail, $logarr);
	
	foreach ($logarr as $logent) {
		$logent = preg_split("/\s+/", $logent, 6);
		echo "<tr valign=\"top\">\n";
		
		if ($withorig) {
			echo "<td class=\"listlr\" nowrap>" . htmlspecialchars(join(" ", array_slice($logent, 0, 3))) . "</td>\n";
			echo "<td class=\"listr\">" . htmlspecialchars($logent[4] . " " . $logent[5]) . "</td>\n";
		} else {
			echo "<td class=\"listlr\" colspan=\"2\">" . htmlspecialchars($logent[5]) . "</td>\n";
		}
		echo "</tr>\n";
	}
}

function conv_clog($logfile, $tail) {
	global $g, $config, $iface, $action, $proto, $srcport, $dstport;
	
	/* make interface/port table */
	$iftable = array();
	$iftable[$config['interfaces']['lan']['if']] = "LAN";
	$iftable[get_real_wan_interface()] = "WAN";
	for ($i = 1; isset($config['interfaces']['opt' . $i]); $i++)
		$iftable[$config['interfaces']['opt' . $i]['if']] = $config['interfaces']['opt' . $i]['descr'];

	$sor = isset($config['syslog']['reverse']) ? "-r" : "";

	exec("/usr/sbin/clog " . $logfile . " | tail {$sor} -n " . $tail, $logarr);
	
	$filterlog = array();

	foreach ($logarr as $logent) {
		$logent = preg_split("/\s+/", $logent, 6);
		$ipfa = explode(" ", $logent[5]);
		
		$flent = array();
		$i = 0;
		$flent['time'] = $ipfa[$i];
		$i++;
		if (substr($ipfa[$i], -1) == "x") {
			$flent['count'] = substr($ipfa[$i], 0, -1);
			$i++;
		}
		
		if ($iftable[$ipfa[$i]])
			$flent['interface'] = $iftable[$ipfa[$i]];
		else if (strpos($ipfa[$i], "ng") !== false)
			$flent['interface'] = "PPTP";
		else
			$flent['interface'] = $ipfa[$i];
		
		if (isset($iface)) {
			if ($iface != $flent['interface'])
				continue;
		}
		
		$i += 2;
		if (!isset($action) || strstr($action, $ipfa[$i]))
			$flent['act'] = $ipfa[$i];
		else
			continue; 
		$i++;
		list($flent['src'], $flent['srcport']) = format_ipf_ip($ipfa[$i],$srcport);
		if (!isset($flent['src']))
			continue;
		$i += 2;
		list($flent['dst'], $flent['dstport']) = format_ipf_ip($ipfa[$i],$dstport);
		if (!isset($flent['dst']))
			continue;
		$i += 2;
		$protocol = strtoupper($ipfa[$i]);
		if (!isset($proto) || ($proto == $protocol))
			$flent['proto'] = $protocol;
		else
			continue;
		if ($protocol == "ICMP") {
			$i += 5;
			$flent['dst'] = $flent['dst'] . ", type " . $ipfa[$i];
		}
		$filterlog[] = $flent;
	}
	
	return $filterlog;
}

function format_ipf_ip($ipfip,$uport) {
	global $resolve;

	list($ip,$port) = explode(",", $ipfip);
	if ($resolve) {
		if (!$port)
			return array(gethostbyaddr($ip), "");
		if ($uport == "" || ($uport == $port))
			return array(gethostbyaddr($ip) . ", port " . $port, $port);
		return;
	}

	if (!$port)
		return array($ip, "");
	if ($uport == "" || ($uport == $port))
		return array($ip . ", port " . $port, $port);
	return;
}
?>

<?php include("fbegin.inc"); ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr><td class="tabnavtbl">
  <ul id="tabnav">
<?php 
   	$tabs = array('System' => 'diag_logs.php',
           		  'Firewall' => 'diag_logs_filter.php',
           		  'DHCP' => 'diag_logs_dhcp.php',
           		  'PPTP VPN' => 'diag_logs_vpn.php',
           		  'Settings' => 'diag_logs_settings.php');
	dynamic_tab_menu($tabs);
?> 
  </ul>
  </td></tr>
  <tr>
    <td class="tabcont">
<?php if (!isset($config['syslog']['rawfilter'])):
	$filterlog = conv_clog("/var/log/filter.log", $nentries);
?>
		<table width="100%" border="0" cellpadding="0" cellspacing="0"><tr>
		  <td colspan="6" class="listtopic"> 
			    Last <?=$nentries;?> firewall log entries</td>
			</tr>
			<tr>
			  <td width="10%" class="listhdrr"><a href="?<?=substr($actstring, 1);?>" style="color:black" title="reset action and reload firewall logs page">Act</a></td>
			  <td width="20%" class="listhdrr">Time</td>
			  <td width="10%" class="listhdrr"><a href="?<?=substr($ifstring, 1);?>" style="color:black" title="reset interface and reload firewall logs page">If</a></td>
			  <td width="20%" class="listhdrr"><a href="?<?=substr($srcstring, 1);?>" style="color:black" title="reset source port entry and reload firewall logs page">Source</a></td>
			  <td width="20%" class="listhdrr"><a href="?<?=substr($dststring, 1);?>" style="color:black" title="reset destination port entry and reload firewall logs page">Destination</a></td>
			  <td width="10%" class="listhdrr"><a href="?<?=substr($prstring, 1);?>" style="color:black" title="reset protocol entry and reload firewall logs page">Proto</a></td>
			</tr>
	<?php
	$actstring .= '">';
	$ifstring  .= '" style="color:black" title="click to select interface">';
	$srcstring .= '" style="color:black" title="click to select source port">';
	$dststring .= '" style="color:black" title="click to select destination port">';
	$prstring  .= '" style="color:black" title="click to select protocol">';
	?>
			 <?php foreach ($filterlog as $filterent): ?>
			<tr>
			  <td class="listlr" nowrap>
			  <?php if (strstr(strtolower($filterent['act']), "p"))
			  			$img = "pass.gif";
					 else 
					 	$img = "block.gif";
			 	?>
			  <a href="?act=<?=$filterent['act'];?><?=$actstring;?><img src="<?=$img;?>" width="11" height="11" align="absmiddle" border="0" title="click to select action"></a>
			  <?php if ($filterent['count']) echo $filterent['count'];?></td>
			  <td class="listr" nowrap><?=htmlspecialchars($filterent['time']);?></td>
			  <td class="listr" nowrap>
			    <a href="?if=<?=$filterent['interface'];?><?=$ifstring;?><?=htmlspecialchars($filterent['interface']);?></a></td>
			  <td class="listr" nowrap>
			    <a href="?sp=<?=htmlspecialchars($filterent['srcport']);?><?=$srcstring;?><?=htmlspecialchars($filterent['src']);?></a></td>
			  <td class="listr" nowrap>
			    <a href="?dp=<?=htmlspecialchars($filterent['dstport']);?><?=$dststring;?><?=htmlspecialchars($filterent['dst']);?></a></td>
			  <td class="listr" nowrap>
			    <a href="?pr=<?=htmlspecialchars($filterent['proto']);?><?=$prstring;?><?=htmlspecialchars($filterent['proto']);?></a></td>
			</tr><?php endforeach; ?>
                    </table>
		<br><table width="100%" border="0" cellspacing="0" cellpadding="0">
                      <tr> 
                        <td width="100%"><strong><span class="red">Note:</span></strong><br>
                          There are many possibilities to filter this log.
                          Just click on the accept (<img src="pass.gif">) or
			  deny symbol (<img src="block.gif">) to filter for
			  accepted or denied IP packets. Do the same for the desired
			  interface, source/destination port or protocol. To deselect
			  a selected filter entry, click on the column description above.
                          To reset all filter entries and reload the firewall logs page,
			  click on the &quot;Firewall&quot; tab below &quot;Diagnostics: Logs&quot;.
                        </td>
		      </tr>
		</table>
<?php else: ?>
		<table width="100%" border="0" cellspacing="0" cellpadding="0">
		  <tr> 
			<td colspan="2" class="listtopic"> 
			  Last <?=$nentries;?> firewall log entries</td>
		  </tr>
		  <?php dump_clog("/var/log/filter.log", $nentries, false); ?>
		</table>
<?php endif; ?>
		<br><form action="diag_logs_filter.php" method="post">
<input name="clear" type="submit" class="formbtn" value="Clear log">
</form>
	</td>
  </tr>
</table>
<?php include("fend.inc"); ?>
