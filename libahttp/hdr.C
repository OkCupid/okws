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

#include "hdr.h"
#include "httpconst.h"
#include <ctype.h>

abuf_stat_t
http_hdr_t::delimit_word (str *wrd, bool qms)
{
  int ch = ABUF_WAITCHAR;
  abuf_stat_t ret = ABUF_OK;
  bool flag = true;
  for ( ; pcp < endp && flag; pcp += (flag ? 1 : 0)) {
    ch = abuf->get ();
    switch (ch) {
    case ABUF_WAITCHAR:
      ret = ABUF_WAIT;
      flag = false;
      break;
    case '?':
      if (qms) {
	ret = ABUF_OK;
	flag = false;
      } else {
	*pcp = ch;
      }
      break;
    case ABUF_EOFCHAR:
    case ' ':
    case '\t':
    case '\n':
    case '\r':
      ret = ABUF_OK;
      flag = false;
      break;
    default:
      *pcp = ch;
      break;
    }
  }

  char *scratch = _scratch->buf ();

  if (ch == ABUF_EOFCHAR && pcp == scratch) {
    ret = ABUF_EOF;
  } else if (pcp == endp) {
    ret = ABUF_OVERFLOW;
  } else if (ret == ABUF_OK) {
    *wrd = str (scratch, pcp - scratch);
    pcp = scratch;
    abuf->unget ();
  }

  return ret;
}

abuf_stat_t
http_hdr_t::eol ()
{
  int ch = abuf->peek ();
  if (ch == ABUF_WAITCHAR) return ABUF_WAIT;
  return ((ch == '\r' || ch == '\n') ? ABUF_OK : ABUF_NOMATCH);
}

//
// http_hdr_t::gobble_crlf
//
//   Looks to eat up a CRLF sequence at the current buffer
//   position, but will not signal an error if none is found.
//   call this function when you're not sure if there's a CRLF,
//   and it's OK for there not to be (such as at the beginning
//   of a new line in an HTTP header).
//
//   Note that this function is somewhat buggy in that it doesn
//   not handle stray '\r' characters well.  This should be fine 
//   in practice as these characters should only appear before a '\n'
//
//   If this function finds something other than a CRLF 
//   (and not a spurious '\r' character) it will unget the
//   last character gotten, so that it can be parsed by
//   some other function.
//
// Return values:
//   ABUF_OK - found CRLF
//   ABUF_EOF - found EOF
//   ABUF_WAIT - need to wait on more data
//   ABUF_NOMATCH - found something other than a CRLF
//   ABUF_PARSE_ERR - stray, unexpected '\r' found
//
abuf_stat_t
http_hdr_t::gobble_crlf ()
{
  abuf_stat_t ret = ABUF_OK;
  bool flag = true;
  while (flag && ret == ABUF_OK) {
    int ch = abuf->get ();
    switch (ch) {
    case '\n':
      CRLF_need_LF = false;
      ret = ABUF_OK;
      flag = false;
      break;
    case '\r':
      if (CRLF_need_LF) {
	CRLF_need_LF = false;
	ret = ABUF_PARSE_ERR;
      } else 
	CRLF_need_LF = true;
      break;
    case ABUF_EOFCHAR:
      ret = ABUF_EOF;
      break;
    case ABUF_WAITCHAR:
      ret = ABUF_WAIT;
      break;
    default:
      abuf->unget (); // XXX might lose a '\r' -- oh well...
      if (CRLF_need_LF) {
	CRLF_need_LF = false;
	ret = ABUF_PARSE_ERR;
      } else
	ret = ABUF_NOMATCH;
      break;
    }
  }
  return ret;
}

//
// http_hdr_t::require_clrf
//
//   Much like gobble_crlf, but a CRLF is required.  Call this
//   function when it's a parse error if a CRLF is not seen,
//   such as at the end of a line in an HTTP header.
//   
abuf_stat_t
http_hdr_t::require_crlf ()
{
  abuf_stat_t r = gobble_crlf ();
  if (r == ABUF_NOMATCH)
    r = ABUF_PARSE_ERR;
  return r;
}

