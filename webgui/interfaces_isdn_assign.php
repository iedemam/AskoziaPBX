#!/usr/local/bin/php
<?php 
/*
	$Id: interfaces_isdn.php 151 2007-07-13 17:11:32Z michael.iedema $
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

$pgtitle = array("Interfaces", "Assign ISDN Ports");
require("guiconfig.inc");

$hfc_lines = isdn_get_recognized_interfaces();

?>
<?php include("fbegin.inc"); ?>
<form action="interfaces_isdn_assign.php" method="post" name="iform" id="iform">
<?php if ($input_errors) print_input_errors($input_errors); ?>
<?php if ($savemsg) print_info_box($savemsg); ?>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tr>
		<td class="tabnavtbl">
			<ul id="tabnav"><?
	
			$tabs = array('Interfaces' => 'interfaces_isdn.php',
						'Assign' => 'interfaces_isdn_assign.php');
			dynamic_tab_menu($tabs);
			
			?></ul>
		</td>
	</tr>
	<tr> 
		<td class="tabcont">
			<table width="100%" border="0" cellpadding="6" cellspacing="0">
				<tr> 
					<td colspan="2" valign="top" class="listtopic">ISDN configuration</td>
				</tr>
				<tr> 
					<td width="22%" valign="top" class="vncellreq">HFC Lines</td>
					<td width="78%" class="vtable"><?
					
					foreach($hfc_lines as $line) {
						echo "$line<br>\n";
					}
					
					?></td>
				</tr>
				<tr> 
					<td valign="top">&nbsp;</td>
					<td> 
						<input name="Submit" type="submit" class="formbtn" value="Save"> 
					</td>
				</tr>
				<tr> 
					<td width="22%" valign="top">&nbsp;</td>
					<td width="78%">
						<span class="vexpl"><span class="red"><strong>Warning:<br>
						</strong></span>after you click &quot;Save&quot;, all current
						calls will be dropped. You may also have to do one or more 
						of the following steps before you can access your pbx again: 
						<ul>
							<li>change the IP address of your computer</li>
							<li>access the webGUI with the new IP address</li>
						</ul>
						</span>
					</td>
				</tr>
			</table>
		</td>
	</tr>
</table>
</form>
<script language="JavaScript">
<!--
type_change();
//-->
</script>
<?php include("fend.inc"); ?>
