// -*-c++-*-
/* $Id: static.T 4148 2009-02-26 15:05:51Z max $ */

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
#include "pub3obj.h"

//-----------------------------------------------------------------------

class oksrvc_reflect_t : public oksrvc_t {
public:
  oksrvc_reflect_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  newclnt_t *make_newclnt (ptr<ahttpcon> x);
private:
};

//-----------------------------------------------------------------------

class okclnt_reflect_t : public okclnt2_t {
public:
  okclnt_reflect_t (ptr<ahttpcon> x, oksrvc_reflect_t *o) : okclnt2_t (x, o) {}
  ~okclnt_reflect_t () {}

  void process (proc_ev_t ev) { process_T (ev); }
  void process_T (proc_ev_t ev, CLOSURE); 

};

//-----------------------------------------------------------------------

tamed void 
okclnt_reflect_t::process_T (okclnt2_t::proc_ev_t ev)
{
  tvars {
    pub3::obj_t pub_obj;
    int opts (0);
    bool rc;
    str file;
    vec<str> vars;
  }

  opts = 0;
  file = cgi["file"];
  cgi.lookup ("vars", &vars);
  for (size_t i = 0; i < vars.size () ; i++) {
    str val;
    if (vars[i] && vars[i].len () && (val = cgi[vars[i]]) && val.len ())
      pub_obj (vars[i]).fancy_assign_to (val);
  }
  if (file && file.len ()) {
    twait { 
      pub3 ()->run (&out, file, mkevent (rc), pub_obj.dict (), 
		    opts|pub3::P_VISERR); 
    }
  } else {
    out << "no file given";
  }
  
  twait { output (out, mkevent ()); }

  ev->trigger (true, HTTP_OK);
}

//-----------------------------------------------------------------------

oksrvc_t::newclnt_t *
oksrvc_reflect_t::make_newclnt (ptr<ahttpcon> x)
{
  return New okclnt_reflect_t (x, this);
}

//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_reflect_t (argc, argv);
  oksrvc->launch ();
  amain ();
}

//-----------------------------------------------------------------------
