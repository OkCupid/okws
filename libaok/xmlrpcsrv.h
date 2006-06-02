// -*-c++-*-
/* $Id: ok.h 1967 2006-06-01 12:51:17Z max $ */

/*
 *
 * Copyright (C) 2002-2004 Maxwell Krohn (max@okcupid.com)
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

#ifndef _LIBAOK_OKXMLSRV_H
#define _LIBAOK_OKXMLSRV_H

# ifdef HAVE_EXPAT
#  include "ok.h"
#  include "okxmlparse.h"
#  include "okxmlwrap.h"


class oksrvc_xmlrpc_t;
class okclnt_xmlrpc_base_t : public okclnt_base_t {
public:
  okclnt_xmlrpc_base_t (ptr<ahttpcon> xx, oksrvc_t *s, u_int to = 0)
    : okclnt_base_t (xx, s), _parser (xx, to) {}

  ~okclnt_xmlrpc_base_t () {}
  void parse (cbi cb) { _parser.parse (cb); }

  /*
   * A smattering of method to access data members of the underlying parser
   * object 
   */
  http_inhdr_t *hdr_p () { return _parser.hdr_p (); }
  const http_inhdr_t &hdr_cr () const { return _parser.hdr_cr (); }
  ptr<const xml_top_level_t> top_level () const 
  { return _parser.top_level (); }
  str errmsg () const { return _parser.errmsg (); }
  xml_parse_status_t errcode () const { return _parser.errcode (); }
  cgi_t &cookie () { return _parser.get_cookie (); }
  cgi_t &url () { return _parser.get_url (); }
  http_inhdr_t &hdr () { return _parser.hdr; }

  void respond (xml_resp_t r);
protected:
  http_parser_xml_t _parser;
};

class oksrvc_xmlrpc_base_t : public oksrvc_t {
public:
  oksrvc_xmlrpc_base_t (int argc, char *argv[]) : oksrvc_t (argc, argv) {}
  virtual ~oksrvc_xmlrpc_base_t () {}
  virtual void handle (okclnt_xmlrpc_base_t *b) = 0;
};

class okclnt_xmlrpc_t : public okclnt_xmlrpc_base_t {
public:
  okclnt_xmlrpc_t (ptr<ahttpcon> xx, oksrvc_xmlrpc_base_t *s, u_int to = 0)
    : okclnt_xmlrpc_base_t (xx, s, to), _srvc (s) {}
  void process () { _srvc->handle (this); }
protected:
  oksrvc_xmlrpc_base_t *_srvc;
};

typedef callback<void, xml_resp_t>::ref xml_resp_cb_t;

template<class C>
class oksrvc_xmlrpc_t : public oksrvc_xmlrpc_base_t {
public:
  typedef void (C::*handler_t) (xml_req_t, xml_resp_cb_t);
  oksrvc_xmlrpc_t (int argc, char *argv[]) 
    : oksrvc_xmlrpc_base_t (argc, argv) {}
  void handle (okclnt_xmlrpc_base_t *c)
  {
    C *cli = reinterpret_cast<C *> (c);
    xml_resp_t resp;
    if (c->errcode () != 0) {
      resp = xml_fault_t (c->errcode (), c->errmsg ());
    } else {
      ptr<const xml_top_level_t> e = c->top_level ();
      if (e->size () < 1) {
	resp = xml_fault_wrap_t (1, "No data given in XML call");
      } else {
	ptr<xml_method_call_t> call = e->get (0)->to_xml_method_call ();
	if (!call) {
	  resp = xml_fault_wrap_t (2, "No methdCall given in request");
	} else {
	  str nm = call->method_name ();
	  handler_t *h = _dispatch_table[nm];
	  if (!h) {
	    resp = xml_fault_wrap_t (3, "Method not found");
	  } else {
	    ((*cli).*(*h)) (xml_req_t (call->params ()),
			    wrap (c, &okclnt_xmlrpc_base_t::respond));
	    return;
	  }
	}
      }
    }
    c->respond (resp);
  }

  okclnt_base_t *make_newclnt (ptr<ahttpcon> lx) { return New C (lx, this); }
protected:
  // register a handler
  void regh (const str &s, xml_handler_t h);
private:
  qhash<str, handler_t> _dispatch_table;
};

# endif /* HAVE_EXPAT */
#endif /* _LIBAOK_OKXMLSRV_H */
