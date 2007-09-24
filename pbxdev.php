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

$php_version		= "php-4.4.7";
$pecl_sqlite_version= "SQLite-1.0.3";
$mini_httpd_version	= "mini_httpd-1.19";
$asterisk_version	= "asterisk-1.4.11";
$msmtp_version		= "msmtp-1.4.11";
$zaptel_version		= "zaptel";
$oslec_version		= "oslec-trunk";

// --[ sounds ]----------------------------------------------------------------

$core_sounds_version= "1.4.7";
$extra_sounds_version="1.4.6";
$sound_languages	= explode(" ", "en de it es fr jp nl se ru");
$sounds				= explode(" ", 
						"auth-thankyou. ".
						"conf-onlyperson. conf-getpin. conf-invalidpin. conf-kicked. ".
						"pbx-transfer. pbx-invalid. pbx-invalidpark. ".
						"vm-intro. vm-theperson. vm-isunavail. vm-isonphone. vm-goodbye.");
$wake_me_sounds		= explode(" ",
						"this-is-yr-wakeup-call. to-snooze-for. to-cancel-wakeup. ".
						"for. press-0. press-1. wakeup-call-cancelled. to-rqst-wakeup-call. ".
						"enter-a-time. im-sorry. 1-for-am-2-for-pm. rqsted-wakeup-for. ".
						"1-yes-2-no. thank-you-for-calling. goodbye.");
$digits				= explode(" ", "0 1 2 3 4 5 6 7 8 9");
$musiconhold		= explode(" ", "fpm-calm-river");

// --[ modules ]---------------------------------------------------------------

$low_power_modules= explode(" ", "codec_speex.so codec_ilbc.so");

// --[ image padding ]---------------------------------------------------------

$mfsroot_pad	= 2560;
$asterisk_pad	= 512;
$image_pad		= 768;

// --[ possible platforms and kernels ]----------------------------------------

$platform_list = "generic-pc net48xx net55xx wrap alix1x hl4xx"; //generic-pc-cdrom net45xx";
$platforms = explode(" ", $platform_list);


// --[ sanity checks and env info ]--------------------------------------------

$dirs['pwd']				= rtrim(shell_exec("pwd"), "\n");
$dirs['boot']				= $dirs['pwd'] . "/build/boot";
$dirs['kernelconfigs']		= $dirs['pwd'] . "/build/kernelconfigs";
$dirs['minibsd']			= $dirs['pwd'] . "/build/minibsd";
$dirs['patches']			= $dirs['pwd'] . "/build/patches";
$dirs['asterisk_modules']	= $dirs['pwd'] . "/build/asterisk_modules";
$dirs['tools']				= $dirs['pwd'] . "/build/tools";
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
	//_exec("cd /usr/ports/net/mDNSResponder; make install");
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

function patch_everything() {
	
	patch_kernel();
	patch_syslogd();
	patch_bootloader();
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
	build_kernels();
}

