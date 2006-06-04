
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

#ifndef _LIBAHTTP_AHPARSE_H
#define _LIBAHTTP_AHPARSE_H

#include "httpconst.h"
#include "arpc.h"
#include "ahttp.h"
#include "okcgi.h"
#include "resp.h"
#include "pub.h"
#include "inhdr.h"
#include "pslave.h"
#include "pubutil.h"
#include "mpfd.h"

#define HTTP_PARSE_BUFLEN 0x4000
#define HTTP_PARSE_BUFLEN2 0x1000

//
// http_parser_base_t -- high level parsing object for HTTP requests;
// parses the headers, and then the bodies appropriately
//
class http_parser_base_t {
public:
  http_parser_base_t (ptr<ahttpcon> xx, u_int to) 
    : x (xx), abuf (New abuf_con_t (xx), true),
      timeout (to ? to : ok_clnt_timeout),
      buflen (HTTP_PARSE_BUFLEN), tocb (NULL),
      destroyed (New refcounted<bool> (false)) {}
  virtual ~http_parser_base_t ();

  str operator[] (const str &k) const { return hdr_cr ().lookup (k); }
  virtual http_inhdr_t *hdr_p () = 0;
  virtual const http_inhdr_t &hdr_cr () const = 0;
  ptr<ahttpcon> get_x () const { return x; }
  abuf_t *get_abuf_p () { return &abuf; }

  void parse (cbi cb);
protected:
  virtual void v_parse_cb1 (int status) { finish (status); }
  virtual void v_cancel () {}

  void parse_cb1 (int status);
  void finish (int status);
  void clnt_timeout ();
  void stop_abuf ();

  ptr<ahttpcon> x;
  abuf_t abuf;
  u_int timeout;
  size_t buflen;
  char scratch[HTTP_PARSE_BUFLEN];
  timecb_t *tocb;
  cbi::ptr cb;
  ptr<bool> destroyed;
};

class http_parser_raw_t : public http_parser_base_t {
public:
  http_parser_raw_t (ptr<ahttpcon> xx, u_int to = 0)
    : http_parser_base_t (xx, to), 
      hdr (&abuf, NULL, NULL, buflen, scratch) {}

  http_inhdr_t *hdr_p () { return &hdr; }
  const http_inhdr_t &hdr_cr () const { return hdr; }
  void v_cancel () { hdr.cancel (); }

  static ptr<http_parser_raw_t> alloc (ptr<ahttpcon> xx, u_int t = 0)
  { return New refcounted<http_parser_raw_t> (xx, t); }

  http_inhdr_t hdr;
};

/*
 * Parses the HTTP header, the potentially CGI-encoded URI,
 * the cookie in the header, and has a hook for parsing
 * a POST request body.
 */
class http_parser_full_t : public http_parser_base_t {
public:
  http_parser_full_t (ptr<ahttpcon> xx, u_int to = 0)
    : http_parser_base_t (xx, to), buflen2 (HTTP_PARSE_BUFLEN2),
      cookie (&abuf, true, buflen2, scratch2),
      url (&abuf, false, buflen2, scratch2),
      hdr (&abuf, &url, &cookie, buflen, scratch) 
  {}
  virtual ~http_parser_full_t () {}

  http_inhdr_t * hdr_p () { return &hdr; }
  const http_inhdr_t &hdr_cr () const { return hdr; }
  void finish2 (int s1, int s2);


  cgi_t & get_cookie () { return cookie; }
  cgi_t & get_url () { return url; }
  http_inhdr_t & get_hdr () { return hdr; }

protected:

  // called to prepare a parsing of a post body.
  cbi::ptr prepare_post_parse (int status);

  size_t buflen2;
  cgi_t cookie;
  cgi_t url;
  char scratch2[HTTP_PARSE_BUFLEN2];

public:
  http_inhdr_t hdr;
};

class http_parser_cgi_t : public http_parser_full_t {
public:
  http_parser_cgi_t (ptr<ahttpcon> xx, int to = 0) :
    http_parser_full_t (xx, to),
    post (&abuf, false, buflen, scratch),
    mpfd (NULL),
    mpfd_flag (false) {}
  ~http_parser_cgi_t () { if (mpfd) delete mpfd; }

  void v_cancel () { hdr.cancel (); post.cancel (); }
  void v_parse_cb1 (int status);
  void enable_file_upload () { mpfd_flag = true; }

  static ptr<http_parser_cgi_t> alloc (ptr<ahttpcon> xx, u_int t = 0)
  { return New refcounted<http_parser_cgi_t> (xx, t); }

  cgi_t & get_post () { return post; }
  cgi_mpfd_t *get_mpfd () { return mpfd; }
  cgiw_t & get_cgi () { return cgi; }

protected:
  cgi_t post;
  cgi_mpfd_t *mpfd;
  cgiw_t cgi;  // wrapper set to either url or post, depending on the method
private:
  bool mpfd_flag;
};

#ifdef HAVE_EXPAT
#include "okxmlparse.h"
class http_parser_xml_t : public http_parser_full_t {
public:
  http_parser_xml_t (ptr<ahttpcon> xx, int to = 0)
    : http_parser_full_t (xx, to), _xml (&abuf) {}
  ~http_parser_xml_t () {}

  void v_cancel () ;
  void v_parse_cb1 (int status) ;

  ptr<xml_top_level_t> top_level () { return _xml.top_level (); }

  ptr<const xml_top_level_t> top_level_const () const 
  { return _xml.top_level_const (); }

  str errmsg () const { return _xml.errmsg (); }
  int errcode () const { return _xml.errcode (); }

private:
  xml_req_parser_t _xml;

};
#endif /* HAVE_EXPAT */

#endif
