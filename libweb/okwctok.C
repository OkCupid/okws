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

#include "owkctok.h"


//-----------------------------------------------------------------------
//
// Tokenizing Web Client Implementation
//

okwc_req_t *
okwc_request (const str &host, u_int16_t port, const str &fn,
	      okwc_token_accepter_t *t, int vrs, int timeout, cgi_t *outcook)
{
  okwc_req_t *req = 
    New okwc_req_tokenizer_t (host, port, fn, t, vrs, timeout, outcook);
  req->launch ();
  return req;
}


okwc_http_t *
okwc_req_tokenizer_t::okwc_http_alloc ()
{
  return New okwc_http_bigstr_t (x, filename,  hostname,
				 accepter, vers, outcookie);
}

void
okwc_req_tokenizer_t::req_fail (int status)
{
  accepter->add_tok (OKWC_TOKEN_EOF);
  accepter->set_status (status);
}

void
okwc_http_tokenizer_t::finish2 (int status)
{
  finish3 (status);
  delete parent; // which will in turn delete us
}

void
okwc_token_acceptor_t::reset ()
{
  toks.clear ();
  cookies.reset ();
  status = HTTP_CLIENT_EOF;
}

okwc_http_tokenizer_t:: okwc_http_tokenizer_t (okwc_req_t *r,
					       ptr<ahttpcon> xx, 
					       const str &f, const str &h, 
					       okwc_token_accepter_t *a,
					       int v, cgi_t *okc)
  : okwc_http_t (xx,f,h,v,okc,a->get_incookies ()), accepter (a), parent (r)
{}
