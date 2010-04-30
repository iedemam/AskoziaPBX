#!/usr/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2009-2010 tecema (a.k.a IKT) <http://www.tecema.de>.
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

$pgtitle = array(gettext("Status"), gettext("Logs"));


include("fbegin.inc");

?><script type="text/JavaScript">
<!--

	var last_line;
	var auto_updating = true;
	var msg_disabled = '<?=gettext("Stopped updates");?>';
	var msg_enabled = '<?=gettext("Resuming updates");?>';

	jQuery(document).ready(function(){
		update();
	});

	function toggle_auto_updating() {
		if (auto_updating) {
			auto_updating = false;
		} else {
			auto_updating = true;
			jQuery("#log_contents").append(
				'<div class="logentry">' + msg_enabled + '</div>'
			);
			update();
		}
	}

	function update() {
		jQuery.get("cgi-bin/ajax.cgi", { exec_shell: "logread" }, function(data){
			var i = 0;
			var lines = data.split(/\n/);

			if (last_line) {
				var overlaps = false;
				for (i = 0; i < lines.length - 1; i++) {
					if (last_line == lines[i]) {
						overlaps = true;
						i++;
						break;
					}
				}
				if (!overlaps) {
					i = 0;
				}
			}

			for (i = i; i < lines.length - 1; i++) {
				jQuery("#log_contents").append(
					'<div class="logentry">' + lines[i] + '</div>'
				);
				last_line = lines[i];
			}

		});

		if (!auto_updating) {
			jQuery("#log_contents").append(
				'<div class="logentry">' + msg_disabled + '</div>'
			);
		} else {
			setTimeout("update()", 5000);
		}
	}

//-->
</script>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr> 
		<td class="tabcont">
			<table width="100%" border="0" cellspacing="0" cellpadding="0">
				<tr> 
					<td class="listtopic"><?=gettext("System log entries");?></td>
				</tr>
			</table>
			<div id="log_contents"></div>
			<div style="float:right; padding-top:5px" id="log_controls">
				<input id=update_toggle" type="submit" class="formbtn" value="<?=gettext("Toggle auto-updates");?>" onclick="toggle_auto_updating()">
			</div>
		</td>
	</tr>
</table>
<?php include("fend.inc"); ?>
