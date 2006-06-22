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

//-----------------------------------------------------------------------

struct queued_cbv_t {
  queued_cbv_t (cbv cb) : _cb (cb) {}
  cbv _cb;
  tailq_entry<queued_cbv_t> _link;
};

//-----------------------------------------------------------------------

class okwc2_dnscache_entry_t : public virtual refcount {
public:
  okwc2_dnscache_entry_t (const str &h, int t = 60) : 
    _hostname (h), _expires (0), _resolving (false), _ttl (t), _err (0), 
    _init (false) {}
  void lookup (ptr<canceller_t> cncl, cbhent cb, CLOSURE);
private:
  void wait_for_resolution (ptr<canceller_t>, cbb cb, CLOSURE);
  void do_resolution (ptr<canceller_t> cncl, cbb cb, CLOSURE);

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

class okwc2_dnscache_t {
public:
  okwc2_dnscache_t () {}
  void lookup (ptr<canceller_t> cncl, const str &n, cbhent cb);
private:
  qhash<str, ptr<okwc2_dnscache_entry_t> > _cache;
};

//-----------------------------------------------------------------------

class okwc2_post_t {
public:
  okwc2_post_t () {}
  virtual size_t len () const = 0;
  virtual void output (strbuf &b) const = 0;
};

//-----------------------------------------------------------------------

class okwc2_req_t : public virtual refcount {
public:
  okwc2_req_t (const str &hn, const str &fn, int v = 1, cgi_t *c = NULL) 
    : _hostname (hn), _filename (fn), _vers (v), _outcookie (c) {}

  virtual ~okwc2_req_t () {}
  virtual void make (ptr<canceller_t> cncl, ptr<ahttpcon> x, cbi cb) 
  { return make_T (cncl, x, cb); }

  virtual okwc2_post_t *get_post () const { return NULL; }
  virtual str get_type () const { return NULL; }

protected:
  void fix_filename ();
  void format_req (strbuf &b);

private:
  void make_T (ptr<canceller_t> cncl, ptr<ahttpcon> x, cbi cb, CLOSURE);

  str _hostname;
  str _filename;
  int _vers;
  cgi_t *_outcookie; // cookie sending out to the server
};

//-----------------------------------------------------------------------

class okwc2_resp_t : public virtual refcount {
public:
  okwc2_resp_t ();
  virtual ~okwc2_resp_t () {}
  void get (ptr<canceller_t> cncl, cbi cb) { get_T (cncl, cb); }
  void setx (ptr<ahttpcon> x);
  const okwc_http_hdr_t *hdr () const { return &_hdr; }
  okwc_http_hdr_t *hdr () { return &_hdr; }

protected:

  virtual void run_chunker (ptr<canceller_t> cncl, cbi cb) 
  { run_chunker_T (cncl, cb); }
  virtual void get_body (ptr<canceller_t> cncl, cbi cb) 
  { get_body_T (cncl, cb); }

  virtual void eat_chunk (ptr<canceller_t> cncl, size_t, cbi cb) = 0;
  virtual void finished_meal (ptr<canceller_t> cncl, int status, cbi cb) = 0;

  ptr<ahttpcon> _x;
  abuf_t _abuf;
  char _scratch[OKWC_SCRATCH_SZ];
  okwc_cookie_set_t _incookies;
  okwc_http_hdr_t _hdr;

private:
  void get_body_T (ptr<canceller_t> cncl, cbi cb, CLOSURE);
  void get_T (ptr<canceller_t> cncl, cbi cb, CLOSURE);
  void run_chunker_T (ptr<canceller_t> cncl, cbi cb, CLOSURE);
};

//-----------------------------------------------------------------------

class okwc2_resp_simple_t : public okwc2_resp_t {
public:
  okwc2_resp_simple_t () : _dumper (&_abuf) {}
  void eat_chunk (ptr<canceller_t> cncl, size_t sz, cbi cb) 
  { eat_chunk_T (cncl, sz, cb); }
  void finished_meal (ptr<canceller_t> cncl, int status, cbi cb);
  const str & body () const { return _body; }
protected:
  async_dumper_t _dumper;
  vec<str> _chunks;
  str _body;
private:
  void eat_chunk_T (ptr<canceller_t> cncl, size_t sz, cbi cb, CLOSURE);
};

//-----------------------------------------------------------------------

class okwc2_t : public virtual refcount {
public:
  okwc2_t (const str &h, int p) : _hostname (h), _port (p) {}

  virtual ptr<canceller_t> 
  req (ptr<okwc2_req_t> req, ptr<okwc2_resp_t> resp, cbi cb)
  { return req_T (req, resp, cb); }

  virtual void 
  timed_req (ptr<okwc2_req_t> req, ptr<okwc2_resp_t> resp, int to, cbi cb)
  { timed_req_T (req, resp, to, cb); }

protected:
  const str _hostname;
  int _port;

private:
  ptr<canceller_t> req_T (ptr<okwc2_req_t> req, 
			  ptr<okwc2_resp_t> resp, cbi cb, CLOSURE);
  void timed_req_T (ptr<okwc2_req_t> req, ptr<okwc2_resp_t> resp,
		    int to, cbi cb, CLOSURE);

};

//-----------------------------------------------------------------------

typedef callback<void, int, ptr<okwc2_resp_simple_t> >::ref okwc2_simple_cb_t;

class okwc2_simple_t : public okwc2_t {
public:
  okwc2_simple_t (const str &h, int p) : okwc2_t (h, p) {}
  void req (str fn, okwc2_simple_cb_t cb, int to = 0, 
	    int v = 1, cgi_t *c = NULL, CLOSURE);
};

//-----------------------------------------------------------------------

#endif /* _LIBWEB_OKWC2_H */
