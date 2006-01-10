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

#ifndef _LIBAHTTP_SUIOLITE_H
#define _LIBAHTTP_SUIOLITE_H 1

#define SUIOLITE_MAX_BUFLEN 0x40000
#define SUIOLITE_DEF_BUFLEN 0x10400

#define MALLOCTEST() \
   vNew char[0x10]

#include "async.h"

struct syscall_stats_t {
  syscall_stats_t () : n_recvmsg (0), n_readvfd (0), n_readv (0),
		       n_writev (0), n_writevfd (0) {}
  void clear () 
  { n_recvmsg = n_readvfd = n_readv = n_writev = n_writevfd = 0; }

  void dump (int tm)
  { warn ("syscalls[%d]: recvmsg:%d readvfd:%d readv:%d writev:%d "
	  "writevfd:%d\n", tm, n_recvmsg, n_readvfd, n_readv,
	  n_writev, n_writevfd); 
  }

  int n_recvmsg;
  int n_readvfd;
  int n_readv;
  int n_writev;
  int n_writevfd;
};

//
// How suiolite works:
//
// Can be in one of two configurations, as shown.  In the following
// diagrams:
//
//   + = "good data waiting to be read"
//   1,2 = "old data that can be written over"
// 
// Configuration 1:
//
//                     
//   [+++++++++++++111111111111111111++++++++++++++++++++++++++++++++]
//  buf          dep[0]             rp                             bep
//                                                                dep[1]
//
//    - data is only input into the region from dep[0] to rp
//    - data is only read from the region from rp to bep
//    - calling rembytes on bep-rp bytes will transition to configuration
//      2.
//    - region 2 is non-existent
//
// Configuration 2:
//
//   [11111111111111+++++++++++++++++++++++++++2222222222222222222222]
//  buf            rp                        dep[1]                 bep
//  dep[0]
//
//    - data is input first into the region from dep[1] to bep, and then
//      from dep[0] to rp.
//    - data is read out of the suiolite from rp to dep[1]
//    - data is removed only from rp to dep[1]
//
//  The rp pointer is only bumped once rembytes() is called, so calling
//  getdata() twice in a row without a call to rembytes() in between
//  will return the same thing.
//  
// Invariants:
//   - Either dep[1] == bep OR dep[0] == buf.
//   - The first case is configration 1, the second case is configuration 2.
//  

class suiolite {
public:
  suiolite (int l = SUIOLITE_DEF_BUFLEN, cbv::ptr s = NULL) 
    : len (min<int> (l, SUIOLITE_MAX_BUFLEN)), buf ((char *)xmalloc (len)),
      bep (buf + len), rp (buf), scb (s), peek (false), bytes_read (0)
  {
    for (int i = 0; i < 2; i++) dep[i] = buf;
  }
  ~suiolite () { xfree (buf); }


  void clear ();
  void recycle (cbv::ptr s = NULL) { setscb (s); }

  void setpeek (bool b = true) { peek = b; }
  void setscb (cbv::ptr c) { scb = c; }
  ssize_t input (int fd, int *nfd = NULL, syscall_stats_t *ss = NULL);
  ssize_t resid () const { return (dep[1] - rp) + (dep[0] - buf); }
  bool full () const { return (resid () == len); }
  char *getdata (ssize_t *nbytes) const { *nbytes = dep[1] - rp; return rp; }
  void rembytes (ssize_t nbytes);
  int getlen () const { return len; }

private:
  const int len;
  char *buf;
  char *bep;     // buffer end pointer
  char *rp;      // read pointer
  char *dep[2];  // data end pointer
  iovec iov[2];
  cbv::ptr scb;  // space CB -- callback if space is available in the uio
  bool peek;
  u_int bytes_read;
};

#endif /* _LIBAHTTP_SUIOLITE_H */
