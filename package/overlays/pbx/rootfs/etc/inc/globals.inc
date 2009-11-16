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
	"conf_default_path" => "/conf.default",
	"cf_path" => "/cf",
	"cf_conf_path" => "/cf/conf",
	"www_path" => "/usr/www",
	"xml_rootobj" => "askoziapbx",
	"debug" => false,
	"wireless_hostap_regex" => "/^(wi|ath|ral|ural)/",
	"latest_config" => "1.6",
	"package_system" => "1.0",
	"technologies" => array("sip", "iax", "isdn", "analog"),
	"phone_types" => array("sip", "iax", "isdn", "analog", "external"),
	"provider_types" => array("sip", "iax", "isdn", "analog"),

	"dahdi_dirty_path" => "/var/run/dahdi.dirty",
	"analog_dirty_path" => "/var/run/analog.dirty"
);

$defaults = array();

$defaults['dahdi']['echo-module'] = 'mg2';
$defaults['dahdi']['echo-taps'] = 128;
$defaults['dahdi']['rxgain'] = 0.0;
$defaults['dahdi']['txgain'] = 0.0;
$defaults['dahdi']['analog']['startsignaling'] = 'ks';
$defaults['dahdi']['bri']['lbo'] = 0;
$defaults['dahdi']['bri']['framing'] = 'ccs';
$defaults['dahdi']['bri']['coding'] = 'ami';

$defaults['accounts']['language'] = 'en-us';
$defaults['accounts']['codec'] = array('ulaw');

$defaults['accounts']['phones']['ringlength'] = "indefinitely";
$defaults['accounts']['phones']['default-extension-start'] = 101;
$defaults['accounts']['phones']['callerid'] = "Default Extension";
$defaults['accounts']['phones']['novmwhenbusy'] = true;
$defaults['accounts']['phones']['publicaccess'] = "allow";

//
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
$defaults['isdn']['interface']['name'] = "(" . gettext("unconfigured") . ")";
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

$defaults['sip'] = array();
$defaults['sip']['port'] = "5060";
$defaults['sip']['defaultexpiry'] = "120";
$defaults['sip']['minexpiry'] = "60";
$defaults['sip']['maxexpiry'] = "3600";
$defaults['sip']['natmode'] = "yes";
$defaults['sip']['phone'] = array();
$defaults['sip']['phone']['calllimit'] = "2";
$defaults['sip']['phone']['busylimit'] = "1";
$defaults['sip']['provider'] = array();
$defaults['sip']['provider']['calleridsource'] = "phones";

$defaults['storage']['system-partition-offset-megabytes'] = "64";
$defaults['storage']['system-media-minimum-size'] = "96";

$defaults['syslog'] = array();
$defaults['syslog']['nentries'] = "100";

$defaults['system'] = array();
$defaults['system']['username'] = "admin";
$defaults['system']['timeservers'] = "pool.ntp.org";
$defaults['system']['timeupdateinterval'] = "4-hours";

$defaults['webgui'] = array();
$defaults['webgui']['protocol'] = "https";

$defaults['voicemail']['authtype'] = "auto";

?>
