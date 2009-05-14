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
  add (HTTP_REDIRECT, "Moved Permanently");
  add (HTTP_MOVEDTEMP, "Found"); 
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
  add (502, "Bad Gateway");
  add (HTTP_UNAVAILABLE, "Service Unavailable");
  add (504, "Gateway Time-out");
  add (505, "HTTP Version not supported");
  add (HTTP_CLIENT_EOF, "Client EOF before response sent.");
}

void
http_resp_header_t::fill (bool gz)
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
  if (gz) gzip ();

  // anything else the user might have added; takes precedence
  // over anything put above
  cleanme = attributes.get_others (&fields);
}

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
http_resp_header_t::fill (bool gz, ssize_t len)
{
  fill (gz);
  if (!gz && len >= 0) 
    add (http_hdr_size_t (len));
}

//-----------------------------------------------------------------------

void
http_resp_header_t::gzip ()
{
  add ("Content-Encoding", "gzip");
  add ("Transfer-Encoding", "chunked");
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
    if (!cleanme || output_me[i])
      b << fields[i].name << ": " << fields[i].val << HTTP_CRLF;
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

ptr<http_pub_t>
http_pub_t::alloc (int n, const pub_base_t &p, const str &fn, aarr_t *env, 
		   htpv_t v)
{
  ptr<http_pub_t> ret (New refcounted<http_pub_t> (n, p, fn, env, v));
  if (ret->err) return NULL;
  else return ret;
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
http_pub2_t::publish (ptr<pub2::remote_publisher_t> p, str fn,
		      cbb cb, aarr_t *env, htpv_t v,
		      bool gz)
{
  tvars {
    zbuf b;
    bool rc;
  }
  twait { p->run (&b, fn, mkevent (rc), env, P_WSS); }
  if (rc) {
    gzip_mode_t gzm = ok_gzip_get_mode (b, v);
    b.compressible_t::to_strbuf (&body, &_hold, gzm);
  }
  if (gz) gzip ();
  TRIGGER (cb, rc);
}

//-----------------------------------------------------------------------

typedef event<ptr<http_pub2_t> >::ref  http_pub2_ev_t;

tamed static void
http_pub2_t::alloc (ptr<pub2::remote_publisher_t> p, int n, str fn,
		    http_pub2_ev_t ev, aarr_t *env, htpv_t v, bool gz)
{
  tvars {
    ptr<http_pub2_t> ret (New refcounted<http_pub2_t> (n, v)); 
    bool rc;
  }
  twait { ret->publish (p, fn, mkevent (rc), env, v, gz); }
  ev->trigger (rc ? ret : NULL);
}

//-----------------------------------------------------------------------

tamed static void
http_pub2_t::alloc2 (ptr<pub2::remote_publisher_t> p, 
		     const http_resp_attributes_t &hra, str fn, 
		     http_pub2_ev_t ev, aarr_t *env)
{
  tvars {
    ptr<http_pub2_t> ret (New refcounted<http_pub2_t> (hra));
    bool rc;
  }
  twait { 
    ret->publish (p, fn, mkevent (rc), env, hra.get_version (), 
		  hra.get_gzip ());
  }
  ev->trigger (rc ? ret : NULL);
}

//-----------------------------------------------------------------------

http_error_t::http_error_t (int n, const str &si, const str &aux, htpv_t v)
  : http_response_t (http_resp_header_t (n, v), make_body (n, si, aux)) 
{ 
  header.fill (false, nbytes); 
}

//-----------------------------------------------------------------------

http_error_t::http_error_t (const http_resp_attributes_t &hra, 
			    const str &si, const str &aux)
  : http_response_t (http_resp_header_t (hra), 
		     make_body (hra.get_status (), si, aux)) 
{
  header.fill (false, nbytes); 
}

//-----------------------------------------------------------------------

http_pub_t::http_pub_t (int n, const pub_base_t &p, const str &fn, 
			aarr_t *env, htpv_t v, bool gz)
  : http_response_t (http_resp_header_t (n, v)), zb (),
    err (!p.include (&zb, fn, 0, env))
{
  if (!err)  {
    gzip_mode_t gzm = ok_gzip_get_mode (zb, v);
    zb.compressible_t::to_strbuf (&body, &_hold, gzm);
  }
}


//-----------------------------------------------------------------------

gzip_mode_t 
ok_gzip_get_mode (const compressible_t &b, int v, bool do_gzip)
{
  bool do_gz = b.inflated_len () > 0 && v > 0 && do_gzip;
  return do_gz ? ok_gzip_mode : GZIP_NONE;
}

//-----------------------------------------------------------------------
