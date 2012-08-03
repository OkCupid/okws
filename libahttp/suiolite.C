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
  dont_peek = false;
  bytes_read = 0;

  for (int i = 0; i < N_REGIONS; i++) dep[i] = buf;
}

void
suiolite::load_iov ()
{
  iov[0].iov_base = dep[1];
  iov[0].iov_len = bep - dep[1];
  iov[1].iov_base = dep[0];
  iov[1].iov_len = rp - dep[0];
}

void
suiolite::account_for_new_bytes (ssize_t n)
{
  assert (n >= 0);

  //
  // we've read in n bytes, which may have been into the two
  // regions of the buffer.  Thus, we need to increment the
  // two data-end-pointers (dep[]) to reflect that there is
  // more data in those regions.  But do this in REVERSE order
  // since we loaded the iovec first with region 2 and then
  // with region 1.
  //
  // alternate implementation (maybe should change...)
  //
  //  int len = min<int> (n, iov[0].iov_len);
  //  dep[1] += len;
  //  dep[0] += (n - len);
  //
  bytes_read += n;
  for (int i = 0; i < N_REGIONS; i++) {
    int len = min<int> (n, iov[i].iov_len);
    dep[1 - i] += len;
    n -= len;
  }
}

size_t
suiolite::load_from_buffer (const char *input, size_t len)
{
  if (len > capacity ()) {
    grow (len + 10);
  }

  size_t ret = 0;
  size_t tmp = bep - dep[1];
  size_t nb = min<size_t> (len, tmp);
  if (nb > 0) {
    memcpy (dep[1], input, nb);
    dep[1] += nb;
    ret += nb;
  }
  len -= nb;
  input += nb;
  tmp = rp - dep[0];
  nb = min<size_t> (len, tmp);
  if (nb > 0) {
    memcpy (dep[0], input, nb);
    dep[0] += nb;
    ret += nb;
  }
  bytes_read += ret;
  return ret;
}

ssize_t
suiolite::input (int fd, int *nfd, syscall_stats_t *ss)
{
  if (full ())
    return 0;

  load_iov ();

  ssize_t n = 0;
  if (nfd) {
    if (ss) ss->n_readvfd ++;
    n = readvfd (fd, iov, N_REGIONS, nfd);
  } else if (peek && !dont_peek) {
    struct msghdr mh;
    bzero (&mh, sizeof (mh));
    mh.msg_iov = (struct iovec *) iov;
    mh.msg_iovlen = N_REGIONS;
    if (ss) ss->n_recvmsg ++;
    n = recvmsg (fd, &mh, MSG_PEEK);
  } else {
    if (ss) ss->n_readv ++;
    n = readv (fd, iov, N_REGIONS);
  }
    
  if (n > 0)  {
    account_for_new_bytes (n);
  }

  return n;
}

void
suiolite::rembytes (ssize_t nbytes)
{
  ssize_t rd = resid ();
  assert (rd >= nbytes);
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

iovec *
suiolite::get_iov (size_t *len)
{
  load_iov ();
  if (len) *len = N_REGIONS;
  return iov;
}

ssize_t 
suiolite::resid () const 
{ 
  size_t a = dep[1] - rp;
  size_t b = dep[0] - buf;
  return (a+b);
}

size_t
suiolite::capacity () const
{
  int inuse = resid ();
  assert (inuse <= len);
  return len - inuse;
}

void
suiolite::grow (size_t ns)
{
  assert (resid () == 0);
  assert (bytes_read == 0);
  xfree (buf);
  len = min<int> (ns, SUIOLITE_MAX_BUFLEN);
  buf = static_cast<char *> (xmalloc (len));
  clear ();
}
