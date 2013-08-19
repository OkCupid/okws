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

#include "pslave.h"
#include "stllike.h"
#include "okdbg.h"

static int
_pub_accept (int pubfd, sockaddr_in *sin)
{
  socklen_t sinlen = sizeof (sockaddr_in);
  bzero (sin, sinlen);
  return accept (pubfd, reinterpret_cast<sockaddr *> (sin), &sinlen);
}
  

static void
pub_accept (pubserv_cb cb, int pubfd)
{
  sockaddr_in sin;
  int fd = _pub_accept (pubfd, &sin);
  if (fd >= 0) {
    warn ("accepting connection from %s\n", inet_ntoa (sin.sin_addr));
    tcp_nodelay (fd);
    ref<axprt_stream> x = axprt_stream::alloc (fd, ok_axprt_ps);
    (*cb) (x);
  } else if (errno != EAGAIN)
    warn ("accept in pub_accept: %m\n");
}

static void
pub_accept_unix (pubserv_cb cb, int pubfd)
{
  sockaddr_in sin;
  int fd = _pub_accept (pubfd, &sin);
  if (fd >= 0) {
    ref<axprt_stream> x = axprt_unix::alloc (fd, ok_axprt_ps);
    (*cb) (x);
  } else if (errno != EAGAIN)
    warn ("accept in pub_accept_unix: %m\n");
}

static void
pub_server_fd (cbv cb, int fd)
{
  close_on_exec (fd);
  listen (fd, 200);
  fdcb (fd, selread, cb);
}

static bool
pub_slave_fd (pubserv_cb cb, int fd, pslave_status_t *s)
{
  assert (fd >= 0);
  if (!isunixsocket (fd))
    return false;
  if (s) *s = PSLAVE_SLAVE;
  ref<axprt_stream> x = axprt_stream::alloc (fd, ok_axprt_ps);
  (*cb) (x);
  return true;
}

pslave_status_t
pub_slave (pubserv_cb cb, u_int port, pslave_status_t *s)
{
  if (pub_slave_fd (cb, 0, s))
    return PSLAVE_SLAVE;
  else if (port > 0) 
    return (pub_server (cb, port) ? PSLAVE_LISTEN : PSLAVE_ERR);
  else 
    return PSLAVE_ERR;
}

bool
pub_server (pubserv_cb cb, u_int port, u_int32_t addr, int *outfd)
{

  if (addr == INADDR_ANY) {
    struct in_addr ia;
    const char *c = getenv ("PUB_SERVER_ADDR");
    if (c && inet_aton (c, &ia) > 0) {
      warn << "binding to PUB_SERVER_ADDR=" << c << "\n";
      addr = ia.s_addr;
    }
  }
  int pubfd = inetsocket (SOCK_STREAM, port, addr);
  if (outfd) { *outfd = pubfd; }
  if (pubfd < 0)
    return false;
  pub_server_fd (wrap (pub_accept, cb, pubfd), pubfd);
  return true;
}

int
pub_server (pubserv_cb cb, const str &s, int *outfd)
{
  int pubfd = unixsocket (s.cstr ());
  if (outfd) { *outfd = pubfd; }
  if (pubfd >= 0) {
    fchmod (pubfd, 0666);
    pub_server_fd (wrap (pub_accept_unix, cb, pubfd), pubfd);
  }
  return pubfd;
}

int
pub_server_clientfd (pubserv_cb cb, int pubfd) {
    pub_server_fd(wrap (pub_accept, cb, pubfd), pubfd);
    return true;
}

void
helper_exec_t::setprivs ()
{
  if ((uid || gid) && setgroups (0, NULL))
    fatal ("could not void grouplist: %m\n");
  if (gid && setgid (*gid))
    warn ("could not setgid (%d): %m\n", *gid);
  if (uid && setuid (*uid))
    warn ("could not setuid (%d): %m\n", *uid);
}

static vec<str> 
argv_combine (const vec<str> a1, const vec<str> a2)
{
  vec<str> r;
  r.push_back (a1[0]);
  for (u_int i = 0; i < a2.size (); i++) {
    r.push_back (a2[i]);
  }
  for (u_int i = 1; i < a1.size (); i++) {
    r.push_back (a1[i]);
  }
  return r;
}

