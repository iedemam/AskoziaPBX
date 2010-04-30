#!/usr/bin/php
<?php 
/*
	$Id$
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
$pgtitle = array(gettext("Ports"), gettext("Edit Provider Group"));


if ($_POST) {
	unset($input_errors);

	$group = dahdi_verify_portgroup(&$_POST, &$input_errors);
	if (!$input_errors) {
		dahdi_save_portgroup($group);
		header("Location: ports_" . $group['technology'] . ".php");
		exit;
	}
}


$colspan = 2;
$carryovers = array(
	"number", "technology", "type", "uniqid"
);

$uniqid = $_GET['uniqid'];
if (isset($_POST['uniqid'])) {
	$uniqid = $_POST['uniqid'];
}
$technology = $_GET['technology'];
if (isset($_POST['technology'])) {
	$technology = $_POST['technology'];
}


if ($_POST) {
	$form = $_POST;
} else if ($uniqid) {
	$form = dahdi_get_portgroup($uniqid);
} else {
	$form = dahdi_generate_default_portgroup($technology);
	if ($form == false) {
		$no_available_groupnumbers = true;
	}
}

$all_uniqids = array();
$all_ports = dahdi_get_ports($form['technology'], $form['type']);
foreach ($all_ports as $port) {
	$all_uniqids[] = $port['uniqid'];
}


include("fbegin.inc");
d_start("ports_groups_edit.php");


	// General
	d_header(gettext("General Settings"));

	d_field(gettext("Name"), "name", 40,
		false, "required");

	d_drag_and_drop(gettext("Ports"), $form['groupmember'], $all_uniqids);


d_submit();
include("fend.inc");