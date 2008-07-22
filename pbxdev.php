#!/usr/local/bin/php -f
<?php
/*
    $Id$
    
    Copyright (C) 2007 IKT <http://www.itison-ikt.de> 
    All rights reserved.
    
    Authors:
        Michael Iedema <michael@askozia.com>.
    
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

// --[ package versions ]------------------------------------------------------

$versions = array(
	"asterisk"		=> "asterisk-1.4.21.1",
	"ezipupdate"	=> "ez-ipupdate-3.0.11b8",
	"i4b"			=> "i4b-trunk",
	"jquery"		=> "jquery-1.2.1",
	"mini_httpd"	=> "mini_httpd-1.19",
	"msmtp"			=> "msmtp-1.4.11",
	"msntp"			=> "msntp-1.6",
	"pecl_sqlite"	=> "SQLite-1.0.3",
	"php"			=> "php-4.4.8",
	"scriptaculous"	=> "scriptaculous-js-1.7.1_beta3",
	"udesc_dump"	=> "udesc_dump-1.3.9",
	"zaptel"		=> "zaptel-trunk"
);

// --[ sounds ]----------------------------------------------------------------

$core_sounds_version= "1.4.8";
$extra_sounds_version="1.4.7";
$sound_languages	= explode(" ", "en en-gb da de it es fr fr-ca jp nl se ru");
$sounds				= explode(" ", 
						"agent-pass. ".
						"auth-thankyou. auth-incorrect. ".
						"conf-onlyperson. conf-getpin. conf-invalidpin. conf-kicked. ".
						"pbx-transfer. pbx-invalid. pbx-invalidpark. ".
						"vm-intro. vm-theperson. vm-isunavail. vm-isonphone. vm-goodbye. vm-password.");
$wake_me_sounds		= explode(" ",
						"this-is-yr-wakeup-call. to-snooze-for. to-cancel-wakeup. ".
						"for. press-0. press-1. wakeup-call-cancelled. to-rqst-wakeup-call. ".
						"enter-a-time. im-sorry. 1-for-am-2-for-pm. rqsted-wakeup-for. ".
						"1-yes-2-no. thank-you-for-calling. goodbye.");
$digits				= explode(" ", "0 1 2 3 4 5 6 7 8 9");
$musiconhold		= explode(" ", "fpm-calm-river");

// --[ modules ]---------------------------------------------------------------

$low_power_modules		= explode(" ", "codec_speex.so");//" codec_ilbc.so");
$low_power_libraries	= explode(" ", "/usr/local/lib/libspeex.so.1");

// --[ image padding ]---------------------------------------------------------

$mfsroot_pad	= 2560;
$asterisk_pad	= 512;
$image_pad		= 768;

// --[ possible platforms and kernels ]----------------------------------------

$platform_list = "generic-pc net48xx net55xx wrap alix1x alix23x hl4xx generic-pc-cdrom";
$platforms = explode(" ", $platform_list);


// --[ sanity checks and env info ]--------------------------------------------

$dirs['pwd']				= rtrim(shell_exec("pwd"), "\n");
$dirs['boot']				= $dirs['pwd'] . "/build/boot";
$dirs['kernelconfigs']		= $dirs['pwd'] . "/build/kernelconfigs";
$dirs['minibsd']			= $dirs['pwd'] . "/build/minibsd";
$dirs['patches']			= $dirs['pwd'] . "/build/patches";
$dirs['asterisk_modules']	= $dirs['pwd'] . "/build/asterisk_modules";
$dirs['tools']				= $dirs['pwd'] . "/build/tools";
$dirs['cgi']				= $dirs['pwd'] . "/build/cgi";
$dirs['etc']				= $dirs['pwd'] . "/etc";
$dirs['phpconf']			= $dirs['pwd'] . "/phpconf";
$dirs['webgui']				= $dirs['pwd'] . "/webgui";
$dirs['files']				= $dirs['pwd'] . "/files";

foreach ($dirs as $dir) {
	if (!file_exists($dir)) {
		_log("\"$dir\" directory does not exist!");
		exit(1);
	}
}

// create the work directory
if (!file_exists("work")) {
	mkdir("work");
}

// ...and subdirectories
$dirs['packages']	= $dirs['pwd'] . "/work/packages";
$dirs['images']		= $dirs['pwd'] . "/work/images";
$dirs['sounds']		= $dirs['pwd'] . "/work/sounds";
$dirs['mfsroots']	= $dirs['pwd'] . "/work/mfsroots";

foreach($dirs as $dir) {
	if (!file_exists($dir)) {
		mkdir($dir);
	}	
}

// --[ the functions ]---------------------------------------------------------

function prepare_environment() {
	global $dirs;
	
	_exec("cd {$dirs['tools']}; gcc -o sign -lcrypto sign.c");
	_exec("cd /usr/ports/audio/speex; make install");
	_exec("cd /usr/ports/net/ilbc; make install");
	_exec("cd /usr/ports/devel/newt; make install");
	_exec("cd /usr/ports/databases/sqlite2; make install");
	_exec("cd /usr/ports/archivers/unrar; make install");
	_exec("cd /usr/ports/audio/sox; make install");
	_exec("cd /usr/ports/sysutils/cdrtools; make install");
}

function patch_kernel() {
	global $dirs;
	
	_exec("cd /usr/src; patch -p0 < {$dirs['patches']}/kernel/kernel-6.patch");
}

function patch_syslogd() {
	global $dirs;

	_exec("cd /usr/src; patch < {$dirs['patches']}/user/syslogd.c.patch");
	_exec("cd /usr/src/usr.sbin; tar xfvz {$dirs['patches']}/user/clog-1.0.1.tar.gz");
}

function patch_bootloader() {
	global $dirs;
	
	_exec("cd /sys/boot/i386/libi386; patch < {$dirs['patches']}/user/libi386.patch");
}

function patch_hostapd() {
	global $dirs;
	
	_exec("cd /usr/src; patch < {$dirs['patches']}/user/hostapd.patch");
}

function patch_everything() {
	
	patch_kernel();
	patch_syslogd();
	patch_bootloader();
	patch_hostapd();
}

function build_kernel($platform) {
	global $dirs;
	
	$kernel = _platform_to_kernel($platform);

	_exec("cp {$dirs['kernelconfigs']}/ASKOZIAPBX_* /sys/i386/conf/");
	
	if (file_exists("/sys/i386/compile/$kernel")) {
		_exec("rm -rf /sys/i386/compile/$kernel");
	}
	_exec("cd /sys/i386/conf/; config $kernel");
	_exec("cd /sys/i386/compile/$kernel; make cleandepend; make depend; make");
	_exec("gzip -9 /sys/i386/compile/$kernel/kernel");
}

function build_srcupdate() {
	
	build_syslogd();
	build_clog();
	build_isdn();
	build_hostapd();
	build_kernels();
}

function build_kernels() {
	global $platforms;
	
	foreach($platforms as $platform) {
		build_kernel($platform);
	}
}

function build_syslogd() {
	_exec("cd /usr/src/usr.sbin/syslogd; make clean; make");
}

function build_clog() {
	_exec("cd /usr/src/usr.sbin/clog; make clean; make obj; make");	
}

function build_php() {
	global $dirs, $versions;

	if (!file_exists("/usr/local/bin/autoconf")) {
		_exec("cd /usr/ports/devel/autoconf213; make install clean");
		_exec("ln -s /usr/local/bin/autoconf213 /usr/local/bin/autoconf");
		_exec("ln -s /usr/local/bin/autoheader213 /usr/local/bin/autoheader");
	}
	if (!file_exists("{$dirs['packages']}/{$versions['php']}.tar.gz")) {
		_exec("cd {$dirs['packages']}; " .
				"fetch http://de.php.net/distributions/{$versions['php']}.tar.gz");
	}
	if (!file_exists("{$dirs['packages']}/{$versions['php']}")) {
		_exec("cd {$dirs['packages']}; tar zxf {$versions['php']}.tar.gz");
	}
	if (!file_exists("{$dirs['packages']}/{$versions['php']}/ext/sqlite")) {
		_exec("cd ". $dirs['packages'] ."/{$versions['php']}/ext; ".
			"fetch http://pecl.php.net/get/{$versions['pecl_sqlite']}.tgz; ".
			"tar zxf {$versions['pecl_sqlite']}.tgz; ".
				"mv {$versions['pecl_sqlite']} sqlite");
	}
	
	_exec("cd {$dirs['packages']}/{$versions['php']}; ".
			"rm configure; ".
			"./buildconf --force; ".
			"./configure --without-mysql --without-pear --with-openssl --with-sqlite --enable-discard-path --enable-sockets --enable-bcmath --with-gettext=/usr/local/bin; ".
			"make");
}

function build_minihttpd() {
	global $dirs, $versions;
	
	if (!file_exists("{$dirs['packages']}/{$versions['mini_httpd']}.tar.gz")) {
		_exec("cd {$dirs['packages']}; ".
			"fetch http://www.acme.com/software/mini_httpd/{$versions['mini_httpd']}.tar.gz");
	}
	if (!file_exists("{$dirs['packages']}/{$versions['mini_httpd']}")) {
		_exec("cd {$dirs['packages']}; tar zxf {$versions['mini_httpd']}.tar.gz");
	}
	if (!_is_patched($versions['mini_httpd'])) {
		_exec("cd {$dirs['packages']}/{$versions['mini_httpd']}; ".
			"patch < {$dirs['patches']}/packages/mini_httpd.patch");
		_stamp_package_as_patched($versions['mini_httpd']);
	}
	_exec("cd {$dirs['packages']}/{$versions['mini_httpd']}; make clean; make");
}

function build_asterisk() {
	global $dirs, $versions;
	
	if (!file_exists("{$dirs['packages']}/{$versions['asterisk']}.tar.gz")) {
		_exec("cd {$dirs['packages']}; ".
			"fetch http://ftp.digium.com/pub/asterisk/releases/{$versions['asterisk']}.tar.gz");
	}
	if (!file_exists("{$dirs['packages']}/{$versions['asterisk']}")) {
		_exec("cd {$dirs['packages']}; tar zxf {$versions['asterisk']}.tar.gz");
	}
	if (!_is_patched($versions['asterisk'])) {
		$patches = array(
			"makefile",
			"cdr_to_syslog",
			//"channel_queue",
			"voicemail_readback_number",
			"chan_sip_default_cid",
			"chan_sip_show_statuses",
			"chan_iax2_show_statuses",
			"chan_local_jitterbuffer"
			//"chan_sip_423"
		);
		foreach ($patches as $patch) {
			_exec("cd {$dirs['packages']}/{$versions['asterisk']}; ".
				"patch < {$dirs['patches']}/packages/asterisk/asterisk_$patch.patch");
			_log("\n\n --- patch: $patch -----------\n\n");
		}
		_stamp_package_as_patched($versions['asterisk']);
	}
	// copy make options
	_exec("cp {$dirs['patches']}/packages/asterisk/menuselect.makeopts /etc/asterisk.makeopts");
	// copy wakeme application
	_exec("cp {$dirs['asterisk_modules']}/app_wakeme.c {$dirs['packages']}/{$versions['asterisk']}/apps");
	// copy cdr modules
	_exec("cp {$dirs['asterisk_modules']}/cdr_*.c {$dirs['packages']}/{$versions['asterisk']}/cdr");
	// reconfigure and make
	_exec("cd {$dirs['packages']}/{$versions['asterisk']}/; ./configure; gmake; gmake install");
}

function build_zaptel() {
	global $dirs, $versions;
	
	if (!file_exists("{$dirs['packages']}/{$versions['zaptel']}")) {
		_exec("cd {$dirs['packages']}; ".
			"svn co --username svn --password svn ".
			"https://svn.pbxpress.com:1443/repos/zaptel-bsd/branches/zaptel-1.4 {$versions['zaptel']}");
	}

	// remove old headers if they're around
	_exec("rm -f /usr/local/include/zaptel.h /usr/local/include/tonezone.h");
	
	// copy over better ztdummy.c
	_exec("cd {$dirs['packages']}/{$versions['zaptel']}/ztdummy; cp ztdummy.c ztdummy_orig.c");
	_exec("cp {$dirs['patches']}/packages/zaptel/ztdummy.c {$dirs['packages']}/{$versions['zaptel']}/ztdummy");
	
	// make stage directory, clear if present
	if (file_exists("{$dirs['packages']}/{$versions['zaptel']}/STAGE")) {
		_exec("rm -rf {$dirs['packages']}/{$versions['zaptel']}/STAGE");
	}
	_exec("mkdir {$dirs['packages']}/{$versions['zaptel']}/STAGE");

	// compile and install standard version
	_exec("cd {$dirs['packages']}/{$versions['zaptel']}; make clean; make; make install");
	_exec("cd {$dirs['packages']}/{$versions['zaptel']}; cp -p ".
			"zaptel/zaptel.ko ".
			"ztdummy/ztdummy.ko ".
			"wcfxo/wcfxo.ko ".
			"wcfxs/wcfxs.ko ".
			"STAGE");
}

function build_isdn() {
	global $dirs, $versions;
	
	if (!file_exists("{$dirs['packages']}/{$versions['i4b']}")) {
		_exec("cd {$dirs['packages']}; ".
			"svn --username anonsvn --password anonsvn co svn://svn.turbocat.net/i4b {$versions['i4b']}");
	}
	if (!_is_patched($versions['i4b'])) {
		_exec("cd {$dirs['packages']}/{$versions['i4b']}; ".
			"patch < {$dirs['patches']}/packages/i4b_alix23_ehci_usb_amd.patch");
		_stamp_package_as_patched($versions['i4b']);
	}
	
	_exec("cd {$dirs['packages']}/{$versions['i4b']}/trunk/i4b/FreeBSD.i4b; make S=../src package; make install");
	_exec("cd {$dirs['packages']}/{$versions['i4b']}/trunk/i4b/src/usr.sbin/i4b; make clean; make all I4B_WITHOUT_CURSES=yes; make install");
	_exec("cd {$dirs['packages']}/{$versions['i4b']}/trunk/i4b/module; make clean; make");

	_exec("cd {$dirs['packages']}/{$versions['i4b']}/trunk/chan_capi/; gmake clean; gmake all");
}

function build_msntp() {
	_exec("cd /usr/ports/net/msntp; make clean; make");
}

function build_udesc_dump() {
	_exec("cd /usr/ports/sysutils/udesc_dump; make clean; make");
}

function build_msmtp() {
	global $dirs, $versions;
	
	if (!file_exists("{$dirs['packages']}/{$versions['msmtp']}.tar")) {
		_exec("cd {$dirs['packages']}; ".
			"fetch http://belnet.dl.sourceforge.net/sourceforge/msmtp/{$versions['msmtp']}.tar.bz2");
	}
	if (!file_exists("{$dirs['packages']}/{$versions['msmtp']}")) {
		_exec("cd {$dirs['packages']}; bunzip2 {$versions['msmtp']}.tar.bz2; tar xf {$versions['msmtp']}.tar");
	}
	
	_exec("cd {$dirs['packages']}/{$versions['msmtp']}; ".
		"./configure --with-ssl=openssl --without-libgsasl --without-libidn --disable-nls; ".
		"make");
}

function build_ezipupdate() {
	global $dirs, $versions;
	
	if (!file_exists($dirs['packages'] ."/{$versions['ezipupdate']}.tar.gz")) {
        _exec("cd ". $dirs['packages'] ."; ".
        "fetch http://dyn.pl/client/UNIX/ez-ipupdate/{$versions['ezipupdate']}.tar.gz");
	}
	
	if (!is_dir($dirs['packages'] ."/{$versions['ezipupdate']}")) {
        _exec("cd ". $dirs['packages'] ."; ".
              "tar zxf {$versions['ezipupdate']}.tar.gz");
	}
	
	if(!_is_patched($versions['ezipupdate'])) {
		_exec("cd ". $dirs['packages'] ."/{$versions['ezipupdate']}; ".
				"patch < ". $dirs['patches'] ."/packages/ez-ipupdate.c.patch");
		_stamp_package_as_patched($versions['ezipupdate']);
	}	
	_exec("cd ". $dirs['packages'] ."/{$versions['ezipupdate']}; ".
			"./configure; ".
			"make");
}

function build_tools() {
	global $dirs;
	
	_exec("cd " . $dirs['tools'] . "; " .
		"gcc -Wall -o verifysig -lcrypto verifysig.c; " .
		"gcc -Wall -o wrapresetbtn wrapresetbtn.c; " .
		"gcc -Wall -o alix23xresetbtn alix23xresetbtn.c");
	/*_exec("cd {$dirs['tools']}; gcc -o zttest zttest.c");*/
	/*_exec("cd ". $dirs['tools'] ."; gcc -o minicron minicron.c");*/
}

