
/* $Id$ */

#include <sys/types.h>
#include <unistd.h>

#include "okd.h"
#include "parseopt.h"
#include "sfsmisc.h"
#include "okprot.h"
#include "ahutil.h"
#include "pub.h"
#include "xpub.h"
#include "pubutil.h"
#include "axprtfd.h"

static void srepub (ptr<ok_repub_t> rpb, okch_t *ch) { ch->repub (rpb); }
static void srelaunch (ptr<ok_res_t> res, okch_t *ch) { ch->kill (); }

void
okd_t::set_signals ()
{
  sigcb (SIGTERM, wrap (this, &okd_t::shutdown, SIGTERM));
  sigcb (SIGINT,  wrap (this, &okd_t::shutdown, SIGINT));
}

okd_t::~okd_t ()
{
  if (logd) delete logd;
  if (pubd) delete pubd;
  delete pprox;
}

void
okd_t::got_pubd_unix (vec<str> s, str loc, bool *errp)
{
  str name;
  if (s.size () != 2 || access (s[1], R_OK) != 0) {
    warn << loc << ": usage: PubdUnix <socketpath>\n";
    *errp = true;
  } else if (!is_safe (s[1])) {
    warn << loc << ": Pubd socket path (" << s[1]
	 << ") contains unsafe substrings\n";
    *errp = true;
  } else {
    pubd = New helper_unix_t (pub_program_1, s[1]);
  }
}

void
okd_t::got_pubd_exec (vec<str> s, str loc, bool *errp)
{
  if (s.size () <= 1) {
    warn << loc << ": usage: PubdExecPath <path-to-pubd>\n";
    *errp = true;
    return;
  } else if (!is_safe (s[1])) {
    warn << loc << ": pubd exec path (" << s[1] 
	 << ") contains unsafe substrings\n";
    *errp = true;
    return;
  }
  str prog = okws_exec (s[1]);
  str err = can_exec (prog);
  if (err) {
    warn << loc << ": cannot open pubd: " << err << "\n";
    *errp = true;
  } else {
    s.pop_front ();
    s[0] = prog;
    pubd = New helper_exec_t (pub_program_1, s);
  }
}

void
okd_t::got_pubd_inet (vec<str> s, str loc, bool *errp)
{
  u_int port = ok_pubd_port;
  str name = "localhost";
  if (s.size () == 2) {
    if (!convertint (s[1], &port))
      name = s[1];
  } else if (s.size () != 3 || !convertint (s[2], &port) || !(name = s[1])) {
    warn << loc << ": usage: PubdInet <hostname> <port>\n";
    *errp = true;
    return;
  }
  pubd = New helper_inet_t (pub_program_1, name, port);
}

void
okd_t::launch_pubd ()
{
  if (!pubd) 
    pubd = New helper_exec_t (pub_program_1, "pubd");
  pubd->connect (wrap (this, &okd_t::launch_pubd_cb));
}

void
okd_t::launch_pubd_cb (bool rc)
{
  if (!rc) {
    warn << "launch of pub daemon (pubd) failed.\n";
    exit (1);
  }
  if (!--launches)
    launch2 ();
}

void
okd_t::launch_logd ()
{
  assert (logfd > 0);
  logd = New fast_log_t (logfd, logfmt);
  logd->connect (wrap (this, &okd_t::launch_logd_cb));
}

void
okd_t::launch_logd_cb (bool rc)
{
  if (!rc) {
    warn << "launch of log daemon (oklogd) failed.\n";
    exit (1);
  }
  if (!--launches)
    launch2 ();
}

void
okd_t::got_alias (vec<str> s, str loc, bool *errp)
{
  if (s.size () != 3) {
    warn << loc << ": usage: Alias <to-URI> <from-URI>\n";
    *errp = true;
    return;
  }
  aliases.insert (s[2], s[1]);
}

void
okd_t::open_mgr_socket ()
{
  // XXX - not secure; adversary can mount DOS-attacks over this port,
  // assuming he has a machine behind the firewall. better to either 
  // authenticate or to have the manager reachable only via a local
  // unix socket
  if (!pub_server (wrap (this, &okd_t::newmgrsrv), ok_mgr_port)) 
    fatal << "Cannot open management port (" << ok_mgr_port << ")\n";
}

void
okd_t::newmgrsrv (ptr<axprt_stream> x)
{
  vNew okd_mgrsrv_t (x, this);
}

