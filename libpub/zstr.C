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

#include "zstr.h"
#include <zconf.h>

ztab_t *ztab = NULL;    // external ztab object
z_stream zm;            // global zstream object
str zhdr;               // Gzip Hdr (10 chars long!)
int zlev;               // deflation level
bool zinitted = false;  // protect against stupid bugs
bool zdebug = false;    // debug messages

static int
zcompress (char *dest, uLong *dlenp, const char *src, uLong slen, int lev)
{
  uLong dlen = *dlenp;
  zm.next_in = const_cast<Bytef *> (reinterpret_cast<const Bytef *> (src));
  zm.avail_in = slen;
  zm.next_out = reinterpret_cast<Bytef *> (dest);
  zm.avail_out = dlen;

  if (!zinitted) {
     warn << "OKWS says: you forgot to call zinit()! i'm doing it for you\n";
     zinit (false);
  }
  assert (zinitted);
    

  if (lev == -1)
    lev = ok_gzip_compress_level;

  if (lev != zlev) {
    deflateParams (&zm, lev, Z_DEFAULT_STRATEGY);
    zlev = lev;
  }

  int err = deflate (&zm, Z_FULL_FLUSH);
  *dlenp = dlen - zm.avail_out;
  
  return err;
}

static str
zcompress (const str &in, int lev)
{
  uLong slen = in.len ();
  uLong dlen = int ((slen * 1001) / 1000) + 16;
  mstr m (dlen);
  int rc = zcompress (m.cstr (), &dlen, in.cstr (), slen, lev);
  if (rc != Z_OK) {
    warn << "deflate returned error: " << rc << "\n";
    return NULL;
  }
  m.setlen (dlen);
  return m;
}

static int
zfinish (char *dest, uLong *dlenp)
{
  uLong dlen = *dlenp;
  zm.next_in = NULL;
  zm.avail_in = 0;
  zm.next_out = reinterpret_cast<Bytef *> (dest);
  zm.avail_out = dlen;
  
  int err = deflate (&zm, Z_FINISH);
  *dlenp = dlen - zm.avail_out;
  
  int err2 = deflateReset (&zm);
  if (err2 != Z_OK)
    warn << "deflateReset failed! " << err2 << "\n";
  
  return err;
}

static int
uLong_to_buf (uLong l, char b[])
{
  for (u_int i = 0; i < 4; i++) {
    b[i] = l & 0xff;
    l >>= 8;
  }
  return 4;
}

bool
zstrobj::compress (int l) const
{
  return ((zs = zcompress (s, l)));
}

const str &
zstrobj::to_zstr (int l) const
{
  if (zs && clev >= l) {
    if (zdebug)
      warn << "compress saved,sz=" << len () << "\n"; // debug
    return zs;
  }
  compress (l);
  return zs;
}

zstr *
ztab_cache_t::lookup (const str &s)
{
  zstr *z = tab[s];
  if (z) {
    q.remove (z);
    q.insert_tail (z);
  }
  return z;
}

void
ztab_cache_t::make_room (size_t l)
{
  while (tot + l > maxtot)
    remove ();
}

void
ztab_cache_t::remove (zstr *z)
{
  if (!z) z = q.first;
  tot -= z->len ();
  q.remove (z);
  delete z;
}

void
ztab_cache_t::insert (zstr *z)
{
  tot += z->len ();
  assert (tot <= maxtot);
  assert (!tab[*z]);
  tab.insert (z);
  q.insert_tail (z);
}

zstr
ztab_cache_t::alloc (const str &s, bool lkp)
{
  zstr *z;
  if (lkp && (z = lookup (s))) {
    if (zdebug)
      warn << "cache hit, sz=" << s.len () << "\n"; // debug
    return *z;
  }
  size_t l = s.len ();
  if (l >= minz && l <= maxz && maxtot) {
    make_room (l);
    z = New zstr (s);
    insert (z);
    return *z;
  } else {
    return zstr (s);
  }
  return zstr (); // XXX should never get here
}

zstr *
ztab_cache_t::lookup (const char *p, size_t l)
{
  zstr *ret = NULL;
  u_int ip = reinterpret_cast<u_int> (p);
  zstr **zp = cptab[ip];
  if (zp) { 
    if ((*zp)->len () == l && !memcmp ((*zp)->cstr (), p, l)) 
      ret = *zp;
    else
      cptab.remove (ip); 
  }
  return ret;
}

