
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
suiolite::input (int fd, int *nfd)
{
  if (full ())
    return 0;

  iov[0].iov_base = dep[1];
  iov[0].iov_len = bep - dep[1];
  iov[1].iov_base = dep[0];
  iov[1].iov_len = rp - dep[0];

  ssize_t n = 0;
  if (nfd) {
    n = readvfd (fd, iov, 2, nfd);
  } else if (peek) {
    struct msghdr mh;
    bzero (&mh, sizeof (mh));
    mh.msg_iov = (struct iovec *) iov;
    mh.msg_iovlen = 2;
    n = recvmsg (fd, &mh, MSG_PEEK);
  } else {
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
