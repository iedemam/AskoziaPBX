#!/usr/local/bin/php -f
<?php
/*
    m0n0dev.php (http://www.askozia.com/m0n0wall/m0n0dev.php)
    
    Copyright (C) 2007 IKT <http://www.itison-ikt.de> 
    All rights reserved.
    
    Authors:
        Michael Iedema <michael.iedema@askozia.com>.
    
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

/*	Notes to self:

	- sanity checks aren't performed in a regular way (some pre, some post call)
	- patching is going to get NASTY if states aren't recorded properly
	- mfsroot and .img sizes can be calculated automatically but they need a 
	  pad of "x" bytes.  Since I can't set this accurately, I'll stick to static
	  values for the time being
	- logging system is a bit overused, the commands say what's going on	
	
	Changelog:
		2007-02-28 - v0.1.0 - first public release
		2007-03-02 - v0.1.1	- added "generic-pc-smp" platform support
*/



// Please set me to the path you checked out the m0n0wall FreeBSD 6 branch to.
$dirs['mwroot'] = "/usr/m0n06branch";	// no trailing slash please!

// --[ package versions ]------------------------------------------------------

$php_version = "php-4.4.5";
$radius_version = "radius-1.2.5";
$mini_httpd_version = "mini_httpd-1.19";
$ez_ipupdate_version = "ez-ipupdate-3.0.11b8";
$mpd_version = "mpd-3.18";
$ipsec_tools_version = "ipsec-tools-0.6.6";


// --[ image sizes ]-----------------------------------------------------------

$mfsroot_size = 13312;
$generic_pc_size = 10240;
$generic_pc_smp_size = 11264;
$wrap_soekris_size = 7808;


// --[ possible platforms and kernels ]----------------------------------------

$platform_list = "net45xx net48xx wrap generic-pc generic-pc-cdrom generic-pc-smp";
$platforms = explode(" ", $platform_list);


// --[ sanity checks and env info ]--------------------------------------------

$dirs['pwd'] = rtrim(shell_exec("pwd"), "\n");
$dirs['boot'] = $dirs['mwroot'] . "/build/boot";
$dirs['kernelconfigs'] = $dirs['mwroot'] . "/build/kernelconfigs";
$dirs['minibsd'] = $dirs['mwroot'] . "/build/minibsd";
$dirs['patches'] = $dirs['mwroot'] . "/build/patches";
$dirs['tools'] = $dirs['mwroot'] . "/build/tools";
$dirs['etc'] = $dirs['mwroot'] . "/etc";
$dirs['phpconf'] = $dirs['mwroot'] . "/phpconf";
$dirs['webgui'] = $dirs['mwroot'] . "/webgui";
$dirs['files'] = $dirs['pwd'] . "/files";

// check to make sure that the directories we expect are there
foreach($dirs as $expected_dir) {
	if(!file_exists($expected_dir)) {
		_log("FATAL: missing directory ($expected_dir)\n".
			"Did you set the \"mwroot\" at the top of this script to the correct path?");
		exit();
	}
}

// create the work directory
if(!file_exists("work")) {
	mkdir("work");
}

// ...and subdirectories
$dirs['packages'] = $dirs['pwd']."/work/packages";
$dirs['images'] = $dirs['pwd']."/work/images";
$dirs['mfsroots'] = $dirs['pwd']."/work/mfsroots";

foreach($dirs as $dir) {
	if(!file_exists($dir)) {
		mkdir($dir);
	}	
}


$error_codes = array(
	/* 0 */ "",
	/* 1 */ "not enough arguments!",
	/* 2 */ "invalid argument!",
	/* 3 */ "invalid platform specified!",
	/* 4 */ "invalid kernel specified!",
	/* 5 */ "invalid image specified!",
	/* 6 */ "image already exists!"
);


// --[ phone home ]------------------------------------------------------------

$h["update"] = "checks for m0n0dev updates";
function _update() {
	$s = file_get_contents("http://www.askozia.com/vcheck.php?p=m0n0dev&cv=0.1.1");
	print("$s\n");
}


// --[ the functions! ]--------------------------------------------------------

$h["patch kernel"] = "patches the kernel sources with any changes needed by m0n0wall";
function patch_kernel() {
	global $dirs;
	
	_exec("cd /usr/src; patch -p0 < ". $dirs['patches'] ."/kernel/kernel-6.patch");
	_log("patched kernel");
	
	_exec("cd /usr/src/sys; patch -p0 < ". $dirs['patches'] ."/kernel/racoon-nattraversal-freebsd6.patch");
	_log("patched racoon nat-traversal");
}


