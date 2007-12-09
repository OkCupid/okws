
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

#include "pubutil.h"
#include "sha1.h"
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>
#include "rxx.h"

bool
mystrcmp (const char *a, const char *b)
{
  while (*a && *b) {
    if (*a != *b) return false;
    a++;
    b++;
  }
  if (*a || *b)
    return false;
  return true;
}

bool
cicmp (const str &s1, const char *c2)
{
  const char *c1 = s1.cstr ();
  while (*c1 && *c2) {
    if (tolower (*c1) != tolower (*c2))
      return false;
    c1++;
    c2++;
  }
  if (*c1 || *c2)
    return false;
  return true;
}

bool 
cicmp (const str &s1, const char *c2, u_int l)
{
  if (s1.len () != l)
    return false;
  const char *c1 = s1.cstr ();
  for (u_int i = 0; i < l; i++)
    if (tolower (c1[i]) != tolower (c2[i]))
      return false;
  return true;
}

int
myopen (const char *arg, u_int mode)
{
  struct stat sb;
  if (stat (arg, &sb) == 0)
    if (!S_ISREG (sb.st_mode)) 
      fatal << arg << ": file exists but is not a regular file\n";
    else if (unlink (arg) < 0 || stat (arg, &sb) == 0)
      fatal << arg << ": could not remove file\n";
 
  int fd = open (arg, O_CREAT|O_WRONLY, mode);
  if (!fd)
    fatal << arg << ": could not open file for writing\n";
  return fd;
}

str
suffix_sub (const char *s, const char *sfx1, const char *sfx2)
{
  if (!s)
    return NULL;
  int len0, len1;
  len0 = strlen (s);
  len1 = strlen (sfx1);
  if (len0 < len1 || !mystrcmp (s + len0 - len1, sfx1))
    return NULL;
  return (strbuf (str (s, len0 - len1)) << sfx2);
}

str
c_escape (const str &s, bool addq)
{
  if (!s) return s;

  const char *p1 = s.cstr ();
  const char *p2 = NULL;
  char *buf = New char[2 * s.len () + 3];
  char *dp = buf;
  size_t span;

  if (addq)
    *dp++ = '"';

  char c;
  while ((p2 = strpbrk (p1, "\\\"\n"))) {
    span = p2 - p1;
    strncpy (dp, p1, span);
    dp += span;
    *dp++ = '\\';
    c = (*p2 == '\n') ? 'n' : *p2;
    *dp++ = c;
    p2++;
    p1 = p2;
  }
  int len = strlen (p1);
  memcpy (dp, p1, len);
  dp += len;
  if (addq) 
    *dp++ = '"';
  *dp = 0;
  str r (buf);
  
  delete [] buf;
  return r;
}

bool
str_split (vec<str> *r, const str &s, bool quoted, int sz)
{
  int len = s.len ();
  if (!len || sz <= 0)
    return false;
  const char *bp = s.cstr ();
  while (len > 0) {
    int i = min (len, sz);
    str s (bp, i);
    r->push_back (quoted ? c_escape (s, true) : s);
    len -= i;
    bp += i;
  }
  return true;
}

bool
phash_t::operator== (const phash_t &p2) const
{
  return (!memcmp ((void *)val, (void *)p2.val, PUBHASHSIZE));
}

bool 
phash_t::operator== (const xpubhash_t &ph) const
{
  return (!memcmp ((void *)val, (void *)ph.base (), PUBHASHSIZE));
}

bool phash_t::operator!= (const xpubhash_t &ph) const
{
  return !(*this == ph);
}

bool phash_t::operator!= (const phash_t &p2) const
{
  return !(*this == p2);
}

hash_t
phash_t::hash_hash () const
{
  u_int *p = (u_int *)val;
  const char *end_c = val + PUBHASHSIZE;
  u_int *end_i = (u_int *)end_c;
  u_int r = 0;
  while (p < end_i)
    r = r ^ *p++;
  return r;
}

#define BUFSIZE 4096

bool
file2hash (const str &fn, phash_t *h, struct stat *sbp)
{
  char buf[BUFSIZE];
  struct stat sb;
  if (!sbp)
    sbp = &sb;
  if (access (fn.cstr (), F_OK | R_OK) != 0)
    return false;
  if (stat (fn.cstr (), sbp) != 0 || !S_ISREG (sbp->st_mode))
    return false;
  int fd = open (fn.cstr (), O_RDONLY);
  if (fd < 0)
    return false;
  size_t n;
  sha1ctx sc;
  while ((n = read (fd, buf, BUFSIZE))) {
    sc.update (buf, n);
  }
  sc.final (h);
  close (fd);
  return true;
}

ptr<phash_t>
file2hash (const str &fn, struct stat *sbp)
{
  ptr<phash_t> p = New refcounted<phash_t> ();
  if (!file2hash (fn, p, sbp))
    return NULL;
  return p;
}

