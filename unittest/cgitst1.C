
/* $Id$ */

#include "okcgi.h"

void
usage ()
{
  fatal << "usage: cgitst <cgistr>\n";
}

int
main (int argc, char *argv[])
{
  if (argc != 2) 
    usage ();
  ptr<cgi_t> t (cgi_t::str_parse (argv[1]));
  t->dump1 ();
}
