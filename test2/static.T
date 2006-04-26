// -*-c++-*-
/* $Id$ */

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

class oksrvc_static_t : public oksrvc_t {
public:
  oksrvc_static_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  okclnt_t *make_newclnt (ptr<ahttpcon> x);
  void post_launch_pub2_T (cbb::ptr callercv,  CLOSURE);
  void post_launch_pub2 (cbb::ptr callercv)
  { post_launch_pub2_T (callercv); }
};

class okclnt_static_t : public okclnt_t {
public:
  okclnt_static_t (ptr<ahttpcon> x, oksrvc_static_t *o)
    : okclnt_t (x, o), ok_static (o) {}
  ~okclnt_static_t () {}

  void process () { process_T (); }
  void process_T (CLOSURE); 

protected:
  oksrvc_static_t *ok_static;
};

TAMED void 
oksrvc_static_t::post_launch_pub2_T (cbb::ptr callercv)
{
  VARS {
    bool res1, res2;
  }
  BLOCK { 
    oksrvc_t::post_launch_pub2 (@(res1));
    pub2 ()->run_cfg ("conf/intl.conf", @(res2)); 
  }
  SIGNAL (res1 && res2);
}

TAMED void 
okclnt_static_t::process_T ()
{
  VARS {
    str ofn;
    aarr_t aarr;
    str in;
    bool rc (true);
    u_int opts (P_IINFO|P_VERBOSE);
  }
  static rxx x ("^(:\\d+)?/static(/.+)$");
  in = hdr.get_target ();

  if (x.match (in)) {
    ofn = x[2];
    assert (ofn);
    cgi.load_aarr (&aarr);

    BLOCK { pub2 ()->run (&out, ofn, @(rc), &aarr, opts); }
    assert (ofn);

    if (!rc) {
      aarr.add ("target", ofn);
      ofn = "/fnf.html";
    }
  } else {
    aarr.add ("target", "&lt; <i>no file specified</i> &gt;");
    ofn = "/fnf.html";
    rc = false;
  }
  if (!rc) {
    BLOCK { pub2 ()->run (&out, ofn, @(rc), &aarr, opts|P_VISERR); }
  }
  output (out);
}

okclnt_t *
oksrvc_static_t::make_newclnt (ptr<ahttpcon> x)
{
  return New okclnt_static_t (x, this);
}

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_static_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
