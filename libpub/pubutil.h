// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 2003 Maxwell Krohn (max@okcupid.com)
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

#ifndef _LIBPUB_PUBUTIL_H 
#define _LIBPUB_PUBUTIL_H 1

#include "amisc.h"
#include "str.h"
#include "async.h"
#include "ihash.h"
#include "xpub.h"
#include "qhash.h"
#include "litetime.h"

str c_escape (const str &s, bool addq = true);
bool mystrcmp (const char *a, const char *b);
bool str_split (vec<str> *r, const str &s, bool quoted, int sz = 50);
str suffix_sub (const char *s, const char *sfx1, const char *sfx2);
int myopen (const char *arg, u_int mode = 0444);
str dir_standardize (const str &s);
void got_dir (str *out, vec<str> s, str loc, bool *errp);
str re_fslash (const char *cp);
str can_exec (const str &p);
bool dir_security_check (const str &p);
str apply_container_dir (const str &d, const str &e);
void got_clock_mode (sfs_clock_t *out, vec<str> s, str lock, bool *errp);
bool is_safe (const str &s);

struct phash_t {
  phash_t () {}
  phash_t (const char *s) { memcpy (val, s, PUBHASHSIZE); }
  phash_t (const xpubhash_t &h) { memcpy (val, h.base (), PUBHASHSIZE); }
  static ptr<phash_t> alloc (const xpubhash_t &x) 
  { return New refcounted<phash_t> (x); }
  char val[PUBHASHSIZE];
  str to_str () const { return armor64 (val, PUBHASHSIZE); }
  hash_t hash_hash () const;
  bool operator== (const phash_t &ph) const;
  bool operator== (const xpubhash_t &ph) const;
  void to_xdr (xpubhash_t *ph) const;
};

typedef ptr<phash_t> phashp_t;
typedef callback<void, phashp_t>::ref phash_cb;

typedef str pfnm_t;

template<> struct hashfn<phashp_t> {
  hashfn () {}
  hash_t operator() (const phash_t *s) const { return s->hash_hash (); } 
};

template<> struct equals<phashp_t> {
  equals () {}
  bool operator() (const phashp_t &s1, const phashp_t &s2) const
  { return (*s1 == *s2); }
};

struct pbinding_t { // binds filenames to content-hashes
  pbinding_t () {}
  pbinding_t (const pfnm_t &f, const phashp_t &h, bool tl = false) 
    : fn (f), hsh (h), toplev (tl) {}
  pbinding_t (const xpub_pbinding_t &x);

  void to_xdr (xpub_pbinding_t *x) const;

  inline phashp_t hash () const { return hsh; }
  inline pfnm_t filename () const { return fn; }

  bool operator== (const phash_t &ph) { return *hsh == ph; }
  bool operator!= (const phash_t &ph) { return !(*hsh == ph); }

  const pfnm_t fn;
  const phashp_t hsh;
  const bool toplev;
  mutable ihash_entry<pbinding_t> hlink;
};

typedef ihash<const pfnm_t, pbinding_t, &pbinding_t::fn,
	      &pbinding_t::hlink> _bindmap_t;

typedef callback<void, pbinding_t *>::ref bindcb;

struct bindtab_t : public _bindmap_t {
public:
  bindtab_t (phash_cb::ptr cb = NULL) : _bindmap_t () , delcb (cb) {}
  void bind (const pbinding_t *p);
  void unbind (const pbinding_t *p);
  void clear2 () { cnt.clear (); clear (); }
private:
  phash_cb::ptr delcb;
  inline void inc (phashp_t h);
  inline void dec (phashp_t h);
  qhash<phashp_t, u_int> cnt;
};


bool file2hash (const str &fn, phash_t *ph);
phashp_t file2hash (const str &fn);
int uname2uid (const str &un);
str uid2uname (int i);
int gname2gid (const str &gn);
bool isdir (const str &d);
int my_log_10 (int64_t in);

//
// Case-Insensitive String Comparison
//
bool cicmp (const str &s1, const char *c2);
bool cicmp (const str &s2, const char *c2, u_int len);
inline bool cicmp (const str &s1, const str &s2)
{ return cicmp (s1, s2.cstr (), s2.len ()); }

// uid / gid stuff

struct ok_idpair_t {
  ok_idpair_t (const str &n) : name (n), id (-1) {}
  ok_idpair_t (int i) : id (i) {}
  virtual ~ok_idpair_t () {}
  virtual bool resolve () const = 0;

  operator bool () const
  { 
    // for anyonomous users/groups
    if (!name && id > 0) return true;

    // for standard users/groups that appear in /etc/(passwd|group)
    bool ret = name && resolve (); 
    if (name && !ret)
      warn << "Could not find " << typ () << " \"" << name << "\"\n";
    return ret;
  }
  virtual str typ () const = 0;

  str getname () const
  {
    if (name) return name;
    else return (strbuf () << id);
  }
  
  int getid () const 
  {
    if (id >= 0) return id;
    else if (!*this) return -1;
    else return id;
  }
protected:
  str name;
  mutable int id;
};

struct ok_usr_t : public ok_idpair_t {
  ok_usr_t (const str &n) : ok_idpair_t (n) {}
  ok_usr_t (int i) : ok_idpair_t (i) {}
  bool resolve () const { return ((id = uname2uid (name)) >= 0); }
  str typ () const { return "user"; }
};

struct ok_grp_t : public ok_idpair_t {
  ok_grp_t (const str &n) : ok_idpair_t (n) {}
  ok_grp_t (int i) : ok_idpair_t (i) {}
  bool resolve () const { return ((id = gname2gid (name)) >= 0); }
  str typ () const { return "group"; }
};

#endif /* _LIBPUB_PUBUTIL_H */
