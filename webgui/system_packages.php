#!/usr/local/bin/php
<?php 
/*
	$Id: interfaces_storage.php 567 2008-06-24 17:11:18Z michael.iedema $
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

$pgtitle = array(gettext("System"), gettext("Packages"));
require("guiconfig.inc");

if ($_POST) {
	echo print_r_html($_POST);
	exit();
}

function check_package_versions() {
	global $g;
	$post = "&check=pbxpackage&platform=" . rawurlencode($g['fullplatform']) . 
		"&version=" . rawurlencode(trim(file_get_contents("/etc/version")));
		
	$rfd = @fsockopen("downloads.askozia.com", 80, $errno, $errstr, 3);
	if ($rfd) {
		$hdr = "POST /index.php HTTP/1.0\r\n";
		$hdr .= "Content-Type: application/x-www-form-urlencoded\r\n";
		$hdr .= "User-Agent: AskoziaPBX-webGUI/1.0\r\n";
		$hdr .= "Host: downloads.askozia.com\r\n";
		$hdr .= "Content-Length: " . strlen($post) . "\r\n\r\n";
		
		fwrite($rfd, $hdr);
		fwrite($rfd, $post);
		
		$inhdr = true;
		$resp = "";
		while (!feof($rfd)) {
			$line = fgets($rfd);
			if ($inhdr) {
				if (trim($line) == "")
					$inhdr = false;
			} else {
				$resp .= $line;
			}
		}
		
		fclose($rfd);

		$lines = explode("\n", $resp);
		foreach ($lines as $line) {
			$line = preg_split("/\s+/", $line, -1, PREG_SPLIT_NO_EMPTY);
			if ($line[0] && $line[1]) {
				$packages[$line[0]] = $line[1];
			}
		}
		return $packages;
	}
	
	return null;
}

/* backup */
if (isset($_GET['package']) && $_GET['action'] == 'backup') {

	$name = $_GET['package'];
	$pkg = packages_get_package($name);

	if ($pkg) {

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
	header("Location: system_packages.php");
	exit;

/* restore a package from a backup archive: the package is completely replaced */
} else if ($_POST['restore-submit'] && is_uploaded_file($_FILES['restorefile']['tmp_name'])) {

	$pkg_install_path = storage_get_media_path("syspart");
	$ultmp_path =  $pkg_install_path . "/ultmp/";
	$file_name = $_FILES['restorefile']['name'];
	$full_file_name = $ultmp_path . "/" . $file_name;

	// rename the uploaded file back to its original name
	move_uploaded_file($_FILES['restorefile']['tmp_name'], $full_file_name);

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

	header("Location: system_packages.php");
	exit;

/* update an existing package: logic is updated */
} else if ($_POST['update-submit'] && is_uploaded_file($_FILES['updatefile']['tmp_name'])) {

	$pkg_install_path = storage_get_media_path("syspart");
	$ultmp_path =  $pkg_install_path . "/ultmp/";
	$file_name = $_FILES['updatefile']['name'];
	$full_file_name = $ultmp_path . "/" . $file_name;

	// rename the uploaded file back to its original name
	move_uploaded_file($_FILES['updatefile']['tmp_name'], $full_file_name);

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

	// if the package does not exist, fail
	if (!($old_pkg = packages_get_package($uploaded_pkg_name))) {
		$input_errors[] = gettext("Existing package not found, update failed.");

	} else {
		// read the uploaded package's configuration
		$uploaded_pkg_path = $ultmp_path . "/" . $uploaded_pkg_name . ".pkg";
		$uploaded_pkg_conf = packages_read_config($uploaded_pkg_path);

		// if the package is not newer, fail
		if ($uploaded_pkg_conf['version'] <= $old_pkg['version']) {
			$input_errors[] = gettext("The uploaded package is not newer than the existing package, update failed.");

		} else {
			// copy the new rc file
			mwexec("/bin/cp " . $uploaded_pkg_path . "/rc " . $pkg_install_path . "/" . $uploaded_pkg_name);
			// copy the new www files if present
			if (file_exists($uploaded_pkg_path . "/www")) {
				mwexec("/bin/cp -R " . $uploaded_pkg_path . "/www " . $pkg_install_path . "/" . $uploaded_pkg_name);
			}
			// execute the update routine
			$ret = mwexec("/etc/rc.pkgexec " . $pkg_install_path . "/" . $uploaded_pkg_name . "/rc update");
			// update package meta info from built-in package
			$updated_pkg_conf = $old_pkg;
			$updated_pkg_conf['version'] = $uploaded_pkg_conf['version'];
			$updated_pkg_conf['descr'] = $uploaded_pkg_conf['descr'];
			// write out package's updated conf.xml
			$updated_pkg_xml = array_to_xml($updated_pkg_conf, "package");
			util_file_put_contents($updated_pkg_xml, $pkg_install_path . "/" . $uploaded_pkg_name . "/conf.xml");

			// activate new package if needed
			if ($old_pkg['active']) {
				packages_exec_rc($uploaded_pkg_name, "activate");
			}

			// remove uploaded files
			mwexec("rm -rf " . $ultmp_path . "/*");

			header("Location: system_packages.php");
			exit;
		}	
	}

	// remove uploaded files
	mwexec("rm -rf " . $ultmp_path . "/*");

/* install a new package */
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

	// if that package exists, fail because we should be installing a new package
	if (packages_get_package($uploaded_pkg_name)) {
		$input_errors[] = gettext("Package already exists, install failed.");

	} else {
		// move over new pkg data
		mwexec("mv " . $ultmp_path . "/" . $uploaded_pkg_name . ".pkg " . $pkg_install_path);

		// remove uploaded files
		mwexec("rm -rf " . $ultmp_path . "/*");

		header("Location: system_packages.php");
		exit;	
	}

	// remove uploaded files
	mwexec("rm -rf " . $ultmp_path . "/*");
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

if (!isset($config['system']['disablepackagechecks'])) {
	$server_packages = check_package_versions();
}

?>

<?php include("fbegin.inc"); ?>
<?php if ($savemsg) print_info_box($savemsg); ?>
<?php if ($input_errors) print_input_errors($input_errors); ?>
<script type="text/javascript" charset="utf-8">

	function show_pane(name) {
		jQuery('#restore-pane').hide();
		jQuery('#update-pane').hide();
		jQuery('#install-pane').hide();
		jQuery('#' + name + '-pane').show();
	}

</script>
<form action="system_packages.php" method="post" enctype="multipart/form-data"><?

if ($server_packages) {
	foreach ($server_packages as $s_p_name => $s_p_version) {
		if ($packages[$s_p_name] && ($packages[$s_p_name]['version'] < $s_p_version)) {
			$pkg_updates[$s_p_name] = $s_p_version;
		} else if (!$packages[$s_p_name]) {
			$pkg_new[$s_p_name] = $s_p_version;
		}
	}
}

if ($pkg_updates) {

	?><p>
		<table border="0" cellspacing="0" cellpadding="4" width="100%">
			<tr>
				<td bgcolor="#687BA4" align="center" valign="top" width="36"><img src="exclam.gif" width="28" height="32"></td>
				<td bgcolor="#D9DEE8" style="padding-left: 8px"><strong><?=gettext("There are updates available for packages on your system.");?></strong><br><?=gettext("Click the <img src=\"update.png\" border=\"0\"> icon below to select and install the updated packages.");?>
				</td>
			</tr>
		</table>
	</p><?

}

if (!$syspart) {

	?><strong><?=gettext("The system storage media is not large enough to install packages.");?></strong> <?=sprintf(gettext("A minimum of %sMB is required. In the future, external media will be able to be used, but currently packages must be stored on the internal system media."), $defaults['storage']['system-media-minimum-size']);

} else {

	?><table width="100%" border="0" cellpadding="6" cellspacing="0">
		<tr>
			<td width="5%" class="list"></td>
			<td width="20%" class="listhdrr"><?=gettext("Name");?></td>
			<td width="15%" class="listhdrr"><?=gettext("Size");?></td>
			<td width="45%" class="listhdr"><?=gettext("Description");?></td>
			<td width="15%" class="list"></td>
		</tr><?
			
	foreach($packages as $pkg) {
    
		if (!$pkg['active']) {
			continue;
		}
    
		?><tr>
			<td valign="middle" nowrap class="list"><a href="?action=deactivate&package=<?=$pkg['name'];?>" onclick="return confirm('<?=gettext("Do you really want to disable this package?");?>')"><img src="enabled.png" title="<?=gettext("click to disable package");?>" border="0"></a></td>
			<td class="listbgl"><?=$pkg['name'];?>&nbsp;(<?=$pkg['version'];?>)</td>
			<td class="listr"><?=format_bytes(packages_get_size($pkg['name']));?></td>
			<td class="listr"><?=htmlspecialchars($pkg['descr']);?></td>
			<td valign="middle" nowrap class="list"><a href="?action=backup&package=<?=$pkg['name'];?>"><img src="backup.png" title="<?=gettext("backup package data");?>" border="0"></a>
				<a href="javascript:{}" onclick="show_pane('restore');"><img src="restore.png" title="<?=gettext("restore package data");?>" border="0"></a>
				<a href="?action=delete&package=<?=$pkg['name'];?>" onclick="return confirm('<?=gettext("Do you really want to permanently delete this package\'s configuration and data?");?>')"><img src="delete.png" title="<?=gettext("delete package configuration and data");?>" border="0"></a></td>
		</tr><?
	}
    
	foreach($packages as $pkg) {
    
		if ($pkg['active']) {
			continue;
		}
    
		?><tr>
			<td valign="middle" nowrap class="list"><a href="?action=activate&package=<?=$pkg['name'];?>"><img src="disabled.png" title="<?=gettext("click to enable package");?>" border="0"></a></td>
			<td class="listbgl"><?=$pkg['name'];?>&nbsp;(<?=$pkg['version'];?>)</td>
			<td class="listr"><?=format_bytes(packages_get_size($pkg['name']));?></td>
			<td class="listr"><?=htmlspecialchars($pkg['descr']);?></td>
			<td valign="middle" nowrap class="list"><a href="?action=backup&package=<?=$pkg['name'];?>"><img src="backup.png" title="<?=gettext("backup package data");?>" border="0"></a>
				<a href="javascript:{}" onclick="show_pane('restore');"><img src="restore.png" title="<?=gettext("activate package from backup data");?>" border="0"></a>
				<a href="?action=delete&package=<?=$pkg['name'];?>" onclick="return confirm('<?=gettext("Do you really want to permanently delete this package\'s configuration and data?");?>')"><img src="delete.png" title="<?=gettext("delete package configuration and data");?>" border="0"></a></td>
		</tr><?
	}

		?><tr> 
			<td class="list" colspan="4"></td>
			<td class="list"><a href="javascript:{}" onclick="show_pane('install');"><img src="add.png" title="<?=gettext("install new package");?>" border="0"></a>
				<a href="javascript:{}" onclick="show_pane('update');"><img src="update.png" title="<?=gettext("update package to newer version");?>" border="0"></a></td>
		</tr>
		<tr> 
			<td class="list" colspan="5" height="12">&nbsp;</td>
		</tr>
		<tr>
			<td class="list"></td>
			<td class="list" colspan="3">

				<div id="update-pane" class="tabcont" style="display: none;">
					<strong><?=gettext("Update Installed Package");?></strong><br>
					<br><?

					if ($pkg_updates) {
						echo gettext("Select which packages you would like to update or select a .pkgpbx file and press 'Update'");
					} else {
						echo gettext("Select a .pbxpkg file and press 'Update'");
					}

					?><br>
					<?=gettext("Existing package data will be preserved.");?><br>
					<br><?

					if ($pkg_updates) {
						foreach ($pkg_updates as $pkg_name => $pkg_version) {
							?><input name="update[]" type="checkbox" id="update[]" value="<?=$pkg_name;?>" checked>&nbsp;<?=$pkg_name;?>&nbsp;(<?=$pkg_version;?>)<br><?
						}
					}

					?><input id="updatefile" name="updatefile" type="file" class="formfld">
					<input name="update-submit" type="submit" class="formbtn" value="<?=gettext("Update");?>">
				</div>

				<div id="install-pane" class="tabcont" style="display: none;">
					<strong><?=gettext("Install a New Package");?></strong><br>
					<br><?

					if ($pkg_new) {
						echo gettext("Select which packages you would like to install or select a .pkgpbx file and press 'Install'");
					} else {
						echo gettext("Select a .pbxpkg file and press 'Install'");
					}

					?><br>
					<br><?

					if ($pkg_new) {
						foreach ($pkg_new as $pkg_name => $pkg_version) {
							?><input name="install[]" type="checkbox" id="install[]" value="<?=$pkg_name;?>">&nbsp;<?=$pkg_name;?><br><?
						}
						echo "<br>\n";
					}

					?><input id="installfile" name="installfile" type="file" class="formfld">
					<input name="install-submit" type="submit" class="formbtn" value="<?=gettext("Install");?>">
				</div>

				<div id="restore-pane" class="tabcont" style="display: none;">
					<strong><?=gettext("Restore from a Backup Archive");?></strong><br>
					<br>
					<?=gettext("Select a backup .tgz archive and press 'Restore'");?><br>
					<?=gettext("Package data and logic will be replaced by the backup.");?><br>
					<br>
					<input id="restorefile" name="restorefile" type="file" class="formfld">
					<input name="restore-submit" type="submit" class="formbtn" value="<?=gettext("Restore");?>">
				</div>
			</td>
			<td class="list"></td>
		</tr>
	</table><?

}

?></form><?

include("fend.inc"); ?>
