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

class oksrvc_simple_t : public oksrvc_t {
public:
  oksrvc_simple_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  okclnt_t *make_newclnt (ptr<ahttpcon> x);
  void post_launch_pub2 (cbb cb) { post_launch_pub2_T (cb); }
protected:
  void post_launch_pub2_T (cbb cb,  CLOSURE);
};

class okclnt_simple_t : public okclnt_t {
public:
  okclnt_simple_t (ptr<ahttpcon> x, oksrvc_simple_t *o)
    : okclnt_t (x, o), ok_simple (o) {}
  ~okclnt_simple_t () {}

  void process () { process_T (); }
  void process_T (CLOSURE); 

protected:
  oksrvc_simple_t *ok_simple;
};

TAMED void 
oksrvc_simple_t::post_launch_pub2_T (cbb cb)
{
  VARS {
    bool res1, res2;
  }
  BLOCK { 
    oksrvc_t::post_launch_pub2 (@(res1));
    pub2 ()->run_cfg ("conf/intl.conf", @(res2)); 
  }
  SIGNAL (cb, res1 && res2);
}

TAMED void 
okclnt_simple_t::process_T ()
{
  VARS {
    aarr_t aarr;
    bool rc (true);
    u_int opts (P_IINFO|P_VERBOSE);
    str file;
  }
  file = cgi["file"];

  if (file) {
    cgi.load_aarr (&aarr);

    BLOCK { pub2 ()->run (&out, file, @(rc), &aarr, opts); }

    if (!rc) {
      aarr.add ("target", file);
      file = "/fnf.html";
    }
  } else {
    aarr.add ("target", "&lt; <i>no <tt>file</tt> specified</i> &gt;");
    file = "/fnf.html";
    rc = false;
  }
  if (!rc) {
    BLOCK { pub2 ()->run (&out, file, @(rc), &aarr, opts|P_VISERR); }
  }
  output (out);
}

okclnt_t *
oksrvc_simple_t::make_newclnt (ptr<ahttpcon> x)
{
  return New okclnt_simple_t (x, this);
}

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_simple_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
