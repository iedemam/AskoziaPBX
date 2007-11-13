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

function js_incoming_extension_selector($current_incomingextensionmap, $multiple=false) {
	
	$options[] = array("", "Outgoing Calls Only");
	$options[] = array("", "");
	$pad = "&nbsp;&nbsp;&nbsp;";

	if (count($a_items = dialplan_get_callgroups())) {
		$options[] = array("", "Call Groups");
		foreach ($a_items as $item) {
			$options[] = array($item['uniqid'], $pad . $item['name']);
		}
	}
	
	if (count($a_items = sip_get_phones())) {
		$options[] = array("", "SIP Phones");
		foreach ($a_items as $item) {
			$options[] = array($item['uniqid'], $pad . $item['callerid'] . " &lt;" . $item['extension'] . "&gt;");
		}
	}
	
	if (count($a_items = iax_get_phones())) {
		$options[] = array("", "IAX Phones");
		foreach ($a_items as $item) {
			$options[] = array($item['uniqid'], $pad . $item['callerid'] . " &lt;" . $item['extension'] . "&gt;");
		}
	}
	
	if (count($a_items = isdn_get_phones())) {
		$options[] = array("", "ISDN Phones");
		foreach ($a_items as $item) {
			$options[] = array($item['uniqid'], $pad . $item['callerid'] . " &lt;" . $item['extension'] . "&gt;");
		}
	}
	
	if (count($a_items = analog_get_phones())) {
		$options[] = array("", "Analog Phones");
		foreach ($a_items as $item) {
			$options[] = array($item['uniqid'], $pad . $item['callerid'] . " &lt;" . $item['extension'] . "&gt;");
		}
	}
	
	if (count($a_items = external_get_phones())) {
		$options[] = array("", "External Phones");
		foreach ($a_items as $item) {
			$options[] = array($item['uniqid'], $pad . $item['name'] . " &lt;" . $item['dialstring'] . "&gt;");
		}
	}
	
	if (count($a_items = conferencing_get_rooms())) {
		$options[] = array("", "Conference Rooms");
		foreach ($a_items as $item) {
			$options[] = array($item['uniqid'], $pad . $item['name'] . " &lt;" . $item['number'] . "&gt;");
		}
	}
	
	if (count($a_items = dialplan_get_applications())) {
		$options[] = array("", "Applications");
		foreach ($a_items as $item) {
			$options[] = array($item['uniqid'], $pad . $item['name']);
		}
	}
	
	$n = count($options);
	for ($i = 0; $i < $n; $i++) {
		$add_options_string .= "\t\t+ '<option value=\"{$options[$i][0]}\">{$options[$i][1]}</option>'\n";
	}

	echo <<<EOD


var next_incoming_extension_selector_id = 0;

function add_incoming_extension_selector(destination_id, pattern, provider_id) {
	if (pattern === undefined) {
		pattern = '';
	}
	
	var id = next_incoming_extension_selector_id;

	var insert_string =
		'<tr id="incoming_extension_selector_' + id + '">'
		+ '<td>'
		+ '<input name="incomingpattern_' + id + '" type="text" class="formfld" '
		+ 'id="incomingpattern_' + id + '" size="20" value="' + pattern + '">'
		+ '<select name="destinationid_' + id + '" class="formfld" id="destinationid_' + id + '">'
		$add_options_string
		+ '</select>'
		+ '</td>';

	if (next_incoming_extension_selector_id == 0) {
		insert_string += '<td valign="middle">&nbsp;</td></tr>';

	} else {
		insert_string += '<td valign="middle">'
			+ '<img src="x.gif" title="remove incoming extension mapping" width="17" height="17" border="0" '
			+ 'style="cursor: pointer; cursor: hand;" onclick="remove_incoming_extension_selector(' + id + ')">'
			+ '</td>' +
			'</tr>';
	}
	
	jQuery("#incoming_extension_selector_last").before(insert_string);

	if (destination_id !== undefined) {
		jQuery('#destinationid_' + id).selectOptions(destination_id, true);
	}

	next_incoming_extension_selector_id++;		
}

function remove_incoming_extension_selector(id) {

	jQuery('#incoming_extension_selector_' + id).remove();

}


EOD;

	if (!count($current_incomingextensionmap)) {
		echo "add_incoming_extension_selector();";
	} else {
		foreach ($current_incomingextensionmap as $map) {
			if (isset($map['incomingpattern'])) {
				echo "add_incoming_extension_selector(\"" . 
					$map['destinationid'] . "\", \"" . $map['incomingpattern'] . "\");";
			} else {
				echo "add_incoming_extension_selector(\"" . $map['destinationid'] . "\");";
			}
		}
	}

}

?>
