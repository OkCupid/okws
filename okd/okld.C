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

#include "okld.h"
#include <sys/types.h>
#include <grp.h>
#include <unistd.h>
#include "parseopt.h"
#include "sfsmisc.h"
#include "okprot.h"
#include "ahutil.h"
#include "pub.h"
#include "xpub.h"
#include "pubutil.h"
#include "okconst.h"

void
okld_t::set_signals ()
{
  sigcb (SIGTERM, wrap (this, &okld_t::shutdown, SIGTERM));
  sigcb (SIGINT,  wrap (this, &okld_t::shutdown, SIGINT));
}

void
okld_t::got_service (vec<str> s, str loc, bool *errp)
{
  if (s.size () != 3 && s.size () != 4) {
    warn << loc << ": usage: Service <exec-path> <URI> [<uid>]\n";
    *errp = true;
    return;
  }
  if (!is_safe (s[1])) {
    warn << loc << ": Service path (" << s[1] 
	 << ") contains unsafe substrings\n";
    *errp = true;
    return;
  }

  int svc_uid = -1;
  ok_usr_t *u = NULL;
  if (s.size () == 4) {
    if (convertint (s[3], &svc_uid)) {
      u = New ok_usr_t (svc_uid);
    } else {
      u = New ok_usr_t (s[3]);
      if (!*u) {
	warn << loc << ": cannot find user " << s[3] << "\n";
	*errp = true;
	delete u;
	return;
      }
    }
  }
  str exe = apply_container_dir (service_bin, s[1]);
  str httppath = re_fslash (s[2].cstr ());
  warn << "Service: URI(" << httppath << ") --> unix(" << exe << ")\n";
  vNew okld_ch_t (exe, httppath, this, loc, u);
}

