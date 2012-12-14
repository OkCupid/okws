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

#include "okcgi.h"
#include "parseopt.h"
#include "httpconst.h"

//-----------------------------------------------------------------------

static const int HEXVALTAB[] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 
   -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 
   13, 14, 15, -1, -1, -1, -1, -1, -1, -1, 
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
   -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 
   11, 12, 13, 14, 15 };

//-----------------------------------------------------------------------

static const bool REGCHARTAB[] = {
  1, 1, 0, // 2d 2e 2f AKA  - . / 
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
  0, 0, 0, 0, 0, 0, 0, 
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 1, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1,
  0, 0, 0, 0 
};

//-----------------------------------------------------------------------

void
cgi_t::encode (strbuf *b) const
{
  encode_t *e = New encode_t (b, ok::alloc_scratch (ok_dflt_cgibuf_sz));
  pairtab_t<cgi_pair_t>::encode (e);
  delete e;
}

//-----------------------------------------------------------------------

void
cookie_t::encode (strbuf *b) const
{
  encode_t *e = New encode_t (b, _scratch);
  pairtab_t<cgi_pair_t>::encode (e);
  str sep = get_sep ();
  expires.encode (e, sep);
  path.encode (e, sep);
  domain.encode (e, sep);
  if (secure)
    *e->out << sep << "Secure";
  if (httponly)
    *e->out << sep << "HttpOnly";
  delete e;
}

//-----------------------------------------------------------------------

str
cgi_t::encode () const 
{
  strbuf b;
  encode (&b);
  return b;
}

//-----------------------------------------------------------------------

str
cgi_encode (const str &in)
{
  strbuf b;
  cgi_encode (in, &b, ok::alloc_scratch (ok_dflt_cgibuf_sz), true);
  return b;
}

//-----------------------------------------------------------------------

str
cgi_decode (const str &in)
{
  abuf_str_t a (in);
  cgi_t c (&a, false, ok::alloc_scratch (ok_dflt_cgibuf_sz));
  str s;
  return (c.parse_key_or_val (&s, false) == ABUF_EOF ? s : (str )NULL);
}

#define MAX_EXPAND_FACTOR 3

//-----------------------------------------------------------------------

size_t
cgi_encode (const str &in, strbuf *out, ptr<ok::scratch_handle_t> scr, bool e)
{
  size_t inlen = in.len ();
  size_t needlen = inlen * MAX_EXPAND_FACTOR;
  
  if (!scr) {
    size_t sz = max<size_t> (needlen, ok_dflt_cgibuf_sz);
    scr = ok::alloc_scratch (sz);
  } else if (needlen > scr->len ()) {
    scr = ok::alloc_nonstd_scratch (needlen);
  }

  char *op = scr->buf ();
  const char *inp = in.cstr ();
  const char *ep = inp + in.len ();
  int inc;
  int chars_out;
  for ( ; inp < ep; inp++) {
    
    // use unsigned characters to not output crap on 
    // international characters
    inc = (unsigned char)*inp; 

    // should probably do something more severe.
    if (inc < 0) {
      warn << "Unexpected character < 0: " << inc << "\n";
      continue;
    }

    if (e) {
      if (inc >= '-' && inc <= '~' && REGCHARTAB[inc - '-']) 
	*op++ = inc;
      else if (inc == ' ')
	*op++ = '+';
      else {
	chars_out = sprintf (op, "%%%02x", inc);
	if (chars_out > MAX_EXPAND_FACTOR) {
	  warn << "Printing %-style CGI-escape sequence took "
	       << "too many characters! (" << chars_out << ")\n";
	} else
	  op += chars_out;
      }
    } else {
      if (inc == ';') 
	op += sprintf (op, "%%%02x", inc);
      else
	*op++ = inc;
    }
  }
  size_t olen = op - scr->buf ();
  out->tosuio ()->copy (scr->buf (), olen);
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
  cgi_encode (key, &c, e->scratch (), encflag);
  size_t s = vals.size ();
  for (u_int i = 0; i < s; i++) {
    if (i > 0)
      *e->out << sep;
    *e->out << c << "=";
    cgi_encode (vals[i], e->out, e->scratch (), encflag);
  }
}

//-----------------------------------------------------------------------

void
cgi_t::init ()
{
  if (_scratch) {
    pcp = _scratch->buf ();
    endp = _scratch->end ();
  }
}

//-----------------------------------------------------------------------

void
cgi_t::set_scratch (ptr<ok::scratch_handle_t> s)
{
  _scratch = s;
  init ();
}

