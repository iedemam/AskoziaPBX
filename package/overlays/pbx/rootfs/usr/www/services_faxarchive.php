#!/usr/bin/php
<?php 
/*
    $Id:$
    part of AskoziaPBX (http://askozia.com/pbx)
    
    Copyright (C) 2010 tecema (a.k.a IKT) <http://www.tecema.de>.
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

$pgtitle = array("Services", "Fax Archive");

if ($_GET['view']) {
	header("Content-Type: application/octet-stream"); 
	header("Content-Disposition: attachment; filename=" . basename($_GET['view']));
	passthru("cat " . $_GET['view']);
	exit;
}

include("fbegin.inc");

?><table width="100%" border="0" cellpadding="6" cellspacing="0">
	<tr>
		<td width="20%" valign="top" class="vncell"><?=gettext("All Virtual Faxes");?></td><?

	if ($disk = storage_service_is_active("faxarchive")) {
		$archivepath = $disk['mountpoint'] . "/askoziapbx/faxarchive/";
		$globber = glob($archivepath . "*.pdf");

		if (count($globber)) {
			?><td width="80%" class="vtable">
				<ul><?

				foreach ($globber as $match) {
					echo "<li><a href=\"?view=" . $match . "\">" . basename($match) . "</a></li>";
				}

				?></ul>
			</td><?
		}
	} else {
		?><td width="80%" class="vtable"><?=gettext("no faxes found");?></td><?
	}

	?></tr>
</table><?

include("fend.inc");
