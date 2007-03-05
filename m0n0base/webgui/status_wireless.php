#!/usr/local/bin/php
<?php 
/*
	$Id: status_wireless.php 173 2006-12-09 18:44:57Z mkasper $
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

$pgtitle = array("Status", "Wireless");
require("guiconfig.inc");

function get_wireless_info($ifdescr) {
	
	global $config, $g;
	
	$ifinfo = array();
	$ifinfo['if'] = $config['interfaces'][$ifdescr]['if'];
	
	if ($config['interfaces'][$ifdescr]['wireless']['mode'] != "hostap") {
		/* get scan list */
		exec("/sbin/ifconfig -v " . $ifinfo['if'] . " list scan", $scanlist);
		
		$ifinfo['scanlist'] = array();
		$title = array_shift($scanlist);
		
		/* determine width of SSID field */
		$ssid_fldwidth = strpos($title, "BSSID");
		
		foreach ($scanlist as $sl) {
			if ($sl) {
				$slent = array();
				
				$slent['ssid'] = trim(substr($sl, 0, $ssid_fldwidth));
				
				$remflds = preg_split("/\s+/", substr($sl, $ssid_fldwidth), 6);
				
				$slent['bssid'] = $remflds[0];
				$slent['channel'] = $remflds[1];
				$slent['rate'] = $remflds[2];
				list($slent['sig'],$slent['noise']) = explode(":", $remflds[3]);
				$slent['int'] = $remflds[4];
				$slent['caps'] = preg_split("/\s+/", $remflds[5]);
				
				$ifinfo['scanlist'][] = $slent;
			}
		}
	}
	
	/* if in hostap mode: get associated stations */
	if ($config['interfaces'][$ifdescr]['wireless']['mode'] == "hostap") {
		exec("/sbin/ifconfig -v " . $ifinfo['if'] . " list sta", $aslist);
		
		$ifinfo['aslist'] = array();
		array_shift($aslist);
		foreach ($aslist as $as) {
			if ($as) {
				$asa = preg_split("/\s+/", $as);
				$aslent = array();
				$aslent['mac'] = $asa[0];
				$aslent['rate'] = str_replace("M", " Mbps", $asa[3]);
				$aslent['rssi'] = $asa[4];
				$aslent['caps'] = $asa[8];
				$aslent['flags'] = $asa[9];
				$ifinfo['aslist'][] = $aslent;
			}
		}
	}
	
	return $ifinfo;
}

?>
<?php include("fbegin.inc"); ?>
<?php $i = 0; $ifdescrs = array();

	if (is_array($config['interfaces']['wan']['wireless']))
			$ifdescrs['wan'] = 'WAN';
			
	if (is_array($config['interfaces']['lan']['wireless']))
			$ifdescrs['lan'] = 'LAN';
	
	for ($j = 1; isset($config['interfaces']['opt' . $j]); $j++) {
		if (is_array($config['interfaces']['opt' . $j]['wireless']) &&
			isset($config['interfaces']['opt' . $j]['enable']))
			$ifdescrs['opt' . $j] = $config['interfaces']['opt' . $j]['descr'];
	}
		
	if (count($ifdescrs) > 0): ?>
            <table width="100%" border="0" cellspacing="0" cellpadding="0">
              <?php
			      foreach ($ifdescrs as $ifdescr => $ifname): 
				  $ifinfo = get_wireless_info($ifdescr);
			  ?>
              <?php if ($i): ?>
              <tr> 
                <td colspan="8" class="list" height="12"></td>
              </tr>
              <?php endif; ?>
              <tr> 
                <td colspan="2" class="listtopic"> 
                  <?=htmlspecialchars($ifname);?> interface (SSID &quot;<?=htmlspecialchars($config['interfaces'][$ifdescr]['wireless']['ssid']);?>&quot;)</td>
              </tr>
              <?php if (isset($ifinfo['scanlist'])): ?>
              <tr> 
                <td width="22%" valign="top" class="vncellt">Last scan results</td>
                <td width="78%" class="listrpad"> 
                  <table width="100%" border="0" cellpadding="0" cellspacing="0">
                    <tr> 
                      <td width="35%" class="listhdrr">SSID</td>
                      <td width="25%" class="listhdrr">BSSID</td>
                      <td width="10%" class="listhdrr">Channel</td>
                      <td width="10%" class="listhdrr">Rate</td>
                      <td width="10%" class="listhdrr">Signal</td>
                      <td width="10%" class="listhdrr">Noise</td>
                    </tr>
                    <?php foreach ($ifinfo['scanlist'] as $ss): ?>
                    <tr> 
                      <td class="listlr" nowrap>
                        <?php if (!$ss['ssid']) echo "<span class=\"gray\">(hidden)</span>"; else echo htmlspecialchars($ss['ssid']);?>
                        <?php if (strpos($ss['caps'][0], "E") !== false): ?>
                        <img src="lock.gif" width="7" height="9">
                        <?php endif; ?>
                      </td>
                      <td class="listr"> 
                        <?=htmlspecialchars($ss['bssid']);?>
                      </td>
                      <td class="listr"> 
                        <?=htmlspecialchars($ss['channel']);?>
                      </td>
                      <td class="listr"> 
                        <?=htmlspecialchars($ss['rate']);?>
                      </td>
                      <td class="listr"> 
                        <?=htmlspecialchars($ss['sig']);?>
                      </td>
                      <td class="listr"> 
                        <?=htmlspecialchars($ss['noise']);?>
                      </td>
                    </tr>
                    <?php endforeach; ?>
                  </table></td>
              </tr><?php endif; ?><?php if (isset($ifinfo['aslist'])): ?>
              <tr> 
                <td width="22%" valign="top" class="vncellt">Associated stations 
                </td>
                <td width="78%" class="listrpad"> 
                  <?php if (count($ifinfo['aslist']) > 0): ?>
                  <table width="100%" border="0" cellpadding="0" cellspacing="0">
                    <tr> 
                      <td width="30%" class="listhdrr">MAC address</td>
                      <td width="15%" class="listhdrr">Rate</td>
                      <td width="15%" class="listhdrr">RSSI</td>
                      <td width="20%" class="listhdrr">Flags</td>
                      <td width="20%" class="listhdrr">Capabilities</td>
                    </tr>
                    <?php foreach ($ifinfo['aslist'] as $as): ?>
                    <tr> 
                      <td class="listlr"> 
                        <?=htmlspecialchars($as['mac']);?>
                      </td>
                      <td class="listr"> 
                        <?=htmlspecialchars($as['rate']);?>
                      </td>
                      <td class="listr"> 
                        <?=htmlspecialchars($as['rssi']);?>
                      </td>
                      <td class="listr"> 
                        <?=htmlspecialchars($as['flags']);?>
                      </td>
                      <td class="listr"> 
                        <?=htmlspecialchars($as['caps']);?>
                      </td>
                    </tr>
                    <?php endforeach; ?>
                  </table><br>
                  Flags: A = authorized, E = Extended Rate (802.11g), P = Power save mode<br>
                  Capabilities: E = ESS (infrastructure mode), I = IBSS (ad-hoc mode), P = privacy (WEP/TKIP/AES),
                  	S = Short preamble, s = Short slot time
                  <?php else: ?>
                  No stations are associated at this time.
                  <?php endif; ?>
                  </td>
              </tr><?php endif; ?>
              <?php $i++; endforeach; ?>
            </table>
<?php else: ?>
<strong>No supported wireless interfaces were found for status display.</strong>
<?php endif; ?>
<?php include("fend.inc"); ?>