function build_cgi() {
	global $dirs;
	
	_exec("cd " . $dirs['cgi'] . "; ".
		"gcc -Wall -o ajax.cgi ajax.c");
}

function build_bootloader() {
	_exec("cd /sys/boot; make clean; make obj; make");
}

function build_hostapd() {
	_exec("cd /usr/src/usr.sbin/wpa/hostapd; make clean; make; make install");	
}

function build_srcpackages() {

	build_php();
	build_msmtp();
	build_minihttpd();
	build_zaptel();
	build_asterisk();
	build_isdn();
	build_ezipupdate();
}

function build_ports() {
	
	build_msntp();
	build_udesc_dump();
}

function build_everything() {

	build_srcpackages();
	build_ports();
	build_syslogd();
	build_clog();
	build_tools();
	build_cgi();
	build_kernels();
	build_bootloader();
	build_hostapd();
}

function create($image_name) {
	global $dirs;

	if (file_exists($image_name)) {
		_log("Image already exists!");
		exit(1);
	}

	_exec("mkdir $image_name");
	$rootfs = "$image_name/rootfs";
	_exec("mkdir $rootfs");
	
	_exec("cd $rootfs; mkdir lib bin cf conf.default dev etc mnt libexec proc root sbin tmp usr var asterisk storage");
	_exec("cd $rootfs/var; mkdir tmp");
	_exec("cd $rootfs; ln -s /cf/conf conf");
	_exec("cd $rootfs/etc/; mkdir inc pkgs");
	_exec("cd $rootfs/usr; mkdir bin lib libexec local sbin share");
	_exec("cd $rootfs/usr/local; mkdir bin lib sbin www etc");
	_exec("mkdir $rootfs/usr/local/etc/asterisk");
	_exec("cd $rootfs/usr/local; ln -s /var/run/htpasswd www/.htpasswd");
	
	_exec("mkdir $image_name/asterisk");
	_exec("mkdir $image_name/asterisk/moh");
	_exec("mkdir $image_name/asterisk/modules");

	_exec("mkdir $image_name/pointstaging");
}

