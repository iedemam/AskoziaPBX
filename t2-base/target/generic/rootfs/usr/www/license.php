#!/usr/bin/php
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

require("guiconfig.inc");

$pgtitle = array("License");
 
?>
<?php include("fbegin.inc"); ?>
	<p><strong>Askozia&reg;PBX is Copyright &copy; 2007 - 2009 IKT (<a href="http://itison-ikt.de/en/startseite">http://www.itison-ikt.de</a>).<br>
	All rights reserved.</strong></p>
	<p> Redistribution and use in source and binary forms, with or without<br>
	modification, are permitted provided that the following conditions 
	are met:<br>
	<br>
	1. Redistributions of source code must retain the above copyright notice,<br>
	this list of conditions and the following disclaimer.<br>
	<br>
	2. Redistributions in binary form must reproduce the above copyright<br>
	notice, this list of conditions and the following disclaimer in the<br>
	documentation and/or other materials provided with the distribution.<br>
	<br>
	<strong>THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,<br>
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY<br>
	AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE<br>
	AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,<br>
	OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF<br>
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS<br>
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN<br>
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)<br>
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE<br>
	POSSIBILITY OF SUCH DAMAGE</strong>.</p>
	<hr size="1">
	<p>AskoziaPBX is derived from m0n0wall&reg; (<a href="http://m0n0.ch/wall" target="_blank">http://m0n0.ch/wall</a>) (FreeBSD 6 branch, r195).<br>
	Many thanks to Manuel Kasper, Chris Buechler and all of the m0n0wall contributors.<br>
	<br>
	<strong>m0n0wall is Copyright &copy; 2002 - 2008 Manuel Kasper (<a href="mailto:mk@neon1.net">mk@neon1.net</a>).<br>
	m0n0wall is a registered trademark of Manuel Kasper.<br>
	All rights reserved.</strong></p>
	<hr size="1">
	<p>AskoziaPBX is also based upon/includes various free software packages, 
	listed below.<br>
	A big thanks to the authors for all of their hard work!</p>
	Asterisk (<a href="http://www.asterisk.org" target="_blank">http://www.asterisk.org</a>)<br>
	Copyright &copy; 1999 - 2008, Digium, Inc.
	All rights reserved.<br>
	<br>
	<p>FreeBSD (<a href="http://www.freebsd.org" target="_blank">http://www.freebsd.org</a>)<br>
	Copyright &copy; 1994 - 2008 FreeBSD, Inc. All rights reserved.<br>
	<br>
	This product includes PHP, freely available from <a href="http://www.php.net/" target="_blank">http://www.php.net</a>.<br>
	Copyright &copy; 1999 - 2008 The PHP Group. All rights reserved.<br>
	<br>
	mini_httpd (<a href="http://www.acme.com/software/mini_httpd" target="_blank">http://www.acme.com/software/mini_httpd</a>)<br>
	Copyright &copy; 1999, 2000 by Jef Poskanzer &lt;jef@acme.com&gt;. 
	All rights reserved.<br>
	<br>
	script.aculo.us (<a href="http://script.aculo.us" target="_blank">http://script.aculo.us</a>)<br>
	Copyright &copy; 2005 - 2008 Thomas Fuchs (http://script.aculo.us, http://mir.aculo.us)<br>
	<br>
	jQuery (<a href="http://jquery.com" target="_blank">http://jquery.com</a>)<br>
	Copyright &copy; 2007 - 2008 John Resig<br>
	<br>
	with plugins:
	<ul>
		<li><a href="http://plugins.jquery.com/project/blockui" target="_blank">blockUI</a> - Copyright &copy; 2007 - 2008 M. Alsup</li>
		<li><a href="http://plugins.jquery.com/project/preloadImages" target="_blank">preloadImages</a> - Copyright &copy; 2007 - 2008 Blair McBride</li>
		<li><a href="http://plugins.jquery.com/project/selectboxes" target="_blank">selectboxes</a> - Copyright &copy; 2006 - 2008 Sam Collett</li>
	</ul>
	<br>
	silk icons from famfamfam (<a href="http://www.famfamfam.com/lab/icons/silk/" target="_blank">http://www.famfamfam.com/lab/icons/silk/</a>)<br>
	Creative Commons Attribution 2.5 Licensed from Mark James<br>
	<br>
	Circular log support for FreeBSD syslogd (<a href="http://software.wwwi.com/syslogd/" target="_blank">http://software.wwwi.com/syslogd</a>)<br>
	Copyright &copy; 2001 Jeff Wheelhouse (jdw@wwwi.com)<br>
	<br>
	msntp (<a href="http://www.hpcf.cam.ac.uk/export" target="_blank">http://www.hpcf.cam.ac.uk/export</a>)<br>
	Copyright &copy; 1996, 1997, 2000 N.M. Maclaren, University of Cambridge. 
	All rights reserved.<br>
	<br>
	msmtp (<a href="http://msmtp.sourceforge.net" target="_blank">http://msmtp.sourceforge.net</a>)<br>
	Copyright &copy; 2005 - 2008 Martin Lambers.
	All rights reserved.<br>
	<br>
	ez-ipupdate (<a href="http://www.gusnet.cx/proj/ez-ipupdate/" target="_blank">http://www.gusnet.cx/proj/ez-ipupdate</a>)<br>
	Copyright &copy; 1998-2001 Angus Mackay. All rights reserved.<br>
	additional patches from <a href="http://m0n0.ch/wall" target="_blank">m0n0wall</a> and jef2000 in the <a href="http://forum.openwrt.org" target="_blank">OpenWRT forums</a><br>
	<br>
	ISDN4BSD by HPS (<a href="http://www.turbocat.net/~hselasky/isdn4bsd/" target="_blank">http://www.turbocat.net/~hselasky/isdn4bsd/</a>)<br>
	Copyright &copy; 2003 - 2008 Hans Petter Selasky.
	All rights reserved.<br>
	<br>
	OSLEC Open Source Line Echo Canceller (<a href="http://www.rowetel.com/ucasterisk/oslec.html" target="_blank">http://www.rowetel.com/ucasterisk/oslec.html</a>)<br>
	Copyright &copy; 2007 - 2008, David Rowe
	All rights reserved.<br>
	<br>
	Original zaptel infrastracture by Digium Inc., ported to FreeBSD by<br>
	Oleksandr Tymoshenko, Dinesh Nair, Konstantin Prokazoff, Luigi Rizzo, Chris Stenton and Yuri Saltykov
	</p>
	<hr size="1">
	<p>webGUI Localization Translators</p>
	<p>
	<ul>
		<li>Bulgarian - Chavdar Shtiliyanov</li>
		<li>Chinese (Traditional & Simplified) - Xing Cao</li>
		<li>Czech - David Moravec</li>
		<li>Danish - Carsten Matzon</li>
		<li>Dutch - Wim Van Clapdurp</li>
		<li>Finnish - Christoffer Lindqvist</li>
		<li>German - Andreas Jud, Helge Ulrich</li>
		<li>Greek - Nassa Perra (<a href="http://allvoip.gr">allVoIP.gr</a>)</li>
		<li>Italian - Roberto Zilli, Giovanni Vallesi</li>
		<li>Japanese - Kenichi Fukaumi</li>
		<li>Polish - Marcin Hańczaruk</li>
		<li>Turkish - Akif Dinç</li>
	</ul>
	</p>
	<hr size="1">
	<p>The multilingual speech prompts used in AskoziaPBX have been made available by the following groups</p>
	<p>Danish (<a href="http://www.globaltelip.dk/downloads/da-voice-prompts.rar" target="_blank">http://www.globaltelip.dk</a>)<br>
	Copyright Information Unknown<br>
	<br>
	Dutch (<a href="http://www.borndigital.nl/voip.asp?page=106&title=Asterisk_Soundfiles" target="_blank">http://www.borndigital.nl</a>)<br>
	Copyright &copy; 2006 Born Digital<br>
	<br>
	English (US) and French (Canada) (<a href="http://ftp.digium.com/pub/telephony/sounds/releases" target="_blank">http://ftp.digium.com/pub/telephony/sounds/releases</a>)<br>
	Copyright &copy; Allison Smith (<a href="http://www.theivrvoice.com" target="_blank">http://www.theivrvoice.com</a>)<br>
	Financed by Digium, Inc. (<a href="http://www.digium.com" target="_blank">http://www.digium.com</a>)<br>
	<br>
	English (UK) (<a href="http://www.enicomms.com/cutglassivr/" target="_blank">http://www.enicomms.com/cutglassivr/</a>)<br>
	Copyright &copy; 2006 ENIcommunications Corp.<br>
	Released under the <a href="http://creativecommons.org/licenses/by-sa/2.5/" target="_blank">Creative Commons Attribution-ShareAlike 2.5 License</a><br>
	<br>
	French (France) (<a href="http://www.sineapps.com" target="_blank">http://www.sineapps.com</a>)<br>
	Copyright Information Unknown<br>
	<br>
	German (<a href="http://www.amooma.de/asterisk/service/deutsche-sprachprompts" target="_blank">http://www.amooma.de</a>)<br>
	Copyright &copy; 2006 amooma GmbH<br>
	Released under the <a href="http://www.gnu.org/copyleft/gpl.html" target="_blank">GPL</a><br>
	<br>
	Italian (<a href="http://mirror.tomato.it/ftp/pub/asterisk/suoni_ita" target="_blank">http://mirror.tomato.it</a>)<br>
	Copyright &copy; 2006 Marco Menardi and Paola Dal Zot<br>
	Released under the <a href="http://www.gnu.org/copyleft/gpl.html" target="_blank">GPL</a><br>
	Voice: Paola Dal Zot<br>
	<br>
	Japanese (<a href="http://www.voip-info.jp/index.php?Japanese" target="_blank">http://www.voip-info.jp</a>)<br>
	Copyright &copy; 2005 Takahashi<br>
	Voice: Eri Takeda.<br>
	Edited by VoIP-Info.jp.<br>
	<br>
	Portuguese (Brazil) (<a href="http://www.disc-os.org" target="_blank">http://www.disc-os.org</a>)<br>
	Copyright &copy; Intelbras S/A<br>
	Released under the <a href="http://creativecommons.org/licenses/by-sa/2.5/" target="_blank">Creative Commons Attribution-ShareAlike 2.5 License</a><br>
	<br>
	Russian (<a href="http://www.asterisk-support.ru/files/ivr/" target="_blank">http://www.asterisk-support.ru</a>)<br>
	Copyright Information Unknown<br>
	<br>
	Spanish (<a href="http://www.voipnovatos.es/index.php?itemid=943" target="_blank">http://www.voipnovatos.es</a>)<br>
	Copyright &copy; 2006 Alberto Sagredo Castro<br>
	<br>
	Swedish (<a href="http://www.danielnylander.se/asterisk-saker" target="_blank">http://www.danielnylander.se</a>)<br>
	Copyright &copy; 2005 Daniel Nylander<br>
	Released under the <a href="http://www.gnu.org/copyleft/gpl.html" target="_blank">GPL</a><br>
	</p>
	

<?php include("fend.inc"); ?>
