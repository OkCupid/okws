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

class oksrvc_configtest_t : public oksrvc_t {
public:
  oksrvc_configtest_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  okclnt_base_t *make_newclnt (ptr<ahttpcon> x);
};

class okclnt_configtest_t : public okclnt_t {
public:
  okclnt_configtest_t (ptr<ahttpcon> x, oksrvc_configtest_t *o)
    : okclnt_t (x, o), ok_configtest (o) {}
  ~okclnt_configtest_t () {}

  void process () { process_T (); }
  void process_T (CLOSURE); 

protected:
  oksrvc_configtest_t *ok_configtest;
};

TAMED void okclnt_configtest_t::process_T ()
{
  VARS {
    bool rc (true);
  }

  BLOCK { pub2 ()->run_cfg (cgi["fn"], @(rc)); }
  if (!rc) {
    out << "Error including file " << cgi["fn"] << " or file wasn't given\n";
  } else {
    str lst = cgi["display"];
    vec<str> vars;
    static rxx comma (",");
    split (&vars, comma, lst);
    for (u_int i = 0; i < vars.size (); i++) {
      out << vars[i] << " -> " ;
      str r = pub2 ()->cfg (vars[i]);
      if (r) out << r;
      else out << "NULL";
      out << "<br>\n";
    }
  }
  output (out);
}

okclnt_base_t *
oksrvc_configtest_t::make_newclnt (ptr<ahttpcon> x)
{
  return New okclnt_configtest_t (x, this);
}

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_configtest_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