#define MAX_SOCKPAIRS 16
void
helper_exec_t::launch (cbb c)
{
  str p = argv[0];
  int sps[MAX_SOCKPAIRS][2];
  str prog = find_program (p.cstr());
  if (!prog) {
    warn << "Cannot find executable: " << p << "\n";
    (*c) (false);
    return;
  }
  vec<str> argv2;
  if (n_add_socks > MAX_SOCKPAIRS) n_add_socks = MAX_SOCKPAIRS;
  for (u_int i = 0; i < n_add_socks; i++) {
    if (socketpair (AF_UNIX, SOCK_STREAM, 0, sps[i])) {
      warn ("sockpair: %m\n");
      (*c) (false); // XXX -- we're leaking already opened sockets
      return;
    }
    close_on_exec (sps[i][0]);

    if (OKDBG2(OKLD_FD_PASSING)) {
      strbuf b;
      b << "for " << prog << ": socket: " << sps[i][0] 
	<< " <-> " << sps[i][1];
      okdbg_warn (CHATTER, b);
    }

    //
    // tell the child about the new sockets FDs on the command
    // line (with the -s<sockfd> command line flags)
    //
    argv2.push_back (_command_line_flag);
    argv2.push_back (strbuf () << sps[i][1]);
  }

  if (n_add_socks) 
    argv = argv_combine (argv, argv2);

  argv[0] = prog;
  ptr<axprt_unix> ux = 
    axprt_unix_aspawnv (prog, argv, ok_axprt_ps, 
			wrap (this, &helper_exec_t::setprivs),
			_env ? *_env : static_cast<char *const *> (NULL));
  _pid = axprt_unix_spawn_pid;
  fd = ux->getfd ();
  x = ux;
  _x_unix = ux;

  for (u_int i = 0; i < n_add_socks; i++) {
    close (sps[i][1]);
    socks.push_back (sps[i][0]);
  }

  mkclnt ();
  ping (c);
}

void
helper_unix_t::launch (cbb c)
{
  fd = unixsocket_connect (sockpath.cstr());
  if (fd < 0) {
    hwarn (strerror (errno));
    (*c) (false);
    return;
  }
  close_on_exec (fd);
  x = axprt_unix::alloc (fd, ok_axprt_ps);
  mkclnt ();
  ping (c);
}

void
helper_fd_t::launch (cbb cb)
{
  if (fd < 0 || !isunixsocket (fd)) {
    (*cb) (false);
  }
  x = axprt_stream::alloc (fd, ok_axprt_ps);
  mkclnt ();
  ping (cb);
}

void
helper_inet_t::launch (cbb c)
{
  if (!hostname || hostname == "-")
    hostname = "localhost";
  tcpconnect (hostname, port, wrap (this, &helper_inet_t::launch_cb, c,
				    destroyed));
}

void
helper_inet_t::launch_cb (cbb c, ptr<bool> df, int f)
{
  if (*df) {
    if (OKDBG2 (HLP_STATUS))
      warn ("helper object destroyed before launch_cb returned\n");
    (*c) (false);
    return;
  }

  bool ret = true;
  if (f < 0) {
    hwarn ("could not connect to host"); 
    ret = false;
  } else {
    fd = f;
    close_on_exec (fd);
    if (!(x = axprt_stream::alloc (fd, ok_axprt_ps)) || !mkclnt ())
      ret = false;
  }
  if (!ret)
    (*c) (ret);
  else
    ping (c);
}

void
helper_t::call_connect_cb(connect_params_t cp, bool success) {
    if (!success) {
        (*cp.cb)(RPC_PROCUNAVAIL);
    } else 
        docall(cp.procno, cp.in, cp.out, cp.cb, cp.duration);
}

void
helper_t::call (u_int32_t procno, const void *in, void *out, aclnt_cb cb,
                time_t duration)
{
    if (status != HLP_STATUS_OK || calls >= max_calls) {
        if ((opts & HLP_OPT_QUEUE) && (status != HLP_STATUS_HOSED) && 
            queue.size () < max_qlen) {
            queue.push_back (New queued_call_t (procno, in, out, cb, duration));
            return;
        } else if (opts & HLP_OPT_CTONDMD && status != HLP_STATUS_HOSED &&
                   status != HLP_STATUS_RETRY) {
            connect_params_t cp(procno, in, out, cb, duration);
            connect( wrap(this, &helper_t::call_connect_cb, cp) );
            return;
        } else {
            (*cb) (RPC_PROCUNAVAIL);
            return;
        }
    }
    docall (procno, in, out, cb, duration);
}

