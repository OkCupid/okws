// -*-c++-*-
/* $Id: purify.T 3190 2008-02-05 15:10:03Z max $ */

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

#include "purify_lib.h"
#include "rxx.h"
#include "tame_connectors.h"
#include "parseopt.h"

namespace purify {

  const char *conf_file_key = "PURIFY_URL";

  tamed void
  purifier_t::init (pub3::ok_iface_t *pub, 
		    str file, evb_t ev, str key)
  {
    tvars {
      bool rc (true);
    }

    if (!key) key = conf_file_key;

    if (file) 
      twait { pub->run_cfg (file, mkevent (rc)); }  

    if (rc) {
      rc = false;
      if (!pub->pub3_config_obj ()(key).to_str (&_url) || 
	  !_url || _url.len() == 0) {
	warn << "purifier: No " << key << " specifed!\n";
      } else if (!init ()) {
	warn << "purifier: Bad URL specified: " << key << " => " 
	     << _url << "\n";
      } else {
	rc = true;
      }
    }
    ev->trigger (rc);
  }

  bool
  purifier_t::init (const str &s)
  {
    bool ret = false;
    if (!(_url = s)) {
      warn << "purifier: no URL passed\n";
    } else if (!init ()) {
      warn << "purifier: Bad URL specified: " << _url << "\n";
    } else {
      ret = true;
    }
    return ret;
  }

  bool
  purifier_t::init ()
  {
    static rxx url_rxx ("http://([^:/]+)(:(\\d+)/)?(.*)");
    bool ret = false;
    assert (_url);
    if (url_rxx.match (_url)) {
      str hn = url_rxx[1];
      str port_str = url_rxx[3];
      str path = url_rxx[4];
      int port = 80;
	
      if (port_str && !convertint (port_str, &port)) {
	warn << "purifier: bad port specified: " << port_str << "\n";
      } else {
	_agent = New refcounted<okwc3::agent_xml_t> (hn, port, path);
	ret = true;
      }
    }
    return ret;
  }

  tamed void
  purifier_t::purify (str inh, ev_t ev)
  {
    tvars {
      str outh;
      status_t status (OK);
      xml_outreq_t req;
      xml_inresp_t resp;
      int hs;
    }

    if (!_agent) {
      status = ERR_INIT;
      warn << "purifier: agent is not initialized\n";
    } else {
      req.set_method_name ("html.purify");
      req[0] = base64_str_t (inh);
      hs = HTTP_TIMEOUT;
      twait { 
	_agent->call (req, connector::timeout (mkevent (hs, resp), _timeout)); 
      } 
      if (hs == HTTP_OK) {
	int code;
	str msg;
	if (resp.is_fault (&code, &msg)) {
	  warn << "purifier: purify Fault (" << code << "): ";
	  if (msg) warnx << msg;
	  warnx << "\n";
	  status = ERR_PURIFY;
	} else {
	  outh = resp[0];
	}
      } else if (hs == HTTP_TIMEOUT) {
	warn << "purify: RPC timeout with server: " << _url << "\n";
	status = ERR_TIMEOUT;
      } else {
	warn << "purify: RPC failure: " << status << "\n";
	status = ERR_PURIFY;
      }
    }
    ev->trigger (status, outh);
  }

};
