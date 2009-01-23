
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
#include "okscratch.h"

//
// http_parser_base_t -- high level parsing object for HTTP requests;
// parses the headers, and then the bodies appropriately
//
class http_parser_base_t {
public:
  http_parser_base_t (ptr<ahttpcon> xx, u_int to, abuf_t *b) 
    : _parser_x (xx), 
      _abuf (b ? b : New abuf_t (New abuf_con_t (xx), true)),
      _del_abuf (b ? false : true),
      timeout (to ? to : ok_clnt_timeout),
      tocb (NULL),
      destroyed (New refcounted<bool> (false)),
      _parsing_header (false),
      _scratch (ok::alloc_scratch (ok_http_inhdr_buflen_big)) {}
  
  virtual ~http_parser_base_t ();

  str operator[] (const str &k) const { return hdr_cr ().lookup (k); }
  virtual http_inhdr_t *hdr_p () = 0;
  virtual const http_inhdr_t &hdr_cr () const = 0;
  ptr<ahttpcon> get_x () const { return _parser_x; }
  abuf_t *get_abuf_p () { return _abuf; }
  virtual int v_timeout_status () const { return HTTP_TIMEOUT; }

  void short_circuit_output ();

  void parse (cbi cb);
protected:
  virtual void v_parse_cb1 (int status) { finish (status); }
  virtual void v_cancel () {}

  void parse_cb1 (int status);
  void finish (int status);
  void clnt_timeout ();
  void stop_abuf ();

private:
  ptr<ahttpcon> _parser_x;
protected:
  abuf_t *_abuf;
  bool _del_abuf;
  u_int timeout;
  size_t buflen;
  timecb_t *tocb;
  cbi::ptr cb;
  ptr<bool> destroyed;
  bool _parsing_header;
  ptr<ok::scratch_handle_t> _scratch;
};

class http_parser_raw_t : public http_parser_base_t {
public:

  http_parser_raw_t (ptr<ahttpcon> xx, u_int to = 0, abuf_t *b = NULL)
    : http_parser_base_t (xx, to, b), 
      hdr (_abuf, _scratch) {}

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
  http_parser_full_t (ptr<ahttpcon> xx, u_int to = 0, abuf_t *b = NULL)
    : http_parser_base_t (xx, to, b), 
      hdr (_abuf, _scratch) 
  {}

  virtual ~http_parser_full_t () {}

  http_inhdr_t * hdr_p () { return &hdr; }
  const http_inhdr_t &hdr_cr () const { return hdr; }
  void finish2 (int s1, int s2);

  cgi_t & get_cookie () { return *hdr.get_cookie (); }
  cgi_t & get_url () { return *hdr.get_url (); }
  ptr<cgi_t> get_url_p () { return hdr.get_url (); }
  http_inhdr_t & get_hdr () { return hdr; }

protected:

  // called to prepare a parsing of a post body.
  cbi::ptr prepare_post_parse (int status);

public:
  http_inhdr_t hdr;
};

class http_parser_cgi_t : public http_parser_full_t {
public:
  http_parser_cgi_t (ptr<ahttpcon> xx, int to = 0, abuf_t *b = NULL) :
    http_parser_full_t (xx, to, b),
    post (_abuf, false, _scratch),
    mpfd (NULL),
    mpfd_flag (false),
    _union_mode (false) {}
  ~http_parser_cgi_t () { if (mpfd) delete mpfd; }

  void v_cancel () { hdr.cancel (); post.cancel (); }
  void v_parse_cb1 (int status);
  void enable_file_upload () { mpfd_flag = true; }

  static ptr<http_parser_cgi_t> alloc (ptr<ahttpcon> xx, u_int t = 0)
  { return New refcounted<http_parser_cgi_t> (xx, t); }

  cgi_t & get_post () { return post; }
  cgi_mpfd_t *get_mpfd () { return mpfd; }
  cgiw_t & get_cgi () { return cgi; }

  void set_union_mode (bool b);
  int v_timeout_status () const;

  cgi_t &cookie () { return get_cookie (); }

protected:
  ptr<cgi_t> get_union_cgi ();
  cgi_t post;
  // In union CGI mode, both POST and GET variables are shoved into one place.
  ptr<cgi_t> _union_cgi;

  cgi_mpfd_t *mpfd;
  cgiw_t cgi;  // wrapper set to either url or post, depending on the method


private:
  bool mpfd_flag;
  bool _union_mode;
};

#endif
