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

common_404_t::common_404_t ()
{
  tab.insert ("/favicon.ico");
}

common_404_t common_404;

void
okd_t::abort ()
{
  panic ("could ABORT signal\n");
}

void
okd_t::set_signals ()
{
  sigcb (SIGTERM, wrap (this, &okd_t::shutdown, SIGTERM));
  sigcb (SIGINT,  wrap (this, &okd_t::shutdown, SIGINT));
  sigcb (SIGABRT, wrap (this, &okd_t::abort));
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
okd_t::got_err_doc (vec<str> s, str loc, bool *errp)
{
  int status;
  if (s.size () != 3 || !convertint (s[1], &status)) {
    warn << loc << ": usage: ErrorDoc <status> <pub-path>\n";
    *errp = true;
  } else if (errdocs[status]) {
    warn << loc << ": duplicate ErrorDoc ID: " << status << "\n";
    *errp = true;
  } else {
    errdocs.insert (New errdoc_t (status, s[2]));
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
  req_err_docs ();
}

static void
errdoc_load_fnset (xpub_fnset_t *f, int *p, errdoc_t *e)
{
  f->files[(*p)++] = e->fn;
}

void
okd_t::req_err_docs ()
{
  if (errdocs.size () == 0) {
    req_err_docs_4 (true);
  } else {
    pprox->r_config (wrap (this, &okd_t::req_err_docs_2), PUB_CONFIG, 
		     pubd->get_clnt ());
  }
}

void
okd_t::req_err_docs_2 (ptr<pub_res_t> res)
{
  // do not quit on error; web server should still boot up with
  // standard error documents
  if (!*res) {
    warn << "cannot retrieve pubd configuration: " << res->to_str () << "\n";
    req_err_docs_4 (false);
    return;
  }

  xpub_fnset_t f;
  int i =0;
  f.files.setsize (errdocs.size ());
  errdocs.traverse (wrap (errdoc_load_fnset, &f, &i));
  ptr<xpub_result_t> r = New refcounted<xpub_result_t> ();
  pubd->call (PUB_FILES, &f, r, 
	      wrap (this, &okd_t::req_err_docs_3, r));
}

void
okd_t::req_err_docs_3 (ptr<xpub_result_t> res, clnt_stat err)
{
  if (err) {
    warn << ": failed to pub error docs: " << err << "\n";
    exit (1);
  }
  bool ret = true;
  if (res->status.status != XPUB_STATUS_OK) {
    warn << ": pub returned failure status on error doc pub:  "
	 << *res->status.error << "\n";
    ret = false;
  } else {
    pprox->cache (res->set);
  }
  req_err_docs_4 (ret);
}

static void
fill_xeds (int *i, xpub_errdoc_set_t *x, errdoc_t *ed)
{
  x->docs[*i].status = ed->status;
  x->docs[*i].fn = ed->fn;
  (*i)++;
}

void
okd_t::req_err_docs_4 (bool rc)
{
  if (!rc) {
    // maybe we don't want to have to reboot in order to bail us out.
    //errdocs.clear ();
    warn << "could not retrieve custom error documents; clearing them\n";
  } else {
    int i = 0;
    xeds.docs.setsize (errdocs.size ());  // x errdoc set
    errdocs.traverse (wrap (fill_xeds, &i, &xeds));
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

    .add ("OkdMaxFDs", &okd_max_fds, 0, 10240)
    .add ("OkdFDHighWat", &okd_fds_high_wat, 0, 10000)
    .add ("SyscallStatDumpInterval", &ok_ssdi, 0, 1000)
    .add ("OkdAcceptMessages", &accept_msgs)

    .add ("PubdUnix", wrap (this, &okd_t::got_pubd_unix))
    .add ("PubdInet", wrap (this, &okd_t::got_pubd_inet))
    .add ("PubdExecPath", wrap (this, &okd_t::got_pubd_exec))
    .add ("ErrorDoc", wrap (this, &okd_t::got_err_doc))
    .add ("SendSockAddrIn", &ok_send_sin)

    .add ("ClientTimeout", &ok_clnt_timeout, 1, 400)

    .add ("OkdName", &reported_name)
    .add ("OkdVersion", &version)
    .add ("OkdUser", &un)
    .add ("OkdGroup", &gn)

    .add ("SfsClockMode", wrap (got_clock_mode, &clock_mode))
    .add ("MmapClockFile", &mmc_file)
    .add ("OkdChildSelectDisable", &okd_child_sel_disable)

    .ignore ("MmapClockDaemon")
    .ignore ("Service")
    .ignore ("CrashSamplingInterval")
    .ignore ("MaxCrahsedProcesses")
    .ignore ("ServiceLowUid")
    .ignore ("ServiceHighUid")
    .ignore ("ServiceGroup")
    .ignore ("ServiceMode")
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
    .ignore ("ShutdownTimeout")

    .ignore ("Gzip")
    .ignore ("GzipLevel")
    .ignore ("GzipSmallStrLen")
    .ignore ("GzipCacheMin")
    .ignore ("GzipCacheMax")
    .ignore ("GzipCacheSize")
    .ignore ("GzipMemLevel")
    .ignore ("UnsafeMode")
    .ignore ("SafeStartup")
    .ignore ("SvcLog")
    .ignore ("FilterCGI")
    .ignore ("ChannelLimit")

    .ignore ("ServiceMaxFDs")
    .ignore ("ServiceFDHighWat")
    .ignore ("ServiceAcceptMessages");


  while (pa.getline (&av, &line)) {
    if (!ct.match (av, cf, line, &errors)) {
      warn << cf << ":" << line << ": unknown config parameter\n";
      errors = true;
    }
  }

  if (okd_fds_high_wat > okd_max_fds) {
    warn << "OkdMaxFDs needs to be greater than OkdFDHighWat\n";
    errors = true;
  }

  if (ok_svc_max_fds > 0 && 
      (ok_svc_fds_high_wat == 0 || ok_svc_fds_high_wat > ok_svc_max_fds)) {
    warn << "ServiceMaxFDs needs to be great than ServiceFDHighWat\n";
    errors = true;
  }

  init_syscall_stats ();

  if (un) okd_usr = ok_usr_t (un);
  if (gn) okd_grp = ok_grp_t (gn);

  if (!hostname)
    hostname = myname ();
  if (errors)
    exit (1);
}

void
okd_t::closed_fd ()
{
  nfd_in_xit --;
  if (nfd_in_xit < int (okd_fds_high_wat) && !accept_enabled)
    enable_accept ();
}

void
okd_t::strip_privileges ()
{
  if (!uid) {

    if (!chroot ())
      fatal << "startup aborted due to failed chroot call\n";
    if (setgid (okd_grp.getid ()) != 0) 
      fatal << "could not setgid for " << okd_grp.getname () << "\n";
    if (setuid (okd_usr.getid ()) != 0)
      fatal << "could not setuid for " << okd_usr.getname () << "\n";
    assert (coredumpdir);
    if (coredumpdir && chdir (coredumpdir.cstr ()) != 0) {
      fatal << "startup aborted; could not chdir to coredump dir ("
	    << coredumpdir << ")\n";
    } else {
      // debug code
      warn << "changed to cumpdir: " << coredumpdir << "\n";
    }
  }
}

void
okd_t::sclone (ref<ahttpcon_clone> x, str s, int status)
{
  if (status != HTTP_OK) {
    x->declone ();
    error (x, status);
  } else if (!s) {
    x->declone ();
    error (x, HTTP_BAD_REQUEST);
  } else {
    str *s2 = aliases[s];
    if (!s2) s2 = &s;
    okch_t *c = servtab[*s2];
    if (!c) {
      x->declone ();
      error (x, HTTP_NOT_FOUND, *s2);
    } else {
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
    nfd_in_xit ++;  // keep track of the number of FDs in transit
    close_on_exec (nfd);
    tcp_nodelay (nfd);
    ref<ahttpcon_clone> x = ahttpcon_clone::alloc (nfd, sin);

    //
    // when this file descriptor is closed on our end, we need
    // to decrement nfd_in_xit
    //
    x->set_close_fd_cb (wrap (this, &okd_t::closed_fd));

    //warn ("accepted connection from %s\n", x->get_remote_ip ().cstr ());
    x->setccb (wrap (this, &okd_t::sclone, x));

    if (nfd_in_xit > int (okd_max_fds) && accept_enabled) {
      disable_accept ();
    }
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

  // once jailed, we can access the mmap'ed clock file (if necessary)
  init_sfs_clock (mmc_file);

  //
  // debug stuff 
  // rats!
  //
  char path[MAXPATHLEN];
  getcwd (path, MAXPATHLEN);
  warn << "working directory: " << path << "\n";

  warn << "listening on " << listenaddr_str << ":" << listenport << "\n";
  enable_accept ();
}

void
okd_t::disable_accept_guts ()
{
  fdcb (listenfd, selread, NULL);
}

void
okd_t::enable_accept_guts ()
{
  fdcb (listenfd, selread, wrap (this, &okd_t::newserv, listenfd));
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
main2 (str cf, int logfd, str cdd)
{
  sfsconst_init ();
  if (!cf) {
    cf = sfsconst_etcfile ("okws_config");
    if (!cf)
      cf = sfsconst_etcfile_required ("okd_config");
  }

  zinit ();
  warn ("version %s, pid %d\n", VERSION, int (getpid ()));
  okd_t *okd = New okd_t (cf, logfd, 0, cdd);
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
  str cdd;  // core dump dir

  int ch;
  while ((ch = getopt (argc, argv, "f:l:D:c:")) != -1)
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
    case 'c':
      cdd = optarg;
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
    stall (debug_stallfile, wrap (main2, cf, logfd, cdd));
  } else {
    main2 (cf, logfd, cdd);
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

    // XXX - debug
    // will need this to debug missing file descriptors
    // warn << "got CTL fd: " << desc->ctlx->pid << "\n";

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
okd_t::turnlog (okrescb cb)
{
  ptr<bool> b = New refcounted<bool> (true);
  logd->turn (cb);
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

