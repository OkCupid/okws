
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


#ifndef _LIBAHTTP_ABUF_H
#define _LIBAHTTP_ABUF_H

#include "suiolite.h"
#include "ahttp.h"

typedef enum { ABUF_OK = 0, ABUF_PARSE_ERR = 1, ABUF_EOF = 2, 
	       ABUF_OVERFLOW = 3, ABUF_WAIT = 4, ABUF_NOMATCH = 5,
	       ABUF_CONTINUE = 6, ABUF_COMPLETE = 7,
               ABUF_SEPARATOR = 8 } abuf_stat_t;

#define ABUF_EOFCHAR  -0xffff0
#define ABUF_WAITCHAR -0xffff1

#define IS_CONTROL_CHAR(x) \
 ((x) <= ABUF_EOFCHAR)

struct abuf_indata_t {
  abuf_indata_t (abuf_stat_t e = ABUF_EOF, const char *b = NULL, int l = 0)
    : erc (e), bp (b), len (l) {}
  abuf_indata_t (suiolite *in);
  abuf_stat_t erc;
  const char *bp;
  ssize_t len;
};

class abuf_src_t {
public:
  virtual void init (cbv cb) = 0;
  virtual abuf_indata_t getdata () = 0;
  virtual void rembytes (int nbytes) = 0;
  virtual void finish () = 0;
  virtual void cancel () {}
  virtual ~abuf_src_t () {}
  virtual bool overflow () const { return false; }
};

class abuf_con_t : public abuf_src_t {
public:
  abuf_con_t (ptr<ahttpcon> xx) 
    : x (xx), in (x->uio ()), eof (x->ateof ()) {}
  ~abuf_con_t () { finish (); }
  void init (cbv c);
  void readcb (int n);
  abuf_indata_t getdata ();
  void rembytes (int n) { in->rembytes (n); }
  void finish () {  x->setrcb (NULL); }
  void cancel () { cb = NULL; }
  bool overflow () const { return x->overflow (); }
  suiolite *uio () const { return in; }

private:
  ptr<ahttpcon> x;
  suiolite *in;
  cbv::ptr cb;
  bool eof;
};

class abuf_t {
public:
  abuf_t (abuf_src_t *s = NULL, bool d = false)
    : src (s), bc (false), lch (0), _basep (NULL), _endp (NULL), _cp (NULL), 
    len (0), erc (ABUF_OK), delsrc (d), spcs (0),
    lim (-1), ccnt (0), mirror_base (NULL), mirror_p (NULL),
    mirror_end (NULL) {}

  ~abuf_t () 
  { 
    if (delsrc && src) {
      delsrc = false;
      delete src; 
      src = NULL;
    }
  }

  inline int get ();
  inline int peek ();

  //
  // MK 2011/7/27
  //
  // NOTE! It's not a good idea to "put back" the WAIT characters
  // since we want to probe for more data the next time through...
  // We've decided to fix this by just not setting the bc (= "buffered char")
  // flag in this situation.
  //
  inline void unget () { if (lch != ABUF_WAITCHAR) { bc = true; } }

  inline abuf_stat_t skip_ws ();
  abuf_stat_t skip_hws (int mn = 0);
  inline abuf_stat_t expectchar (char c);
  inline abuf_stat_t requirechar (char c);
  void finish ();
  void setsrc (abuf_src_t *s, bool d) { src = s; delsrc = d; }
  inline void can_read () { erc = ABUF_OK; }
  abuf_src_t *getsrc () const { return src; }
  void init (cbv c) { src->init (c); }
  inline void setlim (int l) { lim = l; ccnt = 0; }
  void cancel () { if (src) src->cancel (); }
  void mirror (char *p, u_int len);
  str end_mirror ();
  str end_mirror2 (int sublen = 0); // slightly different CRLF semantics
  str mirror_debug ();
  bool overflow () const { return src->overflow (); }
  size_t flush (char *c, size_t l); // flushes buffered data to buffer.
  ssize_t dump (char *buf, size_t len);

  // stream the data in the buffer out in arbitrary-sized blocks
  ssize_t stream (const char **bp);

  void reset () 
  {
    spcs = 0;
    mirror_p = NULL;
  }

  int get_ccnt() const { return ccnt - (bc ? 1 : 0); }

private:
  void moredata ();
  ssize_t get_errchar () const;

  abuf_src_t *src;
  bool bc;   // flag that's on if a char is buffered (due to unget ())
  int lch;
  const char *_basep, *_endp, *_cp;
  ssize_t len;
  abuf_stat_t erc;
  bool delsrc;
  int spcs;  // spaces;
  int lim;
  int ccnt;  // char count

  char *mirror_base;
  char *mirror_p;
  char *mirror_end;

};

class abuf_str_t : public abuf_src_t {
public:
  abuf_str_t (const str &ss) 
    : alloc (false), st (ss), s (ss.cstr ()), len (ss.len ()), 
      eof (len == 0) {}
  abuf_str_t (const char *ss, bool cp = true, int l = 0);
  ~abuf_str_t () { if (alloc && s) delete [] s; }

  void init (cbv c) { (*c) (); }
  abuf_indata_t getdata ();
  void rembytes (int n) {}
  void finish () {}

private:
  bool alloc;
  str st;
  const char *s;
  int len;
  bool eof;
};

abuf_stat_t
abuf_t::skip_ws ()
{
  int ch;
  do { ch = get (); } while (ch > 0 && isspace (ch));
  switch (ch) {
  case ABUF_WAITCHAR:
    return ABUF_WAIT;
  case ABUF_EOFCHAR:
    return ABUF_EOF;
  default:
    unget ();
    return ABUF_OK;
  }
}

abuf_stat_t
abuf_t::expectchar (char ch)
{
  int c = get ();
  if (ch == c) return ABUF_OK;
  unget ();
  switch (c) {
  case ABUF_WAITCHAR:
    return ABUF_WAIT;
  case ABUF_EOFCHAR:
    return ABUF_EOF;
  default:
    return ABUF_NOMATCH;
  }
}

abuf_stat_t
abuf_t::requirechar (char ch)
{
  abuf_stat_t r = expectchar (ch);
  if (r == ABUF_NOMATCH) r = ABUF_PARSE_ERR;
  return r;
}

#define ABUF_T_GET_GOT_CHAR                   \
  ccnt ++;                                    \
  if (mirror_p && mirror_p < mirror_end)      \
     *mirror_p++ = *_cp;                      \
  return (lch = *_cp++)

int
abuf_t::get ()
{
  if (bc) { 
    bc = false; 
    return lch; 
  }

  if (lim == ccnt)
    return ABUF_EOFCHAR;

  if (_cp < _endp) {
    ABUF_T_GET_GOT_CHAR;
  }

  if (erc == ABUF_OK) {
    moredata ();
    if (erc == ABUF_OK && _cp < _endp) {
      ABUF_T_GET_GOT_CHAR;
    }
  }
  return (lch = (erc == ABUF_WAIT ? ABUF_WAITCHAR : ABUF_EOFCHAR));
}


int
abuf_t::peek ()
{
  int r = get ();
  unget ();
  return r;
}


#endif /* _LIBAHTTP_ABUF_H */
