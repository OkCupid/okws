// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "okxml.h"
#include "abuf_pipe.h"
#include "okxmlparse.h"
#include "tame.h"

static void usage (void)
{
  warnx << "usage: " << progname << "\n";
  exit (1);
}

tamed static void
main_T ()
{
  tvars {
    abuf_pipe_t src (ain);
    xml_req_parser_t xrp (&src);
    zbuf out_z;
    strbuf out;
    int res;
    ptr<xml_element_t> top;
  }

  // XXX - workaround
  use_reference (src);

  xrp.init ();
  twait { xrp.parse (mkevent (res)); }
  if (res == 0) {
    top = xrp.top_level ();
    top->dump (out_z);
    out_z.to_strbuf (&out, false);
    out.tosuio ()->output (1);
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
