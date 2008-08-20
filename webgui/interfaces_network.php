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

require("guiconfig.inc");

$pgtitle = array(gettext("Interfaces"), gettext("Network"));
$pghelp = gettext("The settings on this page are critical to ensuring connectivity to VoIP Providers. In particular, incorrect Topology settings can prevent incoming calls from connecting. If this system is behind a NAT, by default AskoziaPBX requires the following ports be forwarded to it: 5060 UDP (SIP), 4569 UDP (IAX) and 10000-10200 UDP (RTP Audio)");

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
$pconfig['hostnameupdatesrc'] = $lancfg['hostnameupdatesrc'];

$pconfig['dyndnsusername'] = $config['dyndns']['username'];
$pconfig['dyndnspassword'] = $config['dyndns']['password'];
$pconfig['dyndnstype'] = $config['dyndns']['type'];
$pconfig['dyndnsenable'] = isset($config['dyndns']['enable']);
$pconfig['dyndnswildcard'] = isset($config['dyndns']['wildcard']);

/* get list without VLAN interfaces */
$networkinterfaces = network_get_interfaces();


if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;

	$reqdfields = explode(" ", "ipaddr subnet gateway topology");
	$reqdfieldsn = explode(",", "IP address,Subnet bit count,Gateway,Network topology");
	verify_input($_POST, $reqdfields, $reqdfieldsn, &$input_errors);
	if ($_POST['hostnameupdatesrc'] == "pbx") {
		$reqdfields = array_merge($reqdfields, explode(" ", "dyndnsusername dyndnspassword dyndnstype"));
		$reqdfieldsn = array_merge($reqdfieldsn, explode(",", "Dynamic DNS Username,Dynamic DNS Password,Dynamic DNS Service Type"));
		verify_input($_POST, $reqdfields, $reqdfieldsn, &$input_errors);
	}
	
	$_POST['spoofmac'] = str_replace("-", ":", $_POST['spoofmac']);
	if (($_POST['spoofmac'] && !verify_is_macaddress($_POST['spoofmac']))) {
		$input_errors[] = gettext("A valid MAC address must be specified.");
	}
	
	if (($_POST['ipaddr'] && !verify_is_ipaddress($_POST['ipaddr']))) {
		$input_errors[] = gettext("A valid IP address must be specified.");
	}
	if (($_POST['subnet'] && !is_numeric($_POST['subnet']))) {
		$input_errors[] = gettext("A valid subnet bit count must be specified.");
	}
	if (($_POST['gateway'] && !verify_is_ipaddress($_POST['gateway']))) {
		$input_errors[] = gettext("A valid gateway must be specified.");
	}
	if (($_POST['dns1'] && !verify_is_ipaddress($_POST['dns1'])) || ($_POST['dns2'] && !verify_is_ipaddress($_POST['dns2'])) || ($_POST['dns3'] && !verify_is_ipaddress($_POST['dns3']))) {
		$input_errors[] = gettext("A valid IP address must be specified for the primary/secondary/tertiary DNS server.");
	}
	
	if ($_POST['topology'] == "natstatic") {
		if (!$_POST['extipaddr']) {
			$input_errors[] = gettext("A public IP address must be entered for this topology.");
		} else if (!verify_is_ipaddress($_POST['extipaddr'])) {
			$input_errors[] = gettext("A valid public IP address must be entered for this topology.");
		}
	}
	
	if (($_POST['topology'] == "natdynamichost") && ($_POST['hostnameupdatesrc'] == "router")) {
		if (!$_POST['exthostname']) {
			$input_errors[] = gettext("A public hostname must be entered for this topology.");
		} else if (!verify_is_domain($_POST['exthostname'])) {
			$input_errors[] = gettext("A valid public hostname must be entered for this topology.");
		}
	}

	if ($_POST['hostnameupdatesrc'] == "pbx") {
		if (($_POST['dyndnsusername'] && !verify_is_dyndns_username($_POST['dyndnsusername']))) {
			$input_errors[] = gettext("The dynamic DNS username contains invalid characters.");
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
		$lancfg['hostnameupdatesrc'] = $_POST['hostnameupdatesrc'];

		$config['dyndns']['type'] = $_POST['dyndnstype'];	
		$config['dyndns']['username'] = $_POST['dyndnsusername'];
		$config['dyndns']['password'] = $_POST['dyndnspassword'];
		$config['dyndns']['wildcard'] = $_POST['dyndnswildcard'] ? true : false;
		
		write_config();
		
		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			services_dyndns_reset();
			$retval = network_lan_configure();
			$retval |= pbx_configure();
			$retval |= system_resolvconf_generate();
			$retval |= services_dyndns_configure();
			config_unlock();
		}
		$savemsg = get_std_save_message($retval);
	}
}
?>
<?php include("fbegin.inc"); ?>
<script type="text/JavaScript">
<!--
	<?=javascript_dyndns("functions");?>

	jQuery(document).ready(function(){

		<?=javascript_dyndns("ready");?>
	});

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
<?php if ($input_errors) display_input_errors($input_errors); ?>
<?php if ($savemsg) display_info_box($savemsg); ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav"><?

			$tabs = array(
				gettext('Network')	=> 'interfaces_network.php',
				gettext('Wireless')	=> 'interfaces_wireless.php',
				gettext('ISDN')		=> 'interfaces_isdn.php',
				gettext('Analog')	=> 'interfaces_analog.php',
				gettext('Storage')	=> 'interfaces_storage.php'
			);
			dynamic_tab_menu($tabs);
			
			?></ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont">
			<table width="100%" border="0" cellpadding="6" cellspacing="0">
				<tr> 
					<td width="22%" valign="top" class="vncellreq"><?=gettext("LAN");?></td>
					<td width="78%" class="vtable">
						
						<? if (count($networkinterfaces) == 1): ?>
							<? foreach ($networkinterfaces as $mainifname => $mainifinfo): ?>
							<? echo htmlspecialchars($mainifname . " (" . $mainifinfo['mac'] . ")"); ?>
							<input id="if" name="if" type="hidden" value="<?=$mainifname;?>">
							<? endforeach; ?>
						<? else: ?>
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
								else if ($ifname == $pconfig['if'])
									echo "disabled";
								?>><?=$ifname;?>&nbsp;&nbsp;
							<? endforeach; ?>
							<br>
							<span class="vexpl"><?=gettext("Checked interfaces will be bridged to the main interface.");?></span>
						<? endif; ?>
					</td>
				</tr>
				<tr> 
					<td width="22%" valign="top" class="vncellreq"><?=gettext("IP Address");?></td>
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
					<td width="22%" valign="top" class="vncellreq"><?=gettext("Gateway");?></td>
					<td width="78%" class="vtable"><?=$mandfldhtml;?><input name="gateway" type="text" class="formfld" id="gateway" size="20" value="<?=htmlspecialchars($pconfig['gateway']);?>"> 
					</td>
				</tr>
				<tr> 
					<td width="22%" valign="top" class="vncell"><?=gettext("DNS Servers");?></td>
					<td width="78%" class="vtable">
						<input name="dns1" type="text" class="formfld" id="dns1" size="20" value="<?=htmlspecialchars($pconfig['dns1']);?>">
						<br>
						<input name="dns2" type="text" class="formfld" id="dns2" size="20" value="<?=htmlspecialchars($pconfig['dns2']);?>">
						<br>
						<input name="dns3" type="text" class="formfld" id="dns3" size="20" value="<?=htmlspecialchars($pconfig['dns3']);?>">
						<br>
						<span class="vexpl"><?=gettext("IP Addresses");?></span>
					</td>
				</tr>
				<tr> 
					<td valign="top" class="vncell"><?=gettext("MAC Address");?></td>
					<td class="vtable">
						<input name="spoofmac" type="text" class="formfld" id="spoofmac" size="30" value="<?=htmlspecialchars($pconfig['spoofmac']);?>"><br>
						<?=gettext("This field can be used to modify (&quot;spoof&quot;) the MAC address of the network interface<br>Enter a MAC address in the following format: xx:xx:xx:xx:xx:xx or leave blank");?>
					</td>
                </tr>
				<tr> 
					<td width="22%" valign="top" class="vncellreq"><?=gettext("Topology");?></td>
					<td width="78%" class="vtable">
						<select name="topology" class="formfld" id="topology">
							<? foreach ($topologies as $topo => $tfriendly): ?>
							<option value="<?=$topo;?>" <? if ($topo == $pconfig['topology']) echo "selected"; ?>><?=$tfriendly;?></option>
							<? endforeach; ?>
						</select>
						<br>
						<span class="vexpl">
							<ul>
								<li><?=gettext("Public IP Address: this PBX has a routable IP address (entered above)");?></li>
								<li><?=gettext("NAT + Static Public IP: this PBX is behind a NAT which has a static public IP. Enter this IP below.");?></li>
								<li><?=gettext("NAT + Dynamic Public IP: this PBX is behind a NAT which has a dynamic public IP. A hostname, constantly updated to point to this network is required. Enter this information below.");?></li>
							</ul>
						</span>
					</td>
				</tr>	
				<tr> 
					<td width="22%" valign="top" class="vncell"><?=gettext("Static Public IP");?></td>
					<td width="78%" class="vtable">
						<input name="extipaddr" type="text" class="formfld" id="extipaddr" size="20" value="<?=htmlspecialchars($pconfig['extipaddr']);?>">
					</td>
				</tr>
				<tr>
					<td width="22%" valign="top" class="vncell"><?=gettext("Public Hostname");?></td>
					<td width="78%" class="vtable">
						<input name="exthostname" type="text" class="formfld" id="exthostname" size="20" value="<?=htmlspecialchars($pconfig['exthostname']);?>"><br>
						<br>
						<?=gettext("This information should be updated by:");?><br>
						<input name="hostnameupdatesrc" id="hostnameupdatesrcrouter" type="radio" value="router" 
						<?php if ($pconfig['hostnameupdatesrc'] == "router") echo "checked"; ?>><?=gettext("My Router");?>
						&nbsp;&nbsp;
						<input name="hostnameupdatesrc" id="hostnameupdatesrcpbx" type="radio" value="pbx"
						<?php if ($pconfig['hostnameupdatesrc'] == "pbx") echo "checked"; ?>><?=gettext("AskoziaPBX");?>
					</td>
				</tr>
				<tr> 
					<td class="list" colspan="2" height="12">&nbsp;</td>
				</tr>
			</table>

		<span id="dyndns_wrapper" style="display: none;">
			<table width="100%" border="0" cellpadding="6" cellspacing="0" summary="content pane">
				<tr> 
					<td valign="top" colspan="2" class="listtopic"><?=gettext("Dynamic DNS Client");?></td>
				</tr>
				<tr> 
					<td width="22%" valign="top" class="vncellreq"><?=gettext("Service Type");?></td>
					<td width="78%" class="vtable">
						<select name="dyndnstype" class="formfld" id="type"><?

						$types = explode(",", "DynDNS,DHS,ODS,DyNS,HN.ORG,ZoneEdit,GNUDip,DynDNS (static),DynDNS (custom),easyDNS,EZ-IP,TZO");
						$vals = explode(" ", "dyndns dhs ods dyns hn zoneedit gnudip dyndns-static dyndns-custom easydns ezip tzo");

						for ($j = 0; $j < count($vals); $j++) {
							?><option value="<?=$vals[$j];?>" <?
							if ($vals[$j] == $pconfig['dyndnstype']) {
								echo "selected";
							}
							?>><?=htmlspecialchars($types[$j]);?></option><?
						}

						?></select></td>
				</tr>
				<tr> 
					<td width="22%" valign="top" class="vncellreq"><?=gettext("Username");?></td>
					<td width="78%" class="vtable"> 
						<input name="dyndnsusername" type="text" class="formfld" id="dyndnsusername" size="20" value="<?=htmlspecialchars($pconfig['dyndnsusername']);?>"> 
					</td>
				</tr>
				<tr> 
				  <td width="22%" valign="top" class="vncellreq"><?=gettext("Password");?></td>
				  <td width="78%" class="vtable"> 
				    	<input name="dyndnspassword" type="password" class="formfld" id="dyndnspassword" size="20" value="<?=htmlspecialchars($pconfig['dyndnspassword']);?>"> 
				  </td>
				</tr>
				<tr> 
					<td width="22%" valign="top" class="vncell">Wildcards</td>
					<td width="78%" class="vtable"> 
						<input name="dyndnswildcard" type="checkbox" id="dyndnswildcard" value="yes" <?php if ($pconfig['dyndnswildcard']) echo "checked"; ?>>
						<?=gettext('Yes, alias "*.hostname.domain" to hostname specified above.');?></td>
				</tr>
			</table>
		</span>
			<table width="100%" border="0" cellpadding="6" cellspacing="0">
				<tr> 
					<td width="22%" valign="top">&nbsp;</td>
					<td width="78%"> 
						<input name="Submit" type="submit" class="formbtn" value="<?=gettext("Save");?>"> 
					</td>
				</tr>
				<tr> 
					<td width="22%" valign="top">&nbsp;</td>
					<td width="78%">
						<span class="vexpl"><span class="red"><strong><?=gettext("Warning:");?><br>
						</strong></span><?=gettext("after you click &quot;Save&quot;, all current calls will be dropped. You may also have to do one or more of the following steps before you can access your PBX again:");?> 
						<ul>
							<li><?=gettext("restart the PBX");?></li>
							<li><?=gettext("change the IP address of your computer");?></li>
							<li><?=gettext("access the webGUI with the new IP address");?></li>
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
