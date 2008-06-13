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

/* backup */
if (isset($_GET['package']) && $_GET['action'] == 'backup') {

	$name = $_GET['package'];
	$pkg = packages_get_package($name);

	if (isset($pkg['active'])) {

		$tgz_filename = "package-$name-" . 
			$config['system']['hostname'] . "." . 
			$config['system']['domain'] . "-" . date("YmdHis") . ".tgz";

		header("Content-Type: application/octet-stream"); 
		header("Content-Disposition: attachment; filename=$tgz_filename");
		passthru("/usr/bin/tar cfz - -C {$pkg['parentpath']} $name.pkg");
		exit;
	}

/* activate, deactivate, delete */
} else if (isset($_GET['package']) && 
	in_array($_GET['action'], array("activate", "deactivate", "delete"))) {

	packages_create_command_file($d_packageconfdirty_path, $_GET['package'], $_GET['action']);
	header("Location: interfaces_storage.php");
	exit;

/* install */
} else if ($_POST['install-submit'] && is_uploaded_file($_FILES['installfile']['tmp_name'])) {

	$pkg_install_path = storage_get_media_path("syspart");
	$ultmp_path =  $pkg_install_path . "/ultmp/";
	$file_name = $_FILES['installfile']['name'];
	$full_file_name = $ultmp_path . "/" . $file_name;

	// rename the uploaded file back to its original name
	move_uploaded_file($_FILES['installfile']['tmp_name'], $full_file_name);

	// cd into ultmp and decompress the newly uploaded file
	mwexec("cd $ultmp_path; /usr/bin/tar zxf $file_name");

	// find the name of the freshly extracted package directory
	$dh = opendir($ultmp_path);
	while ($direntry = readdir($dh)) {
		if (strpos($direntry, ".pkg") !== false) { 
			$uploaded_pkg_name = substr($direntry, 0, strpos($direntry, ".pkg"));
		} 
	}
	closedir($dh);

	// if that package exists, record state and delete it from the system
	if ($old_pkg = packages_get_package($uploaded_pkg_name)) {
		packages_exec_rc($uploaded_pkg_name, "delete");
	}

	// remove active state file and move over restore data
	unlink_if_exists($ultmp_path . "/" . $uploaded_pkg_name . ".pkg/pkg.active");
	mwexec("mv " . $ultmp_path . "/" . $uploaded_pkg_name . ".pkg " . $pkg_install_path);

	// activate new package if needed
	if ($old_pkg['active']) {
		packages_exec_rc($uploaded_pkg_name, "activate");
	}

	// remove uploaded files
	mwexec("rm -rf " . $ultmp_path . "/*");

	// set save message


	header("Location: interfaces_storage.php");
	exit;
}

/* apply changes */
if (file_exists($d_packageconfdirty_path)) {
	if ($command = packages_read_command_file($d_packageconfdirty_path)) {

		$retval = 0;
		if (!file_exists($d_sysrebootreqd_path)) {
			$retval |= packages_exec_rc($command['name'], $command['command']);
		}
		$savemsg = packages_generate_save_message($command['name'], $command['command']);
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
					<td width="30%" class="listhdr">Installed Packages</td>
					<td width="10%" class="list"></td>
				</tr>
				<tr>
					<td class="listbgl">system storage partition</td>
					<td class="listr"><? display_capacity_bar(
						($syspart['size'] - ($defaults['storage']['system-partition-offset-megabytes']*1024*1024)),
						$syspart['usage']);?>&nbsp;</td>
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
								<td><a href="javascript:{}" onclick="jQuery('#packages-file-upload').slideDown();"><img src="r.gif" title="restore package data" width="17" height="17" border="0"></a></td>
							</tr>
							<tr>
								<td align="center" valign="middle"><?
								/* future short cut to packages's settings?

							if ($ss_info['hasoptions']) {
									?><a href="packages_edit_package.php?name=<?=$pkg['name'];?>"><img src="e.gif" title="edit package settings" width="17" height="17" border="0"></a><?
							}

								*/ ?><a href="interfaces_storage.php?action=deactivate&package=<?=$pkg['name'];?>" onclick="return confirm('Do you really want to deactivate this package?')"><img src="minus.gif" title="deactivate package" width="17" height="17" border="0"></a></td>
								<td><a href="interfaces_storage.php?action=delete&package=<?=$pkg['name'];?>" onclick="return confirm('Do you really want to permanently delete this package\'s configuration and data?')"><img src="x.gif" title="delete package configuration and data" width="17" height="17" border="0"></a></td>
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
					<td class="listr"><?=format_bytes(packages_get_size($pkg['name']));?></td>
					<td class="listr">inactive</td>
					<td class="listr"><?=htmlspecialchars($pkg['descr']);?></td>
					<td valign="middle" nowrap class="list">
						<a href="interfaces_storage.php?action=activate&package=<?=$pkg['name'];?>"><img src="plus.gif" title="activate package" width="17" height="17" border="0"></a>
						<a href="javascript:{}" onclick="jQuery('#packages-file-upload').slideDown();"><img src="r.gif" title="activate package from backup data" width="17" height="17" border="0"></a>
					</td>
				</tr><?
			}

				?><tr> 
					<td class="list" colspan="4"></td>
					<td class="list"><a href="javascript:{}" onclick="jQuery('#packages-file-upload').slideDown();"><img src="plus.gif" title="install package" width="17" height="17" border="0"></a></td>
				</tr>
			</table><?

		}

		?><div id="packages-file-upload" style="display: none; margin: 10px">
			Select a package .tgz archive and press "Install"<br>
			<br>
			<input id="installfile" name="installfile" type="file" class="formfld">
			<input name="install-submit" type="submit" class="formbtn" value="Install"> <a href="javascript:{}" onclick="jQuery('#packages-file-upload').slideUp();">cancel</a>
		</div>
		</td>
	</tr>
</table>
</form><?

include("fend.inc"); ?>
