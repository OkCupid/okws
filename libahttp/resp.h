// -*-c++-*-
/* $Id$ */

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

#ifndef _LIBAHTTP_RESP
#define _LIBAHTTP_RESP

#include "ahutil.h"
#include "httpconst.h"
#include "ahttp.h"
#include "ihash.h"
#include "pub.h"
#include "pub3.h"
#include "inhdr.h"

//
// this global variable can be set to whatever the user wants.
//
extern str global_okws_server_label;

//-----------------------------------------------------------------------

class http_status_t {
public:
  http_status_t (int n, const str &sd, const str &ld)
    : status (n), sdesc (sd), ldesc (ld) {}
  int status;
  const str sdesc;
  const str ldesc;
  ihash_entry<http_status_t> lnk;
};

//-----------------------------------------------------------------------

class http_status_set_t {
public:
  http_status_set_t ();
  str get_desc (int n, str *sd) const;
  bool get_ok ();
  void add (int i, const str &s, const str &l = NULL)
  { tab.insert (New http_status_t (i, s, l)); }
  inline str operator[] (int n) 
  { 
    http_status_t *s = tab[n];
    return s ? s->sdesc : str ("");
  }
private:
  ihash<int, http_status_t, &http_status_t::status, &http_status_t::lnk> tab;
};

//-----------------------------------------------------------------------

extern http_status_set_t http_status;

//-----------------------------------------------------------------------

class http_hdr_field_t {
public:
  ~http_hdr_field_t () {}

  http_hdr_field_t (const str &n, const str &v, bool cd = false) 
    : name (n), val (v), _can_duplicate (cd) {}
  http_hdr_field_t (const str &n, int64_t i) 
    : name(n), val (strbuf () << i), _can_duplicate (false) {}

  bool can_duplicate () const { return _can_duplicate; }
  str name;
  str val;

protected:
  const bool _can_duplicate;
};

//-----------------------------------------------------------------------

class http_hdr_date_t : public http_hdr_field_t {
public:
  http_hdr_date_t () : http_hdr_field_t ("Date", getdate ()) {}
};

//-----------------------------------------------------------------------

class http_hdr_size_t : public http_hdr_field_t {
public:
  http_hdr_size_t (ssize_t s) : http_hdr_field_t ("Content-Length", s) {}
};

//-----------------------------------------------------------------------

class http_hdr_cookie_t : public http_hdr_field_t {
public:
  http_hdr_cookie_t (const str &v) 
    : http_hdr_field_t ("Set-Cookie", v, true) {}
};

//-----------------------------------------------------------------------

class http_resp_attributes_t {
public:
  http_resp_attributes_t (u_int s, htpv_t v, http_method_t m = HTTP_MTHD_GET) : 
    _status (s), _version (v), _content_type ("text/html"), 
    _cache_control ("private"), 
    _connection ("close"),
    _bad_chunking (false),
    _method (m) {}

  u_int get_status () const { return _status; }
  htpv_t get_version () const { return _version; }
  str get_content_type () const { return _content_type; }
  str get_cache_control () const { return _cache_control; }
  str get_expires () const { return _expires; }
  str get_content_disposition () const { return _contdisp; }
  bool get_others (vec<http_hdr_field_t> *output);
  str get_connection () const { return _connection;  }
  bool get_chunking_support () const { return !_bad_chunking; }
  http_method_t get_method () const { return _method; }

  const compressible_t::opts_t get_content_delivery () const 
  { return _content_delivery; }
  compressible_t::opts_t &get_content_delivery () 
  { return _content_delivery; }

  void get_others (cbs cb);

  void set_bad_chunking (bool b) { _bad_chunking = b; }
  void set_version (htpv_t v) { _version = v; }
  void set_status (int i) { _status = i; }
  void set_content_type (const str &s) { _content_type = s; }
  void set_cache_control (const str &s) { _cache_control = s; }
  void set_expires (const str &s) { _expires = s; }
  void set_content_disposition (const str s) { _contdisp = s; }
  void set_others (ptr<vec<http_hdr_field_t> > p ) { _others = p; }
  void set_connection (str s) { _connection = s; }
  void set_content_delivery (compressible_t::opts_t o) {_content_delivery = o;}

  u_int _status;
  htpv_t _version;
  str _content_type;
  str _cache_control;
  str _expires;
  str _contdisp;
  str _connection;
  bool _bad_chunking;
  http_method_t _method;
  compressible_t::opts_t _content_delivery;

  ptr<vec<http_hdr_field_t> > _others;

};

//-----------------------------------------------------------------------

class http_resp_header_t {
public:
  http_resp_header_t (const http_resp_attributes_t &a) 
    : attributes (a), cleanme (false) {}
  http_resp_header_t (u_int s, htpv_t v) 
    : attributes (s, v), cleanme (false) {}
  virtual ~http_resp_header_t () {}
  const http_resp_header_t & add (const http_hdr_field_t &f)
  { fields.push_back (f); return *this; }
  const http_resp_header_t & add (const str &n, const str &v)
  { fields.push_back (http_hdr_field_t (n, v)); return *this; }
  const http_resp_header_t & add (const str &n, int64_t i) 
  { fields.push_back (http_hdr_field_t (n, i)); return *this; }
  virtual void fill ();
  void fill_outer (ssize_t len);
  strbuf to_strbuf () const;
  void fill_strbuf (strbuf &b) const;
  inline int get_status () const { return attributes.get_status (); }
  void add_content_delivery_headers ();
  void add_date () { add (http_hdr_date_t ()); }
  void add_server ();
  void add_connection ();
  htpv_t get_version () const { return attributes.get_version (); }
  const http_resp_attributes_t &get_attributes () const { return attributes; }
  http_resp_attributes_t &get_attributes () { return attributes; }
  bool is_head_request () const 
  { return attributes.get_method () == HTTP_MTHD_HEAD; }
  void disable_gzip ();

protected:
  http_resp_attributes_t attributes;
  vec<http_hdr_field_t> fields;
  bool cleanme;
};

