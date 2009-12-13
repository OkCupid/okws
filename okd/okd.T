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
#include "pubutil.h"
#include "okdbg.h"
#include "ok_adebug.h"

okd_t *global_okd;

//-----------------------------------------------------------------------

common_404_t::common_404_t ()
{
  tab.insert ("/favicon.ico");
}

//-----------------------------------------------------------------------

common_404_t common_404;

//-----------------------------------------------------------------------

void
okd_t::abort ()
{
  panic ("caught ABORT trigger\n");
}

//-----------------------------------------------------------------------

void
okd_t::set_signals ()
{
  sigcb (SIGTERM, wrap (this, &okd_t::shutdown, SIGTERM));
  sigcb (SIGINT,  wrap (this, &okd_t::shutdown, SIGINT));
  sigcb (SIGABRT, wrap (this, &okd_t::abort));
}

//-----------------------------------------------------------------------

okd_t::~okd_t ()
{
  if (logd) delete logd;
  if (pubd) delete pubd;
}

//-----------------------------------------------------------------------

void
okd_t::got_err_doc (vec<str> s, str loc, bool *errp)
{
  strip_comments (&s);
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
  }
}

//-----------------------------------------------------------------------

tamed void 
okd_t::launch_logd (evb_t ev)
{
  tvars { bool rc; }
  assert (logfd > 0);
  logd = New fast_log_t (logfd, logfmt);
  twait { logd->connect (mkevent (rc)); }
  ev->trigger (rc);
}

//-----------------------------------------------------------------------

void
okd_t::got_alias (vec<str> s, str loc, bool *errp)
{
  strip_comments (&s);
  if (s.size () != 3) {
    warn << loc << ": usage: Alias <to-URI> <from-URI>\n";
    *errp = true;
    return;
  }
  aliases.insert (fix_uri (s[2]), fix_uri (s[1]));
}

//-----------------------------------------------------------------------

