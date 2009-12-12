// -*-c++-*-
/* $Id: purify.T 3190 2008-02-05 15:10:03Z max $ */

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
#include "ok_adebug.h"
#include "test_const.h"
#include "okwcxml.h"
#include "tame_connectors.h"
#include "purify_lib.h"

class oksrvc_purify_t : public oksrvc_t {
public:
  oksrvc_purify_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  newclnt_t *make_newclnt (ptr<ahttpcon> x);
  void post_launch_pub (evb_t ev) { post_launch_pub_T (ev); }
  purify::purifier_t *purifier () { return &_purifier; }
protected:
  void post_launch_pub_T (evb_t ev,  CLOSURE);
  purify::purifier_t _purifier;
};

class okclnt_purify_t : public okclnt_t {
public:
  okclnt_purify_t (ptr<ahttpcon> x, oksrvc_purify_t *o)
    : okclnt_t (x, o), ok_purify (o) {}
  ~okclnt_purify_t () {}

  void process () { process_T (); }
  void process_T (CLOSURE); 

protected:
  oksrvc_purify_t *ok_purify;
};


tamed void 
oksrvc_purify_t::post_launch_pub_T (evb_t ev)
{
  tvars {
    bool res1, res2;
  }
  twait { 
    oksrvc_t::post_launch_pub (mkevent (res1));
    _purifier.init (pub3 (), "/conf/purify.conf", mkevent (res2));
  }
  if (!res2) {
    warn << "purifier failed to initialize\n";
  }
  ev->trigger (res1 && res2);
}


tamed void 
okclnt_purify_t::process_T ()
{
  tvars {
    ptr<pub3::dict_t> d (pub3::dict_t::alloc ());
    bool rc (true);
    u_int opts (pub3::P_IINFO|pub3::P_VERBOSE);
    str file ("/test/purify.html");
    str dirtyhtml;
    str outhtml;
    purify::status_t status;
  }

  dirtyhtml = cgi["dirtyhtml"];
  if (cgi.blookup ("doit")) {
    outhtml = "<N/A> -- Failure!";
    twait {
      ok_purify->purifier ()->purify (dirtyhtml, mkevent (status, outhtml));
    }
  } else {
    outhtml = dirtyhtml;
  }
  d->insert ("out", outhtml);

  twait { pub3 ()->run (&out, file, mkevent (rc), d, opts|pub3::P_VISERR); }
  output (out);
}

oksrvc_t::newclnt_t *
oksrvc_purify_t::make_newclnt (ptr<ahttpcon> x)
{
  return New okclnt_purify_t (x, this);
}

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_purify_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