void
okd_t::parseconfig ()
{
  const str &cf = configfile;
  warn << "using config file: " << cf << "\n";
  parseargs pa (cf);
  bool errors = false;

  int line;
  vec<str> av;

  str un, gn;
  conftab ct;
  ct.add ("BindAddr", wrap (static_cast<ok_base_t *> (this), 
			    &ok_base_t::got_bindaddr))
    .add ("Alias", wrap (this, &okd_t::got_alias))
    .add ("JailDir", wrap (got_dir, &jaildir))
    .add ("HostName", &hostname)
    .add ("TopDir", &topdir)

    .add ("MaxConQueueSize", &ok_con_queue_max, OK_QMIN, OK_QMAX)
    .add ("OkMgrPort", &ok_mgr_port, OK_PORT_MIN, OK_PORT_MAX)
    .add ("ListenQueueSize", &ok_listen_queue_max, OK_QMIN, OK_QMAX)
    .add ("ChannelLimit", &ok_reqsize_limit, OK_RQSZLMT_MIN, OK_RQSZLMT_MAX)

    .add ("PubdUnix", wrap (this, &okd_t::got_pubd_unix))
    .add ("PubdInet", wrap (this, &okd_t::got_pubd_inet))
    .add ("PubdExecPath", wrap (this, &okd_t::got_pubd_exec))

    .add ("ClientTimeout", &ok_clnt_timeout, 1, 400)

    .add ("OkdName", &reported_name)
    .add ("OkdVersion", &version)
    .add ("OkdUser", &un)
    .add ("OkdGroup", &gn)

    .ignore ("Service")
    .ignore ("CrashSamplingInterval")
    .ignore ("MaxCrahsedProcesses")
    .ignore ("ServiceLowUid")
    .ignore ("ServiceHighUid")
    .ignore ("ServiceGroup")
    .ignore ("OkdExecPath")
    .ignore ("OklogdExecPath")
    .ignore ("LogDir")
    .ignore ("AccessLog")
    .ignore ("ErrorLog")
    .ignore ("AccessLogFmt")
    .ignore ("OklogdUser")
    .ignore ("OklogdGroup")
    .ignore ("LogTick")
    .ignore ("LogPeriod")
    .ignore ("CoreDumpDir")
    .ignore ("SocketDir")
    .ignore ("ServiceBin")

    .ignore ("Gzip")
    .ignore ("GzipLevel")
    .ignore ("GzipSmallStrLen")
    .ignore ("GzipCacheMin")
    .ignore ("GzipCacheMax")
    .ignore ("GzipCacheSize")
    .ignore ("GzipMemLevel")
    .ignore ("UnsafeMode");


  while (pa.getline (&av, &line)) {
    if (!ct.match (av, cf, line, &errors)) {
      warn << cf << ":" << line << ": unknown config parameter\n";
      errors = true;
    }
  }
  if (un) okd_usr = ok_usr_t (un);
  if (gn) okd_grp = ok_grp_t (gn);

  if (!hostname)
    hostname = myname ();
  if (errors)
    exit (1);
}

void
okd_t::strip_privileges ()
{
  if (!uid) {
    if (setgid (okd_grp.getid ()) != 0) 
      fatal << "could not setgid for " << okd_grp.getname () << "\n";
    if (setuid (okd_usr.getid ()) != 0)
      fatal << "could not setuid for " << okd_usr.getname () << "\n";
    if (!chroot ())
      fatal << "startup aborted due to failed chroot call\n";
  }
}

void
okd_t::sclone (ref<ahttpcon_clone> x, str s, int status)
{
  if (status != HTTP_OK) {
    error (x, status);
  } else if (!s) {
    error (x, HTTP_BAD_REQUEST);
  } else {
    str *s2 = aliases[s];
    if (!s2) s2 = &s;
    okch_t *c = servtab[*s2];
    if (!c)
      error (x, HTTP_NOT_FOUND, *s2);
    else {
      c->clone (x);
    }
  }
}

void
okd_t::newserv (int fd)
{
  sockaddr_in *sin = (sockaddr_in *) xmalloc (sizeof (sockaddr_in));
  socklen_t sinlen = sizeof (sockaddr_in);
  bzero (sin, sinlen);
  int nfd = accept (fd, (sockaddr *) sin, &sinlen);
  if (nfd >= 0) {
    close_on_exec (nfd);
    tcp_nodelay (nfd);
    ref<ahttpcon_clone> x = ahttpcon_clone::alloc (nfd, sin);
    warn ("accepted connection from %s\n", x->get_remote_ip ().cstr ());
    x->setccb (wrap (this, &okd_t::sclone, x));
  }
  else if (errno != EAGAIN)
    warn ("accept: %m\n");
}


void
okd_t::launch3 ()
{
  int fd = inetsocket (SOCK_STREAM, listenport, listenaddr);
  listenfd = fd;
  if (fd < 0)
    fatal ("could not bind TCP port %d: %m\n", listenport);
  close_on_exec (fd);
  listen (fd, ok_listen_queue_max);
  strip_privileges ();
  warn << "listening on " << listenaddr_str << ":" << listenport << "\n";
  fdcb (fd, selread, wrap (this, &okd_t::newserv, fd));
}

void
okd_t::stop_listening ()
{
  fdcb (listenfd, selread, NULL);
  close (listenfd);
}

static void
usage ()
{
  warnx << "usage: okd [-D <dbg-file>] -l <logfd> -f <configfile>\n";
  exit (1);
}

