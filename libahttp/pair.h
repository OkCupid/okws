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

#ifndef _LIBAHTTP_PAIR_H
#define _LIBAHTTP_PAIR_H

#include "parseopt.h"
#include "clist.h"

// max buf of 256K
#define XSS_MAX_BUF 0x40000  
str xss_filter (const str &in);
str xss_filter (const char *in, u_int l);

struct encode_t {
  encode_t (strbuf *o, char *s = NULL, size_t l = 0)
    : out (o), scratch (s), len (l), first (true) {}
  strbuf *out;
  char *scratch;
  size_t len;
  bool first;
};

class cgi_mpfd_pair_t;
typedef enum { IV_ST_NONE = 0, IV_ST_FAIL = 1, IV_ST_OK = 2 } iv_state_t;
struct pair_t {
  pair_t (const str &k) : key (k), is (IV_ST_NONE), encflag (true) {}

  pair_t (const str &k, const str &val, bool e = true) 
    : key (k), is (IV_ST_NONE), encflag (e) 
  { if (val) vals.push_back (val); }

  pair_t (const str &k, int64_t i) 
    : key (k), is (IV_ST_OK), encflag (true) { ivals.push_back (i); }
  virtual ~pair_t () {}

  bool to_int (int64_t *v) const;
  vec<int64_t> *to_int () const;
  void dump1 () const;
  virtual void encode (encode_t *e, const str &s) const {} 
  inline bool hasdata () const { return vals.size () > 0; }
  void addval (const str &v) { vals.push_back (v); }

  // terrible kludge; should come up with someting better
  virtual cgi_mpfd_pair_t *to_cgi_mpfd_pair () { return NULL; }

  str key;
  vec<str> vals;
  
  mutable iv_state_t is;
  mutable vec<int64_t> ivals;
  ihash_entry<pair_t> hlink;
  clist_entry<pair_t> lnk;
  bool encflag;
};

static void pair_dump1 (const pair_t &p) { p.dump1 (); }
static void pair_encode (encode_t *e, str sep, const pair_t &p)
{ p.encode (e, sep); }
static void pair_trav (callback<void, const pair_t &>::ref cb, pair_t *p)
{ (*cb) (*p); }

template<class C = pair_t>
class pairtab_t {
public:
  pairtab_t (bool f = false) : empty (""), filter (f) {}
  virtual ~pairtab_t () { tab.deleteall (); }
  inline str lookup (const str &key) const;
  inline bool lookup (const str &key, str *r) const;
  inline bool lookup (const str &key, vec<str> *v) const;
  inline bool blookup (const str &key) const;
  inline vec<int64_t> *ivlookup (const str &key) const;
  template<typename T> inline bool lookup (const str &key, T *v) const;
  pairtab_t<C> &insert (const str &key, const str &val = NULL, 
			bool append = true, bool encode = true);
  void insert (pair_t *p);
  template<typename T> pairtab_t<C> 
  &insert (const str &key, T v, bool append = true, bool encode = true);
  void dump1 () const { tab.traverse (wrap (&pair_dump1)); }
  virtual str get_sep () const { return "&"; }
  virtual void encode (encode_t *e) const 
  { tab.traverse (wrap (&pair_encode, e, get_sep ())); }
  inline str safe_lookup (const str &key) const;
  inline str operator[] (const str &k) const { return safe_lookup (k); }
  inline bool exists (const str &k) const 
  { return safe_lookup (k).len () > 0; }
  inline bool remove (const str &k);
  inline void traverse (callback<void, const pair_t &>::ref cb)
  { lst.traverse (wrap (pair_trav, cb)); }

  void reset ()
  {
    tab.deleteall ();
    lst.clear ();
  }
    
protected:
  virtual pair_t *alloc_pair (const str &k, const str &v, bool e = true) const
  { return New C (k, v, e); }

  str empty;
  ihash<str, pair_t, &pair_t::key, &pair_t::hlink> tab;
  clist_t<pair_t, &pair_t::lnk> lst;
  bool filter;
};


template<class C> str
pairtab_t<C>::safe_lookup (const str &key) const
{
  const str &s = lookup (key);
  if (s && filter && ok_filter_cgi == XSSFILT_SOME) return xss_filter (s);
  else if (s) return s;
  else return empty;
}

template<class C> bool
pairtab_t<C>::lookup (const str &key, str *r) const
{
  assert (key && r);
  pair_t *p = tab[key];
  if (!p) return false;
  if (p->vals.size () >= 1) *r =  p->vals[0];
  else *r = NULL;
  return true;
}

template<class C> bool
pairtab_t<C>::blookup (const str &key) const
{
  int tmp;
  return (lookup (key, &tmp) && tmp != 0);
}

template<class C> vec<int64_t> *
pairtab_t<C>::ivlookup (const str &key) const
{
  pair_t *p = tab[key];
  if (!p) return NULL;
  return p->to_int ();
}

template<class C> str
pairtab_t<C>::lookup (const str &key) const
{
  assert (key);
  pair_t *p = tab[key];
  if (p && p->vals.size () >= 1)
    return p->vals[0];
  return NULL;
}

template<class C> bool
pairtab_t<C>::lookup (const str &key, vec<str> *v) const
{
  assert (key && v);
  pair_t *p = tab[key];
  if (!p)
    return false;
  *v = p->vals;
  return true;
}

template<class C> template<typename T> bool
pairtab_t<C>::lookup (const str &key, T *v) const
{
  assert (key && v);
  int64_t i;
  pair_t *p = tab[key];
  if (!p || !p->to_int (&i))
    return false;
  *v = i;
  return true;
}

template<class C> template<typename T> pairtab_t<C> &
pairtab_t<C>::insert (const str &k, T v, bool append, bool encode)
{
  strbuf b;
  b << v;
  return insert (k, str (b), append, encode);
}

template<class C> bool
pairtab_t<C>::remove (const str &k)
{
  pair_t *p = tab[k];
  if (p) {
    tab.remove (p);
    lst.remove (p);
    delete p;
    return true;
  } else {
    return false;
  }
}

template<class C> pairtab_t<C> &
pairtab_t<C>::insert (const str &key, const str &val, bool append, bool encode)
{
  assert (key);
  if (!key.len ())
    return *this;
  pair_t *p = tab[key];
  if (p && val) {
    if (append) {
      p->vals.push_back (val);
    } else {
      p->vals.clear ();
      p->vals.push_back (val);
    }
  } else if (!p) {
    insert (alloc_pair (key, val, encode));
  }
  return *this;
}

template<class C> void
pairtab_t<C>::insert (pair_t *p)
{
  tab.insert (p);
  lst.insert_tail (p);
}

#endif /* _LIBAHTTP_PAIR_H */