$h["patch syslog"] = "patches circular logging support into syslog";
function patch_syslogd() {
	global $dirs;

	_exec("cd /usr/src; patch < ". $dirs['patches'] ."/user/syslogd.c.patch");
	_exec("cd /usr/src/usr.sbin; tar xfvz ". $dirs['patches'] ."/user/clog-1.0.1.tar.gz");

	_log("patched syslogd");
}


$h["patch bootloader"] = "patches the bootloader to fix some problems with terminal output";
function patch_bootloader() {
	global $dirs;
	
	_exec("cd /sys/boot/i386/libi386; patch < ". $dirs['files'] ."/libi386.patch");
	
	_log("patched bootloader");
}


$h["patch everything"] = "patches the bootloader, kernel and syslogd";
function patch_everything() {
	
	patch_kernel();
	patch_syslogd();
	patch_bootloader();
}


$h["build kernel"] = "(re)builds and compresses the specified platform's kernel ($platform_list)"; 
function build_kernel($platform) {
	global $dirs;
	
	$kernel = _platform_to_kernel($platform);

	_exec("cp ". $dirs['kernelconfigs'] ."/M0N0WALL_* /sys/i386/conf/");
	_exec("cp ". $dirs['files'] ."/M0N0WALL_GENERIC_SMP* /sys/i386/conf/");
	
	if(file_exists("/sys/i386/compile/$kernel")) {
		_exec("rm -rf /sys/i386/compile/$kernel");
	}
	_exec("cd /sys/i386/conf/; config $kernel");
	_exec("cd /sys/i386/compile/$kernel; make cleandepend; make depend; make");
	_exec("gzip -9 /sys/i386/compile/$kernel/kernel");

	_log("built kernel for $platform");
}


$h["build kernels"] = "(re)builds and compresses all kernels";
function build_kernels() {
	global $platforms;
	
	foreach($platforms as $platform) {
		if($platform == "generic-pc-cdrom") {
			continue;
		}
		build_kernel($platform);
	}
}


$h["build syslogd"] = "(re)builds syslogd";
function build_syslogd() {

	_exec("cd /usr/src/usr.sbin/syslogd; make clean; make");
	
	_log("built syslogd");	
}


$h["build clog"] = "(re)builds the circular logging binary which optimizes logging on devices with limited memory";
function build_clog() {

	_exec("cd /usr/src/usr.sbin/clog; make clean; make obj; make");	
	
	_log("built clog");
}


$h["build php"] = "(re)builds php and radius extension, also installs and configures autoconf if not already present";
function build_php() {
	global $dirs, $php_version, $radius_version;

	if(!file_exists("/usr/local/bin/autoconf")) {
		_exec("cd /usr/ports/devel/autoconf213; make install clean");
		_exec("ln -s /usr/local/bin/autoconf213 /usr/local/bin/autoconf");
		_exec("ln -s /usr/local/bin/autoheader213 /usr/local/bin/autoheader");
		_log("installed autoconf");
	}
	
	if(!file_exists($dirs['packages'] ."/$php_version")) {
		_exec("cd ". $dirs['packages'] ."; ".
				"fetch http://br.php.net/distributions/$php_version.tar.gz;" .
				"tar zxf $php_version.tar.gz");
		_log("fetched and untarred $php_version");
		
		_exec("cd ". $dirs['packages'] ."/$php_version/ext; ".
				"fetch http://m0n0.ch/wall/downloads/freebsd-4.11/$radius_version.tgz; ".
				"tar zxf $radius_version.tgz; ".
				"mv $radius_version radius");
		_log("fetched and untarred $radius_version");
	}
	_exec("cd ". $dirs['packages'] ."/$php_version; ".
			"rm configure; ".
			"./buildconf --force; ".
			"./configure --without-mysql --with-pear --with-openssl --enable-discard-path --enable-radius --enable-sockets --enable-bcmath; ".
			"make");
	
	_log("built php");
}


$h["build minihttpd"] = "(re)builds and patches mini_httpd";
function build_minihttpd() {
	global $dirs, $mini_httpd_version;
	
	if(!file_exists($dirs['packages'] ."/$mini_httpd_version")) {
		_exec("cd ". $dirs['packages'] ."; ".
				"fetch http://www.acme.com/software/mini_httpd/$mini_httpd_version.tar.gz; ".
				"tar zxf $mini_httpd_version.tar.gz");
		_log("fetched and untarred $mini_httpd_version");
	}
	if(!_is_patched($mini_httpd_version)) {
		_exec("cd ". $dirs['packages'] ."/$mini_httpd_version; patch < ". $dirs['patches'] . "/packages/mini_httpd.patch");
		_stamp_package_as_patched($mini_httpd_version);
	}
	_exec("cd ". $dirs['packages'] ."/$mini_httpd_version; make clean; make");

	_log("built minihttpd");
}


