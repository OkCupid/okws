
// -*-c++-*-
/* $Id$ */

#include "ok.h"
#include "cgi.h"
#include "pub.h"
#include <unistd.h>

class okclnt_form2a_t : public okclnt_t {
public:
  okclnt_form2a_t (ptr<ahttpcon> x, oksrvc_t *o) : okclnt_t (x, o) {}
  ~okclnt_form2a_t () {}
  static okclnt_t *alloc (ptr<ahttpcon> x, oksrvc_t *o)
  { return New okclnt_form2a_t (x, o); }
  void process ()
  {
    /*<pub>
      ct_include (out, "/home/am0/max/web/a.html");
    </pub>*/
    output (out);
  }
};

int
main (int argc, char *argv[])
{
  oksrvc_t *okc = New oksrvcw_t (argc, argv, wrap (&okclnt_form2a_t::alloc));
  okc->launch ();
  amain ();
}

