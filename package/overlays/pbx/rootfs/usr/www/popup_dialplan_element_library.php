#!/usr/bin/php
<?php 
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2008 IKT <http://itison-ikt.de>.
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

require_once("guiconfig.inc");
require_once("functions.inc");

?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>

	<head>
		<title><?=gettext("Dialplan Element Library");?></title>
		<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
		<script src="jqueryui/js/jquery.js" type="text/javascript"></script>
		<script src="jquery.selectboxes.js" type="text/javascript"></script>
		<script src="jquery.preloadImage.js" type="text/javascript"></script>
		<link href="gui.css" rel="stylesheet" type="text/css">
	</head>

	<body id="linkcolor_default">
		<script type="text/JavaScript">
		<!--
			jQuery.noConflict();
		
			function element_library_populate_applications() {
				jQuery.get('cgi-bin/ajax.cgi', { exec_ami: '"core show applications"' }, function(data){
					var lines = data.split(/\n/);
					var app_pairs = [];
					var i = 0;
					var ii = 0;
					var begin = 0;
					var end = 0;
					var appname = "";
					var appdesc = "";
					var app_list_content = "";
        	
					for (i = 1; i < lines.length - 2; i++) {
						begin = lines[i].search(/\S/);
						end = lines[i].indexOf(":", begin);
						appname = lines[i].substring(begin, end);
						appdesc = lines[i].substr(end + 2);
						app_pairs[ii++] = [appname, appdesc];
					}
        	
					for (i = 0; i < app_pairs.length; i++) {
						app_list_content += 
						"<div class=\"application-entry\"><p><strong><a href=\"javascript:{}\" onclick=\"element_library_get_application_description('" + 
						app_pairs[i][0] + 
						"')\">" +
						app_pairs[i][0] +
						"</a></strong>&nbsp;-&nbsp;" +
						app_pairs[i][1] +
						"<div class=\"application-description\" id=\"application-description-" +
						app_pairs[i][0] +
						"\" style=\"display: none;\">fetching description...</div></p></div>\n";
					}
        	
					jQuery('#element-library-app-content').html(app_list_content);
				});
			}
        	
			function element_library_get_application_description(app) {
				jQuery('#application-description-' + app).toggle("slow");
				if (jQuery('#application-description-' + app).html() == "fetching description...") {
					jQuery.get('cgi-bin/ajax.cgi', { exec_ami: '"core show application ' + app + '"' }, function(data){
						var app_description = "";
						var description_start = 0;
						var description_end = 0;
        	    	
						description_start = data.search(/\[Description\]/);
						description_end = data.indexOf("\n", description_start);
						app_description = data.substr(description_end + 1);
						jQuery('#application-description-' + app).html('<pre>' + app_description + '</pre>');
					});
				}
			}
        	
			function element_library_populate_functions() {
				jQuery.get('cgi-bin/ajax.cgi', { exec_ami: '"core show functions"' }, function(data){
					var lines = data.split(/\n/);
					var func_pairs = [];
					var i = 0;
					var ii = 0;
					var begin = 0;
					var end = 0;
					var funcname = "";
					var funcdesc = "";
					var func_list_content = "";
        	
					for (i = 2; i < lines.length - 2; i++) {
						end = lines[i].search(/\s/);
						funcname = lines[i].substring(0, end);
						funcdesc = lines[i].substr(59);
						func_pairs[ii++] = [funcname, funcdesc];
					}
        	
					for (i = 0; i < func_pairs.length; i++) {
						func_list_content += 
						"<div class=\"function-entry\"><p><strong><a href=\"javascript:{}\" onclick=\"element_library_get_function_description('" + 
						func_pairs[i][0] + 
						"')\">" +
						func_pairs[i][0] +
						"</a></strong>&nbsp;-&nbsp;" +
						func_pairs[i][1] +
						"<div class=\"function-description\" id=\"function-description-" +
						func_pairs[i][0] +
						"\" style=\"display: none;\">fetching description...</div></p></div>\n";
					}
        	
					jQuery('#element-library-func-content').html(func_list_content);
				});
			}
        	
			function element_library_get_function_description(func) {
				jQuery('#function-description-' + func).toggle("slow");
				if (jQuery('#function-description-' + func).html() == "fetching description...") {
					jQuery.get('cgi-bin/ajax.cgi', { exec_ami: '"core show function ' + func + '"' }, function(data){
						var func_description = "";
						var syntax_start = 0;
						var syntax_end = 0;
						var description_start = 0;
						var description_end = 0;
        	    	
						syntax_start = data.search(/\[Syntax\]/) + 9;
						syntax_end = data.indexOf(")", syntax_start) + 1;
						description_start = data.search(/\[Description\]/);
						description_end = data.indexOf("\n", description_start);
						func_description = "<strong>" + data.substring(syntax_start, syntax_end) + "</strong>";
						func_description += "\n\n";
						func_description += data.substr(description_end + 1);
        	
						jQuery('#function-description-' + func).html('<pre>' + func_description + '</pre>');
					});
				}
			}
		
			jQuery(document).ready(function(){
		
				jQuery("#element-library-app-tab").click(function(){
					jQuery(this).removeClass('tabinact').addClass('tabact');
					jQuery('#element-library-func-tab').removeClass('tabact').addClass('tabinact');
					jQuery('#element-library-func-content').hide();
					jQuery('#element-library-phone-tab').removeClass('tabact').addClass('tabinact');
					jQuery('#element-library-phone-content').hide();
					jQuery('#element-library-prompts-tab').removeClass('tabact').addClass('tabinact');
					jQuery('#element-library-prompts-content').hide();
					jQuery('#element-library-app-content').show();
				});

				jQuery("#element-library-func-tab").click(function(){
					jQuery(this).removeClass('tabinact').addClass('tabact');
					jQuery('#element-library-app-tab').removeClass('tabact').addClass('tabinact');
					jQuery('#element-library-app-content').hide();
					jQuery('#element-library-phone-tab').removeClass('tabact').addClass('tabinact');
					jQuery('#element-library-phone-content').hide();
					jQuery('#element-library-prompts-tab').removeClass('tabact').addClass('tabinact');
					jQuery('#element-library-prompts-content').hide();
					jQuery('#element-library-func-content').show();
				});

				jQuery("#element-library-phone-tab").click(function(){
					jQuery(this).removeClass('tabinact').addClass('tabact');
					jQuery('#element-library-app-tab').removeClass('tabact').addClass('tabinact');
					jQuery('#element-library-app-content').hide();
					jQuery('#element-library-func-tab').removeClass('tabact').addClass('tabinact');
					jQuery('#element-library-func-content').hide();
					jQuery('#element-library-prompts-tab').removeClass('tabact').addClass('tabinact');
					jQuery('#element-library-prompts-content').hide();
					jQuery('#element-library-phone-content').show();
				});

				jQuery("#element-library-prompts-tab").click(function(){
					jQuery(this).removeClass('tabinact').addClass('tabact');
					jQuery('#element-library-app-tab').removeClass('tabact').addClass('tabinact');
					jQuery('#element-library-app-content').hide();
					jQuery('#element-library-func-tab').removeClass('tabact').addClass('tabinact');
					jQuery('#element-library-func-content').hide();
					jQuery('#element-library-phone-tab').removeClass('tabact').addClass('tabinact');
					jQuery('#element-library-phone-content').hide();
					jQuery('#element-library-prompts-content').show();
				});

				element_library_populate_applications();
				element_library_populate_functions();
			});
		
		-->
		</script>
		<table width="100%" border="0" cellpadding="0" cellspacing="0">
			<tr>
				<td class="tabnavtbl">
					<ul id="tabnav">
						<li id="element-library-app-tab" class="tabact"><a href="javascript:{}"><?=gettext("Applications");?></a></li>
						<li id="element-library-func-tab" class="tabinact"><a href="javascript:{}"><?=gettext("Functions");?></a></li>
						<li id="element-library-phone-tab" class="tabinact"><a href="javascript:{}"><?=gettext("Phones");?></a></li>
						<li id="element-library-prompts-tab" class="tabinact"><a href="javascript:{}"><?=gettext("Audio Prompts");?></a></li>
					</ul>
				</td>
			</tr>
			<tr>
				<td class="tabcont">
					<span id="element-library-app-content"></span>
					<span id="element-library-func-content" style="display: none"></span>
					<span id="element-library-phone-content" style="display: none">
						<p><em><?=gettext("Below is a list of the currently defined phones along with a basic Dial() command to call them. Many additional options are available for this command, please read its reference documentation in the Applications tab.");?></em></p>
						<table border="0" cellspacing="0" cellpadding="6" width="100%">
							<tr>
								<td width="200px"><strong><?=gettext("Caller ID / Name");?></strong></td>
								<td><strong><?=gettext("Basic");?> <code>Dial()</code> <?=gettext("Command");?></strong></td>
							</tr><?

						$phones = pbx_get_phones();
						foreach ($phones as $p) {
							echo "<tr><td>" .
								$p['callerid'] . "&nbsp;&lt;" . $p['extension'] . "&gt;" .
								"</td><td><code>" .
								"Dial(" . extensions_phone_uniqid_to_dialstring($p['uniqid']) . ")" .
								"</code></td></tr>\n";
						}

						$phones = external_get_phones();
						foreach ($phones as $p) {
							echo "<tr><td>" .
								$p['name'] . "&nbsp;&lt;" . $p['extension'] . "&gt;" .
								"</td><td><code>" .
								"Dial(" . extensions_phone_uniqid_to_dialstring($p['uniqid']) . ")" .
								"</code></td></tr>\n";
						}

						?></table>
					</span>
					<span id="element-library-prompts-content" style="display: none"><?
					
					function get_format($s) {
						return substr($s, strpos($s, ".")+1);
					}
					function get_filename($s) {
						return substr($s, 0, strpos($s, "."));
					}

					$prompt_languages = array(
						"en-us" => gettext("English (US)"),
						"en-gb" => gettext("English (UK)"),
						//"da" => gettext("Danish"),
						//"nl" => gettext("Dutch"),
						//"fr" => gettext("French (France)"),
						"fr-ca" => gettext("French (Canada)"),
						"de-de" => gettext("German"),
						"it-it" => gettext("Italian"),
						//"jp" => gettext("Japanese"),
						//"pt-br" => gettext("Portuguese (Brazil)"),
						"ru" => gettext("Russian"),
						"es-es" => gettext("Spanish (Spain)")
						//"se" => gettext("Swedish")
					);

					$languages = array_keys($prompt_languages);
					$prompts = array();

					$base_path = "/offload/asterisk/sounds";
					$dh = opendir($base_path);
					while (false !== ($filename = readdir($dh))) {
						if (in_array($filename, $languages)) {
							$lang = $filename;
							$path = $base_path . "/" . $lang;
							$dh_sub = opendir($path);
							while (false !== ($filename = readdir($dh_sub))) {
								if (preg_match("/\S+\.\S+/", $filename)) {
									$prompts[$lang][] = array(
										get_filename($filename),
										get_format($filename),
										format_bytes(filesize($path . "/" . $filename))
									);
								}
							}
							closedir($dh_sub);
						}
					}
					closedir($dh);

					?><ul><?
					foreach ($prompt_languages as $lang => $friendly) {
						?><li><a href="#<?=$lang;?>"><?=$friendly;?></a></li><?
					}
					?></ul>
					<br>
					<br>

						<table border="0" cellspacing="0" cellpadding="6" width="100%"><?
						foreach ($prompt_languages as $lang => $friendly) {
							if (!count($prompts[$lang])) {
								continue;
							}
							?><tr>
								<td colspan="4" class="listtopiclight"><a name="<?=$lang;?>"><?=$friendly;?></a></td>
							</tr>
							<tr>
								<td><strong><?=gettext("Filename");?></strong></td>
								<td><strong><?=gettext("Format");?></strong></td>
								<td><strong><?=gettext("Size");?></strong></td>
							</tr><?

							/* for each prompt... */
							foreach ($prompts[$lang] as $prompt) {
								?><tr>
									<td><?=$prompt[0];?></td>
									<td><?=$prompt[1];?></td>
									<td><?=$prompt[2];?></td>
								</tr><?
							}

							?><tr>
								<td class="list" colspan="3" height="12">&nbsp;</td>
							</tr><?

						}

						?></table></span>
					</span>
				</td>
			</tr>
		</table>
	</body>

</html>