$h["build dhcpserver"] = "(re)builds the ISC DHCP server (NOTE: dialog must be confirmed)";
function build_dhcpserver() {
	
	/* TODO: automate compile */
	_prompt("After the screen appears, press TAB and then ENTER. (it's on the TODO list...)", 5);
	_exec("cd /usr/ports/net/isc-dhcp3-server; make clean; make WITHOUT_DHCP_LDAP_SSL=YES WITHOUT_DHCP_PARANOIA=YES WITHOUT_DHCP_JAIL=YES");

	_log("built dhcp server");
}


$h["build dhcprelay"] = "(re)builds the ISC DHCP relay";
function build_dhcprelay() {
	
	_exec("cd /usr/ports/net/isc-dhcp3-relay; make clean; make");

	_log("built dhcp relay");
}


$h["build dnsmasq"] = "(re)builds Dnsmasq (DNS forwarder for NAT firewalls) (NOTE: dialog must be confirmed)";
function build_dnsmasq() {
	
	/* TODO: automate compile */
	_prompt("After the screen appears, press TAB and then ENTER. (it's on the TODO list...)", 5);
	_exec("cd /usr/ports/dns/dnsmasq; make clean; make");
	
	_log("built dnsmasq");
}


$h["build msntp"] = "(re)builds msntp (NTP client)";
function build_msntp() {
	
	_exec("cd /usr/ports/net/msntp; make clean; make");
	
	_log("built msntp");
}


$h["build ezipupdate"] = "(re)builds and patches ez-ipupdate (dynamic dns update client)";
function build_ezipupdate() {
	global $dirs, $ez_ipupdate_version;
	
	if(!file_exists($dirs['packages'] ."/$ez_ipupdate_version")) {
		_exec("cd ". $dirs['packages'] ."; ".
				"fetch http://dyn.pl/client/UNIX/ez-ipupdate/$ez_ipupdate_version.tar.gz; ".
				"tar zxf $ez_ipupdate_version.tar.gz");
		_log("fetched and untarred $ez_ipupdate_version");
	}
	if(!_is_patched($ez_ipupdate_version)) {
		_exec("cd ". $dirs['packages'] ."/$ez_ipupdate_version; ".
				"patch < ". $dirs['patches'] ."/packages/ez-ipupdate.c.patch");
		_stamp_package_as_patched($ez_ipupdate_version);
	}	
	_exec("cd ". $dirs['packages'] ."/$ez_ipupdate_version; ".
			"./configure; ".
			"make");

	_log("built ez-ipupdate");
}


$h["build racoon"] = "(re)builds and patches the ipsec-tools version of racoon";
function build_racoon() {
	global $dirs, $ipsec_tools_version;
	
	// TODO: mklibs.pl fails on this because the library has already been added by hand
	// hacked to install lib temporarily

	// TODO: ugly ugly ugly...make clean, make, make clean, make install!!
	
	/* TODO: automate compile */
	_prompt("After the screen appears, press TAB and then ENTER. (it's on the TODO list...)", 5);
	_exec("cd /usr/ports/security/ipsec-tools; patch < ". $dirs['files'] ."/ipsec-tools-makefile.patch");
	_exec("cd /usr/ports/security/ipsec-tools; make clean; make");
	_exec("cd /usr/ports/security/ipsec-tools/work/$ipsec_tools_version; ".
			"patch < ". $dirs['patches'] ."/packages/$ipsec_tools_version.patch");
	_exec("cd /usr/ports/security/ipsec-tools/work/$ipsec_tools_version; make clean; make install");
	
	_log("built and patched racoon (albeit hackily)");

}


$h["build mpd"] = "(re)builds and patches MPD (Multi-link PPP daemon)";
function build_mpd() {
	global $dirs, $mpd_version;
	
	// TODO: ugly...still need to better judge the port status
	_exec("cd /usr/ports/net/mpd; make clean; make");
	_exec("cd /usr/ports/net/mpd/work/$mpd_version; patch < ". $dirs['patches'] ."/packages/mpd.patch");
	_exec("cd /usr/ports/net/mpd/work/$mpd_version; make");
	
	_log("built and patched MPD");
}


$h["build tools"] = "(re)builds the little \"helper tools\" that m0n0wall needs (choparp, stats.cgi, minicron, verifysig)";
function build_tools() {
	global $dirs;
	
	_exec("cd ". $dirs['tools'] ."; gcc -o choparp choparp.c");
	_log("built choparp");
	
	_exec("cd ". $dirs['tools'] ."; gcc -o stats.cgi stats.c");
	_log("built stats.cgi");
	
	_exec("cd ". $dirs['tools'] ."; gcc -o minicron minicron.c");
	_log("built minicron");
	
	_exec("cd ". $dirs['tools'] ."; gcc -o verifysig -lcrypto verifysig.c");
	_log("built verifysig");
}


