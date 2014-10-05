// -*-c++-*-
/* $Id: okwc.h 1682 2006-04-26 19:17:22Z max $ */

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

#ifndef _LIBWEB_OKWC3_H
#define _LIBWEB_OKWC3_H

#include "okcgi.h"
#include "abuf.h"
#include "aparse.h"
#include "hdr.h"
#include "str.h"
#include "web_prot.h"
#include "async.h"
#include <time.h>
#include "httpconst.h"
#include "async.h"
#include "dns.h"
#include "tame.h"
#include "okwc.h"
#include "list.h"
#include "oksync.h"

#define CANCELLED_STATUS HTTP_TIMEOUT

enum { DEF_HTTP_PORT = 80, DEF_HTTPS_PORT = 443 };

namespace okwc3 {

inline okws1_port_t std_port (bool s) 
{ return s ? DEF_HTTPS_PORT : DEF_HTTP_PORT; }

//-----------------------------------------------------------------------

struct queued_cbv_t {
  queued_cbv_t (cbv cb) : _cb (cb) {}
  cbv _cb;
  tailq_entry<queued_cbv_t> _link;
};

typedef event<ptr<hostent>, int>::ref ev_hent_t;


//-----------------------------------------------------------------------

class dnscache_entry_t : public virtual refcount {
public:
  dnscache_entry_t (const str &h, int t = 60) : 
    _hostname (h), _expires (0), _resolving (false), _ttl (t), _err (0), 
    _init (false) {}
  void lookup (ev_hent_t cb, CLOSURE);
private:
  void wait_for_resolution (evb_t cb, CLOSURE);
  void do_resolution (evb_t cb, CLOSURE);
  void commit_lookup (ptr<hostent> he, int status);

  str _hostname;
  ptr<hostent> _he;
  time_t _expires;
  bool _resolving;
  int _ttl;
  int _err;
  bool _init;

  cbv::ptr _waiter_remove_cb;

  tailq<queued_cbv_t, &queued_cbv_t::_link> _waiters;
};

//-----------------------------------------------------------------------

class dnscache_t {
public:
  dnscache_t () {}
  void lookup (const str &n, ev_hent_t ev);
private:
  qhash<str, ptr<dnscache_entry_t> > _cache;
};

//-----------------------------------------------------------------------

class post_t {
public:
  post_t () {}
  virtual ~post_t () {}
  virtual size_t len () const = 0;
  virtual void output (strbuf &b) const = 0;
};

//-----------------------------------------------------------------------

class simple_post_t : public post_t {
public:
  simple_post_t (const str &s) : post_t (), _s (s) {}
  ~simple_post_t () {}
  size_t len () const { return _s.len (); }
  void output (strbuf &b) const { b << _s; }
private:
  const str _s;
};

//-----------------------------------------------------------------------

class reqinfo_t {
public:
  reqinfo_t () {}
  virtual ~reqinfo_t () {}
  virtual str get_fixed_filename () const = 0;
  virtual str get_hdr_hostname () const = 0;
  virtual bool validate () const = 0;
protected:
};

//-----------------------------------------------------------------------

str fix_url_filename (const str &s);
bool parse_http_url (str in, bool *https, str *host, okws1_port_t *port, 
		     str *file);

//-----------------------------------------------------------------------

class reqinfo_direct_t : public reqinfo_t {
public:
  reqinfo_direct_t (const str &hn, int port, const str &fn)
    : reqinfo_t (), _hostname (hn), _port (port), _filename (fn) {}

  static ptr<reqinfo_t> alloc (const str &hn, int port, const str &fn)
  { return New refcounted<reqinfo_direct_t> (hn, port, fn); }

  str get_fixed_filename () const { return fix_url_filename (_filename); }
  str get_hdr_hostname () const ;
  bool validate () const { return true; }
private:
  const str _hostname;
  const int _port;
  const str _filename;
};

//-----------------------------------------------------------------------

class reqinfo_proxied_t : public reqinfo_t {
public:
  reqinfo_proxied_t (const str &url) ;

  static ptr<reqinfo_t> alloc (const str &url)
  { return New refcounted<reqinfo_proxied_t> (url); }

