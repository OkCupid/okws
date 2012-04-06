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

class oksrvc_double_t : public oksrvc_t {
public:
  oksrvc_double_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  newclnt_t *make_newclnt (ptr<ahttpcon> x);
protected:
};

class okclnt_double_t : public okclnt2_t {
public:
  okclnt_double_t (ptr<ahttpcon> x, oksrvc_double_t *o)
    : okclnt2_t (x, o), ok_double (o) {}
  ~okclnt_double_t () {}

  void process (proc_ev_t ev) { process_T (ev); }
  void process_T (proc_ev_t ev, CLOSURE); 

protected:
  oksrvc_double_t *ok_double;
};

tamed void 
okclnt_double_t::process_T (okclnt2_t::proc_ev_t ev)
{
  tvars {
    ptr<pub3::dict_t> dict (pub3::dict_t::alloc ());
    bool rc (true);
    u_int opts (pub3::P_IINFO|pub3::P_VERBOSE);
    str file;
    double d (0.0);
    float f (0.0);
  }
  file = cgi["file"];
  cgi.lookup ("d", &d);
  cgi.lookup ("f", &f);

  dict->insert("d", pub3::expr_double_t::alloc (d));
  dict->insert("f", pub3::expr_double_t::alloc (f));
  warn << "file: " << file << "\n";
  warn << "d: " << d << "\n";
  warn << "f: " << f << "\n";

  if (file) {

    twait { pub3 ()->run (&out, file, mkevent (rc), dict, opts); }

    if (!rc) {
      dict->insert ("target", file);
      file = FNF_FILE;
    }
  } else {
    dict->insert ("target", "&lt; <i>no <tt>file</tt> specified</i> &gt;");
    file = FNF_FILE;
    rc = false;
  }
  if (!rc) {
    twait { pub3 ()->run (&out, file, mkevent (rc), dict, opts|pub3::P_VISERR); }
  }
  twait { output (out, mkevent ()); }
  ev->trigger (true, HTTP_OK);
}

oksrvc_t::newclnt_t *
oksrvc_double_t::make_newclnt (ptr<ahttpcon> x)
{
  return New okclnt_double_t (x, this);
}

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_double_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
