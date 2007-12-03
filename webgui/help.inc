<?php
/*
	$Id: asterisk.inc 142 2007-06-28 12:55:11Z michael.iedema $
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

$help_language = "en";

$help = array(

	"en" => array(
		
		"display_incoming_extension_selector" =>
			"Enter a pattern or a phone number and select a destination extension. ".
			"To ring multiple phones, simply define them with the same pattern. ".
			"If a phone number is entered, it will used as the \"read back\" number on unanswered calls. ".
			"If nothing is entered, all calls will be directed to the destination extension.<br>\n".
			"<ul>\n".
			"\t<li><strong>X</strong> - matches digits 0-9</li>\n".
			"\t<li><strong>Z</strong> - matches digits 1-9</li>\n".
			"\t<li><strong>N</strong> - matches digits 2-9</li>\n".
			"\t<li><strong>[13-5]</strong> - matches any digit in the brackets (here, 1,3,4,5)</li>\n".
			"\t<li><strong>.</strong> - matches one or more characters</li>\n".
			"\t<li><strong>!</strong> - matches zero or more characters</li>\n".
			"</ul>\n".
			"<strong><span class=\"red\">Note:</span></strong>&nbsp;".
			"Permission must still be given to this phone for it to place outgoing calls.",

		"display_phone_access_selector" => 
			"Checked phones will have access to this provider.",

		"display_provider_dialpattern_editor" => 
			"Enter patterns, one per line, to define this provider's routing. Prefixes can be defined as shown in the following example. A prefix of \"9\" is equivalent to a pattern of \"9|.\"). If no patterns are entered, only incoming calls will be accepted from this provider.<br>\n".
			"<ul>\n".
			"\t<li><strong>+</strong> - adds a prefix (i.e. \"1+555\" matches \"555\" and passes \"1555\" to the provider)</li>\n".
			"\t<li><strong>|</strong> - removes a prefix (i.e. \"1|NXX\" matches \"1555\" but only passes \"555\" to the provider)</li>\n".
			"\t<li><strong>X</strong> - matches digits 0-9</li>\n".
			"\t<li><strong>Z</strong> - matches digits 1-9</li>\n".
			"\t<li><strong>N</strong> - matches digits 2-9</li>\n".
			"\t<li><strong>[13-5]</strong> - matches any digit in the brackets (here, 1,3,4,5)</li>\n".
			"\t<li><strong>.</strong> - matches one or more characters (not allowed before | or +)</li>\n".
			"\t<li><strong>!</strong> - matches zero or more characters (not allowed before | or +)</li>\n".
			"</ul>\n"
	)
	
);


?>