$h["build bootloader"] = "(re)builds the bootloader files";
function build_bootloader() {
	
	_exec("cd /sys/boot; make clean; make obj; make");
	
	_log("compiled boot loader");
}


$h["build packages"] = "(re)builds all necessary packages";
function build_packages() {

	build_php();
	build_minihttpd();
	build_ezipupdate();
}

$h["build ports"] = "(re)builds all necessary ports";
function build_ports() {
	
	build_dnsmasq();
	build_msntp();
	build_dhcpserver();
	build_dhcprelay();
	build_racoon();
	build_mpd();
}

$h["build everything"] = "(re)builds all packages, kernels and the bootloader";
function build_everything() {

	build_packages();
	build_ports();
	build_kernels();
	build_bootloader();
}


$h["create"] = "creates the directory structure for the given \"image_name\"";
function create($image_name) {
	global $dirs;
		
	_exec("mkdir $image_name");
	_exec("cd $image_name; mkdir lib bin cf conf.default dev etc ftmp mnt libexec proc root sbin tmp usr var");
	_exec("cd $image_name; mkdir etc/inc");
	_exec("cd $image_name; ln -s /cf/conf conf");
	_exec("cd $image_name/usr; mkdir bin lib libexec local sbin share");
	_exec("cd $image_name/usr/local; mkdir bin lib sbin www");
	_exec("cd $image_name/usr/local; ln -s /var/run/htpasswd www/.htpasswd");
	
	_log("created directory structure");
}


$h["populate base"] = "populates the base system binaries for the given \"image_name\"";
function populate_base($image_name) {
	global $dirs;

	_exec("perl ". $dirs['minibsd'] ."/mkmini.pl ". $dirs['minibsd'] ."/m0n0wall.files / $image_name");
	
	_log("added base system binaries");
}


$h["populate etc"] = "populates /etc and appropriately links /etc/resolv.conf and /etc/hosts for the given \"image_name\"";
function populate_etc($image_name) {
	global $dirs;
		
	_exec("cp -p ". $dirs['files'] ."/etc/* $image_name/etc/");		// etc stuff not in svn
	_exec("cp -p ". $dirs['etc'] ."/rc* $image_name/etc/");
	_exec("cp ". $dirs['etc'] ."/pubkey.pem $image_name/etc/");
	_log("added etc");
	
	_exec("ln -s /var/etc/resolv.conf $image_name/etc/resolv.conf");
	_exec("ln -s /var/etc/hosts $image_name/etc/hosts");
	_log("added resolv.conf and hosts symlinks");
	
}


$h["populate defaultconf"] = "adds the default xml configuration file to the given \"image_name\"";
function populate_defaultconf($image_name) {
	global $dirs;
	
	_exec("cp ". $dirs['phpconf'] ."/config.xml $image_name/conf.default/");
	
	_log("added default config.xml");
}


$h["populate zoneinfo"] = "adds timezone info to the given \"image_name\"";
function populate_zoneinfo($image_name) {
	global $dirs;
	
	_exec("cp ". $dirs['files'] ."/zoneinfo.tgz $image_name/usr/share/");
	
	_log("added zoneinfo.tgz");
}


$h["populate syslogd"] = "adds syslogd to the given \"image_name\"";
function populate_syslogd($image_name) {
	global $dirs;
	
	_exec("cd /usr/src/usr.sbin/syslogd; ".
			"install -s /usr/obj/usr/src/usr.sbin/syslogd/syslogd $image_name/usr/sbin");

	_log("added syslogd");
}


$h["populate clog"] = "adds circular logging to the given \"image_name\"";
function populate_clog($image_name) {
	global $dirs;
	
	_exec("cd /usr/src/usr.sbin/clog; ".
			"install -s /usr/obj/usr/src/usr.sbin/clog/clog $image_name/usr/sbin");

	_log("added clog");
}


$h["populate php"] = "adds the php interpreter to the given \"image_name\"";
function populate_php($image_name) {
	global $dirs, $php_version;
	
	_exec("cd ". $dirs['packages'] ."/$php_version/; install -s sapi/cgi/php $image_name/usr/local/bin");
	_exec("cp ". $dirs['files'] ."/php.ini $image_name/usr/local/lib/");

	_log("added php");
}


$h["populate minihttpd"] = "adds the mini-httpd server to the given \"image_name\"";
function populate_minihttpd($image_name) {
	global $dirs, $mini_httpd_version;
	
	_exec("cd ". $dirs['packages'] ."/$mini_httpd_version; ".
			"install -s mini_httpd $image_name/usr/local/sbin");
	
	_log("added mini_httpd");
}


