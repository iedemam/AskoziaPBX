#!/usr/bin/php
<?php 
/*
	$Id: services_dyndns.php 238 2008-01-21 18:33:33Z mkasper $
	part of m0n0wall (http://m0n0.ch/wall)
	
	Copyright (C) 2003-2007 Manuel Kasper <mk@neon1.net>.
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

$pgtitle = array("Services", "Dynamic DNS");

if (!is_array($config['dnsupdate'])) {
	$config['dnsupdate'] = array();
}

$pconfig['username'] = $config['dyndns']['username'];
$pconfig['password'] = $config['dyndns']['password'];
$pconfig['host'] = $config['dyndns']['host'];
$pconfig['server'] = $config['dyndns']['server'];
$pconfig['port'] = $config['dyndns']['port'];
$pconfig['mx'] = $config['dyndns']['mx'];
$pconfig['type'] = $config['dyndns']['type'];
$pconfig['enable'] = isset($config['dyndns']['enable']);
$pconfig['wildcard'] = isset($config['dyndns']['wildcard']);

$pconfig['dnsupdate_enable'] = isset($config['dnsupdate']['enable']);
$pconfig['dnsupdate_host'] = $config['dnsupdate']['host'];
$pconfig['dnsupdate_ttl'] = $config['dnsupdate']['ttl'];
if (!$pconfig['dnsupdate_ttl'])
	$pconfig['dnsupdate_ttl'] = 60;
$pconfig['dnsupdate_keydata'] = $config['dnsupdate']['keydata'];
$pconfig['dnsupdate_keyname'] = $config['dnsupdate']['keyname'];
$pconfig['dnsupdate_usetcp'] = isset($config['dnsupdate']['usetcp']);

if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;

	/* input validation */
	$reqdfields = array();
	$reqdfieldsn = array();
	if ($_POST['enable']) {
		$reqdfields = array_merge($reqdfields, explode(" ", "host username password type"));
		$reqdfieldsn = array_merge($reqdfieldsn, explode(",", "Hostname,Username,Password,Service type"));
	}
	if ($_POST['dnsupdate_enable']) {
		$reqdfields = array_merge($reqdfields, explode(" ", "dnsupdate_host dnsupdate_ttl dnsupdate_keyname dnsupdate_keydata"));
		$reqdfieldsn = array_merge($reqdfieldsn, explode(",", "Hostname,TTL,Key name,Key"));
	}
	verify_input($_POST, $reqdfields, $reqdfieldsn, &$input_errors);
	
	if (($_POST['host'] && !verify_is_domain($_POST['host']))) {
		$input_errors[] = "The host name contains invalid characters.";
	}
	if (($_POST['server'] && !verify_is_domain($_POST['server']) && !verify_is_ipaddress($_POST['server']))) {
		$input_errors[] = "The server name contains invalid characters.";
	}
	if (($_POST['port'] && !verify_is_port($_POST['port']))) {
		$input_errors[] = "The server port must be an integer between 1 and 65535.";
	}
	if (($_POST['mx'] && !verify_is_domain($_POST['mx']))) {
		$input_errors[] = "The MX contains invalid characters.";
	}
	if (($_POST['username'] && !verify_is_dyndns_username($_POST['username']))) {
		$input_errors[] = "The username contains invalid characters.";
	}	
	if (($_POST['dnsupdate_host'] && !verify_is_domain($_POST['dnsupdate_host']))) {
		$input_errors[] = "The DNS update host name contains invalid characters.";
	}
	if (($_POST['dnsupdate_ttl'] && !verify_is_numericint($_POST['dnsupdate_ttl']))) {
		$input_errors[] = "The DNS update TTL must be an integer.";
	}
	if (($_POST['dnsupdate_keyname'] && !verify_is_domain($_POST['dnsupdate_keyname']))) {
		$input_errors[] = "The DNS update key name contains invalid characters.";
	}

	if (!$input_errors) {
		$config['dyndns']['type'] = $_POST['type'];	
		$config['dyndns']['username'] = $_POST['username'];
		$config['dyndns']['password'] = $_POST['password'];
		$config['dyndns']['host'] = $_POST['host'];
		$config['dyndns']['server'] = $_POST['server'];
		$config['dyndns']['port'] = $_POST['port'];
		$config['dyndns']['mx'] = $_POST['mx'];
		$config['dyndns']['wildcard'] = $_POST['wildcard'] ? true : false;
		$config['dyndns']['enable'] = $_POST['enable'] ? true : false;
		
		$config['dnsupdate']['enable'] = $_POST['dnsupdate_enable'] ? true : false;
		$config['dnsupdate']['host'] = $_POST['dnsupdate_host'];
		$config['dnsupdate']['ttl'] = $_POST['dnsupdate_ttl'];
		$config['dnsupdate']['keyname'] = $_POST['dnsupdate_keyname'];
		$config['dnsupdate']['keydata'] = $_POST['dnsupdate_keydata'];
		$config['dnsupdate']['usetcp'] = $_POST['dnsupdate_usetcp'] ? true : false;
			
		write_config();
		
		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			/* nuke the cache file */
			config_lock();
			services_dyndns_reset();
			$retval = services_dyndns_configure();
			$retval |= services_dnsupdate_process();
			config_unlock();
		}
		$savemsg = get_std_save_message($retval);
	}
}
?>
<?php include("fbegin.inc"); ?>
<script type="text/javascript">
<!--
function enable_change(enable_change) {
	var endis;
	
	endis = !(document.iform.enable.checked || enable_change);
	document.iform.host.disabled = endis;
	document.iform.server.disabled = endis;
	document.iform.port.disabled = endis;
	document.iform.mx.disabled = endis;
	document.iform.type.disabled = endis;
	document.iform.wildcard.disabled = endis;
	document.iform.username.disabled = endis;
	document.iform.password.disabled = endis;
	
	endis = !(document.iform.dnsupdate_enable.checked || enable_change);
	document.iform.dnsupdate_host.disabled = endis;
	document.iform.dnsupdate_ttl.disabled = endis;
	document.iform.dnsupdate_keyname.disabled = endis;
	document.iform.dnsupdate_keydata.disabled = endis;
	document.iform.dnsupdate_usetcp.disabled = endis;
}
//-->
</script>
            <form action="services_dyndns.php" method="post" name="iform" id="iform">
              <table width="100%" border="0" cellpadding="6" cellspacing="0" summary="content pane">
                <tr> 
                  <td colspan="2" valign="top" class="optsect_t">
				  <table border="0" cellspacing="0" cellpadding="0" width="100%" summary="checkbox pane">
				  <tr><td class="optsect_s"><strong>Dynamic DNS client</strong></td>
				  <td align="right" class="optsect_s"><input name="enable" type="checkbox" value="yes" <?php if ($pconfig['enable']) echo "checked"; ?> onClick="enable_change(false)"> <strong>Enable</strong></td></tr>
				  </table></td>
                </tr>
                <tr> 
                  <td width="22%" valign="top" class="vncellreq">Service type</td>
                  <td width="78%" class="vtable">
					<select name="type" class="formfld" id="type">
                      <?php $types = explode(",", "DynDNS,DHS,ODS,DyNS,HN.ORG,ZoneEdit,GNUDip,DynDNS (static),DynDNS (custom),easyDNS,EZ-IP,TZO");
					        $vals = explode(" ", "dyndns dhs ods dyns hn zoneedit gnudip dyndns-static dyndns-custom easydns ezip tzo");
					  $j = 0; for ($j = 0; $j < count($vals); $j++): ?>
                      <option value="<?=$vals[$j];?>" <?php if ($vals[$j] == $pconfig['type']) echo "selected";?>> 
                      <?=htmlspecialchars($types[$j]);?>
                      </option>
                      <?php endfor; ?>
                    </select></td>
				</tr>
                <tr> 
                  <td width="22%" valign="top" class="vncellreq">Hostname</td>
                  <td width="78%" class="vtable"> 
                    <?=$mandfldhtml;?><input name="host" type="text" class="formfld" id="host" size="30" value="<?=htmlspecialchars($pconfig['host']);?>"> 
                  </td>
				</tr>
                 <tr> 
                  <td width="22%" valign="top" class="vncell">Server</td>
                  <td width="78%" class="vtable"> 
                    <input name="server" type="text" class="formfld" id="server" size="30" value="<?=htmlspecialchars($pconfig['server']);?>">
                    <br>Special server to connect to. This can usually be left blank.</td>
				</tr>
                <tr> 
                <tr> 
                  <td width="22%" valign="top" class="vncell">Port</td>
                  <td width="78%" class="vtable"> 
                    <input name="port" type="text" class="formfld" id="port" size="5" value="<?=htmlspecialchars($pconfig['port']);?>">
                    <br>Special server port to connect to. This can usually be left blank.</td>
				</tr>
                <tr> 
                  <td width="22%" valign="top" class="vncell">MX</td>
                  <td width="78%" class="vtable"> 
                    <input name="mx" type="text" class="formfld" id="mx" size="30" value="<?=htmlspecialchars($pconfig['mx']);?>"> 
                    <br>
                    Set this option only if you need a special MX record. Not 
                    all services support this.</td>
				</tr>
                <tr> 
                  <td width="22%" valign="top" class="vncellreq">Wildcards</td>
                  <td width="78%" class="vtable"> 
                    <input name="wildcard" type="checkbox" id="wildcard" value="yes" <?php if ($pconfig['wildcard']) echo "checked"; ?>>
                    Enable Wildcard</td>
				</tr>
                <tr> 
                  <td width="22%" valign="top" class="vncellreq">Username</td>
                  <td width="78%" class="vtable"> 
                    <?=$mandfldhtml;?><input name="username" type="text" class="formfld" id="username" size="20" value="<?=htmlspecialchars($pconfig['username']);?>"> 
                  </td>
                </tr>
                <tr> 
                  <td width="22%" valign="top" class="vncellreq">Password</td>
                  <td width="78%" class="vtable"> 
                    <?=$mandfldhtml;?><input name="password" type="password" class="formfld" id="password" size="20" value="<?=htmlspecialchars($pconfig['password']);?>"> 
                  </td>
                </tr>

