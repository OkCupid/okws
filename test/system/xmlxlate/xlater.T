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

#define ENABLE_XML_XDR

#include "okxmlxlatemgr.h"
#include "tstprot.h"

class oksrvc_xlater_t;

class okclnt_xlater_t : public okclnt_xmlrpc_t {
public:
  okclnt_xlater_t (ptr<ahttpcon> x, oksrvc_xlater_t *s) ;

  void xlate (xml_req_t q, xml_resp_ev_t ev) { xlate_T (q, ev); }
  void constants (xml_req_t q, xml_resp_ev_t ev) { constants_T (q, ev); }
  
private:
  void xlate_T (xml_req_t q, xml_resp_ev_t ev, CLOSURE);
  void constants_T (xml_req_t q, xml_resp_ev_t cb, CLOSURE);


  oksrvc_xlater_t *_service;
};

class oksrvc_xlater_t : 
  public oksrvc_xmlrpc_t<okclnt_xlater_t, oksrvc_xlater_t> {
public:
  oksrvc_xlater_t (int argc, char *argv[])
    : oksrvc_xmlrpc_t<okclnt_xlater_t, oksrvc_xlater_t> (argc, argv)
  { 
    // Register XML-RPC service methods
    regh ("xdr.xlate", &okclnt_xlater_t::xlate); 
    regh ("xdr.constants", &okclnt_xlater_t::constants);
    _xlmgr.add_file (tstprot_rpc_file);
  }

  okxml::xlate_mgr_t *xlmgr () { return &_xlmgr; }

private:
  okxml::xlate_retry_mgr_t _xlmgr;

};

okclnt_xlater_t::okclnt_xlater_t (ptr<ahttpcon> x, oksrvc_xlater_t *s)
  : okclnt_xmlrpc_t (x, s), _service (s) {}

int
main (int argc, char *argv[])
{
  (New oksrvc_xlater_t (argc, argv))->launch ();
  amain ();
}

tamed void
okclnt_xlater_t::xlate_T (xml_req_t q, xml_resp_ev_t ev)
{
  tvars {
    xml_resp_t res;
  }
  twait {
    _service->xlmgr ()->xlate (q[0], mkevent (res));
  }
  ev->trigger (res);
}

tamed void
okclnt_xlater_t::constants_T (xml_req_t q, xml_resp_ev_t ev)
{
  tvars {
    xml_resp_t res;
  }
  twait {
    _service->xlmgr ()->get_constants (q, mkevent (res));
  }
  ev->trigger (res);
}
