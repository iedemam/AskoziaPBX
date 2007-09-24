#!/usr/local/bin/php
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

$pgtitle = array("Interfaces", "Network");
require("guiconfig.inc");

$lancfg = &$config['interfaces']['lan'];

$pconfig['if'] = $lancfg['if'];
$pconfig['bridge'] = $lancfg['bridge'];
$pconfig['ipaddr'] = $lancfg['ipaddr'];
$pconfig['subnet'] = $lancfg['subnet'];
$pconfig['gateway'] = $lancfg['gateway'];

list($pconfig['dns1'],$pconfig['dns2'],$pconfig['dns3']) = $config['system']['dnsserver'];

$pconfig['topology'] = $lancfg['topology'];
$pconfig['extipaddr'] = $lancfg['extipaddr'];
$pconfig['exthostname'] = $lancfg['exthostname'];
$pconfig['spoofmac'] = $lancfg['spoofmac'];

/* get list without VLAN interfaces */
$networkinterfaces = network_get_interfaces();


if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;

	$reqdfields = explode(" ", "ipaddr subnet gateway topology");
	$reqdfieldsn = explode(",", "IP address,Subnet bit count,Gateway,Network topology");
	do_input_validation($_POST, $reqdfields, $reqdfieldsn, &$input_errors);
	
	$_POST['spoofmac'] = str_replace("-", ":", $_POST['spoofmac']);
	if (($_POST['spoofmac'] && !is_macaddr($_POST['spoofmac']))) {
		$input_errors[] = "A valid MAC address must be specified.";
	}
	
	if (($_POST['ipaddr'] && !is_ipaddr($_POST['ipaddr']))) {
		$input_errors[] = "A valid IP address must be specified.";
	}
	if (($_POST['subnet'] && !is_numeric($_POST['subnet']))) {
		$input_errors[] = "A valid subnet bit count must be specified.";
	}
	if (($_POST['gateway'] && !is_ipaddr($_POST['gateway']))) {
		$input_errors[] = "A valid gateway must be specified.";
	}
	if (($_POST['dns1'] && !is_ipaddr($_POST['dns1'])) || ($_POST['dns2'] && !is_ipaddr($_POST['dns2'])) || ($_POST['dns3'] && !is_ipaddr($_POST['dns3']))) {
		$input_errors[] = "A valid IP address must be specified for the primary/secondary/tertiary DNS server.";
	}
	
	if ($_POST['topology'] == "natstatic") {
		if (!$_POST['extipaddr']) {
			$input_errors[] = "A public IP address must be entered for this topology.";
		} else if (!is_ipaddr($_POST['extipaddr'])) {
			$input_errors[] = "A valid public IP address must be entered for this topology.";
		}
	}
	
	if ($_POST['topology'] == "natdynamichost") {
		if (!$_POST['exthostname']) {
			$input_errors[] = "A public hostname must be entered for this topology.";
		} else if (!is_domain($_POST['exthostname'])) {
			$input_errors[] = "A valid public hostname must be entered for this topology.";
		}
	}
	
	
	if (!$input_errors) {
	
		if ($pconfig['if'] != $lancfg['if']) {
			$lancfg['if'] = $pconfig['if'];
			touch($d_sysrebootreqd_path);
		}
		
		unset($lancfg['bridge']);
		$lancfg['bridge'] = array();
		$pconfig['bridge'] = array();
		$keys = array_keys($_POST);
		foreach ($networkinterfaces as $ifname => $ifinfo) {
			if (in_array($ifname, $keys) && ($ifname != $pconfig['if'])) {
				$lancfg['bridge'][] = $ifname;
				$pconfig['bridge'][] = $ifname;
			}
		}
	
		$lancfg['ipaddr'] = $_POST['ipaddr'];
		$lancfg['subnet'] = $_POST['subnet'];
		$lancfg['gateway'] = $_POST['gateway'];
		
		unset($config['system']['dnsserver']);
		if ($_POST['dns1'])
			$config['system']['dnsserver'][] = $_POST['dns1'];
		if ($_POST['dns2'])
			$config['system']['dnsserver'][] = $_POST['dns2'];
		if ($_POST['dns3'])
			$config['system']['dnsserver'][] = $_POST['dns3'];	

		$lancfg['topology'] = $_POST['topology'];
		$lancfg['extipaddr'] = $_POST['extipaddr'];
		$lancfg['exthostname'] = $_POST['exthostname'];
		$lancfg['spoofmac'] = $_POST['spoofmac'];
		
		write_config();
		
		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval = network_lan_configure();
			$retval |= asterisk_configure();
			$retval |= system_resolvconf_generate();			
			config_unlock();
		}
		$savemsg = get_std_save_message($retval);
	}
}
?>
<?php include("fbegin.inc"); ?>
<script language="JavaScript">
<!--
function type_change() {
	switch (document.iform.topology.selectedIndex) {
		case 0:
			document.iform.extipaddr.disabled = 1;
			document.iform.exthostname.disabled = 1;	
			break;
		case 1:
			document.iform.extipaddr.disabled = 0;
			document.iform.exthostname.disabled = 1;	
			break;
		case 2:
			document.iform.extipaddr.disabled = 1;
			document.iform.exthostname.disabled = 0;	
			break;
	}
}