void
helper_t::docall (u_int32_t procno, const void *in, void *out, aclnt_cb cb,
		  time_t duration)
{
  calls++;
  assert (clnt);
  if (duration)
    clnt->timedcall (duration, procno, in, out, 
		     wrap (this, &helper_t::didcall, cb));
  else
    clnt->call (procno, in, out, 
		wrap (this, &helper_t::didcall, cb));
}

void
helper_t::didcall (aclnt_cb cb, clnt_stat stat)
{
  calls--;
  process_queue ();
  (*cb) (stat);
}

void
helper_t::ping (cbb cb)
{
  if (opts & HLP_OPT_PING)
    clnt->call (0, NULL, NULL, wrap (this, &helper_t::ping_cb, cb, destroyed));
  else
    (*cb) (true);
}

void
helper_t::ping_cb (cbb c, ptr<bool> df, clnt_stat err)
{
  if (*df) {
    if (OKDBG2(HLP_STATUS))
	warn ("helper object destroyed before ping returned\n");
    (*c) (false);
    return;
  }

  //
  // callback c will bring us to helper_t::connected;
  // thus, if the ping failed, no need to call d_retry(), since
  // helper_t::connected will do that for us.  don't want to 
  // call it twice, after all.
  //
  if (err) {
    hwarn (strbuf ("ping failed: " ) << err);
    (*c) (false);
    status_change (HLP_STATUS_ERR);
  } else 
    (*c) (true);
}

void
helper_t::connect (cbb::ptr cb)
{
  launch (wrap (this, &helper_t::connected, cb, destroyed));
}

void
helper_t::connected (cbb::ptr cb, ptr<bool> df, bool b)
{
  // object was destroyed before we got done with the connection
  if (*df) {
    if (OKDBG2(HLP_STATUS))
      warn ("helper object destroyed before connect returned\n");
    if (cb) 
      (*cb) (false);
    return;
  }

  // the connection failed!
  if (!b) {

    // for the first failed connection, check to see if we should
    // enter the retry loop.  we know if it's the first failed connection
    // because if it is, we will **not** be in state HLP_STATUS_RETRY;
    // we know to enter the retry loop if the option HLP_OPT_CNCT1
    // is not set.
    if (!(opts & HLP_OPT_CNCT1) && status != HLP_STATUS_RETRY)
      d_retry ();

    // Even if we're going to retry, we set to status HLP_STATUS_ERR
    // temporarily.
    status_change (HLP_STATUS_ERR);

    // the cb is fired after the first connection attempt; whoever
    // called connect will always get an answer after the first attempt,
    // even if there are more restarts scheduled.
    if (cb)
      (*cb) (b);

    return;
  }

  // successfull connect
  assert (clnt);
  clnt->seteofcb (wrap (this, &helper_t::eofcb, destroyed));

  connect_success (cb);
}

void
helper_t::connect_success (cbb::ptr cb)
{
  status = HLP_STATUS_OK;
  ptr<bool> df = destroyed;

  // keep in mind all callbacks might potentially delete us!
  // therefore, we need to be extra careful with accessing
  // members after calling a callback.
  if (cb)
    (*cb) (true);

  if (!*df) {
    if (status == HLP_STATUS_OK)
      call_status_cb ();
  }

  // check df again! its value might have changed after
  // call_status_cb!
  if (!*df) {
    process_queue ();
  }
}

void
helper_t::process_queue ()
{
  while (calls < max_calls && queue.size () && status == HLP_STATUS_OK) {
    queued_call_t *qc = queue.pop_front ();
    qc->call (this);
    delete qc;
  }
}

void
helper_base_t::hwarn (const str &s) const
{
  warn << getname () << ": " << s << "\n";
}

void
helper_t::kill_aclnt ()
{
  if (clnt) { clnt->seteofcb (NULL); }
  kill_aclnt_priv();
}

