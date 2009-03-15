
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

int 
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  if (argc != 2 && argc !=3)
    usage ();
  str s = file2str (argv[1]);
  u_int chnk = 400;
  if (argc == 3 && !convertint (argv[2], &chnk))
    usage ();
  u_int p = 0;

  if (!s)
    fatal << "cannot open file: " << argv[1];

  Bytef b2[100000];
  u_int obsz = 100000;
  u_int len;
  int err;

  char mp[10];
  memset (mp, 0, 10);
  mp[0] = 0x1f;
  mp[1] = 0x8b;
  mp[2] = 8;
  mp[9] = 0xff;
  rc_ignore (write (1, mp, 10));
  uLong crc = 0;
  Bytef *inb;
  uLong tot = 0;


  z_stream stream;
  stream.zalloc = (alloc_func)0;
  stream.zfree = (free_func)0;
  stream.opaque = (voidpf)0;

  err = deflateInit2 (&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
		      -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
  if (err != Z_OK)
    fatal << "deflate init error: " << err << "\n";

  for (p = 0; p <= s.len (); p += chnk) {
    len = min<u_int> (chnk, (s.len () - p));
    inb = const_cast<Bytef *> (reinterpret_cast<const Bytef *> 
			       (s.cstr () + p));
    stream.next_in = inb;
    stream.avail_in = len;
    stream.next_out = b2;
    stream.avail_out = obsz;

    crc = crc32 (crc, inb, len);
    err = deflate (&stream, Z_FULL_FLUSH);
    u_int olen = obsz - stream.avail_out;
    if (err != Z_OK) 
      fatal << "bad call to deflate: " << err << "\n";
    
    if (write (1, b2, olen) < int (olen))
      fatal << "short write.\n";
  }

  stream.next_in = NULL;
  stream.avail_in = 0;
  stream.next_out = b2;
  stream.avail_out = obsz;
  err = deflate (&stream, Z_FINISH);
  if (err != Z_STREAM_END)
    fatal << "could not end stream: " << err << "\n";
  u_int olen = obsz - stream.avail_out;
  if (write (1, b2, olen) < int (olen))
    fatal << "short write.\n";

  err = deflateEnd (&stream);
  tot = stream.total_in;
  if (err != Z_OK)
    fatal << "Could not end deflate: " << err << "\n";
  warn ("CRC: %lx\n", crc);
  warn ("len : %lu\n", tot);

  char buf[8];
  uLong_to_buf (crc, buf);
  uLong_to_buf (tot, buf+ 4);
  if (write (1, buf, 8) < 8)
    fatal << "short footer write.\n";
  exit (0);
}

