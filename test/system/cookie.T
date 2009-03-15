// -*-c++-*-
/* $Id: cookie.T 1990 2006-06-13 12:53:36Z max $ */

/*
 *
 * Copyright (C) 2003-4 by Maxwell Krohn (max@okcupid.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#include "ok.h"
#include "okcgi.h"
#include "pub.h"
#include <unistd.h>
#include "tame.h"
#include "rxx.h"

class oksrvc_cookie_t : public oksrvc_t {
public:
  oksrvc_cookie_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  newclnt_t *make_newclnt (ptr<ahttpcon> x);
};

class okclnt_cookie_t : public okclnt2_t {
public:
  okclnt_cookie_t (ptr<ahttpcon> x, oksrvc_cookie_t *o)
    : okclnt2_t (x, o), ok_cookie (o) {}
  ~okclnt_cookie_t () {}

  void process (proc_ev_t ev) { process_T (ev); }
  void process_T (proc_ev_t ev, CLOSURE); 

protected:
  oksrvc_cookie_t *ok_cookie;
};

tamed void 
okclnt_cookie_t::process_T (okclnt2_t::proc_ev_t ev)
{
  tvars {
    bool do_redirect (false);
  }

  if (cgi.blookup ("redirect")) {
    do_redirect = true;
  }

  {
    cookie_t *c[3];
    c[0] = add_cookie ("foo.com", "/bar");
    c[1] = add_cookie ("sub.foo.com", "/");
    c[2] = add_cookie ("", "/");
    c[0]->add ("y", 10).add ("x", "XxX").set_expires (1,0,0,0);
    c[1]->add ("zz", 133).add("ww", "WwW");
    c[2]->add ("x", 10).add ("y", "XxX").set_expires (1,0,0,0);
  }

  {
    const char *cookies[] = { "x", "y", NULL };
    for (const char **cp = cookies; *cp; cp++) {
      if (cookie ().exists (*cp)) {
	out << *cp << " -> " << cookie()[*cp] << "<br>\n";
      }
    }
  }

  out << "body\n";

  if (do_redirect) {
    twait { redirect ("http://www.someurl.com/fff", -1, mkevent ()); }
  } else {
    twait { output (out, mkevent ()); }
  }
  ev->trigger (true, HTTP_OK);
}

oksrvc_t::newclnt_t *
oksrvc_cookie_t::make_newclnt (ptr<ahttpcon> x)
{
  return New okclnt_cookie_t (x, this);
}

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_cookie_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
