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
  w[1]("a") = "aa";
  w[1]("b") = "bb";
  w[2]("foo")("bar")("this")("that")[4] = 4;
  w[2]("foo")("biz")[5] = 10;

  xml_const_wrap_t w2 (r);
  xml_const_wrap_t w3 = w2[2];
  xml_const_wrap_t w4 = w3("foo");
  xml_const_wrap_t w5 = w4("biz");
  xml_const_wrap_t w6 = w5[5];

  warn << "i=" << int (w6) << "\n";
  warn << "i=" << int (w2[2]("foo")("biz")[5]) << "\n";

  r->dump (z);
  z.to_strbuf (&b, false);
  b.tosuio ()->output (1);

}

