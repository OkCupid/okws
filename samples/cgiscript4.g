// -*-c++-*-
/* $Id$ */

#include "ok.h"
#include "cgi.h"
#include "pub.h"
#include <unistd.h>

class okc_cgiscript4_t : public oksrvc_t {
public:
  okc_cgiscript4_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  okclnt_t *make_newclnt (ptr<ahttpcon> x);
  void init_publist () { /*<pub> init_publist (); </pub>*/ }
};

class okclnt_cgiscript4_t : public okclnt_t {
public:
  okclnt_cgiscript4_t (ptr<ahttpcon> x, okc_cgiscript4_t *o) 
      : okclnt_t (x, o), ok_cgiscript4 (o) {}
  ~okclnt_cgiscript4_t () {}
  void process ()
  {
    /*o
      include (pub, out, "/cgiscript4.html");
      o*/
    output (out);
  }
  okc_cgiscript4_t *ok_cgiscript4;
};

okclnt_t *
okc_cgiscript4_t::make_newclnt (ptr<ahttpcon> x)
{ 
  return New okclnt_cgiscript4_t (x, this); 
}

int
main (int argc, char *argv[])
{
  oksrvc_t *okc = New okc_cgiscript4_t (argc, argv);
  okc->launch ();
  amain ();
}
