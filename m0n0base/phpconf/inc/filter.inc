<?php
/*
	$Id: filter.inc 178 2006-12-23 17:51:36Z mkasper $
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
	
/* include all configuration functions */
require_once("functions.inc");
	
function filter_resync() {
	global $config, $g;
	
	mwexec("/sbin/ipf -y");
}

function filter_ipmon_start() {
	global $config, $g;
	
	mwexec("/sbin/ipmon -sD");
}

function filter_configure() {
	global $config, $g;
	
	if ($g['booting'])
		echo "Configuring firewall... ";
	
	/* set TCP timeouts */
	if ($config['filter']['tcpidletimeout'] && $g['booting']) {
		$tcpidletimeout = $config['filter']['tcpidletimeout']*2;
		
		/* ipfilter now needs to be disabled briefly in order to change the timeout value :( */
		mwexec("/sbin/ipf -D");
		mwexec("/sbin/sysctl net.inet.ipf.fr_tcpidletimeout={$tcpidletimeout}");
		mwexec("/sbin/ipf -E");
	}
	
	/* generate ipnat rules */
	$ipnatrules = filter_nat_rules_generate();
	
	/* load ipnat rules */
	$fd = popen("/sbin/ipnat -C -f - > /dev/null 2>&1", "w");
	if (!$fd) {
		printf("Cannot open /sbin/ipnat in filter_configure()\n");
		return 1;
	}
		
	fwrite($fd, $ipnatrules);
	pclose($fd);
	
	/* generate ipf rules */
	$ipfrules = filter_rules_generate();
	
	$fd = popen("/sbin/ipf -Fa -f - > /dev/null 2>&1", "w");
	if (!$fd) {
		printf("Cannot open /sbin/ipf in filter_configure()\n");
		return 1;
	}
		
	fwrite($fd, $ipfrules);
	pclose($fd);
	
	if ($g['booting'])
		echo "done\n";
	
	return 0;
}

function filter_flush_nat_table() {
	global $config, $g;
	
	return mwexec("/sbin/ipnat -F");
}

function filter_flush_state_table() {
	global $config, $g;
	
	return mwexec("/sbin/ipf -FS");
}

function filter_nat_rules_generate_if($if, $src, $dst, $target, $portmap = true) {

	if ($target)
		$tgt = $target . "/32";
	else
		$tgt = "0/32";
	
	$natrule = "map $if $src $dst -> {$tgt} proxy port ftp ftp/tcp\n";
	
	if ($portmap)
		$natrule .= "map $if $src $dst -> {$tgt} portmap tcp/udp auto\n";
	
	$natrule .= "map $if $src $dst -> {$tgt}\n";
	
	return $natrule;
}

