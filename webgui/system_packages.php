#!/usr/local/bin/php
<?php 
/*
	$Id: interfaces_storage.php 567 2008-06-24 17:11:18Z michael.iedema $
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2008 IKT <http://itison-ikt.de>.
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

function get_packages() {
	$files = array();
	$i = 0;
	$dh = opendir("/ultmp");
	while ($direntry = readdir($dh)) {
		if (strpos($direntry, "pbxpkg") !== false) {
			$files[$i]['filename'] = "/ultmp/" . $direntry;
			$files[$i]['name'] = substr($direntry, 0, strpos($direntry, "_"));
			$i++;
		}
	}
	closedir($dh);
	return $files;
}

function verify_package(&$pkg) {
		$sigchk = verify_digital_signature($pkg['filename']);
		if ($sigchk == 1) {
			$pkg['sigwarnings'][] = gettext("The digital signature on this package is invalid.");
		} else if ($sigchk == 2) {
			$pkg['sigwarnings'][] = gettext("This package is not digitally signed.");
		} else if (($sigchk == 3) || ($sigchk == 4)) {
			$pkg['sigwarnings'][] = gettext("There has been an error verifying the signature on this package.");
		}

		if (!verify_gzip_file($pkg['filename'])) {
			$pkg['errors'][] = gettext("The package is corrupt and has been deleted.");
			unlink($pkg['filename']);
		}
}

function extract_package(&$pkg) {
	mwexec("cd /ultmp/; /usr/bin/tar xf " . basename($pkg['filename']));
	unlink($pkg['filename']);
}

if ($_POST['warning_continue'] && $_POST['mode']) {
	$ignore_list = implode("&ignoresig[]=", $_POST['ignore_warning']);
	header("Location: system_packages.php?check={$_POST['mode']}&ignoresig[]=$ignore_list");
	exit;
}

if ($_POST['package_install_submit'] && is_uploaded_file($_FILES['package_install_file']['tmp_name'])) {
	move_uploaded_file($_FILES['package_install_file']['tmp_name'], "/ultmp/" . $_FILES['package_install_file']['name']);
	header("Location: system_packages.php?check=install");
	exit;
}

if ($_GET['check'] == "install") {
	$ulpkgs = get_packages();
	$syspart_path = storage_get_media_path("syspart");
	$n = count($ulpkgs);
	for ($i = 0; $i < $n; $i++) {
		verify_package($ulpkgs[$i]);
		if (is_array($_GET['ignoresig']) && in_array($ulpkgs[$i]['name'], $_GET['ignoresig'])) {
			unset($ulpkgs[$i]['sigwarnings']);
		}
		if (!isset($ulpkgs[$i]['errors']) && !isset($ulpkgs[$i]['sigwarnings'])) {
			if (packages_get_package($ulpkgs[$i]['name'])) {
				$ulpkgs[$i]['errors'][] = gettext("Package already exists, install failed.");
				unlink($ulpkgs[$i]['filename']);
			} else {
				extract_package($ulpkgs[$i]);
				if (!file_exists("/ultmp/" . $ulpkgs[$i]['name'] . ".pkg")) {
					$ulpkgs[$i]['errors'][] = gettext("Extracted package not found, install failed.");
				} else {
					mwexec("mv /ultmp/" . $ulpkgs[$i]['name'] . ".pkg $syspart_path");
					$ulpkgs[$i]['successful'] = true;
				}
			}
		}
	}
}

if ($_POST['package_update_submit'] && is_uploaded_file($_FILES['package_update_file']['tmp_name'])) {
	move_uploaded_file($_FILES['package_update_file']['tmp_name'], "/ultmp/" . $_FILES['package_update_file']['name']);
	header("Location: system_packages.php?check=update");
	exit;
}

if ($_GET['check'] == "update") {
	$ulpkgs = get_packages();
	$syspart_path = storage_get_media_path("syspart");
	$n = count($ulpkgs);
	for ($i = 0; $i < $n; $i++) {
		verify_package($ulpkgs[$i]);
		if (is_array($_GET['ignoresig']) && in_array($ulpkgs[$i]['name'], $_GET['ignoresig'])) {
			unset($ulpkgs[$i]['sigwarnings']);
		}
		if (!isset($ulpkgs[$i]['errors']) && !isset($ulpkgs[$i]['sigwarnings'])) {
			if (!($old_pkg = packages_get_package($ulpkgs[$i]['name']))) {
				$ulpkgs[$i]['errors'][] = gettext("Existing package not found, update failed.");
				unlink($ulpkgs[$i]['filename']);
			} else {
				extract_package($ulpkgs[$i]);
				if (!file_exists("/ultmp/" . $ulpkgs[$i]['name'] . ".pkg")) {
					$ulpkgs[$i]['errors'][] = gettext("Extracted package not found, install failed.");
				} else {
					$ulpkgs[$i]['conf'] = packages_read_config("/ultmp/" . $ulpkgs[$i]['name'] . ".pkg");
					if ($ulpkgs[$i]['conf']['version'] <= $old_pkg['version']) {
						$ulpkgs[$i]['errors'][] = gettext("The uploaded package is not newer than the existing package, update failed.");
						unlink($ulpkgs[$i]['filename']);
					} else {
						$srcpkg = "/ultmp/" . $ulpkgs[$i]['name'] . ".pkg";
						$destpkg = "$syspart_path/" . $ulpkgs[$i]['name'] . ".pkg";
						// copy the new rc file
						mwexec("/bin/cp $srcpkg/rc $destpkg");
						// copy the new www files if present
						if (file_exists("$srcpkg/www")) {
							if (!file_exists("$destpkg/www")) {
								mkdir("$destpkg/www");
							}
							mwexec("/bin/cp -R $srcpkg/www $destpkg");
						}
						// execute the update routine
						$ret = mwexec("/etc/rc.pkgexec $destpkg/rc update");
						// update package meta info from built-in package
						$updated_pkg_conf = $old_pkg;
						$updated_pkg_conf['version'] = $ulpkgs[$i]['conf']['version'];
						$updated_pkg_conf['descr'] = $ulpkgs[$i]['conf']['descr'];
						// write out package's updated conf.xml
						$updated_pkg_xml = array_to_xml($updated_pkg_conf, "package");
						util_file_put_contents($updated_pkg_xml, "$destpkg/conf.xml");
						// activate new package if needed
						if ($old_pkg['active']) {
							packages_exec_rc($ulpkgs[$i]['name'], "activate");
						}
						mwexec("rm -rf /ultmp/" . $ulpkgs[$i]['name'] . ".pkg");
						$ulpkgs[$i]['successful'] = true;
					}
				}
			}
		}
	}
}


if ($_POST['package_restore_submit'] && is_uploaded_file($_FILES['package_restore_file']['tmp_name'])) {
	move_uploaded_file($_FILES['package_restore_file']['tmp_name'], "/ultmp/" . $_FILES['package_restore_file']['name']);
	header("Location: system_packages.php?check=restore");
	exit;
}

if ($_GET['check'] == "restore") {

	$syspart_path = storage_get_media_path("syspart");

	// find uploaded restore files
	$files = array();
	$dh = opendir("/ultmp");
	while ($direntry = readdir($dh)) {
		if (strpos($direntry, "tgz") !== false) {
			$files[] = $direntry;
		}
	}
	closedir($dh);

	// extract
	foreach ($files as $f) {
		mwexec("cd /ultmp; /usr/bin/tar zxf $f");
	}

	// find the name of the freshly extracted package directory
	$restore_names = array();
	$dh = opendir("/ultmp");
	while ($direntry = readdir($dh)) {
		if (strpos($direntry, ".pkg") !== false) { 
			$restore_names[] = substr($direntry, 0, strpos($direntry, ".pkg"));
		} 
	}
	closedir($dh);

	foreach ($restore_names as $restore) {
		// if that package exists, record state and delete it from the system
		if ($old_pkg = packages_get_package($restore)) {
			packages_exec_rc($restore, "delete");
		}

		// remove active state file and move over restore data
		unlink_if_exists("/ultmp/$restore.pkg/pkg.active");
		mwexec("mv /ultmp/$restore.pkg $syspart_path");

		// activate new package if needed
		if ($old_pkg['active']) {
			packages_exec_rc($restore, "activate");
		}
	}
}


/* activate, deactivate, delete */
if (isset($_GET['package']) && 
	in_array($_GET['action'], array("activate", "deactivate", "delete"))) {

	packages_create_command_file($d_packageconfdirty_path, $_GET['package'], $_GET['action']);
	header("Location: system_packages.php");
	exit;
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
}