void
pbinding_t::to_xdr (xpub_pbinding_t *x) const
{
  x->fn = fn;
  hsh->to_xdr (&x->hash);
}

void
phash_t::to_xdr (xpubhash_t *x) const
{
  memcpy (x->base (), val, PUBHASHSIZE);
}

pbinding_t::pbinding_t (const xpub_pbinding_t &x) 
  : fn (x.fn), hsh (New refcounted<phash_t> (x.hash.base ())), 
		      toplev (false) {}

void
bindtab_t::unbind (const pbinding_t *p)
{
  dec (p->hash ());
  remove (const_cast<pbinding_t *> (p));
}

void
bindtab_t::bind (const pbinding_t *p)
{
  pbinding_t *p2;
  if ((p2 = (*this)[p->filename ()])) {
    dec (p2->hash ());
    remove (p2);
    delete p2;
  }
  inc (p->hash ());
  insert (const_cast<pbinding_t *> (p));
}

void
bindtab_t::inc (phashp_t h)
{
  u_int *i = cnt[h];
  if (i) (*i)++;
  else cnt.insert (h, 1);
}

void
bindtab_t::dec (phashp_t h)
{
  u_int *i = cnt[h];
  assert (i);
  if (!--(*i)) {
    if (delcb)
      (*delcb) (h);
    cnt.remove (h);
  }
}

int
uname2uid (const str &n)
{
  struct passwd *pw;
  int ret = -1;
  if ((pw = getpwnam (n))) {
    ret = pw->pw_uid;
  }
  endpwent ();
  return ret;
}

str
uid2uname (int i)
{
  struct passwd *pw;
  str ret;
  if ((pw = getpwuid (i))) 
    ret = pw->pw_name;
  endpwent ();
  return ret;
}

int
gname2gid (const str &g)
{
  struct group *gr;
  int ret = -1;
  if ((gr = getgrnam (g))) {
    ret = gr->gr_gid;
  }
  endgrent ();
  return ret;
}

bool
isdir (const str &d)
{
  struct stat sb;
  return (!stat (d, &sb) && (sb.st_mode & S_IFDIR) && !access (d, X_OK|R_OK));
}

int my_log_10 (int64_t in)
{
  int i = 0;
  while ((in = in / 10)) i++;
  return i;
}

str 
dir_standardize (const str &s)
{
  const char *bp = s.cstr ();
  const char *ep = bp + s.len () - 1;
  for ( ; bp <= ep && *ep == '/'; ep--) ;
  ep++;
  return str (bp, ep - bp);
}

void
got_dir (str *out, vec<str> s, str loc, bool *errp)
{
  strip_comments (&s);
  if (s.size () != 2) {
    warn << loc << ": usage: " << s[0] << " <path>\n";
    *errp = true;
    return;
  }
  if (!is_safe (s[1])) {
    warn << loc << ": directory (" << s[1] 
	 << ") contains unsafe substrings\n";
    *errp = true;
    return;
  }
  *out = dir_standardize (s[1]);
}

sfs_clock_t
char2sfsclock (char c)
{
  switch (c) {
  case 'm':
    return SFS_CLOCK_MMAP;
  case 't':
    return SFS_CLOCK_TIMER;
    break;
  case 'd':
    return SFS_CLOCK_GETTIME;
  default:
    warn << "unknow SFS clock type: " << c << "\n";
    return SFS_CLOCK_GETTIME;
  }
}

void
got_clock_mode (sfs_clock_t *out, vec<str> s, str loc, bool *errp)
{
  if (s.size () != 2) {
    warn << loc << ": usage: " << s[0] << " (m|t|d)\n";
    *errp = true;
    return;
  }
  *out = char2sfsclock (s[1][0]);
}


str 
re_fslash (const char *cp)
{
  while (*cp == '/' && *cp) cp++;
  return strbuf ("/") << cp;
}

str
can_exec (const str &p)
{
  if (access (p.cstr (), R_OK)) 
    return ("cannot read executable");
  else if (access (p.cstr (), X_OK)) 
    return ("cannot execute");
  return NULL;
}

bool
can_read (const str &f)
{
  struct stat sb;
  return  (f && stat(f.cstr (), &sb) == 0 && S_ISREG (sb.st_mode)
	   && access (f.cstr(), R_OK) == 0);
}

str
apply_container_dir (const str &d, const str &f)
{
  if (f[0] == '/' || !d)
    return re_fslash (f.cstr ());
  else 
    return (strbuf (d) << "/" << f);
}

static rxx safe_rxx ("(/\\.|\\./)");

bool
is_safe (const str &d)
{
  if (!d || d[0] == '.' || d[d.len () - 1] == '.' || safe_rxx.search (d))
    return false;
  return true;;
}

argv_t::argv_t ()
{
  _v.push_back (NULL);
}