$h["populate dhclient"] = "adds the ISC DHCP client to the given \"image_name\"";
function populate_dhclient($image_name) {
	global $dirs;
	
	_exec("cp /sbin/dhclient $image_name/sbin/");
	_exec("cp ". $dirs['tools'] ."/dhclient-script $image_name/sbin/");
	
	_log("added dhclient");
}


$h["populate dhcpserver"] = "adds the ISC DHCP server to the given \"image_name\"";
function populate_dhcpserver($image_name) {
	global $dirs;
	
	_exec("cd /usr/ports/net/isc-dhcp3-server; ".
		"install -s work/dhcp-*/work.freebsd/server/dhcpd $image_name/usr/local/sbin");
	
	_log("added dhcp server");
}


$h["populate dhcprelay"] = "adds the ISC DHCP relay to the given \"image_name\"";
function populate_dhcprelay($image_name) {
	global $dirs;
	
	_exec("cd /usr/ports/net/isc-dhcp3-relay; ".
		"install -s work/dhcp-*/work.freebsd/relay/dhcrelay $image_name/usr/local/sbin");
	
	_log("added dhcp relay");
}


$h["populate dnsmasq"] = "adds Dnsmasq (DNS forwarder) to the given \"image_name\"";
function populate_dnsmasq($image_name) {
	global $dirs;
	
	_exec("cd /usr/ports/dns/dnsmasq; ".
		"install -s work/dnsmasq-*/src/dnsmasq $image_name/usr/local/sbin");
	
	_log("added dnsmasq");
}


$h["populate msntp"] = "adds msntp (NTP client) to the given \"image_name\"";
function populate_msntp($image_name) {
	global $dirs;
	
	_exec("cd /usr/ports/net/msntp; ".
		"install -s work/msntp-*/msntp $image_name/usr/local/bin");
	
	_log("added msntp");
}


$h["populate ezipupdate"] = "adds ez-ipupdate (dynamic dns client) to the given \"image_name\"";
function populate_ezipupdate($image_name) {
	global $dirs, $ez_ipupdate_version;
	
	_exec("cd ". $dirs['packages'] ."/$ez_ipupdate_version; ".
		"install -s ez-ipupdate $image_name/usr/local/bin");
	
	_log("added ez-ipupdate");
}


$h["populate mpd"] = "adds MPD (Multi-link PPP daemon) to the given \"image_name\"";
function populate_mpd($image_name) {
	global $dirs;
	
	_exec("cd /usr/ports/net/mpd; install -s work/mpd-*/src/mpd $image_name/usr/local/sbin");
	
	_log("added mpd");
}


$h["populate racoon"] = "adds racoon to the given \"image_name\"";
function populate_racoon($image_name) {
	global $dirs, $ipsec_tools_version;
	
	_exec("cd /usr/ports/security/ipsec-tools; ".
		"install -s work/$ipsec_tools_version/src/racoon/.libs/racoon $image_name/usr/local/sbin; ".
		"install -s work/$ipsec_tools_version/src/libipsec/.libs/libipsec.so.0 $image_name/usr/local/lib");
	_exec("cp /usr/ports/security/ipsec-tools/work/$ipsec_tools_version/src/setkey/setkey $image_name/usr/local/sbin/");
	
	_log("added racoon");
}


$h["populate tools"] = "adds the m0n0wall \"helper tools\" to the given \"image_name\"";
function populate_tools($image_name) {
	global $dirs;
	
	_exec("cd ". $dirs['tools'] ."; ".
		"install -s choparp $image_name/usr/local/sbin; ".
		"install -s stats.cgi $image_name/usr/local/www; ".
		"install -s minicron $image_name/usr/local/bin; ".
		"install -s verifysig $image_name/usr/local/bin; ".
		"install runmsntp.sh $image_name/usr/local/bin; ".
		"install ppp-linkup vpn-linkdown vpn-linkup $image_name/usr/local/sbin");
}


$h["populate phpconf"] = "adds the php configuration system to the given \"image_name\"";
function populate_phpconf($image_name) {
	global $dirs;

	_exec("cp ". $dirs['phpconf'] ."/rc* $image_name/etc/");
	_exec("cp ". $dirs['phpconf'] ."/inc/* $image_name/etc/inc/");
	
	_log("added php conf scripts");
}


$h["populate webgui"] = "adds the php webgui files to the given \"image_name\"";
function populate_webgui($image_name) {
	global $dirs;
	
	_exec("cp ". $dirs['webgui'] ."/* $image_name/usr/local/www/");
	
	_log("added webgui");
}


