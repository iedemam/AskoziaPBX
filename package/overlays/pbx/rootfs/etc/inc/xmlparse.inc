<?php
/*
	$Id$
	functions to parse/dump configuration files in XML format
	originally part of m0n0wall (http://m0n0.ch/wall)
	continued modifications as part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
	Copyright (C) 2007-2008 tecema (a.k.a IKT) <http://www.tecema.de>.
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

/* tags that are always to be handled as lists */
$listtags = explode(" ", 
	"key dnsserver route shellcmd cacert earlyshellcmd " .
	"phone provider codec room dialpattern callgroup groupmember incomingextensionmap " .
	"dahdi-port dahdi-portgroup redfone-gateway disk package loadzone bridge application application2 " .
	"manager-user read-permission write-permission app-command manual-attribute manual-file machine"
);

/* xml array used in parsing configuration files
 * XXX : this is a workaround for m0n0wall's global use of $config so it
 *		may be generalized for the packaging system. A lock/unlock has been
 *		added to control access to the parser but, in general, the solution
 *		is dirty and should be reimplemented. Oh the improvements
 *		to be made with OOP in the future
 */
$xml_array = array();

function startElement($parser, $name, $attrs) {
	global $depth, $curpath, $xml_array, $havedata, $listtags;

	array_push($curpath, strtolower($name));
	
	$ptr =& $xml_array;
	foreach ($curpath as $path) {
		$ptr =& $ptr[$path];
	}
	
	/* is it an element that belongs to a list? */
	if (in_array(strtolower($name), $listtags)) {
	
		/* is there an array already? */
		if (!is_array($ptr)) {
			/* make an array */
			$ptr = array();
		}
		
		array_push($curpath, count($ptr));
		
	} else if (isset($ptr)) {
		/* multiple entries not allowed for this element, bail out */
		die(sprintf("XML error: %s at line %d cannot occur more than once\n",
				$name,
				xml_get_current_line_number($parser)));
	}
	
	$depth++;
	$havedata = $depth;
}

function endElement($parser, $name) {
	global $depth, $curpath, $xml_array, $havedata, $listtags;
	
	if ($havedata == $depth) {
		$ptr =& $xml_array;
		foreach ($curpath as $path) {
			$ptr =& $ptr[$path];
		}
		$ptr = "";
	}
	
	array_pop($curpath);

	if (in_array(strtolower($name), $listtags))
		array_pop($curpath);
	
	$depth--;
}

function cData($parser, $data) {
	global $depth, $curpath, $xml_array, $havedata;
	
	$data = trim($data, "\t\n\r");
	
	if ($data != "") {
		$ptr =& $xml_array;
		foreach ($curpath as $path) {
			$ptr =& $ptr[$path];
		}

		if (is_string($ptr)) {
			$ptr .= $data;
		} else {
			if (trim($data, " ") != "") {
				$ptr = $data;
				$havedata++;
			}
		}
	}
}

function xml_to_array($cffile, $rootobj) {
	global $depth, $curpath, $xml_array, $havedata, $listtags;

	xml_parser_lock();

	$xml_array = array();
	$curpath = array();
	$depth = 0;
	$havedata = 0;

	$xml_parser = xml_parser_create();

	xml_set_element_handler($xml_parser, "startElement", "endElement");
	xml_set_character_data_handler($xml_parser, "cdata");

	if (!($fp = fopen($cffile, "r"))) {
		xml_parser_unlock();
		die("Error: could not open XML input\n");
	}

	while ($data = fread($fp, 4096)) {
		if (!xml_parse($xml_parser, $data, feof($fp))) {
			xml_parser_unlock();
			die(sprintf("XML error: %s at line %d\n",
						xml_error_string(xml_get_error_code($xml_parser)),
						xml_get_current_line_number($xml_parser)));
		}
	}
	xml_parser_free($xml_parser);

	if (!$xml_array[$rootobj]) {
		xml_parser_unlock();
		die("XML error: no $rootobj object found!\n");
	}

	fclose($fp);

	xml_parser_unlock();

	return $xml_array[$rootobj];
}

function array_to_xml_sub($arr, $indent) {

	global $listtags;
	
	$xmlconfig = "";

	foreach ($arr as $ent => $val) {
		if (is_array($val)) {
			/* is it just a list of multiple values? */
			if (in_array(strtolower($ent), $listtags)) {
				foreach ($val as $cval) {
					if (is_array($cval)) {
						$xmlconfig .= str_repeat("\t", $indent);
						$xmlconfig .= "<$ent>\n";
						$xmlconfig .= array_to_xml_sub($cval, $indent + 1);
						$xmlconfig .= str_repeat("\t", $indent);
						$xmlconfig .= "</$ent>\n";
					} else {
						$xmlconfig .= str_repeat("\t", $indent);
						if ((is_bool($cval) && ($cval == true)) ||
							($cval === ""))
							$xmlconfig .= "<$ent/>\n";
						else if (!is_bool($cval))
							$xmlconfig .= "<$ent>" . htmlspecialchars($cval) . "</$ent>\n";
					}
				}
			} else {
				/* it's an array */
				$xmlconfig .= str_repeat("\t", $indent);
				$xmlconfig .= "<$ent>\n";
				$xmlconfig .= array_to_xml_sub($val, $indent + 1);
				$xmlconfig .= str_repeat("\t", $indent);
				$xmlconfig .= "</$ent>\n";
			}
		} else {
			if ((is_bool($val) && ($val == true)) || ($val === "")) {
				$xmlconfig .= str_repeat("\t", $indent);
				$xmlconfig .= "<$ent/>\n";
			} else if (!is_bool($val)) {
				$xmlconfig .= str_repeat("\t", $indent);
				$xmlconfig .= "<$ent>" . htmlspecialchars($val) . "</$ent>\n";
			}
		}
	}
	
	return $xmlconfig;
}

function array_to_xml($arr, $rootobj) {

	$xmlconfig = "<?xml version=\"1.0\"?" . ">\n";
	$xmlconfig .= "<$rootobj>\n";

	$xmlconfig .= array_to_xml_sub($arr, 1);

	$xmlconfig .= "</$rootobj>\n";

	return $xmlconfig;
}

/**
 * Locks access xml parser
 * Lock is invalidated after 10 seconds.
 */
function xml_parser_lock() {
	global $g;

	$lockfile = "{$g['varrun_path']}/xmlparser.lock";

	$n = 0;
	while ($n < 10) {
		/* open the lock file in append mode to avoid race condition */
		if ($fd = @fopen($lockfile, "x")) {
			/* succeeded */
			fclose($fd);
			return;
		} else {
			/* file locked, wait and try again */
			sleep(1);
			$n++;
		}
	}
}

/**
 * Unlocks access to the xmlparser
 */
function xml_parser_unlock() {
	global $g;

	$lockfile = "{$g['varrun_path']}/xmlparser.lock";

	if (file_exists($lockfile))
		unlink($lockfile);
}

?>