argv_t::argv_t (const vec<str> &in, const char *const *seed)
{
  init (in, seed);
}

void
argv_t::init (const vec<str> &in, const char *const *seed)
{
  if (_v.size ())
    _v.clear ();

  if (seed)
    for (const char *const *s = seed; *s; s++) 
      _v.push_back (*s);

  for (u_int i = 0; i < in.size (); i++) {
    size_t len = in[i].len () + 1;
    char *n = New char[len];
    memcpy (n, in[i].cstr (), len);
    _free_me.push_back (n);
    _v.push_back (n);
  }
  _v.push_back (NULL);
}

argv_t::~argv_t ()
{
  const char *tmp;
  while (_free_me.size () && (tmp = _free_me.pop_front ()))
    delete [] tmp;
}

void
ls (const str &d)
{

  DIR *dir = opendir (d);
  if (!d) {
    warn << "ls (" << d << "): failed\n";
    return;
  }
  warn << "ls (" << d << "):\n";

  struct dirent *ent;
  while ((ent = readdir (dir))) 
    warn << ent->d_name << "\n";
  closedir (dir);
}

static void
okdbg_dump_iterator (vec<str> *b, const pbinding_t &x)
{
  x.okdbg_dump_vec (b);
}


void
bindtab_t::okdbg_dump_vec (vec<str> *s) const 
{
  strbuf b;
  b.fmt ("===================== Begin Bindtab Dump (%p) "
	 "=====================\n",
	 this);
  s->push_back (b);
  traverse (wrap (okdbg_dump_iterator, s));
  b.tosuio ()->clear ();
  b.fmt ("===================== End Bindtab Dump (%p) "
	 "=====================\n",
	 this);
  s->push_back (b);
}

void
pbinding_t::okdbg_dump_vec (vec<str> *s) const 
{
  if (!fn) {
    s->push_back ("** EMPTY PBINDING **");
  } else {
    strbuf b;
    b << fn << " -> " << hsh->to_str () << " (" << (toplev ? 1 : 0) << ")\n";
    s->push_back (b);
  }
}

int
nfs_safe_stat (const char *fn, struct stat *sb)
{
  int fd;
  int rc;
  if ((fd = open (fn, O_RDONLY)) < 0)
    return fd;
  if ((rc = fstat (fd, sb)) < 0)
    return rc;
  close (fd);
  return rc;
}

bool
basename_dirname (const str &in, str *d, str *b)
{
  size_t l = in.len ();

  if (l == 0) {
    *d = *b = NULL;
    return false;
  }

  bool ret = (in[0] == '/');
  const char *bp = in.cstr ();
  const char *ep = bp + l - 1;
  const char *p;
  for ( p = ep; p >= bp && *p != '/'; p--) ;

  // Set the basename if possible
  if (p == ep) {
    *b = NULL;
  } else {
    *b = str (p + 1);
  }

  // if there were multiple slashes keep backing up
  for ( ; p >= bp && *p == '/' ; p-- );

  if (p >= bp) {
    assert (*p != '/');
    *d = str (bp, p - bp + 1);
  } else {
    *d = NULL;
  }
  
  return ret;
}

str
errcode2str (const xpub_status_t &x)
{
  str r;
  switch (x.status) {
  case XPUB_STATUS_OK:
    break;
  case XPUB_STATUS_NOENT:
    r = "File not found";
    break;
  case XPUB_STATUS_RPC_ERR:
    {
      strbuf b;
      b << "RPC Error " << *x.rpc_err;
      r = b;
    }
    break;
  default:
    r = *x.error;
    break;
  }
  return r;
}

void
strip_comments (vec<str> *in)
{
  ssize_t start_from = -1;
  ssize_t lim = in->size ();
  for (ssize_t i = 0; i < lim; i++) {
    const char *cp = (*in)[i].cstr ();
    const char *pp = strchr (cp, '#');
    if (pp) {
      if (pp == cp) {
	start_from = i;
      } else {
	(*in)[i] = str (cp, pp - cp);
	start_from = i + 1;
      }
      break;
    }
  }
  while ( start_from >= 0 && start_from < ssize_t (in->size ()) )
    in->pop_back ();
}

str
trunc_at_first_null (const str &s)
{
  assert (s);

  const char *cp;
  size_t len = s.len ();
  size_t l;
  const char nullc = '\0';

  // Figure out if a '\0' or the end-of-string comes first.
  for (cp = s.cstr (), l = 0; *cp != nullc && l < len; cp++, l++);

  str ret;

  if (*cp == nullc) {
    
    // The most likely outcome
    if (l == len) ret = s;

    // There was a '\0' inside the string
    else ret = s.cstr ();

  } else {
   
    // String is not null terminated!
    mstr m (len + 1);
    memcpy (m.cstr (), s.cstr (), len);
    m[len] = nullc;
    ret = m;
  }

  return ret;
}
