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

$pgtitle = array("Diagnostics", "Manager Interface");
require("guiconfig.inc");

include("fbegin.inc"); ?>

<script type="text/JavaScript">

	jQuery(document).ready(function(){

		jQuery.preloadImages(['/ajax_busy_round.gif']);

		jQuery("#command").focus();
		
		jQuery("#contents_wrapper").ajaxStart(function(){
			jQuery(this).block('<img src="/ajax_busy_round.gif">');
		});

		jQuery("#contents_wrapper").ajaxStop(function(){
			jQuery(this).unblock();
		});

		jQuery("#command").keyup(function(e){
			if (e.keyCode == 13) {
				jQuery.get("/ajax.cgi", { exec_ami: '"' + jQuery("#command").val() + '"' }, function(data){
					jQuery("#contents").val(data);
				});
			}
		});
	});

</script>

<table width="100%" border="0" cellpadding="6" cellspacing="0">
	<tr> 
		<td width="20%" valign="top" class="vncellreq">Command</td>
		<td width="80%" class="vtable">
			<input name="command" id="command" class="formfld" type="text" size="60">
		</td>
	</tr>
	<tr> 
		<td width="20%" class="vncellreq" valign="top">Output</td>
		<td width="80%" class="listr">
			<div id="contents_wrapper">
				<textarea wrap="nowrap" name="contents" cols="80" rows="21" id="contents" class="pre"></textarea>
			</div>
		</td>
	</tr>
</table>
<?php include("fend.inc"); ?>
