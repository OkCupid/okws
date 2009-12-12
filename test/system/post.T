// -*-c++-*-
/* $Id: static.T 2939 2007-07-12 19:37:53Z max $ */

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

#if !defined(OKWS_AT_VERSION) || !OKWS_AT_VERSION(1,2,0,0)
# error "This service requires OKWS version 1.2.0 or greater!"
#endif

class oksrvc_form_t : public oksrvc_t {
public:
  oksrvc_form_t (int argc, char *argv[]) 
    : oksrvc_t (argc, argv), _union_cgi (true) {}

  newclnt_t *make_newclnt (ptr<ahttpcon> x);
  bool use_union_cgi () const { return _union_cgi; }

  bool _union_cgi;
};

class okclnt_form_t : public okclnt_t {
public:
  okclnt_form_t (ptr<ahttpcon> x, oksrvc_form_t *o)
    : okclnt_t (x, o), ok_form (o) {}
  ~okclnt_form_t () {}

  void process () { process_T (); }
  void process_T (CLOSURE); 

protected:
  oksrvc_form_t *ok_form;
};

static str
v2s (const vec<str> &in)
{
  strbuf b;
  b << "{";
  for (size_t i = 0; i < in.size (); i++) {
    if (i > 0) b << ", ";
    b << in[i];
  }
  b << "}";
  return b;
}

tamed void 
okclnt_form_t::process_T ()
{
  tvars {
    bool ok;
    ptr<pub3::dict_t> d (pub3::dict_t::alloc ());
  }
  
  {
    const char *fields[] = { "X", "Y", NULL };
    vec<str> v;
    for (const char **p = fields; *p; p++) {
      cgi.lookup (*p, &v);
      str s = v2s(v);
      d->insert (*p, s);
    }
  }
  

  twait { pub3 ()->run (&out, "/post.html", mkevent (ok), d); }

  output (out);
}

oksrvc_t::newclnt_t *
oksrvc_form_t::make_newclnt (ptr<ahttpcon> x)
{
  return New okclnt_form_t (x, this);
}

int
main (int argc, char *argv[])
{
  oksrvc_t *oksrvc = New oksrvc_form_t (argc, argv);
  oksrvc->launch ();

  // XXX -- this service, is by design, vulnerable to XSS attacks
  ok_filter_cgi = XSSFILT_NONE;

  // Force interesting overflow behavior.
  ok_cgibuf_limit = 0x40;

  amain ();
}