function populate_base($image_name) {
	global $dirs;

	_exec("perl {$dirs['minibsd']}/mkmini.pl {$dirs['minibsd']}/m0n0wall.files / $image_name/rootfs");
}

function populate_etc($image_name) {
	global $dirs;
	
	$rootfs = "$image_name/rootfs";
		
	_exec("cp -pR {$dirs['files']}/etc/* $rootfs/etc/");
	_exec("cp -p {$dirs['files']}/asterisk/*.conf $rootfs/usr/local/etc/asterisk/");
	_exec("cp {$dirs['etc']}/pubkey.pem $rootfs/etc/");
	
	_exec("ln -s /var/etc/resolv.conf $rootfs/etc/resolv.conf");
	_exec("ln -s /var/etc/hosts $rootfs/etc/hosts");
}

function populate_defaultconf($image_name) {
	global $dirs;
	
	_exec("cp {$dirs['phpconf']}/config.*.xml $image_name/rootfs/conf.default/");
}

function populate_zoneinfo($image_name) {
	global $dirs;
	
	_exec("cp {$dirs['files']}/zoneinfo.tgz $image_name/rootfs/usr/share/");
}

function populate_syslogd($image_name) {
	global $dirs;
	
	_exec("cd /usr/src/usr.sbin/syslogd; ".
			"install -s /usr/obj/usr/src/usr.sbin/syslogd/syslogd $image_name/rootfs/usr/sbin");
}

function populate_clog($image_name) {
	global $dirs;
	
	_exec("cd /usr/src/usr.sbin/clog; ".
			"install -s /usr/obj/usr/src/usr.sbin/clog/clog $image_name/rootfs/usr/sbin");
}

function populate_php($image_name) {
	global $dirs, $versions;
	
	_exec("cd {$dirs['packages']}/{$versions['php']}/; ".
		"install -s sapi/cgi/php $image_name/rootfs/usr/local/bin");
	_exec("cp {$dirs['files']}/php.ini $image_name/rootfs/usr/local/lib/");
}

function populate_ezipupdate($image_name) {
	global $dirs, $versions;
	
	_exec("cd ". $dirs['packages'] ."/{$versions['ezipupdate']}; ".
		"install -s ez-ipupdate $image_name/rootfs/usr/local/bin");
}

function populate_minihttpd($image_name) {
	global $dirs, $versions;
	
	_exec("cd {$dirs['packages']}/{$versions['mini_httpd']}; ".
			"install -s mini_httpd $image_name/rootfs/usr/local/sbin");
}

function populate_msntp($image_name) {
	global $versions;

	_exec("cd /usr/ports/net/msntp; ".
		"install -s work/{$versions['msntp']}/msntp $image_name/rootfs/usr/local/bin");
}

function populate_udesc_dump($image_name) {
	global $versions;

	_exec("cd /usr/ports/sysutils/udesc_dump; ".
		"install -s work/{$versions['udesc_dump']}/udesc_dump $image_name/rootfs/sbin");
}

function populate_msmtp($image_name) {
	global $dirs, $versions;
	
	_exec("cd {$dirs['packages']}/{$versions['msmtp']}/src; ".
		"install -s msmtp $image_name/rootfs/usr/local/bin");
}

function populate_asterisk($image_name) {
	global $dirs, $versions;
	
	$rootfs = "$image_name/rootfs";
	
	_exec("cd {$dirs['packages']}/{$versions['asterisk']}/; ".
		"gmake install DESTDIR=$rootfs");
	
	// link sounds and moh
	_exec("rm -rf $rootfs/usr/local/share/asterisk/sounds");
	_exec("cd $rootfs/usr/local/share/asterisk; ln -s /asterisk/sounds sounds");
	_exec("rm -rf $rootfs/usr/local/share/asterisk/moh");
	_exec("cd $rootfs/usr/local/share/asterisk; ln -s /asterisk/moh moh");
	
	// move modules
	_exec("cp $rootfs/usr/local/lib/asterisk/modules/* $image_name/asterisk/modules");
	_exec("rm -rf $rootfs/usr/local/lib/asterisk/modules");
	_exec("cd $rootfs/usr/local/lib/asterisk; ln -s /asterisk/modules modules");
	
	// clean up
	_exec("rm -rf $rootfs/usr/local/include");
	_exec("rm -rf $rootfs/usr/local/share/man");
}