  str get_fixed_filename () const { return _url; }
  str get_hdr_hostname () const { return _hdr_hostname; }
  bool validate () const { return _valid; }
private:
  const str _url;
  str _hdr_hostname;
  bool _valid;
};

//-----------------------------------------------------------------------

class req_t : public virtual refcount {
public:
  req_t () {}
  virtual ~req_t () {}

  virtual str get_type () const = 0;
  virtual const post_t *get_post () const;
  virtual const vec<str> *get_extra_headers () const = 0;
  virtual htpv_t get_version () const = 0;
  virtual str get_hdr_hostname () const = 0;
  virtual str get_filename () const = 0;
  virtual const cgi_t *get_outcookie () const = 0;
  virtual str get_simple_post_str () const = 0;
  virtual void set_post (const str &p) = 0;
  virtual void set_extra_headers (const vec<str> &v) = 0;

  virtual void make (ptr<ok_xprt_base_t> x, bool k, evi_t cb) 
  { return make_T (x, k, cb); }


protected:
  void make_T (ptr<ok_xprt_base_t> x, bool keepalive, evi_t cb, CLOSURE);
  void format_req (strbuf &b, bool ka);
  str get_custom_host_header();
  std::pair<str,str> get_header_parts(str header);
  mutable ptr<post_t> _post_obj;

};

//-----------------------------------------------------------------------

class req3_t : public req_t {
public:
  req3_t (ptr<reqinfo_t> ri, htpv_t v = 1, cgi_t *c = NULL) 
    : req_t (),
      _reqinfo (ri), 
      _vers (v), 
      _outcookie (c) {}

  virtual ~req3_t () {}

  void set_post (const str &p) { _simple_post = p; }
  void set_extra_headers (const vec<str> &v) ;

  virtual str get_type () const { return NULL; }
  virtual str get_simple_post_str () const { return _simple_post; }
  virtual const vec<str> *get_extra_headers () const;
  htpv_t get_version () const { return _vers; }
  str get_hdr_hostname () const { return _reqinfo->get_hdr_hostname (); }
  str get_filename () const { return _reqinfo->get_fixed_filename (); }
  const cgi_t *get_outcookie () const { return _outcookie; }

protected:
  ptr<const reqinfo_t> _reqinfo;
  htpv_t _vers;
  cgi_t *_outcookie; // cookie sending out to the server

  // implement simple posts inline
private:
  str _simple_post;
  vec<str> _extra_headers;
};

//-----------------------------------------------------------------------

class resp_t : public virtual refcount {
public:
  resp_t (ptr<ok_xprt_base_t> x, ptr<abuf_t> b);
  virtual ~resp_t () {}
  void get (evi_t cb) { get_T (cb); }
  const okwc_http_hdr_t *hdr () const { return &_hdr; }
  okwc_http_hdr_t *hdr () { return &_hdr; }
  virtual str body () const { return NULL; }
  virtual int get_id () const { return 0; }
  const okwc_cookie_set_t &incookies () const { return _incookies; }

protected:

  virtual void run_chunker (evi_t cb) { run_chunker_T (cb); }
  virtual void get_body (evi_t cb) { get_body_T (cb); }

  virtual void eat_chunk (size_t, evi_t cb) = 0;
  virtual void finished_meal (int status, evi_t cb) = 0;
  virtual bool do_get_body (int status) const;

  ptr<ok_xprt_base_t> _x;
  ptr<abuf_t> _abuf;
  ptr<ok::scratch_handle_t> _scratch;
  okwc_cookie_set_t _incookies;
  okwc_http_hdr_t _hdr;

private:
  void get_body_T (evi_t cb, CLOSURE);
  void get_T (evi_t cb, CLOSURE);
  void run_chunker_T (evi_t cb, CLOSURE);
};

//-----------------------------------------------------------------------

class resp_simple_t : public resp_t {
public:
  resp_simple_t (ptr<ok_xprt_base_t> x, ptr<abuf_t> b) 
    : resp_t (x, b),
      _dumper (_abuf) {}

