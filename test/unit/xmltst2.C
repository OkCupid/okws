// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "okxml.h"
#include "abuf_pipe.h"
#include "okxmlparse.h"
#include "tame.h"

int
main (int argc, char *argv[])
{
  zbuf z;
  strbuf b;
  xml_wrapper_t r (xml_method_response_t::alloc ());

  r[0]["a"] = 10;
  r[0]["b"] = "foo";
  r[1] = r[0];
  r[1]["a"] = 12;
  r[2] = false;

  r.dump (z);
  z.to_strbuf (b, false);
  b.tosuio ()->output (1);

}

