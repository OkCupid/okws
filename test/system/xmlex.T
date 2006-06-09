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
    regh ("xmlex.Hello", &okclnt_xmlex_t::hello); 
    regh ("okws.pub", &okclnt_xmlex_t::okws_pub);
  }

  u_int *lookup_opt (const str &s) const { return NULL; }

};

okclnt_xmlex_t::okclnt_xmlex_t (ptr<ahttpcon> x, oksrvc_xmlex_t *s)
  : okclnt_xmlrpc_t (x, s), _service (s) {}

TAMED void
okclnt_xmlex_t::okws_pub_T (xml_req_t q, xml_resp_cb_t cb)
{
  VARS {
    xml_resp_t r;
    size_t i, j;
    str fn;
    u_int opts, *opt;
  }
  BLOCK {
    r.setsize (q.size ());
    for (i = 0; i < q.size (); i++) {
      fn = q[i]("filename");

      opts = 0;
      for (j = 0; j < q[i]("options").size (); j++) {
	if ((opt = _service->lookup_opt (q[i]("options")[j])))
	  opts |= (*opt);
      }

    }
  }

  SIGNAL (cb, r);
}

int
main (int argc, char *argv[])
{
  (New oksrvc_xmlex_t (argc, argv))->launch ();
  amain ();
}
