#!/usr/local/bin/php -f
<?php
/*
    $Id$
    
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

// --[ package versions ]------------------------------------------------------

$php_version		= "php-4.4.6";
$mini_httpd_version	= "mini_httpd-1.19";
$asterisk_version	= "asterisk-1.4.4";
$zaptel_version		= "zaptel";


// --[ image sizes ]-----------------------------------------------------------

$mfsroot_size 			= 40 * 1024;
$generic_pc_size 		= 25 * 1024;
$generic_pc_smp_size 	= 25 * 1024;
$wrap_soekris_size 		= 25 * 1024;


// --[ possible platforms and kernels ]----------------------------------------

$platform_list = "net45xx net48xx wrap generic-pc generic-pc-cdrom";
$platforms = explode(" ", $platform_list);


// --[ sanity checks and env info ]--------------------------------------------

$dirs['pwd']			= rtrim(shell_exec("pwd"), "\n");
$dirs['boot']			= $dirs['pwd'] . "/build/boot";
$dirs['kernelconfigs']	= $dirs['pwd'] . "/build/kernelconfigs";
$dirs['minibsd']		= $dirs['pwd'] . "/build/minibsd";
$dirs['patches']		= $dirs['pwd'] . "/build/patches";
$dirs['tools']			= $dirs['pwd'] . "/build/tools";
$dirs['etc']			= $dirs['pwd'] . "/etc";
$dirs['phpconf']		= $dirs['pwd'] . "/phpconf";
$dirs['webgui']			= $dirs['pwd'] . "/webgui";
$dirs['files']			= $dirs['pwd'] . "/files";

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
$dirs['mfsroots']	= $dirs['pwd'] . "/work/mfsroots";

foreach($dirs as $dir) {
	if (!file_exists($dir)) {
		mkdir($dir);
	}	
}

// --[ the functions! ]--------------------------------------------------------

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
	global $dirs, $php_version, $radius_version;

	if (!file_exists("/usr/local/bin/autoconf")) {
		_exec("cd /usr/ports/devel/autoconf213; make install clean");
		_exec("ln -s /usr/local/bin/autoconf213 /usr/local/bin/autoconf");
		_exec("ln -s /usr/local/bin/autoheader213 /usr/local/bin/autoheader");
	}
	
	if (!file_exists("{$dirs['packages']}/$php_version")) {
		if (!file_exists("{$dirs['packages']}/$php_version.tar.gz")) {
			_exec("cd {$dirs['packages']}; " .
					"fetch http://br.php.net/distributions/$php_version.tar.gz");
		}
		_exec("cd {$dirs['packages']}; tar zxf $php_version.tar.gz");
	}
	_exec("cd {$dirs['packages']}/$php_version; ".
			"rm configure; ".
			"./buildconf --force; ".
			"./configure --without-mysql --with-openssl --enable-discard-path --enable-sockets --enable-bcmath; ".
			"make");
}

function build_minihttpd() {
	global $dirs, $mini_httpd_version;
	
	if (!file_exists("{$dirs['packages']}/$mini_httpd_version")) {
		if (!file_exists("{$dirs['packages']}/$mini_httpd_version.tar.gz")) {
			_exec("cd {$dirs['packages']}; ".
				"fetch http://www.acme.com/software/mini_httpd/$mini_httpd_version.tar.gz");
		}
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
	
	if (!file_exists("{$dirs['packages']}/$asterisk_version")) {
		if (!file_exists("{$dirs['packages']}/$asterisk_version.tar.gz")) {
			_exec("cd {$dirs['packages']}; ".
				"fetch http://ftp.digium.com/pub/asterisk/releases/$asterisk_version.tar.gz; ");
		}
		_exec("cd {$dirs['packages']}; tar zxf $asterisk_version.tar.gz");
	}
	if (!_is_patched($asterisk_version)) {
		_exec("cd {$dirs['packages']}/$asterisk_version; ".
			"patch < {$dirs['patches']}/packages/asterisk_makefile.patch");
		_exec("cd {$dirs['packages']}; ".
			"patch < {$dirs['patches']}/packages/asterisk_cdr_to_syslog.patch");				
		_stamp_package_as_patched($asterisk_version);
	}
	// copy make options
	_exec("cp {$dirs['patches']}/packages/menuselect.makeopts /etc/asterisk.makeopts");
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
	
	_exec("cd {$dirs['packages']}/$zaptel_version; make clean");
	_exec("cd {$dirs['packages']}/$zaptel_version; make");

	if (!file_exists("{$dirs['packages']}/$zaptel_version/STAGE")) {
		_exec("cd {$dirs['packages']}/$zaptel_version/; ".
			"mkdir STAGE STAGE/bin STAGE/lib STAGE/etc");
	}
	
	_exec("cd {$dirs['packages']}/$zaptel_version; ".
		"make install PREFIX={$dirs['packages']}/$zaptel_version/STAGE");
}

function build_msntp() {
	
	_exec("cd /usr/ports/net/msntp; make clean; make");
}

function build_tools() {
	global $dirs;
	
	_exec("cd {$dirs['tools']}; gcc -o stats.cgi stats.c");
	_exec("cd {$dirs['tools']}; gcc -o verifysig -lcrypto verifysig.c");
	_exec("cd {$dirs['tools']}; gcc -o wrapresetbtn wrapresetbtn.c");
	/*_exec("cd ". $dirs['tools'] ."; gcc -o minicron minicron.c");*/
}

