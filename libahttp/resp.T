//-*-c++-*-

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

#include "resp.h"
#include "httpconst.h"
#include "parseopt.h" 

http_status_set_t http_status;
//
// what this OKWS is called when sending responses back to clients;
// we can get sneaky here and call ourselves apache, for instance.
//
str global_okws_server_label = okws_server_label;

http_status_set_t::http_status_set_t ()
{
  // 100 series
  add (100, "Continue");
  add (101, "Switching Protocols");

  // 200 series
  add (HTTP_OK, "OK");
  add (201, "Created");
  add (202, "Accepted");
  add (203, "Non-Authoratative Information");
  add (204, "No Content");
  add (205, "Resent Content");
  add (HTTP_PARTIAL_CONTENT, "Partial Content");

  // 300 series
  add (300, "Multiple  Choices");
  add (HTTP_MOVEDPERM, "Moved Permanently");
  add (HTTP_REDIRECT, "Found"); 
  add (HTTP_SEEOTHER, "See Other");
  add (HTTP_NOT_MODIFIED, "Not Modified");
  add (HTTP_USEPROXY, "Use Proxy");
  add (307, "Temporary Redirect");
  add (HTTP_BAD_REQUEST, "Bad Request");

  // 400 series
  add (401, "Unauthorized");
  add (402, "Payment Required");
  add (HTTP_FORBIDDEN, "Forbidden");
  add (HTTP_NOT_FOUND, "Not Found");
  add (HTTP_NOT_ALLOWED, "Method Not Allowed");
  add (406, "Not Acceptable");
  add (407, "Proxy Authentication Required");
  add (HTTP_TIMEOUT, "Request Time-out");
  add (409, "Conflict");
  add (HTTP_GONE, "Gone");
  add (411, "Length Required");
  add (412, "Precondition Failed");
  add (HTTP_REQ_TOO_BIG, "Request Entity Too Large");
  add (HTTP_URI_TOO_BIG, "Request-URI Too Large");
  add (415, "Unsupported Media Type");
  add (416, "Requested range not satisfied");
  add (HTTP_UNEXPECTED_EOF, "Expectation Failed");

  // 500 series
  add (HTTP_SRV_ERROR, "Internal Server Error");
  add (HTTP_NOT_IMPLEMENTED, "Not Implemented");
  add (HTTP_BAD_GATEWAY, "Bad Gateway");
  add (HTTP_UNAVAILABLE, "Service Unavailable");
  add (504, "Gateway Time-out");
  add (505, "HTTP Version not supported");
  add (HTTP_CLIENT_EOF, "Client EOF before response sent.");
}

//-----------------------------------------------------------------------

void
http_resp_header_t::fill ()
{
  add_date ();
  add ("Content-Type", attributes.get_content_type ());
  add_connection ();
  add ("Cache-control", attributes.get_cache_control ());
  str tmp;
  if ((tmp = attributes.get_expires ()))  
    add ("Expires", tmp);
  if ((tmp = attributes.get_content_disposition ()))  
    add ("Content-Disposition", tmp);
  add_server ();
  add_content_delivery_headers ();

  // anything else the user might have added; takes precedence
  // over anything put above
  cleanme = attributes.get_others (&fields);
}

//-----------------------------------------------------------------------

void
http_resp_header_t::add_connection ()
{
  add ("Connection", attributes.get_connection ());
}

//-----------------------------------------------------------------------

void
http_resp_header_redirect_t::fill (const str &s)
{
  add_date ();
  add_connection ();
  add_server ();
  add ("Location", s);

  // anything else the user might have added; takes precedence
  // over anything put above
  cleanme = attributes.get_others (&fields);
}

//-----------------------------------------------------------------------

void
http_resp_header_t::fill_outer (ssize_t len)
{
  fill ();
  if (!attributes.get_content_delivery().chunked && len >= 0) {
    add (http_hdr_size_t (len));
  }
}

//-----------------------------------------------------------------------

void
http_resp_header_t::add_content_delivery_headers ()
{
  if (attributes.get_content_delivery ().mode != GZIP_NONE) {
    add ("Content-Encoding", "gzip");
  }
  if (attributes.get_content_delivery ().chunked) {
    add ("Transfer-Encoding", "chunked");
  }
}

//-----------------------------------------------------------------------

void
http_resp_header_t::add_server() {
    add ("Server", global_okws_server_label);
    add ("X-OKWS-Version", global_okws_server_label);
}

//-----------------------------------------------------------------------

strbuf
http_resp_header_t::to_strbuf () const
{
  strbuf b;
  fill_strbuf (b);
  return b;
}

//-----------------------------------------------------------------------

