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

#ifndef _LIBAZ_ZSTR
#define _LIBAZ_ZSTR

#include "str.h"
#include <zlib.h>
#include "ihash.h"
#include "list.h"
#include "qhash.h"
#include "async.h"
#include "okconst.h"

#define ZSTR_ENDBUF_SIZE       0x10
#define Z_DISABLE -2

class ztab;
class zstrobj {
public:
  zstrobj (const str &ss) : s (ss) {}
  zstrobj (const str &ss, const str &zz, int l) : s (ss), zs (zz), clev (l) {}
  size_t len () const { return s ? s.len () : 0; }
  const char *dat () const { return s ? s.cstr () : NULL; }
  const str &to_zstr (int level) const;
  inline const str &to_str () const { return s; }
  inline bool compressed () const { return (zs && zs.len () > 0); }

  inline uLong crc32 (uLong in) const 
  { return ::crc32 (in, reinterpret_cast<const Bytef *> (s.cstr ()), 
		    s.len ()); }
private:
  friend class zstr;
  bool compress (int l) const;
  const str s;
  mutable str zs;
  mutable int clev;
};

class zstr {
public:
  zstr () : _scc_p (0) {}
  zstr (const str &s) : b (New refcounted<zstrobj> (s)), _scc_p (0) {}
  zstr (const str &s, const str &z, int l) : 
    b (New refcounted<zstrobj> (s, z, l)), _scc_p (0) { }
  zstr (const zstr &z) : b (z.b) , _scc_p (0) {}
  zstr (const char *p) : 
    b (p ? New refcounted<zstrobj> (p) : NULL), _scc_p (0) {}
  zstr (const char *p, size_t l) : 
    b (New refcounted<zstrobj> (str (p, l))), _scc_p (0) {}
  zstr (const strbuf &bb) : 
    b (New refcounted<zstrobj> (bb)), _scc_p (0) {}

  zstr &operator= (const zstr &z) { b = z.b; _scc_p = 0; return *this; }

  zstr &operator= (const char *p) 
  { 
    b = p ? New refcounted<zstrobj> (p) : NULL; 
    _scc_p = 0;
    return *this;
  }

  zstr &operator= (const str &s) 
  { 
    b = s ? New refcounted<zstrobj> (s) : NULL; 
    _scc_p = 0;
    return *this;
  }

  size_t len () const { return b ? b->len () : 0; }
  const char *cstr () const { return b ? b->dat () : NULL; }
  operator const char *() const { return cstr (); }
  const str &to_str () const { return b->s; }
  operator str() const { return to_str (); }
  char operator[] (ptrdiff_t n) const {
#ifdef CHECK_BOUNDS
    assert (size_t (n) <= len ());
#endif /* CHECK_BOUNDS */
    return (cstr ()[n]);
  }
  int cmp (const str &s) const { return b->s.cmp (s); }
  int cmp (const char *p) const { return b->s.cmp (p); }
  bool operator== (const zstr &z) const { return z.to_str () == b->s; }
  bool operator!= (const zstr &z) const { return z.to_str () != b->s; }
  bool operator== (const str &s) const { return (s == b->s); }
  bool operator!= (const str &s) const { return (s != b->s); }
  bool operator< (const str &s) const { return (b->s < s); }
  bool operator> (const str &s) const { return (b->s > s); }
  bool operator<= (const str &s) const { return (b->s <= s); }
  bool operator>= (const str &s) const { return (b->s >= s); }
  bool operator== (const char *p) const { return (b->s == p); }
  bool operator!= (const char *p) const { return (b->s != p); }
  operator hash_t () const { return (hash_t )(b->s); }
  const str &compress (int l = -1) const { return b->to_zstr (l); }
  inline bool compressed () const { return b->compressed (); }
  inline uLong crc32 (uLong in) const { return b->crc32 (in); }

  // static const char * pointers (for compiled-in strings) --
  // to make caching hints.
  inline void set_scc_p (intptr_t i) { _scc_p = i ; }
  inline intptr_t get_scc_p () const { return _scc_p; }

private:
  friend class ztab_cache_t;
  ihash_entry<zstr> hlnk;
  tailq_entry<zstr> qlnk;
  ptr<zstrobj> b;
  intptr_t _scc_p;
};


