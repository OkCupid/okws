// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "okxml.h"
#include "abuf_pipe.h"
#include "okxmlparse.h"
#include "tame.h"
#include "rxx.h"
#include "okwcxml.h"
#include "okxmlobj.h"
#include "tame_connectors.h"

tamed static void
try_parse (str s, evv_t ev)
{
  tvars {
    holdvar abuf_str_t buf (s);
    xml_req_parser_t parser (&buf);
    int rc;
  }

  parser.init ();
  twait { parser.parse (mkevent (rc)); }
  if (rc == HTTP_OK) {
    xml_obj_const_t obj (parser.top_level ()->get (0)); 
    int i = obj("foo")[3][1];
	i++;
  }

}

tamed static void
main2 (int argc, char **argv)
{
  twait { try_parse (argv[1], mkevent ()); }
}


int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  main2 (argc, argv);
  amain ();
}
