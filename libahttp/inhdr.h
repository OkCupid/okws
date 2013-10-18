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

#ifndef _LIBAHTTP_INHDR_H
#define _LIBAHTTP_INHDR_H

#include "hdr.h"
#include "okcgi.h"
#include "qhash.h"

typedef enum { INHDRST_START = 0,
	       INHDRST_SPC1 = 1,
	       INHDRST_TARGET = 2,
	       INHDRST_URIDAT = 3,
	       INHDRST_SPC2 = 4,
	       INHDRST_OPTPARAM = 5,
	       INHDRST_EOL1 = 6,
	       INHDRST_KEY = 7,
	       INHDRST_SPC3 = 8,
	       INHDRST_VALUE = 9,
	       INHDRST_EOL2A = 10,
	       INHDRST_EOL2B = 11 } inhdrst_t;

typedef enum { HTTP_MTHD_NONE = 0,
	       HTTP_MTHD_POST = 1,
	       HTTP_MTHD_PUT = 2,
	       HTTP_MTHD_DELETE = 3,
           HTTP_MTHD_GET = 4,
	       HTTP_MTHD_HEAD = 5,
           HTTP_MTHD_OPTIONS = 6 } http_method_t;

typedef enum { POST_MTHD_NONE = 0,
	       POST_MTHD_URLENCODED = 1,
	       POST_MTHD_MULTIPART_FORM = 2 } post_method_t;

class methodmap_t {
public:
  methodmap_t ();
  http_method_t lookup (const str &s) const;
private:
  qhash<str,http_method_t> map;
};

class post_methodmap_t {
public:
  post_methodmap_t ();
  post_method_t lookup (const str &s) const;
private:
  qhash<str,post_method_t> map;
};

extern methodmap_t methodmap;

//-----------------------------------------------------------------------

typedef enum { HTTP_CONN_NONE = 0,
	       HTTP_CONN_CLOSED = 1,
	       HTTP_CONN_KEEPALIVE = 2 } http_conn_mode_t;

//-----------------------------------------------------------------------

class http_inhdr_t : public http_hdr_t, public pairtab_t<> {
public:
  http_inhdr_t (abuf_t *a, ptr<ok::scratch_handle_t> s = NULL)
    : async_parser_t (a), 
      http_hdr_t (a, s),
      contlen (-1), 
      state (INHDRST_START),
      _conn_mode (HTTP_CONN_NONE),
      _reqno (0),
      _pipeline_eof_ok (false),
      _parse_query_string (ok_http_parse_query_string) {}

  inline str get_line1 () const { return line1; }
  inline str get_target () const { return target; }
  bool takes_gzip () const;
  bool has_broken_chunking () const;
  inline str get_mthd_str () const { return tmthd; }
  inline str get_vers_str () const { return vers; }
  http_conn_mode_t get_conn_mode () const;
  inline u_int get_reqno () const { return _reqno; }
  str get_connection () const;
  void set_parse_query_string (bool b) { _parse_query_string = b; }

  str get_user_agent (bool null_ok = true) const;
  str get_referrer (bool null_ok = false) const;

  http_method_t mthd;  // method code
  int contlen;     // content-length size

  void set_url (ptr<cgi_t> u) { _url = u; }
  void set_reqno (u_int i, bool pipelining, htpv_t prev_vers);
  bool clean_pipeline_eof_state () const;
  void v_debug ();
  int timeout_status () const;

  ptr<cgi_t> get_cookie ();
  ptr<const cgi_t> get_cookie () const;
  ptr<cgi_t> get_url ();

protected:
  void parse_guts ();
  virtual void ext_parse_cb (int dummy);
  virtual void fixup ();
  ptr<ok::scratch_handle_t> alloc_scratch2 ();

  ptr<cgi_t> _cookie;
  ptr<cgi_t> _url;
  ptr<ok::scratch_handle_t> _scratch2;

  inhdrst_t state;    // parse state

  str tmthd;        // POST, GET, etc...
  str target;       // URI given as target
  str vers;         // HTTP version
  str line1;        // first line of the HTTP req

  http_conn_mode_t _conn_mode;
  u_int _reqno;     // serial # of this request within an HTTP/1.1 pipeline
  bool _pipeline_eof_ok;
  bool _parse_query_string;
};


#endif /* _LIBAHTTP_INHDR_H */