static void
main2 (str cf, int logfd)
{
  sfsconst_init ();
  if (!cf)
    cf = sfsconst_etcfile_required ("okd_config");

  warn ("version %s, pid %d\n", VERSION, int (getpid ()));
  okd_t *okd = New okd_t (cf, logfd, 0);
  okd->set_signals ();
  okd->launch ();
}

int
main (int argc, char *argv[])
{
  str cf;
  int logfd = -1;
  setprogname (argv[0]);
  str debug_stallfile;

  int ch;
  while ((ch = getopt (argc, argv, "f:l:D:")) != -1)
    switch (ch) {
    case 'D':
      debug_stallfile = optarg;
      break;
    case 'f':
      if (cf)
	usage ();
      cf = optarg;
      break;
    case 'l':
      if (!convertint (optarg, &logfd))
	usage ();
      break;
    case '?':
    default:
      usage ();
    }

  argc -= optind;
  argv += optind;

  if (argc > 1)
    usage ();
  if (logfd < 0 || !isunixsocket (logfd)) {
    warn << "no log FD passed to okd or the given FD is not a socket\n";
    warn << "check that okd was launched by okld\n";
    exit (1);
  }

  // for debugging, we'll stall until the given file is touched.
  if (debug_stallfile) {
    stall (debug_stallfile, wrap (main2, cf, logfd));
  } else {
    main2 (cf, logfd);
  }
  amain ();
}

void
okd_t::launch ()
{
  parseconfig ();
  check_runas ();
  open_mgr_socket ();

  launches = 2;
  launch_pubd ();
  launch_logd ();
}

void
okd_t::launch2 ()
{
  okldx = fdsource_t<okws_fd_t>::alloc (okldfd, wrap (this, &okd_t::gotfd));
  assert (okldx);
  launch3 ();
}

void
okd_t::gotfd (int fd, ptr<okws_fd_t> desc)
{
  if (fd < 0) {
    shutdown (0);
    return;
  }

  assert (fd >= 0 && desc);

  switch (desc->fdtyp) {
  case OKWS_SVC_X:
  case OKWS_SVC_CTL_X:
    got_chld_fd (fd, desc);
    break;
  default:
    warn << "unknown FD type received from okld\n";
    break;
  }
  return;
}

void
okd_t::got_chld_fd (int fd, ptr<okws_fd_t> desc)
{
  okch_t *ch;
  switch (desc->fdtyp) {
  case OKWS_SVC_X:
    if (!(ch = servtab[desc->x->name])) {
      warn << "received service FDs out of order!\n";
      close (fd);
    }
    ch->got_new_x_fd (fd, desc->x->pid);
    break;
  case OKWS_SVC_CTL_X:
    if (!(ch = servtab[desc->ctlx->name])) {
      ch = New okch_t (this, desc->ctlx->name);
    }
    ch->got_new_ctlx_fd (fd, desc->ctlx->pid);
    break;
  default:
    assert (false);
  }
}

void
okd_t::check_runas ()
{
  if (uid)
    return;
  if (!okd_usr)
    fatal << configfile 
	  << ": please specify a valid username for \"OkdUser\"\n";
  if (!okd_grp)
    fatal << configfile 
	  << ": please specify a valid group for \"OkdGroup\"\n";
}

void
okd_t::repub (const xpub_fnset_t &f, okrescb cb)
{
  ptr<ok_repub_t> rpb = New refcounted<ok_repub_t> (f, cb);
  pubd->call (PUB_FILES, &f, &rpb->xpr, wrap (this, &okd_t::repub_cb1, rpb));
}


void
okd_t::repub_cb1 (ptr<ok_repub_t> rpb, clnt_stat err)
{
  if (err) {
    *rpb->res << (strbuf ("RPC error in repubbing: ") << err);
    return;
  }
  rpb->res->pub_res_t::add (rpb->xpr.status);
  pprox->cache (rpb->xpr.set);
  rpb->set_new_fnset ();
  servtab.traverse (wrap (srepub, rpb));
}

void
ok_repub_t::set_new_fnset ()
{
  u_int lim = xpr.set.bindings.size ();
  new_fnset.files.setsize (lim);
  for (u_int i = 0; i < lim; i++)
    new_fnset.files[i] = xpr.set.bindings[i].fn;
  new_fnset.rebind = true;
}

void
okd_t::relaunch (const ok_progs_t &x, okrescb cb)
{
  ptr<ok_res_t> res = New refcounted<ok_res_t> ();
  if (x.typ == OK_SET_ALL) {
    servtab.traverse (wrap (srelaunch, res));
  } else if (x.typ == OK_SET_SOME) {
    u_int lim = x.progs->size ();
    for (u_int j = 0; j < lim; j++) {
      str prog = (*x.progs)[j];
      okch_t *o = servtab[prog];
      if (!o) *res << (strbuf ("cannot find program: ") << prog);
      else o->kill ();
    }
  }
  (*cb) (res);
}