function populate_sounds($image_name) {
	global $dirs, $core_sounds_version, $extra_sounds_version, 
			$sound_languages, $sounds, $wake_me_sounds, $digits, $musiconhold;
	
	// create english directories
	_exec("mkdir $image_name/asterisk/sounds");
	_exec("mkdir $image_name/asterisk/sounds/silence");
	_exec("mkdir $image_name/asterisk/sounds/digits");
	
	// sounds
	foreach($sound_languages as $sound_language) {
		
		// create other language directories
		if ($sound_language != "en") {
			_exec("mkdir $image_name/asterisk/sounds/$sound_language");
			_exec("mkdir $image_name/asterisk/sounds/digits/$sound_language");
		}

		// us-english
		if ($sound_language == "en") {
			// ulaw
			$formats = array("ulaw");
			foreach($formats as $format) {
				$distname = "asterisk-core-sounds-$sound_language-$format-$core_sounds_version";
				$disturl = "http://ftp.digium.com/pub/telephony/sounds/releases";
				
				if (!file_exists("{$dirs['sounds']}/$distname.tar.gz"))
						_exec("cd {$dirs['sounds']}; fetch $disturl/$distname.tar.gz");
						
				if (!file_exists("{$dirs['sounds']}/$distname")) {
					_exec("mkdir {$dirs['sounds']}/$distname");
					_exec("cd {$dirs['sounds']}; tar zxf $distname.tar.gz -C $distname");
				}
				
				foreach($sounds as $sound) {
					_exec("cp {$dirs['sounds']}/$distname/$sound* $image_name/asterisk/sounds");
				}
				foreach($digits as $digit) {
					_exec("cp {$dirs['sounds']}/$distname/digits/$digit.* $image_name/asterisk/sounds/digits");
				}
				
				_exec("cp {$dirs['sounds']}/$distname/beep* $image_name/asterisk/sounds");
				_exec("cp {$dirs['sounds']}/$distname/silence/* $image_name/asterisk/sounds/silence");
				
				
				// wakeme sounds
				_exec("cp {$dirs['sounds']}/$distname/minutes.* $image_name/asterisk/sounds");
				_exec("cp {$dirs['sounds']}/$distname/digits/a-m.* $image_name/asterisk/sounds/digits");
				_exec("cp {$dirs['sounds']}/$distname/digits/p-m.* $image_name/asterisk/sounds/digits");
				_exec("cp {$dirs['sounds']}/$distname/digits/1*.* $image_name/asterisk/sounds/digits");
				_exec("cp {$dirs['sounds']}/$distname/digits/20.* $image_name/asterisk/sounds/digits");
				_exec("cp {$dirs['sounds']}/$distname/digits/30.* $image_name/asterisk/sounds/digits");
				_exec("cp {$dirs['sounds']}/$distname/digits/40.* $image_name/asterisk/sounds/digits");
				_exec("cp {$dirs['sounds']}/$distname/digits/50.* $image_name/asterisk/sounds/digits");
				_exec("cp {$dirs['sounds']}/$distname/digits/oclock.* $image_name/asterisk/sounds/digits");
				_exec("cp {$dirs['sounds']}/$distname/digits/oh.* $image_name/asterisk/sounds/digits");

				$distname = "asterisk-extra-sounds-en-$format-$extra_sounds_version";
				if (!file_exists("{$dirs['sounds']}/$distname.tar.gz")) {
					_exec("cd {$dirs['sounds']}; fetch $disturl/$distname.tar.gz");
				}
				if (!file_exists("{$dirs['sounds']}/$distname")) {
					_exec("mkdir {$dirs['sounds']}/$distname");
					_exec("cd {$dirs['sounds']}; tar zxf $distname.tar.gz -C $distname");
				}

				foreach($wake_me_sounds as $sound) {
					_exec("cp {$dirs['sounds']}/$distname/$sound* $image_name/asterisk/sounds");
				}
			}

		// english gb
		} else if ($sound_language == "en-gb") {
			// ulaw
			$distname = "Alison_Keenan-British-English-ulaw";
			$disturl = "http://www.enicomms.com/cutglassivr/audiofiles";

			if (!file_exists("{$dirs['sounds']}/$distname.tar.gz"))
					_exec("cd {$dirs['sounds']}; fetch $disturl/$distname.tar.gz");

			if (!file_exists("{$dirs['sounds']}/$distname")) {
				_exec("mkdir {$dirs['sounds']}/$distname");
				_exec("cd {$dirs['sounds']}; tar zxf $distname.tar.gz -C $distname");
			}

			foreach($sounds as $sound) {
				_exec("cp {$dirs['sounds']}/$distname/$sound* $image_name/asterisk/sounds/en-gb/$sound". "ulaw");
			}
			foreach($digits as $digit) {
				_exec("cp {$dirs['sounds']}/$distname/digits/$digit.* $image_name/asterisk/sounds/digits/en-gb/$digit.ulaw");
			}

		// french
		} else if ($sound_language == "fr") {
			// gsm
			$distname = "FrenchPrompts";
			$disturl = "http://www.sineapps.com";
            
			if (!file_exists("{$dirs['sounds']}/$distname.tar.gz"))
					_exec("cd {$dirs['sounds']}; fetch $disturl/$distname.tar.gz");
            
			if (!file_exists("{$dirs['sounds']}/$distname")) {
				_exec("mkdir {$dirs['sounds']}/$distname");
				_exec("cd {$dirs['sounds']}; tar zxf $distname.tar.gz -C $distname");
			}
            
			foreach($sounds as $sound) {
				if ($sound == "conf-kicked.") // XXX: need a replacement sound
					continue;
				_exec("cp {$dirs['sounds']}/$distname/fr/$sound* $image_name/asterisk/sounds/fr");
			}
			foreach($digits as $digit) {
				_exec("cp {$dirs['sounds']}/$distname/digits/fr/$digit.* $image_name/asterisk/sounds/digits/fr");
			}


		// french canadian
		} else if ($sound_language == "fr-ca") {
			// ulaw
			$formats = array("ulaw");
			foreach($formats as $format) {
				$distname = "asterisk-core-sounds-fr-$format-$core_sounds_version";
				$disturl = "http://ftp.digium.com/pub/telephony/sounds/releases";
            	
				if (!file_exists("{$dirs['sounds']}/$distname.tar.gz"))
						_exec("cd {$dirs['sounds']}; fetch $disturl/$distname.tar.gz");
            	
				if (!file_exists("{$dirs['sounds']}/$distname")) {
					_exec("mkdir {$dirs['sounds']}/$distname");
					_exec("cd {$dirs['sounds']}; tar zxf $distname.tar.gz -C $distname");
				}
            	
				foreach($sounds as $sound) {
					_exec("cp {$dirs['sounds']}/$distname/$sound* $image_name/asterisk/sounds/fr-ca");
				}
				foreach($digits as $digit) {
					_exec("cp {$dirs['sounds']}/$distname/digits/$digit.* $image_name/asterisk/sounds/digits/fr-ca");
				}
			}


		// spanish
		} else if ($sound_language == "es") {
			// ulaw
			$formats = array("711u"); // XXX : files aren't automatically renamed here!
			foreach($formats as $format) {
				$distname = "asterisk-voces-es-v1_2-$format-voipnovatos";
				$disturl = "http://www.voipnovatos.es/voces";
            	
				if (!file_exists("{$dirs['sounds']}/$distname.tar.gz"))
						_exec("cd {$dirs['sounds']}; fetch $disturl/$distname.tar.gz");
            	
				if (!file_exists("{$dirs['sounds']}/$distname")) {
					_exec("cd {$dirs['sounds']}; tar zxf $distname.tar.gz");
				}
            	
				foreach ($sounds as $sound) {
					_exec("cp {$dirs['sounds']}/$distname/es/$sound* $image_name/asterisk/sounds/es");
				}
				foreach($digits as $digit) {
					_exec("cp {$dirs['sounds']}/$distname/es/digits/$digit.* $image_name/asterisk/sounds/digits/es");
				}
			}


		// danish
		} else if ($sound_language == "da") {
			// gsm
			$distname = "da-voice-prompts";
			$disturl = "http://www.globaltelip.dk/downloads";
        
			if (!file_exists("{$dirs['sounds']}/$distname.rar")) {
					_exec("cd {$dirs['sounds']}; fetch $disturl/$distname.rar");
					_exec("cd {$dirs['sounds']}; unrar e $distname.rar");
			}
        
			if (!file_exists("{$dirs['sounds']}/$distname")) {
				_exec("mkdir {$dirs['sounds']}/$distname");
				_exec("cd {$dirs['sounds']}; tar zxf da-voice-gsm.gz -C $distname");
			}
        
			foreach ($sounds as $sound) {
				_exec("cp {$dirs['sounds']}/$distname/sounds/da/$sound* $image_name/asterisk/sounds/da");
			}
			foreach($digits as $digit) {
				_exec("cp {$dirs['sounds']}/$distname/sounds/da/digits/$digit.* $image_name/asterisk/sounds/digits/da");
			}


		// german
		} else if ($sound_language == "de") {
			// gsm
			$distname = "asterisk-1.4-de-prompts";
			$disturl = "http://www.amooma.de/asterisk/sprachbausteine";
			
			if (!file_exists("{$dirs['sounds']}/$distname.tar.gz"))
					_exec("cd {$dirs['sounds']}; fetch $disturl/$distname.tar.gz");

			if (!file_exists("{$dirs['sounds']}/$distname")) {
				_exec("mkdir {$dirs['sounds']}/$distname");
				_exec("cd {$dirs['sounds']}; tar zxf $distname.tar.gz -C $distname");
			}
			
			foreach ($sounds as $sound) {
				_exec("cp {$dirs['sounds']}/$distname/de/$sound* $image_name/asterisk/sounds/de");
			}
			foreach($digits as $digit) {
				_exec("cp {$dirs['sounds']}/$distname/digits/de/$digit.* $image_name/asterisk/sounds/digits/de");
			}
			

		// italian
		} else if ($sound_language == "it") {
			// gsm
			$distname = "it_mm_sounds_20060510";
			$disturl = "http://mirror.tomato.it/ftp/pub/asterisk/suoni_ita";
        
			if (!file_exists("{$dirs['sounds']}/$distname.tar.gz"))
					_exec("cd {$dirs['sounds']}; fetch $disturl/$distname.tar.gz");
        
			if (!file_exists("{$dirs['sounds']}/$distname")) {
				_exec("mkdir {$dirs['sounds']}/$distname");
				_exec("cd {$dirs['sounds']}; tar zxf $distname.tar.gz -C $distname");
			}
        
			foreach ($sounds as $sound) {
				_exec("cp {$dirs['sounds']}/$distname/sounds_it_2006_05_10/$sound* $image_name/asterisk/sounds/it");
			}
			foreach($digits as $digit) {
				_exec("cp {$dirs['sounds']}/$distname/sounds_it_2006_05_10/digits/$digit.* $image_name/asterisk/sounds/digits/it");
			}
		
		
		// japanese
		} else if ($sound_language == "jp") {
			// gsm
			$distname = "asterisk-sound-jp";
			$disturl = "http://voip.gapj.net/files";
			
			if (!file_exists("{$dirs['sounds']}/$distname.tar.gz"))
					_exec("cd {$dirs['sounds']}; fetch $disturl/$distname.tar.gz");
        
			if (!file_exists("{$dirs['sounds']}/$distname")) {
				_exec("mkdir {$dirs['sounds']}/$distname");
				_exec("cd {$dirs['sounds']}; tar zxf $distname.tar.gz -C $distname");
			}
        
			foreach ($sounds as $sound) {
				_exec("cp {$dirs['sounds']}/$distname/jp/$sound* $image_name/asterisk/sounds/jp");
			}
			foreach($digits as $digit) {
				_exec("cp {$dirs['sounds']}/$distname/digits/jp/$digit.* $image_name/asterisk/sounds/digits/jp");
			}


		// dutch
		} else if ($sound_language == "nl") {
			// gsm
			$distname = "asterisksounds-0.4";
			$disturl = "http://www.borndgtl.cistron.nl";

			if (!file_exists("{$dirs['sounds']}/$distname.tar.gz"))
					_exec("cd {$dirs['sounds']}; fetch $disturl/$distname.tar.gz");

			if (!file_exists("{$dirs['sounds']}/$distname")) {
				_exec("mkdir {$dirs['sounds']}/$distname");
				_exec("cd {$dirs['sounds']}; tar zxf $distname.tar.gz -C $distname");
			}

			foreach ($sounds as $sound) {
				_exec("cp {$dirs['sounds']}/$distname/asterisksounds/$sound* $image_name/asterisk/sounds/nl");
			}
			foreach($digits as $digit) {
				_exec("cp {$dirs['sounds']}/$distname/asterisksounds/digits/$digit.* $image_name/asterisk/sounds/digits/nl");
			}

		// swedish
		} else if ($sound_language == "se") {
			// gsm
			$distname = "asterisk-prompt-se_1.045-orig";
			$disturl = "http://www.danielnylander.se/asterisk";

			if (!file_exists("{$dirs['sounds']}/$distname.tar.gz"))
					_exec("cd {$dirs['sounds']}; fetch $disturl/$distname.tar.gz");

			if (!file_exists("{$dirs['sounds']}/$distname")) {
				_exec("mkdir {$dirs['sounds']}/$distname");
				_exec("cd {$dirs['sounds']}; tar zxf $distname.tar.gz -C $distname");
			}

			foreach ($sounds as $sound) {
				_exec("cp {$dirs['sounds']}/$distname/sounds/se/$sound* $image_name/asterisk/sounds/se");
			}
			foreach($digits as $digit) {
				_exec("cp {$dirs['sounds']}/$distname/sounds/digits/se/$digit.* $image_name/asterisk/sounds/digits/se");
			}

		// russian
		} else if ($sound_language == "ru") {
			// gsm
			$distname = "sounds_AST_01";
			$disturl = "http://www.asterisk-support.ru/files/ivr";

			if (!file_exists("{$dirs['sounds']}/$distname.zip"))
					_exec("cd {$dirs['sounds']}; fetch $disturl/$distname.zip");

			if (!file_exists("{$dirs['sounds']}/$distname")) {
				_exec("mkdir {$dirs['sounds']}/$distname");
				_exec("cd {$dirs['sounds']}; unzip $distname.zip -d $distname");
				// make up for missing sounds
				_exec("cd {$dirs['sounds']}/$distname/sounds/; cp privacy-thankyou.gsm auth-thankyou.gsm");
				_exec("cd {$dirs['sounds']}/$distname/sounds/; cp vm-password.gsm agent-pass.gsm");
				_exec("cd {$dirs['sounds']}/$distname/sounds/; cp vm-incorrect.gsm auth-incorrect.gsm");
			}

			foreach ($sounds as $sound) {
				_exec("cp {$dirs['sounds']}/$distname/sounds/$sound* $image_name/asterisk/sounds/ru");
			}
			foreach($digits as $digit) {
				_exec("cp {$dirs['sounds']}/$distname/sounds/digits/$digit.* $image_name/asterisk/sounds/digits/ru");
			}
		}
	}
	
	// music on hold
	$formats = array("ulaw");
	foreach($formats as $format) {
		
		// download music on hold distributions
		$distname = "asterisk-moh-freeplay-$format";
		if (!file_exists("{$dirs['sounds']}/$distname.tar.gz")) {
			_exec("cd {$dirs['sounds']}; ".
				"fetch http://ftp.digium.com/pub/telephony/sounds/releases/$distname.tar.gz");
		}
		if (!file_exists("{$dirs['sounds']}/$distname")) {
			_exec("mkdir {$dirs['sounds']}/$distname");
			_exec("cd {$dirs['sounds']}; tar zxf $distname.tar.gz -C $distname");
		}
	
		// populate music on hold
		foreach ($musiconhold as $moh) {
			_exec("cp {$dirs['sounds']}/$distname/$moh* $image_name/asterisk/moh");
		}
	}
}

