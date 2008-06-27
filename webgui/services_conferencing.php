#!/usr/local/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2008 IKT <http://itison-ikt.de>.
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

$pgtitle = array(gettext("Services"), gettext("Conferencing"));
require("guiconfig.inc");


if (!is_array($config['conferencing']['room']))
	$config['conferencing']['room'] = array();

conferencing_sort_rooms();
$a_rooms = &$config['conferencing']['room'];

if ($_GET['action'] == "delete") {
	if ($a_rooms[$_GET['id']]) {
		unset($a_rooms[$_GET['id']]);
		write_config();
		touch($d_conferencingconfdirty_path);
		header("Location: services_conferencing.php");
		exit;
	}
}

if (file_exists($d_conferencingconfdirty_path)) {
	$retval = 0;
	config_lock();
	$retval |= conferencing_conf_generate();
	$retval |= extensions_conf_generate();
	config_unlock();
	
	$retval |= extensions_reload();

	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($d_conferencingconfdirty_path);
	}
}

?>

<?php include("fbegin.inc"); ?>
<form action="services_conferencing.php" method="post">
<?php if ($savemsg) print_info_box($savemsg); ?>

<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td width="20%" class="listhdrr"><?=gettext("Room Number");?></td>
		<td width="55%" class="listhdrr"><?=gettext("Name");?></td>
		<td width="15%" class="listhdrr"><?=gettext("Pin");?></td>
		<td width="10%" class="list"></td>
	</tr>

	<?php $i = 0; foreach ($a_rooms as $room): ?>
	<tr>
		<td class="listlr">
			<?=htmlspecialchars($room['number']);?>
		</td>
		<td class="listbg">
			<?=htmlspecialchars($room['name']);?>&nbsp;
		</td>
		<td valign="middle" nowrap class="listr">
			<?php if ($room['pin']): ?>
			<img src="lock.gif" width="7" height="9" border="0">
			<?php endif; ?>
			&nbsp;
		</td>
		<td valign="middle" nowrap class="list"><a href="services_conferencing_edit.php?id=<?=$i;?>"><img src="edit.png" title="<?=gettext("edit conference room");?>" border="0"></a>
           <a href="?action=delete&id=<?=$i;?>" onclick="return confirm('<?=gettext("Do you really want to delete this conference room?");?>')"><img src="delete.png" title="<?=gettext("delete conference room");?>" border="0"></a>
		</td>
	</tr>
	<?php $i++; endforeach; ?>

	<tr> 
		<td class="list" colspan="3"></td>
		<td class="list"><a href="services_conferencing_edit.php"><img src="add.png" title="<?=gettext("add conference room");?>" border="0"></a></td>
	</tr>
</table>
</form>
<?php include("fend.inc"); ?>
