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

#ifndef _LIBWEB_OKWC2_H
#define _LIBWEB_OKWC2_H

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

//-----------------------------------------------------------------------

struct queued_cbv_t {
  queued_cbv_t (cbv cb) : _cb (cb) {}
  cbv _cb;
  tailq_entry<queued_cbv_t> _link;
};

typedef event_t<ptr<hostent>, int>::ref ev_hent_t;

//-----------------------------------------------------------------------

class okwc3_dnscache_entry_t : public virtual refcount {
public:
  okwc3_dnscache_entry_t (const str &h, int t = 60) : 
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

class okwc3_dnscache_t {
public:
  okwc3_dnscache_t () {}
  void lookup (const str &n, ev_hent_t ev);
private:
  qhash<str, ptr<okwc3_dnscache_entry_t> > _cache;
};

//-----------------------------------------------------------------------

class okwc3_post_t {
public:
  okwc3_post_t () {}
  virtual ~okwc3_post_t () {}
  virtual size_t len () const = 0;
  virtual void output (strbuf &b) const = 0;
};

//-----------------------------------------------------------------------

class okwc3_req_t : public virtual refcount {
public:
  okwc3_req_t (const str &hn, const str &fn, int v = 1, cgi_t *c = NULL) 
    : _hostname (hn), _filename (fn), _vers (v), _outcookie (c) {}

  virtual ~okwc3_req_t () {}
  virtual void make (ptr<ahttpcon> x, evi_t cb) { return make_T (x, cb); }

  virtual const okwc3_post_t *get_post () const { return NULL; }
  virtual str get_type () const { return NULL; }

protected:
  void fix_filename ();
  void format_req (strbuf &b);

private:
  void make_T (ptr<ahttpcon> x, evi_t cb, CLOSURE);

  str _hostname;
  str _filename;
  int _vers;
  cgi_t *_outcookie; // cookie sending out to the server
};

//-----------------------------------------------------------------------

class okwc3_resp_t : public virtual refcount {
public:
  okwc3_resp_t ();
  virtual ~okwc3_resp_t () {}
  void get (evi_t cb) { get_T (cb); }
  void setx (ptr<ahttpcon> x);
  const okwc_http_hdr_t *hdr () const { return &_hdr; }
  okwc_http_hdr_t *hdr () { return &_hdr; }

protected:

  virtual void run_chunker (evi_t cb) { run_chunker_T (cb); }
  virtual void get_body (evi_t cb) { get_body_T (cb); }

  virtual void eat_chunk (size_t, evi_t cb) = 0;
  virtual void finished_meal (int status, evi_t cb) = 0;

  ptr<ahttpcon> _x;
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

class okwc3_resp_simple_t : public okwc3_resp_t {
public:
  okwc3_resp_simple_t () : _dumper (&_abuf) {}
  void eat_chunk (size_t sz, evi_t cb) { eat_chunk_T (sz, cb); }
  void finished_meal (int status, evi_t cb);
  const str & body () const { return _body; }
protected:
  async_dumper_t _dumper;
  vec<str> _chunks;
  str _body;
private:
  void eat_chunk_T (size_t sz, evi_t cb, CLOSURE);
};

//-----------------------------------------------------------------------

class okwc3_t : public virtual refcount {
public:
  okwc3_t (const str &h, int p) : _hostname (h), _port (p) {}

  virtual void req (ptr<okwc3_req_t> req, ptr<okwc3_resp_t> resp, evi_t cb)
  { req_T (req, resp, cb); }

protected:
  const str _hostname;
  int _port;

private:
  void req_T (ptr<okwc3_req_t> req, ptr<okwc3_resp_t> resp, evi_t cb, CLOSURE);

};

//-----------------------------------------------------------------------

typedef event_t<int, ptr<okwc3_resp_simple_t> >::ref okwc3_simple_ev_t;

class okwc3_simple_t : public okwc3_t {
public:
  okwc3_simple_t (const str &h, int p) : okwc3_t (h, p) {}
  void req (str fn, okwc3_simple_ev_t cb, 
	    int v = 1, cgi_t *c = NULL, CLOSURE);
};

//-----------------------------------------------------------------------

#endif /* _LIBWEB_OKWC2_H */