function populate_isdn($image_name) {
	global $dirs, $versions;

	_exec("cp {$dirs['packages']}/{$versions['i4b']}/trunk/chan_capi/chan_capi.so $image_name/asterisk/modules");
}

function populate_tools($image_name) {
	global $dirs;
	
	$rootfs = "$image_name/rootfs";
	
	_exec("cd {$dirs['tools']}; ".
		/*"install -s minicron $rootfs/usr/local/bin; ".*/
		"install -s verifysig $rootfs/usr/local/bin; ".
		"install -s wrapresetbtn $rootfs/usr/local/sbin; ".
		"install -s alix23xresetbtn $rootfs/usr/local/sbin; ".
		/*"install -s zttest $rootfs/sbin; ".*/
		"install runmsntp.sh $rootfs/usr/local/bin");
}

function populate_cgi($image_name) {
	global $dirs;
	
	$rootfs = "$image_name/rootfs";
	
	_exec("cd {$dirs['cgi']}; ".
		"install -s ajax.cgi $rootfs/usr/local/www");
}

function populate_phpcode($image_name) {
	populate_phpconf($image_name);
	populate_webgui($image_name);
}

function populate_phpconf($image_name) {
	global $dirs;

	_exec("cp -p {$dirs['etc']}/rc* $image_name/rootfs/etc/");
	_exec("cp {$dirs['phpconf']}/rc* $image_name/rootfs/etc/");
	_exec("cp {$dirs['phpconf']}/inc/* $image_name/rootfs/etc/inc/");
}

