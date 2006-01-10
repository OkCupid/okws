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
	       HTTP_MTHD_HEAD = 5 } http_method_t;

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

class http_inhdr_t : public http_hdr_t, public pairtab_t<> {
public:
  http_inhdr_t (abuf_t *a, cgi_t *u = NULL, cgi_t *c = NULL, 
		size_t bfln = HTTPHDR_DEF_SCRATCH, char *b = NULL)
    : async_parser_t (a), http_hdr_t (a, bfln, b),
      contlen (-1), uri (u), cookie (c), state (INHDRST_START) {}

  inline str get_line1 () const { return line1; }
  inline str get_target () const { return target; }
  bool takes_gzip () const;
  inline str get_mthd_str () const { return tmthd; }
  inline str get_vers_str () const { return vers; }

  http_method_t mthd;  // method code
  int contlen;     // content-length size

protected:
  void parse_guts ();
  virtual void ext_parse_cb (int dummy);
  virtual void fixup ();

  cgi_t *uri;
  cgi_t *cookie;
  inhdrst_t state;    // parse state

  str tmthd;        // POST, GET, etc...
  str target;       // URI given as target
  str vers;         // HTTP version
  str line1;        // first line of the HTTP req
};


#endif /* _LIBAHTTP_INHDR_H */
