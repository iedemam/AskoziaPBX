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

$pgtitle = array("Interfaces", "Storage");
require("guiconfig.inc");

/* package restore */
if ($_POST['restore-submit'] && is_uploaded_file($_FILES['packageulfile']['tmp_name'])) {

	$name = $_POST['packageulname'];
	move_uploaded_file($_FILES['packageulfile']['tmp_name'], $_FILES['packageulfile']['name']);
	/* XXX */
}

/* activate a package */
if ($_GET['action'] == "activate" && isset($_GET['package'])) {

	$name = $_GET['package'];

	packages_create_command_file($d_packageconfdirty_path, $name, "activate");

	header("Location: interfaces_storage.php");
	exit;
}

/* delete a package's configuration and storage data */
if ($_GET['action'] == "del" && isset($_GET['package'])) {

	$name = $_GET['package'];
	$pkg = packages_get_package($name);
	packages_exec_rc($pkg['name'], "deactivate");
	packages_exec_rc($pkg['name'], "apply");
	mwexec("rm -rf " . $pkg['path']);
	header("Location: interfaces_storage.php");
	exit;
}

/* backup a package's data */
if ($_GET['action'] == 'backup' && isset($_GET['package'])) {

	$name = $_GET['package'];
	$pkg = packages_get_package($name);

	if (isset($pkg['active'])) {

		$tgz_filename = "package-$name-" . 
			$config['system']['hostname'] . "." . 
			$config['system']['domain'] . "-" . date("YmdHis") . ".tgz";

		header("Content-Type: application/octet-stream"); 
		header("Content-Disposition: attachment; filename=$tgz_filename");
		passthru("/usr/bin/tar cfz - -C {$pkg['path']} $name");
		exit;
	}
}

/* apply changes */
if (file_exists($d_packageconfdirty_path)) {
	if ($command = packages_read_command_file($d_packageconfdirty_path)) {

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			$retval |= packages_exec_rc($command['name'], $command['command']);
		}
		$savemsg = get_std_save_message($retval);
		if ($retval == 0) {
			unlink($d_packageconfdirty_path);
		}
	}	
}

if (storage_syspart_get_state() == "active") {
	$syspart = storage_syspart_get_info();
	$packages = packages_get_packages();
}

?>

<?php include("fbegin.inc"); ?>
<script type="text/JavaScript">
<!--
	function set_package_restore_name(packagename) {
		jQuery("#packageulname").val(packagename);
		jQuery("#packages-restore-file-name").html(packagename);
	}