function populate_webgui($image_name) {
	global $dirs, $versions;
	
	update_locales();

	// copy over webgui files
	_exec("cp -R {$dirs['webgui']}/* $image_name/rootfs/usr/local/www/");
	// remove the .po locale source files
	_exec("find $image_name/rootfs/usr/local/www/locale -type f -name \"*.po\" -delete -print");

	// grab scriptaculous
	if (!file_exists("{$dirs['packages']}/{$versions['scriptaculous']}.zip")) {
		_exec("cd {$dirs['packages']}; ".
			"fetch http://script.aculo.us/dist/{$versions['scriptaculous']}.zip");
	}
	if (!file_exists("{$dirs['packages']}/{$versions['scriptaculous']}")) {
		_exec("cd {$dirs['packages']}; unzip {$versions['scriptaculous']}.zip");
	}
	_exec("cd {$dirs['packages']}/{$versions['scriptaculous']}; ".
		"cp src/dragdrop.js src/effects.js src/scriptaculous.js lib/prototype.js $image_name/rootfs/usr/local/www/");
}

function update_locales() {
	global $dirs;

	$path = $dirs['webgui'] . "/locale";

	// generate a .po skeleton file
	_exec("xgettext -o $path/skeleton.po --no-wrap --no-location --language=PHP " . 
		"phpconf/rc.* " .
		"webgui/*.php " .
		"webgui/*.inc " .
		"phpconf/inc/*.inc");

	$dh = opendir($path);
	while (false !== ($dirname = readdir($dh))) {
		if (preg_match("/[a-z]{2}\_[A-Z]{2}/", $dirname)) {
			// in every locale, make sure a messages.po file exists
			if (!file_exists("$path/$dirname/LC_MESSAGES/messages.po")) {
				_exec("touch $path/$dirname/LC_MESSAGES/messages.po");
			}
			// merge skeleton.po updates
			_exec("msgmerge $path/$dirname/LC_MESSAGES/messages.po $path/skeleton.po " .
					"-o $path/$dirname/LC_MESSAGES/messages.po");
			// compile .mo file
			_exec("cd $path/$dirname/LC_MESSAGES; msgfmt -o messages.mo messages.po");
		}
	}
}

function populate_jquery($image_name) {
	global $dirs, $versions;

	$repo_url = "http://jqueryjs.googlecode.com/svn/trunk/plugins";
	$plugins = array(
		"blockUI",
		"selectboxes",
		array("preloadImage", "http://plugins.jquery.com/files/jquery.preloadImages.js_1.txt")
	);

	// grab jquery
	if (!file_exists("{$dirs['packages']}/{$versions['jquery']}.js")) {
		_exec("cd {$dirs['packages']}; ".
			"fetch http://jqueryjs.googlecode.com/files/{$versions['jquery']}.js");
	}
	_exec("cp {$dirs['packages']}/{$versions['jquery']}.js $image_name/rootfs/usr/local/www/jquery.js");

	foreach ($plugins as $p) {
		if (!is_array($p)) {
			if (!file_exists($dirs['packages'] . "/jquery." . $p . ".js")) {
				_exec("cd " . $dirs['packages'] . "; " .
					"fetch " . $repo_url . "/" . $p . "/jquery." . $p . ".js");
			}
			_exec("cp " . $dirs['packages'] . "/jquery." . $p . ".js " . $image_name . "/rootfs/usr/local/www/");
		} else {
			if (!file_exists($dirs['packages'] . "/jquery." . $p[0] . ".js")) {
				_exec("cd " . $dirs['packages'] . "; " .
					"fetch -o jquery." . $p[0] . ".js " . $p[1]);
			}
			_exec("cp " . $dirs['packages'] . "/jquery." . $p[0] . ".js " . $image_name . "/rootfs/usr/local/www/");
		}
	}

	if (!file_exists("{$dirs['packages']}/jquery.progressbar.1.1.zip")) {
		_exec("cd {$dirs['packages']}; ".
			"fetch http://t.wits.sg/downloads/jquery.progressbar.1.1.zip");
		_exec("cd {$dirs['packages']}; unzip jquery.progressbar.1.1.zip");
	}
	_exec("cd {$dirs['packages']}/jquery.progressbar/; ".
		"cp js/jquery.progressbar.js images/progressbar.gif images/progressbg_black.gif ".
		$image_name . "/rootfs/usr/local/www/");
}

function populate_libs($image_name) {
	global $dirs;
	
	_exec("perl {$dirs['minibsd']}/mklibs.pl $image_name > tmp.libs");
	_exec("perl {$dirs['minibsd']}/mkmini.pl tmp.libs / $image_name/rootfs");
	_exec("rm tmp.libs");
}

function populate_pointstaging($image_name) {
	global $dirs, $versions, $platforms;

	foreach ($platforms as $platform) {
		$kernel = _platform_to_kernel($platform);
		_exec("cp /sys/i386/compile/$kernel/kernel.gz $image_name/pointstaging/kernel_$kernel.gz");
		_exec("cp {$dirs['boot']}/$platform/loader.rc $image_name/pointstaging/loader.rc_$platform");
	}

	_exec("cd /sys/i386/compile/ASKOZIAPBX_GENERIC/modules/usr/src/sys/modules/; " .
		"cp ugen/ugen.ko $image_name/pointstaging");

	_exec("cp {$dirs['packages']}/{$versions['zaptel']}/STAGE/*.ko $image_name/pointstaging");
	_exec("cp {$dirs['packages']}/{$versions['i4b']}/trunk/i4b/module/i4b.ko $image_name/pointstaging");
	_exec("cp /usr/obj/usr/src/sys/boot/i386/cdboot/cdboot $image_name/pointstaging");
	_exec("cp /usr/obj/usr/src/sys/boot/i386/loader/loader $image_name/pointstaging");
	_exec("cp /usr/obj/usr/src/sys/boot/i386/boot2/boot $image_name/pointstaging");
	_exec("cp {$dirs['phpconf']}/config.*.xml $image_name/pointstaging");
}

function populate_everything($image_name) {

	populate_base($image_name);
	populate_etc($image_name);
	populate_defaultconf($image_name);
	populate_zoneinfo($image_name);
	populate_syslogd($image_name);
	populate_clog($image_name);
	populate_php($image_name);
	populate_ezipupdate($image_name);
	populate_minihttpd($image_name);
	populate_msntp($image_name);
	populate_udesc_dump($image_name);
	populate_msmtp($image_name);
	populate_isdn($image_name);
	populate_asterisk($image_name);
	populate_sounds($image_name);
	populate_tools($image_name);
	populate_phpconf($image_name);
	populate_cgi($image_name);
	populate_jquery($image_name);
	populate_webgui($image_name);
	populate_libs($image_name);
	populate_pointstaging($image_name);
	
	$fd = fopen("$image_name/rootfs/etc/versions", "w");
	fwrite($fd, _generate_versions_file());
	fclose($fd);
}