void
okld_t::parseconfig (const str &cf)
{
  warn << "using config file: " << cf << "\n";
  parseargs pa (cf);
  bool errors = false;

  int line;
  vec<str> av;

  conftab ct;
  str grp;
  str okd_gr, okd_un;

  ct.add ("Service", wrap (this, &okld_t::got_service))
    .add ("TopDir", wrap (got_dir, &topdir))
    .add ("JailDir", wrap (got_dir, &jaildir))
    .add ("ServiceBin", wrap (got_dir, &service_bin))

    .add ("CrashSamplingInterval", &ok_csi, 1, 60)
    .add ("MaxCrashedProcesses", &ok_crashes_max, 1, 200)
    .add ("ServiceLowUid", &ok_svc_uid_low, UID_MIN, UID_MAX)
    .add ("ServiceHighUid", &ok_svc_uid_high, UID_MIN, UID_MAX)
    .add ("ServiceMode", &ok_svc_mode, 0, 0777)
    .add ("ServiceGroup", &grp)
    .add ("OkdExecPath", &okdexecpath)
    .add ("CoreDumpDir", wrap (got_dir, &coredumpdir))
    .add ("SocketDir", wrap (got_dir, &sockdir))
    .add ("BindAddr", wrap (static_cast<ok_base_t *> (this), 
			    &ok_base_t::got_bindaddr))
    .add ("OklogdExecPath", wrap (this, &okld_t::got_logd_exec))
    .add ("LogDir", &logd_parms.logdir)
    .add ("AccessLog", &logd_parms.accesslog)
    .add ("ErrorLog", &logd_parms.errorlog)
    .add ("AccessLogFmt", &logd_parms.accesslog_fmt)
    .add ("SvcLog", &logd_parms.svclog)
    .add ("OklogdUser", &logd_parms.user)
    .add ("OklogdGroup", &logd_parms.group)
    .add ("UnsafeMode", &unsafe_mode)
    .add ("SafeStartup", &safe_startup_fl)
    .add ("FilterCGI", &ok_filter_cgi, XSSFILT_NONE, XSSFILT_ALL)
    .add ("SfsClockMode", wrap (got_clock_mode, &clock_mode))
    .add ("MmapClockDaemon", &mmcd)
    .add ("MmapClockFile", &mmc_file)

    .add ("SyscallStatDumpInterval", &ok_ssdi, 0, 1000)

    .add ("OkdUser", &okd_un)
    .add ("OkdGroup", &okd_gr)
    .add ("ShutdownTimeout", &ok_shutdown_timeout, 0, 1000)
    
    .add ("OkdName", &reported_name)
    .add ("OkdVersion", &version)
    .add ("HostName", &hostname)

    .add ("LogTick", &ok_log_tick, 1, 4000)
    .add ("LogPeriod", &ok_log_period, 1, 100)

    .add ("ClientTimeout", &ok_clnt_timeout, 1, 400)

    .add ("Gzip", &ok_gzip)
    .add ("GzipLevel", &ok_gzip_compress_level, Z_NO_COMPRESSION,
	  Z_BEST_COMPRESSION)
    .add ("GzipSmallStrLen", &ok_gzip_smallstr, 0, 0x8000)
    .add ("GzipCacheMin", &ok_gzip_cache_minstr, 0, 0x10000)
    .add ("GzipCacheMax", &ok_gzip_cache_maxstr, 0, 0x1000000)
    .add ("GzipCacheSize", &ok_gzip_cache_storelimit, 0, 0x10000000)
    .add ("GzipMemLevel", &ok_gzip_mem_level, 0, 9)
    .add ("ChannelLimit", &ok_reqsize_limit, OK_RQSZLMT_MIN, OK_RQSZLMT_MAX)
    .add ("SendSockAddrIn", &ok_send_sin)

    .add ("ServiceMaxFDs", &ok_svc_max_fds, 0, 10240)
    .add ("ServiceFDHighWat", &ok_svc_fds_high_wat, 0, 10000)
    .add ("ServiceAcceptMessages", &ok_svc_accept_msgs)

    .ignore ("Alias")
    .ignore ("MaxConQueueSize")
    .ignore ("ListenQueueSize")
    .ignore ("OkMgrPort")
    .ignore ("PubdExecPath")
    .ignore ("ErrorDoc")
    .ignore ("OkdMaxFDs")
    .ignore ("OkdFDHighWat")
    .ignore ("OkdAcceptMessages")
    .ignore ("OkdChildSelectDisable");


  if (grp) svc_grp = ok_grp_t (grp);
  if (okd_un) okd_usr = ok_usr_t (okd_un);
  if (okd_gr) okd_grp = ok_grp_t (okd_gr);

  while (pa.getline (&av, &line)) {
    if (!ct.match (av, cf, line, &errors)) {
      warn << cf << ":" << line << ": unknown config parameter\n";
      errors = true;
    }
  }
  nxtuid = ok_svc_uid_low;

  if (!hostname)
    hostname = myname ();

  if (errors)
    exit (1);
}

void
okld_t::launch_logd (cbb cb)
{
  if (!logexc) 
    logexc = New helper_exec_t (oklog_program_1, "oklogd", 1,
				HLP_OPT_PING|HLP_OPT_QUEUE|HLP_OPT_NORETRY);
  vec<str> *argv = logexc->get_argv ();
  assert (argv);
  argv->push_back (logd_parms.encode ());
  logd = New log_primary_t (logexc);
  logd->connect (cb);
}

void
okld_t::encode_env ()
{
  env.insert ("hostname", hostname)
    .insert ("jaildir", jaildir)
    .insert ("version", version)
    .insert ("listenport", listenport)
    .insert ("okdname", reported_name)
    .insert ("jailed", int (jaildir && !uid))
    .insert ("logfmt", logd_parms.accesslog_fmt)
    .insert ("svclog", int (logd_parms.svclog))
    .insert ("gzip", ok_gzip ? 1 : 0)
    .insert ("gziplev", ok_gzip_compress_level)
    .insert ("gzipcsl", ok_gzip_cache_storelimit)
    .insert ("logtick", ok_log_tick)
    .insert ("logprd", ok_log_period)
    .insert ("clito", ok_clnt_timeout)
    .insert ("reqszlimit", ok_reqsize_limit)
    .insert ("filtercgi", ok_filter_cgi ? 1 : 0)
    .insert ("ssdi", ok_ssdi)
    .insert ("sendsin", ok_send_sin ? 1 : 0)
    .insert ("maxfds", ok_svc_max_fds)
    .insert ("fdhw", ok_svc_fds_high_wat)
    .insert ("acmsg", ok_svc_accept_msgs)
    .insert ("clock", int (clock_mode));

  if (mmc_file)
    env.insert ("mmcf", mmc_file);
}