void
okd_t::got_regex_alias (vec<str> s, str loc, bool *errp)
{
  strip_comments (&s);
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

//-----------------------------------------------------------------------

void
okd_t::open_mgr_socket ()
{
  struct stat sb;
  // XXX - not secure; adversary can mount DOS-attacks over this port,
  // assuming he has a machine behind the firewall. better to either 
  // authenticate or to have the manager reachable only via a local
  // unix socket
  str s = jail2real (_socket_filename);
  if (stat (s, &sb) == 0) {
    warn << "Management socket (" << s << ") already exists or is in use!\n";
    exit (NO_SOCKET_ALLOCATED);
  }

  if (!pub_server (wrap (this, &okd_t::newmgrsrv), s)) {
    warn << "Cannot open management socket (" << s<< ")\n";
    exit (NO_SOCKET_ALLOCATED);
  }

  if (!uid) {
    ok_usr_t u (_socket_owner);
    ok_grp_t g (_socket_group);

    if (!_socket_owner) u = okd_usr;
    if (!_socket_group) g = okd_grp;

    if (!u) fatal << "Cannot find user: " << u.getname () << "\n";
    if (!g) fatal << "Cannot find group: " << g.getname () << "\n";

    if (chown (s.cstr (), u.getid (), g.getid ()) != 0) {
      fatal << "Failed to chown " << u.getname () << "." << g.getname ()
	    << " on the management socket '" << s << "'\n";
    }
  }
  if (_socket_mode >= 0) {
    if (chmod (s.cstr (), _socket_mode) != 0) {
      fatal << "Cannot alter permisions on the management socket "
	    << "'" << s << "'\n";
    }
  }

  warn << "Using management socket: " << _socket_filename
       << " (jail2real: " << s << ")\n";
}

//-----------------------------------------------------------------------

void
okd_t::newmgrsrv (ptr<axprt_stream> x)
{
  vNew okd_mgrsrv_t (x, this);
}

//-----------------------------------------------------------------------

bool
okd_t::parse_file (const str &cf)
{
  conftab ct;
  str sys_sel_tmp;

  ct.add ("BindAddr", wrap (static_cast<ok_base_t *> (this), 
			    &ok_base_t::got_bindaddr))
    .add ("ListenPorts", wrap (static_cast<ok_base_t *> (this),
			       &ok_base_t::got_ports, false))
    .add ("ListenPortsSSL", wrap (static_cast<ok_base_t *> (this),
				  &ok_base_t::got_ports, true))
    .add ("Alias", wrap (this, &okd_t::got_alias))
    .add ("RegexAlias", wrap (this, &okd_t::got_regex_alias))
    .add ("JailDir", wrap (got_dir, &jaildir))
    .add ("TopDir", &topdir)

    .add ("MaxConQueueSize", &ok_con_queue_max, OK_QMIN, OK_QMAX)
    .add ("OkMgrSocket", &_socket_filename)
    .add ("OkMgrSocketOwner", &_socket_owner)
    .add ("OkMgrSocketGroup", &_socket_group)
    .add ("OkMgrSocketAccessMode", &_socket_mode, 0, 0777)
    .add ("ListenQueueSize", &ok_listen_queue_max, OK_QMIN, OK_QMAX)
    .add ("include", wrap (static_cast<config_parser_t *> (this), 
			   &config_parser_t::include))

    .add ("OkdFDHighWat", &okd_fds_high_wat, 
	  OKD_FDS_HIGH_WAT_LL, OKD_FDS_HIGH_WAT_UL)
    .add ("OkdFDLowWat", &okd_fds_low_wat, 
	  OKD_FDS_LOW_WAT_LL, OKD_FDS_HIGH_WAT_UL)
    .add ("ServiceFDQuota", &ok_svc_fd_quota, 
	  OK_SVC_FD_QUOTA_LL, OK_SVC_FD_QUOTA_UL)

    .add ("SyscallStatDumpInterval", &ok_ssdi, 0, 1000)
    .add ("OkdAcceptMessages", &accept_msgs)

    .add ("ErrorDoc", wrap (this, &okd_t::got_err_doc))
    .add ("SendSockAddrIn", &ok_send_sin)

    .add ("ClientTimeout", &ok_clnt_timeout, 1, 400)
    .add ("ShutdownRetries", &ok_shutdown_retries, 1, 10)
    .add ("ShutdownTimeout", &ok_shutdown_timeout, 1, 200)
    .add ("SyslogPriority", &ok_syslog_priority)
    .add ("SyslogTag", &ok_syslog_tag)
    .add ("RecycleSuioLimit", &ok_recycle_suio_limit, OK_RSL_LL, OK_RSL_UL)

    .add ("ServerName", &reported_name)
    .add ("ServerVersion", &version)
    // as reported in HTTP response headers
    .add ("ServerNameHTTP", &global_okws_server_label)
    .add ("HostName", &hostname)

    .add ("OkdUser", &_config_user)
    .add ("OkdGroup", &_config_grp)
    .add ("SfsClockMode", wrap (got_clock_mode, &clock_mode))
    .add ("MmapClockFile", &mmc_file)
    .add ("OkdChildSelectDisable", &okd_child_sel_disable)
    .add ("DemuxTimeout", &ok_demux_timeout, 0, 360)
    .add ("OkdDebugMsgFreq", &okd_debug_msg_freq, 0, 0x10000)
    .add ("AxprtPacketSize", &ok_axprt_ps, 0, INT_MAX)
    .add ("PubChunkSize", &ok_pub3_max_datasz, 0, INT_MAX)
    .add ("SelectPolicy", &sys_sel_tmp)
    .add ("AcceptDelay", &okd_accept_delay, u_int (0), UINT_MAX)
    .add ("SslPrimaryPort", &ok_ssl_port, u_int (0), UINT_MAX)
    .add ("LazyStartup", &_lazy_startup)
    .add ("StatPageURL", &_stat_page_url)
    .add ("TcpNoDelay", &_okd_nodelay)

    // script and service options, just ignore 'em...
    .ignore ("Service")
    .ignore ("Service2")
    .ignore ("Script")
    .ignore ("LogBuffers")

    .ignore ("MmapClockDaemon")
    .ignore ("Interpreter")
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
    .ignore ("SslLog")
    .ignore ("ErrorLog")
    .ignore ("AccessLogFmt")
    .ignore ("OklogdUser")
    .ignore ("OklogdGroup")
    .ignore ("LogTick")
    .ignore ("LogPeriod")
    .ignore ("CoreDumpDir")
    .ignore ("CoreDumpUser")
    .ignore ("CoreDumpGroup")
    .ignore ("CoreDumpMode")
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
    .ignore ("CgiValueLenLimit")
    .ignore ("PubdExecPath")
    .ignore ("PubWSS")
    .ignore ("PubCaching")
    .ignore ("PubVisibleErrors")
    .ignore ("PubSvcNegCacheTimeout")

    .ignore ("ServiceLifeRequests")
    .ignore ("ServiceLifeTime")

    .ignore ("OkssldExecPath")
    .ignore ("SslCertFile")
    .ignore ("SslKeyFile")
    .ignore ("SslCertChainFile")
    .ignore ("SslListenPorts")
    .ignore ("SslTimeout")
    .ignore ("SslUser")
    .ignore ("SslGroup")

    .ignore ("StartupBatchSize")
    .ignore ("StartupBatchWait")

    .ignore ("ServiceFDHighWat")
    .ignore ("ServiceFDLowWat")
    .ignore ("ServiceAcceptMessages")
    .ignore ("SyslogLevels")
    .ignore ("DangerousZbufs")
    .ignore ("OklogdPidfile")
    .ignore ("Pub3JsonStrictEscaping")
    .ignore ("AhttpconZombieWarn")
    .ignore ("AhttpconZombieTimeout")
    ;

  bool ret = ct.run (cf);
  if (ret) {
    ok_sys_sel_policy = sfs_core::select_policy_from_str (sys_sel_tmp);
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
okd_t::post_config (const str &fn)
{
  if (_config_user) okd_usr = ok_usr_t (_config_user);
  if (_config_grp) okd_grp = ok_grp_t (_config_grp);

  if (!hostname)
    hostname = myname ();

  init_syscall_stats ();
  return true;
}

//-----------------------------------------------------------------------

void
okd_t::closed_fd ()
{
  nfd_in_xit --;

  //warn << "debug/global: dec: " << nfd_in_xit << "\n";

  if (_accept_ready &&
      nfd_in_xit < int (okd_fds_low_wat) && 
      !accept_enabled)

    enable_accept ();
}

//-----------------------------------------------------------------------

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

//-----------------------------------------------------------------------

void
okd_t::sclone (ahttpcon_wrapper_t<ahttpcon_clone> acw, str s, int status)
{
  ref<ahttpcon_clone> x (acw.con ());
  okws1_port_t port = acw.demux_data ()->port ();

  if (!s && status == HTTP_OK)
    status = HTTP_TIMEOUT;

  if (status != HTTP_OK) {
    x->declone ();
    error (x, status);
  } else if (_stat_page_url && s == _stat_page_url) {
    render_stats_page (acw.con ());
  } else {
    
    // for services not on the default port, rewrite the request
    // URL with the port explictly mentioned
    if (port != listenport && port != ok_ssl_port) {
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
	  b << "Startup race condition: expected a service " << ra._target 
	    << " but it's not available yet!\n";
	  okdbg_warn (ERROR, b);
	}
      }
      if (ra._rxx.errcode () != 0) {
	strbuf b;
	b << " - cause: RXX failure: " << ra._rxx.errcode () << "\n";
	b << " - input was: '"; 
	if (s) b << s;
	b << "'\n";
	okdbg_warn (ERROR, b);
      }
    }
    
    if (!c) {
      x->declone ();
      error (x, HTTP_NOT_FOUND, *s2);
    } else {
      c->clone (acw);
    }
  }
}

//-----------------------------------------------------------------------

void
okd_t::newserv (int fd)
{
  sockaddr_in *sin = (sockaddr_in *) xmalloc (sizeof (sockaddr_in));
  socklen_t sinlen = sizeof (sockaddr_in);
  bzero (sin, sinlen);
  int nfd = accept (fd, (sockaddr *) sin, &sinlen);
  if (nfd < 0) {
    if (errno != EAGAIN) {
      const char *ip = NULL;
      if (sin) {
	ip = inet_ntoa (sin->sin_addr);
      }
      if (!ip) ip = "<no-ip>";
      warn ("** accept error (%s): %m\n", ip);
    } 
    xfree (sin);
  } else {
    newserv2 (*portmap[fd], nfd, sin, false, NULL);
  }
}

//-----------------------------------------------------------------------

void
okd_t::newserv2 (int port, int nfd, sockaddr_in *sin, bool proxied,
		 const ssl_ctx_t *ssl)
{
  u_int freq = okd_debug_msg_freq;
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


  // I don't think there's really a good need for this.  Plus,
  // certail FiOS users complained that they couldn't receive
  // packets when this flag was enabled.  It's now off by 
  // default.
  if (!proxied && _okd_nodelay) {
    tcp_nodelay (nfd);
  }

  ref<ahttpcon_clone> x (ahttpcon_clone::alloc (nfd, sin));
  ref<demux_data_t> dd (New refcounted<demux_data_t> (port, ssl));
  ahttpcon_wrapper_t<ahttpcon_clone> acw (x, dd);
  
  xtab.reg (x, x->destroyed_p);
  
  //
  // when this file descriptor is closed on our end, we need
  // to decrement nfd_in_xit
  //
  // No longer necesary; for now leave it in
  //
  x->set_close_fd_cb (wrap (this, &okd_t::closed_fd));
  
  
  OKDBG4(OKD_NOISY_CONNECTIONS, CHATTER,
	 "accepted connection from %s\n", x->get_remote_ip ().cstr ());
  
  x->setccb (wrap (this, &okd_t::sclone, acw));
  
  if (nfd_in_xit > int (okd_fds_high_wat) && accept_enabled) {
    disable_accept ();
  }
}

//-----------------------------------------------------------------------

void
okd_t::disable_accept_guts ()
{
  _ssl.disable_accept ();
  u_int sz = listenfds.size ();
  for (u_int i = 0; i < sz; i++) 
    fdcb (listenfds[i], selread, NULL);
}

//-----------------------------------------------------------------------

void
okd_t::enable_accept_guts ()
{
  if (sdflag) {
    warn << "refusing to reenable accept when in shutdown\n";
    return;
  }
  _ssl.enable_accept ();
  u_int sz = listenfds.size ();
  for (u_int i = 0; i < sz; i++) 
    fdcb (listenfds[i], selread, wrap (this, &okd_t::newserv, listenfds[i]));
}

//-----------------------------------------------------------------------

void
okd_t::stop_listening ()
{
  accept_enabled = false;
  _ssl.disable_accept ();
  u_int sz = listenfds.size ();
  for (u_int i = 0; i < sz; i++) {
    fdcb (listenfds[i], selread, NULL);
    close (listenfds[i]);
    listenfds[i] = -1;
  }
  listenfds.clear ();
}

//-----------------------------------------------------------------------

static void
usage ()
{
  warnx << "usage: okd [-D <dbg-file>] [-x <pub-fd>] "
	<< " -l <logfd> -f <configfile>\n";
  exit (NO_SOCKET_ALLOCATED);
}

//-----------------------------------------------------------------------

tamed static 
void start_okd (int argc, char **argv)
{
  tvars {
    str cf;
    int logfd  (-1);
    int pub3fd (-1);
    str cdd;  // core dump dir
    okws1_port_t port (ok_dport);
    bool debug_startup (false);
    okd_t *okd;
  }

  setprogname (argv[0]);
  set_debug_flags ();

  int ch;
  while ((ch = getopt (argc, argv, "f:l:Dc:p:x:")) != -1)
    switch (ch) {
    case 'D':
      debug_startup = true;
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
      if (!convertint (optarg, &pub3fd))
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
    exit (NO_SOCKET_ALLOCATED);
  }

  // for debugging, we'll stall until the given file is touched.
  if (debug_startup) {
    twait { ok::debug_startup (mkevent ()); }
  }

  sfsconst_init (true);
  if (!cf) 
    cf = get_okws_config ();

  zinit ();
  warn ("version %s, pid %d\n", OKWS_PATCHLEVEL_STR, int (getpid ()));
  okd = New okd_t (cf, logfd, 0, cdd, port, pub3fd);
  global_okd = okd;
  okd->set_signals ();
  okd->launch ();
}

//-----------------------------------------------------------------------

tamed void 
okd_t::launch ()
{
  tvars {
    bool pub_rc, logd_rc;
    bool do_exit (false);
  }

  if (!run_configs (configfile))
    exit (NO_SOCKET_ALLOCATED);

  set_sfs_select_policy ();
  
  check_runas ();
  open_mgr_socket ();
  init_pub ();

  twait {
    launch_logd (mkevent (logd_rc));
    launch_pub (mkevent (pub_rc));
  }

  if (!logd_rc) {
    warn << "failed to connect to oklogd; aborting\n";
    do_exit = true;
  }
  if (!pub_rc) {
    warn << "failed to launch pubd; aborting\n";
    do_exit = true;
  }

  if (do_exit) {
    exit (1);
  }

  _okld_x = axprt_unix::alloc (okldfd);
  _okld_srv = asrv::alloc (_okld_x, okld_program_1,
			   wrap (this, &okd_t::okld_dispatch));

  if (lazy_startup()) {
    _okld_cli = aclnt::alloc (_okld_x, okld_program_1);
  }

  if (_http_ports.size () == 0) {
    warn << "No listen addr/ports specified; assuming *:80\n";
    _http_ports.push_back (listenport);
  }

  for (u_int i = 0; i < _http_ports.size () ; i++) {
    int fd = inetsocket (SOCK_STREAM, _http_ports[i], listenaddr);
    if (fd < 0) {
      fatal ("could not bind TCP port %d: %m\n", _http_ports[i]);
    }
    close_on_exec (fd);
    listen (fd, ok_listen_queue_max);
    listenfds.push_back (fd);
    portmap.insert (fd, _http_ports[i]);
    warn << "listening on " << listenaddr_str << ":" << _http_ports[i] << "\n";
  }

  strip_privileges ();

  // once jailed, we can access the mmap'ed clock file (if necessary)
  init_sfs_clock (mmc_file);

  if (okd_accept_delay) {
    warn << "delaying " << okd_accept_delay << "s before enabling accept\n";
    twait { delaycb (okd_accept_delay, 0, mkevent ()); }
    warn << "accept delay complete\n";
  }

  _accept_ready = true;

  enable_accept ();
}

//-----------------------------------------------------------------------

void
okd_t::okld_dispatch (svccb *sbp)
{
  if (!sbp) {
    warn << "initiate shutdown; EOF from okld\n";
    shutdown (0);
  } else {
    switch (sbp->proc ()) {
    case OKLD_NEW_SERVICE:
      {
	RPC::okld_program_1::okld_new_service_srv_t<svccb> srv (sbp);
	const okws_svc_descriptor_t *arg = srv.getarg ();
	int fd = _okld_x->recvfd ();
	ok_xstatus_typ_t st;
	if (fd >= 0)  {
	  got_child_fd (fd, *arg);
	  st = OK_STATUS_OK;
	} else {
	  st = OK_STATUS_BADFD;
	}
	srv.reply (st);
      }
      break;
    case OKLD_RESERVE:
      {
	RPC::okld_program_1::okld_reserve_srv_t<svccb> srv (sbp);
	const okws_svc_reserve_arg_t *arg = srv.getarg ();
	ok_xstatus_typ_t st = reserve_child (arg->name, arg->lazy);
	srv.reply (st);
      }
      break;
    case OKLD_SEND_SSL_SOCKET:
      {
	RPC::okld_program_1::okld_send_ssl_socket_srv_t<svccb> srv (sbp);

	// ignore arg!
	//const okws_send_ssl_arg_t *arg = srv.getarg ();

	int fd = _okld_x->recvfd ();
	ok_xstatus_typ_t st;
	if (fd >= 0) {
	  make_async (fd);
	  if (listen_from_ssl (fd)) {
	    st = OK_STATUS_OK;
	  } else {
	    st = OK_STATUS_NOMORE;
	  }
	} else {
	  st = OK_STATUS_BADFD;
	}
	srv.reply (st);
      }
      break;
    default:
      sbp->reject (PROC_UNAVAIL);
      break;
    }
  }
}

//-----------------------------------------------------------------------

bool
okd_t::listen_from_ssl (int fd)
{
  bool ret = true;
  if (!_ssl.init (fd)) {
    warn << "Multiple SSLs given; using only the first!\n";
    close (fd);
    ret = false;
  }
  return ret;
}

//-----------------------------------------------------------------------

void
okd_t::got_child_fd (int fd, const okws_svc_descriptor_t &d)
{
  str uri = fix_uri (d.name);
  okch_t *ch;
  if (!(ch = servtab[uri])) {
    // calling New will insert the object into the table
    ch = New okch_t (this, uri);
  }
  ch->got_new_ctlx_fd (fd, d.pid);
}

//-----------------------------------------------------------------------

ok_xstatus_typ_t
okd_t::reserve_child (const str &nm, bool lazy)
{
  okch_t *ch;
  ok_xstatus_typ_t ret = OK_STATUS_OK;
  okc_state_t st = lazy ? OKC_STATE_STANDBY : OKC_STATE_NONE;
  if (!(ch = servtab[nm])) {
    ch = New okch_t (this, nm, st);
  } else {
    ch->set_state (st);
    ret = OK_STATUS_DUP;
  }
  return ret;
}

//-----------------------------------------------------------------------

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

tamed void
okd_t::turnlog (svccb *sbp)
{
  tvars {
    RPC::okmgr_program_1::okmgr_turnlog_srv_t<svccb> srv (sbp);
    ok_xstatus_t status (OK_STATUS_OK);
    str s;
  }
  twait { logd->turn (mkevent (s)); }
  if (s) {
    status.set_status (OK_STATUS_ERR);
    *status.error = s;
  }
  srv.reply (status);
}

//=======================================================================

class child_applicator_t {
public:
  child_applicator_t () : _ok (true) {}
  virtual ~child_applicator_t () {}
  void run (okd_t::servtab_t *t, const ok_progs_t &x, evv_t ev, CLOSURE);
  virtual void apply (const ok_prog_t &p, okch_t *ch, evs_t ev, CLOSURE) = 0;
  virtual void nochild (const ok_prog_t &p, evv_t ev, CLOSURE) {}
  void fill_status (ok_xstatus_t *out);
  void hit_error (str prog, str e);
private:
  bool _ok;
  vec<str> _err;
};

//-----------------------------------------------------------------------

tamed void
child_applicator_t::run (okd_t::servtab_t *t, const ok_progs_t &x, evv_t ev)
{
  tvars {
    size_t lim (0), j (0);
    ok_prog_t prog; 
    okch_t *o;
    vec<str> progs;
    vec<str> errors;
    ihash_iterator_t<okch_t, okd_t::servtab_t> it (*t);
  }

  if (x.typ == OK_SET_ALL) {
    lim = t->size ();
    errors.setsize (lim);
    twait {
      while ((o = it.next ())) {
	progs.push_back (o->servpath);
	apply (o->servpath, o, mkevent (errors[j++]));
      }
    }

  } else if (x.typ == OK_SET_SOME) {
    lim = x.progs->size ();
    errors.setsize (lim);
    twait { 
      for (j = 0; j < lim; j++) {
	prog = (*x.progs)[j];
	progs.push_back (prog);
	if ((o = (*t)[prog])) {
	  apply (prog, o, mkevent (errors[j]));
	} else {
	  strbuf b ("cannot find program: ");
	  b << prog;
	  errors[j] = b;
	  nochild (prog, mkevent ());
	}
      }
    }
  }

  for (j = 0; j < lim; j++) {
    hit_error (progs[j], errors[j]);
  }

  ev->trigger ();
}

//-----------------------------------------------------------------------

void
child_applicator_t::hit_error (str prog, str e)
{
  if (e) { 
    strbuf ("in service %s: %s", prog.cstr (), e.cstr ());
    _ok = false;
    _err.push_back (e);
  }
}

//-----------------------------------------------------------------------

void
child_applicator_t::fill_status (ok_xstatus_t *out)
{
  if (_ok) {
    out->set_status (OK_STATUS_OK);
  } else {
    str s = join ("\n", _err);
    out->set_status (OK_STATUS_ERR);
    *out->error = s;
  }
}

//-----------------------------------------------------------------------

class kill_applicator_t : public child_applicator_t {
public:
  kill_applicator_t () {}
  void apply (const ok_prog_t &p, okch_t *ch, evs_t ev, CLOSURE);
};

//-----------------------------------------------------------------------

tamed void 
kill_applicator_t::apply (const ok_prog_t &p, okch_t *ch, evs_t ev) 
{ 
  ch->kill (); 
  ev->trigger (NULL);
}

//-----------------------------------------------------------------------
 
class custom1_applicator_t : public child_applicator_t {
public:
  custom1_applicator_t (const ok_custom_data_t *dat) : _data (dat) {}
  void apply (const ok_prog_t &prog, okch_t *ch, evs_t ev, CLOSURE);
private:
  const ok_custom_data_t *_data;
};
 
//-----------------------------------------------------------------------

tamed void
okd_t::custom1_in (svccb *sbp)
{
  tvars {
    RPC::okctl_program_1::okctl_custom_1_in_srv_t<svccb> srv (sbp);
    const ok_custom_arg_t *arg (srv.getarg ());
    custom1_applicator_t ca (&arg->data);
  ok_xstatus_t res;
  }
  twait { ca.run (&servtab, arg->progs, mkevent ()); }
  ca.fill_status (&res);
  srv.reply (res);
}

//-----------------------------------------------------------------------

tamed void
custom1_applicator_t::apply (const ok_prog_t &p, okch_t *ch, evs_t ev)
{
  tvars {
    str s;
  }
  twait { ch->custom1_out (*_data, mkevent (s)); }
  ev->trigger (s);
}

//-----------------------------------------------------------------------

class custom2_applicator_t : public child_applicator_t {
public:
  custom2_applicator_t (const ok_custom_data_t *dat) : _data (dat) {}
  void apply (const ok_prog_t &prog, okch_t *ch, evs_t ev, CLOSURE);
  void nochild (const ok_prog_t &p, evv_t ev, CLOSURE);
  const ok_custom_res_set_t &res () const { return _res; }
private:
  ok_custom_res_set_t _res;
  const ok_custom_data_t *_data;
};

//-----------------------------------------------------------------------

tamed void
custom2_applicator_t::nochild (const ok_prog_t &p, evv_t ev)
{
  ok_custom_res_t res;
  res.prog = p;
  res.res.set_status (OK_STATUS_NOSUCHCHILD);
  ev->trigger ();
}

//-----------------------------------------------------------------------

tamed void 
custom2_applicator_t::apply (const ok_prog_t &p, okch_t *ch, evs_t ev)
{
  tvars {
    str s;
    ok_custom_res_t res;
  }
  twait { ch->custom2_out (*_data, &res.res, mkevent (s)); }
  res.prog = p;
  _res.results.push_back (res);
  ev->trigger (s);
}

//-----------------------------------------------------------------------

tamed void
okd_t::custom2_in (svccb *sbp)
{
  tvars {
    RPC::okctl_program_1::okctl_custom_2_in_srv_t<svccb> srv (sbp);
    const ok_custom_arg_t *arg (srv.getarg ());
    custom2_applicator_t ca (&arg->data);
  }
  twait { ca.run (&servtab, arg->progs, mkevent ()); }
  srv.reply (ca.res ());
}

//-----------------------------------------------------------------------

void
okd_t::okctl_get_stats (svccb *sbp)
{
  okctl_stats_t res;
  res.status.setsize (servtab.size ());
  int i = 0;
  for (okch_t *p = servtab.first (); p; i++, p = servtab.next (p) ) {
    p->to_status_xdr (&res.status[i]);
  }
  sbp->replyref (res);
}

//-----------------------------------------------------------------------

bool
okd_ssl_t::init (int fd)
{
  bool rc = true;
  assert (fd >= 0);
  if (_fd < 0) {
    _fd = fd;
    _x = axprt_unix::alloc (fd);
    _cli = aclnt::alloc (_x, okssl_program_1);
    _srv = asrv::alloc (_x, okssl_program_1, 
			wrap (this, &okd_ssl_t::dispatch));
  } else {
    rc = false;
  }
  return rc;
}

//-----------------------------------------------------------------------

okd_mgrsrv_t::okd_mgrsrv_t (ptr<axprt_stream> xx, okd_t *p) : 
  x (xx), myokd (p)
{
  srv = asrv::alloc (x, okmgr_program_1, wrap (this, &okd_mgrsrv_t::dispatch));
}

//-----------------------------------------------------------------------

void
okd_mgrsrv_t::dispatch (svccb *sbp)
{
  if (!sbp) {
    delete this;
    return;
  }
  u_int p = sbp->proc ();
  switch (p) {
  case OKMGR_NULL:
    sbp->reply (NULL);
    break;
  case OKMGR_RELAUNCH:
    myokd->relaunch (sbp);
    break;
  case OKMGR_TURNLOG:
    myokd->turnlog (sbp);
    break;
  case OKMGR_CUSTOM_1:
    myokd->custom1_in (sbp);
    break;
  case OKMGR_CUSTOM_2:
    myokd->custom2_in (sbp);
    break;
  case OKMGR_LEAK_CHECKER:
    myokd->toggle_leak_checker (sbp);
    break;
  case OKMGR_PROFILER:
    myokd->toggle_profiler (sbp);
    break;
  default:
    sbp->reject (PROC_UNAVAIL);
    break;
  }
}

//-----------------------------------------------------------------------


void
okd_ssl_t::dispatch (svccb *sbp)
{
  if (!sbp) {
    warn << "okssld shut down; no more SSL connections accepted\n";
    hangup ();
  } else {
    switch (sbp->proc ()) {
    case OKSSL_NULL:
      {
	RPC::okssl_program_1::okssl_null_srv_t<svccb> srv (sbp);
	srv.reply ();
      }
      break;
    case OKSSL_NEW_CONNECTION:
      {
	int fd = _x->recvfd ();
	RPC::okssl_program_1::okssl_new_connection_srv_t<svccb> srv (sbp);
	const okssl_sendcon_arg_t &arg = *srv.getarg (); 
	ok_xstatus_typ_t res = OK_STATUS_OK;

	if (fd < 0) {
	  res = OK_STATUS_BADFD;
	} else if (arg.sin.size () < sizeof (sockaddr_in)) {
	  res = OK_STATUS_ERR;
	} else {
	  sockaddr_in *sin = (sockaddr_in *)xmalloc (sizeof (sockaddr_in));
	  memcpy (sin, arg.sin.base (), sizeof (sockaddr_in));
	  _okd->newserv2 (arg.port, fd, sin, true, &arg.ssl);
	}
	srv.reply (res);
      }
      break;
    default:
      sbp->reject (PROC_UNAVAIL);
      break;
    }
  }
}

//-----------------------------------------------------------------------

tamed void
okd_ssl_t::toggle_accept (bool arg)
{
  tvars {
    ok_xstatus_typ_t res;
    clnt_stat err;
  }

  if (_cli) {
    twait {
      RPC::okssl_program_1::okssl_toggle_accept (_cli, arg, &res, 
						 mkevent (err));
    }
    if (err) {
      warn << "RPC failure in toggling SSL accept: " << err << "\n";
    } else if (res != OK_STATUS_OK) {
      warn << "okssld failed to toggle accept (";
      rpc_print (warnx, res, NULL, 0, NULL);
      warnx << ")\n";
    }
  }
}

//-----------------------------------------------------------------------

void
okd_ssl_t::hangup ()
{
  _fd = -1;
  _x = NULL;
  _cli = NULL;
  _srv = NULL;
}

//-----------------------------------------------------------------------

tamed void
okd_t::awaken (str nm, evb_t ev)
{
  tvars {
    okws_svc_descriptor_t arg;
    ok_xstatus_typ_t res;
    clnt_stat err;
    bool ok (false);
  }
  
  if (_okld_cli) {
    arg.pid = 0;
    arg.name = nm;
    twait {
      RPC::okld_program_1::okld_poke_lazy_service (_okld_cli, arg, &res, 
						   mkevent (err)); 
    }
    if (err) {
      warn << "RPC error awakening service " << nm << ": " << err << "\n";
    } else if (res != OK_STATUS_OK) {
      warn << "okld did not awaken service " << nm 
	   << ": rc=" << int (res) << "\n";
    } else {
      ok = true;
    }
  }
  ev->trigger (ok);
}

//-----------------------------------------------------------------------

tamed void
okd_t::relaunch (svccb *sbp)
{
  tvars {
    RPC::okmgr_program_1::okmgr_relaunch_srv_t<svccb> srv (sbp);
    const ok_progs_t *progs (srv.getarg ());
    kill_applicator_t ka;
    ok_xstatus_t ret;
  }
  twait { ka.run (&servtab, *progs, mkevent ()); }
  ka.fill_status (&ret);
  srv.reply (ret);
}

//-----------------------------------------------------------------------

tamed void
okd_t::toggle_leak_checker (svccb *sbp)
{
  tvars {
    RPC::okmgr_program_1::okmgr_leak_checker_srv_t<svccb> srv (sbp);
    const okmgr_diagnostic_arg_t *arg (srv.getarg ());
    ok_xstatus_typ_t status;
    ok_xstatus_t ret;
    okch_t *ch;
  }
  
  if ((ch = servtab[arg->prog])) {
    twait { ch->toggle_leak_checker (arg->cmd, mkevent (status)); }
  } else {
    status = OK_STATUS_NOSUCHCHILD;
  }
  ret.set_status (status);
  srv.reply (ret);
}

//-----------------------------------------------------------------------

tamed void
okd_t::toggle_profiler (svccb *sbp)
{
  tvars {
    RPC::okmgr_program_1::okmgr_profiler_srv_t<svccb> srv (sbp);
    const okmgr_diagnostic_arg_t *arg (srv.getarg ());
    ok_xstatus_typ_t status;
    ok_xstatus_t ret;
    okch_t *ch;
  }
  
  if ((ch = servtab[arg->prog])) {
    twait { ch->toggle_profiler (arg->cmd, mkevent (status)); }
  } else {
    status = OK_STATUS_NOSUCHCHILD;
  }
  ret.set_status (status);
  srv.reply (ret);
}

//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  sfs_rxx_panic = false; // don't panic on RXX failures
  start_okd (argc, argv);
  amain ();
}

//-----------------------------------------------------------------------
