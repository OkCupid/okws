
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

static void srepub (ptr<ok_repub_t> rpb, okch_t *ch) { ch->repub (rpb); }
static void srelaunch (ptr<ok_res_t> res, okch_t *ch) { ch->relaunch (res); }
static void launchservice (okch_t *s) { s->launch (); }

static void
set_signals (okd_t *okd)
{
  sigcb (SIGTERM, wrap (okd, &okd_t::shutdown, SIGTERM));
  sigcb (SIGINT,  wrap (okd, &okd_t::shutdown, SIGINT));
}

okd_t::~okd_t ()
{
  if (logd) delete logd;
  if (pubd) delete pubd;
  delete pprox;
}

void
okd_t::got_bindaddr (vec<str> s, str loc, bool *errp)
{
  in_addr addr;
  if (s.size () < 2 || s.size () > 3 ||
      !inet_aton (s[1], &addr) ||
      (s.size () == 3 && !convertint (s[2], &listenport))) {
    warn << loc << ": usage: BindAddr addr [port]\n";
    *errp = true;
    return;
  }
  listenaddr_str = s[1];
  listenaddr = ntohl (addr.s_addr);
}

void
okd_t::got_pubd_unix (vec<str> s, str loc, bool *errp)
{
  str name;
  if (s.size () != 2 || access (s[1], R_OK) != 0) {
    warn << loc << ": usage: PubdUnix <socketpath>\n";
    *errp = true;
  } else {
    pubd = New helper_unix_t (pub_program_1, s[1]);
  }
}

void
okd_t::got_pubd_exec (vec<str> s, str loc, bool *errp)
{
  if (s.size () <= 1 || access (s[1], X_OK) != 0) {
    warn << loc << ": usage: PubdExecPath <path-to-pubd>\n";
    *errp = true;
  } else {
    s.pop_front ();
    pubd = New helper_exec_t (pub_program_1, s);
  }
}

