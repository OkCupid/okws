
// -*-c++-*-
/* $Id$ */

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
  zstr () {}
  zstr (const str &s) : b (New refcounted<zstrobj> (s)) {}
  zstr (const str &s, const str &z, int l) : 
    b (New refcounted<zstrobj> (s, z, l)) { }
  zstr (const zstr &z) : b (z.b) {}
  zstr (const char *p) : b (p ? New refcounted<zstrobj> (p) : NULL) {}
  zstr (const char *p, size_t l) : b (New refcounted<zstrobj> (str (p, l))) {}
  zstr (const strbuf &bb) : b (New refcounted<zstrobj> (bb)) {}

  zstr &operator= (const zstr &z) { b = z.b; return *this; }
  zstr &operator= (const char *p) 
  { 
    b = p ? New refcounted<zstrobj> (p) : NULL; 
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
private:
  friend class ztab_cache_t;
  ihash_entry<zstr> hlnk;
  tailq_entry<zstr> qlnk;
  ptr<zstrobj> b;
};

class zbuf {
public:
  zbuf () : endbuf (ZSTR_ENDBUF_SIZE), minstrsize (ok_gzip_smallstr) {} 
  inline zbuf &cat (const zbuf &zb2);
  inline zbuf &cat (const str &s, bool cp = false);
  inline zbuf &cat (const zstr &z, bool cp = false);
  inline zbuf &cat (const char *p, size_t s, bool cp = false);
  inline zbuf &cat (const char *p, bool cp = false) 
  { return cat (p, strlen (p)); }
  inline zbuf &cat (const strbuf &b) { f << b; return (*this); }
  template<typename T> inline zbuf &cat (T i) { f << i; return (*this); }

  void clear ();
  const strbuf &output () { output (&out); return out; }
  const strbuf &compress (int l = -1) { compress (&out); return out; }
  inline const strbuf &to_strbuf (bool gz) {to_strbuf (&out, gz); return out;} 

  size_t inflated_len () const;

  void output (strbuf *b);
  void compress (strbuf *b, int l = -1);
  inline void to_strbuf (strbuf *b, bool gz) 
  { if (gz) compress (b); else output (b); }

  int output (int fd, bool gz = false);
  void to_zstr_vec (vec<zstr> *zs);
private:

  inline void push_zstr (const zstr &z, bool clr = true);
  inline void push_str (const str &s, bool clr = true);
  inline void push_str2zstr (const str &s, bool lkp = true, bool clr = true);
  inline void strbuf2zstr ();

  inline void copy_small_str (const str &z) { copy_small_str (z, z.len ()); }
  inline void copy_small_str (const char *p, size_t len);
  inline void strbuf_clear () { f.tosuio ()->clear (); }
  inline void strbuf_add (const str &s, bool cp);

  vec<zstr> zs;
  strbuf f;
  strbuf out;
  mstr endbuf;
  u_int minstrsize;
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
  qhash<u_int, zstr *> cptab;
};

extern ztab_t *ztab;

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
  push_zstr (ztab->alloc (s, lkp), clr);
}

zbuf &
zbuf::cat (const zbuf &zb2)
{
  u_int zb2l = zb2.zs.size ();
  if (zb2l) {
    strbuf2zstr ();
    for (u_int i = 0; i < zb2l; i++) {
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
  zstr *z = ztab->lookup (s);
  if (z) {
    push_zstr (*z);
  } else if (s.len () <= minstrsize) {
    strbuf_add (s, cp);
  } else {
    push_str2zstr (s, false);
  }
  return (*this);
}

zbuf &
zbuf::cat (const zstr &z, bool cp)
{
  zstr *zp = ztab->lookup (z);
  if (zp) {
    push_zstr (*zp);
  } else if (z.compressed () || z.len () > minstrsize) {
    push_zstr (z);
  } else {
    strbuf_add (z, cp);
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
  else f << s;
}

zbuf &
zbuf::cat (const char *c, size_t l, bool cp)
{
  zstr *zp = ztab->lookup (c, l);
  if (zp) {
    zs.push_back (*zp);
  } else if (l <= minstrsize) {
    copy_small_str (c, l);
  } else {
    zs.push_back (ztab->alloc (c, l,false));
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

#endif /* _LIBAZ_ZSTR */
