// -*-c++-*-
/* $Id$ */

/*
 *
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
#include "okdbg.h"
#include "rpcwindow.h"

static void srepub (ptr<ok_repub_t> rpb, okch_t *ch) { ch->repub (rpb); }

common_404_t::common_404_t ()
{
  tab.insert ("/favicon.ico");
}


common_404_t common_404;

void
okd_t::abort ()
{
  panic ("cought ABORT signal\n");
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

    // for pub v2
    xpub_errdoc_t ed;
    ed.status = status;
    ed.fn = s[2];
    errdocs_x.docs.push_back (ed);
    
    // for pub v1
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

  // push back the packet size
  s.push_back ("-s");
  s.push_back (strbuf("0x%x", ok_axprt_ps));

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

TAMED void 
okd_t::launch_pubd (coordvar_bool_t callercv)
{
  VARS {
    bool rc;
  }
  if (!pubd) 
    pubd = New helper_exec_t (pub_program_1, "pubd");
  BLOCK { pubd->connect (@(rc)); }
  if (!rc) {
    warn << "launch of pub daemon (pubd) failed.\n";
  }
  RESUME (rc);
}


void
okd_t::req_errdoc_set_2 (svccb *sbp)
{
  sbp->replyref (errdocs_x);
}

TAMED void 
okd_t::launch_logd (coordvar_bool_t callercv)
{
  VARS { bool rc; }
  assert (logfd > 0);
  logd = New fast_log_t (logfd, logfmt);
  BLOCK { logd->connect (@(rc)); }
  RESUME (rc);
}

void
okd_t::got_alias (vec<str> s, str loc, bool *errp)
{
  if (s.size () != 3) {
    warn << loc << ": usage: Alias <to-URI> <from-URI>\n";
    *errp = true;
    return;
  }
  aliases.insert (fix_uri (s[2]), fix_uri (s[1]));
}

void
okd_t::got_regex_alias (vec<str> s, str loc, bool *errp)
{
  if (s.size () != 3 && s.size () != 4) {
    warn << loc << ": usage: RegexAlias <to-URI> <regex> [<opts>]\n";
    *errp = true;
    return;
  }
  const char *opts = s.size () == 4 ? s[3].cstr () : "";
  rrxx x;
  if (!x.compile (s[2].cstr (), opts)) {
    warn << loc << ": error compiling regex: " << x.geterr () << "\n";
    *errp = true;
    return;
  }
  regex_aliases.push_back (regex_alias_t (fix_uri (s[1]), x));
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
    .add ("ListenPorts", wrap (static_cast<ok_base_t *> (this),
			       &ok_base_t::got_ports))
    .add ("Alias", wrap (this, &okd_t::got_alias))
    .add ("RegexAlias", wrap (this, &okd_t::got_regex_alias))
    .add ("JailDir", wrap (got_dir, &jaildir))
    .add ("TopDir", &topdir)

    .add ("MaxConQueueSize", &ok_con_queue_max, OK_QMIN, OK_QMAX)
    .add ("OkMgrPort", &ok_mgr_port, OK_PORT_MIN, OK_PORT_MAX)
    .add ("ListenQueueSize", &ok_listen_queue_max, OK_QMIN, OK_QMAX)

    .add ("OkdFDHighWat", &okd_fds_high_wat, 
	  OKD_FDS_HIGH_WAT_LL, OKD_FDS_HIGH_WAT_UL)
    .add ("OkdFDLowWat", &okd_fds_low_wat, 
	  OKD_FDS_LOW_WAT_LL, OKD_FDS_HIGH_WAT_UL)
    .add ("ServiceFDQuota", &ok_svc_fd_quota, 
	  OK_SVC_FD_QUOTA_LL, OK_SVC_FD_QUOTA_UL)

    .add ("SyscallStatDumpInterval", &ok_ssdi, 0, 1000)
    .add ("OkdAcceptMessages", &accept_msgs)

    .add ("PubdUnix", wrap (this, &okd_t::got_pubd_unix))
    .add ("PubdInet", wrap (this, &okd_t::got_pubd_inet))
    .add ("PubdExecPath", wrap (this, &okd_t::got_pubd_exec))
    .add ("ErrorDoc", wrap (this, &okd_t::got_err_doc))
    .add ("SendSockAddrIn", &ok_send_sin)

    .add ("ClientTimeout", &ok_clnt_timeout, 1, 400)
    .add ("ShutdownRetries", &ok_shutdown_retries, 1, 10)
    .add ("ShutdownTimeout", &ok_shutdown_timeout, 1, 200)
    .add ("SyslogPriority", &ok_syslog_priority)
    .add ("RecycleSuioLimit", &ok_recycle_suio_limit, OK_RSL_LL, OK_RSL_UL)

    .add ("ServerName", &reported_name)
    .add ("ServerVersion", &version)
    // as reported in HTTP response headers
    .add ("ServerNameHTTP", &global_okws_server_label)
    .add ("HostName", &hostname)

    .add ("OkdUser", &un)
    .add ("OkdGroup", &gn)
    .add ("SfsClockMode", wrap (got_clock_mode, &clock_mode))
    .add ("MmapClockFile", &mmc_file)
    .add ("OkdChildSelectDisable", &okd_child_sel_disable)
    .add ("DemuxTimeout", &ok_demux_timeout, 0, 30)
    .add ("OkdDebugMsgFreq", &okd_debug_msg_freq, 0, 0x10000)
    .add ("AxprtPacketSize", &ok_axprt_ps, 0, INT_MAX)

    .ignore ("MmapClockDaemon")
    .ignore ("Service")
    .ignore ("Interpreter")
    .ignore ("Script")
    .ignore ("Service2")
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
    .ignore ("Pubd2ExecPath")
    .ignore ("Pub2WSS")
    .ignore ("Pub2Caching")
    .ignore ("Pub2VisibleErrors")
    .ignore ("Pub2SvcNegCacheTimeout")

    .ignore ("ServiceLifeRequests")
    .ignore ("ServiceLifeTime")

    .ignore ("StartupBatchSize")
    .ignore ("StartupBatchWait")

    .ignore ("ServiceFDHighWat")
    .ignore ("ServiceFDLowWat")
    .ignore ("ServiceAcceptMessages")
    .ignore ("DangerousZbufs");


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

  init_syscall_stats ();
}

void
okd_t::closed_fd ()
{
  nfd_in_xit --;

  //warn << "debug/global: dec: " << nfd_in_xit << "\n";

  if (nfd_in_xit < int (okd_fds_low_wat) && !accept_enabled)
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
      OKDBG4(OKD_STARTUP, CHATTER, 
	     "changed to core dump directory: %s\n", coredumpdir.cstr ());
    }
  }
}

void
okd_t::sclone (ref<ahttpcon_clone> x, okws1_port_t port, str s, int status)
{
  if (status != HTTP_OK) {
    x->declone ();
    error (x, status);
  } else if (!s) {
    x->declone ();
    error (x, HTTP_BAD_REQUEST);
  } else {

    // for services not on the default port, rewrite the request
    // URL with the port explictly mentioned
    if (port != listenport) {
      strbuf b (":");
      b << port << s;
      s = b;
    }

    // first check the aliases table
    str *s2 = aliases[s];
    
    // if no alias, then maybe the real name was used
    if (!s2) s2 = &s;

    okch_t *c = servtab[*s2];

    // check the regex table, as a last resort, if no
    // child was found
    for (u_int i = 0; !c && i < regex_aliases.size (); i++) {
      regex_alias_t &ra = regex_aliases[i];
      if (ra._rxx.match (s)) {
	c = servtab[ra._target];
	if (!c) {
	  strbuf b;
	  b << "Expected a service " << ra._target 
	    << " but couldn't find it!\n";
	  okdbg_warn (ERROR, b);
	}
      }
    }
    
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
  u_int freq = okd_debug_msg_freq;
  if (nfd >= 0) {
    reqid ++;

    // debug messaging
    if (freq > 0 && (reqid % freq) == 0) {
      strbuf b;
      b << "nfd_in_xit=" << nfd_in_xit << "; " 
	<< "xtab.nent=" << xtab.n_entries () << "; "
	<< "nfds=" << n_ahttpcon << "\n";
      okdbg_warn (CHATTER, b);
    }

    nfd_in_xit ++;  // keep track of the number of FDs in transit
    close_on_exec (nfd);
    tcp_nodelay (nfd);
    ref<ahttpcon_clone> x = ahttpcon_clone::alloc (nfd, sin);
    
    xtab.reg (x, x->destroyed_p);

    //
    // when this file descriptor is closed on our end, we need
    // to decrement nfd_in_xit
    //
    x->set_close_fd_cb (wrap (this, &okd_t::closed_fd));


    OKDBG4(OKD_NOISY_CONNECTIONS, CHATTER,
	   "accepted connection from %s\n", x->get_remote_ip ().cstr ());

    x->setccb (wrap (this, &okd_t::sclone, x, *portmap[fd]));

    if (nfd_in_xit > int (okd_fds_high_wat) && accept_enabled) {
      disable_accept ();
    }
  }
  else if (errno != EAGAIN)
    warn ("** accept error: %m\n");
}



void
okd_t::disable_accept_guts ()
{
  u_int sz = listenfds.size ();
  for (u_int i = 0; i < sz; i++) 
    fdcb (listenfds[i], selread, NULL);
}

void
okd_t::enable_accept_guts ()
{
  u_int sz = listenfds.size ();
  for (u_int i = 0; i < sz; i++) 
    fdcb (listenfds[i], selread, wrap (this, &okd_t::newserv, listenfds[i]));
}

void
okd_t::stop_listening ()
{
  u_int sz = listenfds.size ();
  for (u_int i = 0; i < sz; i++) {
    fdcb (listenfds[i], selread, NULL);
    close (listenfds[i]);
  }
}

static void
usage ()
{
  warnx << "usage: okd [-D <dbg-file>] [-x <pub2fd>] "
	<< " -l <logfd> -f <configfile>\n";
  exit (1);
}

TAMED static 
void start_okd (int argc, char **argv)
{
  VARS {
    str cf;
    int logfd  (-1);
    int pub2fd (-1);
    str debug_stallfile;
    str cdd;  // core dump dir
    okws1_port_t port (ok_dport);
    okd_t *okd;
  }

  setprogname (argv[0]);
  set_debug_flags ();
  
  int ch;
  while ((ch = getopt (argc, argv, "f:l:D:c:p:x:")) != -1)
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
    case 'p':
      if (!convertint (optarg, &port))
	usage ();
      break;
    case 'x':
      if (!convertint (optarg, &pub2fd))
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
  if (debug_stallfile) 
    BLOCK { stall (debug_stallfile, @()); }

  sfsconst_init ();
  if (!cf) 
    cf = get_okws_config ();

  zinit ();
  warn ("version %s, pid %d\n", VERSION, int (getpid ()));
  okd = New okd_t (cf, logfd, 0, cdd, port, pub2fd);
  okd->set_signals ();
  okd->launch ();
}

TAMED void 
okd_t::launch ()
{
  VARS {
    bool pubd_rc, logd_rc, pub2_rc (true);
  }
  parseconfig ();
  check_runas ();
  open_mgr_socket ();
  init_pub2 ();

  BLOCK {
    launch_pubd (@(pubd_rc));
    launch_logd (@(logd_rc));
    launch_pub2 (@(pub2_rc));
  }

  if (!pubd_rc) {
    warn << "Pubd, version 1, failed to launch: aborting\n";
    exit (1);
  }
  if (!logd_rc) {
    warn << "failed to connect to oklogd; aborting\n";
    exit (1);
  }
  if (!pub2_rc) {
    warn << "failed to launch pub2, for setup that requires it; aborting\n";
    exit (1);
  }

  okldx = fdsource_t<okws_fd_t>::alloc (okldfd, wrap (this, &okd_t::gotfd));
  assert (okldx);

  if (allports.size () == 0) {
    warn << "No listen addr/ports specified; assuming *:80\n";
    allports.push_back (listenport);
  }

  for (u_int i = 0; i < allports.size () ; i++) {
    int fd = inetsocket (SOCK_STREAM, allports[i], listenaddr);
    if (fd < 0)
      fatal ("could not bind TCP port %d: %m\n", allports[i]);
    close_on_exec (fd);
    listen (fd, ok_listen_queue_max);
    listenfds.push_back (fd);
    portmap.insert (fd, allports[i]);
    warn << "listening on " << listenaddr_str << ":" << allports[i] << "\n";
  }

  strip_privileges ();

  // once jailed, we can access the mmap'ed clock file (if necessary)
  init_sfs_clock (mmc_file);

  enable_accept ();
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
    okdbg_warn (ERROR, "unknown FD type received from okld\n");
    break;
  }
  return;
}

void
okd_t::got_chld_fd (int fd, ptr<okws_fd_t> desc)
{
  okch_t *ch;
  str uri;
  switch (desc->fdtyp) {
  case OKWS_SVC_X:
    uri = fix_uri (desc->x->name);
    if (!(ch = servtab[uri])) {
      warn << "received service FDs out of order!\n";
      close (fd);
    }
    ch->got_new_x_fd (fd, desc->x->pid);
    break;
  case OKWS_SVC_CTL_X:
    uri = fix_uri (desc->ctlx->name);
    if (!(ch = servtab[uri])) {
      // calling New will insert the object into the table
      ch = New okch_t (this, uri);
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

//=======================================================================
//
// Repub v2
//
//   In repub v2, we ask pubd for the complete file set descending from
//   the given repub filesets, as before.  However, we find a workaround
//   to the problem that the returned set is often quite large. 
//   
//   In v2, pubd returns a 'summary' of the file set it really wanted to
//   return, composed of the set of filename->content hash bindings,
//   the number of files to fetch, and also a 'cookie.'
//
//   okd then sends follow-up RPCs to get the files in the requested 
//   fileset, using the cookie to indentify the pubd-resident state.
//   
//   Once all files are collected (or an abortive error has occured),
//   okd must be certain to signal pubd to clean up, and therefore calls
//   PUB_FILES2_CLOSE with the session cookie.
//
//   Eventually, v2 should replace v1 entirely.
//
void
okd_t::repub2 (const xpub_fnset_t &f, okrescb cb)
{
  ptr<ok_repub_t> rpb = New refcounted<ok_repub_t> (f, cb);
  pubd->call (PUB_FILES2, &f, &rpb->xpr2, 
	      wrap (this, &okd_t::repub2_cb1, rpb));
}

void
okd_t::repub2_cb1 (ptr<ok_repub_t> rpb, clnt_stat err)
{
  if (err || rpb->xpr2.status.status != XPUB_STATUS_OK) {
    strbuf sb;
    sb << "RPC Error from PUB_FILES2: " << err ;
    okdbg_warn (ERROR, sb);
    repub_cb1 (rpb, err);
  } else {
    rpb->xpr.set.bindings = rpb->xpr2.set.bindings;
    rpb->nfiles = rpb->xpr2.set.nfiles;
    rpb->cache.setsize (rpb->nfiles);
    rpb->cookie = rpb->xpr2.set.cookie;
    rpb->xpr.set.files.setsize (rpb->nfiles);
    repub2_getfiles (rpb);
  }
}

void
okd_t::repub2_getfiles (ptr<ok_repub_t> rpb)
{
  rpc_windower_t *w = 
    New rpc_windower_t (rpb->nfiles, 30, 
			wrap (this, &okd_t::repub2_getfile, rpb),
			wrap (this, &okd_t::repub2_gotfile, rpb),
			wrap (this, &okd_t::repub2_done, rpb));
  w->run ();

  // w will delete itself once repub2_done() is called.
}

bool
okd_t::repub2_getfile (ptr<ok_repub_t> rpb, int i, aclnt_cb cb)
{
  xpub_files2_getfile_arg_t arg;
  arg.cookie = rpb->cookie;
  arg.fileno = i;
  pubd->call (PUB_FILES2_GETFILE, &arg, &rpb->cache[i], cb);
  return true;
}

bool
okd_t::repub2_gotfile (ptr<ok_repub_t> rpb, int i, clnt_stat err)
{
  bool ret = false;
  if (err) {
    strbuf sb;
    sb << "Repub RPC error for filenumber " << i << ": " << err;
    rpb->res->pub_res_t::add (sb);
    okdbg_warn (ERROR, sb) ;
  } else {
    int rc = rpb->cache[i].status;
    if (rc != XPUB_STATUS_OK) {
      strbuf sb;
      sb << "Pubd error " << rc << " for filenumber " << i ;
      if (rc == XPUB_STATUS_ERR)
	sb << ": " << *rpb->cache[i].error;
      okdbg_warn (ERROR, sb);
      rpb->res->pub_res_t::add (sb);
    } else {
      rpb->xpr.set.files[i] = *(rpb->cache[i].file);
      ret = true;
    }
  }
  return ret;
}

static void
close_cb (xpub_status_typ_t *t, clnt_stat err)
{
  if (err) {
    strbuf sb;
    sb << "RPC Error for PUB_FILES2_CLOSE: " << err << "\n";
    okdbg_warn (ERROR, sb);
  } else if (*t != XPUB_STATUS_OK) {
    strbuf sb;
    sb << "PUB_FILES2_CLOSE failed with result code: " << *t << "\n";
    okdbg_warn (ERROR, sb);
  }
  delete t;
}


void
okd_t::repub2_done (ptr<ok_repub_t> rpb, bool rc)
{
  xpub_status_typ_t *t = New xpub_status_typ_t ();
  pubd->call (PUB_FILES2_CLOSE, &rpb->cookie, &t, wrap (close_cb, t));
  repub_cb1 (rpb, static_cast<clnt_stat> (0));
}

//
// End Repub v2
//
//=======================================================================

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

static void 
s_relaunch_cb (okch_t *ch) { ch->kill (); }

void
okd_t::relaunch (const ok_progs_t &x, okrescb cb)
{
  ptr<ok_res_t> res = New refcounted<ok_res_t> ();
  apply_to_children (x, wrap (s_relaunch_cb), res);
  (*cb) (res);
}

static void
s_custom1_cb (ok_custom_data_t data, okch_t *ch) 
{ 
  ch->custom1_out (data); 
}

static void
s_custom2_cb (ptr<ok_custom2_trig_t> trig, ok_custom_data_t data, okch_t *ch)
{
  ch->custom2_out (trig, data);
}

// receive and handle an incoming CUSTOM1 RPC request
void
okd_t::custom1_in (svccb *sbp)
{
  ok_custom_arg_t *c = sbp->Xtmpl getarg<ok_custom_arg_t> ();
  custom1_in (*c, wrap (replystatus, sbp));
}

static void
replytrig (svccb *sbp, ptr<ok_custom_res_set_t> res)
{
  sbp->reply (res);
}

static void
s_notfound_cb (ptr<ok_custom2_trig_t> trig, str s)
{
  trig->add_err (s, OK_STATUS_NOSUCHCHILD);
}

void
okd_t::custom2_in (svccb *sbp)
{
  ok_custom_arg_t *c = sbp->Xtmpl getarg<ok_custom_arg_t> ();

  ptr<ok_custom2_trig_t> trig = New refcounted<ok_custom2_trig_t> ();

  // when the trig counts down to 0, this cb will be called,
  // which will send a reply back to the caller
  trig->setcb (wrap (replytrig, sbp, trig->get_custom_res ()));

  apply_to_children (c->progs, 
		     wrap (s_custom2_cb, trig, c->data), 
		     trig->get_ok_res (),
		     wrap (s_notfound_cb, trig));
}

void
okd_t::custom1_in (const ok_custom_arg_t &x, okrescb cb)
{
  ptr<ok_res_t> res = New refcounted<ok_res_t> ();
  apply_to_children (x.progs, wrap (s_custom1_cb, x.data), res);
  (*cb) (res);
}

void
okd_t::apply_to_children (const ok_progs_t &x, cb_okch_t apply_cb,
			  ptr<ok_res_t> res, cbs::ptr notfoundcb)
{
  if (x.typ == OK_SET_ALL) {
    servtab.traverse (apply_cb);
  } else if (x.typ == OK_SET_SOME) {
    u_int lim = x.progs->size ();
    for (u_int j = 0; j < lim; j++) {
      str prog = (*x.progs)[j];
      okch_t *o = servtab[prog];
      if (!o) {
	*res << (strbuf ("cannot find program: ") << prog);
	if (notfoundcb)
	  (*notfoundcb) (prog);
      } else 
	(*apply_cb) (o);
    }
  }
}

void
ok_custom2_trig_t::add_err (const str &svc, ok_xstatus_typ_t t)
{
  ok_custom_res_t el;
  el.prog = svc;
  el.res.set_status (t);
  _custom_res->results.push_back (el);
}

void
ok_custom2_trig_t::add_succ (const str &svc, const ok_custom_data_t &d)
{
  ok_custom_res_t el;
  el.prog = svc;
  el.res.set_status (OK_STATUS_OK);
  *el.res.dat = d;
  _custom_res->results.push_back (el);
}

int
main (int argc, char *argv[])
{
  start_okd (argc, argv);
  amain ();
}
