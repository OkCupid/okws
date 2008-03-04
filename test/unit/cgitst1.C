
/* $Id$ */

#include "okcgi.h"
#include "pubutil.h"

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

  str s1 =  "abcd\n\n\n\n\n\n\n";
  str s2 =  "abcd\n";
  str s3 =  "abcd";
  fix_trailing_newlines (&s1);
  fix_trailing_newlines (&s2);
  fix_trailing_newlines (&s3);

  warn << s1 << s2 << s3 ;


  ptr<cgi_t> t (cgi_t::str_parse (argv[1]));
  t->dump1 ();
}