  void eat_chunk (size_t sz, evi_t cb) { eat_chunk_T (sz, cb); }
  void finished_meal (int status, evi_t cb);
  virtual str body () const { return _body; }
protected:
  async_dumper_t _dumper;
  vec<str> _chunks;
  str _body;
private:
  void eat_chunk_T (size_t sz, evi_t cb, CLOSURE);
};

//-----------------------------------------------------------------------

class resp_factory_t {
public:
  virtual ~resp_factory_t () {}
  virtual ptr<resp_t> alloc_resp (ptr<ok_xprt_base_t>, ptr<abuf_t> buf) = 0;
};

//-----------------------------------------------------------------------

class obj_factory_t : public resp_factory_t {
public:
  obj_factory_t () {}
  virtual ~obj_factory_t () {}
  virtual ptr<resp_t> alloc_resp (ptr<ok_xprt_base_t>, ptr<abuf_t> buf);
  virtual ptr<req_t> alloc_req (ptr<reqinfo_t> ri, int v, cgi_t *c);
};

//-----------------------------------------------------------------------

typedef event<int, ptr<ok_xprt_base_t>, bool >::ref evixb_t;
typedef event<int, ptr<resp_t> >::ref resp_ev_t;

//-----------------------------------------------------------------------

class agent_t : public virtual refcount {
public:
  virtual ~agent_t () {}
  agent_t (const str &h, okws1_port_t p, bool s = false) 
    : _hostname (h), 
      _port (p),
      _ssl (s) {}
  
  virtual void req (ptr<req_t> req, ptr<resp_factory_t> resp, resp_ev_t cb);

  str hostname () const { return _hostname; }
  okws1_port_t port () const { return _port; }
  bool use_ssl () const { return _ssl; }

protected:

  void get_x (ptr<ok_xprt_base_t> x, evixb_t ev, CLOSURE);

  const str _hostname;
  okws1_port_t _port;
  bool _ssl;

protected:
  void req_oneshot (ptr<req_t> req, ptr<resp_factory_t> resp, 
		    resp_ev_t cb, CLOSURE);
};


//-----------------------------------------------------------------------

typedef event<int, ptr<resp_t> >::ref simple_ev_t;

class agent_get_t : public agent_t {
public:
  agent_get_t (const str &h, int p, bool s, ptr<obj_factory_t> f = NULL) 
    : agent_t (h, p, s),
      _obj_factory (f ? f : New refcounted<obj_factory_t> ()) {}

  virtual void get (const str &fn, simple_ev_t ev,
		    int v = 1, cgi_t *c = NULL, 
		    str post = NULL, vec<str> *eh = NULL) = 0;
protected:
  ptr<obj_factory_t> _obj_factory;
};


//-----------------------------------------------------------------------

class agent_get_direct_t : public agent_get_t {
public:
  virtual ~agent_get_direct_t () {}
  agent_get_direct_t (const str &h, int p, bool s = false, 
                      ptr<obj_factory_t> f = NULL) 
    : agent_get_t (h, p, s, f) {}

  void 
  get (const str &fn, simple_ev_t cb, int v = 1, cgi_t *c = NULL, 
       str post = NULL, vec<str> *eh = NULL)
  { get_T (fn, cb, v, c, post, eh); }

private:
  void get_T (const str &fn, simple_ev_t cb, int v, cgi_t *c, 
	      str post, vec<str> *eh, CLOSURE);
};

//-----------------------------------------------------------------------

class agent_get_proxied_t : public agent_get_t {
public:
  agent_get_proxied_t (const str &h, int p, bool s = false, 
		       ptr<obj_factory_t> f = NULL) 
    : agent_get_t (h, p, s, f) {}
  
  void 
  get (const str &url, simple_ev_t cb, int v = 1, cgi_t *c = NULL, 
       str post = NULL, vec<str> *eh = NULL)
  { get_T (url, cb, v, c, post, eh); }

private:
  void get_T (const str &fn, simple_ev_t cb, int v, cgi_t *c, 
	      str post, vec<str> *eh, CLOSURE);
};

//-----------------------------------------------------------------------


};

#endif /* _LIBWEB_OKWC3_H */