$h["populate libs"] = "adds the required libraries (using mklibs.pl) to the given \"image_name\"";
function populate_libs($image_name) {
	global $dirs;
	
	_exec("perl ". $dirs['minibsd'] ."/mklibs.pl $image_name > tmp.libs");
	_exec("perl ". $dirs['minibsd'] ."/mkmini.pl tmp.libs / $image_name");
	_exec("rm tmp.libs");
	
	_log("added libraries");	
}


$h["populate everything"] = "adds all packages, scripts and config files to the given \"image_name\"";
function populate_everything($image_name) {

	$funcs = get_defined_functions();
	$funcs = $funcs['user'];

	foreach($funcs as $func) {
		if($func[0] == '_') {	// ignore internal functions
			continue;
		}
		$func = explode("_", $func);
 		if($func[0] == "populate" && $func[1] != "everything") {
			$f = "populate_" . $func[1];
			$f($image_name);
		}
	}

}

// TODO: this is quite large and ugly
$h["package"] = "package the specified image directory into an .img for the specified platform and stamp as version (i.e. package generic-pc 1.3b2copy testimage)";
function package($platform, $version, $image_name) {
	global $dirs, $mfsroot_size, $generic_pc_size, $generic_pc_smp_size, $wrap_soekris_size;
	
	_log("packaging $image_name $version for $platform...");
	
	_set_permissions($image_name);
	
	if(!file_exists("tmp")) {
		mkdir("tmp");
		mkdir("tmp/mnt");
	}
	
	$kernel = _platform_to_kernel($platform);
	
	// mfsroots
	if(!file_exists($dirs['mfsroots'] ."/$platform-$version-". basename($image_name) .".gz")) {
				
		_exec("dd if=/dev/zero of=tmp/mfsroot bs=1k count=$mfsroot_size");
		_exec("mdconfig -a -t vnode -f tmp/mfsroot -u 0");
	
		_exec("bsdlabel -rw md0 auto");
		_exec("newfs -O 1 -b 8192 -f 1024 -o space -m 0 /dev/md0c");
	
		_exec("mount /dev/md0c tmp/mnt");
		_exec("cd tmp/mnt; tar -cf - -C $image_name ./ | tar -xpf -");
		
		if($platform == "generic-pc-smp") {
			_exec("cd tmp/mnt/usr/local/www/; patch < ". $dirs['files'] ."/generic-pc-smp.patch");
		}
		
		// modules		
		mkdir("tmp/mnt/boot");
		mkdir("tmp/mnt/boot/kernel");
		if($platform == "generic-pc" || 
			$platform == "generic-pc-cdrom" ||
			$platform == "generic-pc-smp") {
			_exec("cp /sys/i386/compile/$kernel/modules/usr/src/sys/modules/acpi/acpi/acpi.ko tmp/mnt/boot/kernel/");
		}
		_exec("cp /sys/i386/compile/$kernel/modules/usr/src/sys/modules/dummynet/dummynet.ko tmp/mnt/boot/kernel/");
		_exec("cp /sys/i386/compile/$kernel/modules/usr/src/sys/modules/ipfw/ipfw.ko tmp/mnt/boot/kernel/");
		
		_exec("echo \"$version\" > tmp/mnt/etc/version");
		_exec("echo `date` > tmp/mnt/etc/version.buildtime");
		_exec("echo $platform > tmp/mnt/etc/platform");
	
		_exec("umount tmp/mnt");
		_exec("mdconfig -d -u 0");
		_exec("gzip -9 tmp/mfsroot");
		_exec("mv tmp/mfsroot.gz ". $dirs['mfsroots'] ."/$platform-$version-". basename($image_name) .".gz");
	}
	
	// .img
	if($platform != "generic-pc-cdrom" && !file_exists($dirs['images'] ."/$platform-$version-". basename($image_name) .".img")) {
		
		if($platform == "generic-pc") {
			_exec("dd if=/dev/zero of=tmp/image.bin bs=1k count=$generic_pc_size");
		} else if($platform == "generic-pc-smp") {
			_exec("dd if=/dev/zero of=tmp/image.bin bs=1k count=$generic_pc_smp_size");
		} else {
			_exec("dd if=/dev/zero of=tmp/image.bin bs=1k count=$wrap_soekris_size");
		}
			
		_exec("mdconfig -a -t vnode -f tmp/image.bin -u 0");
		_exec("bsdlabel -Brw -b /usr/obj/usr/src/sys/boot/i386/boot2/boot md0 auto");
		_exec("newfs -O 1 -b 8192 -f 1024 -o space -m 0 /dev/md0a");
		_exec("mount /dev/md0a tmp/mnt");
		_exec("cp ". $dirs['mfsroots'] ."/$platform-$version-". basename($image_name) .".gz tmp/mnt/mfsroot.gz");
		
		// boot
		mkdir("tmp/mnt/boot");
		mkdir("tmp/mnt/boot/kernel");
	    _exec("cp /usr/obj/usr/src/sys/boot/i386/loader/loader tmp/mnt/boot/");
		$platform == "generic-pc-smp" ?
		_exec("cp ". $dirs['boot'] ."/generic-pc/loader.rc tmp/mnt/boot/") :
		_exec("cp ". $dirs['boot'] ."/$platform/loader.rc tmp/mnt/boot/");
	
		// conf
		mkdir("tmp/mnt/conf");
		_exec("cp ". $dirs['phpconf'] ."/config.xml tmp/mnt/conf");
		_exec("cp /sys/i386/compile/$kernel/kernel.gz tmp/mnt/kernel.gz");		
		_exec("umount tmp/mnt");
		_exec("mdconfig -d -u 0");
		_exec("gzip -9 tmp/image.bin");
		_exec("mv tmp/image.bin.gz ". $dirs['images'] ."/$platform-$version-". basename($image_name) .".img");
		
	// .iso
	} else if($platform == "generic-pc-cdrom" && !file_exists($dirs['images'] ."/$platform-$version-". basename($image_name) .".iso")) {

		_exec("mkdir tmp/cdroot");
		_exec("cp ". $dirs['mfsroots'] ."/$platform-$version-". basename($image_name) .".gz tmp/cdroot/mfsroot.gz");
		_exec("cp /sys/i386/compile/$kernel/kernel.gz tmp/cdroot/kernel.gz");		

		_exec("mkdir tmp/cdroot/boot");
	    _exec("cp /usr/obj/usr/src/sys/boot/i386/cdboot/cdboot tmp/cdroot/boot/");		
	    _exec("cp /usr/obj/usr/src/sys/boot/i386/loader/loader tmp/cdroot/boot/");
		_exec("cp ". $dirs['boot'] ."/$platform/loader.rc tmp/cdroot/boot/");
		_exec("cp /usr/obj/usr/src/sys/boot/i386/boot2/boot tmp/cdroot/boot/");

		_exec("mkisofs -b \"boot/cdboot\" -no-emul-boot -A \"m0n0wall CD-ROM image\" ".
			"-c \"boot/boot.catalog\" -d -r -publisher \"m0n0.ch\" ".
			"-p \"Your Name\" -V \"m0n0wall_cd\" -o \"m0n0wall.iso\" tmp/cdroot/");
			
		_exec("mv m0n0wall.iso ". $dirs['images'] ."/cdrom-$version-". basename($image_name) .".iso");
	}
	
	_exec("rm -rf tmp");
}


