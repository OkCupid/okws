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

#include "ahttp.h"
#include "httpconst.h"
#include <sys/types.h>
#include <sys/socket.h>

//
// hacked in here for now...
//
vec<suio *> recycled_suios;
vec<suiolite *> recycled_suiolites;
vec<suiolite *> recycled_suiolites_small;

syscall_stats_t *global_syscall_stats = NULL;
time_t global_ssd_last = 0;
int ahttpcon_spawn_pid;
int n_ahttpcon = 0;

void
ahttpcon::clone (ref<ahttpcon_clone> xc)
{
  assert (!xc->ateof ());
  sockaddr_in *sin_tmp;
  if (ok_send_sin && (sin_tmp = xc->get_sin ()))
    out->copy ((void *)sin_tmp, sizeof (*sin_tmp));
  sendfd (xc->takefd (), true, xc->get_close_fd_cb ());
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
ahttpcon::sendfd (int sfd, bool closeit, ptr<cbv_countdown_t> cb)
{
  fdsendq.push_back (fdtosend (sfd, closeit, cb));
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
    if (ss) ss->n_writev ++;
    ssize_t skip = writev (fd, iov, cnt);
    if (skip < 0 && errno != EAGAIN) {
      fail ();
      // still need to signal done sending....
      if (cb) (*cb) ();
      return;
    } else
      out->copyv (iov, cnt, max<ssize_t> (skip, 0));
  } else {
    out->copyv (iov, cnt, 0);
  }
  drained_cb = cb;
  output ();
}

void
ahttpcon::call_drained_cb ()
{
  cbv::ptr c = drained_cb;
  if (c) {
    drained_cb = NULL;
    (*c) ();
  }
}

void
ahttpcon::output ()
{
  if (fd < 0)
    return;

  ssize_t n = 0;
  int cnt = 0;
  do {
    while (!syncpts.empty () && out->iovno () >= syncpts.front ())
      syncpts.pop_front ();
    cnt = syncpts.empty () ? (size_t) -1 
      : int (syncpts.front () - out->iovno ());
  } while ((n = dowritev (cnt)) > 0);

  bool more_to_write = out->resid () || !fdsendq.empty ();
  if (n < 0) {
    fail ();
  } else if (more_to_write && !wcbset) {
    wcbset = true;
    fdcb (fd, selwrite, wrap (this, &ahttpcon::output));
  }
  else if (!more_to_write && wcbset) {
    wcbset = false;
    fdcb (fd, selwrite, NULL);
  }

  if (!out->resid () && drained_cb) {
    call_drained_cb ();
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
  if (!fdwait (fd, selwrite, &ztv)) {
    return 0;
  }

  if (cnt < 0)
    cnt = out->iovcnt ();
  if (cnt > UIO_MAXIOV)
    cnt = UIO_MAXIOV;

  // statistics collection
  if (ss) ss->n_writevfd ++ ;

  bool ignore_err = (out->resid () == 0);
  ssize_t n = writevfd (fd, out->iov (), cnt, fdsendq.front ().fd);

  //
  // no need to report an error if writevd returned -1 due to no
  // data to send aside from the FD
  //
  if (n < 0 && (!ignore_err || errno != EAGAIN)) {
    return errno == EAGAIN ? 0 : -1;
  }

  fdsendq.pop_front ();
  if (n >= 0)
    out->rembytes (n);
  return 1;
}


ahttpcon_clone::ahttpcon_clone (int f, sockaddr_in *s, size_t ml)
  : ahttpcon (f, s, ml), maxline (ml), ccb (NULL), 
  found (false), delimit_state (0), delimit_status (HTTP_OK),
  delimit_start (NULL), bytes_scanned (0), decloned (false),
  trickle_state (0), dcb (NULL)
{
  in->setpeek ();
}

ahttpcon_clone::~ahttpcon_clone ()
{
  if (dcb) {
    timecb_remove (dcb);
    dcb = NULL;
  }
  *destroyed_p = true;
}
      

ahttpcon::~ahttpcon ()
{ 
  // 
  // bookeeping for debug purposes.
  //
  --n_ahttpcon;

  destroyed = true;
  *destroyed_p = true;
  fail ();
  if (sin && sin_alloced) xfree (sin);
  recycle (in);
  recycle (out);
}

void
ahttpcon::fail (int s)
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
    else if (drained_cb) {
      call_drained_cb ();
    }
    fail2 (s);
  }
}

void
ahttpcon_listen::fail2 (int dummy)
{
  if (lcb) 
    (*lcb) (NULL);
}

void
ahttpcon_clone::fail2 (int s)
{
  if (ccb) {
    // we're being paranoid since the hold in fail2 should cover
    // us, but in case anything changes in the future, we should
    // assume that settint ccb = NULL causes this object to be
    // destroyed.
    clonecb_t::ptr c = ccb;
    ccb = NULL;
    (*c) (NULL, s);
  }
}

