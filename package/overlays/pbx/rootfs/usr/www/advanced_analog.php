#!/usr/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2009 IKT <http://itison-ikt.de>.
	All rights reserved.
	
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
	
	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.
	
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	
	THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
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

$pgtitle = array(gettext("Advanced"), gettext("Analog"));

$form['loadzone'] = dahdi_get_loadzones();


if ($_POST) {

	unset($input_errors);
	$form = $_POST;

	parse_str($_POST['loadzones']);
	$form['loadzone'] = $gme;

	if (!$input_errors) {
		dahdi_save_loadzones($form['loadzone']);
		header("Location: advanced_analog.php");
		exit;
	}
}

if (file_exists($g['dahdi_dirty_path'])) {
	$retval = 0;
	if (!file_exists($d_sysrebootreqd_path)) {
		config_lock();
		$retval |= dahdi_configure();
		$retval |= pbx_restart();
		config_unlock();
	}

	$savemsg = get_std_save_message($retval);
}

include("fbegin.inc");

$dahdi_loadzones = array(
	"us" => gettext("United States / North America"),
	"au" => gettext("Australia"),
	"at" => gettext("Austria"),
	"be" => gettext("Belgium"),
	"br" => gettext("Brazil"),
	"bg" => gettext("Bulgaria"),
	"cl" => gettext("Chile"),
	"cn" => gettext("China"),
	"cz" => gettext("Czech Republic"),
	"dk" => gettext("Denmark"),
	"ee" => gettext("Estonia"),
	"fi" => gettext("Finland"),
	"fr" => gettext("France"),
	"de" => gettext("Germany"),
	"gr" => gettext("Greece"),
	"hu" => gettext("Hungary"),
	"in" => gettext("India"),
	"il" => gettext("Israel"),
	"it" => gettext("Italy"),
	"jp" => gettext("Japan"),
	"lt" => gettext("Lithuania"),
	"my" => gettext("Malaysia"),
	"mx" => gettext("Mexico"),
	"nl" => gettext("Netherlands"),
	"nz" => gettext("New Zealand"),
	"no" => gettext("Norway"),
	"ph" => gettext("Philippines"),
	"pl" => gettext("Poland"),
	"pt" => gettext("Portugal"),
	"ru" => gettext("Russian Federation / Former Soviet Union"),
	"sg" => gettext("Singapore"),
	"za" => gettext("South Africa"),
	"es" => gettext("Spain"),
	"se" => gettext("Sweden"),
	"ch" => gettext("Switzerland"),
	"tw" => gettext("Taiwan"),
	"th" => gettext("Thailand"),
	"uk" => gettext("United Kingdom"),
	"us-old" => gettext("United States Circa 1950/ North America"),
	"ve" => gettext("Venezuela / South America")
);

?><form action="advanced_analog.php" method="post" name="iform" id="iform">
	<table width="100%" border="0" cellpadding="6" cellspacing="0">
		<tr> 
			<td width="20%" valign="top" class="vncell"><?=gettext("Tone Zones");?></td>
			<td width="40%" class="vtable" valign="top"><strong><?=gettext("Loaded");?></strong>&nbsp;<i>(<?=gettext("drag-and-drop");?>)</i>
				<ul id="gme" class="gme"><? 

				foreach ($form['loadzone'] as $loadzone) {
					if (array_key_exists($loadzone, $dahdi_loadzones)) {
						?><li class="gme" id="gme[]_<?=$loadzone;?>"><?=htmlspecialchars($dahdi_loadzones[$loadzone]);?></li><?
					}
				}

				?></ul>
			</td>
			<td width="40%" class="vtable" valign="top"><strong><?=gettext("Inactive");?></strong>
				<ul id="gmd" class="gmd"><?

				foreach ($dahdi_loadzones as $abbreviation=>$friendly) {
					if (!in_array($abbreviation, $form['loadzone'])) {
						?><li class="gmd" id="gmd[]_<?=$abbreviation;?>"><?=htmlspecialchars($dahdi_loadzones[$abbreviation]);?></li><?
					}
				}

				?></ul>
			</td>
		</tr>
		<tr>
			<td width="20%" valign="top" class="vncell">&nbsp;</td>
			<td width="40%" class="vtable" valign="top" colspan="2">
				<span class="vexpl"><strong><span class="red"><?=gettext("Note:");?></span></strong>
				<?=gettext("Select all indication tone zones the analog hardware should support. <strong>The first tone zone will be the default tonezone.");?></strong></span>
			</td>
		</tr>
		<tr> 
			<td valign="top">&nbsp;</td>
			<td colspan="2">
				<input name="Submit" type="submit" class="formbtn" value="Save" onclick="save_loadzone_states()">
				<input id="loadzones" name="loadzones" type="hidden" value="">
			</td>
		</tr>
	</table>
</form>
<script type="text/javascript" charset="utf-8">
// <![CDATA[

	jQuery(document).ready(function() {
		jQuery('ul.gme').sortable({ connectWith: ['ul.gmd'], revert: true });
		jQuery('ul.gmd').sortable({ connectWith: ['ul.gme'], revert: true });
	});

	function save_loadzone_states() {
		var gms = document.getElementById('loadzones');
		gms.value = jQuery('ul.gme').sortable('serialize', {key: 'gme[]'});
	}
// ]]>
</script>
<? include("fend.inc"); ?>