//-----------------------------------------------------------------------

cgi_t::cgi_t (abuf_t *a, bool ck, ptr<ok::scratch_handle_t> s)
  : async_parser_t (a), 
    pairtab_t<cgi_pair_t> (true),
    cookie (ck), 
    inhex (false), 
    pstate (cookie ? CGI_CKEY : CGI_KEY), 
    hex_i (0), 
    hex_h (0), 
    hex_lch (0), 
    uri_mode (false),
    _scratch (s),
    _maxlen (-1)
{
  init ();
}

//-----------------------------------------------------------------------

bool
cgi_t::extend_scratch ()
{
  bool ret = false;
  ssize_t limit = _maxlen;
  if (limit < 0 || limit > (ssize_t) ok_cgibuf_limit) { 
    limit = ok_cgibuf_limit; 
  }
  if ((ssize_t) _scratch->len() >= limit) {
    return ret;
  }
  ssize_t newlen = _scratch->len() * 2;
  if (newlen > limit) { 
    newlen = limit; 
  }
  ptr<ok::scratch_handle_t> n = _scratch->grow (newlen);
  if (n) {
    ret = true;
    _scratch = n;
    endp = _scratch->end ();
  }
  return ret;
}

//-----------------------------------------------------------------------

void
cgi_t::reset_state ()
{
  pstate = cookie ? CGI_CKEY : CGI_KEY;
  inhex = false;
  hex_i = 0;
  hex_h = 0;
  hex_lch = 0;
  uri_mode = false;
}

//-----------------------------------------------------------------------

cgi_t::cgi_t (abuf_src_t *s, bool ck, ptr<ok::scratch_handle_t> scr)
  : async_parser_t (s), 
    pairtab_t<cgi_pair_t> (true),
    cookie (ck), 
    inhex (false), 
    pstate (cookie ? CGI_CKEY : CGI_KEY), 
    hex_i (0), 
    hex_h (0), 
    hex_lch (0), 
    uri_mode (false),
    _scratch (scr),
    _maxlen (-1)
{
  init ();
}

//-----------------------------------------------------------------------

cgi_t::~cgi_t ()
{
  tab.deleteall ();
  _scratch = NULL;
}

//-----------------------------------------------------------------------

void
cgi_t::parse_guts ()
{
  abuf_stat_t rc;
  do { rc = parse_guts_driver (); } while (rc != ABUF_EOF && rc != ABUF_WAIT);
  if (rc == ABUF_EOF) {
    finish_parse (HTTP_OK);
  }
}

//-----------------------------------------------------------------------

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
    

//-----------------------------------------------------------------------

abuf_stat_t
cgi_t::parse_guts_driver ()
{
  if (parse_still_waiting())
    return ABUF_WAIT;

  str s;
  abuf_stat_t rc = parse_key_or_val (&s);
  return process_parsed_key_or_val(rc, s);
}

//-----------------------------------------------------------------------

bool
cgi_t::parse_still_waiting()
{
  // MK 12/27/07: bugfix.  In cookie mode, make sure
  // we don't gobble up \r\n's.  If so, we might fail
  // if the browser gave an empty Cookie: line!!
  // Note we only advance past white space in the case of
  // a Cookie: line in the first place.
  return (pstate             == CGI_CKEY             && 
	  pcp                == _scratch->buf ()     && 
	  abuf->skip_hws (0) == ABUF_WAIT);
}

//-----------------------------------------------------------------------

abuf_stat_t
cgi_t::process_parsed_key_or_val(abuf_stat_t rc, str& s)
{
  switch (rc) {
  case ABUF_SEPARATOR:
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
  case ABUF_PARSE_ERR:
    // MW: this is definitely unexpected... especially since
    // parse_key_or_val(&s,vt) doesn't return this error code!
    panic ("Unexpected ABUF_PARSE_ERR return in CGI parse routines\n");
    break;
  default:
    break;
  }
  return rc;
}

//-----------------------------------------------------------------------

