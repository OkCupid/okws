#!/usr/bin/env perl

#
# $Id$
#
# Copyright (C) 2002-2004 Maxwell Krohn (max@okcupid.com)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA
#
#


use strict;

sub arglist {
    my ($n, $typ, $r) = @_;
    return "(" . join (", ", map { $typ .  " " . ($r ? "&" : "" ) . "p$_" } 
		       (0..$n-1) ) . ")" ;
}

sub make_arr {
    my ($n, $typ, $lpfx, $name, $pfx) = @_;
    $name = "arr" unless $name;
    $pfx = "p" unless $pfx;
    $lpfx = "" unless $lpfx;
    return "${lpfx}vec<$typ> $name;\n" .
            "${lpfx}$name.reserve($n);\n${lpfx}" .
            join (" ", map { "$name.push_back($pfx$_);" } (0..$n-1)) ."\n";
}

sub execute {
    my ($n, $hdr) = @_;
    my $s = "bool" . ($hdr ? " " : "\nmystmt_t::");
    $s .= "execute " . arglist ($n, "mybind_param_t");
    return ($s . ";") if $hdr;
    $s .= ("\n{\n" .
           "  MYSQL_BIND bnd[" . "$n];\n" .
	   make_arr ($n, "mybind_param_t", "  ") .  "\n" .
	   "  return execute1 (bnd, arr);\n" .
	   "}\n");
    return $s;
}

sub fetch {
    my ($n, $hdr) = @_;
    my $s = "adb_status_t" . ($hdr ? " " : "\nmystmt_t::");
    $s .= "fetch " . arglist ($n, "mybind_res_t");
    return ($s . ";") if $hdr;
    $s .= ("\n{\n" .
       make_arr($n, "mybind_res_t", "  ") .
	   "  return fetch2 (arr, true);\n" .
	   "}\n");
    return $s;
}
    

sub headers {
    my ($m) = @_;
    return (join ("\n", map { "  " . fetch ($_,1) } (1..$m)) . "\n" .
	    join ("\n", map { "  " . execute ($_,1) } (1..$m)));
}

sub codes {
    my ($m) = @_;
    return (join ("\n", map { fetch ($_) } (1..$m)) . "\n" .
	    join ("\n", map { execute ($_) } (1..$m)));
}

sub header_file {
    my ($i) = @_;
    print headers ($i);
}

sub c_file {
    my ($i) = @_;
    print qq!#include "mystmt.h"\n\n! . 
    qq!#ifndef __GXX_EXPERIMENTAL_CXX0X__\n\n! .
	codes ($i) .
    qq!\n#endif\n!;
}

my $N = 120;
if ($ARGV[0] eq "-h") {
    header_file ($N);
} elsif ($ARGV[0] eq '-c') {
    c_file ($N);
} else{
    die "usage: $0 [-h | -c]\n";
}


__END__