function package($platform, $image_name) {
	global $dirs, $versions;
	global $mfsroot_pad, $asterisk_pad, $image_pad;
	global $low_power_modules, $low_power_libraries;
		
	_set_permissions($image_name);
	
	if (!file_exists("tmp")) {
		mkdir("tmp");
		mkdir("tmp/mnt");
		mkdir("tmp/stage");
	}
	
	$kernel = _platform_to_kernel($platform);
	
	// mfsroot

	// add rootfs
	_exec("cd tmp/stage; tar -cf - -C $image_name/rootfs ./ | tar -xpf -");

	// ...system modules		
	_exec("mkdir tmp/stage/boot");
	_exec("mkdir tmp/stage/boot/kernel");
	_exec("cp $image_name/pointstaging/*.ko tmp/stage/boot/kernel/");

	if ($platform != "generic-pc") {
		foreach ($low_power_libraries as $lpl) {
			_exec("rm tmp/stage/$lpl");
		}
	}

	// ...stamps
	_exec("echo \"". basename($image_name) ."\" > tmp/stage/etc/version");
	_exec("echo `date` > tmp/stage/etc/version.buildtime");
	_exec("echo " . time() . " > tmp/stage/etc/version.buildtime.unix");
	_exec("echo $platform > tmp/stage/etc/platform");		
	
	// get size and package mfsroot
	$mfsroot_size = _get_dir_size("tmp/stage") + $mfsroot_pad;
	
	_exec("dd if=/dev/zero of=tmp/mfsroot ibs=1k obs=1k count=$mfsroot_size");
	_exec("mdconfig -a -t vnode -f tmp/mfsroot -u 0");

	_exec("bsdlabel -rw md0 auto");
	_exec("newfs -O 1 -b 8192 -f 1024 -o space -m 0 /dev/md0c");

	_exec("mount /dev/md0c tmp/mnt");
	_exec("cd tmp/mnt; tar -cf - -C ../stage ./ | tar -xpf -");
	
	_log("---- $platform - " . basename($image_name) . " - mfsroot ----");
	_exec("df tmp/mnt");

	_exec("umount tmp/mnt");
	_exec("rm -rf tmp/stage/*");
	_exec("mdconfig -d -u 0");
	_exec("gzip -9 tmp/mfsroot");
	_exec("mv tmp/mfsroot.gz {$dirs['mfsroots']}/$platform-". basename($image_name) .".gz");
	
	// add mfsroot
	_exec("cp {$dirs['mfsroots']}/$platform-". basename($image_name) .".gz ".
		"tmp/stage/mfsroot.gz");
	
	// ...boot
	_exec("mkdir tmp/stage/boot");
	_exec("mkdir tmp/stage/boot/kernel");
	_exec("cp $image_name/pointstaging/loader tmp/stage/boot/");
	_exec("cp $image_name/pointstaging/loader.rc_$platform tmp/stage/boot/loader.rc");
	
	// ...conf
	_exec("mkdir tmp/stage/conf");
	_exec("cp $image_name/pointstaging/config.$platform.xml tmp/stage/conf/config.xml");
	_exec("cp $image_name/pointstaging/kernel_$kernel.gz tmp/stage/kernel.gz");
	
	$asterisk_size = _get_dir_size("$image_name/asterisk") + $asterisk_pad;
	$image_size = _get_dir_size("tmp/stage") + $asterisk_size + $image_pad;
	$image_size += 16 - ($image_size % 16);
	
	$a_size = ($image_size - $asterisk_size) * 2 - 16;
	$b_size = $asterisk_size * 2;
	$c_size = $image_size * 2;
    
	$label  = "# /dev/md0:\n";
	$label .= "8 partitions:\n";
	$label .= "#        size   offset    fstype   [fsize bsize bps/cpg]\n";
	$label .= " a: " . $a_size . "  16  4.2BSD  0  0\n";
	$label .= " b: " . $b_size . "   *  4.2BSD  0  0\n";
	$label .= " c: " . $c_size . "   0  unused  0  0\n\n";
    
	$fd = fopen("tmp/formatted.label", "w");
	if (!$fd) {
		printf("Error: cannot open label!\n");
		exit(1);
	}
	fwrite($fd, $label);
	fclose($fd);
	
	_exec("dd if=/dev/zero of=tmp/image.bin ibs=1k obs=1k count=$image_size");			
	_exec("mdconfig -a -t vnode -f tmp/image.bin -u 0");
	_exec("bsdlabel -BR -b $image_name/pointstaging/boot md0 tmp/formatted.label");
	_exec("cp tmp/formatted.label tmp/stage/original.bsdlabel");

	_exec("newfs -O 1 -b 8192 -f 1024 -o space -m 0 /dev/md0a");
	_exec("newfs -O 1 -b 8192 -f 1024 -o space -m 0 /dev/md0b");

	_exec("mount /dev/md0a tmp/mnt");
	_exec("cd tmp/mnt; tar -cf - -C ../stage ./ | tar -xpf -");
	_log("---- $platform - " . basename($image_name) . " - system partition ----");
	_exec("df tmp/mnt");
	_exec("umount tmp/mnt");
	
	_exec("mount /dev/md0b tmp/mnt");
	_exec("cd tmp/mnt; tar -cf - -C $image_name/asterisk ./ | tar -xpf -");
	// XXX quick fix to remove low power modules
	if ($platform != "generic-pc") {
		foreach ($low_power_modules as $lpm) {
			_exec("rm tmp/mnt/modules/$lpm");
		}
	}
	_log("---- $platform - " . basename($image_name) . " - asterisk partition ----");
	_exec("df tmp/mnt");
	_exec("umount tmp/mnt");		
	
	// cleanup
	_exec("mdconfig -d -u 0");
	_exec("gzip -9 tmp/image.bin");
	_exec("mv tmp/image.bin.gz {$dirs['images']}/pbx-$platform-". basename($image_name) .".img");


	_exec("rm -rf tmp");
}

function package_rootfs($image_name) {
	global $dirs;
	
	$name = basename($image_name);

	// tar
	_exec("cd {$dirs['images']}; tar -cf rootfs-$name.tar $name");
	// zip
	_exec("cd {$dirs['images']}; gzip -9 rootfs-$name.tar");
	// rename
	_exec("mv {$dirs['images']}/rootfs-$name.tar.gz {$dirs['images']}/pbx-rootfs-$name.tgz");

}

function package_cd($image_name) {
	global $dirs, $versions, $mfsroot_pad;

	_set_permissions($image_name);

	if (!file_exists("tmp")) {
		mkdir("tmp");
		mkdir("tmp/mnt");
		mkdir("tmp/stage");
	}

	$platform = "generic-pc-cdrom";
	$kernel = _platform_to_kernel($platform);
	$image_version = basename($image_name);

	// mfsroot

	// add rootfs
	_exec("cd tmp/stage; tar -cf - -C $image_name/rootfs ./ | tar -xpf -");
	_exec("cd tmp/stage/asterisk; tar -cf - -C $image_name/asterisk ./ | tar -xpf -");
	_exec("cp {$dirs['images']}/pbx-generic-pc-$image_version.img tmp/stage/");

	// ...system modules		
	_exec("mkdir tmp/stage/boot");
	_exec("mkdir tmp/stage/boot/kernel");
	_exec("cp $image_name/pointstaging/*.ko tmp/stage/boot/kernel/");

	// ...stamps
	_exec("echo \"$image_version\" > tmp/stage/etc/version");
	_exec("echo `date` > tmp/stage/etc/version.buildtime");
	_exec("echo " . time() . " > tmp/stage/etc/version.buildtime.unix");
	_exec("echo $platform > tmp/stage/etc/platform");		

	// get size and package mfsroot
	$mfsroot_size = _get_dir_size("tmp/stage") + $mfsroot_pad;

	_exec("dd if=/dev/zero of=tmp/mfsroot ibs=1k obs=1k count=$mfsroot_size");
	_exec("mdconfig -a -t vnode -f tmp/mfsroot -u 0");

	_exec("bsdlabel -rw md0 auto");
	_exec("newfs -O 1 -b 8192 -f 1024 -o space -m 0 /dev/md0c");

	_exec("mount /dev/md0c tmp/mnt");
	_exec("cd tmp/mnt; tar -cf - -C ../stage ./ | tar -xpf -");

	_log("---- $platform - $image_version - mfsroot ----");
	_exec("df tmp/mnt");

	_exec("umount tmp/mnt");
	_exec("rm -rf tmp/stage/*");
	_exec("mdconfig -d -u 0");
	_exec("gzip -9 tmp/mfsroot");
	_exec("mv tmp/mfsroot.gz {$dirs['mfsroots']}/$platform-$image_version.gz");

	// .iso
	_exec("mkdir tmp/cdroot");
	_exec("cp {$dirs['mfsroots']}/$platform-$image_version.gz tmp/cdroot/mfsroot.gz");
	_exec("cp $image_name/pointstaging/kernel_$kernel.gz tmp/cdroot/kernel.gz");
   
	_exec("mkdir tmp/cdroot/boot");
	_exec("cp $image_name/pointstaging/cdboot tmp/cdroot/boot/");
	_exec("cp $image_name/pointstaging/loader tmp/cdroot/boot/");
	_exec("cp $image_name/pointstaging/boot tmp/cdroot/boot/");
	_exec("cp {$dirs['boot']}/$platform/loader.rc tmp/cdroot/boot/");
    
	_exec("mkisofs -b \"boot/cdboot\" -no-emul-boot -A \"AskoziaPBX CD-ROM image\" ".
		"-c \"boot/boot.catalog\" -d -r -publisher \"askozia.com\" ".
		"-p \"AskoziaPBX\" -V \"AskoziaPBXCD\" -o \"AskoziaPBX.iso\" tmp/cdroot/");
		
	_exec("mv AskoziaPBX.iso {$dirs['images']}/pbx-cdrom-$image_version.iso");

	_exec("rm -rf tmp");
}