abuf_stat_t
cgi_t::parse_key_or_val (str *r, bool use_internal_state)
{
  int ch;
  bool flag = true;
  abuf_stat_t ret = ABUF_OK;
  abuf_stat_t rc;
  // this line replaces a switch that used to happen implicitly 
  // based on whatever the caller passed in. feels cleaner to me not to
  // pass a member variable in as a parameter. probably that was
  // happening b/c what was meant was "am I using the internal state or
  // not? if not, I need another switch." the line below makes that
  // explicit.
  cgi_var_t vt = use_internal_state ? pstate : CGI_NONE;

  if (inhex) {
    rc = parse_hexchar (&pcp, endp);
    if (rc == ABUF_WAIT || rc == ABUF_EOF)
      return rc;
    else
      inhex = false;
  }

    
  while ( flag && ret == ABUF_OK) {

    if (pcp >= endp) {
      size_t off = pcp - _scratch->buf ();
      if (extend_scratch ()) {
	pcp = _scratch->buf () + off;
	assert (pcp < endp);
      } else {
	ret = ABUF_OVERFLOW;
	break;
      }
    }

    ch = abuf->get ();
    switch (ch) {
    case CGI_SEPCHAR:
      if (vt == CGI_KEY)
	ret = ABUF_SEPARATOR;
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
	ret = ABUF_SEPARATOR;
      else if (vt == CGI_CVAL)
	flag = false;
      else
	*pcp++ = ch;
      break;
    case CGI_CRCHAR:
      if (uri_mode) {
	ret = ABUF_EOF;
      } else if (!COOKIE_STATE (vt)) {
	*pcp++ = ch;
      }
      break;
    case CGI_EOLCHAR:
      if (COOKIE_STATE (vt) || uri_mode) {
	abuf->unget ();
	ret = ABUF_EOF;
      } else {
	*pcp++ = ch;
      }
      break;
    case ABUF_EOFCHAR:
      ret = ABUF_EOF;
      break;
    case ABUF_WAITCHAR:
      ret = ABUF_WAIT;
      break;
    case CGI_HEXCHAR:
      inhex = true;
      rc = parse_hexchar (&pcp, endp);
      if (rc == ABUF_WAIT || rc == ABUF_EOF) {
	ret = rc;
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
	break;
      } // otherwise fall through to default
    default:
      *pcp++ = ch;
      break;
    }
  }

  char *scratch = _scratch->buf ();
  if (ret != ABUF_WAIT) {
    assert (ret == ABUF_OK || ret == ABUF_SEPARATOR || ret == ABUF_EOF ||
	    ret == ABUF_OVERFLOW);
    if (ret == ABUF_OVERFLOW || pcp >= endp) {
      *r = NULL;
      ret =  ABUF_OVERFLOW;
    } else {
      if (ok_filter_cgi == XSSFILT_ALL)
	*r = xss_escape (scratch, pcp - scratch);
      else
	*r = str (scratch, pcp - scratch);
    }
    pcp = scratch; // reset for next time
  }
  return ret;
}

//-----------------------------------------------------------------------

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
    else if (ch == ABUF_EOFCHAR)
      return ABUF_EOF;
    else if ((t = char_to_hex (ch)) < 0) {
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

//-----------------------------------------------------------------------

ptr<cgi_t>
cgi_t::str_parse (const str &s)
{
  abuf_str_t cgis (s);
  ptr<ok::scratch_handle_t> scr = ok::alloc_scratch (ok_dflt_cgibuf_sz);
  ptr<cgi_t> r = New refcounted<cgi_t, vbase> (&cgis, false, scr);
  r->parse (NULL);
  return r;
}

//-----------------------------------------------------------------------

str
expire_in (int d, int h, int m, int s, rfc_number_t rfc)
{
#define MAXLEN 60
  h += d*24;
  m += h*60;
  s += m*60;
  time_t t = sfs_get_timenow() + s;
  mstr out (MAXLEN);
  struct tm *stm = gmtime (&t);
  const char *fmt = rfc_date_fmt (rfc);
  size_t l = strftime (out.cstr (), MAXLEN, fmt, stm);
  out.setlen (l);
  return out;
#undef MAXLEN
}


//-----------------------------------------------------------------------

ptr<cgi_t>
cgi_t::alloc (abuf_t *a, bool ck, ptr<ok::scratch_handle_t> h)
{
  return New refcounted<cgi_t, vbase> (a, ck, h);
}

//-----------------------------------------------------------------------

ptr<cgi_t>
cgi_t::alloc (abuf_src_t *a, bool ck, ptr<ok::scratch_handle_t> h)
{
  return New refcounted<cgi_t, vbase> (a, ck, h);
}

//-----------------------------------------------------------------------

static ptr<cgi_t> g_empty;

ptr<const cgi_t>
cgi_t::global_empty ()
{
  if (!g_empty) {
    g_empty = New refcounted<cgi_t> ();
  }
  return g_empty;
}

//-----------------------------------------------------------------------