void
okld_t::got_logd_exec (vec<str> s, str loc, bool *errp)
{
  if (s.size () < 1) {
    warn << loc << ": usage: OklogdExecPath <path-to-oklogd>\n";
    *errp = true;
    return;
  }

  if (!is_safe (s[1])) {
    warn << loc << ": Log exec path (" << s[1]
	 << ") contains unsafe substrings\n";
    *errp = true;
    return;
  }

  str prog = okws_exec (s[1]);
  str err = can_exec (prog);
  if (err) {
    warn << loc << ": cannot open oklogd: " << err << "\n";
    *errp = true;
  } else {
    s.pop_front ();
    s[0] = prog;
    logexc = New helper_exec_t (oklog_program_1, s, 1,
				HLP_OPT_PING|HLP_OPT_QUEUE|HLP_OPT_NORETRY);
  }
}

bool
okld_t::fix_uids ()
{
  if (!will_jail ())
    return true;

  bool ret = true;
  nxtuid = ok_svc_uid_low;
  int lim = svcs.size ();
  bhash<u_int> uids_in_use;
  bhash<u_int> owners;
  if (lim > ok_svc_uid_high - ok_svc_uid_low && !unsafe_mode) {
    warn << "too many services / ran out of UIDs\n";
    return false;
  }

  // struct passwd *pw;
  // struct group *gr;
  okld_ch_t *svc;
  for (int i = 0; i < lim; i++) {
    svc = svcs[i];

    int ueuid_orig = svc->get_exec_uid ();
    int ueuid_own = ueuid_orig;
    int uegid_orig = svc->get_exec_gid ();
    int uegid = uegid_orig;
    int mode = svc->get_exec_mode ();

    if (unsafe_mode) {
      svc->assign_uid (ueuid_own);
      continue;
    }

    while ((getpwuid (uegid) ||  getgrgid (uegid) ||
	    uids_in_use[uegid] || owners[uegid]) && uegid <= ok_svc_uid_high) {
      warn << "UID/GID=" << uegid
	   << " is in use; refusing to use it.\n";
      uegid = ++nxtuid;
    }

    if (uegid > ok_svc_uid_high) {
      warn << "too many services / ran out of UIDs\n";
      return false;
    }

    uids_in_use.insert (uegid);

    if (uids_in_use[ueuid_own]) {
      warn << svc->loc () << ": UID " << ueuid_own
	   << " in use / changing ownership on file to root\n";
      ueuid_own = 0;
    } 

    owners.insert (ueuid_own);
    svc->assign_exec_ownership (ueuid_own, uegid);
    if ((ueuid_own != ueuid_orig || uegid_orig != uegid) && !svc->chown ())
      ret = false;

    // owner cannot exec this script; group -- only -- can
    svc->assign_mode (ok_svc_mode);
    if (!svc->chmod (mode))
      ret = false;
    svc->assign_uid (uegid);
    svc->assign_gid (uegid);

  }
  endgrent ();
  endpwent ();
  return ret;
}

void
okld_t::shutdown (int sig)
{
  sdflag = true;
  warn << "received shutdown signal (" << sig << "); shutting down\n";
  okdx = NULL;
}

void
okld_t::shutdown2 (int status)
{
  if (status != 0)
    warn << "okd exitted uncleanly, with status=" << status << "\n";
  else
    warn << "caught clean okd exit\n";
  warn << "shutdown complete\n";

  // turn of the clock daemon cleanly. not entirely necessary.
  if (mmcd_pid > 0) {
    close (mmcd_ctl_fd);
  }

  delete this;
  exit (0);
}

