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

$pgtitle = array("Interfaces", "ISDN");
require("guiconfig.inc");

/*
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

	
	if (!$input_errors) {
	
		$lancfg['ipaddr'] = $_POST['ipaddr'];
		$lancfg['subnet'] = $_POST['subnet'];
		$lancfg['gateway'] = $_POST['gateway'];
		
		write_config();
		
		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval = interfaces_lan_configure();
			$retval |= asterisk_configure();
			$retval |= system_resolvconf_generate();			
			config_unlock();
		}
		$savemsg = get_std_save_message($retval);
	}
}
*/

$hfc_lines = isdn_get_recognized_interfaces();

?>
<?php include("fbegin.inc"); ?>
<form action="interfaces_isdn.php" method="post" name="iform" id="iform">
<?php if ($input_errors) print_input_errors($input_errors); ?>
<?php if ($savemsg) print_info_box($savemsg); ?>

<table width="100%" border="0" cellpadding="6" cellspacing="0">
	<tr> 
		<td colspan="2" valign="top" class="listtopic">ISDN configuration</td>
	</tr>
	<tr> 
		<td width="20%" valign="top" class="vncellreq">HFC Lines</td>
		<td width="80%" class="vtable"><?
		
		foreach($hfc_lines as $line) {
			echo "$line<br>\n";
		}
		
		?></td>
	</tr>
	<tr> 
		<td valign="top">&nbsp;</td>
		<td> 
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
<script language="JavaScript">
<!--
type_change();
//-->
</script>
<?php include("fend.inc"); ?>
