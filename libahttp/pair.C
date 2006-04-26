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

#include "pair.h"

void
pair_t::dump1 () const
{
  warnx << key;
  if (vals.size () == 0) {
    warnx << "\n";
    return;
  }
  warnx << " --> ";
  if (vals[0])
    warnx << vals[0];

  warnx << "\n";
}

//
// XXX - todo -- fold this and the following into one function
//
vec<u_int64_t> *
pair_t::to_uint64 () const
{
  vec<u_int64_t> *ret = NULL;
  const char *nptr;
  char *endptr;
  size_t s;

  switch (uis) {
  case IV_ST_NONE:
    s = vals.size ();
    uivals.setsize (s);
    for (u_int i = 0; i < s; i++) {

      nptr = vals[i];
      endptr = NULL;

      errno = 0;
      uivals[i] = strtoull (nptr, &endptr, 0);
      if (*endptr || endptr == nptr) {
	uis = IV_ST_FAIL;
	break;
      }
    }
    if (uis != IV_ST_FAIL) {
      uis = IV_ST_OK;
      ret = &uivals;
    }
    break;
  case IV_ST_OK:
    ret = &uivals;
    break;
  case IV_ST_FAIL:
  default:
    break;
  }

  return ret;
}

vec<int64_t> *
pair_t::to_int () const
{
  vec<int64_t> *ret = NULL;
  const char *nptr;
  char *endptr;
  size_t s;

  switch (is) {
  case IV_ST_NONE:
    {
      s = vals.size ();
      ivals.setsize (s);
      for (u_int i = 0; i < s; i++) {

	nptr = vals[i];
	endptr = NULL;

	errno = 0;
	ivals[i] = strtoll (nptr, &endptr, 0);
	if (*endptr || endptr == nptr || errno == ERANGE) {
	  is = IV_ST_FAIL;
	  break;
	}
      }
      if (is != IV_ST_FAIL) {
	is = IV_ST_OK;
	ret = &ivals;
      }
      break;
    }
  case IV_ST_OK:
    ret = &ivals;
    break;
  case IV_ST_FAIL:
  default:
    break;
  }
  return ret;
}

bool
pair_t::to_int (int64_t *v) const
{
  vec<int64_t> *p = to_int ();
  if (p && p->size () > 0) {
    *v = (*p)[0];
    return true;
  } 
  return false;
}

bool
pair_t::to_uint64 (u_int64_t *v) const
{
  vec<u_int64_t> *p = to_uint64 ();
  if (p && p->size () > 0) {
    *v = (*p)[0];
    return true;
  }
  return false;
}

char *xss_buf = NULL;
size_t xss_buflen = 0x1000;

str xss_filter (const str &s) { return xss_filter (s.cstr (), s.len ()); }

str
xss_filter (const char *in, u_int inlen)
{
  int maxseqlen = 5;

  size_t biggest = maxseqlen * inlen;
  if (xss_buflen < biggest && xss_buflen != XSS_MAX_BUF && xss_buf) {
    delete [] xss_buf;
    xss_buf = NULL;
  }
  if (!xss_buf) {
    while (xss_buflen < biggest && xss_buflen < XSS_MAX_BUF)
      xss_buflen = (xss_buflen << 1);
    xss_buflen = min<size_t> (xss_buflen, XSS_MAX_BUF);
    xss_buf = New char[xss_buflen];
  }

  char *op = xss_buf;
  size_t outlen = 0;
  size_t inc;
  const char *end = in + inlen;
  for (const char *cp = in; 
       cp < end && maxseqlen + outlen < xss_buflen; 
       cp++) {
    switch (*cp) {
    case '<':
      inc = sprintf (op, "&lt;");
      break;
    case '>':
      inc = sprintf (op, "&gt;");
      break;
    case '(':
      inc = sprintf (op, "&#40;");
      break;
    case '&':
      inc = sprintf (op, "&#38;");
      break;
    case '#':
      inc = sprintf (op, "&#35;");
      break;
    default:
      *op = *cp;
      inc = 1;
      break;
    }
    outlen += inc;
    op += inc;
  }
  return str (xss_buf, outlen);
}