// apply changes
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

if (!isset($config['system']['disablepackagechecks']) && $syspart && !$ulpkgs) {
	$server_packages = packages_check_server_versions();
	if ($server_packages) {
		foreach ($server_packages as $s_p_name => $s_p_a) {
			if ($packages[$s_p_name] && ($packages[$s_p_name]['version'] < $s_p_a['version'])) {
				$pkg_updates[$s_p_name] = $s_p_a;
			} else if (!$packages[$s_p_name]) {
				$pkg_new[$s_p_name] = $s_p_a;
			}
		}
	}
} else {
	$disablepackagechecks = true;
}


include("fbegin.inc");
if ($savemsg) print_info_box($savemsg);
if ($input_errors) print_input_errors($input_errors);

?><script type="text/javascript" charset="utf-8">

	var type;
	var index = 0;
	var urls = [];
	var currenturl;
	var currentfile;
	var currentid;
	var total_bytes;
	var current_bytes;
	var percentage = 0;

	function start_downloads(operationtype) {
		type = operationtype;
		jQuery('input').attr('disabled', 'disabled');
		var i = 0;
		jQuery('input:checked[name^=package_' + type + ']').each(function() {
			urls[i] = jQuery(this).val();
			i++;
		});
		index = 0;
		percentage = 0;
		start_dl(urls[index]);
		setTimeout("dl_looper()", 2000);
	}

	function dl_looper() {
		// start the first download
		if (percentage < 100) {
			setTimeout("dl_looper()", 2000);
			return;

		// done, start the next one
		} else if (index < urls.length-1) {
			percentage = 0;
			index++;
			start_dl(urls[index]);
			setTimeout("dl_looper()", 2000);
			return;

		// all done
		} else {
			window.location = "?check=" + type;
		}
	}

	function start_dl(url) {
		currenturl = url;
		currentfile = url.replace(/.*\//, "");
		currentid = '.' + type + '_' + currentfile.replace(/\./g,"_");
		jQuery.get("dl.php?new=" + currenturl, function(data) {
			if (!data) {
				alert("download start failed!");
			} else {
				jQuery(currentid).progressBar({ boxImage: 'progressbar.gif', barImage: 'progressbg_black.gif', showText: false} );
				total_bytes = parseInt(data);
				setTimeout("update_dl_progress()", 1000);
			}
		});
	}

	function update_dl_progress() {
		jQuery.get("ajax.cgi?exec_shell=ls%20-l%20/ultmp/" + currentfile, function(data) {
			var pieces = data.split(/\s+/);
			if (!pieces[4]) {
				alert("download update failed!");
			} else {
				current_bytes = parseInt(pieces[4]);
				percentage = Math.floor(100 * current_bytes / total_bytes);
				jQuery(currentid).progressBar(percentage);
				if (percentage != 100) {
					setTimeout("update_dl_progress()", 1000);
				}
			}
		});
	}

	function show_pane(name) {
		jQuery('#restore-pane').hide();
		jQuery('#update-pane').hide();
		jQuery('#install-pane').hide();
		jQuery('#' + name + '-pane').show();
	}

</script><?

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

	if ($ulpkgs) {
		foreach ($ulpkgs as $ulpkg) {
			if ($ulpkg['successful']) {
				$success[] = $ulpkg;
			} else if ($ulpkg['errors']) {
				$errors[] = "<strong>" . $ulpkg['name'] . "</strong> - " .
					implode("<br>", $ulpkg['errors']);
			} else if ($ulpkg['sigwarnings']) {
				$sig_warnings[$ulpkg['name']] = implode(", ", $ulpkg['sigwarnings']);
			}
		}
	}

	if ($sig_warnings || $errors) {

		if ($errors) {
			print_input_errors($errors);
			echo "<br><br>";
		}

		?><form action="system_packages.php" method="post"><?

		if ($sig_warnings) {

			$warning_text = gettext("The following packages issued warnings while verifying their digital signature. This means that they are not official/supported packages and may lead to unexpected behavior or security compromises. Only install packages that come from sources that you trust, and make sure that they have not been tampered with.<br><br>Check the packages you wish to install despite this warning.") . "<br><br>\n";

			foreach ($sig_warnings as $sw_name => $sw_message) {
				$warning_text .= "<input name=\"ignore_warning[]\" type=\"checkbox\" value=\"$sw_name\"> ";
				$warning_text .= "<strong>$sw_name</strong> - $sw_message<br>\n";
			}

			print_info_box($warning_text, "keep");	
		}

		?><input name="warning_continue" type="submit" class="formbtn" value=" <?=gettext("Continue");?> ">
		<input name="mode" type="hidden" value="<?=$_GET['check'];?>">
		</form><?

	} else {

	/*
		Array
		(
		    [0] => Array
		        (
		            [filename] => /ultmp/logging_1.1.pbxpkg
		            [name] => logging
		            [successful] => 1
		        )
		)
	*/

	?><table width="100%" border="0" cellpadding="6" cellspacing="0">
		<tr>
			<td width="5%" class="list"></td>
			<td width="20%" class="listhdrr"><?=gettext("Name");?></td>
			<td width="15%" class="listhdrr"><?=gettext("Size");?></td>
			<td width="45%" class="listhdr"><?=gettext("Description");?></td>
			<td width="15%" class="list"></td>
		</tr><?
			
	foreach($packages as $pkg) {

		?><tr><?

		if ($pkg['active']) {

			?><td valign="middle" nowrap class="list"><a href="?action=deactivate&package=<?=$pkg['name'];?>" onclick="return confirm('<?=gettext("Do you really want to disable this package?");?>')"><img src="enabled.png" title="<?=gettext("click to disable package");?>" border="0"></a></td><?

		} else {

			?><td valign="middle" nowrap class="list"><a href="?action=activate&package=<?=$pkg['name'];?>"><img src="disabled.png" title="<?=gettext("click to enable package");?>" border="0"></a></td><?

		}

			?><td class="listbgl"><?=$pkg['name'];?>&nbsp;(<?=$pkg['version'];?>)</td>
			<td class="listr"><?=format_bytes(packages_get_size($pkg['name']));?></td>
			<td class="listr"><?=htmlspecialchars($pkg['descr']);?></td>
			<td valign="middle" nowrap class="list"><a href="?action=backup&package=<?=$pkg['name'];?>"><img src="backup.png" title="<?=gettext("backup package data");?>" border="0"></a>
				<a href="?action=delete&package=<?=$pkg['name'];?>" onclick="return confirm('<?=gettext("Do you really want to permanently delete this package\'s configuration and data?");?>')"><img src="delete.png" title="<?=gettext("delete package configuration and data");?>" border="0"></a></td>
		</tr><?
	}

		?><tr> 
			<td class="list" colspan="4"></td>
			<td class="list"><a href="javascript:{}" onclick="show_pane('install');"><img src="add.png" title="<?=gettext("install new package");?>" border="0"></a>
				<a href="javascript:{}" onclick="show_pane('update');"><img src="update.png" title="<?=gettext("update package to newer version");?>" border="0"></a>
				<a href="javascript:{}" onclick="show_pane('restore');"><img src="restore.png" title="<?=gettext("restore package data");?>" border="0"></a></td>
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
						echo gettext("Select which packages you would like to update, existing package data will be preserved.") . "<br>\n";

						foreach ($pkg_updates as $pkg_name => $pkg_a) {
							?><input name="package_update" type="checkbox" value="<?=$pkg_a['url'];?>" checked>
							<span class="update_<?=str_replace(".", "_", $pkg_a['filename']);?>"></span>
							<?=$pkg_name;?>&nbsp;(<?=$pkg_a['version'];?>)
							<br><?
						}

						?><br><input name="package_update" type="submit" id="update_submit" class="formbtn" value="<?=gettext("Update");?>" onclick="start_downloads('update')"><?

					} else if (!$disablepackagechecks) {
						?><em>No package updates available.</em><br><?
					}

					?><br>
					<br>
					<?=gettext("Select a .pbxpkg file from your computer to update from, existing package data will be preserved.");?>
					<br>
					<form action="system_packages.php" method="post" enctype="multipart/form-data">
						<input id="package_update_file" name="package_update_file" type="file" class="formfld">
						<input name="package_update_submit" type="submit" class="formbtn" value="<?=gettext("Manual Update");?>">
					</form>
				</div>

				<div id="install-pane" class="tabcont" style="display: none;">
					<strong><?=gettext("Install a New Package");?></strong><br>
					<br><?

					if ($pkg_new) {
						echo gettext("Select which packages you would like to install.") . "<br>\n";

						foreach ($pkg_new as $pkg_name => $pkg_a) {
							?><input name="package_install" type="checkbox" value="<?=$pkg_a['url'];?>" checked>
							<span class="install_<?=str_replace(".", "_", $pkg_a['filename']);?>"></span>
							<?=$pkg_name;?>&nbsp;(<?=$pkg_a['version'];?>)
							<br><?
						}

						?><br><input name="package_install" type="submit" id="install_submit" class="formbtn" value="<?=gettext("Install");?>" onclick="start_downloads('install')"><?

					} else if (!$disablepackagechecks) {
						?><em>No new packages available.</em><br><?
					}

					?><br>
					<br>
					<?=gettext("Select a .pbxpkg file from your computer to install.");?>
					<br>
					<form action="system_packages.php" method="post" enctype="multipart/form-data">
						<input id="package_install_file" name="package_install_file" type="file" class="formfld">
						<input name="package_install_submit" type="submit" class="formbtn" value="<?=gettext("Manual Install");?>">
					</form>
				</div>

				<div id="restore-pane" class="tabcont" style="display: none;">
					<strong><?=gettext("Restore from a Backup Archive");?></strong><br>
					<br>
					<?=gettext("Select a backup .tgz archive and press 'Restore'");?><br>
					<?=gettext("Package data and logic will be replaced by the backup.");?><br>
					<br>
					<form action="system_packages.php" method="post" enctype="multipart/form-data">
						<input id="package_restore_file" name="package_restore_file" type="file" class="formfld">
						<input name="package_restore_submit" type="submit" class="formbtn" value="<?=gettext("Restore");?>">
					</form>
				</div>

			</td>
			<td class="list"></td>
		</tr>
	</table><?

	}
}

include("fend.inc"); ?>
