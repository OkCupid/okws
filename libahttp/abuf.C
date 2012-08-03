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


#include "abuf.h"
#include "httpconst.h"
#include "pubutil.h"

void
abuf_t::moredata ()
{
  if (len > 0) {
    src->rembytes (len);
  }
  abuf_indata_t in = src->getdata ();
  _basep = _cp = in.bp;
  len = in.len;

  _endp = _basep + in.len;
  erc = in.erc;

}

void 
abuf_t::finish ()
{
  ssize_t nbytes = _cp - _basep;

  assert (nbytes >= 0);
  assert (len >= nbytes);

  src->rembytes (nbytes);


  // especially important if we're going to reuse this abuf
  len -= nbytes;

  // move the base buf pointer up, making potentially some data
  // still available...
  _basep = _cp;

  src->finish ();
}

void
abuf_con_t::init (cbv c)
{
  cb = c;
  x->setrcb (wrap (this, &abuf_con_t::readcb));
}

void
abuf_con_t::readcb (int n)
{
  if (n == 0) 
    eof = true;
  if (cb)
    (*cb) ();
}

abuf_indata_t
abuf_con_t::getdata ()
{
  if (eof) return abuf_indata_t ();
  return abuf_indata_t (in);
}

abuf_indata_t::abuf_indata_t (suiolite *in)
{
  bp = in->getdata (&len);
  erc = len ? ABUF_OK : ABUF_WAIT;
}

abuf_indata_t
abuf_str_t::getdata ()
{
  if (eof) 
    return abuf_indata_t ();
  else {
    eof = true;
    return abuf_indata_t (ABUF_OK, s, len);
  }
}

abuf_str_t::abuf_str_t (const char *ss, bool cp, int l) : len (l)
{
  if (!len && ss)
    len = strlen (ss);
  if (cp) {
    alloc = true;
    char *t = New char[len];
    memcpy (t, ss, len);
    s = t;
  } else {
    s = ss;
  }
  eof = len > 0;
}

void
abuf_t::mirror (char *p, u_int len)
{
  mirror_base = mirror_p = p;
  mirror_end = mirror_base + len;
}

str
abuf_t::end_mirror ()
{
  while ((mirror_p > mirror_base) && 
	 (mirror_p[-1] == '\r' || mirror_p[-1] == '\n'))
    mirror_p--;
  str r = str (mirror_base, mirror_p - mirror_base);
  mirror_base = mirror_p = mirror_end = NULL;
  return r;
}

str
abuf_t::mirror_debug ()
{
  return str (mirror_base, mirror_p - mirror_base);
}

str 
abuf_t::end_mirror2 (int sublen)
{
  assert (mirror_p - mirror_base >= sublen);
  mirror_p -= sublen;
  *mirror_p = 0;
  if (mirror_p >= mirror_base + 2 && mystrcmp (mirror_p - 2, HTTP_CRLF))
    mirror_p -= 2;
  str r = str (mirror_base, mirror_p - mirror_base);
  mirror_base = mirror_p = mirror_end = NULL;
  return r;
}

size_t
abuf_t::flush (char *buf, size_t len)
{
  if (len == 0 || _cp == _endp)
    return 0;
  
  if (bc) {
    *buf++ = lch;
    len--;
  }
    
  size_t readlen = min<size_t> (len, _endp - _cp);
  memcpy (buf, _cp, readlen);
  _cp += readlen;
  ccnt += readlen;
  
  // add the buffered char to the tally
  readlen += (bc ? 1 : 0);
  bc = false;

  return readlen;
}

ssize_t
abuf_t::get_errchar () const
{
  if (lim >= 0 && ccnt >= lim) return ABUF_EOFCHAR;

  switch (erc) {
  case ABUF_EOF:
    return ABUF_EOFCHAR;
  case ABUF_WAIT:
    return ABUF_WAITCHAR;
  default:
    break;
  }
  return 0;
}

ssize_t
abuf_t::dump (char *buf, size_t len)
{
  ssize_t r;
  if ((r = get_errchar ()) < 0) return r;

  char *buf_p = buf;

  //
  // XXX:  involves some extra memcopies, but this will have to do for now.
  //
  ssize_t spaceleft = len;
  while (spaceleft > 0 && erc == ABUF_OK) {
    size_t rc = flush (buf_p, spaceleft);
    spaceleft -= rc;
    buf_p += rc;
    if (spaceleft > 0) 
      moredata ();
  }
  assert (spaceleft >= 0);
  return (buf_p - buf);
}

ssize_t
abuf_t::stream (const char **bp)
{
  static char c;
  ssize_t r;
  if (bc) {
    assert (lch >= 0);
    c = lch;
    
    *bp = &c;
    bc = false;
    return 1;
  }

  if ((r = get_errchar ()) < 0) return r;
  if (_endp - _cp == 0) {
    moredata ();
    if ((r = get_errchar ()) < 0) return r;
  }

  assert (erc == ABUF_OK);
  *bp = _cp;
  ssize_t ret = _endp - _cp;
  _cp = _endp;
  ccnt += ret;
  return ret;
}

abuf_stat_t
abuf_t::skip_hws (int mn)
{
  int ch;
  do { ch = get (); spcs ++; } while (ch == ' ' || ch == '\t');
  spcs --;
  if (ch == ABUF_WAITCHAR)
    return ABUF_WAIT;
  abuf_stat_t ret = ABUF_OK;
  if (spcs < mn)
    ret = ABUF_PARSE_ERR;
  spcs = 0;
  unget ();
  return ret;
}
