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

#include "suiolite.h"

void
suiolite::clear ()
{
  scb = NULL;
  bep = buf + len;
  rp = buf;
  peek = false;
  bytes_read = 0;

  for (int i = 0; i < 2; i++) dep[i] = buf;
}


ssize_t
suiolite::input (int fd, int *nfd, syscall_stats_t *ss)
{
  if (full ())
    return 0;

  iov[0].iov_base = dep[1];
  iov[0].iov_len = bep - dep[1];
  iov[1].iov_base = dep[0];
  iov[1].iov_len = rp - dep[0];

  ssize_t n = 0;
  if (nfd) {
    if (ss) ss->n_readvfd ++;
    n = readvfd (fd, iov, 2, nfd);
  } else if (peek) {
    struct msghdr mh;
    bzero (&mh, sizeof (mh));
    mh.msg_iov = (struct iovec *) iov;
    mh.msg_iovlen = 2;
    if (ss) ss->n_recvmsg ++;
    n = recvmsg (fd, &mh, MSG_PEEK);
  } else {
    if (ss) ss->n_readv ++;
    n = readv (fd, iov, 2);
  }
    
  if (n <= 0)
    return n;
  bytes_read += n;
  ssize_t tn = n;
  for (int i = 0; i < 2; i++) {
    int len = min<int> (tn, iov[i].iov_len);
    dep[1 - i] += len;
    tn -= len;
  }
  return n;
}

void
suiolite::rembytes (ssize_t nbytes)
{
  assert (resid () >= nbytes);
  bool docall = full () && nbytes > 0 && scb;
  int len2 = bep - rp;
  if (nbytes >= len2) {
    nbytes -= len2;
    rp = buf + nbytes;
    dep[1] = dep[0];
    dep[0] = buf; 
  } else {
    rp += nbytes;
  }
  if (docall)
    (*scb) ();
}
