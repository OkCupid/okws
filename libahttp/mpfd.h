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

//
// mpfd = Multi Part Form Data
//

#include "okcgi.h"
#include "inhdr.h"
#include "kmp.h"

// mpfd parsing states
typedef enum { MPFD_START = 0,
	       MPFD_EOL0 = 1,
	       MPFD_KEY = 2,
	       MPFD_SPC = 3,
	       MPFD_VALUE = 4,
	       MPFD_EOL1A = 5,
	       MPFD_EOL1B = 6,
	       MPFD_SEARCH = 7,
	       MPFD_SEARCH2 = 8,
	       MPFD_SEARCH3 = 9,
	       MPFD_EOF = 10 } mpfdst_t;

typedef enum { MPFD_OTHER = 0,
	       MPFD_DISPOSITION = 1,
	       MPFD_TYPE = 2 } mpfdkt_t; // key types

class cgi_mpfd_pair_t : public cgi_pair_t {
public:
  cgi_mpfd_pair_t (const str &k, const str &v, bool e = true)
    : cgi_pair_t (k,v,e), files (NULL) {}
  cgi_mpfd_pair_t (const str &k) : cgi_pair_t (k), files (NULL) {}
  ~cgi_mpfd_pair_t () { if (files) delete (files); }
  void add (const cgi_file_t &f);
  cgi_mpfd_pair_t *to_cgi_mpfd_pair () { return this; }
  cgi_files_t *files;
};

class mpfd_ktmap_t {
public:
  mpfd_ktmap_t ();
  mpfdkt_t lookup (const str &s) const;
private:
  qhash<str,mpfdkt_t> map;
};

typedef enum { CONTDISP_NONE = 0,
	       CONTDISP_FORMDAT = 1,
	       CONTDISP_ATTACH = 2 } contdisp_type_t;

typedef enum { CONTDISP_START = 0,
	       CONTDISP_SEP1 = 1,
	       CONTDISP_NAME_KEY = 2,
	       CONTDISP_NAME_VAL = 3,
	       CONTDISP_SEP2A = 4,
	       CONTDISP_SEP2B = 5,
	       CONTDISP_SEP2C = 6,
	       CONTDISP_FILENAME_KEY = 7,
	       CONTDISP_FILENAME_VAL = 8 } contdisp_state_t;

// content-disposition: parser
class contdisp_parser_t : public http_hdr_t {
public:
  contdisp_parser_t (abuf_t *a, ptr<ok::scratch_handle_t> s)
    : async_parser_t (a), 
      http_hdr_t (a, s),
      typ (CONTDISP_NONE), 
      state (CONTDISP_START) {}

  void parse_guts ();
  void reset ();

  contdisp_type_t typ;
  str name;
  str filename;
  contdisp_state_t state;

  str typ_scr, filename_scr;
};


class cgi_mpfd_t : public cgi_t, public http_hdr_t {
public:
  cgi_mpfd_t (abuf_t *a, ptr<ok::scratch_handle_t> s)
    : async_parser_t (a),
      cgi_t (a, false, s),
      http_hdr_t (a, s),
      cdp (a, s),
      cdm ("content-disposition", true),
      cbm (NULL), 
      _scratch (s),
      state (MPFD_START),
      to_start (false), 
      attach (false), 
      match_ended (false) {}

  ~cgi_mpfd_t (); 
	      
  bool flookup (const str &k, cgi_files_t **v) const;
  void finsert (const str &k, const cgi_file_t &f);
  pair_t *alloc_pair (const str &k, const str &v, bool e = true) const
  { return New cgi_mpfd_pair_t (k,v,e); }
  static bool match (const http_inhdr_t &hdr, str *b);
  static cgi_mpfd_t *alloc (abuf_t *a, size_t sz, const str &b);
  void add_boundary (const str &b);
  abuf_stat_t parse_2dash ();

 protected:
  void parse_guts ();
  void remove_boundary ();
  abuf_stat_t match_boundary (str *dat = NULL);
private:
  void ext_parse_cb (int status);

  contdisp_parser_t cdp; // content-disposition parser
  kmp_matcher_t cdm; // "Content-disposition" matcher
  vec<kmp_matcher_t *> boundaries;
  kmp_matcher_t *cbm; // current boundary matcher
  ptr<ok::scratch_handle_t> _scratch;
  mpfdst_t state;
  mpfdkt_t kt;
  bool to_start;

  str mpfd_key; // i.e. Content-type: or Content-Disposition:
  str content_typ;
  str cgi_key; // key supplied by web page in form
  str dat;
  str filename;

  mpfdst_t nxt_state;
  bool attach;
  bool match_ended;
};


