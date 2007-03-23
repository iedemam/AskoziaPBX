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

/* <extension>142</extension>
<name>Askozia Test Account #2</name>
<secret>askozia</secret>
<descr>Another test account</descr>*/

$pgtitle = array("Phones", "SIP", "Edit Accounts");
require("guiconfig.inc");

/* grab and sort the sip phones in our config */
if (!is_array($config['phones']['sipphone']))
	$config['phones']['sipphone'] = array();

asterisk_sip_sort_phones();
$a_sipphones = &$config['phones']['sipphone'];
$a_providers = asterisk_get_providers();


$id = $_GET['id'];
if (isset($_POST['id']))
	$id = $_POST['id'];

/* pull current config into pconfig */
if (isset($id) && $a_sipphones[$id]) {
	$pconfig['extension'] = $a_sipphones[$id]['extension'];
	$pconfig['callerid'] = $a_sipphones[$id]['callerid'];
	$pconfig['secret'] = $a_sipphones[$id]['secret'];
	$pconfig['provider'] = $a_sipphones[$id]['provider'];
	$pconfig['codec'] = $a_sipphones[$id]['codec'];
	$pconfig['descr'] = $a_sipphones[$id]['descr'];
}

if ($_POST) {

	unset($input_errors);
	$pconfig = $_POST;

	/* input validation */
	$reqdfields = explode(" ", "extension callerid secret");
	$reqdfieldsn = explode(",", "Extension,Caller ID,Secret");
	
	do_input_validation($_POST, $reqdfields, $reqdfieldsn, &$input_errors);

	if (($_POST['extension'] && !asterisk_is_valid_extension($_POST['extension']))) {
		$input_errors[] = "An extension must be four digit number.";
	}
	if (($_POST['callerid'] && !asterisk_is_valid_callerid($_POST['callerid']))) {
		$input_errors[] = "A valid Caller ID must be specified.";
	}
	if (($_POST['secret'] && !asterisk_is_valid_secret($_POST['secret']))) {
		$input_errors[] = "A valid secret must be specified.";
	}
	if (asterisk_extension_exists($_POST['extension'])) {
		$input_errors[] = "A phone with this extension already exists.";
	}

	if (!$input_errors) {
		$sp = array();
		$sp['extension'] = $_POST['extension'];
		$sp['callerid'] = $_POST['callerid'];
		$sp['secret'] = $_POST['secret'];
		$sp['descr'] = $_POST['descr'];

		$sp['provider'] = array();
		foreach ($a_providers as $provider)
			if($_POST[$provider['uniqid']] == true)
				$sp['provider'][] = $provider['uniqid'];
		
		$sp['codec'] = array();
		foreach ($codecs as $codec=>$friendly)
			if($_POST[$codec] == true)
				$sp['codec'][] = $codec;


		if (isset($id) && $a_sipphones[$id])
			$sp['uniqid'] = uniqid();
			$a_sipphones[$id] = $sp;
		else
			$a_sipphones[] = $sp;		
		
		touch($d_sipphonesdirty_path);
		
		write_config();
		
		header("Location: phones_sip.php");
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
            <form action="phones_sip_edit.php" method="post" name="iform" id="iform">
              <table width="100%" border="0" cellpadding="6" cellspacing="0">
                <tr> 
                  <td valign="top" class="vncellreq">Extension</td>
                  <td class="vtable"><?=$mandfldhtml;?><input name="extension" type="text" class="formfld" id="name" size="40" maxlength="4" value="<?=htmlspecialchars($pconfig['extension']);?>"> 
                    <br> <span class="vexpl">Four digits.</span></td>
                </tr>
                <tr> 
                  <td valign="top" class="vncellreq">Caller ID</td>
                  <td class="vtable"><?=$mandfldhtml;?><input name="callerid" type="text" class="formfld" id="name" size="40" value="<?=htmlspecialchars($pconfig['callerid']);?>"> 
                    <br> <span class="vexpl">Text to be displayed for Caller ID.</span></td>
                </tr>
                <tr> 
                  <td valign="top" class="vncellreq">Secret</td>
                  <td class="vtable"><?=$mandfldhtml;?><input name="secret" type="text" class="formfld" id="name" size="40" value="<?=htmlspecialchars($pconfig['secret']);?>"> 
                    <br> <span class="vexpl">Secrets may not contain '#' or ';'</span></td>
                </tr>
				<tr> 
                  <td width="22%" valign="top" class="vncell">Providers</td>
                  <td width="78%" class="vtable">
				  <? foreach ($a_providers as $provider): ?>
					<input name="<?=$provider['uniqid']?>" id="<?=$provider['uniqid'?>" type="checkbox" value="yes" onclick="enable_change(false)" <?php if ($pconfig['provider'][$provider]) echo "checked"; ?>><?=$provider['name']?><br>
				  <? endforeach; ?>
				  </td>
				</tr>
                <tr> 
                  <td width="22%" valign="top" class="vncell">Codecs</td>
                  <td width="78%" class="vtable">
				  <? foreach ($codecs as $codec=>$friendly): ?>
					<input name="<?=$codec?>" id="<?=$codec?>" type="checkbox" value="yes" onclick="enable_change(false)" <?php if ($pconfig['codec'][$codec]) echo "checked"; ?>><?=$friendly?><br>
				  <? endforeach; ?>
				</tr>
                <tr> 
                  <td width="22%" valign="top" class="vncell">Description</td>
                  <td width="78%" class="vtable"> <input name="descr" type="text" class="formfld" id="descr" size="40" value="<?=htmlspecialchars($pconfig['descr']);?>"> 
                    <br> <span class="vexpl">You may enter a description here 
                    for your reference (not parsed).</span></td>
                </tr>
                <tr> 
                  <td width="22%" valign="top">&nbsp;</td>
                  <td width="78%"> <input name="Submit" type="submit" class="formbtn" value="Save"> 
                    <?php if (isset($id) && $a_sipphones[$id]): ?>
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
