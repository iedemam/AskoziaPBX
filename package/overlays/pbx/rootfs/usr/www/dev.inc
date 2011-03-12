<?php
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)

	Copyright (C) 2009 tecema (a.k.a IKT) <http://www.tecema.de>.
	All rights reserved.

	Askozia®PBX is a registered trademark of tecema. Any unauthorized use of
	this trademark is prohibited by law and international treaties.
	
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
	
	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.
	
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	
	3. Redistribution in any form at a charge, that in whole or in part
	   contains or is derived from the software, including but not limited to
	   value added products, is prohibited without prior written consent of
	   tecema.
	
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

<script type="text/JavaScript">
<!--

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

			// ami tab selected
			} else if (tmp.indexOf('#devtabs-2') != -1) {

			// file tab selected
			} else if (tmp.indexOf('#devtabs-3') != -1) {

			// beta tab selected
			} else if (tmp.indexOf('#devtabs-4') != -1) {

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
	function dev_file_copycontents() {
		jQuery("#devfilemanualcontents").val(jQuery("#devfilecontents").val());
	}

//-->
</script>
<div id="devpanel">
	<h1>Integrator Panel</h1>
	<div id="devtabs">
		<ul>
			<li><a href="#devtabs-1">Console</a></li>
			<li><a href="#devtabs-2">AMI</a></li>
			<li><a href="#devtabs-3">Configuration Files</a></li>
			<li><a href="#devtabs-4">Beta Features</a></li>
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
				<option>Asterisk Configuration</option>
				<option value="/etc/asterisk/adsi.conf"> - adsi.conf</option>
				<option value="/etc/asterisk/adtranvofr.conf"> - adtranvofr.conf</option>
				<option value="/etc/asterisk/agents.conf"> - agents.conf</option>
				<option value="/etc/asterisk/ais.conf"> - ais.conf</option>
				<option value="/etc/asterisk/alarmreceiver.conf"> - alarmreceiver.conf</option>
				<option value="/etc/asterisk/alsa.conf"> - alsa.conf</option>
				<option value="/etc/asterisk/amd.conf"> - amd.conf</option>
				<option value="/etc/asterisk/asterisk.conf"> - asterisk.conf</option>
				<option value="/etc/asterisk/cdr.conf"> - cdr.conf</option>
				<option value="/etc/asterisk/cdr_adaptive_odbc.conf"> - cdr_adaptive_odbc.conf</option>
				<option value="/etc/asterisk/cdr_custom.conf"> - cdr_custom.conf</option>
				<option value="/etc/asterisk/cdr_manager.conf"> - cdr_manager.conf</option>
				<option value="/etc/asterisk/cdr_odbc.conf"> - cdr_odbc.conf</option>
				<option value="/etc/asterisk/cdr_pgsql.conf"> - cdr_pgsql.conf</option>
				<option value="/etc/asterisk/cdr_sqlite3_custom.conf"> - cdr_sqlite3_custom.conf</option>
				<option value="/etc/asterisk/cdr_tds.conf"> - cdr_tds.conf</option>
				<option value="/etc/asterisk/chan_dahdi.conf"> - chan_dahdi.conf</option>
				<option value="/etc/asterisk/chan_ooh323.conf"> - chan_ooh323.conf</option>
				<option value="/etc/asterisk/cli.conf"> - cli.conf</option>
				<option value="/etc/asterisk/cli_aliases.conf"> - cli_aliases.conf</option>
				<option value="/etc/asterisk/cli_permissions.conf"> - cli_permissions.conf</option>
				<option value="/etc/asterisk/codecs.conf"> - codecs.conf</option>
				<option value="/etc/asterisk/console.conf"> - console.conf</option>
				<option value="/etc/asterisk/dbsep.conf"> - dbsep.conf</option>
				<option value="/etc/asterisk/dnsmgr.conf"> - dnsmgr.conf</option>
				<option value="/etc/asterisk/dsp.conf"> - dsp.conf</option>
				<option value="/etc/asterisk/dundi.conf"> - dundi.conf</option>
				<option value="/etc/asterisk/extconfig.conf"> - extconfig.conf</option>
				<option value="/etc/asterisk/extensions.conf"> - extensions.conf</option>
				<option value="/etc/asterisk/extensions_minivm.conf"> - extensions_minivm.conf</option>
				<option value="/etc/asterisk/features.conf"> - features.conf</option>
				<option value="/etc/asterisk/festival.conf"> - festival.conf</option>
				<option value="/etc/asterisk/followme.conf"> - followme.conf</option>
				<option value="/etc/asterisk/func_odbc.conf"> - func_odbc.conf</option>
				<option value="/etc/asterisk/gsm.conf"> - gsm.conf</option>
				<option value="/etc/asterisk/gtalk.conf"> - gtalk.conf</option>
				<option value="/etc/asterisk/h323.conf"> - h323.conf</option>
				<option value="/etc/asterisk/http.conf"> - http.conf</option>
				<option value="/etc/asterisk/iax.conf"> - iax.conf</option>
				<option value="/etc/asterisk/iaxprov.conf"> - iaxprov.conf</option>
				<option value="/etc/asterisk/indications.conf"> - indications.conf</option>
				<option value="/etc/asterisk/jabber.conf"> - jabber.conf</option>
				<option value="/etc/asterisk/jingle.conf"> - jingle.conf</option>
				<option value="/etc/asterisk/logger.conf"> - logger.conf</option>  
				<option value="/etc/asterisk/manager.conf"> - manager.conf</option>
				<option value="/etc/asterisk/meetme.conf"> - meetme.conf</option>
				<option value="/etc/asterisk/mgcp.conf"> - mgcp.conf</option>
				<option value="/etc/asterisk/minivm.conf"> - minivm.conf</option>
				<option value="/etc/asterisk/misdn.conf"> - misdn.conf</option>
				<option value="/etc/asterisk/modules.conf"> - modules.conf</option>
				<option value="/etc/asterisk/musiconhold.conf"> - musiconhold.conf</option>
				<option value="/etc/asterisk/muted.conf"> - muted.conf</option>
				<option value="/etc/asterisk/osp.conf"> - osp.conf</option>
				<option value="/etc/asterisk/oss.conf"> - oss.conf</option>
				<option value="/etc/asterisk/phone.conf"> - phone.conf</option>
				<option value="/etc/asterisk/phoneprov.conf"> - phoneprov.conf</option>
				<option value="/etc/asterisk/phpagi.conf"> - phpagi.conf</option>
				<option value="/etc/asterisk/queuerules.conf"> - queuerules.conf</option>
				<option value="/etc/asterisk/queues.conf"> - queues.conf</option>
				<option value="/etc/asterisk/res_config_sqlite.conf"> - res_config_sqlite.conf</option>
				<option value="/etc/asterisk/res_ldap.conf"> - res_ldap.conf</option>
				<option value="/etc/asterisk/res_odbc.conf"> - res_odbc.conf</option>
				<option value="/etc/asterisk/res_pgsql.conf"> - res_pgsql.conf</option>
				<option value="/etc/asterisk/res_snmp.conf"> - res_snmp.conf</option>
				<option value="/etc/asterisk/rtp.conf"> - rtp.conf</option>
				<option value="/etc/asterisk/say.conf"> - say.conf</option>
				<option value="/etc/asterisk/sip.conf"> - sip.conf</option>
				<option value="/etc/asterisk/sip_notify.conf"> - sip_notify.conf</option>
				<option value="/etc/asterisk/skinny.conf"> - skinny.conf</option>
				<option value="/etc/asterisk/sla.conf"> - sla.conf</option>
				<option value="/etc/asterisk/smdi.conf"> - smdi.conf</option>
				<option value="/etc/asterisk/udptl.conf"> - udptl.conf</option>
				<option value="/etc/asterisk/unistim.conf"> - unistim.conf</option>
				<option value="/etc/asterisk/usbradio.conf"> - usbradio.conf</option>
				<option value="/etc/asterisk/users.conf"> - users.conf</option>
				<option value="/etc/asterisk/voicemail.conf"> - voicemail.conf</option>
				<option value="/etc/asterisk/vpb.conf"> - vpb.conf</option>
				<option>Misc. Configuration</option>
				<option value="/etc/dahdi/system.conf"> - /etc/dahdi/system.conf</option>
				<option value="/var/spool/cron/crontabs/root"> - /var/spool/cron/crontabs/root</option>
			</select>
			&nbsp;<a href="javascript:{}" onclick="dev_file_get();" class="ui-state-default ui-corner-all">edit</a>
			<br>
			<br>
			<div id="devfilepanel">
				<strong>System Auto-Generated Content</strong><br>
				<textarea id="devfilecontents" rows="15" wrap="off" class="devfixedwidth" disabled="true"></textarea>
				<br>
				<br>
				<br>
				<strong>Append</strong><input name="devfilemode" type="radio" value="append">
				or <strong>Override</strong><input name="devfilemode" type="radio" value="override" onclick='dev_file_copycontents();'>
				with this text
				<br><textarea id="devfilemanualcontents" rows="15" wrap="off" class="devfixedwidth"></textarea>
				<br>
				<a href="javascript:{}" onclick='dev_file_edit_stop();' class="ui-state-default ui-corner-all">cancel</a>
				<a href="javascript:{}" onclick="dev_file_put();" class="ui-state-default ui-corner-all">save file contents</a>
			</div>
		</div>
		<div id="devtabs-4">
			<p>Hidden goodies not yet stable or complete enough to be linked from the main menu. Testing and feedback greatly appreciated.</p>
			<ul>
				<li>Skinny Phones - <a href="phones_skinny_edit.php">phones_skinny_edit.php</a></li>
				<li>Q-Stat Performance Statistics Recording - <a href="debug_qstat.php">debug_qstat.php</a></li>
			</ul>
		</div>
	</div>
</div>
