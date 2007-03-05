#!/usr/local/bin/php
<?php 
/*
	$Id: firewall_nat.php 72 2006-02-10 11:13:01Z jdegraeve $
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

$pgtitle = array("Firewall", "NAT", "Inbound");
require("guiconfig.inc");

if (!is_array($config['nat']['rule'])) {
	$config['nat']['rule'] = array();
}
nat_rules_sort();
$a_nat = &$config['nat']['rule'];

if ($_POST) {

	$pconfig = $_POST;

	if ($_POST['apply']) {
		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval |= filter_configure();
			config_unlock();
		}
		$savemsg = get_std_save_message($retval);
		
		if ($retval == 0) {
			if (file_exists($d_natconfdirty_path))
				unlink($d_natconfdirty_path);
			if (file_exists($d_filterconfdirty_path))
				unlink($d_filterconfdirty_path);
		}
	}
}

if ($_GET['act'] == "del") {
	if ($a_nat[$_GET['id']]) {
		unset($a_nat[$_GET['id']]);
		write_config();
		touch($d_natconfdirty_path);
		header("Location: firewall_nat.php");
		exit;
	}
}
?>
<?php include("fbegin.inc"); ?>
<form action="firewall_nat.php" method="post">
<?php if ($savemsg) print_info_box($savemsg); ?>
<?php if (file_exists($d_natconfdirty_path)): ?><p>
<?php print_info_box_np("The NAT configuration has been changed.<br>You must apply the changes in order for them to take effect.");?><br>
<input name="apply" type="submit" class="formbtn" id="apply" value="Apply changes"></p>
<?php endif; ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr><td class="tabnavtbl">
  <ul id="tabnav">
<?php
   	$tabs = array('Inbound' => 'firewall_nat.php',
           		  'Server NAT' => 'firewall_nat_server.php',
           		  '1:1' => 'firewall_nat_1to1.php',
           		  'Outbound' => 'firewall_nat_out.php');
	dynamic_tab_menu($tabs);
?>         
  </ul>
  </td></tr>
  <tr> 
    <td class="tabcont">
              <table width="100%" border="0" cellpadding="0" cellspacing="0">
                <tr> 
                  <td width="5%" class="listhdrr">If</td>
                  <td width="5%" class="listhdrr">Proto</td>
                  <td width="20%" class="listhdrr">Ext. port range</td>
                  <td width="20%" class="listhdrr">NAT IP</td>
                  <td width="20%" class="listhdrr">Int. port range</td>
                  <td width="20%" class="listhdr">Description</td>
                  <td width="5%" class="list"></td>
				</tr>
			  <?php $i = 0; foreach ($a_nat as $natent): ?>
                <tr valign="top"> 
				  <td class="listlr">
                  <?php
						if (!$natent['interface'] || ($natent['interface'] == "wan"))
							echo "WAN";
						else
							echo htmlspecialchars($config['interfaces'][$natent['interface']]['descr']);
				  ?>
                  </td>
                  <td class="listr"> 
                    <?=strtoupper($natent['protocol']);?>
                  </td>
                  <td class="listr">
                    <?php 
						list($beginport, $endport) = split("-", $natent['external-port']);
						if ((!$endport) || ($beginport == $endport)) {
				  			echo $beginport;
							if ($wkports[$beginport])
								echo " (" . $wkports[$beginport] . ")";
						} else
							echo $beginport . " - " . $endport;
				  ?>
                  </td>
                  <td class="listr"> 
                    <?=$natent['target'];?>
					<?php if ($natent['external-address'])
						echo "<br>(ext.: " . $natent['external-address'] . ")";
					?>
                  </td>
                  <td class="listr"> 
                    <?php if ((!$endport) || ($beginport == $endport)) {
				  			echo $natent['local-port'];
							if ($wkports[$natent['local-port']])
								echo " (" . $wkports[$natent['local-port']] . ")";
						} else
							echo $natent['local-port'] . " - " . 
								($natent['local-port']+$endport-$beginport);
				  ?>
                  </td>
                  <td class="listbg"> 
                    <?=htmlspecialchars($natent['descr']);?>&nbsp;
                  </td>
                  <td valign="middle" class="list" nowrap> <a href="firewall_nat_edit.php?id=<?=$i;?>"><img src="e.gif" title="edit rule" width="17" height="17" border="0"></a>
                     &nbsp;<a href="firewall_nat.php?act=del&id=<?=$i;?>" onclick="return confirm('Do you really want to delete this rule?')"><img src="x.gif" title="delete rule" width="17" height="17" border="0"></a></td>
				</tr>
			  <?php $i++; endforeach; ?>
                <tr> 
                  <td class="list" colspan="6"></td>
                  <td class="list"> <a href="firewall_nat_edit.php"><img src="plus.gif" title="add rule" width="17" height="17" border="0"></a></td>
				</tr>
              </table><br>
                    <span class="vexpl"><span class="red"><strong>Note:<br>
                      </strong></span>It is not possible to access NATed services 
                      using the WAN IP address from within LAN (or an optional 
                      network).</span></td>
  </tr>
</table>
            </form>
<?php include("fend.inc"); ?>