function _set_permissions($image_name) {
	
	_exec("chmod 755 $image_name/etc/rc*");
	_exec("chmod 644 $image_name/etc/pubkey.pem");
	
	_exec("chmod 644 $image_name/etc/inc/*");
	
	_exec("chmod 644 $image_name/conf.default/config.xml");

	_exec("chmod 644 $image_name/usr/local/www/*");
	_exec("chmod 755 $image_name/usr/local/www/*.php");
	_exec("chmod 755 $image_name/usr/local/www/*.cgi");
	
	_log("permissions set.");
}



function _stamp_package_as_patched($package_version) {
	global $dirs;
	
	touch($dirs['packages'] ."/$package_version/$package_version.patched");
	
	_log("patched $package_version");
}

function _is_patched($package_version) {
	global $dirs;
	
	return(file_exists($dirs['packages'] ."/$package_version/$package_version.patched"));
}


function _platform_to_kernel($platform) {
	global $platforms;
	
	if(array_search($platform, $platforms) === false) {
		_usage(3);
	}
	
	if($platform == "generic-pc-cdrom" || $platform == "generic-pc") {
		$kernel = "M0N0WALL_GENERIC";
	} else if($platform == "generic-pc-smp") {
		$kernel = "M0N0WALL_GENERIC_SMP";
	} else {
		$kernel = "M0N0WALL_" . strtoupper($platform);
	}

	return $kernel;
}


function _exec($cmd) {
	$ret = 0;
	passthru($cmd, $ret);
	if($ret != 0) {
		_log("COMMAND FAILED: $cmd");
		exit();
	}
}


function _log($msg) {
	print "$msg\n";
}


function _prompt($msg, $duration=0) {
	
	$msg = wordwrap(" - $msg", 74, "\n - ");
	if($duration) {
		print "--[ Attention! ]-------------------------------------------------------------\n\n";
	}	
	print "$msg\n\n";
	if($duration) {
		print "--[ T-MINUS ";
		print "$duration";
		$i = $duration-1;
		for($i; $i>0; $i--) {
			sleep(1);
			print ", $i";
		}
		sleep(1);
		print "\n\n";
	}
}

