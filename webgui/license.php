#!/usr/local/bin/php
<?php
/*
	$Id$
	part of m0n0wall (http://m0n0.ch/wall)
	
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
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

$pgtitle = array(gettext("License"));
require("guiconfig.inc"); 
?>
<?php include("fbegin.inc"); ?>
	<p><strong><?=gettext("AskoziaPBX is Copyright &copy; 2007 - 2008 IKT ");?>(<a href="http://itison-ikt.de/en/startseite"><?=gettext("http://www.itison-ikt.de");?></a>).<br>
	<?=gettext("All rights reserved.");?></strong></p>
	<p> <?=gettext("Redistribution and use in source and binary forms, with or without<br> modification, are permitted provided that the following conditions are met:");?><br>
	<br>
	<?=gettext("1. Redistributions of source code must retain the above copyright notice,<br>this list of conditions and the following disclaimer.");?><br>
	<br>
	<?=gettext("2. Redistributions in binary form must reproduce the above copyright<br>notice, this list of conditions and the following disclaimer in the<br> documentation and/or other materials provided with the distribution.");?><br>
	<br>
	<strong><?=gettext("THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,<br>INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY<br>AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE<br>AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,<br>OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF<br>SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS<br>INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN<br>CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)<br>ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE<br>POSSIBILITY OF SUCH DAMAGE");?></strong>.</p>
	<hr size="1">
	<p><?=gettext("AskoziaPBX is derived from m0n0wall ");?>(<a href="http://m0n0.ch/wall" target="_blank">http://m0n0.ch/wall</a>)<?=gettext(" (FreeBSD 6 branch, r195).");?><br>
	<?=gettext("Many thanks to Manuel Kasper, Chris Buechler and all of the m0n0wall contributors.");?><br>
	<br>
	<strong><?=gettext("m0n0wall&reg; is Copyright &copy; 2002 - 2008 Manuel Kasper ");?>(<a href="mailto:mk@neon1.net">mk@neon1.net</a>).<br>
	<?=gettext("m0n0wall is a registered trademark of Manuel Kasper.");?><br>
	<?=gettext("All rights reserved.");?></strong></p>
	<hr size="1">
	<p><?=gettext("AskoziaPBX is also based upon/includes various free software packages, listed below.");?><br>
	<?=gettext("A big thanks to the authors for all of their hard work!");?></p>
	<?=gettext("Asterisk ");?>(<a href="http://www.asterisk.org" target="_blank">http://www.asterisk.org</a>)<br>
	<?=gettext("Copyright &copy; 1999 - 2008, Digium, Inc. ");?> 
	<?=gettext("All rights reserved.");?><br>
	<br>
	<p><?=gettext("FreeBSD ");?>(<a href="http://www.freebsd.org" target="_blank">http://www.freebsd.org</a>)<br>
	<?=gettext("Copyright &copy; 1994 - 2008 FreeBSD, Inc. ");?>
	<?=gettext("All rights reserved.");?><br>
	<br>
	<?=gettext("This product includes PHP, freely available from ");?><a href="http://www.php.net/" target="_blank">http://www.php.net</a>.<br>
	<?=gettext("Copyright &copy; 1999 - 2008 The PHP Group. ");?>
	<?=gettext("All rights reserved.");?><br>
	<br>
	<?=gettext("mini_httpd ");?>(<a href="http://www.acme.com/software/mini_httpd" target="_blank">http://www.acme.com/software/mini_httpd</a>)<br>
	<?=gettext("Copyright &copy; 1999, 2000 by Jef Poskanzer &lt;jef@acme.com&gt;. ");?> 
	<?=gettext("All rights reserved.");?><br>
	<br>
	<?=gettext("script.aculo.us ");?>(<a href="http://script.aculo.us" target="_blank">http://script.aculo.us</a>)<br>
	<?=gettext("Copyright &copy; 2005 - 2008 Thomas Fuchs ");?>(http://script.aculo.us, http://mir.aculo.us)<br>
	<br>
	<?=gettext("jQuery ");?>(<a href="http://jquery.com" target="_blank">http://jquery.com</a>)<br>
	<?=gettext("Copyright &copy; 2007 - 2008 John Resig");?><br>
	<br>
	<?=gettext("with plugins:");?>
	<ul>
		<li><a href="http://plugins.jquery.com/project/blockui" target="_blank"><?=gettext("blockUI</a> - Copyright &copy; 2007 - 2008 M. Alsup");?></li>
		<li><a href="http://plugins.jquery.com/project/preloadImages" target="_blank"><?=gettext("preloadImages</a> - Copyright &copy; 2007 - 2008 Blair McBride");></li>
		<li><a href="http://plugins.jquery.com/project/selectboxes" target="_blank"><?=gettext("selectboxes</a> - Copyright &copy; 2006 - 2008 Sam Collett");?></li>
	</ul>
	<br>
	<?=gettext("silk icons from famfamfam ");?>(<a href="http://www.famfamfam.com/lab/icons/silk/" target="_blank">http://www.famfamfam.com/lab/icons/silk/</a>)<br>
	<?=gettext("Creative Commons Attribution 2.5 Licensed from Mark James");?><br>
	<br>
	<?=gettext("Circular log support for FreeBSD syslogd ");?>(<a href="http://software.wwwi.com/syslogd/" target="_blank">http://software.wwwi.com/syslogd</a>)<br>
	<?=gettext("Copyright &copy; 2001 Jeff Wheelhouse (jdw@wwwi.com)");?><br>
	<br>
	<?=gettext("msntp ");?>(<a href="http://www.hpcf.cam.ac.uk/export" target="_blank">http://www.hpcf.cam.ac.uk/export</a>)<br>
	<?=gettext("Copyright &copy; 1996, 1997, 2000 N.M. Maclaren, University of Cambridge. ");?> 
	<?=gettext("All rights reserved.");?><br>
	<br>
	<?=gettext("msmtp ");?>(<a href="http://msmtp.sourceforge.net" target="_blank">http://msmtp.sourceforge.net</a>)<br>
	<?=gettext("Copyright &copy; 2005 - 2008 Martin Lambers. ");?>
	<?=gettext("All rights reserved.");?><br>
	<br>
	<?=gettext("ISDN4BSD by HPS ");?>(<a href="http://www.turbocat.net/~hselasky/isdn4bsd/" target="_blank">http://www.turbocat.net/~hselasky/isdn4bsd/</a>)<br>
	<?=gettext("Copyright &copy; 2003 - 2008 Hans Petter Selasky. ");?>
	<?=gettext("All rights reserved.");?><br>
	<br>
	<?=gettext("OSLEC Open Source Line Echo Canceller ");?>(<a href="http://www.rowetel.com/ucasterisk/oslec.html" target="_blank">http://www.rowetel.com/ucasterisk/oslec.html</a>)<br>
	<?=gettext("Copyright &copy; 2007 - 2008, David Rowe ");?>
	<?=gettext("All rights reserved.");?><br>
	<br>
	<?=gettext("Original zaptel infrastracture by Digium Inc., ported to FreeBSD by");?><br>
	<?=gettext("Oleksandr Tymoshenko, Dinesh Nair, Konstantin Prokazoff, Luigi Rizzo, Chris Stenton and Yuri Saltykov");?>
	</p>
	<hr size="1">
	<p><?=gettext("The multilingual speech prompts used in AskoziaPBX have been made available by the following groups");?></p>
	<p><?=gettext("Danish ");?>(<a href="http://www.globaltelip.dk/downloads/da-voice-prompts.rar" target="_blank">http://www.globaltelip.dk</a>)<br>
	<?=gettext("Copyright Information Unknown");?><br>
	<br>
	<?=gettext("Dutch ");?>(<a href="http://www.borndigital.nl/voip.asp?page=106&title=Asterisk_Soundfiles" target="_blank">http://www.borndigital.nl</a>)<br>
	<?=gettext("Copyright &copy; 2006 Born Digital");?><br>
	<br>
	<?=gettext("English (US) and French (Canada) ");?>(<a href="http://ftp.digium.com/pub/telephony/sounds/releases" target="_blank">http://ftp.digium.com/pub/telephony/sounds/releases</a>)<br>
	<?=gettext("Copyright &copy; Allison Smith ");?>(<a href="http://www.theivrvoice.com" target="_blank">http://www.theivrvoice.com</a>)<br>
	<?=gettext("Financed by Digium, Inc. ");?>(<a href="http://www.digium.com" target="_blank">http://www.digium.com</a>)<br>
	<br>
	<?=gettext("English (UK) ");?>(<a href="http://www.enicomms.com/cutglassivr/" target="_blank">http://www.enicomms.com/cutglassivr/</a>)<br>
	<?=gettext("Copyright &copy; 2006 ENIcommunications Corp.");?><br>
	<?=gettext("Released under the ");?><a href="http://creativecommons.org/licenses/by-sa/2.5/" target="_blank"><?=gettext("Creative Commons Attribution-ShareAlike 2.5 License");?></a><br>
	<br>
	<?=gettext("French (France) ");?>(<a href="http://www.sineapps.com" target="_blank">http://www.sineapps.com</a>)<br>
	<?=gettext("Copyright Information Unknown");?><br>
	<br>
	<?=gettext("German ");?>(<a href="http://www.amooma.de/asterisk/service/deutsche-sprachprompts" target="_blank">http://www.amooma.de</a>)<br>
	<?=gettext("Copyright &copy; 2006 amooma GmbH");?><br>
	<?=gettext("Released under the ");?><a href="http://www.gnu.org/copyleft/gpl.html" target="_blank"><?=gettext("GPL");?></a><br>
	<br>
	<?=gettext("Italian ");?>(<a href="http://mirror.tomato.it/ftp/pub/asterisk/suoni_ita" target="_blank">http://mirror.tomato.it</a>)<br>
	<?=gettext("Copyright &copy; 2006 Marco Menardi and Paola Dal Zot");?><br>
	<?=gettext("Released under the ");?><a href="http://www.gnu.org/copyleft/gpl.html" target="_blank"><?=gettext("GPL");?></a><br>
	<?=gettext("Voice: Paola Dal Zot");?><br>
	<br>
	<?=gettext("Japanese ");?>(<a href="http://www.voip-info.jp/index.php?Japanese" target="_blank">http://www.voip-info.jp</a>)<br>
	<?=gettext("Copyright &copy; 2005 Takahashi");?><br>
	<?=gettext("Voice: Eri Takeda.");?><br>
	<?=gettext("Edited by VoIP-Info.jp.");?><br>
	<br>
	<?=gettext("Russian ");?>(<a href="http://www.asterisk-support.ru/files/ivr/" target="_blank">http://www.asterisk-support.ru</a>)<br>
	<?=gettext("Copyright Information Unknown");?><br>
	<br>
	<?=gettext("Spanish ");?>(<a href="http://www.voipnovatos.es/index.php?itemid=943" target="_blank">http://www.voipnovatos.es</a>)<br>
	<?=gettext("Copyright &copy; 2006 Alberto Sagredo Castro");?><br>
	<br>
	<?=gettext("Swedish ");?>(<a href="http://www.danielnylander.se/asterisk-saker" target="_blank">http://www.danielnylander.se</a>)<br>
	<?=gettext("Copyright &copy; 2005 Daniel Nylander");?><br>
	<?=gettext("Released under the ");?><a href="http://www.gnu.org/copyleft/gpl.html" target="_blank"><?=gettext("GPL");?></a><br>
	</p>
	

<?php include("fend.inc"); ?>
