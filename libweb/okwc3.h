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

#define CANCELLED_STATUS HTTP_TIMEOUT

namespace okwc3 {

inline int std_port (bool s) { return s ? 443 : 80 ; }

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
  reqinfo_t (bool b) : _https (b) {}
  virtual ~reqinfo_t () {}
  virtual str get_fixed_filename () const = 0;
  virtual str get_hdr_hostname () const = 0;
  virtual bool validate () const = 0;
  bool https () const { return _https; }
protected:
  const bool _https;
};

//-----------------------------------------------------------------------

class reqinfo_direct_t : public reqinfo_t {
public:
  reqinfo_direct_t (const str &hn, int port, const str &fn, bool s)
    : reqinfo_t (s), _hostname (hn), _port (port), _filename (fn) {}

  static ptr<reqinfo_t> alloc (const str &hn, int port, const str &fn, bool s)
  { return New refcounted<reqinfo_direct_t> (hn, port, fn, s); }

  str get_fixed_filename () const;
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
  reqinfo_proxied_t (const str &url, bool s) ;

  static ptr<reqinfo_t> alloc (const str &url, bool s)
  { return New refcounted<reqinfo_proxied_t> (url, s); }

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
  req_t (ptr<reqinfo_t> ri, int v = 1, cgi_t *c = NULL) 
    : _reqinfo (ri), _vers (v), _outcookie (c) {}

  virtual ~req_t () {}
  virtual void make (ptr<ok_xprt_base_t> x, evi_t cb) { return make_T (x, cb); }

  virtual const post_t *get_post () const;
  virtual const vec<str> *get_extra_headers () const;

  virtual str get_type () const { return NULL; }
  bool https () const { return _reqinfo->https (); }

  void set_post (const str &p) { _simple_post = p; }
  void set_extra_headers (const vec<str> &v);

protected:
  void format_req (strbuf &b);

private:
  void make_T (ptr<ok_xprt_base_t> x, evi_t cb, CLOSURE);

protected:
  ptr<const reqinfo_t> _reqinfo;
  int _vers;
  cgi_t *_outcookie; // cookie sending out to the server

  // implement simple posts inline
private:
  str _simple_post;
  mutable ptr<post_t> _post_obj;
  vec<str> _extra_headers;
};

//-----------------------------------------------------------------------

class resp_t : public virtual refcount {
public:
  resp_t ();
  virtual ~resp_t () {}
  void get (evi_t cb) { get_T (cb); }
  void setx (ptr<ok_xprt_base_t> x);
  const okwc_http_hdr_t *hdr () const { return &_hdr; }
  okwc_http_hdr_t *hdr () { return &_hdr; }
  virtual str body () const { return NULL; }

protected:

  virtual void run_chunker (evi_t cb) { run_chunker_T (cb); }
  virtual void get_body (evi_t cb) { get_body_T (cb); }

  virtual void eat_chunk (size_t, evi_t cb) = 0;
  virtual void finished_meal (int status, evi_t cb) = 0;
  virtual bool do_get_body (int status) const;

  ptr<ok_xprt_base_t> _x;
  abuf_t _abuf;
  char _scratch[OKWC_SCRATCH_SZ];
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
  resp_simple_t () : _dumper (&_abuf) {}
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

class obj_factory_t {
public:
  obj_factory_t () {}
  virtual ~obj_factory_t () {}
  virtual ptr<resp_t> alloc_resp ();
  virtual ptr<req_t> alloc_req (ptr<reqinfo_t> ri, int v, cgi_t *c);
};

//-----------------------------------------------------------------------

class agent_t : public virtual refcount {
public:
  virtual ~agent_t () {}
  agent_t (const str &h, int p, ptr<obj_factory_t> a = NULL) 
    : _hostname (h), 
      _port (p),
      _obj_factory (a ? a : New refcounted<obj_factory_t> ()) {}

  virtual void req (ptr<req_t> req, ptr<resp_t> resp, evi_t cb)
  { req_T (req, resp, cb); }

  virtual ptr<resp_t> alloc_resp () { return _obj_factory->alloc_resp (); }
  virtual ptr<req_t> alloc_req (ptr<reqinfo_t> ri, int v, cgi_t *c)
  { return _obj_factory->alloc_req (ri, v, c); }

protected:

  const str _hostname;
  int _port;
  ptr<obj_factory_t> _obj_factory;

private:
  void req_T (ptr<req_t> req, ptr<resp_t> resp, evi_t cb, CLOSURE);
};


//-----------------------------------------------------------------------

typedef event<int, ptr<resp_t> >::ref simple_ev_t;

class agent_get_t : public agent_t {
public:
  agent_get_t (const str &h, int p, ptr<obj_factory_t> a = NULL) 
    : agent_t (h, p, a) {}

  virtual void get (const str &fn, simple_ev_t ev,
		    int v = 1, cgi_t *c = NULL, bool https = false,
		    str post = NULL, vec<str> *eh = NULL) = 0;
};


//-----------------------------------------------------------------------

class agent_get_direct_t : public agent_get_t {
public:
  virtual ~agent_get_direct_t () {}
  agent_get_direct_t (const str &h, int p, ptr<obj_factory_t> f = NULL) 
    : agent_get_t (h, p, f) {}

  void 
  get (const str &fn, simple_ev_t cb, int v = 1, cgi_t *c = NULL, 
       bool https = false, str post = NULL, vec<str> *eh = NULL)
  { get_T (fn, cb, v, c, https, post, eh); }

private:
  void get_T (const str &fn, simple_ev_t cb, int v, cgi_t *c, bool s, 
	      str post, vec<str> *eh, CLOSURE);
};

//-----------------------------------------------------------------------

class agent_get_proxied_t : public agent_get_t {
public:
  agent_get_proxied_t (const str &h, int p, ptr<obj_factory_t> f = NULL) 
    : agent_get_t (h, p, f) {}
  
  void 
  get (const str &url, simple_ev_t cb, int v = 1, cgi_t *c = NULL, 
       bool https = false, str post = NULL, vec<str> *eh = NULL)
  { get_T (url, cb, v, c, https, post, eh); }

private:
  void get_T (const str &fn, simple_ev_t cb, int v, cgi_t *c, bool s, 
	      str post, vec<str> *eh, CLOSURE);
};

//-----------------------------------------------------------------------

};

#endif /* _LIBWEB_OKWC3_H */
