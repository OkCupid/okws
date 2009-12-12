// -*-c++-*-
/* $Id: objtest.T 2314 2006-11-08 20:22:44Z max $ */

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
#include "pub3.h"

//-----------------------------------------------------------------------

class oksrvc_objtest_t : public oksrvc_t {
public:
  oksrvc_objtest_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  newclnt_t *make_newclnt (ptr<ahttpcon> x);
};

//-----------------------------------------------------------------------

class okclnt_objtest_t : public okclnt_t {
public:
  okclnt_objtest_t (ptr<ahttpcon> x, oksrvc_objtest_t *o)
    : okclnt_t (x, o), ok_objtest (o) {}
  ~okclnt_objtest_t () {}

  void process () { process_T (); }
  void process_T (CLOSURE); 

protected:
  oksrvc_objtest_t *ok_objtest;
};

//-----------------------------------------------------------------------

tamed void 
okclnt_objtest_t::process_T ()
{
  tvars {
    pub3::obj_t o;
    pub3::obj_dict_t d;
    pub3::obj_t o2;
    bool rc;
  }
  
  {
    o("bar")("a") = "A";
    o("bar")("b") = 2;
    pub3::ref_t x = o("bar");
    o2 = x;
    d("o") = o2;
  }

  twait { 
    pub3 ()->run (&out, "/test/objtest2.html", mkevent (rc), d.dict ());
  }

  output (out);
}

oksrvc_t::newclnt_t *
oksrvc_objtest_t::make_newclnt (ptr<ahttpcon> x)
{
  return New okclnt_objtest_t (x, this);
}

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_objtest_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
