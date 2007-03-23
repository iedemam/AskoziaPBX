#!/usr/local/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007 IKT <http://itison-ikt.de>.
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

$pgtitle = array("Providers", "SIP", "Edit Account");
require("guiconfig.inc");

if (!is_array($config['providers']['sipprovider']))
	$config['providers']['sipprovider'] = array();

asterisk_sip_sort_providers();
$a_sipproviders = &$config['providers']['sipprovider'];


$id = $_GET['id'];
if (isset($_POST['id']))
	$id = $_POST['id'];

/* pull current config into pconfig */
if (isset($id) && $a_sipproviders[$id]) {
	$pconfig['name'] = $a_sipproviders[$id]['name'];
	$pconfig['username'] = $a_sipproviders[$id]['username'];
	$pconfig['authuser'] = $a_sipproviders[$id]['authuser'];
	$pconfig['secret'] = $a_sipproviders[$id]['secret'];
	$pconfig['host'] = $a_sipproviders[$id]['host'];
	$pconfig['port'] = $a_sipproviders[$id]['port'];
	$pconfig['prefix'] = $a_sipproviders[$id]['prefix'];
	$pconfig['codec'] = $a_sipproviders[$id]['codec'];
	
}

if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;

	/* input validation */
	$reqdfields = explode(" ", "name username host prefix");
	$reqdfieldsn = explode(",", "Name,Username,Host,Prefix");
	
	do_input_validation($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

	if (($_POST['username'] && !asterisk_is_valid_username($_POST['username']))) {
		$input_errors[] = "A valid username must be specified.";
	}
	if (($_POST['secret'] && !asterisk_is_valid_secret($_POST['secret']))) {
		$input_errors[] = "A valid secret must be specified.";
	}
	if (($_POST['host'] && !asterisk_is_valid_host($_POST['host']))) {
		$input_errors[] = "A valid host must be specified.";
	}
	if (!isset($id) && in_array($_POST['prefix'], asterisk_get_prefixes())) {
		$input_errors[] = "A provider with this prefix already exists.";
	} else if (!asterisk_is_valid_prefix($_POST['prefix'])) {
		$input_errors[] = "A valid prefix must be specified.";
	}
	// TODO: more checking on optional fields

	if (!$input_errors) {
		$sp = array();		
		$sp['name'] = $_POST['name'];
		$sp['username'] = $_POST['username'];
		$sp['authuser'] = $_POST['authuser'];
		$sp['secret'] = $_POST['secret'];
		$sp['host'] = $_POST['host'];
		$sp['port'] = $_POST['port'];
		$sp['prefix'] = $_POST['prefix'];
		
		$sp['codec'] = array();
		foreach ($codecs as $codec=>$friendly)
			if($_POST[$codec] == true)
				$sp['codec'][] = $codec;


		if (isset($id) && $a_sipproviders[$id]) {
			$sp['uniqid'] = $a_sipproviders[$id]['uniqid'];
			$a_sipproviders[$id] = $sp;
		 } else {
			$sp['uniqid'] = uniqid(rand());
			$a_sipproviders[] = $sp;
		}
		
		touch($d_sipconfdirty_path);
		
		write_config();
		
		header("Location: providers_sip.php");
		exit;
	}
}
?>
<?php include("fbegin.inc"); ?>
<script language="JavaScript">
<!--
function typesel_change() {

}
//-->
</script>
<?php if ($input_errors) print_input_errors($input_errors); ?>
            <form action="providers_sip_edit.php" method="post" name="iform" id="iform">
              <table width="100%" border="0" cellpadding="6" cellspacing="0">
                <tr> 
                  <td valign="top" class="vncellreq">Name</td>
                  <td class="vtable"><?=$mandfldhtml;?><input name="name" type="text" class="formfld" id="name" size="40" value="<?=htmlspecialchars($pconfig['name']);?>"> 
                    <br> <span class="vexpl">Descriptive name of this provider.</span></td>
                </tr>
                <tr> 
                  <td valign="top" class="vncellreq">Prefix</td>
                  <td class="vtable"><?=$mandfldhtml;?><input name="prefix" type="text" class="formfld" id="prefix" size="40" value="<?=htmlspecialchars($pconfig['prefix']);?>"> 
                    <br> <span class="vexpl">Dialing prefix to access this provider (pattern matching notes here).</span></td>
                </tr>
                <tr> 
                  <td valign="top" class="vncellreq">Username</td>
                  <td class="vtable"><?=$mandfldhtml;?><input name="username" type="text" class="formfld" id="username" size="40" value="<?=htmlspecialchars($pconfig['username']);?>"> 
                    <br> <span class="vexpl">Username</span></td>
                </tr>
                <tr> 
                  <td valign="top" class="vncell">Authuser</td>
                  <td class="vtable"><?=$mandfldhtml;?><input name="authuser" type="text" class="formfld" id="authuser" size="40" value="<?=htmlspecialchars($pconfig['authuser']);?>"> 
                    <br> <span class="vexpl">Some providers require a seperate authorization name.</span></td>
                </tr>
                <tr> 
                  <td valign="top" class="vncell">Secret</td>
                  <td class="vtable"><?=$mandfldhtml;?><input name="secret" type="text" class="formfld" id="secret" size="40" value="<?=htmlspecialchars($pconfig['secret']);?>"> 
                    <br> <span class="vexpl">Secrets may not contain a '#' or ';'.</span></td>
                </tr>
                <tr> 
                  <td valign="top" class="vncellreq">Host</td>
                  <td class="vtable"><?=$mandfldhtml;?><input name="host" type="text" class="formfld" id="host" size="40" value="<?=htmlspecialchars($pconfig['host']);?>"> 
                    <br> <span class="vexpl">SIP proxy host URL or IP address.</span></td>
                </tr>
                <tr> 
                  <td valign="top" class="vncell">Port</td>
                  <td class="vtable"><?=$mandfldhtml;?><input name="port" type="text" class="formfld" id="port" size="20" value="<?=htmlspecialchars($pconfig['port']);?>"> 
                    <br> <span class="vexpl">defaults to 5060</span></td>
                </tr>
                <tr> 
                  <td width="22%" valign="top" class="vncell">Codecs</td>
                  <td width="78%" class="vtable">
				  <? foreach ($codecs as $codec=>$friendly): ?>
					<input name="<?=$codec?>" id="<?=$codec?>" type="checkbox" value="yes" onclick="enable_change(false)" <?php if (in_array($codec, $pconfig['codec'])) echo "checked"; ?>><?=$friendly?><br>
				  <? endforeach; ?>
				</tr>
                <tr> 
                  <td width="22%" valign="top">&nbsp;</td>
                  <td width="78%"> <input name="Submit" type="submit" class="formbtn" value="Save"> 
                    <?php if (isset($id) && $a_sipproviders[$id]): ?>
                    <input name="id" type="hidden" value="<?=$id;?>"> 
                    <?php endif; ?>
                  </td>
                </tr>
              </table>
</form>
<script language="JavaScript">
<!--
typesel_change();
//-->
</script>
<?php include("fend.inc"); ?>
