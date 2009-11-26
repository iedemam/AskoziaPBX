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
	
	.devoutput {
		border: 2px solid #435370;
		background: #F0F0F0;
		padding: 1em;
		font-family: courier new, courier;
		white-space: pre;
		line-height: 10pt;
		font-size: 10pt;
	}

</style>
<script type="text/JavaScript">
<!--

	var devmodeversion = "0.1";
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
			// translation tab selected
			} else if (tmp.indexOf('#devtabs-3') != -1) {
				dev_translation_enable();
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

//-->
</script>
<div id="devpanel">
	<h1>Developer Panel</h1>
	<div id="devtabs">
		<ul>
			<li><a href="#devtabs-1">Console</a></li>
			<li><a href="#devtabs-2">Asterisk Manager Interface</a></li>
			<li><a href="#devtabs-3">Fix a Translation</a></li>
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
			<ul>
				<li>E-Mail notifications must be configured correctly to submit changes.</li>
				<li>Compatible strings react when moused-over.</li>
				<li>Click on the string to open a submission form.</li>
			</ul>
			<br>
			<div id="devtranslationpanel">
				<textarea id="devlocaleoldstring" cols="30" rows="3" disabled="true"></textarea>
				<textarea id="devlocalenewstring" cols="30" rows="3"></textarea>
				<br><a href="javascript:{}" onclick='jQuery("#devtranslationpanel").slideUp("slow");'>cancel</a>&nbsp;|&nbsp;<a href="javascript:{}" onclick="dev_translation_submit();">submit locale improvement</a>
			</div>
		</div>
	</div>
</div>