void
okd_t::got_logd_exec (vec<str> s, str loc, bool *errp)
{
  if (s.size () <= 1 || access (s[1], X_OK) != 0) {
    warn << loc << ": usage: OklogdExecPath <path-to-oklogd>\n";
    *errp = true;
  } else {
    s.pop_front ();
    lexc = New helper_exec_t (oklog_program_1, s, 1,
			      HLP_OPT_PING|HLP_OPT_QUEUE|HLP_OPT_NORETRY);
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

  vec<str> *s = pubd->get_argv ();
  if (s && pdjdir) {
    str jd;
    if (pdjdir[0] == '/' || !jaildir) {
      jd = pdjdir;
    } else {
      jd = strbuf (jaildir) << "/" << pdjdir;
    }
    s->push_back ("-j");
    s->push_back (jd);
  }
  pubd->connect (wrap (this, &okd_t::launch_pubd_cb));
}

void
okd_t::launch_logd ()
{
  if (!lexc) 
    lexc = New helper_exec_t (oklog_program_1, "oklogd", 1,
			      HLP_OPT_PING|HLP_OPT_QUEUE|HLP_OPT_NORETRY);
  vec<str> *argv = lexc->get_argv ();
  assert (argv);
  argv->push_back (logd_parms.encode ());
  logd = New log_primary_t (lexc);
  logd->connect (wrap (this, &okd_t::launch_logd_cb));
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
okd_t::launch_logd_cb (bool rc)
{
  if (!rc) {
    warn << "launch of log daemon (oklogd) failed.\n";
    exit (1);
  }
  if (!--launches)
    launch2 ();
}

static void
got_dir (str *out, vec<str> s, str loc, bool *errp)
{
  if (s.size () != 2) {
    warn << loc << ": usage: " << s[0] << " <path>\n";
    *errp = true;
    return;
  }
  *out = dir_standardize (s[1]);
}

str
okd_t::make_execpath (const str &exe, bool chrt)
{
  if (chrt && ((!uid && jaildir) || jailed)) return exe;
  else {
    strbuf b (jaildir);
    if (exe[0] != '/') b << "/";
    b << exe;
    return b;
  }
}

static str 
re_fslash (const char *cp)
{
  while (*cp == '/' && *cp) cp++;
  return strbuf ("/") << cp;
}


void
okd_t::got_cgiserver (vec<str> s, str loc, bool *errp)
{
  if (s.size () != 3) {
    warn << loc << ": usage: Service <exec-path> <URI>\n";
    *errp = true;
    return;
  }
  str exe = re_fslash (s[1].cstr ());
  str execpath = make_execpath (exe, false);

  str cgipath = re_fslash (s[2].cstr ());
  vNew okch_t (exe, cgipath, this, loc);
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
okd_t::parseconfig (const str &cf)
{
  warn << "using config file: " << cf << "\n";
  parseargs pa (cf);
  bool errors = false;

  int line;
  vec<str> av;

  conftab ct;
  ct.add ("BindAddr", wrap (this, &okd_t::got_bindaddr))
    .add ("CGIServer", wrap (this, &okd_t::got_cgiserver))
    .add ("Service", wrap (this, &okd_t::got_cgiserver))
    .add ("Alias", wrap (this, &okd_t::got_alias))
    .add ("JailDir", wrap (got_dir, &jaildir))
    .add ("HostName", &hostname)
    .add ("TopDir", &topdir)
    .add ("MaxConQSize", &ok_con_queue_max, OK_QMIN, OK_QMAX)
    .add ("CrashSamplingInterval", &ok_csi, 1, 60)
    .add ("MaxCrashedProcesses", &ok_crashes_max, 1, 200)
    .add ("OkMgrPort", &ok_mgr_port, OK_PORT_MIN, OK_PORT_MAX)
    .add ("ListenQueueSize", &ok_listen_queue_max, OK_QMIN, OK_QMAX)

    .add ("PubdUnix", wrap (this, &okd_t::got_pubd_unix))
    .add ("PubdInet", wrap (this, &okd_t::got_pubd_inet))
    .add ("PubdJaildir", wrap (got_dir, &pdjdir))
    .add ("PubdExecPath", wrap (this, &okd_t::got_pubd_exec))

    .add ("OklogdExecPath", wrap (this, &okd_t::got_logd_exec))
    .add ("OklogdSockname", &logd_parms.sockname)
    .add ("LogDir", &logd_parms.logdir)
    .add ("AccessLog", &logd_parms.accesslog)
    .add ("ErrorLog", &logd_parms.errorlog)
    .add ("AccessLogFmt", &logd_parms.accesslog_fmt)
    .add ("OklogdUser", &logd_parms.user)
    .add ("OklogdGroup", &logd_parms.group)

    .add ("OkdName", &okdname)
    .add ("OkdVersion", &version)
    .add ("OkdUser", &okd_usr.name)
    .add ("OkdGroup", &okd_grp.name)
    .add ("ServiceUser", &svc_usr.name)
    .add ("ServiceGroup", &svc_grp.name)

    .add ("Gzip", &ok_gzip)
    .add ("GzipLevel", &ok_gzip_compress_level, Z_NO_COMPRESSION,
	  Z_BEST_COMPRESSION)
    .add ("GzipSmallStrLen", &ok_gzip_smallstr, 0, 0x8000)
    .add ("GzipCacheMin", &ok_gzip_cache_minstr, 0, 0x10000)
    .add ("GzipCacheMax", &ok_gzip_cache_maxstr, 0, 0x1000000)
    .add ("GzipCacheSize", &ok_gzip_cache_storelimit, 0, 0x10000000)
    .add ("GzipMemLevel", &ok_gzip_mem_level, 0, 9)

    .add ("LogTick", &ok_log_tick, 1, 4000)
    .add ("LogPeriod", &ok_log_period, 1, 100)

    .add ("ClientTimeout", &ok_clnt_timeout, 1, 400);

  while (pa.getline (&av, &line)) {
    if (!ct.match (av, cf, line, &errors)) {
      warn << cf << ":" << line << ": unknown config parameter\n";
      errors = true;
    }
  }
  if (!hostname)
    hostname = myname ();
  if (errors)
    exit (1);
}

void
okd_t::chroot ()
{
  if (jaildir && !uid) {
    ::chroot (jaildir);
    jailed = true;
  }
}

void
okd_t::set_svc_ids ()
{
  if (!uid) {
    setgid (svc_grp.id);
    setuid (svc_usr.id);
  }
}

void
okd_t::set_okd_ids ()
{
  if (!uid) {
    setgid (okd_grp.id);
    setuid (okd_usr.id);
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
okd_t::launchservices ()
{
  int fd = inetsocket (SOCK_STREAM, listenport, listenaddr);
  listenfd = fd;
  encode_env ();
  if (fd < 0)
    fatal ("could not bind TCP port %d: %m\n", listenport);
  close_on_exec (fd);
  listen (fd, ok_listen_queue_max);
  warn << "listening on " << listenaddr_str << ":" << listenport << "\n";
  fdcb (fd, selread, wrap (this, &okd_t::newserv, fd));

  chroot ();
  set_svc_ids (); // used to be set_okd_ids

  chkcnt = servtab.size ();
  if (chkcnt == 0)
    fatal << "no Services specified in configuration file.\n";
  servtab.traverse (wrap (this, &okd_t::checkservice));
}

void
okd_t::stop_listening ()
{
  fdcb (listenfd, selread, NULL);
  close (listenfd);
}

void
okd_t::checkservice (okch_t *s)
{
  if (!s->can_exec ())
    bdlnch = true;
  if (!--chkcnt)
    launchservices2 ();
}

void
okd_t::launchservices2 ()
{
  if (bdlnch)
    fatal << "did not have permission to launch all servers\n";
  servtab.traverse (wrap (launchservice));
}

static void
usage ()
{
  warnx << "usage: okd [-q?] [-f <configfile>]\n";
  exit (1);
}

int
main (int argc, char *argv[])
{
  bool opt_daemon = false;
  str cf;
  setprogname (argv[0]);

  int ch;
  while ((ch = getopt (argc, argv, "qf:?")) != -1)
    switch (ch) {
    case 'f':
      if (cf)
	usage ();
      cf = optarg;
      break;
    case 'q':
      opt_daemon = true;
      break;
    case '?':
    default:
      usage ();
    }

  argc -= optind;
  argv += optind;

  if (argc > 1)
    usage ();

  sfsconst_init ();
  if (!cf)
    cf = sfsconst_etcfile_required ("okd_config");

  if (opt_daemon) {
    syslog_priority = "local3.info";
    daemonize ();
  }

  warn ("version %s, pid %d\n", VERSION, int (getpid ()));
  okd_t *okd = New okd_t ();
  set_signals (okd);
  okd->launch (cf);
  amain ();
}

void
okd_t::launch (const str &cf)
{
  parseconfig (cf);
  get_runas (cf);
  open_mgr_socket ();

  launches = 2;
  launch_pubd ();
  launch_logd ();
}

void
okd_t::launch2 ()
{
  launchservices ();
}

void
okd_t::get_runas (const str &cf)
{
  if (uid)
    return;

  if (!okd_usr)
    fatal << cf << ": please specify a valid username for \"OkdUser\"\n";
  if (!okd_grp)
    fatal << cf << ": please specify a valid group for \"OkdGroup\"\n";
  if (!svc_usr)
    fatal << cf << ": please specify a valid username for \"ServiceUser\"\n";
  if (!svc_grp)
    fatal << cf << ": please specify a valid groupname for \"ServiceGroup\"\n";
}

void
okd_t::encode_env ()
{
  env.insert ("hostname", hostname)
    .insert ("jaildir", jaildir)
    .insert ("version", version)
    .insert ("listenport", listenport)
    .insert ("okdname", okdname)
    .insert ("jailed", int (jaildir && !uid))
    .insert ("logfmt", logd_parms.accesslog_fmt)
    .insert ("gzip", ok_gzip ? 1 : 0)
    .insert ("gziplev", ok_gzip_compress_level)
    .insert ("gzipcsl", ok_gzip_cache_storelimit)
    .insert ("logtick", ok_log_tick)
    .insert ("logprd", ok_log_period)
    .insert ("clito", ok_clnt_timeout);
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
      else o->relaunch (res);
    }
  }
  (*cb) (res);
}