abuf_stat_t
http_hdr_t::delimit_val (str *v)
{
  abuf_stat_t ret = ABUF_OK;
  int ch;
  bool flag = true;
  for ( ; pcp < endp && flag; pcp += (flag ? 1 : 0)) {
    ch = abuf->get ();
    switch (ch) {
    case ABUF_WAITCHAR:
      ret = ABUF_WAIT;
      flag = false;
      break;
    case ABUF_EOFCHAR:
      ret = ABUF_EOF;
      flag = false;
      break;
    case '\r':
    case '\n':
      abuf->unget ();
      flag = false;
      break;
    default:
      *pcp = ch;
      break;
    }
  }
  if (pcp == endp)
    ret = ABUF_OVERFLOW;
  if (ret == ABUF_OK) {
    char *sb = _scratch->buf ();
    *v = str (sb, pcp - sb);
    pcp = sb;
  }
  return ret;
}

abuf_stat_t
http_hdr_t::delimit_status (str *v)
{
  return delimit_val (v);
}

abuf_stat_t
http_hdr_t::force_match (const char *s, bool tol)
{
  assert (s && *s);

  // MK Note 11/5/07: Thanks to Benjie Chen for submitting a patch
  // that fixes earlier bugs in this function.

  const char *cp;
  if (s == curr_match) {
    // In this first case, we're matching the same field as we did 
    // last time, and therefore, we pick off where the last attempt to
    // match left off. 
    cp = fmcp;
  } else {
    // In this second case, we're matching on a new string, so throw
    // away the old state in 'fmcp' and start from the beginning of
    // the given string.
    cp = curr_match = s;
  }

  abuf_stat_t ret = ABUF_CONTINUE;
  int ch;
  while (ret == ABUF_CONTINUE) {
    ch = abuf->get ();
    if (ch == ABUF_WAITCHAR) 
      ret = ABUF_WAIT;
    else {
      if ((tol ? tolower (ch) : ch) != (tol ? tolower (*cp) : *cp))
	ret = ABUF_PARSE_ERR;
      else {
	cp++;
	if (!*cp) {
	  ret = ABUF_OK;
	}
      }
    }
  }

  if (ret == ABUF_OK || ret == ABUF_PARSE_ERR) {
    // If we won't be matching on this string anymore, reset state
    // for the next time through.
    fmcp = curr_match = NULL;
  } else {
    // We're waiting, in which case we want to pick up the match from
    // where we left off.
    fmcp = cp;
  }

  return ret;
}

abuf_stat_t
http_hdr_t::delimit (str *k, char stopchar, bool tol, bool gobble)
{
  abuf_stat_t ret = ABUF_CONTINUE;
  int ch;
  bool adv;

  for ( ; pcp < endp && ret == ABUF_CONTINUE; pcp += (adv ? 1 : 0)) {
    adv = false;
    ch = abuf->get ();
    if (ch == stopchar) {
      if (!gobble)
	abuf->unget ();
      ret = ABUF_OK;
    } else {
      switch (ch) {
      case ABUF_WAITCHAR:
	ret = ABUF_WAIT;
	break;
      case ABUF_EOFCHAR:
	ret = ABUF_EOF;
	break;
      case '\n':
      case '\r':
	ret = ABUF_PARSE_ERR;
	abuf->unget ();
	break;
      default:
	adv = true;
	*pcp = tol ? tolower (ch) : ch;
	break;
      }
    }
  }
  if (pcp == endp)
    ret = ABUF_OVERFLOW;
  if (ret == ABUF_OK) { 
    char *sb = _scratch->buf ();
    *k = str (sb, pcp - sb);
    pcp = sb;
  }
  return ret;
}

abuf_stat_t
http_hdr_t::delimit_key (str *k)
{
  abuf_stat_t ret = ABUF_OK;
  int ch;
  bool flag = true;
  for ( ; pcp < endp && flag; pcp += (flag ? 1 : 0)) {
    ch = abuf->get ();
    switch (ch) {
    case ABUF_WAITCHAR:
      ret = ABUF_WAIT;
      flag = false;
      break;
    case ':':
      ret = ABUF_OK;
      flag = false;
      break;
    case ABUF_EOFCHAR:
      ret = ABUF_EOF;
      flag = false;
      break;
    case ' ':
    case '\t':
    case '\n':
    case '\r':
      ret = ABUF_PARSE_ERR;
      flag = false;
      break;
    default:
      *pcp = tolower (ch);
      break;
    }
  }
  if (pcp == endp)
    ret = ABUF_OVERFLOW;
  if (ret == ABUF_OK) {
    char *sb = _scratch->buf ();
    *k = str (sb, pcp - sb);
    pcp = sb;
  }
  return ret;
}

void 
http_hdr_t::reset ()
{
  async_parser_t::reset ();
  pcp = _scratch->buf ();
  CRLF_need_LF = false;
  nvers = 0;
  noins = false;
  curr_match = NULL;
  fmcp = NULL;
}
