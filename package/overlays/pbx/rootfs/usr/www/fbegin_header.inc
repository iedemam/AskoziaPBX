<body link="#0000CC" vlink="#0000CC" alink="#0000CC"><?

	include("dev.inc");

	?><table width="750" border="0" cellspacing="0" cellpadding="2">
		<tr valign="bottom"> 
			<td width="150" height="65" align="center" valign="middle">
				<strong><a href="index.php"><img src="logo.png" border="0"></a></strong>
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
