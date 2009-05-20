#!/usr/bin/php
<?php 
/*
	$Id: index.php 610 2008-06-26 12:18:23Z kryysler $
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

if ($_GET['new']) {
	$url = $_GET['new'];
	$file = basename($url);
	$output = exec("/usr/bin/fetch -s $url");
	if (is_numeric($output)) {
		echo $output;
		if (file_exists("/ultmp/$file")) {
			unlink("/ultmp/$file");
		}
		exec("nohup /usr/bin/fetch -a -w 2 -q -o /ultmp/$file $url > /dev/null 2>&1 &");
	} else if (strpos($output, "Not Found") !== false) {
		echo "file not found";
	} else {
		echo "unknown error";
	}
}

?>