function build_kernels() {
	global $platforms;
	
	foreach($platforms as $platform) {
		if($platform == "generic-pc-cdrom") {
			continue;
		}
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
	global $dirs, $php_version, $pecl_sqlite_version;

	if (!file_exists("/usr/local/bin/autoconf")) {
		_exec("cd /usr/ports/devel/autoconf213; make install clean");
		_exec("ln -s /usr/local/bin/autoconf213 /usr/local/bin/autoconf");
		_exec("ln -s /usr/local/bin/autoheader213 /usr/local/bin/autoheader");
	}
	if (!file_exists("{$dirs['packages']}/$php_version.tar.gz")) {
		_exec("cd {$dirs['packages']}; " .
				"fetch http://br.php.net/distributions/$php_version.tar.gz");
	}
	if (!file_exists("{$dirs['packages']}/$php_version")) {
		_exec("cd {$dirs['packages']}; tar zxf $php_version.tar.gz");
	}
	if (!file_exists("{$dirs['packages']}/$php_version/ext/sqlite")) {
		_exec("cd ". $dirs['packages'] ."/$php_version/ext; ".
			"fetch http://pecl.php.net/get/$pecl_sqlite_version.tgz; ".
			"tar zxf $pecl_sqlite_version.tgz; ".
				"mv $pecl_sqlite_version sqlite");
	}
	
	_exec("cd {$dirs['packages']}/$php_version; ".
			"rm configure; ".
			"./buildconf --force; ".
			"./configure --without-mysql --with-pear --with-openssl --enable-discard-path --enable-sockets --enable-bcmath --enable sqlite; ".
			"make");
}

function build_minihttpd() {
	global $dirs, $mini_httpd_version;
	
	if (!file_exists("{$dirs['packages']}/$mini_httpd_version.tar.gz")) {
		_exec("cd {$dirs['packages']}; ".
			"fetch http://www.acme.com/software/mini_httpd/$mini_httpd_version.tar.gz");
	}
	if (!file_exists("{$dirs['packages']}/$mini_httpd_version")) {
		_exec("cd {$dirs['packages']}; tar zxf $mini_httpd_version.tar.gz");
	}
	if (!_is_patched($mini_httpd_version)) {
		_exec("cd {$dirs['packages']}/$mini_httpd_version; ".
			"patch < {$dirs['patches']}/packages/mini_httpd.patch");
		_stamp_package_as_patched($mini_httpd_version);
	}
	_exec("cd {$dirs['packages']}/$mini_httpd_version; make clean; make");
}

function build_asterisk() {
	global $dirs, $asterisk_version;
	
	if (!file_exists("{$dirs['packages']}/$asterisk_version.tar.gz")) {
		_exec("cd {$dirs['packages']}; ".
			"fetch http://ftp.digium.com/pub/asterisk/releases/$asterisk_version.tar.gz");
	}
	if (!file_exists("{$dirs['packages']}/$asterisk_version")) {
		_exec("cd {$dirs['packages']}; tar zxf $asterisk_version.tar.gz");
	}
	if (!_is_patched($asterisk_version)) {
		$patches = array(
			"makefile",
			"cdr_to_syslog",
			//"channel_queue",
			"voicemail_readback_number",
			"chan_sip_default_cid"
		);
		foreach ($patches as $patch) {
			_exec("cd {$dirs['packages']}/$asterisk_version; ".
				"patch < {$dirs['patches']}/packages/asterisk_$patch.patch");
			_log("\n\n --- patch: $patch -----------\n\n");
		}
		_stamp_package_as_patched($asterisk_version);
	}
	// copy make options
	_exec("cp {$dirs['patches']}/packages/menuselect.makeopts /etc/asterisk.makeopts");
	// copy wakeme application
	_exec("cp {$dirs['asterisk_modules']}/app_wakeme.c {$dirs['packages']}/$asterisk_version/apps");
	// copy includes
	_exec("cp -pR {$dirs['packages']}/$asterisk_version/include/* /usr/local/include");
	// reconfigure and make
	_exec("cd {$dirs['packages']}/$asterisk_version/; ./configure; gmake");
}

function build_zaptel() {
	global $dirs, $zaptel_version;
	
	if (!file_exists("{$dirs['packages']}/$zaptel_version")) {
		_exec("cd {$dirs['packages']}; ".
			"svn co --username svn --password svn ".
			"https://svn.pbxpress.com:1443/repos/zaptel-bsd/branches/zaptel-1.4 $zaptel_version");
	} 
	// remove old headers if they're around
	_exec("rm -f /usr/local/include/zaptel.h /usr/local/include/tonezone.h");
	
	// make new directory for moved zaptel headers
	if (!file_exists("/usr/local/include/zaptel/")) {
		_exec("mkdir /usr/local/include/zaptel");
	}
	// copy over header files
	_exec("cd {$dirs['packages']}/$zaptel_version/; ".
		"cp zaptel/zaptel.h ztcfg/tonezone.h /usr/local/include/zaptel/");
		
	// copy over better ztdummy.c
	_exec("cd {$dirs['packages']}/$zaptel_version/ztdummy; cp ztdummy.c ztdummy_orig.c");
	_exec("cp {$dirs['patches']}/packages/ztdummy.c {$dirs['packages']}/$zaptel_version/ztdummy");
	
	_exec("cd {$dirs['packages']}/$zaptel_version; make clean");
	_exec("cd {$dirs['packages']}/$zaptel_version; make; make install");

	if (!file_exists("{$dirs['packages']}/$zaptel_version/STAGE")) {
		_exec("cd {$dirs['packages']}/$zaptel_version/; ".
			"mkdir STAGE STAGE/bin STAGE/lib STAGE/etc");
	}
	
	_exec("cd {$dirs['packages']}/$zaptel_version; ".
		"make install PREFIX={$dirs['packages']}/$zaptel_version/STAGE");
}

function build_isdn() {
	global $dirs;
	
	if (!file_exists("{$dirs['packages']}/i4b")) {
		_exec("cd {$dirs['packages']}; ".
			"svn --username anonsvn --password anonsvn co svn://svn.turbocat.net/i4b i4b");
	}
	_exec("cd {$dirs['packages']}/i4b/trunk/i4b/FreeBSD.i4b; make S=../src package; make install");
	
	_exec("cd {$dirs['packages']}/i4b/trunk/i4b/src/usr.sbin/i4b; make clean; make all I4B_WITHOUT_CURSES=yes; make install");
		
	_exec("cd {$dirs['packages']}/i4b/trunk/chan_capi/; gmake clean; gmake all");
}

function build_msntp() {
	
	_exec("cd /usr/ports/net/msntp; make clean; make");
}

function build_udesc_dump() {
	
	_exec("cd /usr/ports/sysutils/udesc_dump; make clean; make");
}

function build_msmtp() {
	global $dirs, $msmtp_version;
	
	if (!file_exists("{$dirs['packages']}/$msmtp_version.tar.bz2")) {
		_exec("cd {$dirs['packages']}; ".
			"fetch http://belnet.dl.sourceforge.net/sourceforge/msmtp/$msmtp_version.tar.bz2");
	}
	if (!file_exists("{$dirs['packages']}/$msmtp_version")) {
		_exec("cd {$dirs['packages']}; bunzip2 $msmtp_version.tar.bz2; tar xf $msmtp_version.tar");
	}
	
	_exec("cd {$dirs['packages']}/$msmtp_version; ".
		"./configure --with-ssl=openssl --without-libgsasl --without-libidn --disable-nls; ".
		"make");
}


function build_tools() {
	global $dirs;
	
	_exec("cd {$dirs['tools']}; gcc -o stats.cgi stats.c");
	_exec("cd {$dirs['tools']}; gcc -o verifysig -lcrypto verifysig.c");
	_exec("cd {$dirs['tools']}; gcc -o wrapresetbtn wrapresetbtn.c");
	_exec("cd {$dirs['tools']}; gcc -o zttest zttest.c");
	/*_exec("cd ". $dirs['tools'] ."; gcc -o minicron minicron.c");*/
}

function build_bootloader() {
	
	_exec("cd /sys/boot; make clean; make obj; make");
}

function build_res_bonjour() {
	global $dirs;
	
	$resver = "res_bonjour-2.0rc1";
	if (file_exists(("{$dirs['packages']}/$resver"))) {
		_exec("cd {$dirs['packages']}; fetch http://www.mezzo.net/asterisk/$resver.tgz");
		_exec("cd {$dirs['packages']}; tar zxf $resver.tgz");
	}
	
	
	
}

function build_oslec() {
	global $dirs, $oslec_version;
	
	if (!file_exists("{$dirs['packages']}/$oslec_version")) {
		_exec("cd {$dirs['packages']}; ".
			"svn http://svn.rowetel.com/software/oslec/trunk/ $oslec_version");
	}
	
	_exec("cd {$dirs['packages']}/$oslec_version/kernel-freebsd; make clean");
	_exec("cd {$dirs['packages']}/$oslec_version/kernel-freebsd; make");
	
}

function build_packages() {

	build_php();
	build_msmtp();
	build_minihttpd();
	build_zaptel();
	build_asterisk();
	build_isdn();
	build_oslec();
	//build_res_bonjour();
	//build_fop();
}

function build_ports() {
	
	build_msntp();
	build_udesc_dump();
}

function build_everything() {

	build_packages();
	build_ports();
	build_syslogd();
	build_clog();
	build_tools();
	build_kernels();
	build_bootloader();
}

function create($image_name) {
	global $dirs;

	if (file_exists($image_name)) {
		_exec("rm -rf $image_name");
		_exec("rm -rf {$dirs['images']}/*$image_name.img");
		_exec("rm -rf {$dirs['mfsroots']}/*$image_name.gz");
	}

	_exec("mkdir $image_name");
	$rootfs = "$image_name/rootfs";
	_exec("mkdir $rootfs");
	
	_exec("cd $rootfs; mkdir lib bin cf conf.default dev etc ftmp mnt libexec proc root sbin tmp usr var asterisk");
	_exec("cd $rootfs/var; mkdir tmp");
	_exec("cd $rootfs; ln -s /cf/conf conf");
	_exec("mkdir $rootfs/etc/inc");
	_exec("cd $rootfs/usr; mkdir bin lib libexec local sbin share");
	_exec("cd $rootfs/usr/local; mkdir bin lib sbin www etc");
	_exec("mkdir $rootfs/usr/local/etc/asterisk");
	_exec("cd $rootfs/usr/local; ln -s /var/run/htpasswd www/.htpasswd");
	
	_exec("mkdir $image_name/asterisk");
	_exec("mkdir $image_name/asterisk/moh");
	_exec("mkdir $image_name/asterisk/modules");
}

function populate_base($image_name) {
	global $dirs;

	_exec("perl {$dirs['minibsd']}/mkmini.pl ".
		"{$dirs['minibsd']}/m0n0wall.files / $image_name/rootfs");
}

function populate_etc($image_name) {
	global $dirs;
	
	$rootfs = "$image_name/rootfs";
		
	_exec("cp -p {$dirs['files']}/etc/* $rootfs/etc/");
	_exec("cp -p {$dirs['files']}/asterisk/*.conf $rootfs/usr/local/etc/asterisk/");
	_exec("cp -p {$dirs['etc']}/rc* $rootfs/etc/");
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
	global $dirs, $php_version;
	
	_exec("cd {$dirs['packages']}/$php_version/; ".
		"install -s sapi/cgi/php $image_name/rootfs/usr/local/bin");
	_exec("cp {$dirs['files']}/php.ini $image_name/rootfs/usr/local/lib/");
}

function populate_minihttpd($image_name) {
	global $dirs, $mini_httpd_version;
	
	_exec("cd {$dirs['packages']}/$mini_httpd_version; ".
			"install -s mini_httpd $image_name/rootfs/usr/local/sbin");
}

function populate_msntp($image_name) {
	
	_exec("cd /usr/ports/net/msntp; ".
		"install -s work/msntp-*/msntp $image_name/rootfs/usr/local/bin");
}

function populate_udesc_dump($image_name) {
	
	_exec("cd /usr/ports/sysutils/udesc_dump; ".
		"install -s work/udesc_dump-*/udesc_dump $image_name/rootfs/sbin");
}

function populate_msmtp($image_name) {
	global $dirs, $msmtp_version;
	
	_exec("cd {$dirs['packages']}/$msmtp_version/src; ".
		"install -s msmtp $image_name/rootfs/usr/local/bin");
}

function populate_asterisk($image_name) {
	global $dirs, $asterisk_version;
	
	$rootfs = "$image_name/rootfs";
	
	_exec("cd {$dirs['packages']}/$asterisk_version/; ".
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
			// ulaw and gsm
			$formats = array("gsm", "ulaw");
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
			}
			
			// wakeme sounds
			$distname = "asterisk-core-sounds-en-gsm-$core_sounds_version";
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
			
			$distname = "asterisk-extra-sounds-en-gsm-$extra_sounds_version";
			if (!file_exists("{$dirs['sounds']}/$distname.tar.gz"))
					_exec("cd {$dirs['sounds']}; fetch $disturl/$distname.tar.gz");
					
			if (!file_exists("{$dirs['sounds']}/$distname")) {
				_exec("mkdir {$dirs['sounds']}/$distname");
				_exec("cd {$dirs['sounds']}; tar zxf $distname.tar.gz -C $distname");
			}
			
			foreach($wake_me_sounds as $sound) {
				_exec("cp {$dirs['sounds']}/$distname/$sound* $image_name/asterisk/sounds");
			}


		// french
		} else if ($sound_language == "fr") {
			// ulaw and gsm
			$formats = array("gsm", "ulaw");
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
					_exec("cp {$dirs['sounds']}/$distname/$sound* $image_name/asterisk/sounds/fr");
				}
				foreach($digits as $digit) {
					_exec("cp {$dirs['sounds']}/$distname/digits/$digit.* $image_name/asterisk/sounds/digits/fr");
				}
			}


		// spanish
		} else if ($sound_language == "es") {
			// gsm & 711u
			$formats = array("gsm", "711u");
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
				_exec("cd {$dirs['sounds']}/$distname/sounds/; cp privacy-thankyou.gsm auth-thankyou.gsm");
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
	$formats = array("gsm", "ulaw");
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

function populate_zaptel($image_name) {
	global $dirs, $zaptel_version;

	if (file_exists("{$dirs['packages']}/$zaptel_version/STAGE/bin/zttest")) {
		_exec("rm {$dirs['packages']}/$zaptel_version/STAGE/bin/zttest");
	}
	_exec("cd {$dirs['packages']}/$zaptel_version/STAGE; ".
		"install -s bin/* $image_name/rootfs/sbin; ".
		"cp lib/* $image_name/rootfs/lib");
}

function populate_isdn($image_name) {
	global $dirs;
	
	_exec("cd {$dirs['packages']}/i4b/trunk/i4b/src/usr.sbin/i4b; ".
		"cp -p isdnconfig/isdnconfig isdndebug/isdndebug isdndecode/isdndecode ".
		"isdnmonitor/isdnmonitor isdntest/isdntest isdntrace/isdntrace ".
		"$image_name/rootfs/sbin; ");
	_exec("cp {$dirs['packages']}/i4b/trunk/chan_capi/chan_capi.so $image_name/asterisk/modules");
}

function populate_tools($image_name) {
	global $dirs;
	
	$rootfs = "$image_name/rootfs";
	
	_exec("cd {$dirs['tools']}; ".
		"install -s stats.cgi $rootfs/usr/local/www; ".
		/*"install -s minicron $rootfs/usr/local/bin; ".*/
		"install -s verifysig $rootfs/usr/local/bin; ".
		"install -s wrapresetbtn $rootfs/usr/local/sbin; ".
		"install -s zttest $rootfs/sbin; ".
		"install runmsntp.sh $rootfs/usr/local/bin");
}

function populate_phpconf($image_name) {
	global $dirs;

	_exec("cp {$dirs['phpconf']}/rc* $image_name/rootfs/etc/");
	_exec("cp {$dirs['phpconf']}/inc/* $image_name/rootfs/etc/inc/");
}

function populate_webgui($image_name) {
	global $dirs;
	
	_exec("cp {$dirs['webgui']}/* $image_name/rootfs/usr/local/www/");
}

function populate_libs($image_name) {
	global $dirs, $asterisk_version;
	
	_exec("perl {$dirs['minibsd']}/mklibs.pl $image_name > tmp.libs");
	_exec("perl {$dirs['minibsd']}/mkmini.pl tmp.libs / $image_name/rootfs");
	_exec("rm tmp.libs");
}

function populate_everything($image_name) {

	populate_base($image_name);
	populate_etc($image_name);
	populate_defaultconf($image_name);
	populate_zoneinfo($image_name);
	populate_syslogd($image_name);
	populate_clog($image_name);
	populate_php($image_name);
	populate_minihttpd($image_name);
	populate_msntp($image_name);
	populate_udesc_dump($image_name);
	populate_msmtp($image_name);
	populate_zaptel($image_name);
	populate_isdn($image_name);
	populate_asterisk($image_name);
	populate_sounds($image_name);
	populate_tools($image_name);
	populate_phpconf($image_name);
	populate_webgui($image_name);
	populate_libs($image_name);
}

function package($platform, $image_name) {
	global $dirs;
	global $mfsroot_pad, $asterisk_pad, $image_pad;
	global $zaptel_version, $oslec_version, $asterisk_version;
	global $low_power_modules;
		
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
	if ($platform == "generic-pc" || 
		$platform == "generic-pc-cdrom") {
		_exec("cp /sys/i386/compile/$kernel/modules/usr/src/sys/modules/acpi/acpi/acpi.ko tmp/stage/boot/kernel/");
	}
	
	// ...zaptel modules
	$zaptel_modules = array(
		"zaptel/zaptel.ko",
		"ztdummy/ztdummy.ko",
		
		//"qozap/qozap.ko",
		//"tau32pci/tau32pci.ko",
		"wcfxo/wcfxo.ko",
		"wcfxs/wcfxs.ko",
		//"wct1xxp/wct1xxp.ko",
		//"wct4xxp/wct4xxp.ko",
		//"wcte11xp/wcte11xp.ko"
		//"zaphfc/zaphfc.ko"
	);
	foreach ($zaptel_modules as $zaptel_module) {
		_exec("cp {$dirs['packages']}/$zaptel_version/$zaptel_module tmp/stage/boot/kernel/");
	}
	// ...oslec module
	_exec("cp {$dirs['packages']}/$oslec_version/kernel-freebsd/oslec.ko tmp/stage/boot/kernel/");
	
	// XXX i4b module
	//_exec("cp {$dirs['packages']}/i4b/trunk/i4b/STAGE/boot/kernel/i4b.ko tmp/stage/boot/kernel/");
	
	// ...stamps
	_exec("echo \"". basename($image_name) ."\" > tmp/stage/etc/version");
	_exec("echo `date` > tmp/stage/etc/version.buildtime");
	_exec("echo $platform > tmp/stage/etc/platform");		
	
	// get size and package mfsroot
	$mfsroot_size = _get_dir_size("tmp/stage") + $mfsroot_pad;
	
	_exec("dd if=/dev/zero of=tmp/mfsroot bs=1k count=$mfsroot_size");
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


	// .img
	if ($platform != "generic-pc-cdrom") {
		
		// add mfsroot
		_exec("cp {$dirs['mfsroots']}/$platform-". basename($image_name) .".gz ".
			"tmp/stage/mfsroot.gz");
		
		// ...boot
		_exec("mkdir tmp/stage/boot");
		_exec("mkdir tmp/stage/boot/kernel");
	    _exec("cp /usr/obj/usr/src/sys/boot/i386/loader/loader tmp/stage/boot/");
		_exec("cp {$dirs['boot']}/$platform/loader.rc tmp/stage/boot/");
	
		// ...conf
		_exec("mkdir tmp/stage/conf");
		_exec("cp {$dirs['phpconf']}/config.$platform.xml tmp/stage/conf/config.xml");
		_exec("cp /sys/i386/compile/$kernel/kernel.gz tmp/stage/kernel.gz");
		
		$asterisk_size = _get_dir_size("$image_name/asterisk") + $asterisk_pad;
		$image_size = _get_dir_size("tmp/stage") + $asterisk_size + $image_pad;
		$image_size += 16 - ($image_size % 16);
		
		$a_size = ($image_size - $asterisk_size) * 2 - 16;
		$b_size = $asterisk_size * 2;
		$c_size = $image_size * 2;

		$label  = "# /dev/md0:\n";
		$label .= "8 partitions:\n";
		$label .= "#        size   offset    fstype   [fsize bsize bps/cpg]\n";
		$label .= " a: " . $a_size . "  16  unused  0  0\n";
		$label .= " b: " . $b_size . "   *  unused  0  0\n";
		$label .= " c: " . $c_size . "   0  unused  0  0\n\n";

		$fd = fopen("tmp/formatted.label", "w");
		if (!$fd) {
			printf("Error: cannot open label!\n");
			exit(1);
		}
		fwrite($fd, $label);
		fclose($fd);
		
		_exec("dd if=/dev/zero of=tmp/image.bin bs=1k count=$image_size");			
		_exec("mdconfig -a -t vnode -f tmp/image.bin -u 0");
		_exec("bsdlabel -BR -b /usr/obj/usr/src/sys/boot/i386/boot2/boot md0 tmp/formatted.label");
		
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
		
	// .iso
	} else if($platform == "generic-pc-cdrom") {

		_exec("mkdir tmp/cdroot");
		_exec("cp {$dirs['mfsroots']}/$platform-". basename($image_name) .".gz ".
			"tmp/cdroot/mfsroot.gz");
		_exec("cp /sys/i386/compile/$kernel/kernel.gz tmp/cdroot/kernel.gz");		

		_exec("mkdir tmp/cdroot/boot");
	    _exec("cp /usr/obj/usr/src/sys/boot/i386/cdboot/cdboot tmp/cdroot/boot/");		
	    _exec("cp /usr/obj/usr/src/sys/boot/i386/loader/loader tmp/cdroot/boot/");
		_exec("cp {$dirs['boot']}/$platform/loader.rc tmp/cdroot/boot/");
		_exec("cp /usr/obj/usr/src/sys/boot/i386/boot2/boot tmp/cdroot/boot/");

		_exec("mkisofs -b \"boot/cdboot\" -no-emul-boot -A \"m0n0wall CD-ROM image\" ".
			"-c \"boot/boot.catalog\" -d -r -publisher \"m0n0.ch\" ".
			"-p \"Your Name\" -V \"m0n0wall_cd\" -o \"m0n0wall.iso\" tmp/cdroot/");
			
		_exec("mv m0n0wall.iso {$dirs['images']}/cdrom-". basename($image_name) .".iso");
	}
	
	_exec("rm -rf tmp");
}

function release($name) {
	global $platforms, $dirs;
	
	// tar
	_exec("cd {$dirs['images']}; tar -cf rootfs-".basename($name).".tar ".basename($name));
	// zip
	_exec("cd {$dirs['images']}; gzip -9 rootfs-".basename($name).".tar");
	// rename
	_exec("mv {$dirs['images']}/rootfs-".basename($name).".tar.gz {$dirs['images']}/pbx-rootfs-".basename($name).".tgz");
	
	// signing & html generator
	$html = "\n";
	foreach($platforms as $platform) {
		$filename = "pbx-" . $platform . "-" . basename($name) . ".img";
		_exec("{$dirs['tools']}/sign ../sig/AskoziaPBX_private_key.pem {$dirs['images']}/$filename");
		
		$html .= "<tr>\n";
		$html .= "\t<td><a href=\"/downloads/$filename\">$platform</a></td>\n";
		$html .= "\t<td>". round(((filesize("{$dirs['images']}/$filename")/1024)/1024),1)." MB</td>\n";
		$html .= "\t<td>". md5_file("{$dirs['images']}/$filename")."</td>\n";
		$html .= "</tr>\n";
	}
	
	$filename = "pbx-rootfs-".basename($name).".tgz";
	
	$html .= "<tr>\n";
	$html .= "\t<td><a href=\"/downloads/$filename\">rootfs</a></td>\n";
	$html .= "\t<td>". round(((filesize("{$dirs['images']}/$filename")/1024)/1024),1)." MB</td>\n";
	$html .= "\t<td>". md5_file("{$dirs['images']}/$filename")."</td>\n";
	$html .= "</tr>\n";	
	$html .= "\n";

	print $html;
}

function _get_dir_size($dir) {
	exec("du -d 0 $dir", $out);
	$out = preg_split("/\s+/", $out[0]);
	return $out[0];
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

function _platform_to_kernel($platform) {
	global $platforms;
	
	if(array_search($platform, $platforms) === false) {
		_log("Platform doesn't exist!");
		exit(1);
	}
	
	if($platform == "generic-pc-cdrom" || $platform == "generic-pc") {
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
	// we're packaging all platforms go right ahead
	if ($argv[2] == "all") {
		foreach($platforms as $platform) {
			package($platform, $image_name);			
		}
	// check the specific platform before attempting to package
	} else if (in_array($argv[2], $platforms)) {
		package($argv[2], $image_name);
	// not a valid package command...
	} else {
		_log("Invalid packaging command!");
		exit(1);
	}

} else if ($argv[1] == "parsecheck") {

	passthru("find webgui/ -type f -name \"*.php\" -exec php -l {} \; -print | grep Parse");
	passthru("find webgui/ -type f -name \"*.inc\" -exec php -l {} \; -print | grep Parse");
	passthru("find phpconf/ -type f -name \"*rc.*\" -exec php -l {} \; -print | grep Parse");
	passthru("find phpconf/ -type f -name \"*.inc\" -exec php -l {} \; -print | grep Parse");

} else if ($argv[1] == "release") {

	$image_name = "{$dirs['images']}/" . rtrim($argv[2], "/");
	create($image_name);
	populate_everything($image_name);
	foreach($platforms as $platform) {
		package($platform, $image_name);			
	}
	release($image_name);
	
} else {
	_log("Huh?");
	exit(1);
}

exit();

?>