// TODO: this needs help
// - maybe split into error and usage messages
// - this is could be generated
function _usage($err=0) {
	global $error_codes;
	
	print "./m0n0dev.php patch everything\n";
	print "./m0n0dev.php patch bootloader\n";
	print "./m0n0dev.php patch kernel\n";
	print "./m0n0dev.php patch syslogd\n";
	print "./m0n0dev.php build everything\n";
	print "./m0n0dev.php build ports\n";
	print "./m0n0dev.php build packages\n";
	print "./m0n0dev.php build kernels\n";
	print "./m0n0dev.php build kernel kernel_name\n";
	print "./m0n0dev.php build bootloader\n";
	print "./m0n0dev.php build tools\n";
	print "./m0n0dev.php populate everything iamge_name\n";
	print "./m0n0dev.php package platform_name version_string image_name\n";
	print "./m0n0dev.php package all version_string image_name\n";
	print "./m0n0dev.php update\n";
	
	print "Help is available by prefixing the command with \"help\" (i.e. help create)\n";
	
	if($err != 0) {
		print "\n";
		_log($error_codes[$err]);
	}
	
	exit($err);
}


// TODO: I could generate these...
$h["patch"] = "available patch options: bootloader, kernel, syslogd, everything";
$h["build"] = "available build options: kernel, kernels, syslogd, clog, php, minihttpd, ".
	"dhcpserver, dhcprelay, dnsmasq, msntp, ezipupdate, racoon, mpd, ".
	"tools, bootloader, everything";
$h["populate"] = "available populate options: base, etc, defaultconf, zoneinfo, syslogd, ".
	"clog, php, minihttpd, dhclient, dhcpserver, dhcprelay, dnsmasq, msntp, ".
	"ezipupdate, mpd, racoon, tools, phpconf, webgui, libs, everything";


// --[ command line parsing ]--------------------------------------------------

// nothing to do, here's what's possible
if($argc == 1) {
	_usage();

// phone home and check version
} else if($argv[1] == "update") {
	_update();
	exit();

// here's some help if it's available
} else if($argv[1] == "help") {
	// not enough arguments
	if($argc == 2) {
		_usage();
	}
	// form a command name and see if it's in the help array
	$c = implode(" ", array_slice($argv, 2));
	array_key_exists($c, $h) ? 
		_prompt($h[$c]) : 
		print "no help available on ($c)! :(\n";

// create a new image directory
} else if($argv[1] == "create") {
	// construct an absolute path to the image
	$image_name = $dirs['images']. "/" . rtrim($argv[2], "/");
	// create the directory structure if this image's name isn't taken
	file_exists($image_name) ?
		_usage(6) :
		create($image_name);

// patch functions are all defined with no arguments
} else if($argv[1] == "patch") {
	$f = implode("_", array_slice($argv, 1));
	function_exists($f) ?
		$f() :
		_usage(2);		

// build functions are all defined with no arguments except for "build_kernel"
} else if($argv[1] == "build") {
	if($argv[2] == "kernel") {
		build_kernel($argv[3]);
	} else {
		$f = implode("_", array_slice($argv, 1));
		function_exists($f) ?
			$f() :
			_usage(2);
	}

// populate functions are all defined with a single argument:
// (image_name directory)
} else if($argv[1] == "populate") {
	// make a function name out of the arguments
	$f = implode("_", array_slice($argv, 1, 2));
	// not a valid function, show usage
	if(!function_exists($f)) {
		_usage(2);
	}
	// construct an absolute path to the image
	$image_name = $dirs['images']. "/" . rtrim($argv[3], "/");
	// not a valid image, show usage
	if(!file_exists($image_name)) {
		_usage(5);
	}
	$f($image_name);


// the package function is defined with three arguments:
// (platform, version, image_name)
} else if($argv[1] == "package") {
	// construct an absolute path to the image
	$image_name = $dirs['images']. "/" . rtrim($argv[4], "/");
	// not a valid image, show usage
	if(!file_exists($image_name)) {
		_usage(5);
	}
	// we're packaging all platforms go right ahead
	if($argv[2] == "all") {
		foreach($platforms as $platform) {
			package($platform, $argv[3], $image_name);			
		}
	// check the specific platform before attempting to package
	} else if(in_array($argv[2], $platforms)) {
		package($argv[2], $argv[3], $image_name);
	// not a valid package command...
	} else {
		_usage(3);
	}

// hmmm, don't have any verbs like that!
} else {
	_usage(2);
}

exit();

?>
