
// -*-c++-*-
/* $Id$ */

#include "ok.h"
#include "cgi.h"
#include "pub.h"
#include <unistd.h>

class okclnt_form1a_t : public okclnt_t {
public:
  okclnt_form1a_t (ptr<ahttpcon> x, oksrvc_t *o) : okclnt_t (x, o) {}
  ~okclnt_form1a_t () {}
  static okclnt_t *alloc (ptr<ahttpcon> x, oksrvc_t *o)
  { return New okclnt_form1a_t (x, o); }
  void process ()
  {
    /*o
      print (out) <<EOF;
<html>
 <head>
   <title>Simple Web Page</title>
 </head>
 <body bgcolor=white>
 <h1>Very Simple Web Page</h1>
 And here is some text on this very simple web page.
 Here are some variable resolutions:<p>
 <tt>a</tt> &nbsp; &nbsp; @{cgi["a"]}<br>
 <tt>c</tt> &nbsp; &nbsp; @{cgi["c"]}<br>
 </body>
</html>
EOF
    o*/
    output (out);
  }
};

int
main (int argc, char *argv[])
{
  oksrvc_t *okc = New oksrvcw_t (argc, argv, wrap (&okclnt_form1a_t::alloc));
  okc->launch ();
  amain ();
}

