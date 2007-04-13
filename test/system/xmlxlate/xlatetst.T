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
#include "okxmlxlatemgr.h"
#include "tstprot_xml.h"

class oksrvc_xlatetst_t;

class okclnt_xlatetst_t : public okclnt_xmlrpc_t {
public:
  okclnt_xlatetst_t (ptr<ahttpcon> x, oksrvc_xlatetst_t *s) ;

  void xlate (xml_req_t q, xml_resp_cb_t cb) { xlate_T (q, cb); }
  
private:
  void xlate_T (xml_req_t q, xml_resp_cb_t cb, CLOSURE);


  oksrvc_xlatetst_t *_service;
};

class oksrvc_xlatetst_t : 
  public oksrvc_xmlrpc_t<okclnt_xlatetst_t, oksrvc_xlatetst_t> {
public:
  oksrvc_xlatetst_t (int argc, char *argv[])
    : oksrvc_xmlrpc_t<okclnt_xlatetst_t, oksrvc_xlatetst_t> (argc, argv)
  { 
    // Register XML-RPC service methods
    regh ("xlatetst.xlate", &okclnt_xlatetst_t::xlate); 
    _xlmgr.add_file (tstprot_xml_rpc_file);
  }

  void xlate (xml_req_t q, xml_resp_cb_t cb, CLOSURE);

private:
  okxml::xlate_mgr_t _xlmgr;

};

okclnt_xlatetst_t::okclnt_xlatetst_t (ptr<ahttpcon> x, oksrvc_xlatetst_t *s)
  : okclnt_xmlrpc_t (x, s), _service (s) {}

int
main (int argc, char *argv[])
{
  (New oksrvc_xlatetst_t (argc, argv))->launch ();
  amain ();
}

tamed void
okclnt_xlatetst_t::xlate_T (xml_req_t q, xml_resp_cb_t cb)
{
  tvars {
    xml_resp_t res;
  }
  twait {
    _service->xlate (q, mkevent (res));
  }
  (*cb) (res);
}

tamed void
oksrvc_xlatetst_t::xlate (xml_req_t in, xml_resp_cb_t cb)
{
  tvars {
    xml_resp_t res;
    clnt_stat stat;
  }
  twait { _xlmgr.xlate (in[0], &res, mkevent (stat)); }

  (*cb) (res);
}