function filter_nat_rules_generate() {
	global $config, $g;
	
	$wancfg = $config['interfaces']['wan'];
	$lancfg = $config['interfaces']['lan'];
	
	$pptpdcfg = $config['pptpd'];
	$wanif = get_real_wan_interface();
	
	$lansa = gen_subnet($lancfg['ipaddr'], $lancfg['subnet']);
	
	$natrules = "";
	
	/* any 1:1 mappings? */
	if (is_array($config['nat']['onetoone'])) {
		foreach ($config['nat']['onetoone'] as $natent) {
			if (!is_numeric($natent['subnet']))
				$sn = 32;
			else
				$sn = $natent['subnet'];
			
			if (!$natent['interface'] || ($natent['interface'] == "wan"))
				$natif = $wanif;
			else
				$natif = $config['interfaces'][$natent['interface']]['if'];
			
			$natrules .= "bimap {$natif} {$natent['internal']}/{$sn} -> {$natent['external']}/{$sn}\n";
		}
	}
	
	/* outbound rules - advanced or standard */
	if (isset($config['nat']['advancedoutbound']['enable'])) {
		/* advanced outbound rules */
		if (is_array($config['nat']['advancedoutbound']['rule'])) {
			foreach ($config['nat']['advancedoutbound']['rule'] as $obent) {				
				$dst = "";
				$src = "";
				if (!isset($obent['destination']['any'])) {
					$src = "from ";
					if (isset($obent['destination']['not'])) 
						$dst = "! to ";
					else
						$dst = "to ";
					$dst .= $obent['destination']['network'];
				}
				$src .= $obent['source']['network'];
				
				if (!$obent['interface'] || ($obent['interface'] == "wan"))
					$natif = $wanif;
				else
					$natif = $config['interfaces'][$obent['interface']]['if'];
				
				$natrules .= filter_nat_rules_generate_if($natif, $src, $dst,
					$obent['target'], !isset($obent['noportmap']));
			}
		}	
	} else {
		/* standard outbound rules (one for each interface) */
		$natrules .= filter_nat_rules_generate_if($wanif,
			$lansa . "/" . $lancfg['subnet'], "", null);
	
		/* optional interfaces */
		for ($i = 1; isset($config['interfaces']['opt' . $i]); $i++) {
			$optcfg = $config['interfaces']['opt' . $i];
			
			if (isset($optcfg['enable']) && !$optcfg['bridge']) {
				$optsa = gen_subnet($optcfg['ipaddr'], $optcfg['subnet']);
				$natrules .= filter_nat_rules_generate_if($wanif, 
					$optsa . "/" . $optcfg['subnet'], "", null);
			}
		}
		
		/* PPTP subnet */
		if ($pptpdcfg['mode'] == "server") {
			$natrules .= filter_nat_rules_generate_if($wanif,
				$pptpdcfg['remoteip'] . "/" . $g['pptp_subnet'], "", null);
		}
		
		/* static routes */
		if (is_array($config['staticroutes']['route'])) {
			foreach ($config['staticroutes']['route'] as $route) {
				if ($route['interface'] != "wan")
					$natrules .= filter_nat_rules_generate_if($wanif,
						$route['network'], "", null);
			}
		}
	}
	
	/* DIAG: add ipv6 NAT, if requested */
	if (isset($config['diag']['ipv6nat']['enable'])) {
		$natrules .= "rdr $wanif 0/0 port 0 -> " . 
		"{$config['diag']['ipv6nat']['ipaddr']} port 0 ipv6\n";
	}
	
	if (isset($config['nat']['rule'])) {
		foreach ($config['nat']['rule'] as $rule) {
		
			$extport = explode("-", $rule['external-port']);
			$target = alias_expand_host($rule['target']);
			
			if (!$target)
				continue;	/* unresolvable alias */
			
			if ($rule['external-address'])
				$extaddr = $rule['external-address'] . "/32";
			else
				$extaddr = "0/0";
			
			if (!$rule['interface'] || ($rule['interface'] == "wan"))
				$natif = $wanif;
			else
				$natif = $config['interfaces'][$rule['interface']]['if'];
			
			if ((!$extport[1]) || ($extport[0] == $extport[1])) {
				$natrules .=
					"rdr $natif {$extaddr} port {$extport[0]} -> {$target} " .
					"port {$rule['local-port']} {$rule['protocol']}";
			} else {
				$natrules .=
					"rdr $natif {$extaddr} port {$extport[0]}-{$extport[1]} " .
					"-> {$target} " .
					"port {$rule['local-port']} {$rule['protocol']}";
			}
				
			$natrules .= "\n";
		}
	}
	
	if ($pptpdcfg['mode'] == "redir" && $pptpdcfg['redir']) {
	
		$natrules .= <<<EOD

# PPTP
rdr $wanif 0/0 port 0 -> {$pptpdcfg['redir']} port 0 gre
rdr $wanif 0/0 port 1723 -> {$pptpdcfg['redir']} port 1723 tcp

EOD;
	}
	
	return $natrules;
}