<? /* XXX -this stuff is not yet implemented-
                <tr> 
                  <td colspan="2" class="list" height="12"></td>
                </tr>
                <tr> 
                  <td colspan="2" valign="top" class="optsect_t">
				  <table border="0" cellspacing="0" cellpadding="0" width="100%" summary="checkbox pane">
				  <tr>
				    <td class="optsect_s"><strong>RFC 2136 Dynamic DNS updates</strong></td>
				    <td align="right" class="optsect_s"><input name="dnsupdate_enable" type="checkbox" value="yes" <?php if ($pconfig['dnsupdate_enable']) echo "checked"; ?> onClick="enable_change(false)"> <strong>Enable</strong></td></tr>
				  </table></td>
                </tr>
                <tr> 
                  <td width="22%" valign="top" class="vncellreq">Hostname</td>
                  <td width="78%" class="vtable"> 
                    <?=$mandfldhtml;?><input name="dnsupdate_host" type="text" class="formfld" id="dnsupdate_host" size="30" value="<?=htmlspecialchars($pconfig['dnsupdate_host']);?>"> 
                  </td>
				</tr>
                <tr>
                  <td valign="top" class="vncellreq">TTL</td>
                  <td class="vtable">
                    <?=$mandfldhtml;?><input name="dnsupdate_ttl" type="text" class="formfld" id="dnsupdate_ttl" size="6" value="<?=htmlspecialchars($pconfig['dnsupdate_ttl']);?>"> 
                  seconds</td>
                </tr>
                <tr>
                  <td valign="top" class="vncellreq">Key name</td>
                  <td class="vtable">
                    <?=$mandfldhtml;?><input name="dnsupdate_keyname" type="text" class="formfld" id="dnsupdate_keyname" size="30" value="<?=htmlspecialchars($pconfig['dnsupdate_keyname']);?>">
                    <br> 
                    This must match the setting on the DNS server.</td>
                </tr>
                <tr>
                  <td valign="top" class="vncellreq">Key</td>
                  <td class="vtable">
                    <?=$mandfldhtml;?><input name="dnsupdate_keydata" type="text" class="formfld" id="dnsupdate_keydata" size="70" value="<?=htmlspecialchars($pconfig['dnsupdate_keydata']);?>">
                    <br> 
                    Paste an HMAC-MD5 key here.</td>
                </tr>
                <tr> 
                  <td width="22%" valign="top" class="vncellreq">Protocol</td>
                  <td width="78%" class="vtable"> 
                    <input name="dnsupdate_usetcp" type="checkbox" id="dnsupdate_usetcp" value="yes" <?php if ($pconfig['dnsupdate_usetcp']) echo "checked"; ?>>
                    <strong>Use TCP instead of UDP</strong></td>
				</tr>
*/?>

                <tr> 
                  <td width="22%" valign="top">&nbsp;</td>
                  <td width="78%"> 
                    <input name="Submit" type="submit" class="formbtn" value="Save" onClick="enable_change(true)"> 
                  </td>
                </tr>
              </table>
</form>
<script type="text/javascript">
<!--
enable_change(false);
//-->
</script>
<?php include("fend.inc"); ?>
