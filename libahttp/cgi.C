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

#include "cgi.h"
#include "parseopt.h"
#include "httpconst.h"

static const int HEXVALTAB[] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 
   -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 
   13, 14, 15, -1, -1, -1, -1, -1, -1, -1, 
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
   -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 
   11, 12, 13, 14, 15 };

static const bool REGCHARTAB[] = {
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
   0, 0, 0, 0, 0, 0, 0, 
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1,
   0, 0, 0, 0, 0, 0,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 1, 1, 1, 1, 1 };

#define CGISS_SIZE 0x10000
static char cgi_static_scratch[CGISS_SIZE];

void
cgi_t::encode (strbuf *b) const
{
  encode_t *e = New encode_t (b, scratch, buflen);
  pairtab_t<cgi_pair_t>::encode (e);
  delete e;
}

void
cookie_t::encode (strbuf *b) const
{
  encode_t *e = New encode_t (b, scratch, buflen);
  pairtab_t<cgi_pair_t>::encode (e);
  str sep = get_sep ();
  expires.encode (e, sep);
  path.encode (e, sep);
  domain.encode (e, sep);
  delete e;
}

str
cgi_t::encode () const 
{
  strbuf b;
  encode (&b);
  return b;
}

str
cgi_encode (const str &in)
{
  strbuf b;
  cgi_encode (in, &b, cgi_static_scratch, CGISS_SIZE, true);
  return b;
}

str
cgi_decode (const str &in)
{
  abuf_str_t a (in);
  cgi_t c (&a, false, CGISS_SIZE, cgi_static_scratch);
  str s;
  return (c.parse_key_or_val (&s, CGI_NONE) == ABUF_EOF ? s : (str )NULL);
}

size_t
cgi_encode (const str &in, strbuf *out, char *scratch, size_t len, bool e)
{
  bool freeit = false;
  size_t inlen = in.len ();
  size_t needlen = inlen * 3;

  // XXX: some check here?
  if (inlen > needlen) {
    if (needlen > CGI_MAXLEN)
      return 0;
    freeit = true;
    scratch = (char *)xmalloc (needlen + 1);
  }
  char *op = scratch;
  const char *inp = in.cstr ();
  const char *ep = inp + in.len ();
  int inc;
  for ( ; inp < ep; inp++) {
    inc = *inp;
    if (e) {
      if (inc >= '0' && inc <= 'z' && REGCHARTAB[inc - '0']) 
	*op++ = inc;
      else if (inc == ' ')
	*op++ = '+';
      else {
	op += sprintf (op, "%%%02x", inc);
      }
    } else {
      if (inc == ';') 
	op += sprintf (op, "%%%02x", inc);
      else
	*op++ = inc;
    }
  }
  size_t olen = op - scratch;
  out->tosuio ()->copy (scratch, olen);
  if (freeit)
    xfree (scratch);
  return olen;
}

static inline
int char_to_hex (int c)
{
  if (c < '0' || c > 'f')
    return -1;
  return HEXVALTAB[c - '0'];
}

void
cgi_pair_t::encode (encode_t *e, const str &sep) const
{
  if (!hasdata ())
    return;

  if (e->first)
    e->first = false;
  else
    *e->out << sep;
  strbuf c;
  cgi_encode (key, &c, e->scratch, e->len, encflag);
  size_t s = vals.size ();
  for (u_int i = 0; i < s; i++) {
    if (i > 0)
      *e->out << sep;
    *e->out << c << "=";
    cgi_encode (vals[i], e->out, e->scratch, e->len, encflag);
  }
}

void
cgi_t::init (char *buf)
{
  if (buf) {
    scratch = buf;
    bufalloc = false;
  } else {
    scratch = (char *) xmalloc (buflen);
    bufalloc = true;
  }
  assert (scratch);
  pcp = scratch;
  endp = scratch + buflen;
}

/*
 * XXX - should fix this 
 *
void
cgi_t::reset ()
{
  pairtab_t<cgi_pair_t>::reset ();
  async_parser_t::reset ();
}
*/
    
cgi_t::cgi_t (abuf_t *a, bool ck, u_int bfln, char *buf)
  : async_parser_t (a), pairtab_t<cgi_pair_t> (true),
    cookie (ck), bufalloc (false),
    inhex (false), pstate (cookie ? CGI_CKEY : CGI_KEY), 
    hex_i (0), hex_h (0), hex_lch (0), uri_mode (false),
    buflen (min<u_int> (bfln, CGI_MAX_SCRATCH))
{
  init (buf);
}

cgi_t::cgi_t (abuf_src_t *s, bool ck, u_int bfln, char *buf)
  : async_parser_t (s), pairtab_t<cgi_pair_t> (true),
  cookie (ck), bufalloc (false),
  inhex (false), pstate (cookie ? CGI_CKEY : CGI_KEY), 
  hex_i (0), hex_h (0), hex_lch (0), uri_mode (false),
  buflen (min<u_int> (bfln, CGI_MAX_SCRATCH))
{
  init (buf);
}

cgi_t::~cgi_t ()
{
  tab.deleteall ();
  if (bufalloc && scratch)
    xfree (scratch);
}

void
cgi_t::parse_guts ()
{
  abuf_stat_t rc;
  do { rc = parse_key_or_val (); } while (rc != ABUF_EOF && rc != ABUF_WAIT);
  if (rc == ABUF_EOF) {
    if (bufalloc) {
      xfree (scratch);
      scratch = NULL;
    }
    finish_parse (HTTP_OK);
  }
}

