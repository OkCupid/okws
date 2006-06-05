// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "okxml.h"
#include "abuf_pipe.h"
#include "okxmlparse.h"
#include "okxmlobj.h"
#include "tame.h"

int
main (int argc, char *argv[])
{
  zbuf z;
  strbuf b;
  xml_resp_t w;


  w[0][0] = "hi";
  w[0][1] = 3;
  w[0][2][0] = "bye";
  w[0][2][1] = 10;
  w[0][3] = "yo";
  w[1]("a") = "aa";
  w[1]("b") = "bb";
  w[2]("foo")("bar")("this")("that")[4] = 4;
  w[2]("foo")("biz")[5] = 10;

  xml_obj_const_t w2 (w);
  warn << "i=" << int (w2[2]("foo")("biz")[5]) << "\n";


  w.output (z);
  z << "-------------------------------\n";
  w = xml_fault_obj_t (10, "Error code #@#$ = 10");
  w.output (z);

  xml_resp_t w3;
  w3[0]("one") = w[0];
  w3[0]("this") = w[1];
  w3[1] = "that";
  w3[2] = false;
  z << "-------------------------------------\n";
  w3.output (z);

  z.to_strbuf (&b, false);
  b.tosuio ()->output (1);


}

