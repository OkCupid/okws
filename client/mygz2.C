
/* $Id$ */

#include "async.h"
#include "rxx.h"
#include "parseopt.h"
#include "vec.h"
#include "ahttp.h"
#include <stdio.h>
#include "zlib.h"

#define DEF_MEM_LEVEL 9

static void
usage ()
{
  fatal << "usage: mygz <file>\n";
}

void
uLong_to_buf (uLong l, char b[])
{
  for (u_int i = 0; i < 4; i++) {
    b[i] = l & 0xff;
    l >>= 8;
  }

}

timeval tv;

inline void startt () { gettimeofday (&tv, NULL); }
inline int stopt () 
{ 
  timeval tv2; 
  gettimeofday (&tv2, NULL); 
  return ( (tv2.tv_sec - tv.tv_sec) * 1000000 +
	   (tv2.tv_usec - tv.tv_usec)) ;
}

u_int
go (const str &s, mstr *m, u_int out_avail)
{
  u_int chnk = 0x8000;
  u_int len;
  int err;

  char *outp = m->cstr ();
  memset (outp, 0, 10);
  outp[0] = 0x1f;
  outp[1] = 0x8b;
  outp[2] = 8;
  outp[9] = 0xff;

  outp += 10;
  out_avail -= 10;

  uLong crc = 0;
  Bytef *inb;
  uLong tot = 0;

  z_stream stream;
  stream.zalloc = (alloc_func)0;
  stream.zfree = (free_func)0;
  stream.opaque = (voidpf)0;
  stream.avail_out = out_avail;

  err = deflateInit2 (&stream, Z_BEST_COMPRESSION, Z_DEFLATED,
		      -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
  if (err != Z_OK)
    fatal << "deflate init error: " << err << "\n";

  //
  // might not be big enough for some cases... oh wel...
  //

  u_int p;
  for (p = 0; p <= s.len (); p += chnk) {
    len = min (chnk, static_cast<u_int> ((s.len () - p)));
    inb = const_cast<Bytef *> (reinterpret_cast<const Bytef *> 
			       (s.cstr () + p));
    stream.next_in = inb;
    stream.avail_in = len;
    stream.next_out = reinterpret_cast<Bytef *> (outp);

    uLong t = stream.avail_out;
    crc = crc32 (crc, inb, len);
    err = deflate (&stream, Z_NO_FLUSH);

    // keep track of how many bytes written
    outp += (t - stream.avail_out);

    if (err != Z_OK) 
      fatal << "bad call to deflate: " << err << "\n";
  }

  stream.next_in = NULL;
  stream.avail_in = 0;
  stream.next_out = reinterpret_cast<Bytef *> (outp);
  warn << "left loop\n";
  uLong t = stream.avail_out;
  err = deflate (&stream, Z_FINISH);
  if (err != Z_STREAM_END)
    fatal << "could not end stream: " << err << "\n";
  outp += (t - stream.avail_out);

  err = deflateEnd (&stream);
  tot = stream.total_in;
  if (err != Z_OK)
    fatal << "Could not end deflate: " << err << "\n";

  uLong_to_buf (crc, outp);
  outp += 4;
  uLong_to_buf (tot, outp);
  outp += 4;

  u_int osz = outp - m->cstr ();

  warn ("CRC: %lx\n", crc);
  warn ("len : %lu\n", tot);
  warn ("out : %d\n", osz);

  return osz;
}


int 
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  if (argc != 2)
    usage ();
  str s = file2str (argv[1]);

  if (!s)
    fatal << "cannot open file: " << argv[1];

  u_int niter = 10;
  u_int osz = 0;
  u_int32_t tot = 0;
  mstr *outbuf = NULL;

  u_int slen = s.len ();
  u_int out_avail = u_int ((slen / 1000) * 1001) + 16;

  for (u_int i = 0; i < niter; i++) {

    if (outbuf)
      delete outbuf;
    outbuf = New mstr (out_avail);

    warn << "+ starting compression\n";
    startt ();
    osz = go (s, outbuf, out_avail); 
    u_int t = stopt ();
    warn << "- ending compression (time=" << t << ")\n";
    tot += t;
    
  }

  outbuf->setlen (osz);

  //
  // write out the buffer once, just to make sure we were getting reasonable
  // data output (and not some bullshit)
  //
  u_int i = 0;
  do {
    int rc = write (1, outbuf->cstr () + i, min<u_int> (2048, osz - i));
    if (rc < 0)
      panic ("write error!\n");
    i += rc;
  } while (i < osz);

  u_int64_t bw = osz / 1024;
  bw *= 1000000;
  bw /= tot;
  bw *= niter;


  warn ("Input: %d bytes\n"
	"Output: %d bytes\n"
	"Iterations: %d\n"
	"Usecs Total: %d\n"
	"Compression ratio * 1000: %d\n"
	"Throughput (kB/sec): %d\n",
	slen, osz, niter, tot, osz * 1000 / slen, u_int32_t (bw));

}

