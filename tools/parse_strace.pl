#!/usr/bin/perl

#
# Parse the output from strace to see which syscalls are taking which
# percent of the time.
# 
# $Id$
#
use strict;
my (%calls, %times);
my $tot = 0;
my @out;

while (<>) {
    if ( /^([a-z0-9_]+)\(.*?\).*<([.0-9]+)>/) {
	$calls{$1} ++;
	$times{$1} += $2;
	$tot += $2;
    }
}

foreach my $k (sort keys %calls) {
    my $pct = $times{$k} / $tot;
    push @out, [ $k, $pct ];
}

@out = sort { $b->[1] <=> $a->[1] } @out;
foreach my $o (@out) {
	print $o->[0], "\t", sprintf ("%0.2f", $o->[1] * 100) , "%\n";
}
