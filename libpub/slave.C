
/* $Id$ */

#include "pslave.h"
#include "stllike.h"
#include "axprtfd.h"

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
    ref<axprt_stream> x = axprt_stream::alloc (fd, OK_DEFPS);
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
    ref<axprt_stream> x = axprt_unix::alloc (fd, OK_DEFPS);
    (*cb) (x);
  } else if (errno != EAGAIN)
    warn ("accept in pub_accept_unix: %m\n");
}

static void
pub_server_fd (cbv cb, int fd)
{
  close_on_exec (fd);
  listen (fd, 20);
  fdcb (fd, selread, cb);
}

static bool
pub_slave_fd (pubserv_cb cb, int fd, pslave_status_t *s)
{
  assert (fd >= 0);
  if (!isunixsocket (fd))
    return false;
  if (s) *s = PSLAVE_SLAVE;
  ref<axprt_stream> x = axprt_stream::alloc (fd, OK_DEFPS);
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
pub_server (pubserv_cb cb, u_int port)
{
  int pubfd = inetsocket (SOCK_STREAM, port);
  if (pubfd < 0)
    return false;
  pub_server_fd (wrap (pub_accept, cb, pubfd), pubfd);
  return true;
}

int
pub_server (pubserv_cb cb, const str &s)
{
  int pubfd = unixsocket (s.cstr ());
  if (pubfd >= 0) {
    fchmod (pubfd, 0666);
    pub_server_fd (wrap (pub_accept_unix, cb, pubfd), pubfd);
  }
  return pubfd;
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
  str prog = find_program (p);
  if (!prog) {
    warn << "Cannot find executable: " << p << "\n";
    (*c) (false);
    return;
  }
  vec<str> argv2;
  if (n_add_socks > MAX_SOCKPAIRS) n_add_socks = MAX_SOCKPAIRS;
  for (u_int i = 0; i < n_add_socks; i++) {
    if (socketpair (AF_UNIX, SOCK_DGRAM, 0, sps[i])) {
      warn ("sockpair: %m\n");
      (*c) (false); // XXX -- we're leaking already opened sockets
      return;
    }
    close_on_exec (sps[i][0]);
    argv2.push_back ("-s");
    argv2.push_back (strbuf () << sps[i][1]);
  }

  if (n_add_socks) 
    argv = argv_combine (argv, argv2);

  argv[0] = prog;
  ptr<axprt_unix> ux = 
    axprt_unix_aspawnv (prog, argv, 0, 
			wrap (this, &helper_exec_t::setprivs));
  fd = ux->getfd ();
  x = ux;

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
  fd = unixsocket_connect (sockpath);
  if (fd < 0) {
    hwarn (strerror (errno));
    (*c) (false);
    return;
  }
  close_on_exec (fd);
  x = axprt_unix::alloc (fd, OK_DEFPS);
  mkclnt ();
  ping (c);
}

void
helper_fd_t::launch (cbb cb)
{
  if (fd < 0 || !isunixsocket (fd)) {
    (*cb) (false);
  }
  x = axprt_stream::alloc (fd, OK_DEFPS);
  mkclnt ();
  ping (cb);
}

void
helper_inet_t::launch (cbb c)
{
  if (!hostname || hostname == "-")
    hostname = "localhost";
  tcpconnect (hostname, port, wrap (this, &helper_inet_t::launch_cb, c));
}

void
helper_inet_t::launch_cb (cbb c, int f)
{
  bool ret = true;
  if (f < 0) {
    hwarn ("could not connect to host"); 
    ret = false;
  } else {
    fd = f;
    close_on_exec (fd);
    if (!(x = axprt_stream::alloc (fd, OK_DEFPS)) || !mkclnt ())
      ret = false;
  }
  if (!ret)
    (*c) (ret);
  else
    ping (c);
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
    (*c) (false);
    return;
  }
  if (err) {
    hwarn (strbuf ("ping failed: " ) << err);
    (*c) (false);
    status = HLP_STATUS_ERR;
    d_retry ();
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
  if (*df) {
    if (cb)
      (*cb) (false);
    return;
  }
  if (!b) {
    if (!(opts & HLP_OPT_CNCT1) && status != HLP_STATUS_RETRY)
      d_retry ();
    status = HLP_STATUS_ERR;
  } else {
    assert (clnt);
    clnt->seteofcb (wrap (this, &helper_t::eofcb));
    status = HLP_STATUS_OK;
    process_queue ();
  }
  if (cb)
    (*cb) (b);
  call_status_cb ();
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
helper_t::hwarn (const str &s) const
{
  warn << getname () << ": " << s << "\n";
}

void
helper_t::eofcb ()
{
  x = NULL;
  clnt = NULL;
  fd = -1;
  hwarn ("connection dropped");
  if ((opts & HLP_OPT_NORETRY))
    status = HLP_STATUS_HOSED;
  else if (can_retry ())
    retry ();
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
    delaycb (rdelay, 0, wrap (this, &helper_t::retry));
}

void
helper_t::retry ()
{
  hwarn ("attempting to reconnect");
  status = HLP_STATUS_RETRY;
  retries = 0;
  connect (wrap (this, &helper_t::retried));
}

void
helper_t::retried (bool b)
{
  if (!b) {
    if (++retries > max_retries) {
      status = HLP_STATUS_HOSED;
    } else {
      d_retry ();
    }
    hwarn ("reconnect attempt failed");
  } else {
    hwarn ("reconnect succeded");
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

