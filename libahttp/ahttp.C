
/* $Id$ */

#include "ahttp.h"
#include "httpconst.h"
#include <sys/types.h>
#include <sys/socket.h>

int ahttpcon_spawn_pid;

void
ahttpcon::clone (ref<ahttpcon_clone> xc)
{
  assert (!xc->ateof ());
  sockaddr_in *sin;
  if ((sin = xc->get_sin ()))
    out->print ((void *)sin, sizeof (*sin));
  sendfd (xc->takefd ());
}

/*
void
ahttpcon::stopread () 
{
  if (rcbset) {
    fdcb (fd, selread, NULL);
    rcbset = false;
  }
}
*/

void
ahttpcon::sendfd (int sfd, bool closeit)
{
  fdsendq.push_back (fdtosend (sfd, closeit));
  wrsync ();
  output ();
}

int
ahttpcon_clone::takefd ()
{
  int ret = fd;
  if (fd >= 0) {
    fdcb (fd, selread, NULL);
    fdcb (fd, selwrite, NULL);
    wcbset = false;
  }
  fd = -1;
  ccb = NULL;
  return ret;
}

void
ahttpcon::send (const strbuf &b, cbv::ptr cb)
{
  return sendv (b.iov (), b.iovcnt (), cb);
}

void
ahttpcon::copyv (const iovec *iov, int cnt)
{
  out->copyv (iov, cnt, 0);
}

void
ahttpcon::sendv (const iovec *iov, int cnt, cbv::ptr cb)
{
  assert (!destroyed);
  u_int32_t len = iovsize (iov, cnt);
  if (fd < 0) {
    // if an EOF happened after the read but before the send,
    // we'll wind up here.
    warn ("write not possible due to EOF\n");
    if (cb) (*cb) ();
    return;
  }
  bytes_sent += len;
  if (!out->resid () && cnt < min (16, UIO_MAXIOV)) {
    ssize_t skip = writev (fd, iov, cnt);
    if (skip < 0 && errno != EAGAIN) {
      warn ("error in ahttpcon::sendv: %m\n"); // XXX debug
      fail ();
      // still need to signal done sending....
      if (cb) (*cb) ();
      return;
    } else
      out->copyv (iov, cnt, max<ssize_t> (skip, 0));
  } else {
    out->copyv (iov, cnt, 0);
  }
  if (cb)
    out->iovcb (cb);
  output ();
}

void
ahttpcon::output ()
{
  ssize_t n = 0;
  int cnt = 0;
  do {
    while (!syncpts.empty () && out->iovno () >= syncpts.front ())
      syncpts.pop_front ();
    cnt = syncpts.empty () ? (size_t) -1 
      : int (syncpts.front () - out->iovno ());
  } while ((n = dowritev (cnt)) > 0);

  if (n < 0) {
    warn ("error in ahttpcon::output: %m\n"); // XXX -- debug
    fail ();
  } else if (out->resid () && !wcbset) {
    wcbset = true;
    fdcb (fd, selwrite, wrap (this, &ahttpcon::output));
  }
  else if (!out->resid () && wcbset) {
    wcbset = false;
    fdcb (fd, selwrite, NULL);
  }
}

void
ahttpcon::setrcb (cbi::ptr cb)
{
  if (!cb) {
    rcb = NULL;
    return;
  }
    
  if (enable_selread ()) {
    rcb = cb;
    int i = in->resid ();
    if (i)
      (*rcb) (i);
  } else {
    (*cb) (0);
  }
}

void
ahttpcon::wrsync ()
{
  u_int64_t iovno = out->iovno () + out->iovcnt ();
  if (!syncpts.empty () && syncpts.back () == iovno)
    return;
  syncpts.push_back (iovno);
  out->breakiov ();
}

int
ahttpcon_dispatch::dowritev (int cnt)
{
  if (fdsendq.empty ())
    return out->output (fd, cnt);

  static timeval ztv;
  if (!fdwait (fd, selwrite, &ztv))
    return 0;

  if (cnt < 0)
    cnt = out->iovcnt ();
  if (cnt > UIO_MAXIOV)
    cnt = UIO_MAXIOV;
  ssize_t n = writevfd (fd, out->iov (), cnt, fdsendq.front ().fd);
  if (n < 0)
    return errno == EAGAIN ? 0 : -1;
  fdsendq.pop_front ();
  out->rembytes (n);
  return 1;
}


ahttpcon_clone::ahttpcon_clone (int f, sockaddr_in *s, size_t ml) 
  : ahttpcon (f, s, ml), maxline (ml), ccb (NULL), 
    found (false), delimit_state (0), delimit_status (HTTP_OK),
    delimit_start (NULL), bytes_scanned (0)
{
  in->setpeek ();
}

