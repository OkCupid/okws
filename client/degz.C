
/* $Id$ */

#include "async.h"
#include "rxx.h"
#include "parseopt.h"
#include "vec.h"
#include "ahttp.h"
#include <stdio.h>
#include "zlib.h"

static void
usage ()
{
  fatal << "usage: degz <file>\n";
}

static int
getsize (const char **cp)
{
  int sz;
  int rc = sscanf (*cp, "%x\r\n", &sz);
  if (rc != 1) return -1;
#ifdef __clang_analyzer__
  assert(*cp)
#endif
  const char *cp2 = strstr (*cp, "\r\n");
  if (!cp2) return -1;
  *cp = cp2 + 2;
  return sz;
}

static int
myunc (Bytef *dest, uLong *destLen, const Bytef *source, uLong sourceLen)
{
    z_stream stream;
    int err;

    stream.next_in = (Bytef*)source;
    stream.avail_in = (uInt)sourceLen;
    /* Check for source > 64K on 16-bit machine: */
    if ((uLong)stream.avail_in != sourceLen) return Z_BUF_ERROR;

    stream.next_out = dest;
    stream.avail_out = (uInt)*destLen;
    if ((uLong)stream.avail_out != *destLen) return Z_BUF_ERROR;

    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;

    err = inflateInit2(&stream, -MAX_WBITS);
    if (err != Z_OK) return err;

    err = inflate(&stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        inflateEnd(&stream);
        return err == Z_OK ? Z_BUF_ERROR : err;
    }
    *destLen = stream.total_out;

    err = inflateEnd(&stream);
    return err;
}

int 
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  if (argc != 2 && argc !=3)
    usage ();
  str s = file2str (argv[1]);
  int off = -1;
  if (argc == 3 && !convertint (argv[2], &off))
    usage ();

  if (!s)
    fatal << "cannot open file: " << argv[1];

  const char *cp = strstr (s.cstr (), "\r\n\r\n");
  if (!cp)
    fatal << "file is not a raw HTTP dump: " << argv[1];
  cp += 4;
  size_t sz;
  Bytef outbuf[100000];
  uLong outlen = 100000;
  int rc = -1;
  u_int i = 0;
  Bytef b2[100000];
  Bytef *bp = b2;
  u_int t = 0;
  while ((sz = getsize (&cp)) > 0) {
    /*
    for (i = 0; i < sz && rc < 0; i++) {
      for (j = sz; j >= 0 && rc < 0; j--) {
	rc = uncompress (outbuf, &outlen, 
			 reinterpret_cast<const Bytef *> (cp+i), j);
	warn ("%d,%d,%d,%x,%x\n", i, j, rc, char (cp[i]), char (cp[i+1]));
      }
    }
    */
    memcpy (bp, cp, sz);
    bp += sz;
    t += sz;
    cp += (2 + sz);
  }
  
  rc = myunc (outbuf, &outlen, 
	      reinterpret_cast<const Bytef *> (b2+10), t - 10);
  if (rc < 0)
    fatal << "uncompress failed: " << rc << "\n";
  warn << "eureka!!! " << i << "\n";
  if (write (1, outbuf, outlen) < int (outlen))
    fatal << "short write.\n";
  exit (0);
}

