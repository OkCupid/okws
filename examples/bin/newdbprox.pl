#!/usr/bin/perl

use strict;
use IO::File;

sub new_dbprx {
    my ($db) = @_;
    my $of = $db . ".C";
    my $fh = new IO::File (">$of");
    if (!$fh) {
	warn "** $of: cannot open\n";
	return;
    }
    my $dbprx = $db . "_dbprox_t";
    my $port = uc ($db) . "_PORT";
    print $fh <<EOF;
// -*-c++-*-
/* \$Id\$ */

#include "${db}_prot.h"
#include "pslave.h"

#include "acgiconf.h"
#include "amysql.h"
#include "mystmt.h"
#include "web.h"

class $dbprx : public amysql_thread_t {
public:
  $dbprx (mtd_thread_arg_t *a) : amysql_thread_t (a), err (false) {}
  void dispatch (svccb *sbp);
  bool init ();
  static mtd_thread_t *alloc (mtd_thread_arg_t *arg) 
  { return New $dbprx (arg); }
protected:
private:
  bool err;
};

bool
${dbprx}::init ()
{
  return true;
}

void
${dbprx}::dispatch (svccb *sbp)
{
  u_int p = sbp->proc ();
  switch (p) {
  default:
    reject ();
  }
}

int
main (int argc, char *argv[])
{
  ssrv_t *s = New ssrv_t (wrap (&${dbprx}::alloc), ${db}_prog_1, 
                          MTD_PTH, 3);
  if (!pub_server (wrap (s, &ssrv_t::accept), $port))
    fatal << "Cannot bind to port " << $port << "\\n";
  amain ();
}
EOF
    $fh->close ();
    print "wrote file: $of\n";
}

foreach my $srv (@ARGV) {
    new_dbprx ($srv);
}