#define KEY_STATE(s)                      \
   ((s) == CGI_KEY || (s) == CGI_CKEY)

#define COOKIE_STATE(s)                   \
   ((s) == CGI_CKEY || (s) == CGI_CVAL)

#define STATE_TRANS(s)                    \
   if (s == CGI_KEY)                      \
     s = CGI_VAL;                         \
   else if (s == CGI_VAL)                 \
     s = CGI_KEY;                         \
   else if (s == CGI_CKEY)                \
     s = CGI_CVAL;                        \
   else                                   \
     s = CGI_CKEY;
    

abuf_stat_t
cgi_t::parse_key_or_val ()
{
  if (pstate == CGI_CKEY && pcp == scratch && abuf->skip_ws () == ABUF_WAIT)
    return ABUF_WAIT;

  str s;
  abuf_stat_t rc = parse_key_or_val (&s, pstate);
  switch (rc) {
  case ABUF_PARSE_ERR:
    assert (KEY_STATE (pstate));
    insert (s);
    rc = ABUF_OK;
    break;
  case ABUF_OK:
    if (KEY_STATE (pstate)) {
      key = s;
    } else {
      insert (key, s);
      key = NULL;
    }
    STATE_TRANS (pstate);
    break;
  case ABUF_EOF:
    if (KEY_STATE (pstate))
      insert (s);
    else 
      insert (key, s);
    break;
  default:
    break;
  }
  return rc;
}

abuf_stat_t
cgi_t::parse_key_or_val (str *r, cgi_var_t vt)
{
  int ch;
  bool flag = true;
  abuf_stat_t ret = ABUF_OK;

  if (inhex) {
    if (parse_hexchar (&pcp, endp) == ABUF_WAIT)
      return ABUF_WAIT;
    else
      inhex = false;
  }
    
  while ( pcp < endp && flag ) {
    ch = abuf->get ();
    switch (ch) {
    case CGI_SEPCHAR:
      if (vt == CGI_KEY)
	ret = ABUF_PARSE_ERR;
      else if (vt == CGI_VAL)
	flag = false;
      else
	*pcp++ = ch;
      break;
    case CGI_BINDCHAR:
      if (KEY_STATE (vt))
	flag = false;
      else 
	*pcp++ = ch;
      break;
    case CGI_CVALEND:
      if (vt == CGI_CKEY)
	ret = ABUF_PARSE_ERR;
      else if (vt == CGI_CVAL)
	flag = false;
      else
	*pcp++ = ch;
      break;
    case CGI_CRCHAR:
      if (uri_mode) {
	ret = ABUF_EOF;
	flag = false;
      } else if (!COOKIE_STATE (vt)) {
	*pcp++ = ch;
      }
      break;
    case CGI_EOLCHAR:
      if (COOKIE_STATE (vt) || uri_mode) {
	abuf->unget ();
	ret = ABUF_EOF;
	flag = false;
      } else {
	*pcp++ = ch;
      }
      break;
    case ABUF_EOFCHAR:
      ret = ABUF_EOF;
      flag = false;
      break;
    case ABUF_WAITCHAR:
      ret = ABUF_WAIT;
      flag = false;
      break;
    case CGI_HEXCHAR:
      inhex = true;
      if (parse_hexchar (&pcp, endp) == ABUF_WAIT) {
	ret = ABUF_WAIT;
	flag = false;
      } else
	inhex = false;
      break;
    case CGI_SPACECHAR:
      *pcp++ = ' ';
      break;
    case ' ':
    case '\t':
      if (uri_mode) {
	abuf->unget ();
	ret = ABUF_EOF;
	flag = false;
	break;
      } // otherwise fall through to default
    default:
      *pcp++ = ch;
    }
  }

  if (ret != ABUF_WAIT) {
    if (pcp >= endp) {
      *r = NULL;
      ret =  ABUF_OVERFLOW;
    } else {
      if (ok_filter_cgi == XSSFILT_ALL)
	*r = xss_filter (scratch, pcp - scratch);
      else
	*r = str (scratch, pcp - scratch);
    }
    pcp = scratch; // reset for next time
  }
  return ret;
}

abuf_stat_t
cgi_t::parse_hexchar (char **pp, char *end)
{
  int len = 2;
  int t, ch;
  char *p = *pp;
  for (; hex_i < len; hex_i++) {
    ch = abuf->get ();
    if (ch == ABUF_WAITCHAR)
      return ABUF_WAIT;
    if ((t = char_to_hex (ch)) < 0) {
      abuf->unget ();
      break;
    }
    hex_lch = ch;
    if (hex_i == 0) hex_h = t;
    else hex_h = (hex_h << 4) | t;
  }
  if (hex_i == len)
    *p++ = hex_h;
  else {
    *p++ = CGI_HEXCHAR;
    if ((p + 1) < end && hex_lch) *p++ = hex_lch;
  }

  hex_i = 0; // crucial to reset state for next time
  hex_h = 0;
  hex_lch = 0;

  *pp = p;
  return ABUF_OK;
}

ptr<cgi_t>
cgi_t::str_parse (const str &s)
{
  abuf_str_t cgis (s);
  ptr<cgi_t> r = New refcounted<cgi_t, vbase> (&cgis);
  r->parse (NULL);
  return r;
}

str
expire_in (int d, int h, int m, int s)
{
  h += d*24;
  m += h*60;
  s += m*60;
  time_t t = timenow + s;
  mstr out (40);
  struct tm *stm = gmtime (&t);
  size_t l = strftime (out.cstr (), 40, "%A, %d-%b-%Y %T GMT", stm);
  out.setlen (l);
  return out;
}