bool
okld_t::launch_okd (int logfd)
{
  str prog = okws_exec (okdexecpath);
  str err = can_exec (prog);
  if (err) 
    fatal << "OKD execute error: " << err << "\n";
  vec<str> argv;
  argv.push_back (prog);
  argv.push_back ("-f");
  argv.push_back (configfile);
  argv.push_back ("-l");
  argv.push_back (strbuf () << logfd);
  argv.push_back ("-c");
  argv.push_back (okd_dumpdir);
		  
  if (debug_stallfile) {
    argv.push_back ("-D");
    argv.push_back (strbuf (debug_stallfile) << ".okd");
  }

  // launch okd synchronously; no point in us running if okd puked.
  ptr<axprt_unix> x = axprt_unix_spawnv (prog, argv, 0, NULL, false);
  if (!x)
    return false;

  // we've passed this to OKD; don't need it anymore
  close (logfd);

  // two shutdown events -- first on close of fdsource_t<> on
  // okd.  Second, is okd's actual exit, which we'll wait on.
  okd_pid = axprt_unix_spawn_pid;
  chldcb (okd_pid, wrap (this, &okld_t::shutdown2));
  okdx = fdsink_t::alloc (x, wrap (this, &okld_t::shutdown, SIGTERM));
  return true;
}

bool
okld_t::checkservices ()
{
  if (!will_jail ())
    return true;

  bool ret = true;
  u_int lim = svcs.size ();
  for (u_int i = 0; i < lim; i++) {
    // at this point in the boot process, we have not called chroot yet
    if (!svcs[i]->can_exec (false))
      ret = false;
  }
  return ret;
}

static void
usage ()
{
  warnx << "usage: okld [-q?] [-D <dbg>] [-f <configfile>]\n";
  exit (1);
}

