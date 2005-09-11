#!/usr/bin/perl

#
# A file used to load in data for the pt1.g/pt1d.C/pt1_prot testing
# suite.  Populates a database with a series of numbers between 
# 1 and 1,000,000 along with their SHA-1 hashes.
#
# $Id$
#
use strict;
use DBI;
use IO::File;
use SHA;

sub usage ()
{
    die "usage: $0 [hostname]\n";
}

my $host = "127.0.0.1";

if ($#ARGV == 0) {
    $host = $ARGV[0];
} elsif ($#ARGV != -1) {
    usage ();
}

my $dbh = DBI->connect ("DBI:mysql:database=pt1;host=$host", "root", ""); 
die "Cannot connect to DB on host $host" unless $dbh;

my $sth = $dbh->prepare ("insert into sha1_tab (x,y) values (?,?)");
my $sha = new SHA;
my $h;
for (my $i = 0; $i < 1_000_000; $i++) {
    $h = $sha->hexhash ($i);
    $sth->execute ($i, $h);
    if ($i % 10_000 == 0) {
	print $i , "  -->  ", $h, "\n";
    }
}
$sth->finish ();
$dbh->disconnect ();

