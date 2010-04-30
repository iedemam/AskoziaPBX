<?php
/*
	$Id$
	part of AskoziaPBX (http://askozia.com/pbx)
	
	Copyright (C) 2007-2008 tecema (a.k.a IKT) <http://www.tecema.de>.
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


/**
 * Generates indications.conf which contains regional tone information and sets this
 * installation's tonezone country
 */
function indications_conf_generate($return=false) {
	global $config;

	$conf = "[general]\n";
	
	if ($config['system']['tonezone']) {
		$conf .= "country=". $config['system']['tonezone'] ."\n\n";
	} else {
		$conf .= "country=us\n\n";
	}
	
	$conf .= <<<EOD


;
; updated from Asterisk 1.6.1.14 source
;

[at]
description = Austria
ringcadence = 1000,5000
dial = 420
busy = 420/400,0/400
ring = 420/1000,0/5000
congestion = 420/200,0/200
callwaiting = 420/40,0/1960
dialrecall = 420
record = 1400/80,0/14920
info = 950/330,1450/330,1850/330,0/1000
stutter = 380+420

[au]
description = Australia
ringcadence = 400,200,400,2000
dial = 413+438
busy = 425/375,0/375
ring = 413+438/400,0/200,413+438/400,0/2000
congestion = 425/375,0/375,420/375,0/375
callwaiting = 425/200,0/200,425/200,0/4400
dialrecall = 413+438
record = !425/1000,!0/15000,425/360,0/15000
info = 425/2500,0/500
std = !525/100,!0/100,!525/100,!0/100,!525/100,!0/100,!525/100,!0/100,!525/100
facility = 425
stutter = 413+438/100,0/40
ringmobile = 400+450/400,0/200,400+450/400,0/2000

[bg]
description = Bulgaria
ringcadence = 1000,4000
dial = 425
busy = 425/500,0/500
ring = 425/1000,0/4000
congestion = 425/250,0/250
callwaiting = 425/150,0/150,425/150,0/4000
dialrecall = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425
record = 1400/425,0/15000
info = 950/330,1400/330,1800/330,0/1000
stutter = 425/1500,0/100

[br]
description = Brazil
ringcadence = 1000,4000
dial = 425
busy = 425/250,0/250
ring = 425/1000,0/4000
congestion = 425/250,0/250,425/750,0/250
callwaiting = 425/50,0/1000
dialrecall = 350+440
record = 425/250,0/250
info = 950/330,1400/330,1800/330
stutter = 350+440

[be]
description = Belgium
ringcadence = 1000,3000
dial = 425
busy = 425/500,0/500
ring = 425/1000,0/3000
congestion = 425/167,0/167
callwaiting = 1400/175,0/175,1400/175,0/3500
dialrecall = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440"
record = 1400/500,0/15000
info = 900/330,1400/330,1800/330,0/1000
stutter = 425/1000,0/250

[ch]
description = Switzerland
ringcadence = 1000,4000
dial = 425
busy = 425/500,0/500
ring = 425/1000,0/4000
congestion = 425/200,0/200
callwaiting = 425/200,0/200,425/200,0/4000
dialrecall = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425
record = 1400/80,0/15000
info = 950/330,1400/330,1800/330,0/1000
stutter = 425+340/1100,0/1100

[cl]
description = Chile
ringcadence = 1000,3000
dial = 400
busy = 400/500,0/500
ring = 400/1000,0/3000
congestion = 400/200,0/200
callwaiting = 400/250,0/8750
dialrecall = !400/100,!0/100,!400/100,!0/100,!400/100,!0/100,400
record = 1400/500,0/15000
info = 950/333,1400/333,1800/333,0/1000
stutter = !400/100,!0/100,!400/100,!0/100,!400/100,!0/100,!400/100,!0/100,!400/100,!0/100,!400/100,!0/100,400

[cn]
description = China
ringcadence = 1000,4000
dial = 450
busy = 450/350,0/350
ring = 450/1000,0/4000
congestion = 450/700,0/700
callwaiting = 450/400,0/4000
dialrecall = 450
record = 950/400,0/10000
info = 450/100,0/100,450/100,0/100,450/100,0/100,450/400,0/400
stutter = 450+425

[cz]
description = Czech Republic
ringcadence = 1000,4000
dial = 425/330,0/330,425/660,0/660
busy = 425/330,0/330
ring = 425/1000,0/4000
congestion = 425/165,0/165
callwaiting = 425/330,0/9000
dialrecall = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425/330,0/330,425/660,0/660
record = 1400/500,0/14000
info = 950/330,0/30,1400/330,0/30,1800/330,0/1000
stutter = 425/450,0/50

[de]
description = Germany
ringcadence = 1000,4000
dial = 425
busy = 425/480,0/480
ring = 425/1000,0/4000
congestion = 425/240,0/240
callwaiting = !425/200,!0/200,!425/200,!0/5000,!425/200,!0/200,!425/200,!0/5000,!425/200,!0/200,!425/200,!0/5000,!425/200,!0/200,!425/200,!0/5000,!425/200,!0/200,!425/200,0
dialrecall = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425
record = 1400/80,0/15000
info = 950/330,1400/330,1800/330,0/1000
stutter = 425+400

[dk]
description = Denmark
ringcadence = 1000,4000
dial = 425
busy = 425/500,0/500
ring = 425/1000,0/4000
congestion = 425/200,0/200
callwaiting = !425/200,!0/600,!425/200,!0/3000,!425/200,!0/200,!425/200,0
dialrecall = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425
record = 1400/80,0/15000
info = 950/330,1400/330,1800/330,0/1000
stutter = 425/450,0/50

[ee]
description = Estonia
ringcadence = 1000,4000
dial = 425
busy = 425/300,0/300
ring = 425/1000,0/4000
congestion = 425/200,0/200
callwaiting = 950/650,0/325,950/325,0/30,1400/1300,0/2600
dialrecall = 425/650,0/25
record = 1400/500,0/15000
info = 950/650,0/325,950/325,0/30,1400/1300,0/2600
stutter = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425

[es]
description = Spain
ringcadence = 1500,3000
dial = 425
busy = 425/200,0/200
ring = 425/1500,0/3000
congestion = 425/200,0/200,425/200,0/200,425/200,0/600
callwaiting = 425/175,0/175,425/175,0/3500
dialrecall = !425/200,!0/200,!425/200,!0/200,!425/200,!0/200,425
record = 1400/500,0/15000
info = 950/330,0/1000
dialout = 500


[fi]
description = Finland
ringcadence = 1000,4000
dial = 425
busy = 425/300,0/300
ring = 425/1000,0/4000
congestion = 425/200,0/200
callwaiting = 425/150,0/150,425/150,0/8000
dialrecall = 425/650,0/25
record = 1400/500,0/15000
info = 950/650,0/325,950/325,0/30,1400/1300,0/2600
stutter = 425/650,0/25

[fr]
description = France
ringcadence = 1500,3500
dial = 440
busy = 440/500,0/500
ring = 440/1500,0/3500
congestion = 440/250,0/250
callwait = 440/300,0/10000
dialrecall = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440
record = 1400/500,0/15000
info = !950/330,!1400/330,!1800/330
stutter = !440/100,!0/100,!440/100,!0/100,!440/100,!0/100,!440/100,!0/100,!440/100,!0/100,!440/100,!0/100,440

[gr]
description = Greece
ringcadence = 1000,4000
dial = 425/200,0/300,425/700,0/800
busy = 425/300,0/300
ring = 425/1000,0/4000
congestion = 425/200,0/200
callwaiting = 425/150,0/150,425/150,0/8000
dialrecall = 425/650,0/25
record = 1400/400,0/15000
info = !950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,0
stutter = 425/650,0/25

[hu]
description = Hungary
ringcadence = 1250,3750
dial = 425
busy = 425/300,0/300
ring = 425/1250,0/3750
congestion = 425/300,0/300
callwaiting = 425/40,0/1960
dialrecall = 425+450
record = 1400/400,0/15000
info = !950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,0
stutter = 350+375+400

[il]
description = Israel
ringcadence = 1000,3000
dial = 414
busy = 414/500,0/500
ring = 414/1000,0/3000
congestion = 414/250,0/250
callwaiting = 414/100,0/100,414/100,0/100,414/600,0/3000
dialrecall = !414/100,!0/100,!414/100,!0/100,!414/100,!0/100,414
record = 1400/500,0/15000
info = 1000/330,1400/330,1800/330,0/1000
stutter = !414/160,!0/160,!414/160,!0/160,!414/160,!0/160,!414/160,!0/160,!414/160,!0/160,!414/160,!0/160,!414/160,!0/160,!414/160,!0/160,!414/160,!0/160,!414/160,!0/160,414


[in]
description = India
ringcadence = 400,200,400,2000
dial = 400*25
busy = 400/750,0/750
ring = 400*25/400,0/200,400*25/400,0/2000
congestion = 400/250,0/250
callwaiting = 400/200,0/100,400/200,0/7500
dialrecall = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440
record = 1400/500,0/15000
info = !950/330,!1400/330,!1800/330,0/1000
stutter = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440

[it]
description = Italy
ringcadence = 1000,4000
dial = 425/200,0/200,425/600,0/1000
busy = 425/500,0/500
ring = 425/1000,0/4000
congestion = 425/200,0/200
callwaiting = 425/400,0/100,425/250,0/100,425/150,0/14000
dialrecall = 470/400,425/400
record = 1400/400,0/15000
info = !950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,0
stutter = 470/400,425/400

[lt]
description = Lithuania
ringcadence = 1000,4000
dial = 425
busy = 425/350,0/350
ring = 425/1000,0/4000
congestion = 425/200,0/200
callwaiting = 425/150,0/150,425/150,0/4000
dialrecall = 425/500,0/50
record = 1400/500,0/15000
info = !950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,0
stutter = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425

[jp]
description = Japan
ringcadence = 1000,2000
dial = 400
busy = 400/500,0/500
ring = 400+15/1000,0/2000
congestion = 400/500,0/500
callwaiting = 400+16/500,0/8000
dialrecall = !400/200,!0/200,!400/200,!0/200,!400/200,!0/200,400
record = 1400/500,0/15000
info = !950/330,!1400/330,!1800/330,0
stutter = !400/100,!0/100,!400/100,!0/100,!400/100,!0/100,!400/100,!0/100,!400/100,!0/100,!400/100,!0/100,400

[mx]
description = Mexico
ringcadence = 2000,4000
dial = 425
busy = 425/250,0/250
ring = 425/1000,0/4000
congestion = 425/250,0/250
callwaiting = 425/200,0/600,425/200,0/10000
dialrecall = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440
record = 1400/500,0/15000
info = 950/330,0/30,1400/330,0/30,1800/330,0/1000
stutter = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440

[my]
description = Malaysia
ringcadence = 2000,4000
dial = 425
busy = 425/500,0/500
ring = 425/400,0/200
congestion = 425/500,0/500

[nl]
description = Netherlands
ringcadence = 1000,4000
dial = 425
busy = 425/500,0/500
ring = 425/1000,0/4000
congestion = 425/250,0/250
callwaiting = 425/500,0/9500
dialrecall = 425/500,0/50
record = 1400/500,0/15000
info = 950/330,1400/330,1800/330,0/1000
stutter = 425/500,0/50

[no]
description = Norway
ringcadence = 1000,4000
dial = 425
busy = 425/500,0/500
ring = 425/1000,0/4000
congestion = 425/200,0/200
callwaiting = 425/200,0/600,425/200,0/10000
dialrecall = 470/400,425/400
record = 1400/400,0/15000
info = !950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,0
stutter = 470/400,425/400

[nz]
description = New Zealand
ringcadence = 400,200,400,2000
dial = 400
busy = 400/250,0/250
ring = 400+450/400,0/200,400+450/400,0/2000
congestion = 400/375,0/375
callwaiting = !400/200,!0/3000,!400/200,!0/3000,!400/200,!0/3000,!400/200
dialrecall = !400/100!0/100,!400/100,!0/100,!400/100,!0/100,400
record = 1400/425,0/15000
info = 400/750,0/100,400/750,0/100,400/750,0/100,400/750,0/400
stutter = !400/100!0/100,!400/100,!0/100,!400/100,!0/100,!400/100!0/100,!400/100,!0/100,!400/100,!0/100,400
unobtainable = 400/75,0/100,400/75,0/100,400/75,0/100,400/75,0/400

[ph]
description = Philippines
ringcadence = 1000,4000
dial = 425
busy = 480+620/500,0/500
ring = 425+480/1000,0/4000
congestion = 480+620/250,0/250
callwaiting = 440/300,0/10000
dialrecall = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440
record = 1400/500,0/15000
info = !950/330,!1400/330,!1800/330,0
stutter = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440


[pl]
description = Poland
ringcadence = 1000,4000
dial = 425
busy = 425/500,0/500
ring = 425/1000,0/4000
congestion = 425/500,0/500
callwaiting = 425/150,0/150,425/150,0/4000
dialrecall = 425/500,0/50
record = 1400/500,0/15000
info = !950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000,!950/330,!1400/330,!1800/330,!0/1000
stutter = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425

[pt]
description = Portugal
ringcadence = 1000,5000
dial = 425
busy = 425/500,0/500
ring = 425/1000,0/5000
congestion = 425/200,0/200
callwaiting = 440/300,0/10000
dialrecall = 425/1000,0/200
record = 1400/500,0/15000
info = 950/330,1400/330,1800/330,0/1000
stutter = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425

[ru]
description = Russian Federation / ex Soviet Union
ringcadence = 1000,4000
dial = 425
busy = 425/350,0/350
ring = 425/1000,0/4000
congestion = 425/175,0/175
callwaiting = 425/200,0/5000
record = 1400/400,0/15000
info = 950/330,1400/330,1800/330,0/1000
dialrecall = 425/400,0/40
stutter = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425

[se]
description = Sweden
ringcadence = 1000,5000
dial = 425
busy = 425/250,0/250
ring = 425/1000,0/5000
congestion = 425/250,0/750
callwaiting = 425/200,0/500,425/200,0/9100
dialrecall = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425
record = 1400/500,0/15000
info = !950/332,!0/24,!1400/332,!0/24,!1800/332,!0/2024,!950/332,!0/24,!1400/332,!0/24,!1800/332,!0/2024,!950/332,!0/24,!1400/332,!0/24,!1800/332,!0/2024,!950/332,!0/24,!1400/332,!0/24,!1800/332,!0/2024,!950/332,!0/24,!1400/332,!0/24,!1800/332,0
stutter = !425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,!425/100,!0/100,425

[sg]
description = Singapore
ringcadence = 400,200,400,2000
dial        = 425
ring        = 425*24/400,0/200,425*24/400,0/2000     ; modulation should be 100%, not 90%
busy        = 425/750,0/750
congestion  = 425/250,0/250
callwaiting = 425*24/300,0/200,425*24/300,0/3200
stutter     = !425/200,!0/200,!425/600,!0/200,!425/200,!0/200,!425/600,!0/200,!425/200,!0/200,!425/600,!0/200,!425/200,!0/200,!425/600,!0/200,425
info        = 950/330,1400/330,1800/330,0/1000       ; not currently in use acc. to reference
dialrecall  = 425*24/500,0/500,425/500,0/2500        ; unspecified in IDA reference, use repeating Holding Tone A,B
record      = 1400/500,0/15000                       ; unspecified in IDA reference, use 0.5s tone every 15s
nutone      = 425/2500,0/500
intrusion   = 425/250,0/2000
warning     = 425/624,0/4376                         ; end of period tone, warning
acceptance  = 425/125,0/125
holdinga    = !425*24/500,!0/500                     ; followed by holdingb
holdingb    = !425/500,!0/2500

[th]
description = Thailand
ringcadence = 1000,4000
dial = 400*50
busy = 400/500,0/500
ring = 420/1000,0/5000
congestion = 400/300,0/300
callwaiting = 1000/400,10000/400,1000/400
dialrecall = 400*50/400,0/100,400*50/400,0/100
record = 1400/500,0/15000
info = 950/330,1400/330,1800/330
stutter = !400/200,!0/200,!400/600,!0/200,!400/200,!0/200,!400/600,!0/200,!400/200,!0/200,!400/600,!0/200,!400/200,!0/200,!400/600,!0/200,400

[uk]
description = United Kingdom
ringcadence = 400,200,400,2000
dial = 350+440
specialdial = 350+440/750,440/750
busy = 400/375,0/375
congestion = 400/400,0/350,400/225,0/525
specialcongestion = 400/200,1004/300
unobtainable = 400
ring = 400+450/400,0/200,400+450/400,0/2000
callwaiting = 400/100,0/4000
specialcallwaiting = 400/250,0/250,400/250,0/250,400/250,0/5000
creditexpired = 400/125,0/125
confirm = 1400
switching = 400/200,0/400,400/2000,0/400
info = 950/330,0/15,1400/330,0/15,1800/330,0/1000
record = 1400/500,0/60000
stutter = 350+440/750,440/750

[us]
description = United States / North America
ringcadence = 2000,4000
dial = 350+440
busy = 480+620/500,0/500
ring = 440+480/2000,0/4000
congestion = 480+620/250,0/250
callwaiting = 440/300,0/10000
dialrecall = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440
record = 1400/500,0/15000
info = !950/330,!1400/330,!1800/330,0
stutter = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440

[us-old]
description = United States Circa 1950/ North America
ringcadence = 2000,4000
dial = 600*120
busy = 500*100/500,0/500
ring = 420*40/2000,0/4000
congestion = 500*100/250,0/250
callwaiting = 440/300,0/10000
dialrecall = !600*120/100,!0/100,!600*120/100,!0/100,!600*120/100,!0/100,600*120
record = 1400/500,0/15000
info = !950/330,!1400/330,!1800/330,0
stutter = !600*120/100,!0/100,!600*120/100,!0/100,!600*120/100,!0/100,!600*120/100,!0/100,!600*120/100,!0/100,!600*120/100,!0/100,600*120

[tw]
description = Taiwan
ringcadence = 1000,4000
dial = 350+440
busy = 480+620/500,0/500
ring = 440+480/1000,0/2000
congestion = 480+620/250,0/250
callwaiting = 350+440/250,0/250,350+440/250,0/3250
dialrecall = 300/1500,0/500
record = 1400/500,0/15000
info = !950/330,!1400/330,!1800/330,0
stutter = !350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,!350+440/100,!0/100,350+440

[ve]
description = Venezuela / South America
ringcadence = 1000,4000
dial = 425
busy = 425/500,0/500
ring = 425/1000,0/4000
congestion = 425/250,0/250
callwaiting = 400+450/300,0/6000
dialrecall = 425
record = 1400/500,0/15000
info = !950/330,!1440/330,!1800/330,0/1000


[za]
description = South Africa
ringcadence = 400,200,400,2000
dial = 400*33
ring = 400*33/400,0/200,400*33/400,0/2000
callwaiting = 400*33/250,0/250,400*33/250,0/250,400*33/250,0/250,400*33/250,0/250
congestion = 400/250,0/250
busy = 400/500,0/500
dialrecall = 350+440
record = 1400/500,0/10000
info = 950/330,1400/330,1800/330,0/330
stutter = !400*33/100,!0/100,!400*33/100,!0/100,!400*33/100,!0/100,!400*33/100,!0/100,!400*33/100,!0/100,!400*33/100,!0/100,400*33


EOD;

	return ($return) ? $conf : util_file_write("/etc/asterisk/indications.conf", $conf);

}

?>
