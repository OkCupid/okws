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

http_status_set_t http_status;
//
// what this OKWS is called when sending responses back to clients;
// we can get sneaky here and call ourselves apache, for instance.
//
str global_okws_server_label = okws_server_label;

http_status_set_t::http_status_set_t ()
{
  add (100, "Continue");
  add (101, "Switching Protocols");
  add (HTTP_OK, "OK");
  add (201, "Created");
  add (202, "Accepted");
  add (203, "Non-Authoratative Information");
  add (204, "No Content");
  add (205, "Resent Content");
  add (HTTP_PARTIAL_CONTENT, "Partial Content");
  add (300, "Multiple  Choices");
  add (HTTP_REDIRECT, "Moved Permanently");
  add (HTTP_MOVEDTEMP, "Found"); 
  add (HTTP_SEEOTHER, "See Other");
  add (HTTP_NOT_MODIFIED, "Not Modified");
  add (HTTP_USEPROXY, "Use Proxy");
  add (307, "Temporary Redirect");
  add (HTTP_BAD_REQUEST, "Bad Request");
  add (401, "Unauthorized");
  add (402, "Payment Required");
  add (HTTP_FORBIDDEN, "Forbidden");
  add (HTTP_NOT_FOUND, "Not Found");
  add (HTTP_NOT_ALLOWED, "Method Not Allowed");
  add (406, "Not Acceptable");
  add (407, "Proxy Authentication Required");
  add (HTTP_TIMEOUT, "Request Time-out");
  add (409, "Conflict");
  add (410, "Gone");
  add (411, "Length Required");
  add (412, "Precondition Failed");
  add (HTTP_REQ_TOO_BIG, "Request Entity Too Large");
  add (HTTP_URI_TOO_BIG, "Request-URI Too Large");
  add (415, "Unsupported Media Type");
  add (416, "Requested range not satisfied");
  add (HTTP_UNEXPECTED_EOF, "Expectation Failed");
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
  add_closed ();
  add ("Cache-control", attributes.get_cache_control ());
  if (attributes.get_expires ()) {
    add ("Expires", attributes.get_expires ());
  }
  add_server ();
  if (gz) gzip ();
}

void
http_resp_header_redirect_t::fill (const str &s)
{
  add_date ();
  add_closed ();
  add_server ();
  add ("Location", s);
}

void
http_resp_header_t::fill (bool gz, int len)
{
  fill (gz);
  if (!gz) add (http_hdr_size_t (len));
}

void
http_resp_header_t::gzip ()
{
  add ("Content-Encoding", "gzip");
  add ("Transfer-Encoding", "chunked");
}

strbuf
http_resp_header_t::to_strbuf () const
{
  strbuf b;
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
  for (int i = 0; i < lim; i++) {
    b << fields[i].name << ": " << fields[i].val << HTTP_CRLF;
  }
  b << HTTP_CRLF;
  return b;
}

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

ptr<http_pub_t>
http_pub_t::alloc (int n, const pub_base_t &p, const str &fn, aarr_t *env, 
		   htpv_t v)
{
  ptr<http_pub_t> ret (New refcounted<http_pub_t> (n, p, fn, env, v));
  if (ret->err) return NULL;
  else return ret;
}

u_int
http_response_t::send (ptr<ahttpcon> x, cbv::ptr cb)
{ 
  const strbuf &b = to_strbuf ();
  u_int ret = b.tosuio ()->resid ();
  x->send (b, cb); 
  return ret;
}