function build_bootloader() {
	
	_exec("cd /sys/boot; make clean; make obj; make");
}

function build_packages() {

	build_php();
	build_minihttpd();
	build_zaptel();
	build_asterisk();
}

function build_ports() {
	
	build_msntp();
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
		
	_exec("mkdir $image_name");
	$rootfs = "$image_name/rootfs";
	_exec("mkdir $rootfs");
	
	_exec("cd $rootfs; mkdir lib bin cf conf.default dev etc ftmp mnt libexec proc root sbin tmp usr var");
	_exec("cd $rootfs; ln -s /cf/conf conf");
	_exec("mkdir $rootfs/etc/inc");
	_exec("cd $rootfs/usr; mkdir bin lib libexec local sbin share");
	_exec("cd $rootfs/usr/local; mkdir bin lib sbin www etc");
	_exec("mkdir $rootfs/usr/local/etc/asterisk");
	_exec("cd $rootfs/usr/local; ln -s /var/run/htpasswd www/.htpasswd");
	
	_exec("mkdir $image_name/asterisk");
	_exec("mkdir $image_name/asterisk/moh");
	_exec("mkdir $image_name/asterisk/sounds");
	_exec("mkdir $image_name/asterisk/sounds/silence");
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
	
	_exec("cp {$dirs['phpconf']}/config.xml $image_name/rootfs/conf.default/");
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
	global $dirs;
	
	_exec("cd /usr/ports/net/msntp; ".
		"install -s work/msntp-*/msntp $image_name/rootfs/usr/local/bin");
}

