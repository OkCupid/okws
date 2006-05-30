// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "okxml.h"
#include "abuf_pipe.h"
#include "okxmlparse.h"
#include "okxmlwrap.h"
#include "tame.h"

int
main (int argc, char *argv[])
{
  zbuf z;
  strbuf b;
  ptr<xml_element_t> r (xml_method_response_t::alloc ());
  xml_wrap_t w (r);


  w[0][0] = "hi";
  w[0][1] = 3;
  w[0][2][0] = "bye";
  w[0][2][1] = 10;
  w[0][3] = "yo";

  /*
  xml_wrap_t w1 = w[3];
  w1 = 3;

  w[0]("a") = 10;
  w[0]("a")[40] = 10;
  w[0]("b") = "foo";
  w[1] = w[0];
  w[1]("a") = 12;
  w[2] = false;
  w[4][0] = "hi";
  w[4][1] = "bye";
  w[4][2] = "yo";
  */
  
  xml_const_wrap_t w2 (r);
  warn << "i=" << int (w2[0]("a")[40]) << "\n";

  r->dump (z);
  z.to_strbuf (&b, false);
  b.tosuio ()->output (1);

}