//-----------------------------------------------------------------------

class http_resp_header_redirect_t : public http_resp_header_t {
public:
  http_resp_header_redirect_t (const str &loc, const http_resp_attributes_t &a)
    : http_resp_header_t (a) { fill (loc); }
  void fill (const str &loc);
};

//-----------------------------------------------------------------------

class http_resp_header_ok_t : public http_resp_header_t {
public:
  http_resp_header_ok_t (ssize_t s, const http_resp_attributes_t &a);
};

//-----------------------------------------------------------------------

typedef event<ssize_t>::ref ev_ssize_t;

//-----------------------------------------------------------------------

/**
 * a new base class for responses version 2
 */
class http_response_base_t {
public:
  http_response_base_t ()
    : _uid (0),
      _inflated_len (0) {}

  virtual ~http_response_base_t () {}

  // accessors to header information --------------------------------------
  virtual const http_resp_header_t *get_header () const = 0;
  virtual http_resp_header_t *get_header () = 0;

  // the all-import output function ---------------------------------------
  virtual u_int send (ptr<ahttpcon> x, cbv::ptr cb) = 0;
  virtual void send2 (ptr<ahttpcon> x, ev_ssize_t ev) = 0;

  // fixup logging information --------------------------------------------
  virtual void set_uid (u_int64_t u) { _uid = u; }
  virtual void set_inflated_len (size_t s) { _inflated_len = s; }
  virtual void set_custom_log2 (const str &s) { _custom_log2 = s; }

  // access for logging info ---------------------------------------------
  virtual u_int64_t get_uid () const { return _uid; }
  virtual size_t get_inflated_len () const { return _inflated_len; }
  virtual str get_custom_log2 () const { return _custom_log2; }

  // access for response ------------------------------------------------
  int get_status () const { return get_header ()->get_status (); }
  virtual size_t get_nbytes () const = 0;

private:
  u_int64_t _uid;
  size_t _inflated_len;
  str _custom_log2;

};

//-----------------------------------------------------------------------

class http_response_t : public http_response_base_t {
public:
  http_response_t (const http_resp_header_t &h) 
    : header (h), nbytes (0), uid (0), inflated_len (0) {}
  http_response_t (const http_resp_header_t &h, const strbuf &b)
    : header (h), body (b), nbytes (b.tosuio ()->resid ()), uid (0),
      inflated_len (0) {}
  strbuf to_strbuf () const;
  u_int send (ptr<ahttpcon> x, cbv::ptr cb) ;
  size_t get_nbytes () const { return nbytes; }
  void send2 (ptr<ahttpcon> x, ev_ssize_t ev) { send2_T (x, ev); }

  const http_resp_header_t *get_header () const { return &header; }
  http_resp_header_t *get_header () { return &header; }
  bool is_head_request () const { return header.is_head_request (); }

  http_resp_header_t header;
protected:

  void send2_T (ptr<ahttpcon> x, ev_ssize_t ev, CLOSURE);

  strbuf body;
  size_t nbytes;
  u_int64_t uid;
  size_t inflated_len;
  str _custom_log2;
};

//-----------------------------------------------------------------------

class http_response_ok_t : public http_response_t {
public:
  http_response_ok_t (const strbuf &b, const http_resp_attributes_t &a) :
    http_response_t (http_resp_header_ok_t (b.tosuio ()->resid (), a), b) 
  {}

//-----------------------------------------------------------------------
  
  // for piece-meal output mode
  http_response_ok_t (size_t s, const http_resp_attributes_t &a) :
    http_response_t (http_resp_header_ok_t (s, a)) {}
};

//-----------------------------------------------------------------------

class http_response_redirect_t : public http_response_t {
public:
  http_response_redirect_t (const str &s, const http_resp_attributes_t &a) :
    http_response_t (http_resp_header_redirect_t (s, a)) {}
};

//-----------------------------------------------------------------------

class http_error_t : public http_response_t {
public:

  http_error_t (int n, const str &si, const str &aux = NULL, htpv_t v = 0);
  http_error_t (const http_resp_attributes_t &hra, const str &si,
		const str &aux = NULL);
  static strbuf make_body (int n, const str &si, const str &aux);
};

//-----------------------------------------------------------------------

class http_pub_t : public http_response_t {
public:
  http_pub_t (int n, htpv_t v = 0)
    : http_response_t (http_resp_header_t (n, v)) {} 
  http_pub_t (const http_resp_attributes_t &ha)
    : http_response_t (http_resp_header_t (ha)) {}

  void publish (ptr<pub3::remote_publisher_t> p, str fn,
		evb_t ev, 
		ptr<pub3::dict_t> env = NULL,
		htpv_t v = 0, 
		compressible_t::opts_t o = compressible_t::opts_t (), 
		CLOSURE);

  static void
  alloc2 (ptr<pub3::remote_publisher_t> p, 
	  const http_resp_attributes_t &hra, str fn,
	  event<ptr<http_pub_t> >::ref cb,
	  ptr<pub3::dict_t> env = NULL, CLOSURE);

  vec<str> _hold;

};

//-----------------------------------------------------------------------

gzip_mode_t 
ok_gzip_get_mode (const compressible_t &b, int v, bool do_gzip = true);

#endif /* _LIBAHTTP_RESP */
