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
$pgtitle = array(gettext("Dialplan"), gettext("Edit Application"));
$needs_codemirror = true;

if ($_POST) {
	unset($input_errors);

	$application = applications_verify_application(&$_POST, &$input_errors);
	if (!$input_errors) {
		applications_save_application($application);
		header("Location: dialplan_applications.php");
		exit;
	}
}


$colspan = 1;
$carryovers = array(
	"uniqid",
	"type"
);

$uniqid = $_GET['uniqid'];
if (isset($_POST['uniqid'])) {
	$uniqid = $_POST['uniqid'];
}
$type = $_GET['type'];
if (isset($_POST['type'])) {
	$type = $_POST['type'];
}

if ($_POST) {
	$form = $_POST;
} else if ($uniqid) {
	$form = applications_get_application($uniqid);
} else {
	$form = applications_generate_default_application($type);
}


include("fbegin.inc");
d_start("dialplan_applications_edit.php");


	// General
	d_header(gettext("General Settings"));

	d_field(gettext("Name"), "name", 40,
		false, "required");

	d_field(gettext("Number"), "extension", 20,
		gettext("The number or pattern used to dial this application."), "required");
		// verify pattern and extension for uniqueness and also validity, toggle show pattern help here

	d_field(gettext("Description"), "descr", 40,
		gettext("You may enter a description here for your reference (not parsed)."));
	d_spacer();


	// Security
	d_header(gettext("Security"));

	display_public_access_editor($form['publicaccess'], $form['publicname'], 1);

	d_spacer();


	// Application Logic
	d_header(gettext("Application Logic"));

	d_codemirror(base64_decode($form['applicationlogic']));

	d_spacer();


d_submit();
include("fend.inc");
