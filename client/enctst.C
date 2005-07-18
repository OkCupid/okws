
#include "okcgi.h"


int
main (int argc, char *argv[])
{
  if (argc != 2)
    exit (2);

  ok_filter_cgi = XSSFILT_NONE;

  str s1 = cgi_decode (argv[1]);
  str s2 = cgi_encode (s1);

  printf ("%s\n%s\n", s1.cstr (), s2.cstr ());
  if (s2 != argv[1]) {
	printf ("mismatch! BUG!!!\n");
	exit (2);
   }
}
