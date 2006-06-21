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

struct queued_cbhent_t {
  queued_cbhent_t (cbhent c, bool i) 
    : _cb (c), _in_charge (i), _in_list (true) {}
  cbhent _cb;
  tailq_entry<queued_cbhent_t> _link;
  bool _in_charge;
  bool _in_list;
};

class okwc2_dnscache_entry_t : public virtual refcount {
public:
  okwc2_dnscache_entry_t (const str &h, int t = 60) : 
    _hostname (h), _expires (0), _resolving (false), _ttl (t), _err (0), 
    _init (false) {}
  void lookup (cbhent cb, ptr<canceller_t> cncl, CLOSURE);
private:
  void dnscb (ptr<hostent> he, int status);
  str _hostname;
  ptr<hostent> _he;
  time_t _expires;
  bool _resolving;
  int _ttl;
  int _err;
  bool _init;

  tailq<queued_cbhent_t, &queued_cbhent_t::_link> _cbq;
};

class okwc2_dnscache_t {
public:
  okwc2_dnscache_t () {}
  ptr<canceller_t> lookup (const str &n, cbhent cb);
private:
  qhash<str, ptr<okwc2_dnscache_entry_t> > _cache;
};

class okwc2_post_t {
public:
  okwc2_post_t () {}
  virtual size_t len () const = 0;
  virtual void output (strbuf &b) const = 0;
};

class okwc2_req_t : public virtual refcount {
public:
  okwc2_req_t (const str &hn, const str &fn, int v = 1, cgi_t *c = NULL) 
    : _hostname (hn), _filename (fn), _vers (v), _outcookie (c) {}

  virtual ~okwc2_req_t () {}
  virtual void cancel () { _c.cancel (); }
  virtual void too_late_to_cancel () { _c.toolate (); }
  virtual void notify_on_cancel (cbv cb) { _c.wait (cb); }
  virtual ptr<canceller_t> make (ptr<ahttpcon> x, cbi cb) 
  { return make_T (x, cb); }

  virtual okwc2_post_t *get_post () const { return NULL; }
  virtual str get_type () const { return NULL; }

protected:
  void fix_filename ();
  void format_req (strbuf &b);

private:
  ptr<canceller_t> make_T (ptr<ahttpcon> x, cbi cb, CLOSURE);

  str _hostname;
  str _filename;
  canceller_t _c;
  int _vers;
  cgi_t *_outcookie; // cookie sending out to the server
};

class okwc2_resp_t : public virtual refcount {
public:
  okwc2_resp_t (ptr<ahttpcon> x);
  virtual ~okwc2_resp_t () {}
  ptr<canceller_t> get (cbi cb) { return get_T (cb); }
protected:

  virtual void run_chunker (cbi cb) { run_chunker_T (cb); }
  virtual void get_body (cbi cb) { get_body_T (cb); }

  virtual void eat_chunk (size_t, cbi cb) = 0;
  virtual void finished_meal (int status, cbi cb) = 0;

  ptr<ahttpcon> _x;
  abuf_t _abuf;
  char _scratch[OKWC_SCRATCH_SZ];
  okwc_cookie_set_t _incookies;
  okwc_http_hdr_t _hdr;

private:
  void get_body_T (cbi cb, CLOSURE);
  ptr<canceller_t> get_T (cbi cb, CLOSURE);
  void run_chunker_T (cbi cb, CLOSURE);
};

class okwc2_resp_bigstr_t : public okwc2_resp_t {
public:
  okwc2_resp_bigstr_t (ptr<ahttpcon> x) : okwc2_resp_t (x), _dumper (&_abuf) {}
  void eat_chunk (size_t sz, cbi cb) { eat_chunk_T (sz, cb); }
  void finished_meal (int status, cbi cb);
  const str & body () const { return _body; }
protected:
  async_dumper_t _dumper;
  vec<str> _chunks;
  str _body;
private:
  void eat_chunk_T (size_t sz, cbi cb, CLOSURE);
};

typedef callback<void, int, ptr<okwc2_resp_t> >::ref okwc2_cb_t;

class okwc2_t : public virtual refcount {
public:
  okwc2_t (const str &h, int p) : _hostname (h), _port (p) {}
  virtual void req (ptr<okwc2_req_t> req, okwc2_cb_t cb) { req_T (req, cb); }
  virtual void timed_req (ptr<okwc2_req_t> req, int to, okwc2_cb_t cb)
  { timed_req_T (req, to, cb); }

  virtual ptr<okwc2_resp_t> alloc_resp (ptr<ahttpcon> x) = 0;
private:
  void req_T (ptr<okwc2_req_t> req, okwc2_cb_t cb, CLOSURE);
  void timed_req_T (ptr<okwc2_req_t> req, int to, okwc2_cb_t cb, CLOSURE);
  const str _hostname;
  int _port;
};

class okwc2_bigstr_t : public okwc2_t {
public:
  okwc2_bigstr_t (const str &h, int p) : okwc2_t (h, p) {}
  ptr<okwc2_resp_t> alloc_resp (ptr<ahttpcon> x) 
  { return New refcounted<okwc2_resp_bigstr_t> (x); }
};

#endif /* _LIBWEB_OKWC2_H */
