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

#ifndef _LIBAHTTP_CGI_H
#define _LIBAHTTP_CGI_H

#include <async.h>
#include <ihash.h>
#include <ctype.h>
#include "ahttp.h"
#include "abuf.h"
#include "pair.h"
#include "aparse.h"
#include "pub3expr.h"

#define CGI_DEF_SCRATCH  0x10000

#define CGI_MAXLEN 131072
#define CGI_BINDCHAR '='
#define CGI_SEPCHAR '&'
#define CGI_HEXCHAR '%'
#define CGI_SPACECHAR '+'
#define CGI_CVALEND ';'
#define CGI_EOLCHAR '\n'
#define CGI_CRCHAR '\r'

typedef enum { CGI_KEY = 1, CGI_VAL = 2, 
	       CGI_CKEY = 3, CGI_CVAL = 4, CGI_NONE = 5 } cgi_var_t;

class cgi_file_t {
public:
  cgi_file_t (const str &n, const str &t, const str &d) 
    : filename (n), type (t), dat (d) {}
  str filename;
  str type;
  str dat;
};
typedef vec<cgi_file_t> cgi_files_t;

class cgi_pair_t : public pair_t {
public:
  cgi_pair_t (const str &k) : pair_t (k) {}
  cgi_pair_t (const str &k, const str &v, bool e = true) 
    : pair_t (k, v, e) {}
  cgi_pair_t (const str &k, int64_t i) 
    : pair_t (k, i) {}

  void encode (encode_t *e, const str &sep) const;
};

size_t cgi_encode (const str &in, strbuf *out, 
		   ptr<ok::scratch_handle_t> s = NULL, bool e = true);
str cgi_encode (const str &in);
str cgi_decode (const str &in);

class cgi_t : public virtual async_parser_t,
	      public pairtab_t<cgi_pair_t> {
public:
  cgi_t (abuf_src_t *s = NULL, bool ck = false, 
	 ptr<ok::scratch_handle_t> scr = NULL);
  cgi_t (abuf_t *a, bool ck = false, 
	 ptr<ok::scratch_handle_t> scr = NULL);
  ~cgi_t ();

  static ptr<cgi_t> alloc (abuf_t *a, bool ck, ptr<ok::scratch_handle_t> h);
  static ptr<cgi_t> alloc (abuf_src_t *a, bool ck, ptr<ok::scratch_handle_t> h);

  void set_scratch (ptr<ok::scratch_handle_t> s);

  virtual void encode (strbuf *b) const;
  virtual str encode () const;
  
  static ptr<cgi_t> str_parse (const str &s);

  void reset_state ();
  void set_uri_mode (bool b) { uri_mode = b; }
  // next function is virtual for MW, who wants to hook in at this point
  // in the parsing.
  virtual abuf_stat_t parse_guts_driver ();
  abuf_stat_t parse_key_or_val (str *r, bool use_internal_state = true);

  virtual bool flookup (const str &k, cgi_files_t **v) const { return false; }

  void set_max_scratchlen (ssize_t i) { _maxlen = i; }

  static ptr<const cgi_t> global_empty();
private:
  void init ();
  virtual void parse_guts ();
  abuf_stat_t parse_hexchar (char **pp, char *end);

  bool cookie;

  bool inhex;       // inhex when forced to wait
  cgi_var_t pstate; // parse state
  char *pcp;        // parse character pointer
  char *endp;       // end of the scratch buffer
  int hex_i;        // i in parse_hexchar
  int hex_h;        // h in parse_hexchar
  int hex_lch;      // last char in parse_hexchar
  str key;          // key used in parsing key/val pairs

  bool uri_mode;    // on if parsing within a URI
protected:
  ptr<ok::scratch_handle_t> _scratch;
  ssize_t _maxlen;      // maximum len it can ever grow to

  abuf_stat_t process_parsed_key_or_val(abuf_stat_t rc, str& s);
  bool        parse_still_waiting();
  bool        extend_scratch();
};

class cgiw_t {
public:
  cgiw_t (cgi_t *cc = NULL) : c (cc) {}
  cgiw_t &operator= (cgi_t *cc) { c = cc; return *this; }
  operator cgi_t *() const { return c; }
  inline str operator[] (const str &k) const { return (*c)[k]; }

  // It's there and it's not the empty string: p=0 is true, p= is not
  inline bool exists (const str &k) const { return c->exists (k); }
  
  // It's there, and it's possibly the empty string: p=0 is true, p= is true
  inline bool strict_exists(const str &k) const { return c->strict_exists(k); }

  inline bool lookup (const str &k, str *r) const { return c->lookup (k,r); }
  inline bool blookup (const str &k) const { return c->blookup (k); }
  inline vec<int64_t> *ivlookup (const str &k) const { return c->ivlookup (k);}
  inline vec<u_int64_t> *uivlookup (const str &k) const 
  { return c->uivlookup (k); }
  inline bool lookup (const str &k, vec<str> *v) const 
  { return c->lookup (k, v); }
  template<typename T> bool lookup (const str &k, T *v) const
  { return c->lookup (k, v); }
  bool lookup (const str &k, double *d) const { return c->lookup (k, d); }
  bool lookup (const str &k, float *f) const { return c->lookup (k, f); }
  template<typename T> cgiw_t & insert (const str &k, T v, bool ap = true) 
  { c->insert (k, v, ap); return (*this); }
  cgi_t *cgi () const { return c; }
  bool flookup (const str &k, cgi_files_t **v) const { return c->flookup (k,v); }
  void load_dict (pub3::dict_t *d) const { return c->load_dict (d); }
  
private:
  cgi_t *c;
};

str expire_in (int d, int h, int m, int s, rfc_number_t rfc = RFC_1123);

//
// cookie_t is for setting OUTGOING cookies only.
//
class cookie_t : protected cgi_t
{
public:
  cookie_t (const str &d = NULL, const str &p = "/", const str &e = NULL,
	    bool s = false, bool ho = false) 
    : cgi_t (), domain ("Domain", d, false), path ("Path", p, false), 
      expires ("Expires", e, false), secure (s), httponly(ho) {}
							       
  cookie_t &add (const str &k, const str &v) 
  { insert (k, v, false); return (*this); }

  // not sure why this isn't templated...
  cookie_t &add (const str &k, u_int i) { insert (k, i); return (*this); }
  cookie_t &add (const str &k, int i) { insert (k, i); return (*this); }
  cookie_t &add (const str &k, u_int64_t i) { insert (k, i); return (*this); }
  cookie_t &add (const str &k, int64_t i) { insert (k, i); return (*this); }

  cookie_t &set_expires (int d, int h, int m, int s, 
			 rfc_number_t rfc = RFC_1123)
  { return set_expires (expire_in (d, h, m, s, rfc)); }

  cookie_t &set_expires (const str &s) { expires.addval (s); return (*this); }
  
  cookie_t &set_secure (bool fl = true) { secure = fl; return (*this); }
  cookie_t &set_httponly (bool fl = true) { httponly = fl; return (*this); }
  
  str to_str () const { return cgi_t::encode (); }
  str get_sep () const { return "; "; }
  void encode (strbuf *b) const;

  cgi_pair_t domain;
  cgi_pair_t path;
  cgi_pair_t expires;
  bool secure;
  bool httponly;
};


#endif /* _LIBAHTTP_CGI_H */