function filter_rules_generate() {
	global $config, $g;
	
	$wancfg = $config['interfaces']['wan'];
	$lancfg = $config['interfaces']['lan'];
	$pptpdcfg = $config['pptpd'];
	
	$lanif = $lancfg['if'];
	$wanif = get_real_wan_interface();
	
	$curwanip = get_current_wan_address();
	
	/* rule groups (optional interfaces: see below) */
	$ifgroups = array("lan" => 100, "wan" => 200);
	
	$lanip = $lancfg['ipaddr'];
	$lansa = gen_subnet($lancfg['ipaddr'], $lancfg['subnet']);
	$lansn = $lancfg['subnet'];
	
	/* optional interfaces */
	$optcfg = array();
	
	for ($i = 1; isset($config['interfaces']['opt' . $i]); $i++) {
		$oc = $config['interfaces']['opt' . $i];
		
		if (isset($oc['enable']) && $oc['if']) {
			$oic = array();
			$oic['if'] = $oc['if'];
			
			if ($oc['bridge']) {
				if (!strstr($oc['bridge'], "opt") || 
					 isset($config['interfaces'][$oc['bridge']]['enable'])) {
					if (is_ipaddr($config['interfaces'][$oc['bridge']]['ipaddr'])) {
						$oic['ip'] = $config['interfaces'][$oc['bridge']]['ipaddr'];
						$oic['sn'] = $config['interfaces'][$oc['bridge']]['subnet'];
						$oic['sa'] = gen_subnet($oic['ip'], $oic['sn']);
					}
				}
				$oic['bridge'] = 1;
				$oic['bridge_if'] = $oc['bridge'];
			} else {
				$oic['ip'] = $oc['ipaddr'];
				$oic['sn'] = $oc['subnet'];
				$oic['sa'] = gen_subnet($oic['ip'], $oic['sn']);
			}
			
			$optcfg['opt' . $i] = $oic;
			$ifgroups['opt' . $i] = ($i * 100) + 200;
		}
	}
	
	if ($pptpdcfg['mode'] == "server") {
		$pptpip = $pptpdcfg['localip'];
		$pptpsa = $pptpdcfg['remoteip'];
		$pptpsn = $g['pptp_subnet'];
	}
	
	/* default block logging? */
	if (!isset($config['syslog']['nologdefaultblock']))
		$log = "log";
	else
		$log = "";
	
	$ipfrules = <<<EOD
# loopback
pass in quick on lo0 all
pass out quick on lo0 all

# block short packets
block in $log quick all with short

# block IP options
block in $log quick all with ipopts

# allow access to DHCP server on LAN
pass in quick on $lanif proto udp from any port = 68 to 255.255.255.255 port = 67
pass in quick on $lanif proto udp from any port = 68 to $lanip port = 67
pass out quick on $lanif proto udp from $lanip port = 67 to any port = 68

EOD;

	/* allow access to DHCP server on optional interfaces */
	foreach ($optcfg as $on => $oc) {
		if ((isset($config['dhcpd'][$on]['enable']) && !$oc['bridge']) ||
				($oc['bridge'] && isset($config['dhcpd'][$oc['bridge_if']]['enable']))) {
					
			$ipfrules .= <<<EOD

# allow access to DHCP server on {$on}
pass in quick on {$oc['if']} proto udp from any port = 68 to 255.255.255.255 port = 67
pass in quick on {$oc['if']} proto udp from any port = 68 to {$oc['ip']} port = 67
pass out quick on {$oc['if']} proto udp from {$oc['ip']} port = 67 to any port = 68

EOD;
		}
	}
	
	/* pass traffic between statically routed subnets and the subnet on the
	   interface in question to avoid problems with complicated routing
	   topologies */
	if (isset($config['filter']['bypassstaticroutes']) && is_array($config['staticroutes']['route']) && count($config['staticroutes']['route'])) {
		foreach ($config['staticroutes']['route'] as $route) {
			unset($sa);
			
			if ($route['interface'] == "lan") {
				$ip = $lanip;
				$sa = $lansa;
				$sn = $lansn;
				$if = $lanif;
			} else if (strstr($route['interface'], "opt")) {
				$oc = $optcfg[$route['interface']];
				if ($oc['ip']) {
					$ip = $oc['ip'];
					$sa = $oc['sa'];
					$sn = $oc['sn'];
					$if = $oc['if'];	
				}
			}
			
			if ($sa) {
				$ipfrules .= <<<EOD
skip 2 in on {$if} from any to {$ip}
pass in quick on {$if} from {$sa}/{$sn} to {$route['network']}
pass in quick on {$if} from {$route['network']} to {$sa}/{$sn}
pass out quick on {$if} from {$sa}/{$sn} to {$route['network']}
pass out quick on {$if} from {$route['network']} to {$sa}/{$sn}

EOD;
			}
		}
	}

	$ipfrules .= <<<EOD

# WAN spoof check
block in $log quick on $wanif from $lansa/$lansn to any

EOD;

	foreach ($optcfg as $oc) {
		if (!$oc['bridge'])
			$ipfrules .= "block in $log quick on $wanif from {$oc['sa']}/{$oc['sn']} to any\n";
	}
	
	/* allow PPTP traffic if PPTP client is enabled on WAN */
	if ($wancfg['ipaddr'] == "pptp") {
		$ipfrules .= <<<EOD

# allow PPTP client
pass in quick on {$wancfg['if']} proto gre from any to any
pass out quick on {$wancfg['if']} proto gre from any to any
pass in quick on {$wancfg['if']} proto tcp from any port = 1723 to any
pass out quick on {$wancfg['if']} proto tcp from any to any port = 1723

EOD;
	}

	$ipfrules .= <<<EOD

# allow our DHCP client out to the WAN
# XXX - should be more restrictive
# (not possible at the moment - need 'me' like in ipfw)
pass out quick on $wanif proto udp from any port = 68 to any port = 67
block in $log quick on $wanif proto udp from any port = 67 to $lansa/$lansn port = 68
pass in quick on $wanif proto udp from any port = 67 to any port = 68

# LAN/OPT spoof check (needs to be after DHCP because of broadcast addresses)

EOD;

	/* LAN spoof check */
	/* omit if any interface is bridged to LAN and the filtering bridge is on */
	$lanbridge = false;
	foreach ($optcfg as $on => $oc) {
		if ($oc['bridge'] && $oc['bridge_if'] == "lan") {
			$lanbridge = true;
			break;
		}
	}
	if (!$lanbridge || !isset($config['bridge']['filteringbridge']))
		$ipfrules .= filter_rules_spoofcheck_generate('lan', $lanif, $lansa, $lansn, $log);

	/* OPT spoof check */
	foreach ($optcfg as $on => $oc) {
		/* omit for bridged interfaces when the filtering bridge is on */
		$isbridged = false;
		foreach ($optcfg as $on2 => $oc2) {
			if ($oc2['bridge'] && $oc2['bridge_if'] == $on) {
				$isbridged = true;
				break;
			}
		}
		
		if ($oc['ip'] && !(($oc['bridge'] || $isbridged) && isset($config['bridge']['filteringbridge'])))
			$ipfrules .= filter_rules_spoofcheck_generate($on, $oc['if'], $oc['sa'], $oc['sn'], $log);
	}
	
	/* block private networks on WAN? */
	if (isset($config['interfaces']['wan']['blockpriv'])) {
		$ipfrules .= <<<EOD

# block anything from private networks on WAN interface
block in $log quick on $wanif from 10.0.0.0/8 to any
block in $log quick on $wanif from 127.0.0.0/8 to any
block in $log quick on $wanif from 172.16.0.0/12 to any
block in $log quick on $wanif from 192.168.0.0/16 to any

EOD;
	}
	
	/* IPsec enabled? */
	if (isset($config['ipsec']['enable']) && 
		((is_array($config['ipsec']['tunnel']) &&
		count($config['ipsec']['tunnel'])) ||
			isset($config['ipsec']['mobileclients']['enable']))) {
		
		if ($curwanip)
			$ipfrules .= filter_rules_ipsec_generate($wanif, $curwanip);
		
		$ipfrules .= filter_rules_ipsec_generate($lanif, $lanip);
		
		foreach ($optcfg as $on => $oc) {
			if ($oc['ip'])
				$ipfrules .= filter_rules_ipsec_generate($oc['if'], $oc['ip']);
		}
	}

	$out_kf = "";
	if (isset($config['filter']['allowipsecfrags']))
		$out_kf = "keep frags";

	/* XXX - the first section is only needed because ipf refuses to
		parse rules that have "flags S/SAFR" and proto "tcp/udp" set because
		UDP does not have flags, but we still want to offer the TCP/UDP protocol
		option to the user */
		
	$ipfrules .= <<<EOD

# Block TCP packets that do not mark the start of a connection
skip 1 in proto tcp all flags S/SAFR
block in $log quick proto tcp all

#---------------------------------------------------------------------------
# group head 100 - LAN interface
#---------------------------------------------------------------------------
block in $log quick on $lanif all head 100

# let out anything from the firewall host itself and decrypted IPsec traffic
pass out quick on $lanif all keep state $out_kf

#---------------------------------------------------------------------------
# group head 200 - WAN interface
#---------------------------------------------------------------------------
block in $log quick on $wanif all head 200

# let out anything from the firewall host itself and decrypted IPsec traffic
pass out quick on $wanif all keep state $out_kf

EOD;

	/* group heads for optional interfaces */
	foreach ($optcfg as $on => $oc) {
	
		$ingroup = $ifgroups[$on];
	
		$ipfrules .= <<<EOD
		
#---------------------------------------------------------------------------
# group head {$ingroup} - {$on} interface
#---------------------------------------------------------------------------
block in $log quick on {$oc['if']} all head {$ingroup}

# let out anything from the firewall host itself and decrypted IPsec traffic
pass out quick on {$oc['if']} all keep state $out_kf

EOD;
	
	}
	
	if (!isset($config['system']['webgui']['noantilockout'])) {
	
	$ipfrules .= <<<EOD

# make sure the user cannot lock himself out of the webGUI
pass in quick from $lansa/$lansn to $lanip keep state group 100

EOD;
	}
	
	/* PPTPd enabled? */
	if ($pptpdcfg['mode'] && ($pptpdcfg['mode'] != "off")) {
	
		if ($pptpdcfg['mode'] == "server")
			$pptpdtarget = $curwanip;
		else
			$pptpdtarget = $pptpdcfg['redir'];
		
		if ($pptpdtarget) {
			$ipfrules .= <<<EOD

# PPTP rules
pass in quick proto gre from any to $pptpdtarget keep state group 200
pass in quick proto tcp from any to $pptpdtarget port = 1723 keep state group 200

EOD;
		}
	}
	
	$i = 0;
	
	$ipfrules .= "\n# User-defined rules follow\n";
	
	if (isset($config['filter']['rule']))
		foreach ($config['filter']['rule'] as $rule) {
		
		/* don't include disabled rules */
		if (isset($rule['disabled'])) {
			$i++;
			continue;
		}
		
		/* does the rule deal with a PPTP interface? */
		if ($rule['interface'] == "pptp") {
		
			if ($pptpdcfg['mode'] != "server") {
				$i++;
				continue;
			}
				
			$nif = $g['n_pptp_units'];
			$ispptp = true;
		} else {
			
			if (strstr($rule['interface'], "opt")) {
				if (!array_key_exists($rule['interface'], $optcfg)) {
					$i++;
					continue;
				}
			}
			
			$nif = 1;
			$ispptp = false;
		}
		
		if ($pptpdcfg['mode'] != "server") {
			if (($rule['source']['network'] == "pptp") ||
				($rule['destination']['network'] == "pptp")) {
					$i++;
					continue;
				}
		}
		
		if ($rule['source']['network'] && strstr($rule['source']['network'], "opt")) {
			if (!array_key_exists($rule['source']['network'], $optcfg)) {
				$i++;
				continue;
			}
		}
		if ($rule['destination']['network'] && strstr($rule['destination']['network'], "opt")) {
			if (!array_key_exists($rule['destination']['network'], $optcfg)) {
				$i++;
				continue;
			}
		}
		
		/* check for unresolvable aliases */
		if ($rule['source']['address'] && !alias_expand($rule['source']['address'])) {
			$i++;
			continue;
		}
		if ($rule['destination']['address'] && !alias_expand($rule['destination']['address'])) {
			$i++;
			continue;
		}
		
		for ($iif = 0; $iif < $nif; $iif++) {
			
			if (!$ispptp) {
			
				$groupnum = $ifgroups[$rule['interface']];
				
				if (!$groupnum) {
					printf("Invalid interface name in rule $i\n");
					break;
				}
			}
			
			$type = $rule['type'];
			if ($type != "pass" && $type != "block" && $type != "reject") {
				/* default (for older rules) is pass */
				$type = "pass";
			}

			if ($type == "reject") {
				/* special reject packet */
				if ($rule['protocol'] == "tcp") {
					$line = "block return-rst";
				} else if ($rule['protocol'] == "udp") {
					$line = "block return-icmp(port-unr)";
				} else {
					$line = "block";
				}
			} else {
				$line = $type;
			}
			
			$line .= " in ";

			if (isset($rule['log']))
				$line .= "log first ";

			$line .= "quick ";
			
			if ($ispptp) {
				$line .= "on ng" . ($iif+1) . " ";
			}
			
			if (isset($rule['protocol'])) {
				$line .= "proto {$rule['protocol']} ";
			}
			
			/* source address */
			if (isset($rule['source']['any'])) {
				$src = "any";
			} else if ($rule['source']['network']) {
				
				if (strstr($rule['source']['network'], "opt")) {
					$src = $optcfg[$rule['source']['network']]['sa'] . "/" .
						$optcfg[$rule['source']['network']]['sn'];
				} else {
					switch ($rule['source']['network']) {
						case 'wanip':
							$src = $curwanip;
							break;
						case 'lan':
							$src = "$lansa/$lansn";
							break;
						case 'pptp':
							$src = "$pptpsa/$pptpsn";
							break;
					}
				}
			} else if ($rule['source']['address']) {
				$src = alias_expand($rule['source']['address']);
			}
			
			if (!$src || ($src == "/")) {
				//printf("No source address found in rule $i\n");
				break;
			}
			
			if (isset($rule['source']['not'])) {
				$line .= "from !$src ";
			} else {
				$line .= "from $src ";
			}
			
			if (in_array($rule['protocol'], array("tcp","udp","tcp/udp"))) {
				
				if ($rule['source']['port']) {
					$srcport = explode("-", $rule['source']['port']);
					
					if ((!$srcport[1]) || ($srcport[0] == $srcport[1])) {
						$line .= "port = {$srcport[0]} ";
					} else if (($srcport[0] == 1) && ($srcport[1] == 65535)) {
						/* no need for a port statement here */
					} else if ($srcport[1] == 65535) {
						$line .= "port >= {$srcport[0]} "; 
					} else if ($srcport[0] == 1) {
						$line .= "port <= {$srcport[1]} "; 
					} else {
						$srcport[0]--;
						$srcport[1]++;
						$line .= "port {$srcport[0]} >< {$srcport[1]} ";
					}
				}
			}
			
			/* destination address */
			if (isset($rule['destination']['any'])) {
				$dst = "any";
			} else if ($rule['destination']['network']) {
				
				if (strstr($rule['destination']['network'], "opt")) {
					$dst = $optcfg[$rule['destination']['network']]['sa'] . "/" .
						$optcfg[$rule['destination']['network']]['sn'];
				} else {
					switch ($rule['destination']['network']) {
						case 'wanip':
							$dst = $curwanip;
							break;
						case 'lan':
							$dst = "$lansa/$lansn";
							break;
						case 'pptp':
							$dst = "$pptpsa/$pptpsn";
							break;
					}
				}
			} else if ($rule['destination']['address']) {
				$dst = alias_expand($rule['destination']['address']);
			}
			
			if (!$dst || ($dst == "/")) {
				//printf("No destination address found in rule $i\n");
				break;
			}
			
			if (isset($rule['destination']['not'])) {
				$line .= "to !$dst ";
			} else {
				$line .= "to $dst ";
			}
			
			if (in_array($rule['protocol'], array("tcp","udp","tcp/udp"))) {
				
				if ($rule['destination']['port']) {
					$dstport = explode("-", $rule['destination']['port']);
					
					if ((!$dstport[1]) || ($dstport[0] == $dstport[1])) {
						$line .= "port = {$dstport[0]} ";
					} else if (($dstport[0] == 1) && ($dstport[1] == 65535)) {
						/* no need for a port statement here */
					} else if ($dstport[1] == 65535) {
						$line .= "port >= {$dstport[0]} "; 
					} else if ($dstport[0] == 1) {
						$line .= "port <= {$dstport[1]} "; 
					}  else {
						$dstport[0]--;
						$dstport[1]++;
						$line .= "port {$dstport[0]} >< {$dstport[1]} ";
					}
				}
			}
			
			if (($rule['protocol'] == "icmp") && $rule['icmptype']) {
				$line .= "icmp-type {$rule['icmptype']} ";
			}
			
			if ($type == "pass") {
				$line .= "keep state ";
			
				if (isset($rule['frags']))
					$line .= "keep frags ";
			}
			
			if ($type == "reject" && $rule['protocol'] == "tcp") {
				/* special reject packet */
				$line .= "flags S/SA ";
			}
				
			if (!$ispptp) {
				$line .= "group $groupnum ";
			}
			
			$line .= "\n";
			
			$ipfrules .= $line;
		}
		
		$i++;
	}
	
	$ipfrules .= <<<EOD
	
#---------------------------------------------------------------------------
# default rules (just to be sure)
#---------------------------------------------------------------------------
block in $log quick all
block out $log quick all

EOD;
	
	return $ipfrules;
}

