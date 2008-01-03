<?php
/*
	$Id$
    originally part of m0n0wall (http://m0n0.ch/wall)
	continued modifications as part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
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

$g = array(
    "varrun_path" => "/var/run",
    "varetc_path" => "/var/etc",
    "vardb_path" => "/var/db",
    "varlog_path" => "/var/log",
    "etc_path" => "/etc",
    "tmp_path" => "/tmp",
    "conf_path" => "/conf",
    "ftmp_path" => "/ftmp",
    "conf_default_path" => "/conf.default",
    "cf_path" => "/cf",
    "cf_conf_path" => "/cf/conf",
    "www_path" => "/usr/local/www",
    "xml_rootobj" => "askoziapbx",
    "debug" => false,
    "wireless_hostap_regex" => "/^(wi|ath|ral|ural)/",
    "latest_config" => "1.4",
	"technologies" => array("sip", "iax", "isdn", "analog")
);

$defaults = array();

$defaults['analog'] = array();
$defaults['analog']['interface'] = array();
$defaults['analog']['interface']['name'] = "(unconfigured)";
$defaults['analog']['interface']['startsignal'] = "ks";
$defaults['analog']['interface']['echocancel'] = "128";
$defaults['analog']['loadzone'] = array("us");

$defaults['codec'] = array("ulaw");

$defaults['featuremap'] = array();
$defaults['featuremap']['attendedtransfer'] = "**";
$defaults['featuremap']['blindtransfer'] = "##";
$defaults['featuremap']['featuredigittimeout'] = "500";
$defaults['featuremap']['atxfernoanswertimeout'] = "15";
$defaults['featuremap']['transferdigittimeout'] = "3";

$defaults['iax'] = array();
$defaults['iax']['port'] = "4569";
$defaults['iax']['authentication'] = "plaintext";
$defaults['iax']['provider'] = array();
$defaults['iax']['provider']['calleridsource'] = "phones";

$defaults['internationalization'] = array();
$defaults['internationalization']['prompt-language'] = "en";

$defaults['isdn'] = array();
$defaults['isdn']['interface'] = array();
$defaults['isdn']['interface']['name'] = "(unconfigured)";
$defaults['isdn']['law'] = "alaw";
$defaults['isdn']['nationalprefix'] = "0";
$defaults['isdn']['internationalprefix'] = "00";

$defaults['parking'] = array();
$defaults['parking']['parkext'] = "700";
$defaults['parking']['parkposstart'] = "701";
$defaults['parking']['parkposend'] = "720";
$defaults['parking']['parktime'] = "30";

$defaults['qos'] = array();
$defaults['qos']['qualify'] = "2";

$defaults['rtp'] = array();
$defaults['rtp']['lowport'] = "10000";
$defaults['rtp']['highport'] = "10200";

$defaults['sip'] = array();
$defaults['sip']['port'] = "5060";
$defaults['sip']['defaultexpiry'] = "120";
$defaults['sip']['minexpiry'] = "60";
$defaults['sip']['maxexpiry'] = "3600";
$defaults['sip']['phone'] = array();
$defaults['sip']['phone']['calllimit'] = "2";
$defaults['sip']['phone']['busylimit'] = "1";
$defaults['sip']['provider'] = array();
$defaults['sip']['provider']['calleridsource'] = "phones";

$defaults['syslog'] = array();
$defaults['syslog']['nentries'] = "100";

$defaults['system'] = array();
$defaults['system']['username'] = "admin";
$defaults['system']['timezone'] = "Etc/UTC";
$defaults['system']['timeservers'] = "pool.ntp.org";
$defaults['system']['timeupdateinterval'] = "300";

$defaults['webgui'] = array();
$defaults['webgui']['protocol'] = "https";

?>
