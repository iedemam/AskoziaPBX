#!/usr/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2011 tecema (a.k.a IKT) <http://www.tecema.de>.
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
$pgtitle = array(gettext("Services"), gettext("Music-on-Hold"));

//	case "deactivate"
//		$config['media']['custom-moh'] = false;
//	break;
	
$disk = storage_service_is_active("media");
$mohdir = $disk['mountpoint'] . "/askoziapbx/media/moh-custom";
$mohfile = $mohdir."/moh_personal.ulaw";

if($_POST['save'])
{
	if($_POST["custom-moh"] == true)
	{
		
		if(!is_dir($mohdir))
		{
			mkdir($mohdir);
		}
		
		if (is_uploaded_file($_FILES['audiofile']['tmp_name'])) {
			
			// move file to asterisk music-on-hold directory on media storage
			$new_filename = "/tmp/" . $_FILES['audiofile']['name'];
			move_uploaded_file($_FILES['audiofile']['tmp_name'], $new_filename);
			
			// get filetype
			$file_array = explode(".",$_FILES['audiofile']['name']);
			$filetype = $file_array[count($file_array)-1];
			
			// convert it to ulaw if it possible
			if($filetype != "ulaw" && $filetype != "wav" && $filetype != "mp3")
			{
				$input_errors[] = gettext("The filetype could not be recognized (file upload error).");
			} else {
				if($filetype == "wav")
				{
					exec("sox -V1 \"".$new_filename."\" -r 8000 -c 1 -t ul \"".$mohfile."\" 2>&1",$error_msg);
				} else if($filetype == "mp3") {
					exec("sox -V1 \"".$new_filename."\" -r 8000 -c 1 -t ul \"".$mohfile."\" 2>&1",$error_msg);
				} else {
					exec("mv ".$new_filename." ".$mohfile);
				}
								
				if(count($error_msg) == 0) {
					$config['media']['custom-moh'] = true;
					write_config();
					touch($g['moh_dirty_path']);
				} else {
					$input_errors[] = gettext("The audio file could not be uploaded. (filetype error)").": ".$error_msg[0];
				}
			}
		
		} else if(is_file($mohfile)) {
			$savemsg = gettext("The changes have been applied successfully. No upload file found but there was an old custom Music-on-Hold file.");
			$config['media']['custom-moh'] = true;
			write_config();
			touch($g['moh_dirty_path']);
		} else {
			$input_errors[] = gettext("The audio file could not be uploaded. (file upload error).");
		}
	} else {
		$config['media']['custom-moh'] = false;
		write_config();
		touch($g['moh_dirty_path']);
	}
} 

// save config if it neccesary
if (file_exists($g['moh_dirty_path'])) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= musiconhold_conf_generate();
		config_unlock();
		
		$retval |= pbx_exec("moh reload");
	}
	$savemsg = get_std_save_message($retval);
	if ($retval == 0) {
		unlink($g['moh_dirty_path']);
	}
}

if(isset($config['media']['custom-moh']))
{
	$form["custom-moh"] = " checked";
}

if(is_file($mohfile))
{
	$filetime = date("F j, Y, g:i a",filemtime($mohfile));
} else {
	$filetime = gettext("No last upload.");
}

include("fbegin.inc");

?><script type="text/JavaScript">
<!--
	<?=javascript_userdefined_moh_file_upload("functions");?>

	jQuery(document).ready(function(){

		<?=javascript_userdefined_moh_file_upload("ready");?>
	});

//-->
</script>

<form action="services_media_moh.php" method="post" enctype="multipart/form-data"><table width="100%" border="0" cellpadding="0" cellspacing="0"> <?php
	
	display_media_tab_menu();
	
	?><tr>
		<td class="tabcont">
			<table width="100%" border="0" cellpadding="6" cellspacing="0">
			
			<tr>
				<td colspan="2" valign="top" class="d_header"><?=gettext("Custom Music-on-Hold");?></td>
			</tr>
			
			
			
				<tr> 
					<td width="20%" valign="top" class="vncell">&nbsp;</td>
					<td width="80%" class="vtable">
						<input name="custom-moh" type="checkbox" id="custom-moh" value="yes" <?=$form["custom-moh"]?>> <?=gettext("Activate custom Music-On-Hold");?>
						<? display_moh_file_upload($filetime); ?><br>
					</td>
				</tr>
			
			<tr>
				<td colspan="2">
					&nbsp;
				</td>	
			</tr>
			
			<tr>
				<td></td><td valign="top">
				<input name="save" type="submit" class="formbtn" id="save" value="<?=gettext("Save");?>">
				</td>
			</tr>
		</table>
		</td>
	</tr>
</table></form><?php

include("fend.inc");