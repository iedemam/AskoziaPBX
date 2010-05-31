<?php
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2010 tecema (a.k.a IKT) <http://www.tecema.de>.
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

/* include all configuration functions */
require_once("functions.inc");


function gsm_conf_generate($return=false) {

	$conf = <<<EOD
[general]
;Directory where the sms files are stored.
smsdir=/var/asterisk/spool/sms-incoming

;if you want to periodically check for the homezone
check_homezone=yes

;if you want to skip the '+' prefix, set this to yes
skip_plus=no

;set this to yes to enable debugging via syslog
syslogdebug=yes

;set this to yes to enable radio/gsm optimzed dtmf detection mechanism
relaxdtmf=no

;set to yes in order to limit the double digit dtmf send
dtmfbegin=yes

;all port configuration should go to gsm_additional.conf
;which has a sample configuration that is commented out


;; sample configuration for non-FreePBX users
;[test]
;initfile=/etc/asterisk/gsm-init-file

;;go into this context in the extensions.conf
;context=from-pstn
;;use this extension in the above context
;exten=s

;;sim card pin
;pin=none

;;don't show our callerid
;hidecallerid=no

;;enable pdu mode
;sms_pdu_mode=no

;;smsc (leave blank to go with default if you don't know it)
;smsc=

;;never reset a port, set this to the amount of seconds after 
;;the ports should be reset (if they're not in use)
;resetinterval=0
;channel=1

;;receive gain (set to 0 for default values)
;rxgain=0

;;transmit gain (set to 0 for default values)
;txgain=0


EOD;

	return ($return) ? $conf : util_file_write("/etc/asterisk/gsm.conf", $conf);
}


function gsm_generate_init_file($port) {

	$conf = <<<EOD
at&c0	ok	0	10
ate0	ok	0	10
at+creg=1	ok	0	10
at+creg?	ok	0	10
at+sidet=0	ok	0	10
at+csdt=1	ok	0	10
at+csmins=1	ok	0	10
at+clip=1	ok	0	10

EOD;

	return util_file_write("/etc/asterisk/" . $port['uniqid'] . ".init", $conf);
}

?>
