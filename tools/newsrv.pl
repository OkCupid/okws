#!/usr/bin/perl

use strict;
use IO::File;

sub new_server {
    my ($srv) = @_;
    my $of = $srv . ".g";
    my $fh = new IO::File (">$of");
    if (!$fh) {
	warn "** $of: cannot open\n";
	return;
    }
    my $st = $srv . "_t";
    print $fh <<EOF;
// -*-c++-*-
/* \$Id\$ */

#include "ok.h"
#include "okcgi.h"
#include "pub.h"
#include <unistd.h>

class oksrvc_$st : public oksrvc_t {
public:
  oksrvc_$st (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  okclnt_t *make_newclnt (ptr<ahttpcon> x);
  void init_publist () { /*o init_publist (); o*/ }
};

class okclnt_$st : public okclnt_t {
public:
  okclnt_$st (ptr<ahttpcon> x, oksrvc_$st *o) 
      : okclnt_t (x, o), ok_$srv (o) {}
  ~okclnt_$st () {}
  void process ()
  {
    output (out);
  }
  oksrvc_$st *ok_$srv;
};

okclnt_t *
oksrvc_${st}::make_newclnt (ptr<ahttpcon> x)
{ 
  return New okclnt_$st (x, this); 
}

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_$st (argc, argv);
  oksrvc->launch ();
  amain ();
}
EOF
    $fh->close ();
    print "wrote file: $of\n";
}

foreach my $srv (@ARGV) {
    new_server ($srv);
}