int
main (int argc, char *argv[])
{
  bool opt_daemon = false;
  str cf;
  setprogname (argv[0]);
  str dbf;

  int ch;
  while ((ch = getopt (argc, argv, "qf:D:?")) != -1)
    switch (ch) {
    case 'D':
      dbf = optarg;
      break;
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
  if (!cf) {
    cf = sfsconst_etcfile ("okws_config");
    if (!cf)
      cf = sfsconst_etcfile_required ("okd_config");
  }

  if (opt_daemon) {
    syslog_priority = ok_syslog_priority;
    daemonize ();
  }

  warn ("version %s, pid %d\n", VERSION, int (getpid ()));
  okld_t *okld = New okld_t ();
  okld->debug_stallfile = dbf;
  okld->set_signals ();
  okld->launch (cf);
  amain ();
}

void
okld_t::launch (const str &cf)
{
  configfile = cf;
  parseconfig (cf);

  init_clock_daemon ();

  encode_env ();
  if (!(checkservices () && fix_uids () && init_jaildir ()))
    exit (1);
  if (jaildir)
    warn ("JailDirectory: %s\n", jaildir.cstr ());
  launch_logd (wrap (this, &okld_t::launch_logd_cb));
}

void
okld_t::launch_logd_cb (bool rc)
{
  if (!rc) {
    warn << "launch of log daemon (oklogd) failed\n";
    exit (1);
  }
  logd->clone (wrap (this, &okld_t::launch_logd_cb2));

}

void
okld_t::launch_logd_cb2 (int logfd)
{
  if (logfd < 0) {
    warn << "oklogd did not send a file descriptor; aborting\n";
    exit (1);
  }
  launch2 (logfd);
}

void
okld_t::launch2 (int logfd)
{
  if (!launch_okd (logfd))
    exit (1);
  
  chroot ();
  launchservices ();
}

void
okld_t::launchservices ()
{
  if (sdflag) { 
    warn << "not launching due to shutdown flag\n";
    return;
  }

  warn << "launching services (" << launchp << ")\n";

  u_int lim = svcs.size ();
  for (u_int i = 0; i < 30 && launchp < lim; i++) {
    svcs[launchp++]->launch ();
  }

  //
  // XXX - hack for now; there is a problem when we launch too many
  // services at once, in that we lose file descriptors around the
  // 128th FD is sent.  this is probably not a coincindence, and is
  // most likely limited to some kernel limit as to the number of
  // outstanding FDs allowed.  Note that **okd** is failing to 
  // retrieve the 128th file descriptor. Some small changes
  // to david's libraries might be able to fix this. For now,
  // we'll hack in a timeout to let okd catch up, if possible.
  //
  if (launchp < lim) {
    // XXX - should be configurable, too (as should the hard
    // coded limit of 30, as given above).
    delaycb (1, 0, wrap (this, &okld_t::launchservices));
  }
}

bool
okld_t::init_jaildir ()
{
  if (!will_jail ())
    return true;
  ok_usr_t root ("root");
  ok_grp_t wheel ("wheel");

  if (!root || !wheel) {
    warn << "cannot access root.wheel in /etc/passwd\n";
    return false;
  }

  root_coredir = apply_container_dir (coredumpdir, "0");

  int ret = true;
  if (!jail_mkdir ("/etc/", 0755, &root, &wheel) ||  
      !jail_cp ("/etc/resolv.conf", 0644, &root, &wheel) || 
      !jail_mkdir_p (coredumpdir, 0755, &root, &wheel) ||
      !jail_mkdir (root_coredir, 0700, &root, &wheel) ||
      !jail_mkdir_p (sockdir, 0755, &root, &wheel)) 
    ret = false;

  u_int lim = svcs.size ();
  if (!svc_grp) {
    warn << "cannot find service group (" << svc_grp.getname () 
	 << " does not exist)\n";
    return false;
  }

  if (!okd_usr) {
    warn << "cannot find a user for okd (" << okd_usr.getname () 
	 << " does not exist)\n";
    return false;
  }

  if (!okd_grp) {
    warn << "cannot find group for okd (" << okd_grp.getname ()
	 << "does not exist)\n";
    return false;
  }

  okd_dumpdir = apply_container_dir (coredumpdir, 
				     strbuf () << okd_usr.getid ());
  if (!jail_mkdir (okd_dumpdir, 0700, &okd_usr, &okd_grp)) {
    warn << "cannot allocate jail directory for okd\n";
    return false;
  }

  // separate coredump directory for each service UID
  for (u_int i = 0; i < lim; i++) {
    str d = apply_container_dir (coredumpdir, 
				 strbuf () << svcs[i]->usr ()->getid ());
    svcs[i]->set_run_dir (d);
    if (!jail_mkdir (d, 0700, svcs[i]->usr (), &svc_grp))
      ret = false;
  }

  return ret;
}

void
okld_t::clock_daemon_died (int sig)
{
  warn << "mmcd died with status=" << sig << "\n";
  assert (mmcd_pid > 0);
  chldcb (mmcd_pid, NULL);
  mmcd_pid = -1;
}

void
okld_t::relaunch_clock_daemon (int sig)
{
  clock_daemon_died (sig);
  init_clock_daemon ();
}


void
okld_t::init_clock_daemon ()
{
  assert (mmcd_pid < 0);

  if (clock_mode == SFS_CLOCK_MMAP) {

    //
    // second argument false; we haven't jailed yet.
    //
    str f = jail2real (ok_mmc_file, false);
    const char *args[] = { mmcd.cstr (), f.cstr (), NULL };

    // note that we're running the clock daemon as root.
    // this should be fine. 
    warn << "*unstable: launching mmcd: " << args[0] << " " << args[1] << "\n";

    int fds[2];
    bool ok = false;
    if (socketpair (AF_UNIX, SOCK_STREAM, 0, fds) != 0) { 
      warn ("cannot created socketpair: %m\n");
    } else {
      close_on_exec (fds[0]);
      if ((mmcd_pid = spawn (args[0], args, fds[1])) < 0) {
	close (fds[0]);
	warn ("cannot start mmcd %s: %m\n", args[0]);
      } else {
	mmcd_ctl_fd = fds[0];
	str foo ("foorbad adsfi qoewirj oiasjdf oijqweroijqweorijqwer\n");
	write (mmcd_ctl_fd, foo.cstr (), foo.len ());
	ok = true;
      }
      close (fds[1]);
    }

    if (ok) {

      // okld does not change its clock type, since it's mainly idle.
      chldcb (mmcd_pid, wrap (this, &okld_t::clock_daemon_died));
      
      // okld will not be relaunching the clock daemon if it crashes;
      // to do so, it would have to copy it into the chroot jail;
      // this is easy enough to do, but i can't imagine a case in which 
      // the clock daemon would actually die, so it's just not worth it.
      //
      //chldcb (mmcd_pid, wrap (this, &okld_t::relaunch_clock_daemon));

    } else {
      clock_mode = SFS_CLOCK_GETTIME;
    }
  }

}
