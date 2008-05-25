// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "okxml.h"
#include "abuf_pipe.h"
#include "okxmlparse.h"
#include "okxmlgeneric.h"
#include "okxmlobj.h"
#include "tame.h"

static void usage (void)
{
  warnx << "usage: " << progname << "\n";
  exit (1);
}

static void
explore (ptr<const xml_generic_t> top)
{
  xml_gobj_const_t o (top);
  zbuf zb;
  o.dump (zb);
  strbuf b;
  zb.to_strbuf (&b, false);
  warn << b ;
}

tamed static void
main_T ()
{
  tvars {
    holdvar abuf_pipe_t src (ain);
    xml_req_parser_generic_t xrp (&src);
    int res;
    ptr<const xml_generic_t> top;
  }

  xrp.init ();
  twait { xrp.parse (mkevent (res)); }
  if (res == HTTP_OK) {
    top = xrp.top_level_g ();
    explore (top);
  } else {
    assert (xrp.errcode () != XML_PARSE_OK);
    warn << "XML parse error: " << xrp.errmsg () << "\n";
  }
  exit (0);
}

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  if (argc != 1) 
    usage ();

  main_T ();
  amain ();

}
