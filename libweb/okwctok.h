
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

#ifndef _LIBWEB_OKWCTOK_H
#define _LIBWEB_OKWCTOK_H

#include "okwc.h"

enum okwc_token_status_t { OKWC_TOKEN_OK = 0,
			   OKWC_TOKEN_EOF = 1,
			   OKWC_TOKEN_OVERFLOW = 2 };
class okwc_token_t {
public:
  okwc_token_t (okwc_token_status_t st, str s) : status (st), token (s) {}
  okwc_token_status_t status;
  str token;
};

class okwc_token_accepter_t {
public:
  okwc_token_accepter_t (const str &s, size_t tl) 
    : seperator (s), toklim (tl), status (HTTP_CLIENT_EOF) {}
  virtual void eat_tokens () = 0;
  void reset ();

  inline void add_tok (okwc_token_status_t st, str s = NULL) 
  { toks.push_back (okwc_token_t (st, s)); }
  inline void set_status (int s) { status = s; }
  inline okwc_cookie_set_t *get_incookies () { return &cookies; }
  inline size_t get_toklim () const { return toklim; }

protected:
  vec<okwc_token_t> toks;
  const str seperator;
  const size_t toklim;
  int status;
  okwc_cookie_set_t cookies;
};

//
// returns the body of the HTTP response tokenized
//
class okwc_http_tokenizer_t : public okwc_http_t {
public:
  okwc_http_tokenizer_t (okwc_req_t *r, ptr<ahttpcon> xx, const str &f, 
			 const str &h, okwc_token_accepter_t *t, 
			 int v, cgi_t *ock);
protected:
  void body_parsed (str bod);
  void finish2 (int status);
  void cancel2 ();
  virtual void finish3 (int status) = 0;
  okwc_token_accepter_t *accepter;
  okwc_req_t *parent;
};

class okwc_http_tokenizer_empty_t : public okwc_http_tokenizer_t {
public:
  okwc_http_tokenizer_empty_t (ptr<ahttpcon> xx, const str &f, const str &h,
			       okwc_token_accepter_t *t, int v, cgi_t *okc)
    : okwc_http_tokenizer_t (xx, f, h, t, v, okc), bufsz (t->get_toklim ()),
      curr (NULL)
  {}
  void body_parse_guts ();
private:
  mstr *curr;
  size_t bufsz;
};

class okwc_req_tokenizer_t : public okwc_req_t {
public:
  okwc_req_tokenizer_t (const str &ht, u_int16_t p, const str &fn,
			okwc_token_accepter_t *t, int vers, int timeout, 
			cgi_t *ock)
    : okwc_req_t (ht, p, fn, vers, timeout, ock), accepter (t) {}
protected:
  virtual void req_fail (int status);
  okwc_http_t *okwc_http_alloc () ;
  okwc_token_accepter_t *accepter;
};

okwc_req_t *
owkc_request (const str &h, u_int16_t port, const str &uri, 
	      okwc_token_accepter_t *tok, int vers = 0, int timeout = -1,
	      cgi_t *outcook = NULL);
		

#endif
