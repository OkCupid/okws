// -*-c++-*-
/* $Id: static.T 1777 2006-05-24 03:44:57Z max $ */

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
#include "xmlrpcsrv.h"
#include "okcgi.h"
#include "pub.h"
#include <unistd.h>
#include "tame.h"
#include "rxx.h"

class oksrvc_xmlex_t;

class okclnt_xmlex_t : public okclnt_xmlrpc_t {
public:
  okclnt_xmlex_t (ptr<ahttpcon> x, oksrvc_xmlex_t *s) ;

  void hello (xml_req_t q, xml_resp_cb_t cb) {
    xml_resp_t s;
    s[0]("zero") = q[0];
    s[0]("one") = q[1];
    s[1] = "<testing><this&that>";

    SIGNAL (cb, s);
  };

  void okws_pub (xml_req_t q, xml_resp_cb_t cb) { okws_pub_T (q, cb); }
  void okws_pub_T (xml_req_t q, xml_resp_cb_t cb, CLOSURE);

private:
  oksrvc_xmlex_t *_service;
};

class oksrvc_xmlex_t : public oksrvc_xmlrpc_t<okclnt_xmlex_t, oksrvc_xmlex_t> {
public:
  oksrvc_xmlex_t (int argc, char *argv[])
    : oksrvc_xmlrpc_t<okclnt_xmlex_t, oksrvc_xmlex_t> (argc, argv)
  { 
    // Register XML-RPC service methods
    regh ("xmlex.Hello", &okclnt_xmlex_t::hello); 
    regh ("okws.pub", &okclnt_xmlex_t::okws_pub);
    
    init_optmap ();
  }

  void init_optmap ()
  {
    const char *options[] = { "debug", "includeInfo", "verbose",
			      "visibleErrors", "WSS", "raw", NULL };
    u_int optvals[] = { P_DEBUG, P_IINFO, P_VERBOSE, P_VISERR,
		      P_WSS, P_NOPARSE };
    u_int *vp;
    const char **o;
    for (o = options, vp = optvals; *o; o++, vp++) {
      _pub_options.insert (*o, *vp);
    }
  }

  const u_int *lookup_opt (const str &s) const { return _pub_options[s]; }
private:
  qhash<str, u_int> _pub_options;

};

okclnt_xmlex_t::okclnt_xmlex_t (ptr<ahttpcon> x, oksrvc_xmlex_t *s)
  : okclnt_xmlrpc_t (x, s), _service (s) {}

static bool
load_args (xml_obj_const_t x, const oksrvc_xmlex_t *_service,
	      str *fn, u_int *opts, aarr_t *a)
{
  *fn = x("filename");
  const u_int *opt;
  *opts = 0;
  for (size_t j = 0; j < x("options").size (); j++) {
    if ((opt = _service->lookup_opt (x("options")[j])))
      *opts |= (*opt);
  }
  for (size_t j = 0; j < x("variables").size (); j++) {
    xml_obj_const_t o = x("variables")[j];
    if (o.name () && o.name ().len ()) {
      a->add (o.name (), str (o.value ()));
    }
  }
  
  return (*fn && fn->len () > 0);
}

#define XML_PUB_ARGLOAD_FAILED  101

TAMED void
okclnt_xmlex_t::okws_pub_T (xml_req_t q, xml_resp_cb_t cb)
{
  VARS {
    xml_resp_t r;
    size_t i;
    str fn;
    u_int opts;
    vec<ptr<zbuf> > out;
    vec<xpub_status_t> pub_errc;
    aarr_t aa;
    vec<size_t> calls;
  }
  warn << "called...\n";
  BLOCK {
    r.setsize (q.size ());
    out.setsize (q.size ());
    pub_errc.setsize (q.size ());
    for (i = 0; i < q.size (); i++) {
      out[i] = New refcounted<zbuf> ();
      aa.clear ();
      if (load_args (q[i], _service, &fn, &opts, &aa)) {
	calls.push_back (i);
	pub2 ()->run_full (out[i], fn, @(pub_errc[i]), &aa, opts);
      } else {
	r[i]("error")("code") = XML_PUB_ARGLOAD_FAILED;
	r[i]("error")("desc") = "failed to load Pub arguments";
      }
    }
  }
  for (i = 0; i < calls.size (); i++) {
    size_t j = calls[i];
    if (pub_errc[j].status != XPUB_STATUS_OK) {
      r[j]("error")("code") = int (pub_errc[j].status);
      r[j]("error")("desc") = errcode2str (pub_errc[j]);
    } else {
      strbuf b ;
      out[j]->to_strbuf (&b, false);
      r[j]("data") = base64_str_t (armor64 (b));
    }
  }

  /*
  zbuf z;
  r.output (z);
  strbuf b;
  z.to_strbuf (&b, false);
  str s (b);
  warn << "output: " << s << "\n";
  */

  SIGNAL (cb, r);
}

int
main (int argc, char *argv[])
{
  (New oksrvc_xmlex_t (argc, argv))->launch ();
  amain ();
}
