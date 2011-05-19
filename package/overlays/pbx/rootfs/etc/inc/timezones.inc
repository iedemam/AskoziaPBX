<?php
/*
	$Id:$
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

$tz['0000'] = array(gettext("UTC"), "UTC", "UTC");

$tmp = gettext("Asia");
$tz['0100'] = array($tmp.", ".gettext("Jakarta"), "WIB-7", "Asia/Jakarta");
$tz['0200'] = array($tmp.", ".gettext("Singapore"), "SGT-8", "Asia/Singapore");
$tz['0300'] = array($tmp.", ".gettext("Ulaanbaatar, Mongolia"), "ULAT-8ULAST,M3.5.0/2,M9.5.0/2", "Asia/Ulaanbaatar");
$tz['0350'] = array($tmp.", ".gettext("Tokyo, Japan"), "JST", "Asia/Tokyo");
$tmp = gettext("Australia");
$tz['0400'] = array($tmp.", ".gettext("Adelaide"), "CST-9:30CDT-10:30,M10.5.0/02:00:00,M3.5.0/03:00:00", "Australia/Adelaide");
$tz['0500'] = array($tmp.", ".gettext("Brisbane"), "EST-10", "Australia/Brisbane");
$tz['0600'] = array($tmp.", ".gettext("Canberra"), "EST-10EDT-11,M10.5.0/02:00:00,M3.5.0/03:00:00", "Australia/Canberra");
$tz['0700'] = array($tmp.", ".gettext("Darwin"), "CST-9:30", "Australia/Darwin");
$tz['0800'] = array($tmp.", ".gettext("Hobart"), "EST-10EDT-11,M10.1.0/02:00:00,M3.5.0/03:00:00", "Australia/Hobart");
$tz['0900'] = array($tmp.", ".gettext("Melbourne"), "EST-10EDT-11,M10.5.0/02:00:00,M3.5.0/03:00:00", "Australia/Melbourne");
$tz['1000'] = array($tmp.", ".gettext("Perth"), "WST-8", "Australia/Perth");
$tz['1100'] = array($tmp.", ".gettext("Sydney"), "EST-10EDT-11,M10.5.0/02:00:00,M3.5.0/03:00:00", "Australia/Sydney");

$tmp = gettext("Central and South America");
$tz['1200'] = array($tmp.", ".gettext("Argentina"), "UTC+3", "America/Argentina/Buenos_Aires");
$tz['1300'] = array($tmp.", ".gettext("Brazil, São Paulo"), "BRST+3BRDT+2,M10.3.0,M2.3.0", "America/Sao_Paulo");
$tz['1400'] = array($tmp.", ".gettext("Central America"), "CST+6", "CST6CDT");

$tmp = gettext("Europe");
$tz['1500'] = array($tmp.", ".gettext("Amsterdam, Netherlands"), "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", "Europe/Amsterdam");
$tz['1600'] = array($tmp.", ".gettext("Athens, Greece"), "EET-2EEST-3,M3.5.0/03:00:00,M10.5.0/04:00:00", "Europe/Athens");
$tz['1700'] = array($tmp.", ".gettext("Barcelona, Spain"), "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", "Europe/Berlin");
$tz['1800'] = array($tmp.", ".gettext("Berlin, Germany"), "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", "Europe/Berlin");
$tz['1900'] = array($tmp.", ".gettext("Brussels, Belgium"), "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", "Europe/Brussels");
$tz['2000'] = array($tmp.", ".gettext("Budapest, Hungary"), "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", "Europe/Budapest");
$tz['2100'] = array($tmp.", ".gettext("Copenhagen, Denmark"), "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", "Europe/Copenhagen");
$tz['2200'] = array($tmp.", ".gettext("Dublin, Ireland"), "GMT+0IST-1,M3.5.0/01:00:00,M10.5.0/02:00:00", "Europe/Dublin");
$tz['2300'] = array($tmp.", ".gettext("Geneva, Switzerland"), "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", "Europe/Copenhagen");
$tz['2400'] = array($tmp.", ".gettext("Helsinki, Finland"), "EET-2EEST-3,M3.5.0/03:00:00,M10.5.0/04:00:00", "Europe/Helsinki");
$tz['2500'] = array($tmp.", ".gettext("Kiev, Ukraine"), "EET-2EEST,M3.5.0/3,M10.5.0/4", "Europe/Kiev");
$tz['2600'] = array($tmp.", ".gettext("Lisbon, Portugal"), "WET-0WEST-1,M3.5.0/01:00:00,M10.5.0/02:00:00", "Europe/Lisbon");
$tz['2700'] = array($tmp.", ".gettext("London, Great Britain"), "GMT+0BST-1,M3.5.0/01:00:00,M10.5.0/02:00:00", "Europe/London");
$tz['2800'] = array($tmp.", ".gettext("Madrid, Spain"), "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", "Europe/Madrid");
$tz['2900'] = array($tmp.", ".gettext("Oslo, Norway"), "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", "Europe/Oslo");
$tz['3000'] = array($tmp.", ".gettext("Paris, France"), "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", "Europe/Paris");
$tz['3100'] = array($tmp.", ".gettext("Prague, Czech Republic"), "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", "Europe/Prague");
$tz['3200'] = array($tmp.", ".gettext("Rome, Italy"), "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", "Europe/Rome");
$tz['3300'] = array($tmp.", ".gettext("Moscow, Russia"), "MSK-3MSD,M3.5.0/2,M10.5.0/3", "Europe/Moscow");
//$tz['3400'] = array($tmp.", ".gettext("St.Petersburg, Russia"), "MST-3MDT,M3.5.0/2,M10.5.0/3"); /* need zoneinfo file */
$tz['3500'] = array($tmp.", ".gettext("Stockholm, Sweden"), "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", "Europe/Stockholm");
$tz['3600'] = array($tmp.", ".gettext("Tallinn, Estonia"), "EET-2EEST-3,M3.5.0/03:00:00,M10.5.0/04:00:00", "Europe/Tallinn");

