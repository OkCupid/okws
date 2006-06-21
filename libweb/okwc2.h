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

class okwc2_post_t {
public:
  okwc2_post_t () {}
  virtual size_t len () const = 0;
  virtual void output (strbuf &b) const = 0;
};

class okwc2_req_t : public refcount {
public:
  okwc2_req_t (const str &hn, const str &fn, int v = 1, cgi_t *c = NULL) 
    : _hostname (hn), _filename (fn), _vers (v), _outcookie (c) {}

  virtual ~okwc2_req_t () {}
  virtual void cancel () { _c.cancel (); }
  virtual void too_late_to_cancel () { _c.toolate (); }
  virtual void notify_on_cancel (cbv cb) { _c.wait (cb); }
  virtual void make (ptr<ahttpcon> x, cbi cb) { make_T (x, cb); }

  virtual okwc2_post_t *get_post () const = 0;
  virtual str get_type () const = 0;

protected:
  void fix_filename ();
  void format_req (strbuf &b);

private:
  void make_T (ptr<ahttpcon> x, cbi cb, CLOSURE);

  str _hostname;
  str _filename;
  canceller_t _c;
  int _vers;
  cgi_t *_outcookie; // cookie sending out to the server
};

class okwc2_resp_t : public refcount {
public:
  virtual ~okwc2_resp_t () {}
  virtual void get (ptr<okwc2_req_t> r, ptr<ahttpcon> x, cbi s) = 0;
};

typedef callback<void, int, ptr<okwc2_resp_t> >::ref okwc2_cb_t;

class okwc2_t : public refcount {
public:
  okwc2_t (const str &h, int p) : _hostname (h), _port (p) {}
  virtual void req (ptr<okwc2_req_t> req, okwc2_cb_t cb) { req_T (req, cb); }
  virtual ptr<okwc2_resp_t> alloc_resp () = 0;
private:
  void req_T (ptr<okwc2_req_t> req, okwc2_cb_t cb, CLOSURE);
  const str _hostname;
  int _port;
};

#endif /* _LIBWEB_OKWC2_H */
