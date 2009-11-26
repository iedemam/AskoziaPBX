#!/usr/bin/php
<?php 
/*
	$Id: index.php 1104 2009-09-03 14:54:33Z michael.iedema $
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2009 IKT <http://itison-ikt.de>.
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


if ($_POST['report'] == "translation") {
	report_translation(
		$_POST['devmodever'],
		$_POST['page'],
		$_POST['language'],
		$_POST['original'],
		$_POST['improved']
	);

} else if ($_POST['report'] == "hardware") {
	report_hardware(
		$_POST['devmodever'],
		$_POST['description']
	);
}


function report_translation($devmodever, $page, $language, $original, $improved) {
	$mail_sent = mail(
		"localebot@askozia.com",
		"Localization Improvement (" . $language . ") " . $page,
		"developer mode version: " . $devmodever . "\n" .
		"page: " . $page . "\n" .
		"language: " . $language . "\n" .
		"\n" .
		"original:\n" .
		$original . "\n" .
		"\n" .
		"improved:\n" .
		$improved . "\n",
		"From: localebot@askozia.com\r\n"
	);

	if ($mail_sent) {
		echo "Submitted successfully. Thanks!";
	} else {
		echo "Failed to send. Check e-mail notifications settings.";
	}
}

function report_hardware($devmodever, $description) {
	$lspci = array();
	exec("lspci -v", $lspci);
	$mail_sent = mail(
		"hardwarebot@askozia.com",
		"Hardware Report",
		"developer mode version: " . $devmodever . "\n" .
		"description: " . $description . "\n" .
		"\n" .
		"lspci:\n" .
		implode("\n", $lspci) .
		"\n",
		"From: hardwarebot@askozia.com\r\n"
	);

	if ($mail_sent) {
		echo "Submitted successfully. Thanks!";
	} else {
		echo "Failed to send. Check e-mail notifications settings.";
	}
}

?>