ahttpcon::~ahttpcon ()
{ 
  destroyed = true;
  fail ();
  if (sin) xfree (sin);
  delete in;
  delete out;
}

void
ahttpcon::fail ()
{
  if (fd >= 0) {
    fdcb (fd, selread, NULL);
    fdcb (fd, selwrite, NULL);
    close (fd);
  }
  fd = -1;
  if (!destroyed) {
    ref<ahttpcon> hold (mkref (this));
    if (rcb)
      (*rcb) (0);
    else if (eofcb)
      (*eofcb) ();
    fail2 ();
  }
}

void
ahttpcon_listen::fail2 ()
{
  if (lcb) 
    (*lcb) (NULL);
}

void
ahttpcon_clone::fail2 ()
{
  if (ccb)
    (*ccb) (NULL, HTTP_BAD_REQUEST);
}

void
ahttpcon_clone::setccb (clonecb_t c)
{
  assert (!destroyed);
  if (enable_selread ()) 
    ccb = c;
  else
    (*ccb) (NULL, HTTP_BAD_REQUEST);
}

void
ahttpcon::spacecb ()
{
  enable_selread ();
}

bool
ahttpcon::enable_selread ()
{
  if (fd < 0)
    return false;
  if (!rcbset) {
    fdcb (fd, selread, wrap (this, &ahttpcon::input));
    rcbset = true;
  }
  return true;
}

void
ahttpcon_listen::setlcb (listencb_t c)
{
  if (enable_selread ())
    lcb = c;
  else
    (*c) (NULL);
}

ssize_t
ahttpcon_listen::doread (int fd)
{
  int tnfd = -1;
  assert (!in->resid ());
  ssize_t n = in->input (fd, &tnfd);
  bool sin_used = false;
  sockaddr_in *sin2 = NULL;
  if (n == sizeof (sockaddr_in)) {
    sin2 = (sockaddr_in *) xmalloc (n);
    char *p = (char *)sin2;
    char *p2;
    ssize_t n2, tot;
    tot = 0;
    while ((p2 = in->getdata (&n2)) && n2) {
      memcpy (p, p2, n2);
      p += n2;
      tot += n2;
      in->rembytes (n2);
    }
    assert (tot == n);
  } else if (n > 0) {
    in->rembytes (n);
    warn ("random junk received on FD listen socket");
  }

  if (tnfd >= 0) 
    if (lcb) {
      (*lcb) (New refcounted<ahttpcon> (tnfd, sin2));
      sin_used = true;
    } else {
      close (tnfd);
      warn ("ahttpcon_listen: no one listening to claim file descriptor\n");
    }
  
  if (sin2 && !sin_used)
    xfree (sin2);

  return n;
}

ssize_t
ahttpcon::doread (int fd)
{
  ssize_t s = in->input (fd);
  return s;
}

void
ahttpcon::input ()
{
  if (fd < 0)
    return;
  ref<ahttpcon> hold (mkref (this));  // Don't let this be freed under us
  if (in->full ()) {
    fdcb (fd, selread, NULL);
    rcbset = false;
    return;
  }
  ssize_t n = doread (fd);
  if (n < 0) {
    if (errno != EAGAIN) {
      warn ("error in ahttpcon::input: %m\n"); // XXX debug      
      fail ();
    }
    return;
  }
  if (n == 0) {
    eof = true;
    fail ();
    return;
  }

  bytes_recv += n;

  // stop DOS attacks?
  if (recv_limit > 0 && bytes_recv > recv_limit) {
    eof = true;
    fail ();
    return;
  }

  recvd_bytes (n);
}

void 
ahttpcon::recvd_bytes (int n)
{
  if (rcb) // XXX -- what is this??  // && ((n && !in->resid ()) || !n)) 
    (*rcb) (n);
}

void
ahttpcon_clone::recvd_bytes (int n)
{
  str s = delimit (n);
  if (s || delimit_status != HTTP_OK) {
    if (ccb) {
      (*ccb) (s, delimit_status);
      ccb = NULL;
    }
    end_read ();
  }
}

void
ahttpcon_clone::end_read ()
{
  if (fd >= 0) {
    fdcb (fd, selread, NULL);
    found = true;
  }
}

