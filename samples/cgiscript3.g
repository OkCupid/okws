
// -*-c++-*-
/* $Id$ */

#include "ok.h"
#include "cgi.h"
#include "pub.h"
#include <unistd.h>

class okclnt_form3a_t : public okclnt_t {
public:
  okclnt_form3a_t (ptr<ahttpcon> x, oksrvc_t *o) : okclnt_t (x, o) {}
  ~okclnt_form3a_t () {}
  void process ()
  {
    /*o
      include (pub, out, "/home/u1/max/web/3a.html", 
               { "X" => @{cgi["a"]}, "Y" => @{cgi["b"]} });
    o*/
    output (out);
  }
};

class okc_form3a_t : public oksrvc_t {
public:
  okc_form3a_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  okclnt_t *make_newclnt (ptr<ahttpcon> x)
  { return New okclnt_form3a_t (x, this); }
  void init_publist () { /*<pub> init_publist (); </pub>*/ }
};


int
main (int argc, char *argv[])
{
  oksrvc_t *okc = New okc_form3a_t (argc, argv);
  okc->launch ();
  amain ();
}

