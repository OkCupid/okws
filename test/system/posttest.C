// -*-c++-*-
/* $Id: posttest.T 3190 2008-02-05 15:10:03Z max $ */

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
#include "crypt.h"

class oksrvc_posttest_t : public oksrvc_t {
public:
  oksrvc_posttest_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  newclnt_t *make_newclnt (ptr<ahttpcon> x);
protected:
};

class okclnt_posttest_t : public okclnt_t {
public:
  okclnt_posttest_t (ptr<ahttpcon> x, oksrvc_posttest_t *o)
    : okclnt_t (x, o), ok_posttest (o) {}
  ~okclnt_posttest_t () {}

  void process ();

protected:
  oksrvc_posttest_t *ok_posttest;
};


void 
okclnt_posttest_t::process ()
{
  set_content_type ("text/plain");
  cgi_t *tab = cgi.cgi ();
  const pair_t *p;
  char buf[sha1::hashsize];

  for (p = tab->lfirst (); p; p = tab->lnext (p)) {
    if (p->vals.size () > 0) {
      str v = p->vals[0];
      str k = p->key;
      size_t l = v.len ();
      sha1_hash (buf, v.cstr (), v.len ());
      str f = strbuf () << hexdump (buf, sha1::hashsize);
      out << k << ": " << l << " " << f << "\n";
    }
  }
  output (out);
}

oksrvc_t::newclnt_t *
oksrvc_posttest_t::make_newclnt (ptr<ahttpcon> x)
{
  return New okclnt_posttest_t (x, this);
}

int
main (int argc, char *argv[])
{
  ok_cgibuf_limit = 0x400000; // 4MB!
  oksrvc_t *oksrvc = New oksrvc_posttest_t (argc, argv);
  oksrvc->launch ();
  amain ();
}
