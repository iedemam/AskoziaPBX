<body link="#0000CC" vlink="#0000CC" alink="#0000CC"><div id="allbody"><?

	include("dev.inc");

	?>
	<div id="user" align="center">
	<table width="100%" border="0" cellspacing="0" cellpadding="2">
		<tr valign="bottom"> 
			<td id="logo_topleft" align="center" height="65" valign="middle">
				<a href="index.php"><img src="logo.png" border="0"></a>
			</td>
			<td height="65" class="webgui_bg">
				<table border="0" cellspacing="0" cellpadding="0" width="100%">
					<tr>
						<td align="left" valign="bottom"><span class="tfrtitle">&nbsp;<?=gettext("webGUI Configuration");?></span></td>
		  				<td align="right" valign="bottom">
							<span class="hostname"><?=$config['system']['hostname'] . "." . $config['system']['domain'];?>&nbsp;</span>
						</td>
					</tr>
				</table>
			</td>
		</tr>