$tmp = gettext("New Zealand");
$tz['3700'] = array($tmp.", ".gettext("Auckland"), "NZST-12NZDT-13,M10.1.0/02:00:00,M3.3.0/03:00:00", "Pacific/Auckland");
$tz['3800'] = array($tmp.", ".gettext("Wellington"), "NZST-12NZDT-13,M10.1.0/02:00:00,M3.3.0/03:00:00", "Pacific/Auckland");

$tmp = gettext("USA & Canada");
$tz['3900'] = array($tmp.", ".gettext("Alaska Time"), "AKST9AKDT", "US/Alaska");
$tz['4000'] = array($tmp.", ".gettext("Atlantic Time"), "AST4ADT", "Canada/Atlantic");
//$tz['4100'] = array($tmp.", ".gettext("Atlantic Time (New Brunswick)"), "AST4ADT,M4.1.0/00:01:00,M10.5.0/00:01:00");/* !! */
$tz['4200'] = array($tmp.", ".gettext("Central Time"), "CST6CDT", "US/Central");
$tz['4300'] = array($tmp.", ".gettext("Eastern Time"), "EST5EDT", "US/Eastern");
$tz['4400'] = array($tmp.", ".gettext("Hawaii Time"), "HAW10", "US/Hawaii");
$tz['4500'] = array($tmp.", ".gettext("Mountain Time"), "MST7MDT", "US/Mountain");
$tz['4600'] = array($tmp.", ".gettext("Mountain Time (Arizona, no DST)"), "MST7", "US/Arizona");
$tz['4700'] = array($tmp.", ".gettext("Newfoundland Time"), "NST+3:30NDT+2:30,M4.1.0/00:01:00,M10.5.0/00:01:00", "Canada/Newfoundland");
$tz['4800'] = array($tmp.", ".gettext("Pacific Time"), "PST8PDT", "US/Pacific");

?>
