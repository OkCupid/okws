
// -*-c++-*-
/* $Id$ */

#ifndef _LIBAHTTP_SUIOLITE_H
#define _LIBAHTTP_SUIOLITE_H 1

#define SUIOLITE_MAX_BUFLEN 0x40000
#define SUIOLITE_DEF_BUFLEN 0x10400

#define MALLOCTEST() \
   vNew char[0x10]

#include "async.h"

class suiolite {
public:
  suiolite (int l = SUIOLITE_DEF_BUFLEN, cbv::ptr s = NULL) 
    : len (min<int> (l, SUIOLITE_MAX_BUFLEN)), buf ((char *)xmalloc (len)),
      bep (buf + len), rp (buf), scb (s), peek (false), bytes_read (0)
  {
    for (int i = 0; i < 2; i++) dep[i] = buf;
  }
  ~suiolite () { xfree (buf); }

  void setpeek () { peek = true; }
  void setscb (cbv::ptr c) { scb = c; }
  ssize_t input (int fd, int *nfd = NULL);
  ssize_t resid () const { return (dep[1] - rp) + (dep[0] - buf); }
  bool full () const { return (resid () == len); }
  char *getdata (ssize_t *nbytes) const { *nbytes = dep[1] - rp; return rp; }
  void rembytes (ssize_t nbytes);

private:
  int len;
  char *buf;
  char *bep;     // buffer end pointer
  char *rp;      // read pointer
  char *dep[2];  // data end pointer
  iovec iov[2];
  cbv::ptr scb;  // space CB -- callback if space is available in the uio
  bool peek;
  bool bytes_read;
};

#endif /* _LIBAHTTP_SUIOLITE_H */
