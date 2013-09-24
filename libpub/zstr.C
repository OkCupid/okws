
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
#include "sfs_profiler.h"

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
    sfs_profiler::enter_vomit_lib ();
    deflateParams (&zm, lev, Z_DEFAULT_STRATEGY);
    sfs_profiler::exit_vomit_lib ();
    zlev = lev;
  }

  sfs_profiler::enter_vomit_lib ();
  int err = deflate (&zm, Z_FULL_FLUSH);
  sfs_profiler::exit_vomit_lib ();
  *dlenp = dlen - zm.avail_out;
  
  return err;
}

size_t
max_compressed_len (size_t in)
{
  return ((in * 1001) / 1000) + 16;
}

str
zcompress (const str &in, int lev)
{
  uLong slen = in.len ();
  uLong dlen = max_compressed_len (slen);
  mstr m (dlen);
  int rc = zcompress (m.cstr (), &dlen, in.cstr (), slen, lev);
  if (rc != Z_OK) {
    warn << "deflate returned error: " << rc << "\n";
    return NULL;
  }
  m.setlen (dlen);
  return m;
}

static bool 
zdecompress_ok (int i) { return (i == Z_STREAM_END || i == Z_BUF_ERROR); }

str
zdecompress (const str &in)
{
  vec<str> v;
  strbuf b;
  size_t seglen = 0x1000;
  z_stream stream;
  int err;
  int rc;
  str ret;

  const Bytef *inbuf = reinterpret_cast<const Bytef *> (in.cstr ());
  uLong inlen = static_cast<uLong> (in.len ());

  stream.next_in = const_cast<Bytef *> (inbuf);
  stream.avail_in = inlen;
  stream.avail_out = 0;
  stream.zalloc = (alloc_func) NULL;
  stream.zfree = (free_func) NULL;

  err = inflateInit2 (&stream, -MAX_WBITS);
  if (err != Z_OK) {
    warn << "zlib::inflateInit2: error " << err << "\n";
  } else {
    do {
      mstr m (seglen);
      stream.avail_out = static_cast<uLong> (seglen);
      stream.next_out = reinterpret_cast<Bytef *> (m.cstr ());
      
      rc = inflate (&stream, Z_FINISH);
      
      if (zdecompress_ok (rc)) {
	m.setlen (seglen - static_cast<size_t> (stream.avail_out));
	str s = m;
	v.push_back (s);
	b << s;
      }
      
    } while (stream.avail_in > 0 && rc == Z_BUF_ERROR);
    
    if (!zdecompress_ok (rc)) {
      warn << "zlib::inflate: error " << rc << "\n";
    } else if (stream.avail_in != 0) {
      warn << "zlib::inflate: data leftover!\n";
    } else {
      rc = inflateEnd (&stream);
      if (rc != Z_OK) {
	warn << "zlib::inflateEnd: error " << rc << "\n";
      } else {
	ret = b;
      }
    }
  }
  return ret;
}