str
ahttpcon_clone::delimit (int dummy)
{
  int nbytes;
  int i;
  while (in->resid ()) {
    i = 0;
    for (char *p = in->getdata (&nbytes); i < nbytes; i++, p++) {
      if (++bytes_scanned > maxline) {
	delimit_status = HTTP_URI_TOO_BIG;
	in->rembytes (i);
	return NULL;
      }
      switch (delimit_state) {
      case 0:
	if (*p == ' ') 
	  delimit_state = 1;
	else if (*p < 'A' || *p > 'Z') {
	  delimit_status = HTTP_BAD_REQUEST;
	  in->rembytes (i);
	  return NULL;
	}
	break;
      case 1:
	// note we're falling through to case 2 if non-space; RFC2616-compliant
	// browsers will separate the request method (e.g. "GET" or "HEAD")
	// from the Request-URI with 1 space, although broken browsers
	// might not.
	if (*p == ' ')
	  break;
	delimit_state = 2;
      case 2:
	if (!delimit_start) 
	  delimit_start = p;
	if (*p == ' ' || *p == '?') {
	  return str (delimit_start, p - delimit_start);
	} else if (*p == '\n' || *p == '\r') {
	  delimit_status = HTTP_BAD_REQUEST;
	  in->rembytes (i);
	  return NULL;
	}
	break;
      default:
	delimit_status = HTTP_BAD_REQUEST;
	in->rembytes (i);
	return NULL;
      }
    }
    in->rembytes (i);
  }
  return (NULL);
}

ptr<ahttpcon>
ahttpcon_aspawn (str execpath, cbv::ptr postforkcb, ptr<axprt_unix> *ctlx)
{
  vec<str> v;
  v.push_back (execpath);
  int ctlfd, fd;
  ptr<ahttpcon> x;
  fd = ahttpcon_spawn (execpath, v, postforkcb, true, NULL, 
		       ctlx ? &ctlfd : NULL);
  if (fd < 0)
    return NULL;
  if (ctlx)
    *ctlx = axprt_unix::alloc (ctlfd, axprt_unix::defps);

  return ahttpcon::alloc (fd);
}

int
ahttpcon_aspawn (str execpath, const vec<str> &v, cbv::ptr pfcb,
		 int *ctlx)
{
  return ahttpcon_spawn (execpath, v, pfcb, true, NULL, ctlx);
}

int
ahttpcon_spawn (str path, const vec<str> &avs,
		cbv::ptr postforkcb, bool async, char *const *env,
		int *ctlx)
{
  ahttpcon_spawn_pid = -1;
  vec<const char *> av;

  for (const str *s = avs.base (), *e = avs.lim (); s < e; s++) 
    av.push_back (s->cstr ());
  av.push_back (NULL);

  int fds[2][2];
  for (int i = 0; i < 2; i++) {
    if (socketpair (AF_UNIX, SOCK_STREAM, 0, fds[i])) {
      warn ("socketpair: %m\n");
      return -1;
    }
    close_on_exec (fds[i][0]);
  }
  pid_t pid;
  if (async)
    pid = aspawn (path, av.base (), fds[0][1], fds[1][1], 2, postforkcb, env);
  else
    pid = spawn (path, av.base (), fds[0][1], fds[1][1], 2, postforkcb, env);
  ahttpcon_spawn_pid = pid;
  for (int i = 0; i < 2; i++)
    close (fds[i][1]);
  if (pid < 0) {
    for (int i = 0; i < 2; i++)
      close (fds[i][0]);
    return -1;
  }
  if (ctlx)
    *ctlx = fds[1][0];
  return fds[0][0];
}

void
ahttpcon::set_remote_ip ()
{
  if (sin) 
    remote_ip = inet_ntoa (sin->sin_addr);
}

static void
http_server_fd (cbv cb, int fd)
{
  make_async (fd);
  close_on_exec (fd);
  listen (fd, 20);
  fdcb (fd, selread, cb);
}

static void
http_accept (listencb_t lcb, int fd)
{
  sockaddr_in *sin = (sockaddr_in *) xmalloc (sizeof (sockaddr_in));
  socklen_t sinlen = sizeof (sockaddr_in);
  bzero (sin, sinlen);
  int nfd = accept (fd, reinterpret_cast<sockaddr *> (sin), &sinlen);
  if (nfd >= 0) {
    warn ("accepting connection from %s\n", inet_ntoa (sin->sin_addr));
    tcp_nodelay (nfd);
    ref<ahttpcon> x = ahttpcon::alloc (nfd, sin);
    (*lcb) (x);
  } else if (errno != EAGAIN) 
    warn ("accept: %m\n");
}

bool
http_server (listencb_t lcb, int port)
{
  int fd = inetsocket (SOCK_STREAM, port);
  if (fd < 0)
    return false;
  http_server_fd (wrap (http_accept, lcb, fd), fd);
  return true;
}

