// -*-c++-*-
/* $Id: resp.h 3923 2009-01-14 21:43:23Z max $ */

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

#ifndef __LIBAHTTP_RESP2__
#define __LIBAHTTP_RESP2__

#include "resp.h"

//------------------------------------------------------------------------

class http_response_ok2_t : public http_response_base_t {
public:
  http_response_ok2_t (const http_resp_attributes_t &a, 
		       ptr<compressible_t> x, bool compress = false)
    : _header (a),
      _body (x),
      _compressed (compress),
      _uid (0),
      _inflated_len (0),
      _n_bytes (0) {}

  const http_resp_header_t *get_header () const { return &_header; }
  http_resp_header_t *get_header () { return &_header; }

  void send2 (ptr<ahttpcon> x, ev_ssize_t ev) { send2_T (x, ev); }
  size_t get_nbytes () const { return _n_bytes; }

protected:
  void send2_T (ptr<ahttpcon> x, ev_ssize_t ev, CLOSURE);

  http_resp_header_t _header;
  ptr<compressible_t> _body;

  bool _compressed;
  u_int64_t _uid;
  size_t _inflated_len;
  str _custom_log2;
  size_t _n_bytes;

};

//------------------------------------------------------------------------

#endif /* __LIBAHTTP_RESP2__ */