void
http_resp_header_t::fill_strbuf (strbuf &b) const
{
  vec<bool> output_me;

  b << "HTTP/";
  switch (attributes.get_version ()) {
  case 1:
    b << "1.1 ";
    break;
  default:
    b << "1.0 ";
    break;
  }
  u_int status = attributes.get_status ();
  b << status;
  if (status == HTTP_OK)
    b << " OK";
  b << HTTP_CRLF;
  int lim = fields.size ();

  // 2 of the same field in, last in wins;
  // eventually, some fields might allow multiple winners;
  // this would be pretty easy to set up.
  if (cleanme) {
    output_me.setsize (lim);
    bhash<str> winner_hsh;
    for (int i = lim - 1; i >= 0; i--) {
      if (fields[i].can_duplicate ()) {
        output_me[i] = true;
      } else {
        str n = mytolower (fields[i].name);
        if (winner_hsh[n]) {
          output_me[i] = false;
        } else {
          winner_hsh.insert (n);
          output_me[i] = true;
        }
      }
    }
  }

  for (int i = 0; i < lim; i++) {
    if (!cleanme || output_me[i]) {
      const str &name = fields[i].name;
      const str &val = fields[i].val;
      // If the header's name or value contains a newline, drop it on the floor
      if (
        !strchr(name.cstr(), '\r') && !strchr(name.cstr(), '\n') &&
        !strchr(val.cstr(), '\r') && !strchr(val.cstr(), '\n')
      ) { b << name << ": " << val << HTTP_CRLF; }
    }
  }
  b << HTTP_CRLF;

}

//-----------------------------------------------------------------------

str
http_status_set_t::get_desc (int n, str *l) const
{
  http_status_t *s = tab[n];
  if (!s) {
    if (l) *l = "";
    return "";
  }
  if (l) *l = s->ldesc;
  return s->sdesc;
}

//-----------------------------------------------------------------------

u_int
http_response_t::send (ptr<ahttpcon> x, cbv::ptr cb)
{ 
  const strbuf &b = to_strbuf ();
  u_int ret = b.tosuio ()->resid ();
  x->send (b, cb); 
  return ret;
}

//-----------------------------------------------------------------------

tamed void 
http_response_t::send2_T (ptr<ahttpcon> x, ev_ssize_t ev)
{
  tvars {
    ssize_t ret;
    strbuf b;
  }

  header.fill_strbuf (b);
  b.take (body);

  twait { 
    x->send2 (b, mkevent (ret));
  }

  ev->trigger (ret);
}

//-----------------------------------------------------------------------

bool
http_resp_attributes_t::get_others (vec<http_hdr_field_t> *out)
{
  if (_others) {
    for (u_int i = 0; i < _others->size (); i++) {
      out->push_back ((*_others)[i]);
    }
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------

tamed void
http_pub_t::publish (ptr<pub3::remote_publisher_t> p, str fn,
		     evb_t ev, ptr<pub3::dict_t> env, htpv_t v,
		     compressible_t::opts_t o)
{
  tvars {
    zbuf b;
    bool rc;
  }
  twait { p->run (&b, fn, mkevent (rc), env); }
  if (rc) {
    b.to_strbuf (&body, o);
  }

  // Fill in the headers once we've published the body...
  header.fill_outer (body.len ());
  ev->trigger (rc);
}

//-----------------------------------------------------------------------

typedef event<ptr<http_pub_t> >::ref  http_pub2_ev_t;

//-----------------------------------------------------------------------

tamed static void
http_pub_t::alloc2 (ptr<pub3::remote_publisher_t> p, 
		     const http_resp_attributes_t &hra, str fn, 
		     http_pub2_ev_t ev, ptr<pub3::dict_t> env)
{
  tvars {
    ptr<http_pub_t> ret (New refcounted<http_pub_t> (hra));
    bool rc;
  }
  twait { 
    ret->publish (p, fn, mkevent (rc), env, hra.get_version (), 
		  hra.get_content_delivery ());
  }
  ev->trigger (rc ? ret : NULL);
}

//-----------------------------------------------------------------------

http_error_t::http_error_t (int n, const str &si, const str &aux, htpv_t v)
  : http_response_t (http_resp_header_t (n, v), make_body (n, si, aux)) 
{
  // If we're making a default body (via make_body), then disable
  // gzip in the reply.
  header.disable_gzip ();
  header.fill_outer (nbytes);
}

//-----------------------------------------------------------------------

http_error_t::http_error_t (const http_resp_attributes_t &hra, 
			    const str &si, const str &aux)
  : http_response_t (http_resp_header_t (hra), 
		     make_body (hra.get_status (), si, aux)) 
{
  // If we're making a default body (via make_body), then disable
  // gzip in the reply.
  header.disable_gzip ();
  header.fill_outer (nbytes);
}

//-----------------------------------------------------------------------

gzip_mode_t 
ok_gzip_get_mode (const compressible_t &b, int v, bool do_gzip)
{
  bool do_gz = b.inflated_len () > 0 && v > 0 && do_gzip;
  return do_gz ? ok_gzip_mode : GZIP_NONE;
}

//-----------------------------------------------------------------------

http_resp_header_ok_t::http_resp_header_ok_t (ssize_t s, 
					      const http_resp_attributes_t &a)
  : http_resp_header_t (a) 
{ 
  fill_outer (s);
}

//-----------------------------------------------------------------------

strbuf 
http_response_t::to_strbuf () const 
{
  strbuf b = header.to_strbuf ();
  if (!is_head_request ()) {
    b << body;
  }
  return b;
}

//-----------------------------------------------------------------------

void
http_resp_header_t::disable_gzip ()
{
  attributes.get_content_delivery ().mode = GZIP_NONE;
}

//-----------------------------------------------------------------------
