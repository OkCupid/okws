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

//
// this global variable can be set to whatever the user wants.
//
extern str global_okws_server_label;

class http_status_t {
public:
  http_status_t (int n, const str &sd, const str &ld)
    : status (n), sdesc (sd), ldesc (ld) {}
  int status;
  const str sdesc;
  const str ldesc;
  ihash_entry<http_status_t> lnk;
};

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

extern http_status_set_t http_status;

class http_hdr_field_t {
public:
  http_hdr_field_t (const str &n, const str &v) : name (n), val (v) {}
  http_hdr_field_t (const str &n, int64_t i) : name(n), val (strbuf () << i) {}
  str name;
  str val;
};

class http_hdr_date_t : public http_hdr_field_t {
public:
  http_hdr_date_t () : http_hdr_field_t ("Date", getdate ()) {}
};

class http_hdr_size_t : public http_hdr_field_t {
public:
  http_hdr_size_t (ssize_t s) : http_hdr_field_t ("Content-Length", s) {}
};

class http_hdr_cookie_t : public http_hdr_field_t {
public:
  http_hdr_cookie_t (const str &v) : http_hdr_field_t ("Set-Cookie", v) {}
};

class http_resp_attributes_t {
public:
  http_resp_attributes_t (u_int s, htpv_t v) : 
    _status (s), _version (v), _content_type ("text/html"), 
    _cache_control ("private"), 
    _gzip (false) {}

  u_int get_status () const { return _status; }
  htpv_t get_version () const { return _version; }
  str get_content_type () const { return _content_type; }
  str get_cache_control () const { return _cache_control; }
  str get_expires () const { return _expires; }
  str get_content_disposition () const { return _contdisp; }
  bool get_gzip () const { return _gzip; }
  bool get_others (vec<http_hdr_field_t> *output);

  void get_others (cbs cb);

  void set_version (htpv_t v) { _version = v; }
  void set_content_type (const str &s) { _content_type = s; }
  void set_cache_control (const str &s) { _cache_control = s; }
  void set_expires (const str &s) { _expires = s; }
  void set_gzip (bool b) { _gzip = b; }
  void set_content_disposition (const str s) { _contdisp = s; }
  void set_others (ptr<vec<http_hdr_field_t> > p ) { _others = p; }

  u_int _status;
  htpv_t _version;
  str _content_type;
  str _cache_control;
  str _expires;
  str _contdisp;
  bool _gzip;

  ptr<vec<http_hdr_field_t> > _others;

};

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
  virtual void fill (bool gz = false);
  void fill (bool gz, ssize_t len);
  strbuf to_strbuf () const;
  inline int get_status () const { return attributes.get_status (); }
  void gzip ();
  void add_date () { add (http_hdr_date_t ()); }
  void add_server () { add ("Server", global_okws_server_label); }
  void add_closed () { add ("Connection", "close"); }
private:
  http_resp_attributes_t attributes;
  vec<http_hdr_field_t> fields;
  bool cleanme;
};

class http_resp_header_redirect_t : public http_resp_header_t {
public:
  http_resp_header_redirect_t (const str &loc, const http_resp_attributes_t &a)
    : http_resp_header_t (a) { fill (loc); }
  void fill (const str &loc);
};

class http_resp_header_ok_t : public http_resp_header_t {
public:
  http_resp_header_ok_t (const http_resp_attributes_t &a)
    : http_resp_header_t (a) { fill (a.get_gzip ()); }
  http_resp_header_ok_t (ssize_t s, const http_resp_attributes_t &a)
    : http_resp_header_t (a) { fill (a.get_gzip (), s); }
};

class http_response_t {
public:
  http_response_t (const http_resp_header_t &h) 
    : header (h), nbytes (0), uid (0), inflated_len (0) {}
  http_response_t (const http_resp_header_t &h, const strbuf &b)
    : header (h), body (b), nbytes (b.tosuio ()->resid ()), uid (0),
      inflated_len (0) {}
  strbuf to_strbuf () const { return (header.to_strbuf () << body); }
  u_int send (ptr<ahttpcon> x, cbv::ptr cb) ;
  inline int get_status () const { return header.get_status (); }
  inline int get_nbytes () const { return nbytes; }
  inline size_t get_inflated_len () const { return inflated_len; }
  inline void gzip () { header.gzip (); }

  void set_inflated_len (size_t l) { inflated_len = l; }
  inline void set_uid (u_int64_t i) { uid = i; }
  inline u_int64_t get_uid () const { return uid; }

  http_resp_header_t header;
protected:
  strbuf body;
  u_int nbytes;
  u_int64_t uid;
  size_t inflated_len;
};

class http_response_ok_t : public http_response_t {
public:
  http_response_ok_t (const strbuf &b, const http_resp_attributes_t &a) :
    http_response_t (http_resp_header_ok_t (b.tosuio ()->resid (), a), b) 
  {}
  
  // for piece-meal output mode
  http_response_ok_t (size_t s, const http_resp_attributes_t &a) :
    http_response_t (http_resp_header_ok_t (s, a)) {}
};

class http_response_redirect_t : public http_response_t {
public:
  http_response_redirect_t (const str &s, const http_resp_attributes_t &a) :
    http_response_t (http_resp_header_redirect_t (s, a)) {}
};

class http_error_t : public http_response_t {
public:
  http_error_t (int n, const str &si, const str &aux = NULL, htpv_t v = 0)
    : http_response_t (http_resp_header_t (n, v), make_body (n, si, aux)) 
  { header.fill (); }
  static strbuf make_body (int n, const str &si, const str &aux);
};

class http_pub_t : public http_response_t {
public:
  http_pub_t (int n, const pub_base_t &p, const str &fn, aarr_t *env = NULL,
	      htpv_t v = 0, bool gz = false) 
    : http_response_t (http_resp_header_t (n, v)), zb (),
      err (!p.include (&zb, fn, 0, env))
  { if (!err) zb.to_strbuf (&body, v > 0 && gz); }
     
  zbuf zb;
  bool err;
  static ptr<http_pub_t> alloc (int n, const pub_base_t &p, const str &fn,
				aarr_t *env = NULL, htpv_t v = 0);
};


#endif /* _LIBAHTTP_RESP */