void
ahttpcon_clone::setccb (clonecb_t c)
{
  assert (!destroyed);
  if (enable_selread ()) 
    ccb = c;
  else
    fail2 (HTTP_BAD_REQUEST);
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
ahttpcon::disable_selread ()
{
  fdcb (fd, selread, NULL);
  rcbset = false;
}

void
ahttpcon_listen::setlcb (listencb_t c)
{
  if (enable_selread ()) {
    fd_accept_enabled = true;
    lcb = c;
  } else
    (*c) (NULL);
}

void
ahttpcon_listen::disable_fd_accept ()
{
  ahttpcon::disable_selread ();
  fd_accept_enabled = false;
}

void
ahttpcon_listen::enable_fd_accept ()
{
  ahttpcon::enable_selread ();
  fd_accept_enabled = true;
}

ssize_t
ahttpcon_listen::doread (int fd)
{
  if (!fd_accept_enabled) {
    errno = EAGAIN;
    return -1;
  }

  int tnfd = -1;
  assert (!in->resid ());
  ssize_t n = in->input (fd, &tnfd, ss);
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
 
  /*
   * not using the getpeername technique, since okd must send data
   * over the channel with the FD, so it may as well send the sockaddr
   * information
   *
   * this code to be deleted...
   *
   *
  else if ((n == 0 || n == -1) && tnfd >= 0 && !ok_send_sin) {
    socklen_t sinlen;
    sin2 = (sockaddr_in *) xmalloc (sizeof (sockaddr_in));
    if (getpeername (tnfd, (struct sockaddr *)sin2, &sinlen) != 0) {
      warn ("getpeername on socket %d failed: %m\n", tnfd);
      close (tnfd);
      tnfd = -1;
      xfree (sin2);
      sin2 = NULL;
    } else if (sinlen != sizeof (sockaddr_in)) {
      warn ("weird sockaddr_in returned from getpeername; wrong size\n");
    }
  }
  */

  if (tnfd >= 0) 
    if (lcb) {
      (*lcb) (New refcounted<ahttpcon> (tnfd, sin2, -1, -1, false));
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
  ssize_t s = in->input (fd, NULL, ss);
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

    if (errno == EMSGSIZE) {
      warn ("Too many fds (%d) in ahttpcon::input: %m\n", n_ahttpcon);
      too_many_fds ();
    } else if (errno != EAGAIN) {
      warn ("nfds=%d; Error in ahttpcon::input: %m\n", n_ahttpcon);
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

    warn << "Channel limit exceded ";
    if (remote_ip)
      warnx << "(" << remote_ip << ")";
    warnx << "\n";

    eof = true;
    disable_selread ();
    overflow_flag = true;
    n = 0;
  }

  recvd_bytes (n);
}

void 
ahttpcon::recvd_bytes (int n)
{
  if (rcb) 
    (*rcb) (n);
}

void
ahttpcon_clone::recvd_bytes (int n)
{
  if (decloned) {
    ahttpcon::recvd_bytes (n);
    return;
  }
  str s = delimit (n);
  if (s || delimit_status != HTTP_OK) {

    if (ccb) {
      (*ccb) (s, delimit_status);
      ccb = NULL;
    }

    // Tricky! Need to check decloned flag again, because it might
    // have been set from within (*ccb). If the header is more than
    // one packet, then we should not be turning off reads, since the
    // header needs to be parsed for logging purposes
    if (s && !decloned)
      end_read ();
    else {
      //
      // if it was an error, we still need to parse the rest of the
      // headers, and for that, we'll need to keep reading on this
      // ahttpcon_clone connection
      //
      // we should only parse the header here, and therefore should
      // read much less data
      recv_limit = ok_hdrsize_limit;
    }
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

void
ahttpcon_clone::declone ()
{
  decloned = true;
  in->setpeek (false);
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

  if (bytes_scanned > maxscan ()) {
    delimit_status = HTTP_NOT_FOUND;
    return NULL;
  }

  // at this point, the client is "trickling" out data --
  // 

  u_int to;
  switch (trickle_state) {
  case 0: 

    warn << "slow trickle client";
    if (remote_ip)
      warnx << ": " << remote_ip;
    warnx << "\n";

    to = ok_demux_timeout / 2;
    if (to) {
      dcb = delaycb (to, 0,
		     wrap (this, &ahttpcon_clone::trickle_cb, destroyed_p));
      disable_selread ();
      reset_delimit_state ();
      break;
    } else {
      trickle_state = 1;
      //fall through to next case in switch statement
    }
  case 1:

    warnx << "abandoning slow trickle client";
    if (remote_ip)
      warnx << ": " << remote_ip;
    warnx << "\n";

    delimit_status = HTTP_TIMEOUT;
    break;
  default:
    assert (false);
    break;
  }
  trickle_state ++;
  return (NULL);
}

void
ahttpcon_clone::trickle_cb (ptr<bool> destroyed_local)
{
  if (*destroyed_local) 
    return;
  dcb = NULL;
  enable_selread ();
}

int
ahttpcon::set_lowwat (int lev)
{
  return setsockopt (fd, SOL_SOCKET, SO_RCVLOWAT, (char *)&lev, sizeof (lev));
}

void
ahttpcon_clone::reset_delimit_state ()
{
  delimit_state = 0;
  bytes_scanned = 0;
}

ptr<ahttpcon>
ahttpcon_aspawn (str execpath, cbv::ptr postforkcb, ptr<axprt_unix> *ctlx,
		 char *const *env)
{
  vec<str> v;
  v.push_back (execpath);
  int ctlfd, fd;
  ptr<ahttpcon> x;
  fd = ahttpcon_spawn (execpath, v, postforkcb, true, env,
		       ctlx ? &ctlfd : NULL);
  if (fd < 0)
    return NULL;
  if (ctlx)
    *ctlx = axprt_unix::alloc (ctlfd, axprt_unix::defps);

  return ahttpcon::alloc (fd);
}

int
ahttpcon_aspawn (str execpath, const vec<str> &v, cbv::ptr pfcb,
		 int *ctlx, char *const *env)
{
  return ahttpcon_spawn (execpath, v, pfcb, true, env, ctlx);
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
  if (!sin && !ok_send_sin) {
    socklen_t sinlen;
    if (getpeername (fd, (struct sockaddr *)&sin3, &sinlen) != 0) {
      warn ("getpeername failed on socket %d: %m\n", fd);
    } else if (sinlen != sizeof (sockaddr_in)) {
      warn ("getpeername returned strange sockaddr, sized: %d\n", sinlen);
    } else {
      sin = &sin3;
    }
  }

  if (sin) 
    remote_ip = inet_ntoa (sin->sin_addr);
  else
    remote_ip = "0.0.0.0";
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
    //warn ("accepting connection from %s\n", inet_ntoa (sin->sin_addr));
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

void
recycle (suio *s)
{
  if (recycled_suios.size () < RECYCLE_LIMIT) {
    s->clear ();
    recycled_suios.push_back (s);
  } else {
    delete s;
  }
}

void
recycle (suiolite *s)
{
  if (s->getlen () == AHTTP_MAXLINE &&
      recycled_suiolites_small.size () < RECYCLE_LIMIT) {
    s->clear ();
    recycled_suiolites_small.push_back (s);
  } else if (s->getlen () == SUIOLITE_DEF_BUFLEN &&
	     recycled_suiolites.size () < RECYCLE_LIMIT) {
    s->clear ();
    recycled_suiolites.push_back (s);
  } else {
    delete s;
  }
}

suio *
suio_alloc ()
{
  if (recycled_suios.size ()) {
    return recycled_suios.pop_front ();
  } else {
    return New suio ();
  }
}

suiolite *
suiolite_alloc (int mb, cbv::ptr s)
{
  suiolite *ret;
  if (recycled_suiolites.size () && mb == SUIOLITE_DEF_BUFLEN) {
    ret = recycled_suiolites.pop_front ();
    ret->recycle (s);
  } else if (recycled_suiolites_small.size () && mb == AHTTP_MAXLINE) {
    ret = recycled_suiolites_small.pop_front ();
    ret->recycle (s);
  } else {
    ret = New suiolite (mb, s);
  }
  return ret;
}

void
ahttp_tab_t::reg (ahttpcon *a, ptr<bool> d)
{
  ahttp_tab_node_t *n = New ahttp_tab_node_t (a, d);
  q.insert_tail (n);
  nent++;
}

void
ahttp_tab_t::run ()
{
  dcb = NULL;
  ahttp_tab_node_t *n;

  bool flag = true;
  while ((n = q.first) && flag) {
    if (* n->_destroyed_p ) {
      unreg (n);
    } else if (int (timenow - n->_a->start) > int (ok_demux_timeout)) {
      warn << "XXX: removing deadbeat http connection\n"; //debug
      n->_a->timed_out ();
      unreg (n);
    } else {
      flag = false;
    }
  }
  sched ();
}

void
ahttp_tab_t::unreg (ahttp_tab_node_t *n)
{
  q.remove (n);
  delete n;
  nent--;
}

void
ahttp_tab_t::sched ()
{
  dcb = delaycb (interval, 0, wrap (this, &ahttp_tab_t::run));
}
