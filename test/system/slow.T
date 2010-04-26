// -*-c++-*-
/* $Id: slow.T 5166 2010-03-18 01:59:50Z max $ */

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
#include "mimetypes.h"
#include "oklocale.h"
#include "okws_sfs.h"
#include "test_const.h"
#include "okclib.h"

//=======================================================================

class oksrvc_slow_t : public oksrvc_t {
public:
  oksrvc_slow_t (int argc, char *argv[]) 
    : oksrvc_t (argc, argv), _ready (false) {}

  newclnt_t *make_newclnt (ptr<ahttpcon> x);
  void post_launch_pub_T (evb_t ev, CLOSURE);
  void post_launch_pub (evb_t ev) { post_launch_pub_T (ev); }
  bool is_ready () const { return _ready; }
private:
  bool _ready;
};

//=======================================================================

class okclnt_slow_t : public okclnt2_t {
public:
  okclnt_slow_t (ptr<ahttpcon> x, oksrvc_slow_t *o)
    : okclnt2_t (x, o), ok_slow (o) {}
  ~okclnt_slow_t () {}

  void process (proc_ev_t ev) { process_T (ev); }
  void process_T (proc_ev_t ev, CLOSURE); 

protected:
  oksrvc_slow_t *ok_slow;
};

//-----------------------------------------------------------------------

tamed void 
oksrvc_slow_t::post_launch_pub_T (evb_t ev)
{
  tvars {
    time_t t (1);
    bool ok;
  }
  twait { 
    oksrvc_t::post_launch_pub (mkevent (ok));
  }
  warn << "+ starting " << t << "s wait..\n";
  twait { delaycb (t, 0, mkevent ()); }
  warn << "- the wait is over\n";
  _ready = true;
  ev->trigger (ok);
}

//-----------------------------------------------------------------------

tamed void 
okclnt_slow_t::process_T (okclnt2_t::proc_ev_t ev)
{
  out << "ready=" << ok_slow->is_ready ();
  twait { output (out, mkevent ()); }
  ev->trigger (true, HTTP_OK);
}

//-----------------------------------------------------------------------

oksrvc_t::newclnt_t *
oksrvc_slow_t::make_newclnt (ptr<ahttpcon> x)
{
  return New okclnt_slow_t (x, this);
}

//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_slow_t (argc, argv);
  oksrvc->launch ();
  amain ();
}

//-----------------------------------------------------------------------
