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

$pconfig['ipaddr'] = $lancfg['ipaddr'];
$pconfig['subnet'] = $lancfg['subnet'];
$pconfig['gateway'] = $lancfg['gateway'];

list($pconfig['dns1'],$pconfig['dns2'],$pconfig['dns3']) = $config['system']['dnsserver'];

$pconfig['topology'] = $lancfg['topology'];
$pconfig['extipaddr'] = $lancfg['extipaddr'];
$pconfig['exthostname'] = $lancfg['exthostname'];


if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;

	$reqdfields = explode(" ", "ipaddr subnet gateway topology");
	$reqdfieldsn = explode(",", "IP address,Subnet bit count,Gateway,Network topology");
	do_input_validation($_POST, $reqdfields, $reqdfieldsn, &$input_errors);
		
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
	if (($_POST['extipaddr'] && !is_ipaddr($_POST['extipaddr']))) {
		$input_errors[] = "A valid external IP address must be specified.";
	}
	
	if (!$input_errors) {
	
		unset($lancfg['ipaddr']);
		unset($lancfg['subnet']);
		unset($lancfg['gateway']);
	
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

		unset($lancfg['topology']);
		$lancfg['topology'] = $_POST['topology'];

		unset($lancfg['extipaddr']);
		if ($_POST['extipaddr'])
			$lancfg['extipaddr'] = $_POST['extipaddr'];
			
		unset($lancfg['exthostname']);
		if ($_POST['exthostname'])
			$lancfg['exthostname'] = $_POST['exthostname'];
		
		write_config();
		
		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval = interfaces_lan_configure();
			$retval |= asterisk_start();
			$retval |= system_resolvconf_generate();			
			config_unlock();
		}
		$savemsg = get_std_save_message($retval);
	}
}
?>
<?php include("fbegin.inc"); ?>
<form action="interfaces_network.php" method="post" name="iform" id="iform">
<?php if ($input_errors) print_input_errors($input_errors); ?>
<?php if ($savemsg) print_info_box($savemsg); ?>

<table width="100%" border="0" cellpadding="6" cellspacing="0">
	<tr> 
		<td colspan="2" valign="top" class="listtopic">LAN configuration</td>
	</tr>
	<tr> 
		<td width="22%" valign="top" class="vncellreq">IP address</td>
		<td width="78%" class="vtable"><?=$mandfldhtml;?><input name="ipaddr" type="text" class="formfld" id="ipaddr" size="20" value="<?=htmlspecialchars($pconfig['ipaddr']);?>">
			/ 
			<select name="subnet" class="formfld" id="subnet">
			<?php
				$snmax = 31;
				for ($i = $snmax; $i > 0; $i--): ?>
					<option value="<?=$i;?>" <?php if ($i == $pconfig['subnet']) echo "selected"; ?>> 
					<?=$i;?>
					</option>
				<?php endfor; ?>
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
		<td width="22%" valign="top" class="vncellreq">Topology</td>
		<td width="78%" class="vtable">
			<select name="topology" class="formfld" id="topology">
				<option></option>
				<?php foreach ($topologies as $topo => $tfriendly): ?>
				<option value="<?=$topo;?>" <?php if ($topo == $pconfig['topology']) echo "selected"; ?>><?=$tfriendly;?></option>
				<?php endforeach; ?>
			</select>
		</td>
	</tr>	
	<tr> 
		<td width="22%" valign="top" class="vncell">External IP address</td>
		<td width="78%" class="vtable">
			<input name="extipaddr" type="text" class="formfld" id="extipaddr" size="20" value="<?=htmlspecialchars($pconfig['extipaddr']);?>">
			<br>
			<span class="vexpl">Static external IP</span>
		</td>
	</tr>
	<tr>
		<td width="22%" valign="top" class="vncell">External hostname</td>
		<td width="78%" class="vtable">
			<input name="exthostname" type="text" class="formfld" id="exthostname" size="20" value="<?=htmlspecialchars($pconfig['exthostname']);?>">
			<br>
			<span class="vexpl">External hostname to lookup dynamic IPs</span>
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
			of the following steps before you can access your pbx again: 
			<ul>
				<li>change the IP address of your computer</li>
				<li>access the webGUI with the new IP address</li>
			</ul>
			</span>
		</td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
