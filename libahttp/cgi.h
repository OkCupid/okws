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

#define CGI_DEF_SCRATCH  0x10000
#define CGI_MAX_SCRATCH  0x40000


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

size_t cgi_encode (const str &in, strbuf *out, char *scratch = NULL, 
		   size_t len = 0, bool e = true);
str cgi_encode (const str &in);
str cgi_decode (const str &in);

class cgi_t : public virtual async_parser_t,
	      public pairtab_t<cgi_pair_t> {
public:
  cgi_t (abuf_src_t *s = NULL, bool ck = false, 
	 u_int bfln = CGI_DEF_SCRATCH, char *buf = NULL);
  cgi_t (abuf_t *a, bool ck = false, u_int buflen = CGI_DEF_SCRATCH,
	 char *buf = NULL);
  ~cgi_t ();

  virtual void encode (strbuf *b) const;
  virtual str encode () const;
  
  static ptr<cgi_t> str_parse (const str &s);

  void set_uri_mode (bool b) { uri_mode = b; }
  abuf_stat_t parse_key_or_val ();
  abuf_stat_t parse_key_or_val (str *r, cgi_var_t vt);

  virtual bool flookup (const str &k, cgi_files_t **v) { return false; }
private:
  void init (char *buf);
  virtual void parse_guts ();
  abuf_stat_t parse_hexchar (char **pp, char *end);

  bool cookie;
  bool bufalloc;    // on if we alloced this buf; off if we're borrowing it

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
  char *scratch;    // buffer for parse / scratch
  u_int buflen;
};

class cgiw_t {
public:
  cgiw_t (cgi_t *cc = NULL) : c (cc) {}
  cgiw_t &operator= (cgi_t *cc) { c = cc; return *this; }
  operator cgi_t *() const { return c; }
  inline str operator[] (const str &k) const { return (*c)[k]; }
  inline bool exists (const str &k) const { return c->exists (k); }
  inline bool lookup (const str &k, str *r) const { return c->lookup (k,r); }
  inline bool blookup (const str &k) const { return c->blookup (k); }
  inline vec<int64_t> *ivlookup (const str &k) const { return c->ivlookup (k);}
  inline bool lookup (const str &k, vec<str> *v) const 
  { return c->lookup (k, v); }
  template<typename T> bool lookup (const str &k, T *v) const
  { return c->lookup (k, v); }
  template<typename T> cgiw_t & insert (const str &k, T v, bool ap = true) 
  { c->insert (k, v, ap); return (*this); }
  cgi_t *cgi () const { return c; }
  bool flookup (const str &k, cgi_files_t **v) { return c->flookup (k,v); }
  
private:
  cgi_t *c;
};

str expire_in (int d, int h, int m, int s);

class cookie_t : protected cgi_t
{
public:
  cookie_t (const str &d = NULL, const str &p = "/", const str &e = NULL) 
    : cgi_t (), domain ("domain", d, false), path ("path", p, false), 
      expires ("expires", e, false) {}
							       
  cookie_t &add (const str &k, const str &v) 
  { insert (k, v, false); return (*this); }
  cookie_t &add (const str &k, u_int i) { insert (k, i); return (*this); }
  cookie_t &add (const str &k, int i) { insert (k, i); return (*this); }

  cookie_t &set_expires (int d, int h, int m, int s)
  { return set_expires (expire_in (d, h, m, s)); }

  cookie_t &set_expires (const str &s) { expires.addval (s); return (*this); }
  
  str to_str () const { return cgi_t::encode (); }
  str get_sep () const { return "; "; }
  void encode (strbuf *b) const;

  cgi_pair_t domain;
  cgi_pair_t path;
  cgi_pair_t expires;
};


#endif /* _LIBAHTTP_CGI_H */