class compressible_t {
public:

  struct opts_t {
    opts_t (gzip_mode_t m = GZIP_NONE, bool c = false, int l= -1);
    gzip_mode_t mode;
    bool chunked;
    int lev;
  };

  virtual ~compressible_t () {}
  virtual const strbuf &to_strbuf (opts_t o) = 0;
  virtual size_t inflated_len () const = 0;
  virtual void clear () = 0;
  virtual int to_strbuf (strbuf *out, opts_t o) = 0;

  // For backwards-compatibility
  int to_strbuf (strbuf *out, bool gz)
  { return to_strbuf (out, opts_t (gz ? GZIP_SMART : GZIP_NONE, gz)); }
  const strbuf &to_strbuf (bool gz)
  { return to_strbuf (opts_t (gz ? GZIP_SMART : GZIP_NONE, gz)); }

};

class zbuf : public compressible_t {
public:
  zbuf () : endbuf (ZSTR_ENDBUF_SIZE), minstrsize (ok_gzip_smallstr) {} 
  ~zbuf () {}
  inline zbuf &cat (const zbuf &zb2);
  inline zbuf &cat (const str &s, bool cp = false);
  inline zbuf &cat (const zstr &z, bool cp = false);
  inline zbuf &cat (const char *p, size_t s, bool cp = false);
  inline zbuf &cat (const char *p, bool cp = false) 
  { return cat (p, strlen (p)); }
  inline zbuf &cat (const strbuf &b) { f << b; return (*this); }
  template<typename T> inline zbuf &cat (T i) { f << i; return (*this); }

  // implement the compressible contract
  inline const strbuf &to_strbuf (compressible_t::opts_t o) 
  { to_strbuf (&out, o); return out;} 

  int to_strbuf (strbuf *b, compressible_t::opts_t o);
  int to_strbuf (strbuf *b, bool gz)
  { return compressible_t::to_strbuf (b, gz); }
  const strbuf &to_strbuf (bool gz)
  { return compressible_t::to_strbuf (gz); }

  size_t inflated_len () const;

  int naive_compress (strbuf *b, int lev) ;

  void clear ();
  const strbuf &output () { output (&out); return out; }
  const strbuf &compress (compressible_t::opts_t o) 
  { compress (&out, o); return out; }


  void output (strbuf *b, bool doclear = true);
  void compress (strbuf *b, compressible_t::opts_t o);

  int output (int fd, compressible_t::opts_t o = opts_t ());
  void to_zstr_vec (vec<zstr> *zs);
private:

  inline void push_zstr (const zstr &z, bool clr = true);
  inline void push_str (const str &s, bool clr = true);
  inline void push_str2zstr (const str &s, bool lkp = true, bool clr = true);
  inline void strbuf2zstr ();

  inline void copy_small_str (const str &z) { copy_small_str (z.cstr(), z.len ()); }
  inline void copy_small_str (const char *p, size_t len);
  inline void strbuf_clear () { f.tosuio ()->clear (); }
  inline void strbuf_add (const str &s, bool cp);

  vec<zstr> zs;
  vec<str> hold_strings; // eliminate programmer bugs unless dangerous is set!
  strbuf f;
  strbuf out;
  mstr endbuf;
  size_t minstrsize;
};

template<class C> inline zbuf &operator<< (zbuf &z, C c) { return z.cat (c); }

typedef ptr<zstrobj> zobjp;

class ztab_t {
public:
  virtual ~ztab_t () {}
  virtual zstr alloc (const str &s, bool l = true) { return zstr (s); }
  virtual zstr alloc (const char *p, size_t len, bool lkp = true) 
  { return zstr (p, len); }
  virtual zstr *lookup (const str &s) { return NULL; }
  virtual zstr *lookup (const char *p, size_t len) { return NULL; }
};

template<> struct equals<zstr> {
  equals () {}
  bool operator() (const zstr &z1, const zstr &z2) const { return z1 == z2; }
};

template<> struct hashfn<zstr> {
  hashfn () {}
  hash_t operator() (const zstr &z) const { return z; }
};