zstr
ztab_cache_t::alloc (const char *p, size_t l, bool lkp)
{
  zstr *z;
  if (lkp && (z = lookup (p, l))) {
    if (zdebug)
      warn << "cache hit, sz=" << l << "\n"; // debug
    return *z;
  }

  u_int ip = reinterpret_cast<u_int> (p);
  if (l >= minz && l <= maxz) {
    make_room (l);
    z = New zstr (p, l);
    insert (z);
    cptab.insert (ip, z);
    return *z;
  } else {
    return zstr (p, l);
  }
  return zstr (); // XXX should never get here
}

void
zbuf::compress (strbuf *p, int level)
{
  strbuf2zstr ();
  u_int lim = zs.size ();
  p->tosuio ()->clear ();

  uLong crc = ::crc32 (0L, Z_NULL, 0);
  uLong ilen = 0;
  uLong dlen;
  int err;
  u_int dlui;
  char *ebcp;
  str s;
  u_int len;

  if (zdebug)
    warn << "zbuf::compress: ";

  for (u_int i = 0; i < lim; i++) {
    str z = zs[i].compress (level);
    
    if (zdebug)
      warnx << zs[i].len () << "," ;

    if (!z)
      goto compress_err;

    // first time through the loop add the gzip header; see zinit ()
    // for what that header actually looks like
    len = (i == 0) ? zhdr.len () + z.len () : z.len ();
    p->fmt ("%x\r\n", len);
    if (i == 0)  
      (*p) << zhdr;
    (*p) << z << "\r\n";
    crc = zs[i].crc32 (crc);
    ilen += zs[i].len ();
  }

  if (zdebug)
    warnx << "\n";

  //
  // Gzip File Trailer:
  //   (2bytes) - zlib final block
  //   (4bytes) - 32-bit CRC
  //   (4bytes) - 32-bit input file length
  //
  dlen = endbuf.len ();
  ebcp = endbuf.cstr ();
  if ((err = zfinish (ebcp, &dlen)) != Z_STREAM_END) {
    warn << "zfinish returned failure: " << err << "\n";
    goto compress_err;
  }
  dlen += uLong_to_buf (crc, ebcp + dlen);
  dlen += uLong_to_buf (ilen, ebcp + dlen);
  dlui = u_int (dlen);
  p->fmt ("%x\r\n", dlui);
  endbuf.setlen (dlui);
  s = endbuf;
  (*p) << s << "\r\n0\r\n\r\n";

	  
  return;

 compress_err:
  p->tosuio ()->clear ();
}

void
zbuf::output (strbuf *p)
{
  strbuf2zstr ();
  u_int lim = zs.size ();
  p->tosuio ()->clear ();
  for (u_int i = 0; i < lim; i++) {
    (*p) << zs[i].to_str ();
  }
}

size_t
zbuf::inflated_len () const
{
  size_t len = 0;
  u_int lim = zs.size ();
  for (u_int i = 0; i < lim; i++) 
    len += zs[i].to_str ().len ();
  return len;
}

void zinit (bool cache, int lev)
{
  zinitted = true;
  ztab = cache ? New ztab_cache_t () : New ztab_t ();
  zm.zalloc = (alloc_func)0;
  zm.zfree = (free_func)0;
  zm.opaque = (voidpf )0;
  zdebug = getenv ("GZIP_DEBUG");
  int err = deflateInit2 (&zm, lev, Z_DEFLATED, -MAX_WBITS, 
			  ok_gzip_mem_level, Z_DEFAULT_STRATEGY);
  if (err != Z_OK)
    fatal << "could not initialize zlib stream\n";

  u_int zhdrsize = 10;
  mstr ztmp (zhdrsize);
  char *zcp = ztmp.cstr ();
  memset (zcp, 0, zhdrsize);
  zcp[0] = 0x1f;     // gzip magic 0
  zcp[1] = 0x8b;     // gzip magic 1
  zcp[2] = 8;        // inflate/zlib
  zcp[9] = 0xff;     // unknown OS code
  zhdr = ztmp;

  zlev = lev == -1 ? ok_gzip_compress_level : lev;
}

void
zbuf::clear ()
{
  f.tosuio ()->clear ();
  zs.clear ();
}

int
zbuf::output (int fd, bool gz)
{
  to_strbuf (gz);
  int rc = 0;
  suio *uio = out.tosuio ();
  while (uio->resid () && (rc >= 0 || errno == EAGAIN)) {
    rc = uio->output (fd);
  }
  if (rc < 0) 
    warn ("in output: %m\n");

  return rc;
}

void
zbuf::to_zstr_vec (vec<zstr> *zs2)
{
  strbuf2zstr ();
  u_int l = zs.size ();
  zs2->setsize (l);
  for (u_int i = 0; i < l; i++)
    (*zs2)[i] = zs[i];
}
