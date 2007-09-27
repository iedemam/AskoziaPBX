/*
	$Id: interfaces_wlan.js 220 2007-08-25 20:09:05Z mkasper $
	part of m0n0wall (http://m0n0.ch/wall)
	
	Copyright (C) 2003-2007 Manuel Kasper <mk@neon1.net>.
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

function wlan_enable_change(enable_over) {

	var if_enable = (document.iform.enable.checked || enable_over);
	
	// WPA only in hostap mode
	//var wpa_enable = (if_enable && document.iform.mode.options[document.iform.mode.selectedIndex].value == "hostap") || enable_over;
	var wpa_enable = (if_enable || enable_over);
	var wpa_opt_enable = (if_enable && document.iform.wpamode.options[document.iform.wpamode.selectedIndex].value != "none") || enable_over;
	
	// enter WPA PSK only in PSK mode
	var wpa_psk_enable = (if_enable && document.iform.wpamode.options[document.iform.wpamode.selectedIndex].value == "psk") || enable_over;
	
	// RADIUS server only in WPA Enterprise mode
	//var wpa_ent_enable = (if_enable && document.iform.wpamode.options[document.iform.wpamode.selectedIndex].value == "enterprise") || enable_over;
	
	// WEP only if WPA is disabled
	var wep_enable = (if_enable && document.iform.wpamode.options[document.iform.wpamode.selectedIndex].value == "none") || enable_over;
	
	document.iform.standard.disabled = !if_enable;
	//document.iform.mode.disabled = !if_enable;
	document.iform.ssid.disabled = !if_enable;
	document.iform.channel.disabled = !if_enable;
	document.iform.hidessid.disabled = !if_enable;
	document.iform.wep_enable.disabled = !wep_enable;
	document.iform.key1.disabled = !wep_enable;
	document.iform.key2.disabled = !wep_enable;
	document.iform.key3.disabled = !wep_enable;
	document.iform.key4.disabled = !wep_enable;
	
	document.iform.wpamode.disabled = !if_enable;
	document.iform.wpaversion.disabled = !wpa_opt_enable;
	document.iform.wpacipher.disabled = !wpa_opt_enable;
	document.iform.wpapsk.disabled = !wpa_psk_enable;
	//document.iform.radiusip.disabled = !wpa_ent_enable;
	//document.iform.radiusauthport.disabled = !wpa_ent_enable;
	//document.iform.radiusacctport.disabled = !wpa_ent_enable;
	//document.iform.radiussecret.disabled = !wpa_ent_enable;
}
