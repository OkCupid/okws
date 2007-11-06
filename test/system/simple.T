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
#include "ok_adebug.h"
#include "test_const.h"

class oksrvc_simple_t : public oksrvc_t {
public:
  oksrvc_simple_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  newclnt_t *make_newclnt (ptr<ahttpcon> x);
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

tamed void 
oksrvc_simple_t::post_launch_pub2_T (cbb cb)
{
  tvars {
    bool res1, res2;
  }
  twait { 
    oksrvc_t::post_launch_pub2 (mkevent (res1));
    pub2 ()->run_cfg (INTL_CONF_FILE, mkevent (res2)); 
  }
  TRIGGER (cb, res1 && res2);
}

tamed void 
okclnt_simple_t::process_T ()
{
  tvars {
    aarr_t aarr;
    bool rc (true);
    u_int opts (P_IINFO|P_VERBOSE);
    str file;
  }
  file = cgi["file"];

  syslog_warning << "Test A B C D\n";
  syslog_warning_x << "Test 1 2 3 4\n";
  syslog_info << "Test X Y Z\n";
  syslog_crit << "Test P Q R\n";

  if (file) {
    cgi.load_aarr (&aarr);

    twait { pub2 ()->run (&out, file, mkevent (rc), &aarr, opts); }

    if (!rc) {
      aarr.add ("target", file);
      file = FNF_FILE;
    }
  } else {
    aarr.add ("target", "&lt; <i>no <tt>file</tt> specified</i> &gt;");
    file = FNF_FILE;
    rc = false;
  }
  if (!rc) {
    twait { pub2 ()->run (&out, file, mkevent (rc), &aarr, opts|P_VISERR); }
  }
  output (out);
}

oksrvc_t::newclnt_t *
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