function filter_rules_spoofcheck_generate($ifname, $if, $sa, $sn, $log) {

	global $g, $config;
	
	$ipfrules = "";

	if (is_array($config['staticroutes']['route']) && count($config['staticroutes']['route'])) {
		/* count rules */
		$n = 1;
		foreach ($config['staticroutes']['route'] as $route) {
			if ($route['interface'] == $ifname)
				$n++;
		}
		
		/* output skip rules */
		foreach ($config['staticroutes']['route'] as $route) {
			if ($route['interface'] == $ifname) {
				$ipfrules .= "skip $n in on $if from {$route['network']} to any\n";
				$n--;
			}
		}
		$ipfrules .= "skip 1 in on $if from $sa/$sn to any\n";
		$ipfrules .= "block in $log quick on $if all\n";
	} else {
		$ipfrules .= "block in $log quick on $if from ! $sa/$sn to any\n";
	}
	
	return $ipfrules;
}

function filter_rules_ipsec_generate($ifname, $ip) {

	$ipfrules = <<<EOD

# Pass IKE packets
pass in quick on {$ifname} proto udp from any to {$ip} port = 500
pass out quick on {$ifname} proto udp from {$ip} port = 500 to any

# Pass NAT-T encapsulated ESP packets
pass in quick on {$ifname} proto udp from any to {$ip} port = 4500
pass out quick on {$ifname} proto udp from {$ip} port = 4500 to any

# Pass ESP packets
pass in quick on {$ifname} proto esp from any to {$ip}
pass out quick on {$ifname} proto esp from {$ip} to any

# Pass AH packets
pass in quick on {$ifname} proto ah from any to {$ip}
pass out quick on {$ifname} proto ah from {$ip} to any

EOD;
	
	return $ipfrules;
}

?>