function lan_if_change() {
	switch (document.iform.if.value) {
		<? foreach ($networkinterfaces as $ifname => $ifinfo): ?>
		case "<?=$ifname;?>":
			<? foreach ($networkinterfaces as $i_ifname => $i_ifinfo): ?>
			document.iform.<?=$i_ifname;?>.disabled = 0;
			document.iform.<?=$i_ifname;?>.checked = 0;
			<? endforeach; ?>
			document.iform.<?=$ifname;?>.disabled = 1;	
			break;
		<? endforeach; ?>
	}
}
//-->
</script>
<form action="interfaces_network.php" method="post" name="iform" id="iform">
<?php if ($input_errors) print_input_errors($input_errors); ?>
<?php if ($savemsg) print_info_box($savemsg); ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav"><?

			$tabs = array(
				'Network'	=> 'interfaces_network.php',
				//'Wireless'	=> 'interfaces_wireless.php',
				'ISDN'		=> 'interfaces_isdn.php',
				'Analog'	=> 'interfaces_analog.php',
				//'Storage'	=> 'interfaces_storage.php'
			);
			dynamic_tab_menu($tabs);
			
			?></ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<table width="100%" border="0" cellpadding="6" cellspacing="0">
				<tr> 
					<td width="22%" valign="top" class="vncellreq">LAN</td>
					<td width="78%" class="vtable">
						<select name="if" class="formfld" id="if" onchange="lan_if_change()">
						<? foreach ($networkinterfaces as $mainifname => $mainifinfo): ?>
							<option value="<?=$mainifname;?>" <? if ($mainifname == $pconfig['if']) echo "selected";?>> 
						<? echo htmlspecialchars($mainifname . " (" . $mainifinfo['mac'] . ")"); ?></option>
						<? endforeach; ?>
						</select>
						&nbsp;&nbsp;
						<? foreach ($networkinterfaces as $ifname => $ifinfo): ?>
							<input name="<?=$ifname;?>" id="<?=$ifname;?>" type="checkbox" value="yes"<?
							if (in_array($ifname, $pconfig['bridge'])) 
								echo "checked";
							else if ($ifname == $mainifname)
								echo "disabled";
							?>><?=$ifname;?>&nbsp;&nbsp;
						<? endforeach; ?>
						<? if (count($networkinterfaces) >= 2): ?>
						<br>
						<span class="vexpl">Checked interfaces will be bridged to the main interface.</span>
						<? endif; ?>
					</td>
				</tr>
				<tr> 
					<td width="22%" valign="top" class="vncellreq">IP address</td>
					<td width="78%" class="vtable"><?=$mandfldhtml;?><input name="ipaddr" type="text" class="formfld" id="ipaddr" size="20" value="<?=htmlspecialchars($pconfig['ipaddr']);?>">
						/ 
						<select name="subnet" class="formfld" id="subnet"><?

							$snmax = 31;
							for ($i = $snmax; $i > 0; $i--): ?>
								<option value="<?=$i;?>" <? if ($i == $pconfig['subnet']) echo "selected"; ?>> 
								<?=$i;?>
								</option>
							<? endfor; ?>
						</select>
					</td>
				</tr>
				<tr> 
					<td width="22%" valign="top" class="vncellreq">Gateway</td>
					<td width="78%" class="vtable"><?=$mandfldhtml;?><input name="gateway" type="text" class="formfld" id="gateway" size="20" value="<?=htmlspecialchars($pconfig['gateway']);?>"> 
					</td>
				</tr>
				<tr> 
					<td width="22%" valign="top" class="vncell">DNS servers</td>
					<td width="78%" class="vtable">
						<input name="dns1" type="text" class="formfld" id="dns1" size="20" value="<?=htmlspecialchars($pconfig['dns1']);?>">
						<br>
						<input name="dns2" type="text" class="formfld" id="dns2" size="20" value="<?=htmlspecialchars($pconfig['dns2']);?>">
						<br>
						<input name="dns3" type="text" class="formfld" id="dns3" size="20" value="<?=htmlspecialchars($pconfig['dns3']);?>">
						<br>
						<span class="vexpl">IP addresses</span>
					</td>
				</tr>
				<tr> 
					<td valign="top" class="vncell">MAC address</td>
					<td class="vtable">
						<input name="spoofmac" type="text" class="formfld" id="spoofmac" size="30" value="<?=htmlspecialchars($pconfig['spoofmac']);?>"><br>
						This field can be used to modify (&quot;spoof&quot;) the MAC address of the network interface<br>
						Enter a MAC address in the following format: xx:xx:xx:xx:xx:xx or leave blank
					</td>
                </tr>
				<tr> 
					<td width="22%" valign="top" class="vncellreq">Topology</td>
					<td width="78%" class="vtable">
						<select name="topology" class="formfld" id="topology" onchange="type_change()">
							<? foreach ($topologies as $topo => $tfriendly): ?>
							<option value="<?=$topo;?>" <? if ($topo == $pconfig['topology']) echo "selected"; ?>><?=$tfriendly;?></option>
							<? endforeach; ?>
						</select>
						<br>
						<span class="vexpl">
							<ul>
								<li>Public IP Address: this pbx has a routable IP address</li>
								<li>NAT + static public IP: this pbx is behind a NAT which has a static public IP. Enter this IP below.</li>
								<li>NAT + dynamic public IP: this pbx is behind a NAT which has a dynamic public IP. A hostname, constantly updated to point to this network is required. Enter this hostname below.</li>
							</ul>
						</span>
					</td>
				</tr>	
				<tr> 
					<td width="22%" valign="top" class="vncell">Public IP address</td>
					<td width="78%" class="vtable">
						<input name="extipaddr" type="text" class="formfld" id="extipaddr" size="20" value="<?=htmlspecialchars($pconfig['extipaddr']);?>">
					</td>
				</tr>
				<tr>
					<td width="22%" valign="top" class="vncell">Public hostname</td>
					<td width="78%" class="vtable">
						<input name="exthostname" type="text" class="formfld" id="exthostname" size="20" value="<?=htmlspecialchars($pconfig['exthostname']);?>">
					</td>
				</tr>
				<tr> 
					<td width="22%" valign="top">&nbsp;</td>
					<td width="78%"> 
						<input name="Submit" type="submit" class="formbtn" value="Save"> 
					</td>
				</tr>
				<tr> 
					<td width="22%" valign="top">&nbsp;</td>
					<td width="78%">
						<span class="vexpl"><span class="red"><strong>Warning:<br>
						</strong></span>after you click &quot;Save&quot;, all current
						calls will be dropped. You may also have to do one or more 
						of the following steps before you can access your PBX again: 
						<ul>
							<li>restart the PBX</li>
							<li>change the IP address of your computer</li>
							<li>access the webGUI with the new IP address</li>
						</ul>
						</span>
					</td>
				</tr>
			</table>
		</td>
	</tr>
</table>
</form>
<script language="JavaScript">
<!--
type_change();
//-->
</script>
<?php include("fend.inc"); ?>