function release($name) {
	global $platforms, $dirs;
	
	package_rootfs($name);

	// signing
	foreach($platforms as $platform) {
		$filename = "pbx-" . $platform . "-" . basename($name) . ".img";
		_exec("{$dirs['tools']}/sign ../sig/AskoziaPBX_private_key.pem {$dirs['images']}/$filename");
	}
}

function clean_dots() {
	passthru("find ./ -type f -name \"._*\" -delete -print");
}

function parse_check() {
	passthru("find webgui/ -type f -name \"*.php\" -exec php -l {} \; -print | grep Parse");
	passthru("find webgui/ -type f -name \"*.inc\" -exec php -l {} \; -print | grep Parse");
	passthru("find phpconf/ -type f -name \"*rc*\" -exec php -l {} \; -print | grep Parse");
	passthru("find phpconf/ -type f -name \"*.inc\" -exec php -l {} \; -print | grep Parse");
}

function _get_dir_size($dir) {
	$line = exec("du -d 0 $dir");
	$pieces = preg_split("/\s+/", $line);
	return $pieces[0];
}

function _set_permissions($image_name) {
	
	_exec("chmod 755 $image_name/rootfs/etc/rc*");
	_exec("chmod 644 $image_name/rootfs/etc/pubkey.pem");
	
	_exec("chmod 644 $image_name/rootfs/etc/inc/*");
	
	_exec("chmod 644 $image_name/rootfs/conf.default/config.*.xml");

	_exec("chmod 644 $image_name/rootfs/usr/local/www/*");
	_exec("chmod 755 $image_name/rootfs/usr/local/www/*.php");
	_exec("chmod 755 $image_name/rootfs/usr/local/www/*.cgi");
}

function _stamp_package_as_patched($package_version) {
	global $dirs;
	
	touch("{$dirs['packages']}/$package_version/$package_version.patched");
}

function _is_patched($package_version) {
	global $dirs;
	
	return(file_exists("{$dirs['packages']}/$package_version/$package_version.patched"));
}

function _generate_versions_file() {
	global $dirs, $versions;
	
	$contents = "";

	foreach ($versions as $v) {
		if (strstr($v, "trunk")) {
			$contents .= $v . " : revision " . _get_svn_revision_number($dirs['packages'] . "/" . $v) . "\n";
		} else {
			$contents .= $v . "\n";
		}
	}
	
	return $contents;
}

function _get_svn_revision_number($path) {

	exec("svn info $path | grep Revision", $output);
	$pair = explode(" ", $output[0]);

	return $pair[1];
}

function _platform_to_kernel($platform) {
	global $platforms;
	
	if (array_search($platform, $platforms) === false) {
		_log("Platform doesn't exist!");
		exit(1);
	}
	
	if ($platform == "generic-pc" || $platform == "generic-pc-cdrom") {
		$kernel = "ASKOZIAPBX_GENERIC";
	} else {
		$kernel = "ASKOZIAPBX_" . strtoupper($platform);
	}

	return $kernel;
}

function _exec($cmd) {
	$ret = 0;
	_log($cmd);
	passthru($cmd, $ret);
	if($ret != 0) {
		_log("COMMAND FAILED: $cmd");
		exit(1);
	}
}

function _log($msg) {
	echo "$msg\n";
}


// --[ command line parsing ]--------------------------------------------------

if ($argv[1] == "prepare") {

	$f = implode("_", array_slice($argv, 1));
	if (!function_exists($f)) {
		_log("Invalid patch command!");
		exit(1);
	}
	$f();

} else if ($argv[1] == "new") {
	
	$image_name = "{$dirs['images']}/" . rtrim($argv[2], "/");
	create($image_name);
	populate_everything($image_name);

// patch functions are all defined with no arguments
} else if($argv[1] == "patch") {
	$f = implode("_", array_slice($argv, 1));
	if (!function_exists($f)) {
		_log("Invalid patch command!");
		exit(1);
	}
	$f();

// build functions are all defined with no arguments except for "build_kernel"
} else if($argv[1] == "build") {
	if($argv[2] == "kernel") {
		build_kernel($argv[3]);
	} else {
		$f = implode("_", array_slice($argv, 1));
		if (!function_exists($f)) {
			_log("Invalid build command!");
			exit(1);
		}
		$f();
	}

// populate functions are all defined with a single argument:
// (image_name directory)
} else if($argv[1] == "populate") {
	// make a function name out of the arguments
	$f = implode("_", array_slice($argv, 1, 2));
	if (!function_exists($f)) {
		_log("Invalid populate command!");
		exit(1);
	}
	// construct an absolute path to the image
	$image_name = "{$dirs['images']}/" . rtrim($argv[3], "/");
	$f($image_name);


// the package function is defined with two arguments:
// (platform, image_name)
} else if ($argv[1] == "package") {
	// construct an absolute path to the image
	$image_name = "{$dirs['images']}/" . rtrim($argv[3], "/");
	// not a valid image, show usage
	if (!file_exists($image_name)) {
		_log("Image does not exist!");
		exit(1);
	}
	// clean up any .svn directories
	passthru("find -d $image_name -name \".svn\" -exec rm -r '{}' \; -print");
	// clean up and ._ files
	passthru("find $image_name -type f -name \"._*\" -delete -print");
	// we're packaging all platforms go right ahead
	if ($argv[2] == "all") {
		foreach($platforms as $platform) {
			package($platform, $image_name);			
		}
		package_rootfs($image_name);
		package_cd($image_name);

	// packaging the root file system distribution
	} else if ($argv[2] == "rootfs") {
		package_rootfs($image_name);

	// packaging the cd iso
	} else if ($argv[2] == "cd") {
		package_cd($image_name);

	// check the specific platform before attempting to package
	} else if (in_array($argv[2], $platforms)) {
		package($argv[2], $image_name);

	// not a valid package command...
	} else {
		_log("Invalid packaging command!");
		exit(1);
	}

}  else if ($argv[1] == "release") {

	$image_name = "{$dirs['images']}/" . rtrim($argv[2], "/");
	foreach($platforms as $platform) {
		package($platform, $image_name);			
	}
	release($image_name);

// handle all two argument commands
//
// XXX : single argument commands should be moved into functions and handled
//			in the same manner
} else if (isset($argv[1]) && isset($argv[2])) {
	$f = implode("_", array_slice($argv, 1));
	if (!function_exists($f)) {
		_log("Invalid " . $argv[2] . " command!");
		exit(1);
	}
	$f();
	
} else {
	_log("Huh?");
	exit(1);
}

exit();

?>
