<?php

$audio_codecs = array(
	"ulaw" => "G.711 u-law",
	"alaw" => "G.711 A-law",
	"gsm" => "GSM",
	"g729" => "G.729A",
	"ilbc" => "iLBC",
	"speex" => "SpeeX",
	"g723" => "G.723.1",
	"g726aal2" => "G.726 AAL2",
	"adpcm" => "ADPCM",
	"slin" => "16 bit Signed Linear PCM",
	"lpc10" => "LPC10",
	"g726" => "G.726 RFC3551",
	"g722" => "G722"
);

$pconfig['codec'] = array("ulaw", "alaw", "gsm", "ilbc");

if ($_POST) {
	parse_str($_POST['a_codecs']);
	$pconfig['codec'] = $ace;
}


?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
	"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
	<head>
		<title>Codecs Sorter</title>
		<meta http-equiv="content-type" content="text/html; charset=utf-8" />
		<script src="scriptaculous/lib/prototype.js" type="text/javascript"></script>
		<script src="scriptaculous/src/scriptaculous.js" type="text/javascript"></script>
		<link rel="stylesheet" href="scriptaculous/test/test.css" type="text/css" />
		<link rel="stylesheet" href="gui.css" type="text/css" />
		<style type="text/css" media="screen">
	    #ace li {
			margin: 5px auto;
			padding: 2px;
			width: 100%;
			text-align:left;
			list-style-type:none;
			border: 2px solid #779;
			background-color: #dde
	   }
	    #acd li {
			margin: 5px auto;
			padding: 2px;
			width: 100%;
			text-align:left;
			list-style-type:none;
			border: 2px solid #779;
			background-color: #FFCC66
	   }	
	  </style>		
	</head>
	<body>
		<form action="sortable.php" method="post" name="iform" id="iform">
			<table width="600" border="0" cellpadding="10" cellspacing="0">
				<tr> 
					<td width="20%" valign="top" class="vncell">Audio Codecs</td>
					<td width="40%" class="vtable" valign="top"><strong>Enabled</strong>
						<ul id="ace">
							<? foreach ($pconfig['codec'] as $codec): ?>
							<li id="ace_<?=$codec;?>"><?=$audio_codecs[$codec];?></li>
							<? endforeach; ?>
						</ul>
					</td>
					<td width="40%" class="vtable" valign="top"><strong>Disabled</strong>
						<ul id="acd">
						<? foreach ($audio_codecs as $codec=>$friendly): ?>
							<? if (!in_array($codec,$pconfig['codec'])): ?>
							<li id="acd_<?=$codec;?>"><?=$friendly;?></li>
							<? endif; ?>
						<? endforeach; ?>
						</ul>
					</td>
				</tr>
				<tr> 
					<td width="20%" valign="top">&nbsp;</td>
					<td width="80%" colspan="2"> <input name="Submit" type="submit" class="formbtn" value="Save" onclick="save_codec_states()">
					<input id="a_codecs" name="a_codecs" type="hidden" value=""></td>
				</tr>
			</table>
		</form>		
		<script type="text/javascript" charset="utf-8">
		// <![CDATA[
			Sortable.create("ace",
				{dropOnEmpty:true,containment:["ace","acd"],constraint:false});
			Sortable.create("acd",
				{dropOnEmpty:true,containment:["ace","acd"],constraint:false});
			
			function save_codec_states() {
				var acs = document.getElementById('a_codecs');
				acs.value = Sortable.serialize('ace');
			}
		// ]]>			
		</script>		
	</body>	
</html>