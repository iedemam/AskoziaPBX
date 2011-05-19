<?php
/*
	$Id$
    originally part of m0n0wall (http://m0n0.ch/wall)
	continued modifications as part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2003-2006 Manuel Kasper <mk@neon1.net>.
	Copyright (C) 2007-2010 tecema (a.k.a IKT) <http://www.tecema.de>.
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
	"latest_config" => "2.1",
	"package_system" => "1.0",
	"technologies" => array("sip", "iax", "isdn", "analog", "skinny"),
	"phone_types" => array("sip", "iax", "isdn", "analog", "skinny", "external"),
	"fax_types" => array("analog", "virtual"),
	"provider_types" => array("sip", "iax", "isdn", "analog"),

	"storage_services" => array("media", "persistence", "astlogs", "faxarchive"),

	"dahdi_dirty_path" => "/var/run/dahdi.dirty",
	"analog_dirty_path" => "/var/run/analog.dirty",
	"sip_dirty_path" => "/var/run/sip.dirty",
	"iax_dirty_path" => "/var/run/iax.dirty",
	"isdn_dirty_path" => "/var/run/isdn.dirty",
	"redfone_dirty_path" => "/var/run/redfone.dirty",
	"skinny_dirty_path" => "/var/run/skinny.dirty",
	"storage_dirty_path" => "/var/run/storage.dirty",
	"dialplan_dirty_path" => "/var/run/dialplan.dirty",
	"external_dirty_path" => "/var/run/external.dirty",
	"virtualfax_dirty_path" => "/var/run/virtualfax.dirty",
	"moh_dirty_path" => "/var/run/moh.dirty",
	"vmgreeting_dirty_path" => "/var/run/vmgreeting.dirty"
);

$defaults = array();

$defaults['dahdi']['echo-taps'] = 128;
$defaults['dahdi']['rxgain'] = 0.0;
$defaults['dahdi']['txgain'] = 0.0;
$defaults['dahdi']['analog']['startsignaling'] = 'ks';
$defaults['dahdi']['bri']['lbo'] = 0;
$defaults['dahdi']['bri']['framing'] = 'ccs';
$defaults['dahdi']['bri']['coding'] = 'ami';

$defaults['dahdi']['loadzones'] = array('us');

$defaults['accounts']['language'] = 'en-us';
$defaults['accounts']['codec'] = array('ulaw', 'alaw', 'gsm');
$defaults['accounts']['qualify'] = 2;

$defaults['accounts']['phones']['ringlength'] = 'indefinitely';
$defaults['accounts']['phones']['default-extension-start'] = 101;
$defaults['accounts']['phones']['callerid'] = 'Default Extension';
$defaults['accounts']['phones']['dialprovider'] = 'sipuri';

$defaults['accounts']['providers']['calleridsource'] = 'phones';

$defaults['sip']['dtmfmode'] = 'auto';
$defaults['sip']['natmode'] = 'yes';

$defaults['iax']['authentication'] = 'md5';

//


$defaults['storage']['system-partition-offset-megabytes'] = "64";
$defaults['storage']['system-media-minimum-size'] = "96";

$defaults['system'] = array();
$defaults['system']['username'] = "admin";
$defaults['system']['timeservers'] = "pool.ntp.org";
$defaults['system']['timeupdateinterval'] = "4-hours";

// regional settings defaults
$defaults['system']['regional']['analog']['fxo']['daa'] = 'FCC';

?>