template<> struct keyfn<zstr,str> {
  keyfn () {}
  const str &operator() (zstr *z) const { return z->to_str (); }
};

class ztab_cache_t : public ztab_t {
public:
  ztab_cache_t (ssize_t n = -1, ssize_t x = -1, ssize_t t = -1)
    : minz (n == -1 ? ok_gzip_cache_minstr : size_t (n)),
      maxz (n == -1 ? ok_gzip_cache_maxstr : size_t (n)),
      maxtot (n == -1 ? ok_gzip_cache_storelimit : size_t (n)),
      tot (0) { }
  ~ztab_cache_t () {}
  zstr alloc (const str &s, bool l = true);
  zstr alloc (const char *p, size_t len, bool l = true);
  zstr *lookup (const str &s);
  zstr *lookup (const char *p, size_t l);

private:
  void remove (zstr *s = NULL);
  void insert (zstr *s);
  void make_room (size_t l);

  size_t minz, maxz, maxtot, tot;
  fhash<str, zstr, &zstr::hlnk> tab;
  tailq<zstr, &zstr::qlnk> q;
  qhash<intptr_t, zstr *> cptab;
};

extern ztab_t *ztab;

inline ztab_t *
get_ztab()
{
  if (!ztab)
    ztab = New ztab_t ();
  return ztab;
}


void
zbuf::push_str (const str &s, bool clr)
{
  if (clr) strbuf2zstr ();
  push_str2zstr (s, false);
}

void
zbuf::push_zstr (const zstr &z, bool clr)
{
  if (clr) strbuf2zstr ();
  zs.push_back (z);
}

void 
zbuf::push_str2zstr (const str &s, bool lkp, bool clr) 
{ 
  push_zstr (get_ztab ()->alloc (s, lkp), clr);
}

zbuf &
zbuf::cat (const zbuf &zb2)
{
  size_t zb2l = zb2.zs.size ();
  if (zb2l) {
    strbuf2zstr ();
    for (size_t i = 0; i < zb2l; i++) {
      zs.push_back (zb2.zs[i]);
    }
  }

  // flatten zb2's strbuf, and copy its content to ours; zb2 might
  // contain some strings that will go out of scope right after this call.
  strbuf_add (zb2.f, true);
  return (*this);
}

zbuf &
zbuf::cat (const str &s, bool cp)
{
  if (s.len () <= minstrsize) {
    strbuf_add (s, cp);
  } else {
    zstr *z = get_ztab ()->lookup (s);
    if (z) {
      push_zstr (*z);
    } else {
      push_str2zstr (s, false);
    }
  }
  return (*this);
}

zbuf &
zbuf::cat (const zstr &z, bool cp)
{
  if (z.len () <= minstrsize) { 
    strbuf_add (z, cp);
  } else { 
    zstr *zp = get_ztab ()->lookup (z);
    if (zp) {
      push_zstr (*zp);
    } else {
      push_zstr (z);
    }
  }
  return (*this);
}

void
zbuf::copy_small_str (const char *p, size_t len)
{
  f.tosuio ()->copy (p, len);
}

void
zbuf::strbuf_add (const str &s, bool cp)
{
  if (cp) copy_small_str (s);
  else {
    f << s;
    if (!ok_dangerous_zbufs)
      hold_strings.push_back (s);
  }
}

zbuf &
zbuf::cat (const char *c, size_t l, bool cp)
{
  if (l <= minstrsize) { 
    copy_small_str (c, l);
  } else {
    zstr *zp = get_ztab ()->lookup (c, l);
    if (zp) {
      zs.push_back (*zp);
    } else {
      zs.push_back (get_ztab ()->alloc (c, l,false));
    }
  }
  return (*this);
}

void 
zbuf::strbuf2zstr () 
{ 
  if (f.tosuio ()->resid ()) {
    push_str2zstr (f, true, false); 
    strbuf_clear (); 
  }
}

void zinit (bool cache = true, int lev = -1);

str zdecompress (const str &in);
str zcompress (const str &in, int lev);

#endif /* _LIBAZ_ZSTR */
