#!/usr/bin/perl

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
    my ($n, $typ, $name, $pfx) = @_;
    $name = "arr" unless $name;
    $pfx = "p" unless $pfx;
    my $r = "$typ *" .  $name . "[$n] = {" . 
	join (", ", map { "&" . $pfx . $_ } (0..$n-1)) .
	"};";
    return $r;
}

sub execute {
    my ($n, $hdr) = @_;
    my $s = "bool" . ($hdr ? " " : "\nmystmt_t::");
    $s .= "execute " . arglist ($n, "mybind_param_t");
    return ($s . ";") if $hdr;
    $s .= ("\n{\n" .
           "  MYSQL_BIND bnd[" . "$n];\n" .
	   "  " . make_arr ($n, "mybind_param_t") .  "\n" .
	   "  return execute (bnd, arr, $n);\n" .
	   "}\n");
    return $s;
}

sub fetch {
    my ($n, $hdr) = @_;
    my $s = "adb_status_t" . ($hdr ? " " : "\nmystmt_t::");
    $s .= "fetch " . arglist ($n, "mybind_res_t");
    return ($s . ";") if $hdr;
    $s .= ("\n{\n" .
           "  alloc_res_arr ($n);\n" .
	   join ("\n", map { "  res_arr[". "$_] = p$_;" } (0..$n-1)) ."\n" .
	   "  return fetch2 (true);\n" .
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
    my $stuff = headers ($i);

print <<EOF;

#ifndef _LIBAMYSQL_MYSTMT_H
#define _LIBAMYSQL_MYSTMT_H

#include "amysql.h"

class mystmt_t {
public:
  mystmt_t () : res_arr (NULL), errno_n (0) {}
  virtual ~mystmt_t () ;
  virtual adb_status_t fetch2 (bool bnd = false) = 0;
  str error () const { return err; }
  unsigned int errnum () const { return errno_n; }
  bool execute () { 
     return execute ((MYSQL_BIND *)NULL, (mybind_param_t **)NULL, 0);
  }
  virtual str get_last_qry () const { return NULL; }
protected:
  virtual bool execute (MYSQL_BIND *b, mybind_param_t **arr, u_int n) = 0;
  void alloc_res_arr (u_int n);
  void assign ();

  mybind_res_t *res_arr;
  u_int res_n;
  str err;
  unsigned int errno_n;

public:
$stuff
};

typedef ptr<mystmt_t> sth_t;

#endif /* _LIBMYSQL_MYSTMT_H */

EOF

}

sub c_file {
    my ($i) = @_;
    print qq!#include "mystmt.h"\n\n! . 
	codes ($i) . "\n\n";
}

my $N = 35;
if ($ARGV[0] eq "-h") {
    header_file ($N);
} elsif ($ARGV[0] eq '-c') {
    c_file ($N);
} else{
    die "usage: $0 [-h | -c]\n";
}


__END__