//-->
</script>
<?php if ($savemsg) print_info_box($savemsg); ?>
<?php if ($input_errors) print_input_errors($input_errors); ?>
<form action="interfaces_storage.php" method="post" enctype="multipart/form-data">
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav"><?

			$tabs = array(
				'Network'	=> 'interfaces_network.php',
				'Wireless'	=> 'interfaces_wireless.php',
				'ISDN'		=> 'interfaces_isdn.php',
				'Analog'	=> 'interfaces_analog.php',
				'Storage'	=> 'interfaces_storage.php'
			);
			dynamic_tab_menu($tabs);

			?></ul>
		</td>
	</tr>
	<tr>
		<td class="tabcont"><?

		if (!$syspart) {

			?><strong>The system storage media is not large enough to install packages.</strong> A minimum of <?=$defaults['storage']['system-media-minimum-size'];?>MB is required. In the future, external media will be able to be used, but currently packages must be stored on the internal system media.<?

		} else {

			?><table width="100%" border="0" cellpadding="6" cellspacing="0">
				<tr>
					<td colspan="4" class="listtopic">Media</td>
					<td class="list">&nbsp;</td>
				</tr>
				<tr>
					<td width="20%" class="listhdrr">Name</td>
					<td width="25%" class="listhdrr">Capacity</td>
					<td width="15%" class="listhdrr">State</td>
					<td width="30%" class="listhdr">Packages</td>
					<td width="10%" class="list"></td>
				</tr>
				<tr>
					<td class="listbgl">system storage partition</td>
					<td class="listr"><? display_capacity_bar($syspart['size'], $syspart['usage']);?>&nbsp;</td>
					<td class="listr"><?=htmlspecialchars($syspart['state']);?>&nbsp;</td>
					<td class="listr"><?=htmlspecialchars(
						implode(", ", array_keys($syspart['packages'])));?>&nbsp;</td>
					<td valign="middle" nowrap class="list"><? /*
						<a href="interfaces_storage_syspart_edit.php"><img src="e.gif" title="edit system storage partition" width="17" height="17" border="0"></a> */ ?>&nbsp;
					</td>
				</tr>
				<tr> 
					<td class="list" colspan="5" height="12">&nbsp;</td>
				</tr>
				<tr>
					<td colspan="4" class="listtopic">Packages</td>
					<td class="list">&nbsp;</td>
				</tr>
				<tr>
					<td width="20%" class="listhdrr">Name</td>
					<td width="25%" class="listhdrr">Size</td>
					<td width="15%" class="listhdrr">State</td>
					<td width="30%" class="listhdr">Description</td>
					<td width="10%" class="list"></td>
				</tr><?
				
			foreach($packages as $pkg) {

				if (!$pkg['active']) {
					continue;
				}

				?><tr>
					<td class="listbgl"><?=$pkg['name'];?>&nbsp;(<?=$pkg['version'];?>)</td>
					<td class="listr"><?=format_bytes(packages_get_size($pkg['name']));?></td>
					<td class="listr">active</td>
					<td class="listr"><?=htmlspecialchars($pkg['descr']);?></td>
					<td valign="middle" nowrap class="list">
						<table border="0" cellspacing="0" cellpadding="1">
							<tr>
								<td><a href="interfaces_storage.php?action=backup&package=<?=$pkg['name'];?>"><img src="b.gif" title="backup package data" width="17" height="17" border="0"></a></td>
								<td><a href="javascript:{}" onclick="set_package_restore_name('<?=$pkg['name'];?>'); jQuery('#packages-restore-file-upload').slideDown();"><img src="r.gif" title="restore package data" width="17" height="17" border="0"></a></td>
							</tr>
							<tr>
								<td align="center" valign="middle"><?

							if ($ss_info['hasoptions']) {
									?><a href="packages_edit_package.php?name=<?=$pkg['name'];?>"><img src="e.gif" title="edit package settings" width="17" height="17" border="0"></a><?
							}

								?></td>
								<td><a href="interfaces_storage.php?action=del&package=<?=$pkg['name'];?>" onclick="return confirm('Do you really want to permanently delete this package\'s configuration and data?')"><img src="x.gif" title="delete package configuration and data" width="17" height="17" border="0"></a></td>
							</tr>
						</table>
					</td>
				</tr><?
			}

			foreach($packages as $pkg) {

				if ($pkg['active']) {
					continue;
				}

				?><tr>
					<td class="listbgl"><?=$pkg['name'];?>&nbsp;(<?=$pkg['version'];?>)</td>
					<td class="listr">&nbsp;</td>
					<td class="listr">inactive</td>
					<td class="listr"><?=htmlspecialchars($pkg['descr']);?></td>
					<td valign="middle" nowrap class="list">
						<a href="interfaces_storage.php?action=activate&package=<?=$pkg['name'];?>"><img src="plus.gif" title="activate package" width="17" height="17" border="0"></a>
						<a href="javascript:{}" onclick="set_package_restore_name('<?=$pkg['name'];?>'); jQuery('#packages-restore-file-upload').slideDown();"><img src="r.gif" title="activate package from backup data" width="17" height="17" border="0"></a>
					</td>
				</tr><?
			}

			?></table><?

		}

		?><div id="packages-restore-file-upload" style="display: none; margin: 10px">
			Restoring <strong><span id="packages-restore-file-name"></span></strong> package. Select a backup archive to restore from and press "Restore."<br>
			<br>
			<input id="packageulfile" name="packageulfile" type="file" class="formfld">
			<input name="restore-submit" type="submit" class="formbtn" value="Restore"> <a href="javascript:{}" onclick="jQuery('#packages-restore-file-upload').slideUp();">cancel</a>
			<input id="packageulname" name="packageulname" type="hidden" value="">
		</div>
		</td>
	</tr>
</table>
</form><?

include("fend.inc"); ?>
