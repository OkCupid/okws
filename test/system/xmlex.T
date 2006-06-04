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

  void hello (xml_req_t q) {
    xml_resp_t s;
    s("zero") = q[0];
    s("one") = q[1];

    zbuf z;
    s.output (z);
    strbuf b;
    z.to_strbuf (&b, false);
    b.tosuio ()->output (2);
    
    reply (s);
  };

private:
  oksrvc_xmlex_t *_service;
};

class oksrvc_xmlex_t : public oksrvc_xmlrpc_t<okclnt_xmlex_t, oksrvc_xmlex_t> {
public:
  oksrvc_xmlex_t (int argc, char *argv[])
    : oksrvc_xmlrpc_t<okclnt_xmlex_t, oksrvc_xmlex_t> (argc, argv)
  { regh ("xmlex.Hello", &okclnt_xmlex_t::hello); }

};

okclnt_xmlex_t::okclnt_xmlex_t (ptr<ahttpcon> x, oksrvc_xmlex_t *s)
  : okclnt_xmlrpc_t (x, s), _service (s) {}

int
main (int argc, char *argv[])
{
  (New oksrvc_xmlex_t (argc, argv))->launch ();
  amain ();
}
