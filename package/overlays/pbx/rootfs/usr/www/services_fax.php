#!/usr/bin/php
<?php 
/*
	$Id: services_fax.php 1515 2010-04-30 11:38:34Z michael.iedema $
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2008 tecema (a.k.a IKT) <http://www.tecema.de>.
	All rights reserved.
	
	Askozia®PBX is a registered trademark of tecema. Any unauthorized use of
	this trademark is prohibited by law and international treaties.
	
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
	
	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.
	
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	
	3. Redistribution in any form at a charge, that in whole or in part
	   contains or is derived from the software, including but not limited to
	   value added products, is prohibited without prior written consent of
	   tecema.
	
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

$pgtitle = array(gettext("Services"), gettext("FAX"));
$pghelp = gettext("FAX-Receiver Description");
$pglegend = array("add", "edit", "delete");

if (!is_array($config['fax']['machine']))
	$config['fax']['machine'] = array();

//fax_sorting();
$a_faxs = &$config['fax']['machine'];

if ($_GET['action'] == "delete") {
	if ($a_faxs[$_GET['id']]) {
		unset($a_faxs[$_GET['id']]);
		write_config();
		touch($d_faxconfdirty_path);
		header("Location: services_fax.php");
		exit;
	}
}

if (file_exists($d_faxconfdirty_path)) {
	$retval = 0;
	config_lock();
	// $retval |= fax_conf_generate();
	$retval |= extensions_conf_generate();
	config_unlock();
	
	$retval |= pbx_exec("dialplan reload");

	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($d_faxconfdirty_path);
	}
}


include("fbegin.inc");
?><form action="services_fax.php" method="post">
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="20%" class="listhdrr"><?=gettext("Fax Number");?></td>
		<td width="55%" class="listhdrr"><?=gettext("Name");?></td>
		<td width="15%" class="listhdrr"><?=gettext("email Receipient");?></td>
		<td width="10%" class="list"></td>
	</tr>

	<?php $i = 0; foreach ($a_faxs as $fax): ?>
	<tr>
		<td class="listlr">
			<?=htmlspecialchars($fax['number']);?>
		</td>
		<td class="listbg">
			<?=htmlspecialchars($fax['name']);?>&nbsp;
		</td>
		<td valign="middle" nowrap class="listr">
			<?=htmlspecialchars($fax['email']);?>&nbsp;
		</td>
		<td valign="middle" nowrap class="list"><a href="services_fax_edit.php?id=<?=$i;?>"><img src="edit.png" title="<?=gettext("edit fax");?>" border="0"></a>
           <a href="?action=delete&id=<?=$i;?>" onclick="return confirm('<?=gettext("Do you really want to delete this fax?");?>')"><img src="delete.png" title="<?=gettext("delete fax");?>" border="0"></a>
		</td>
	</tr>
	<?php $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="3"></td>
		<td class="list"><a href="services_fax_edit.php"><img src="add.png" title="<?=gettext("add fax");?>" border="0"></a></td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
