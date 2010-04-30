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

class oksrvc_forloop_t : public oksrvc_t {
public:
  oksrvc_forloop_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  newclnt_t *make_newclnt (ptr<ahttpcon> x);
private:
};

//-----------------------------------------------------------------------

class okclnt_forloop_t : public okclnt2_t {
public:
  okclnt_forloop_t (ptr<ahttpcon> x, oksrvc_forloop_t *o) : okclnt2_t (x, o) {}
  ~okclnt_forloop_t () {}

  void process (proc_ev_t ev) { process_T (ev); }
  void process_T (proc_ev_t ev, CLOSURE); 

};

//-----------------------------------------------------------------------

tamed void 
okclnt_forloop_t::process_T (okclnt2_t::proc_ev_t ev)
{
  tvars {
    pub3::obj_t pub_obj;
    int opts (0);
    bool rc;
    const char *tstf;
  }

  tstf = "/test/forloop2.html";

  if (cgi.blookup ("wss")) {
    opts |= pub3::P_WSS;
  }
  
  for (int64_t i = 0; i < 3; i++) {
    pub_obj("row")[i]("val") = i;
    for (int64_t j = 0; j < 4; j++) {
      pub_obj("row")[i]("col")[j]("val") = j;
    }
  }

  for (int64_t i = 0; i < 4; i++) {
    pub_obj("simple")[i]("val") = int64_t (2 *i + 1);
  }

  for (int64_t i = 0; i < 5; i++) {
    pub3::ref_t o = pub_obj("push").push_back ();
    strbuf b ("xx-%d-xx", int (i));
    o("val") = b;
  }

  pub_obj("single")[0]("val") = "The-One-True-God";
  
  pub_obj("double")[0]("val") = "Mike";
  pub_obj("double")[1]("val") = "Ike";

  twait { 
    pub3 ()->run (&out, tstf, mkevent (rc), pub_obj.dict (), 
		  opts|pub3::P_VISERR); 
  }
  twait { output (out, mkevent ()); }

  ev->trigger (true, HTTP_OK);
}

//-----------------------------------------------------------------------

oksrvc_t::newclnt_t *
oksrvc_forloop_t::make_newclnt (ptr<ahttpcon> x)
{
  return New okclnt_forloop_t (x, this);
}

//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_forloop_t (argc, argv);
  oksrvc->launch ();
  amain ();
}

//-----------------------------------------------------------------------
