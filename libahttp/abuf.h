
// -*-c++-*-
/* $Id$ */

#ifndef _LIBAHTTP_ABUF_H
#define _LIBAHTTP_ABUF_H

#include "suiolite.h"
#include "ahttp.h"

typedef enum { ABUF_OK = 0, ABUF_PARSE_ERR = 1, ABUF_EOF = 2, 
	       ABUF_OVERFLOW = 3, ABUF_WAIT = 4, ABUF_NOMATCH = 5,
	       ABUF_CONTINUE = 6 } abuf_stat_t;

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
  int len;
};

class abuf_src_t {
public:
  virtual void init (cbv cb) = 0;
  virtual abuf_indata_t getdata () = 0;
  virtual void rembytes (int nbytes) = 0;
  virtual void finish () = 0;
  virtual void cancel () {}
  virtual ~abuf_src_t () 
  { warn << "~abuf_src_t called\n"; } // debug
  virtual bool overflow () const { return false; }
};

class abuf_con_t : public abuf_src_t {
public:
  abuf_con_t (ptr<ahttpcon> xx) 
    : x (xx), in (x->uio ()), eof (x->ateof ()) {}
  ~abuf_con_t () 
  { 
    warn << "~abuf_con_t called\n"; // debug
    finish (); 
  }
  void init (cbv c);
  void readcb (int n);
  abuf_indata_t getdata ();
  void rembytes (int n) { in->rembytes (n); }
  void finish () {  x->setrcb (NULL); }
  void cancel () { cb = NULL; }
  bool overflow () const { return x->overflow (); }

private:
  ptr<ahttpcon> x;
  suiolite *in;
  cbv::ptr cb;
  bool eof;
};

class abuf_t {
public:
  abuf_t (abuf_src_t *s, bool d = false)
    : src (s), bc (false), lch (0), buf (NULL), endp (NULL), cp (NULL), 
    len (0), erc (ABUF_OK), delsrc (d), spcs (0),
    lim (-1), ccnt (0), mirror_base (NULL), mirror_p (NULL),
    mirror_end (NULL) {}

  ~abuf_t () 
  { 
    warn ("~abuf_t called (delsrc: %d, src: %p)\n", 
	  delsrc ? 1 : 0, src); // debug;
    if (delsrc && src) {
      delsrc = false;
      delete src; 
      src = NULL;
    }
  }

  inline int get ();
  inline int peek ();
  inline void unget () { bc = true; }
  inline abuf_stat_t skip_ws ();
  inline abuf_stat_t skip_hws (int mn = 0);
  inline abuf_stat_t expectchar (char c);
  inline abuf_stat_t requirechar (char c);
  void finish ();
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

  void reset () 
  {
    spcs = 0;
    mirror_p = NULL;
  }

private:
  void moredata ();

  abuf_src_t *src;
  bool bc;
  int lch;
  const char *buf, *endp, *cp;
  int len;
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
  return ABUF_OK;
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
     *mirror_p++ = *cp;                       \
  return (lch = *cp++)

int
abuf_t::get ()
{
  if (bc) { 
    bc = false; 
    return lch; 
  }

  if (lim == ccnt)
    return ABUF_EOFCHAR;

  if (cp < endp) {
    ABUF_T_GET_GOT_CHAR;
  }

  if (erc == ABUF_OK) {
    moredata ();
    if (erc == ABUF_OK && cp < endp) {
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
