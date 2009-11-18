<body link="#0000CC" vlink="#0000CC" alink="#0000CC">
	<script type="text/JavaScript">
	<!--

		var devmodeversion = "0.1";
		var devmodeactivated = false;

		jQuery(document).ready(function(){
			jQuery(document).keydown(function(e) {
				// ESCAPE key pressed
				if (e.keyCode == 27) {
					dev_mode_toggle();
				}
			});
		});

		function dev_mode_toggle() {
			if (!devmodeactivated) {
				jQuery('#devpanel').slideDown('slow');
				jQuery('.spannit').click(function(e) {
					var htmlStr = jQuery(this).html();
					jQuery('#oldstring').val(htmlStr);
					jQuery('#newstring').val(htmlStr);
					jQuery('#devlocalepanel').slideDown('slow');
					return false;
				});
				jQuery('.spannit').mouseover(function(){
					jQuery(this).css('border','2px solid red');
				}).mouseout(function(){
					jQuery(this).css('border','0px solid red');
				});
				devmodeactivated = true;

			} else {
				jQuery('#devpanel').slideUp('slow');
				jQuery('.spannit').unbind('click');
				jQuery('.spannit').unbind('mouseover');
				jQuery('.spannit').unbind('mouseout');
				devmodeactivated = false;
			}
		}

		function dev_submit_locale() {
			jQuery.post("dev.php",
				{
					devmodever: devmodeversion,
					page: '<?=$_SERVER["SCRIPT_NAME"];?>',
					language: '<?=$current_language;?>',
					original: jQuery("#oldstring").val(),
					improved: jQuery("#newstring").val()
				},
				function(data){
					alert(data);
					jQuery("#devlocalepanel").slideUp("slow");
				});
		}

	//-->
	</script>
	<div id="devpanel" class="devpanel">
		<p><strong>Developer Mode Activated</strong>&nbsp;<em>(pre-alpha)</em>&nbsp;:</p>
		<p>Submit Localization Improvements:
		<ul>
			<li>E-Mail notifications must be configured correctly to submit changes.</li>
			<li>Compatible strings react when moused-over.</li>
			<li>Click on the string to open a submission form.</li>
		</ul></p>
		<br>
		<div id="devlocalepanel" class="devlocalepanel">
			<textarea id="oldstring" cols="30" rows="3" class="notes" disabled="true"></textarea>
			<textarea id="newstring" cols="30" rows="3" class="notes"></textarea>
			<br><a href="javascript:{}" onclick='jQuery("#devlocalepanel").slideUp("slow");'>cancel</a>&nbsp;|&nbsp;<a href="javascript:{}" onclick="dev_submit_locale();">submit locale improvement</a>
		</div>
	</div>
	<table width="750" border="0" cellspacing="0" cellpadding="2">
		<tr valign="bottom"> 
			<td width="150" height="65" align="center" valign="middle">
				<strong><a href="http://askozia.com/pbx" target="_blank"><img src="logo.png" border="0"></a></strong>
			</td>
			<td height="65" class="webgui_bg">
				<table border="0" cellspacing="0" cellpadding="0" width="100%">
					<tr>
						<td align="left" valign="bottom"><span class="tfrtitle">&nbsp;<?=spanify(gettext("webGUI Configuration"));?></span></td>
		  				<td align="right" valign="bottom">
							<span class="hostname"><?=$config['system']['hostname'] . "." . $config['system']['domain'];?>&nbsp;</span>
						</td>
					</tr>
				</table>
			</td>
		</tr>
