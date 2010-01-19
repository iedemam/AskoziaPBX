<?php
/*
	$Id$
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
?>

<style type="text/css">

	#devpanel {
		z-index: 50;
		width: 95%;
		padding: 10px;
		margin-bottom: 10px;
		display: none;
	}

	#devtranslationpanel {
		width: 100%;
		display: none;
	}

	#devfilepanel {
		width: 100%;
		display: none;
	}

	.devoutput {
		border: 2px solid #435370;
		background: #F0F0F0;
		padding: 1em;
		font-family: courier new, courier;
		white-space: pre;
		line-height: 10pt;
		font-size: 10pt;
	}

	.devfixedwidth {
		font-family: Courier New, Courier, monospaced;
		font-size: 10px;
	}

</style>
<script type="text/JavaScript">
<!--

	var devmodeversion = "0.2";
	var devmodeactivated = false;

	jQuery(document).ready(function(){
		dev_mode_setup();
	});


	//--- MAIN PANEL ---//
	function dev_mode_setup() {
		// set up the panel tabs
		jQuery('#devtabs').tabs().bind('tabsselect', function(event, ui) {
			var tmp = ui.tab.toString();
			// console tab selected
			if (tmp.indexOf('#devtabs-1') != -1) {
				dev_translation_disable();
			// ami tab selected
			} else if (tmp.indexOf('#devtabs-2') != -1) {
				dev_translation_disable();
			// file tab selected
			} else if (tmp.indexOf('#devtabs-3') != -1) {
				dev_translation_disable();
			// translation tab selected
			} else if (tmp.indexOf('#devtabs-4') != -1) {
				dev_translation_enable();
			// hardware tab selected
			} else if (tmp.indexOf('#devtabs-5') != -1) {
				dev_translation_disable();
			}
		});

		// listen for the ESC key to toggle dev mode
		jQuery(document).keydown(function(e) {
			// ESCAPE key pressed
			if (e.keyCode == 27) {
				dev_mode_toggle();
			}
		});

		// listen for the ENTER key to submit a console command
		jQuery("#devconsolecmd").keyup(function(e){
			if (e.keyCode == 13) {
				dev_console_execute();
			}
		});

		// listen for the ENTER key to submit an ami command
		jQuery("#devamicmd").keyup(function(e){
			if (e.keyCode == 13) {
				dev_ami_execute();
			}
		});
	}
	function dev_mode_toggle() {
		if (!devmodeactivated) {
			jQuery('#devpanel').slideDown('slow');
			devmodeactivated = true;

		} else {
			jQuery('#devpanel').slideUp('slow');
			dev_translation_disable();
			devmodeactivated = false;
		}
	}


	//--- CONSOLE PANEL ---//
	function dev_console_execute() {
		var command = jQuery("#devconsolecmd").val();
		jQuery.get("cgi-bin/ajax.cgi", { exec_shell: command }, function(data){
			jQuery("#devconsoleoutput").text('$ ' + command + '\n' + data);
		});
		jQuery("#devconsolecmd").focus().select();
	}


	//--- AMI PANEL ---//
	function dev_ami_execute() {
		var command = "'" + jQuery("#devamicmd").val() + "'";
		jQuery.get("cgi-bin/ajax.cgi", { exec_ami: command }, function(data){
			jQuery("#devamioutput").text(data);
		});
		jQuery("#devamicmd").focus().select();
	}


	//--- TRANSLATION PANEL ---//
	function dev_translation_submit() {
		jQuery.post("dev.php",{
			report: "translation",
			devmodever: devmodeversion,
			page: '<?=$_SERVER["SCRIPT_NAME"];?>',
			language: '<?=$current_language;?>',
			original: jQuery("#devlocaleoldstring").val(),
			improved: jQuery("#devlocalenewstring").val()

		},function(data){
			alert(data);
			jQuery("#devtranslationpanel").slideUp("slow");
		});
	}
	function dev_translation_enable() {
		jQuery('.spannit').click(function(e) {
			var htmlStr = jQuery(this).html();
			jQuery('#devlocaleoldstring').val(htmlStr);
			jQuery('#devlocalenewstring').val(htmlStr);
			jQuery('#devtranslationpanel').slideDown('slow');
			return false;
		}).mouseover(function(){
			jQuery(this).css('border','2px solid red');
		}).mouseout(function(){
			jQuery(this).css('border','0px solid red');
		});
	}
	function dev_translation_disable() {
		jQuery('.spannit').unbind('click');
		jQuery('.spannit').unbind('mouseover');
		jQuery('.spannit').unbind('mouseout');
	}


	//--- HARDWARE PANEL ---//
	function dev_hardware_submit() {
		jQuery.post("dev.php",{
			report: "hardware",
			devmodever: devmodeversion,
			description: jQuery("#devhardwaredescription").val()

		},function(data){
			alert(data);
			jQuery("#devhardwaredescription").val("");
		});
	}


	//--- FILE PANEL ---//
	function dev_file_get() {
		jQuery.get("dev.php?genfile=" + jQuery("#devfilelist").val(),function(data){                                                      
			jQuery("#devfilecontents").val(data);
		});
		jQuery.getJSON("dev.php?getfile=" + jQuery("#devfilelist").val(),function(data){                                                      
			jQuery("#devfilemanualcontents").val(data['contents']);
			if (data['mode'] == 'append') {
				jQuery("input[name=devfilemode]")[0].checked = true;
			} else {
				jQuery("input[name=devfilemode]")[1].checked = true;
			}
			dev_file_edit_start();
		});
	}
	function dev_file_put() {
		jQuery.post("dev.php",{
			putfile: "yes",
			fullpath: jQuery("#devfilelist").val(),
			contents: jQuery("#devfilemanualcontents").val(),
			mode: jQuery("input[name=devfilemode]:checked").val()

		},function(data){
			dev_file_edit_stop();
		});
	}
	function dev_file_edit_start() {
		jQuery("#devfilelist").attr("disabled","disabled");
		jQuery("#devfilepanel").slideDown("slow");
	}
	function dev_file_edit_stop() {
		jQuery("#devfilepanel").slideUp("slow");
		jQuery("#devfilelist").removeAttr("disabled");
	}


//-->
</script>
<div id="devpanel">
	<h1>Integrator Panel</h1>
	<div id="devtabs">
		<ul>
			<li><a href="#devtabs-1">Console</a></li>
			<li><a href="#devtabs-2">AMI</a></li>
			<li><a href="#devtabs-3">Files</a></li>
			<li><a href="#devtabs-4">Fix Translations</a></li>
			<li><a href="#devtabs-5">Report Hardware</a></li>
		</ul>
		<div id="devtabs-1">
			<input id="devconsolecmd" type="text" size="60" value="">
			<br><pre id="devconsoleoutput" class="devoutput">...enter command above</pre>
		</div>
		<div id="devtabs-2">
			<input id="devamicmd" type="text" size="60" value="">
			<br><pre id="devamioutput" class="devoutput">...enter command above</pre>
		</div>
		<div id="devtabs-3">
			<select name="devfilelist" id="devfilelist">
				<option value="/etc/asterisk/asterisk.conf">/etc/asterisk/asterisk.conf</option>
				<option value="/etc/asterisk/chan_dahdi.conf">/etc/asterisk/chan_dahdi.conf</option>
				<option value="/etc/asterisk/extensions.conf">/etc/asterisk/extensions.conf</option>
				<option value="/etc/asterisk/features.conf">/etc/asterisk/features.conf</option>
				<option value="/etc/asterisk/iax.conf">/etc/asterisk/iax.conf</option>
				<option value="/etc/asterisk/indications.conf">/etc/asterisk/indications.conf</option>
				<option value="/etc/asterisk/manager.conf">/etc/asterisk/manager.conf</option>
				<option value="/etc/asterisk/meetme.conf">/etc/asterisk/meetme.conf</option>
				<option value="/etc/asterisk/modules.conf">/etc/asterisk/modules.conf</option>
				<option value="/etc/asterisk/rtp.conf">/etc/asterisk/rtp.conf</option>
				<option value="/etc/asterisk/sip.conf">/etc/asterisk/sip.conf</option>
				<option value="/etc/asterisk/voicemail.conf">/etc/asterisk/voicemail.conf</option>
				<option value="/etc/dahdi/system.conf">/etc/dahdi/system.conf</option>
				<option value="/var/spool/cron/crontabs/root">/var/spool/cron/crontabs/root</option>
			</select>
			&nbsp;<a href="javascript:{}" onclick="dev_file_get();" class="ui-state-default ui-corner-all">edit</a>
			<br>
			<br>
			<div id="devfilepanel">
				<strong>System Auto-Generated Content</strong><br>
				<textarea id="devfilecontents" cols="80" rows="15" class="devfixedwidth" disabled="true"></textarea>
				<br>
				<br>
				<br>
				<strong>Append</strong><input name="devfilemode" type="radio" value="append">
				or <strong>Override</strong><input name="devfilemode" type="radio" value="override">
				with this text
				<br><textarea id="devfilemanualcontents" cols="80" rows="15" class="devfixedwidth"></textarea>
				<br>
				<a href="javascript:{}" onclick='dev_file_edit_stop();' class="ui-state-default ui-corner-all">cancel</a>
				<a href="javascript:{}" onclick="dev_file_put();" class="ui-state-default ui-corner-all">save file contents</a>
			</div>
		</div>
		<div id="devtabs-4">
			<ul>
				<li>Compatible strings react when moused-over.</li>
				<li>Click on the string to open a submission form.</li>
				<li><strong>Note:</strong> E-Mail notifications must be configured correctly to submit reports.</li>
			</ul>
			<br>
			<div id="devtranslationpanel">
				<textarea id="devlocaleoldstring" cols="30" rows="3" disabled="true"></textarea>
				<textarea id="devlocalenewstring" cols="30" rows="3"></textarea>
				<br>
				<a href="javascript:{}" onclick='jQuery("#devtranslationpanel").slideUp("slow");' class="ui-state-default ui-corner-all">cancel</a>
				<a href="javascript:{}" onclick="dev_translation_submit();" class="ui-state-default ui-corner-all">submit locale improvement</a>
			</div>
		</div>
		<div id="devtabs-5">
			<ul>
				<li>Describe what is not being recognized on your system.</li>
				<li>Technical details about your hardware will automatically be submitted.</li>
				<li><strong>Note:</strong> E-Mail notifications must be configured correctly to submit reports.</li>
			</ul>
			<br>
			<textarea id="devhardwaredescription" cols="60" rows="6"></textarea>
			<br>
			<a href="javascript:{}" onclick="dev_hardware_submit();" class="ui-state-default ui-corner-all">submit hardware report</a>
		</div>
	</div>
</div>
