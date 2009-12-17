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
	
	THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
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

require_once("functions.inc");

/**
 * Generates features.conf which contains call parking, transfer settings and feature maps
 */
function features_conf_generate($return=false) {
	global $config;
	
	$parkingconfig = $config['dialplan']['callparking'];
	$pc['parkext'] = isset($parkingconfig['parkext']) ? $parkingconfig['parkext'] : "700";
	$pc['parkposstart'] = isset($parkingconfig['parkposstart']) ? $parkingconfig['parkposstart'] : "701";
	$pc['parkposend'] = isset($parkingconfig['parkposend']) ? $parkingconfig['parkposend'] : "720";
	$pc['parktime'] = isset($parkingconfig['parktime']) ? $parkingconfig['parktime'] : "30";
	
	$featuremapconfig = $config['dialplan']['featuremap'];
	$pc['attendedtransfer'] = isset($featuremapconfig['attendedtransfer']) ? $featuremapconfig['attendedtransfer'] : "**";
	$pc['blindtransfer'] = isset($featuremapconfig['blindtransfer']) ? $featuremapconfig['blindtransfer'] : "##";
	//$pc['disconnect'] = isset($featuremapconfig['disconnect']) ? $featuremapconfig['disconnect'] : "*0";
	$pc['featuredigittimeout'] = isset($featuremapconfig['featuredigittimeout']) ? $featuremapconfig['featuredigittimeout'] : "500";
	$pc['atxfernoanswertimeout'] = isset($featuremapconfig['atxfernoanswertimeout']) ? $featuremapconfig['atxfernoanswertimeout'] : "15";
	$pc['transferdigittimeout'] = isset($featuremapconfig['transferdigittimeout']) ? $featuremapconfig['transferdigittimeout'] : "3";

	$conf = "[general]\n";

	$conf .= "parkext => " . $pc['parkext'] . "\n";
	$conf .= "parkpos => " . $pc['parkposstart'] . "-" . $pc['parkposend'] . "\n";
	$conf .= "context => parkedcalls\n";
	$conf .= "parkingtime => " . $pc['parktime'] . "\n";
	$conf .= "featuredigittimeout = " . $pc['featuredigittimeout'] . "\n";
	$conf .= "atxfernoanswertimeout = " . $pc['atxfernoanswertimeout'] . "\n";
	$conf .= "transferdigittimeout = " . $pc['transferdigittimeout'] . "\n";
	$conf .= "\n";
	
	$conf .= "[featuremap]\n";
	$conf .= "atxfer => " . $pc['attendedtransfer'] . "\n";
	$conf .= "blindxfer => " . $pc['blindtransfer'] . "\n";
//	$conf .= "disconnect => ". $pc['disconnect'] . "\n";
//	$conf .= "automon => *1\n";//                  ; One Touch Record

	$conf .= "\n";

	return ($return) ? $conf : util_file_write("/etc/asterisk/features.conf", $conf);
}

?>
