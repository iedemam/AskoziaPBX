#!/usr/bin/perl

# $Id: mkmini.pl 52 2006-02-07 14:35:14Z mkasper $

# arguments: source_tree dest_tree

use File::Copy;

exit unless $#ARGV == 2;

print "Populating MiniBSD tree: $ARGV[2]\n";

# populate_tree(treefile, srcpath, destpath)
sub populate_tree {
	my @args = @_;
	
	open TREEFILE, $args[0];
	
	TREE: while (<TREEFILE>) {
		
		next TREE if /^#/;
		next TREE if /^ *$/;
		
		@srcfiles = split(/:/);
		chomp @srcfiles;

		$srcfile = shift(@srcfiles);
		@srcstat = stat($args[1] . "/" . $srcfile);		

		if (copy($args[1] . "/" . $srcfile, $args[2] . "/" . $srcfile)) {
			printf "Copy $args[1]/$srcfile -> $args[2]/$srcfile ($srcstat[4]/$srcstat[5]/%04o)\n", ($srcstat[2] & 07777);
			chown $srcstat[4], $srcstat[5], $args[2] . "/" . $srcfile;
			chmod $srcstat[2] & 07777, $args[2] . "/" . $srcfile;
		} else {
			print "ERROR while copying file $args[1]/$srcfile\n";
		}

		foreach $lnfile (@srcfiles) {
			if (link($args[2] . "/" . $srcfile, $args[2] . "/" . $lnfile)) {
				print "Link $args[2]/$srcfile -> $args[2]/$lnfile\n";
			} else {
				print "ERROR while linking file $args[2]/$srcfile\n";
			}
		}
	}
}

populate_tree $ARGV[0], $ARGV[1], $ARGV[2];
