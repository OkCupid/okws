#
# $Id$
#
#!/usr/bin/perl

use IO::File;
use strict;

sub usage {
    die "usage: $0 <file1.spl> <file2.spl> ....\n";
}

sub dofile {
    my ($file) = @_;
    unless ($file =~ m!(.*/)?([^/]+)\.spl!) {
	warn "$file: file type must be of suffix .spl\n";
	return;
    }
    my $dir = $1 ? $1 : "";
    my $fn = $2;
    my $infh = new IO::File ("<$file");
    if (!$infh) {
	warn "$file: cannot open\n";
	return;
    }
    my $i = 0;
    my @splts;
    my $ln;
    while (($ln = <$infh>)) {
	if ($ln =~ /<!--\s*#split\s*;?\s*-->/i) {
	    $splts[$i] .= $`;
	    $splts[++$i] = $'; #'
        } else {
	    $splts[$i] .= $ln;
	}
    }
    $infh->close ();
    for (my $j = 0; $j <= $i; $j++) {
	my $k = $j + 1;
	my $of = $dir . $fn . "-" . $k . ".html";
	my $outfh = new IO::File (">$of");
	if (!$outfh) {
	    warn "$of: cannot write file\n";
	    next;
	}
	print $outfh $splts[$j];
	$outfh->close ();
	print "wrote file: $of\n";
    }
}

if ($#ARGV < 0) {
    usage ();
}

foreach my $f (@ARGV) {
    dofile ($f);
}


