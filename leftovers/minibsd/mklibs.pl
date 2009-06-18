#!/usr/bin/perl

# $Id: mklibs.pl 52 2006-02-07 14:35:14Z mkasper $

# arguments: binaries_tree

use File::Find;

exit unless $#ARGV == 0;

undef @liblist;

# check_libs(path)
sub check_libs {
	@filestat = stat($File::Find::name);
	
	if ((($filestat[2] & 0170000) == 0100000) &&
		($filestat[2] & 0111) && (!/.ko$/)) {

		@curlibs = qx{/usr/bin/ldd -f "%p\n" $File::Find::name 2>/dev/null};

		push(@liblist, @curlibs);
	}
}

# walk the directory tree
find(\&check_libs, $ARGV[0]);

# throw out dupes
undef %hlib;
@hlib{@liblist} = ();
@liblist = sort keys %hlib;

foreach $lib (@liblist) {
	$lib = substr($lib, 1);
}

print @liblist;

