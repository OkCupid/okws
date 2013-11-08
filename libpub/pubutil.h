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

#include "okws_sfs.h"
#include "ihash.h"
#include "pub3prot.h"
#include "qhash.h"
#include <dirent.h>
#include "okdbg-int.h"
#include "okws_sfs.h"

str c_escape (const str &s, bool addq = true);
bool mystrcmp (const char *a, const char *b);
bool str_split (vec<str> *r, const str &s, bool quoted, int sz = 50);
str suffix_sub (const char *s, const char *sfx1, const char *sfx2);
int myopen (const char *arg, u_int mode = 0444);
str dir_standardize (const str &s);
void got_dir (str *out, vec<str> s, str loc, bool *errp);
str re_fslash (const char *cp);
str can_exec (const str &p);
bool can_read (const str &f);
bool dir_security_check (const str &p);
str apply_container_dir (const str &d, const str &e);
void got_clock_mode (sfs_clock_t *out, vec<str> s, str lock, bool *errp);
bool is_safe (const str &s);
int nfs_safe_stat (const char *f, struct stat *sb);
inline time_t okwstime () { return sfs_get_timenow(); }
inline double okwstsnow ()
{
    timespec ts = sfs_get_tsnow();
    double d = 1000000000;
    return ts.tv_sec + ts.tv_nsec / d;
}
inline time_t okwsmstime (bool refresh = false)
{
    timespec ts        = sfs_get_tsnow(refresh);
    double   ms_to_us  = 1000000;
    int      sec_to_ms = 1000;
    time_t   t         = time_t(ts.tv_sec * sec_to_ms + ts.tv_nsec / ms_to_us);
    return t;
}
str errcode2str (const xpub_status_t &e);
bool to_hostname_and_port (const str &in, str *out, int *port);
str html_wss (str in);
str dir_merge (str d, str f);
bool has_null_byte (const str &s);
str trunc_after_null_byte (str s);
size_t utf8_len (str s);

// Given in, put the directory component in d, and the basename component
// in b.  Return TRUE if absolute and FALSE if otherwise. d set to NULL
// if no directory.  b set to NULL if no filename (i.e., ends in '/').
// Dirname is normalized (i.e., double '/'s are mapped to one '/').
bool basename_dirname (const str &in, str *d, str *b);

class argv_t {
public:
  argv_t ();
  argv_t (const vec<str> &v, const char *const *seed = NULL);
  void init (const vec<str> &v, const char *const *seed = NULL);
  ~argv_t ();
  size_t size () const { return _v.size () - 1; }
  void copy (const argv_t &in);

  // BOOOOO; but getopt and everyone else seem to use char *const *
  // and not const char * const * as i suspect they should.
  operator char* const* () const 
  { return const_cast<char *const *> (_v.base ()); }

protected:
  vec<const char *> _v;
private:
  vec<const char *> _free_me;
};

class env_argv_t : public argv_t {
public:
  env_argv_t (const vec<str> &v, const char *const *seed = NULL);
  env_argv_t () : argv_t () {}
  void prune ();
};

typedef str pfnm_t;

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
str my_tolower (const str &in);

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

// debug function
void ls (const str &s);

// strip comments after getline returns
void strip_comments (vec<str> *in);

str trunc_at_first_null (const str &s);
void fix_trailing_newlines (str *s);

const char * getenvval (const char *s);

//-----------------------------------------------------------------------

template<size_t n> bool cstr2opaque (const char *cc, size_t l, rpc_bytes<n> &o)
{
  bool ret = false;
  if (l) {
    o.setsize (l);
    memcpy (o.base (), cc, l);
    ret = true;
  }
  return ret;
}

//-----------------------------------------------------------------------

template<size_t n> bool str2opaque (const str &s, rpc_bytes<n> &o)
{
  bool ret = false;
  if (s) {
    ret = true;
    size_t l = s.len ();
    o.setsize (l);
    memcpy (o.base (), s.cstr (), l);
  }
  return ret;
}

//-----------------------------------------------------------------------

template<size_t n> str opaque2str (const rpc_bytes<n> &o)
{
  size_t l = o.size ();
  mstr m (l + 1);
  memcpy (m.cstr (), o.base (), l);
  m[l] = '\0';
  m.setlen (l);
  return m;
}

//-----------------------------------------------------------------------

#endif /* _LIBPUB_PUBUTIL_H */