static int
zfinish (char *dest, uLong *dlenp)
{
  uLong dlen = *dlenp;
  zm.next_in = NULL;
  zm.avail_in = 0;
  zm.next_out = reinterpret_cast<Bytef *> (dest);
  zm.avail_out = dlen;
  
  sfs_profiler::enter_vomit_lib ();
  int err = deflate (&zm, Z_FINISH);
  sfs_profiler::exit_vomit_lib ();
  *dlenp = dlen - zm.avail_out;
  
  sfs_profiler::enter_vomit_lib ();
  int err2 = deflateReset (&zm);
  sfs_profiler::exit_vomit_lib ();
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
  return (bool(zs = zcompress (s, l)));
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
  tab.remove (z);

  intptr_t scc_p; // scc_p = static const char pointer
  if ((scc_p = z->get_scc_p ())) {
    assert (cptab[scc_p]);
    cptab.remove (scc_p);
  }

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
  intptr_t ip = reinterpret_cast<intptr_t> (p);
  zstr **zp = cptab[ip];
  if (zp) { 
    if ((*zp)->len () == l && !memcmp ((*zp)->cstr (), p, l)) 
      ret = *zp;
    else {
      (*zp)->set_scc_p (0);
      cptab.remove (ip); 
    }
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

  intptr_t ip = reinterpret_cast<intptr_t> (p);
  if (l >= minz && l <= maxz) {
    make_room (l);
    z = New zstr (p, l);
    insert (z);
    z->set_scc_p (ip);
    cptab.insert (ip, z);
    return *z;
  } else {
    return zstr (p, l);
  }
  return zstr (); // XXX should never get here
}

//-----------------------------------------------------------------------

void
zbuf::compress (strbuf *p, compressible_t::opts_t o)
{
  strbuf2zstr ();
  size_t lim = zs.size ();
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

  for (size_t i = 0; i < lim; i++) {
    str z = zs[i].compress (o.lev);
    
    if (zdebug)
      warnx << zs[i].len () << "," ;

    if (!z)
      goto compress_err;

    // first time through the loop add the gzip header; see zinit ()
    // for what that header actually looks like
    len = (i == 0) ? zhdr.len () + z.len () : z.len ();
    if (o.chunked) p->fmt ("%x\r\n", len);
    if (i == 0)  
      (*p) << zhdr;
    (*p) << z;
    if (o.chunked) { (*p) << "\r\n"; }
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
  if (o.chunked) { p->fmt ("%x\r\n", dlui); }
  endbuf.setlen (dlui);
  s = endbuf;
  (*p) << s;
  if (o.chunked) { (*p) << "\r\n0\r\n\r\n"; }
	  
  return;

 compress_err:
  p->tosuio ()->clear ();
}

//-----------------------------------------------------------------------

void
zbuf::output (strbuf *p, bool doclear)
{
  strbuf2zstr ();
  size_t lim = zs.size ();
  if (doclear)
    p->tosuio ()->clear ();
  for (size_t i = 0; i < lim; i++) {
    (*p) << zs[i].to_str ();
  }
}

//-----------------------------------------------------------------------

size_t
zbuf::inflated_len () const
{
  size_t len = 0;
  size_t lim = zs.size ();
  for (size_t i = 0; i < lim; i++) 
    len += zs[i].to_str ().len ();

  // add the stuff also in the buf that hasn't been compressed yet.
  len += f.tosuio ()->resid ();
  return len;
}

//-----------------------------------------------------------------------

void zinit (bool cache, int lev)
{
  zinitted = true;
  ztab = cache ? New ztab_cache_t () : New ztab_t ();
  zm.zalloc = (alloc_func)0;
  zm.zfree = (free_func)0;
  zm.opaque = (voidpf )0;
  zdebug = getenv ("GZIP_DEBUG");
  sfs_profiler::enter_vomit_lib ();
  int err = deflateInit2 (&zm, lev, Z_DEFLATED, -MAX_WBITS, 
			  ok_gzip_mem_level, Z_DEFAULT_STRATEGY);
  sfs_profiler::exit_vomit_lib ();
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

//-----------------------------------------------------------------------

void
zbuf::clear ()
{
  f.tosuio ()->clear ();
  zs.clear ();
}

//-----------------------------------------------------------------------

int
zbuf::output (int fd, compressible_t::opts_t o)
{
  to_strbuf (o);
  int rc = 0;
  suio *uio = out.tosuio ();
  while (uio->resid () && (rc >= 0 || errno == EAGAIN)) {
    rc = uio->output (fd);
  }
  if (rc < 0) 
    warn ("in output: %m\n");

  return rc;
}

//-----------------------------------------------------------------------

void
zbuf::to_zstr_vec (vec<zstr> *zs2)
{
  strbuf2zstr ();
  size_t l = zs.size ();
  zs2->setsize (l);
  for (size_t i = 0; i < l; i++)
    (*zs2)[i] = zs[i];
}

//-----------------------------------------------------------------------

int
zbuf::naive_compress (strbuf *b, int lev)
{
  if (!zinitted) {
     warn << "OKWS says: you forgot to call zinit()! i'm doing it for you\n";
     zinit (false);
  }
  assert (zinitted);
  z_stream z;
  z.zalloc = Z_NULL;
  z.zfree = Z_NULL;
  z.opaque = Z_NULL;
  int rc = deflateInit2 (&z, lev, Z_DEFLATED, -MAX_WBITS, 
			 ok_gzip_mem_level, Z_DEFAULT_STRATEGY);
  if (rc != Z_OK)
    return rc;

  strbuf2zstr ();
  size_t inlen = inflated_len ();
  strbuf tmp;

  // with little extra room for headers and footers
  size_t outlen = max_compressed_len (inlen) + 64;

  tmp << zhdr;

  uLong crc = ::crc32 (0L, Z_NULL, 0);

  mstr out (outlen);
  char *outp = out.cstr ();
  char *endp = out.cstr () + outlen;
  z.avail_out = outlen;
  deflateParams (&z, lev, Z_DEFAULT_STRATEGY);

  for (size_t i = 0; rc == 0 && i <= zs.size (); i++) {

    bool end = (i == zs.size ());
    const char *src;
    size_t srclen;

    if (end) {
      src = NULL;
      srclen = 0;
    } else {
      src = zs[i].cstr ();
      srclen = zs[i].len ();
    }

    if ((src && srclen) || end) {
      z.next_out = reinterpret_cast<Bytef *> (outp);
      size_t avail_out_pre = endp - outp;
      z.avail_out = avail_out_pre;
      z.next_in = const_cast<Bytef *> (reinterpret_cast<const Bytef *> (src));
      z.avail_in = srclen;
      if (!end) { crc = zs[i].crc32(crc); }
      int drc = deflate (&z, end ? Z_FINISH : Z_NO_FLUSH);
      if ((!end && drc != Z_OK) || (end && drc != Z_STREAM_END)) {
	warn << "Compression error: " << drc << "\n";
	rc = -1;
      }
      outp += (avail_out_pre - z.avail_out);
    }
  }
  
  if (rc == 0) {
    assert (outp <= endp);
    out.setlen (outp - out.cstr ());
    str out_s = out;
    tmp << out_s;
    b->hold_onto (out_s);

    size_t trailer_size = 16;
    mstr trailer (trailer_size);
    char *p = trailer.cstr ();
    p += uLong_to_buf (crc, p);
    p += uLong_to_buf (inlen, p);
    size_t bytes = p - trailer.cstr ();
    assert (bytes < trailer_size);
    trailer.setlen (bytes);
    str trailer_s = trailer;

    tmp << trailer_s;
    b->hold_onto (trailer_s);
    b->take (tmp);
  }

  deflateEnd (&z);

  return rc;
}

//-----------------------------------------------------------------------

int 
zbuf::to_strbuf (strbuf *out, compressible_t::opts_t o)
{
  int rc = 0;
  switch (o.mode) {
  case GZIP_NONE:
    output (out);
    break;
  case GZIP_SMART:
    compress (out, o);
    break;
  case GZIP_NAIVE:
    rc = naive_compress (out, o.lev);
    break;
  default:
    warn << "unknown gzip mode given\n";
    rc = -1;
    break;
  }
  return rc;
}

//-----------------------------------------------------------------------

// We'll only ever output chunked output if it's requested,
// and we're attempting smart gzipping.  In other case, we prepare
// a regular body.
compressible_t::opts_t::opts_t (gzip_mode_t m, bool chk, int lev)
  : mode (m),
    chunked (m == GZIP_SMART && chk && ok_gzip_chunking),
    lev (lev) {}

//-----------------------------------------------------------------------