function populate_asterisk($image_name) {
	global $dirs, $asterisk_version;
	
	$rootfs = "$image_name/rootfs";
	
	_exec("cd {$dirs['packages']}/$asterisk_version/; ".
		"gmake install DESTDIR=$rootfs");
	
	// filter and link sounds
	$sounds = explode(" ", "conf-* vm-rec-name.* beep.* auth-thankyou.*");
	foreach ($sounds as $sound) {
		_exec("cp $rootfs/usr/local/share/asterisk/sounds/$sound ".
			"$image_name/asterisk/sounds");
	}
	_exec("cp $rootfs/usr/local/share/asterisk/sounds/silence/*.gsm ".
		"$image_name/asterisk/sounds/silence");
	_exec("rm -rf $rootfs/usr/local/share/asterisk/sounds");
	_exec("cd $rootfs/usr/local/share/asterisk; ln -s /asterisk/sounds sounds");
	
	// filter music on hold
	$musiconhold = explode(" ", "fpm-calm-river.gsm fpm-calm-river.ulaw");
	foreach ($musiconhold as $moh) {
		_exec("cp $rootfs/usr/local/share/asterisk/moh/$moh $image_name/asterisk/moh");
	}
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

function populate_zaptel($image_name) {
	global $dirs, $zaptel_version;
	
	_exec("cp {$dirs['packages']}/$zaptel_version/STAGE/etc/zaptel.conf.sample ".
		"$image_name/rootfs/etc/zaptel.conf");
}

function populate_tools($image_name) {
	global $dirs;
	
	$rootfs = "$image_name/rootfs";
	
	_exec("cd {$dirs['tools']}; ".
		"install -s stats.cgi $rootfs/usr/local/www; ".
		/*"install -s minicron $rootfs/usr/local/bin; ".*/
		"install -s verifysig $rootfs/usr/local/bin; ".
		"install -s wrapresetbtn $rootfs/usr/local/sbin; ".		
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
	populate_zaptel($image_name);
	populate_asterisk($image_name);
	populate_tools($image_name);
	populate_phpconf($image_name);
	populate_webgui($image_name);
	populate_libs($image_name);
}

function package($platform, $image_name) {
	global $dirs, $mfsroot_size, $generic_pc_size, $generic_pc_smp_size, $wrap_soekris_size, $zaptel_version, $asterisk_version;
		
	_set_permissions($image_name);
	
	if (!file_exists("tmp")) {
		mkdir("tmp");
		mkdir("tmp/mnt");
	}
	
	$kernel = _platform_to_kernel($platform);
	
	// mfsroots
	if (!file_exists("{$dirs['mfsroots']}/$platform-". basename($image_name) .".gz")) {

		_exec("dd if=/dev/zero of=tmp/mfsroot bs=1k count=$mfsroot_size");
		_exec("mdconfig -a -t vnode -f tmp/mfsroot -u 0");
	
		_exec("bsdlabel -rw md0 auto");
		_exec("newfs -O 1 -b 8192 -f 1024 -o space -m 0 /dev/md0c");
	
		_exec("mount /dev/md0c tmp/mnt");
		_exec("cd tmp/mnt; tar -cf - -C $image_name ./ | tar -xpf -");
		
		// system modules		
		_exec("mkdir tmp/mnt/boot");
		_exec("mkdir tmp/mnt/boot/kernel");
		if ($platform == "generic-pc" || 
			$platform == "generic-pc-cdrom") {
			_exec("cp /sys/i386/compile/$kernel/modules/usr/src/sys/modules/acpi/acpi/acpi.ko tmp/mnt/boot/kernel/");
		}
		
		// zaptel modules
		_exec("cp {$dirs['packages']}/$zaptel_version/zaptel/zaptel.ko tmp/mnt/boot/kernel/");
		_exec("cp {$dirs['packages']}/$zaptel_version/ztdummy/ztdummy.ko tmp/mnt/boot/kernel/");
		
		// stamps
		_exec("echo \"". basename($image_name) ."\" > tmp/mnt/etc/version");
		_exec("echo `date` > tmp/mnt/etc/version.buildtime");
		_exec("echo $platform > tmp/mnt/etc/platform");
		
		_exec("df");
	
		_exec("umount tmp/mnt");
		_exec("mdconfig -d -u 0");
		_exec("gzip -9 tmp/mfsroot");
		_exec("mv tmp/mfsroot.gz {$dirs['mfsroots']}/$platform-". basename($image_name) .".gz");
	}
	
	// .img
	if ($platform != "generic-pc-cdrom") {
		
		if ($platform == "generic-pc") {
			_exec("dd if=/dev/zero of=tmp/image.bin bs=1k count=$generic_pc_size");
		} else {
			_exec("dd if=/dev/zero of=tmp/image.bin bs=1k count=$wrap_soekris_size");
		}
			
		_exec("mdconfig -a -t vnode -f tmp/image.bin -u 0");
		_exec("bsdlabel -Brw -b /usr/obj/usr/src/sys/boot/i386/boot2/boot md0 auto");
		_exec("newfs -O 1 -b 8192 -f 1024 -o space -m 0 /dev/md0a");
		_exec("mount /dev/md0a tmp/mnt");
		_exec("cp {$dirs['mfsroots']}/$platform-". basename($image_name) .".gz ".
			"tmp/mnt/mfsroot.gz");
		
		// boot
		_exec("mkdir tmp/mnt/boot");
		_exec("mkdir tmp/mnt/boot/kernel");
	    _exec("cp /usr/obj/usr/src/sys/boot/i386/loader/loader tmp/mnt/boot/");
		_exec("cp {$dirs['boot']}/$platform/loader.rc tmp/mnt/boot/");
	
		// conf
		_exec("mkdir tmp/mnt/conf");
		_exec("cp {$dirs['phpconf']}/config.xml tmp/mnt/conf");
		_exec("cp /sys/i386/compile/$kernel/kernel.gz tmp/mnt/kernel.gz");
		
		// asterisk modules
		_exec("mkdir tmp/mnt/asterisk");
		_exec("mkdir tmp/mnt/asterisk/modules");
		_exec("cp {$dirs['packages']}/$asterisk_version/TMP_MODULES/* tmp/mnt/asterisk/modules");

		// cleanup
		_exec("df");
		_exec("umount tmp/mnt");
		_exec("mdconfig -d -u 0");
		_exec("gzip -9 tmp/image.bin");
		_exec("mv tmp/image.bin.gz {$dirs['images']}/$platform-". basename($image_name) .".img");
		
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

function _set_permissions($image_name) {
	
	_exec("chmod 755 $image_name/rootfs/etc/rc*");
	_exec("chmod 644 $image_name/rootfs/etc/pubkey.pem");
	
	_exec("chmod 644 $image_name/rootfs/etc/inc/*");
	
	_exec("chmod 644 $image_name/rootfs/conf.default/config.xml");

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

if ($argv[1] == "new") {

	$image_name = "{$dirs['images']}/" . rtrim($argv[2], "/");

	if (file_exists($image_name)) {
		_exec("rm -rf $image_name");
		_exec("rm -rf {$dirs['images']}/* {$argv[2]}.img");
		_exec("rm -rf {$dirs['mfsroots']}/* {$argv[2]}.gz");
	}
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

} else {
	_log("Huh?");
	exit(1);
}

exit();

?>