void
helper_t::kill_aclnt_priv()
{
  x = NULL;
  clnt = NULL;
  fd = -1;

  // always warn when a connection was dropped
  hwarn ("connection dropped");

  if ((opts & HLP_OPT_NORETRY) || !can_retry ())
    status_change (HLP_STATUS_HOSED);
  else 
    retry (destroyed);
}

void
helper_t::eofcb (ptr<bool> df)
{
  if (*df) {
    if (OKDBG2 (HLP_STATUS))
      warn ("helper object destroyed before eofcb\n");
    return;
  }
  kill_aclnt_priv();
}

void
helper_t::status_change (hlp_status_t new_status)
{
  hlp_status_t old_status = status;
  status = new_status;

  if (new_status == HLP_STATUS_HOSED) 
    hwarn ("giving up connection, and changing to failed state");

  if (new_status != old_status) 
    call_status_cb ();
}

helper_t::~helper_t ()
{
  *destroyed = true;
  x = NULL;
  clnt = NULL;
}

void
helper_t::d_retry ()
{
  if (can_retry () && !(opts & HLP_OPT_NORETRY)) 
    delaycb (rdelay, 0, wrap (this, &helper_t::retry, destroyed));
}

void
helper_t::retry (ptr<bool> df)
{
  if (*df) {
    if (OKDBG2 (HLP_STATUS))
      warn ("helper object destroyed while attempting to retry\n");
    return;
  }
  if (OKDBG2 (HLP_STATUS))
    hwarn ("attempting to reconnect");

  status_change (HLP_STATUS_RETRY);

  // calling status change might have deleted us, so let's be extra careful!
  if (!*df) 
    connect (wrap (this, &helper_t::retried, destroyed));
}

void
helper_t::retried (ptr<bool> df, bool b)
{
  if (*df) {
    if (OKDBG2 (HLP_STATUS))
      warn ("helper object destroyed while attempting retry connection\n");
    return;
  }

  if (!b) {

    if (++retries > max_retries) {
      status_change (HLP_STATUS_HOSED);
    } else {
      if (OKDBG2 (HLP_STATUS))
	hwarn ("reconnect attempt failed");
      d_retry ();
    }

  } else {
    
    // always print a success out!
    hwarn (strbuf ("reconnect succeded (retries=%d)", retries));
    retries = 0;
  }
}

void
helper_exec_t::kill (cbv cb, ptr<okauthtok_t> t, oksig_t s)
{
  if (clnt) {
    ok_killsig_t ks;
    ks.authtok = *t;
    ks.sig = s;
    clnt->seteofcb (cb);
    clnt->call (HELPER_KILL, &ks, NULL, aclnt_cb_null);
  } else {
    (*cb) ();
  }
}

str
status2str (hlp_status_t st)
{
#define D(x) case x: return #x;
  switch (st) {
    D(HLP_STATUS_NONE);
    D(HLP_STATUS_OK);
    D(HLP_STATUS_CONNECTING);
    D(HLP_STATUS_ERR);
    D(HLP_STATUS_RETRY);
    D(HLP_STATUS_HOSED);
    D(HLP_STATUS_DENIED);
  default:
    return "HLP_STATUS_UNKNOWN";
  }
#undef D
}

//-----------------------------------------------------------------------

bool
helper_exec_t::make_pidfile (const str &f)
{
  bool ret = false;
  if (_pid == 0) {
    warn << "No PID for process given!\n";
    errno = EINVAL;
  } else {
    strbuf b;
    b << _pid;
    str s = b;
    if ((ret = str2file (f, s, 0644))) {
      _pidfile = f;
    }
  }
  return ret;
}

//-----------------------------------------------------------------------

bool 
helper_exec_t::remove_pidfile ()
{
  bool ret = true;
  if (_pidfile) {
    int rc = unlink (_pidfile.cstr ());
    ret = (rc == 0);
  }
  return ret;
}

//-----------------------------------------------------------------------

bool 
helper_inet_t::operator== (const helper_inet_t &i) const
{ return i.hostname == hostname && i.port == port; }

//-----------------------------------------------------------------------

bool 
helper_inet_t::operator!= (const helper_inet_t &i) const 
{ return !(*this == i); }

//-----------------------------------------------------------------------

