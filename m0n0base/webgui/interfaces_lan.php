#!/usr/local/bin/php
<?php 
/*
	$Id: interfaces_wan.php 170 2006-11-20 21:24:05Z mkasper $
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

$pgtitle = array("Interfaces", "LAN");
require("guiconfig.inc");

$lancfg = &$config['interfaces']['lan'];
$optcfg = &$config['interfaces']['lan'];

$pconfig['dhcphostname'] = $lancfg['dhcphostname'];

if ($lancfg['ipaddr'] == "dhcp") {
	$pconfig['type'] = "DHCP";
} else {
	$pconfig['type'] = "Static";
	$pconfig['ipaddr'] = $lancfg['ipaddr'];
	$pconfig['subnet'] = $lancfg['subnet'];
	$pconfig['gateway'] = $lancfg['gateway'];
}


if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;

	/* input validation */
	if ($_POST['type'] == "Static") {
		$reqdfields = explode(" ", "ipaddr subnet gateway");
		$reqdfieldsn = explode(",", "IP address,Subnet bit count,Gateway");
		do_input_validation($_POST, $reqdfields, $reqdfieldsn, &$input_errors);
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

	if (!$input_errors) {
	
		unset($lancfg['ipaddr']);
		unset($lancfg['subnet']);
		unset($lancfg['gateway']);
		unset($lancfg['dhcphostname']);
	
		if ($_POST['type'] == "Static") {
			$lancfg['ipaddr'] = $_POST['ipaddr'];
			$lancfg['subnet'] = $_POST['subnet'];
			$lancfg['gateway'] = $_POST['gateway'];

		} else if ($_POST['type'] == "DHCP") {
			$lancfg['ipaddr'] = "dhcp";
			$lancfg['dhcphostname'] = $_POST['dhcphostname'];
		}
		
		write_config();
		
		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			config_lock();
			$retval = interfaces_lan_configure();
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
	switch (document.iform.type.selectedIndex) {
		case 0:
			document.iform.ipaddr.disabled = 0;
			document.iform.subnet.disabled = 0;
			document.iform.gateway.disabled = 0;
			document.iform.dhcphostname.disabled = 1;
			break;
		case 1:
			document.iform.ipaddr.disabled = 1;
			document.iform.subnet.disabled = 1;
			document.iform.gateway.disabled = 1;
			document.iform.dhcphostname.disabled = 0;
			break;
	}
}
//-->
</script>
<?php if ($input_errors) print_input_errors($input_errors); ?>
<?php if ($savemsg) print_info_box($savemsg); ?>
            <form action="interfaces_lan.php" method="post" name="iform" id="iform">
              <table width="100%" border="0" cellpadding="6" cellspacing="0">
                <tr> 
                  <td valign="middle"><strong>Type</strong></td>
                  <td><select name="type" class="formfld" id="type" onchange="type_change()">
                      <?php $opts = split(" ", "Static DHCP");
				foreach ($opts as $opt): ?>
                      <option <?php if ($opt == $pconfig['type']) echo "selected";?>> 
                      <?=htmlspecialchars($opt);?>
                      </option>
                      <?php endforeach; ?>
                    </select></td>
                </tr>
                <tr> 
                  <td colspan="2" valign="top" height="4"></td>
                </tr>
                <tr> 
                  <td colspan="2" valign="top" class="listtopic">Static IP configuration</td>
                </tr>
                <tr> 
                  <td width="100" valign="top" class="vncellreq">IP address</td>
                  <td class="vtable"><?=$mandfldhtml;?><input name="ipaddr" type="text" class="formfld" id="ipaddr" size="20" value="<?=htmlspecialchars($pconfig['ipaddr']);?>">
                    / 
                    <select name="subnet" class="formfld" id="subnet">
                    <?php
                      $snmax = 31;
                      for ($i = $snmax; $i > 0; $i--): ?>
                      <option value="<?=$i;?>" <?php if ($i == $pconfig['subnet']) echo "selected"; ?>> 
                      <?=$i;?>
                      </option>
                      <?php endfor; ?>
                    </select></td>
                </tr>
                <tr> 
                  <td valign="top" class="vncellreq">Gateway</td>
                  <td class="vtable"><?=$mandfldhtml;?><input name="gateway" type="text" class="formfld" id="gateway" size="20" value="<?=htmlspecialchars($pconfig['gateway']);?>"> 
                  </td>
                </tr>
                <tr> 
                  <td colspan="2" valign="top" height="16"></td>
                </tr>
                <tr> 
                  <td colspan="2" valign="top" class="listtopic">DHCP client configuration</td>
                </tr>
                <tr> 
                  <td valign="top" class="vncell">Hostname</td>
                  <td class="vtable"> <input name="dhcphostname" type="text" class="formfld" id="dhcphostname" size="40" value="<?=htmlspecialchars($pconfig['dhcphostname']);?>">
                    <br>
                    The value in this field is sent as the DHCP client identifier 
                    and hostname when requesting a DHCP lease.</td>
                </tr>
                <tr> 
                  <td colspan="2" valign="top" height="16"></td>
                </tr>
                <tr> 
                  <td width="100" valign="top">&nbsp;</td>
                  <td> &nbsp;<br> <input name="Submit" type="submit" class="formbtn" value="Save"> 
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
