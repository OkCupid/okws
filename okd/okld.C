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
#include <grp.h>
#include <unistd.h>
#include "okconst.h"
#include "okld.h"
#include "parseopt.h"
#include "sfsmisc.h"
#include "okprot.h"
#include "ahutil.h"
#include "pub.h"
#include "xpub.h"
#include "pubutil.h"
#include "rxx.h"

extern char ** environ;
extern int optind;

void
okld_t::set_signals ()
{
  sigcb (SIGTERM, wrap (this, &okld_t::shutdown, SIGTERM));
  sigcb (SIGINT,  wrap (this, &okld_t::shutdown, SIGINT));
}

static void
get_env_params (vec<str> *in, vec<str> *out)
{
  while (in->size () && strchr ((*in)[0].cstr (), '='))
    out->push_back (in->pop_front ());
}


void
okld_ch_t::add_args (const vec<str> &v)
{
  for (u_int i = 0; i < v.size (); i++) 
    args.push_back (v[i]);
}

bool
okld_t::check_uri (const str &loc, const str &in, okws1_port_t *port) const
{
  u_int32_t t;
  if (port)
    *port = 0;
  static rxx uri_spec ("((:(\\d+))?/)?([a-zA-Z0-9/._~-]+)");

  if (in.len () > OK_MAX_URI_LEN) {
    warn << loc << ": service URI (" << in
	 << ") exceeds maximum allowable length (" << OK_MAX_URI_LEN
	 << ")\n";
    return false;
  }

  if (!uri_spec.match (in)) {
    warn << loc << ": malformed service URI: " << in << "\n";
    return false;
  }

  if (uri_spec[3]) {
    if (!convertint (uri_spec[3], &t) && t > PORT_MAX) {
      warn << loc << ": port out of range: " << uri_spec[3] << "\n";
      return false;
    }
    if (port)
      *port = t;
  }
  return true;
}

void
okld_t::got_okd_exec (vec<str> s, str loc, bool *errp)
{
  //
  // pop off "OkdExec"
  //
  str es;
  s.pop_front ();
  vec<str> env;

  static rxx envsep ("=");

  bool fnd_dbg = false;

  while (s.size () && strchr (s[0].cstr (), '=')) {
    vec<str> tmp;
    assert (split (&tmp, envsep, s[0]));
    if (tmp[0] == OKWS_DEBUG_OPTIONS) 
      fnd_dbg = true;
    env.push_back (s.pop_front ());
  }

  // 
  // if our options were on, but none were passed to okd, then
  // we turn it on here
  //
  str opts = getenv (OKWS_DEBUG_OPTIONS);
  if (!fnd_dbg && opts) {
    strbuf b;
    b.fmt ("%s=%s", OKWS_DEBUG_OPTIONS, opts.cstr ());
    env.push_back (b);
  }

  if (s.size () != 1)
    goto usage;
  
  okdexecpath = s.pop_front ();

  if (!is_safe (okdexecpath)) {
    warn << loc << ": okd path (" << okdexecpath
	 << ") contains unsafe substrings\n";
    goto err;
  }

  es = can_exec (okws_exec (okdexecpath));
  if (es) {
    warn << loc << ": cannot exec okd: " << es <<"\n";
    goto err;
  }
  
  if (env.size ()) 
    okdenv.init (env, environ);

  return;

 usage:
    warn << loc << ": usage: OkdExecPath [<ENV>] <okd-execpath>\n";
 err:
    *errp = true;
}

static bool
parse_service_options (vec<str> *v, ok_usr_t **u, const str &loc,
		       int *svc_reqs, int *svc_time)
{
  int svc_uid = -1;
  optind = 0;
  bool rc = true;
  int ch;

  argv_t argv (*v);
  while ((ch = getopt (argv.size (), argv, "u:r:t:")) != -1 && rc) {
    switch (ch) {
    case 'u':
      if (*u) {
	warn << loc << ": -u option // specified more than once!\n";
 	rc = false;
      } else if (convertint (optarg, &svc_uid)) {
	*u = New ok_usr_t (svc_uid);
      } else {
	*u = New ok_usr_t (optarg);
	
	// only check if user exists if supplying non-integer
 	// user ID (i.e., a username). In the above case, we don't
 	// check, since we're allowing random user IDs not in 
 	// /etc/passwd
	if (!**u) {
	  warn << loc << ": cannot find user " << optarg << "\n";
	  rc = false;
	}
      }
      break;
    case 't':
      if (!convertint (optarg, svc_time)) {
	warn << loc << ": -t expects an integer argument\n";
	rc = false;
      }
      break;
    case 'r':
      if (!convertint (optarg, svc_reqs)) {
	warn << loc << ": -r expects an integer argument\n";
	rc = false;
      }
      break;
    default:
      warn << loc << ": unrecognized option given\n";
      rc = false;
      break;
    }
  }
  
  // pop off "bin" and also all arguments
  for (int i = 0; i < optind; i++) {
    v->pop_front ();
  }

  return rc;
}

void
okld_t::got_interpreter (vec<str> s, str loc, bool *errp)
{
  // pop off "Interpreter"
  str cmd = s.pop_front ();

  str name, group, path;
  vec<str> env, args;
  okld_interpreter_t *ipret = NULL;
  str errstr;

  if (!s.size ()) goto usage;
  name = s.pop_front ();

  get_env_params (&s, &env);
  if (!s.size ())
    goto usage;
  path = s.pop_front ();

  // remaining are args to exe
  args = s;

  if (interpreters[name]) {
    warn << loc << ": " << name << ": interpreter keyword already in use\n";
    goto err;
  }

  ipret = New okld_interpreter_t (name, ok_root, ok_wheel, env, path, args, 
				  this, loc);
  interpreters.insert (ipret);

  if (!ipret->check (&errstr)) {
    warn << loc << ": " << errstr;
    goto err;
  }
  return;

 usage:
  warn << loc << ": usage: " << cmd << " <shortname> <exe>\n";
 err:
  *errp = true;
}



void
okld_t::got_service2 (vec<str> s, str loc, bool *errp)
{
  ok_usr_t *u = NULL;

  str bin;
  str uri;
  vec<str> env;
  str exe, httppath;
  int svc_reqs = -1, svc_time = -1;

  okws1_port_t port;
  okld_ch_t *chld;

  //
  // pop off "Service2"
  //
  s.pop_front ();

  if (s.size () < 2) 
    goto usage;

  get_env_params (&s, &env);

  if (!s.size ()) 
    goto usage;

  uri = s.pop_front ();
  if (!check_uri (loc, uri, &port))
    goto err;

  if (s.size () < 1)
    goto usage;

  bin = s.pop_front ();
  if (!is_safe (bin)) {
    warn << loc << ": Service path (" << bin 
	 << ") contains unsafe substrings\n";
    goto err;
  }
  exe = apply_container_dir (service_bin, bin);

  if (!parse_service_options (&s, &u, loc, &svc_reqs, &svc_time))
    goto err;
  
  //
  // if a port was specified for this URI, then there is no need
  // to prepend it with a leading slash.
  //
  if (port) {
    httppath = uri;
    used_ports.insert (port);
  } else {
    httppath = re_fslash (uri);
    used_primary_port = true;
  }
  
  warn << "Service2: URI(" << httppath << ") --> unix(" << exe;
  for (u_int i = 0; i < s.size (); i++) 
    warnx << " " << s[i];
  warnx << ")\n";
 
  chld = New okld_ch_t (exe, httppath, this, loc, u, env, port);
  chld->add_args (s);

  if (svc_time >= 0) chld->set_svc_life_time (svc_time); 
  if (svc_reqs >= 0) chld->set_svc_life_reqs (svc_reqs);

  svcs.push_back (chld);
  return;

 usage:
  warn << loc << ": usage: Service2 [<env>] <URI> [<options>] [<exec>]\n";
 err:
  *errp = true;
  if (u) delete u;
}

void
okld_t::got_service (bool script, vec<str> s, str loc, bool *errp)
{
  ok_usr_t *u = NULL;

  str bin;
  str uri;
  vec<str> env;
  str exe, httppath;
  str cmd;
  okws1_port_t port;
  okld_interpreter_t *ipret = NULL;
  str ipret_str;
  okld_ch_t *ch = NULL;
  int svc_reqs = -1, svc_time = -1;

  //
  // pop off "Service" or "Script"
  //
  cmd = s.pop_front ();

  // now get environmental parameters (if any)
  get_env_params (&s, &env);

  // now get the interpreter to run if the command specified was "Script"
  if (script) {
    if (!s.size ())
      goto usage;
    ipret_str = s.pop_front ();
    if (!(ipret = interpreters[ipret_str])) {
      warn << loc << ": interpreter '" << ipret_str << "' is not defined!\n";
      goto err;
    }
    // need to add the 
    vec<str> tmp = ipret->get_env ();
    for (u_int i = 0; i < env.size (); i++) 
      tmp.push_back (env[i]);
    env = tmp;
  }

  // now get the executable or the script, as the case may be
  if (!s.size ()) 
    goto usage;
  bin = s.pop_front ();
  if (!is_safe (bin)) {
    warn << loc << ": Service path (" << bin
	 << ") contains unsafe substrings\n";
    goto err;
  }

  // there might be a -u option specified (and others TK)
  // pass bin into parse_service_options so it has a first
  // argument to deal with.
  if (!parse_service_options (&s, &u, loc, &svc_reqs, &svc_time))
    goto err;

  if (!s.size ())
    goto usage;
  uri = s[0];
  if (!check_uri (loc, uri, &port))
    goto err;

  if (script)
    exe = bin;
  else
    exe = apply_container_dir (service_bin, bin);

  //
  // if a port was specified for this URI, then there is no need
  // to prepend it with a leading slash.
  //
  if (port) {
    httppath = uri;
    used_ports.insert (port);
  } else {
    httppath = re_fslash (uri);
    used_primary_port = true;
  }

  warn << cmd << ": URI(" << httppath << ") --> unix(" << exe << ")\n";
  if (script)
    ch = New okld_ch_script_t (exe, httppath, this, loc, ipret, u, env, port);
  else
    ch = New okld_ch_t (exe, httppath, this, loc, u, env, port);

  if (svc_reqs >= 0) ch->set_svc_life_reqs (svc_reqs);
  if (svc_time >= 0) ch->set_svc_life_time (svc_time);

  svcs.push_back (ch);

  return;

 usage:
  warn << loc << ": usage: " << cmd << " [<env>] ";
  if (script)
    warnx << "<interpreter> ";
  warnx << "<exec-path> [<options>] <URI>\n";
 err:
  *errp = true;
  if (u) delete u;
}

void
okld_t::got_alias (vec<str> s, str loc, bool *errp)
{
  if (s.size () != 3) {
    warn << loc << ": usage: Alias <to-URI> <from-URI>\n";
    *errp = true;
    return;
  }

  okws1_port_t port;
  if (!check_uri (loc, s[1]) || !check_uri (loc, s[2], &port)) {
    *errp = true;
    return;
  }
  if (port)
    used_ports.insert (port);
  else
    used_primary_port = true;

  aliases_tmp.push_back (alias_t (s[1], s[2], loc, port));
}

bool
okld_t::check_services_and_aliases ()
{
  bhash<str> exetab;
  bhash<str> svctab;
  bhash<str> aliases_bmap;
  bool ret = true;

  // first perform sanity check among services
  u_int sz = svcs.size ();
  for (u_int i = 0; i < sz; i++) {
    okld_ch_t *svc = svcs[i];
    str s = fix_uri (svc->servpath);
    str loc = svc->loc ();
    if (svctab[s]) {
      warn << loc << ": service (" << s << ") specified more than once\n";
      ret = false;
    } else {
      svctab.insert (s);
    }
    svc->servpath = s;

    s = svc->get_execpath_relative_to_chroot ();
    if (exetab[s]) {
      warn << loc << ": executable (" << s << ") specified more than once\n";
    } else {
      exetab.insert (s);
    }
  }

  while (aliases_tmp.size ()) {

    alias_t a = aliases_tmp.pop_front ();
    a.to = fix_uri (a.to);
    a.from = fix_uri (a.from);

    if (!svctab[a.to]) {
      warn << a.loc << ": No service found for alias: " << a.to_str () << "\n";
      ret = false;
    } else if (aliases_bmap[a.from]) {
      warn << a.loc << ": Doubly-specified alias for URI " << a.from  << "\n";
      ret = false;
    } else if (svctab[a.from]) {
      warn << a.loc << ": Alias URI is already taken by a service: " 
	   << a.from << "\n";
      ret = false;
    } else if (a.port && !allports_map[a.port]) {
      warn << a.loc << ": alias uses port (" << a.port << ") that OKD "
	   << "does not listen on\n";
      ret = false;
    } else {
      aliases_bmap.insert (a.from);
    }
  }

  return ret;
}

bool
okld_t::check_ports ()
{
  bool ret = true;
  while (allports.size ()) {
    okws1_port_t p = allports.pop_front ();
    if (!(used_ports[p] || (p == listenport && used_primary_port))) {
      warn << "OKD will listen on port " << p << ", but no "
	   << "services will be using it!\n";
      ret = false;
    }
  }

  return ret;
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
  str root, wheel;

  ct.add ("Service", wrap (this, &okld_t::got_service, false))
    .add ("Service2", wrap (this, &okld_t::got_service2))
    .add ("Interpreter", wrap (this, &okld_t::got_interpreter))
    .add ("Script", wrap (this, &okld_t::got_service, true))
    .add ("Alias", wrap (this, &okld_t::got_alias))
    .add ("TopDir", wrap (got_dir, &topdir))
    .add ("JailDir", wrap (got_dir, &jaildir))
    .add ("ServiceBin", wrap (got_dir, &service_bin))

    .add ("CrashSamplingInterval", &ok_csi, 1, 60)
    .add ("MaxCrashedProcesses", &ok_crashes_max, 1, 200)
    .add ("ServiceLowUid", &ok_svc_uid_low, OK_UID_MIN, OK_UID_MAX)
    .add ("ServiceHighUid", &ok_svc_uid_high, OK_UID_MIN, OK_UID_MAX)
    .add ("ServiceMode", &ok_svc_mode, 0, 0777)
    .add ("ServiceGroup", &grp)
    .add ("OkdExecPath", wrap (this, &okld_t::got_okd_exec))
    .add ("CoreDumpDir", wrap (got_dir, &coredumpdir))
    .add ("SocketDir", wrap (got_dir, &sockdir))
    .add ("BindAddr", wrap (static_cast<ok_base_t *> (this), 
			    &ok_base_t::got_bindaddr))
    .add ("ListenPorts", wrap (static_cast<ok_base_t *> (this),
			       &ok_base_t::got_ports))
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
    .add ("FilterCGI", &ok_filter_cgi, int (XSSFILT_NONE), int (XSSFILT_ALL))
    .add ("SfsClockMode", wrap (got_clock_mode, &clock_mode))
    .add ("MmapClockDaemon", &mmcd)
    .add ("MmapClockFile", &mmc_file)
    .add ("DangerousZbufs", &ok_dangerous_zbufs)
    .add ("SyslogPriority", &ok_syslog_priority)
    .add ("AxprtPacketSize", &ok_axprt_ps, 0, INT_MAX)
    
    .add ("ServiceLifeRequests", &ok_svc_life_reqs, 0, INT_MAX)
    .add ("ServiceLifeTime", &ok_svc_life_time, 0, INT_MAX)

    .add ("RootUser", &root)
    .add ("RootGroup", &wheel)

    .add ("SyscallStatDumpInterval", &ok_ssdi, 0, 1000)

    .add ("OkdUser", &okd_un)
    .add ("OkdGroup", &okd_gr)
    
    .add ("ServerName", &reported_name)
    .add ("ServerVersion", &version)
    .add ("HostName", &hostname)
    // as reported in HTTP headers
    .add ("ServerNameHTTP", &global_okws_server_label) 

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
    .add ("RecycleSuioLimit", &ok_recycle_suio_limit, OK_RSL_LL, OK_RSL_UL)

    .add ("ServiceFDHighWat", &ok_svc_fds_high_wat, 0, 10240)
    .add ("ServiceFDLowWat", &ok_svc_fds_low_wat, 0, 10000)
    .add ("ServiceAcceptMessages", &ok_svc_accept_msgs)


    .add ("OkdFDHighWat", &okd_fds_high_wat, 
	  OKD_FDS_HIGH_WAT_LL, OKD_FDS_HIGH_WAT_UL)
    .add ("OkdFDLowWat", &okd_fds_low_wat, 
	  OKD_FDS_LOW_WAT_LL, OKD_FDS_HIGH_WAT_UL)
    .add ("ServiceFDQuota", &ok_svc_fd_quota, 
	  OK_SVC_FD_QUOTA_LL, OK_SVC_FD_QUOTA_UL)

    .add ("StartupBatchSize", &okld_startup_batch_size, 0, 128)
    .add ("StartupBatchWait", &okld_startup_batch_wait, 0, 128)

    .ignore ("MaxConQueueSize")
    .ignore ("ListenQueueSize")
    .ignore ("OkMgrPort")
    .ignore ("PubdExecPath")
    .ignore ("PubdInet")
    .ignore ("PubdUnix")
    .ignore ("ErrorDoc")
    .ignore ("OkdAcceptMessages")
    .ignore ("OkdChildSelectDisable")
    .ignore ("ShutdownRetries")
    .ignore ("ShutdownTimeout")
    .ignore ("OkdDebugMsgFreq");


  while (pa.getline (&av, &line)) {
    if (!ct.match (av, cf, line, &errors)) {
      warn << cf << ":" << line << ": unknown config parameter\n";
      errors = true;
    }
  }

  // allow these to be set
  if (root) ok_root = root;
  if (wheel) ok_wheel = wheel;

  if (grp) svc_grp = ok_grp_t (grp);
  if (okd_un) okd_usr = ok_usr_t (okd_un);
  if (okd_gr) okd_grp = ok_grp_t (okd_gr);

  nxtuid = ok_svc_uid_low;

  //
  // perform checks for okd to make shutdown cleaner in the
  // case of bad parameters specified.
  //
  if (okd_fds_low_wat > okd_fds_high_wat) {
    warn << "OkdFDHighWat needs to be greater than OkdFDLowWat\n";
    errors = true;
  }

  if (ok_svc_fd_quota > okd_fds_low_wat) {
    warn << "ServiceFDQuota must be less than OkdFDLowWat\n";
    errors = true;
  }

  if (ok_svc_fds_high_wat > 0 && 
      (ok_svc_fds_low_wat == 0 || ok_svc_fds_low_wat > ok_svc_fds_high_wat)) {
    warn << "ServiceMaxFDs needs to be greater than ServiceFDHighWat\n";
    errors = true;
  }

  if (allports.size () == 0) {
    warn << "No listen ports given; assuming default port (80)\n";
    allports.push_back (listenport);
    allports_map.insert (listenport);
  }

  if (!check_services_and_aliases () || !check_ports () 
      || !check_service_ports ()) 
    errors = true;

  if (!hostname)
    hostname = myname ();

  if (errors)
    okld_exit (1);
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
    .insert ("okwsname", reported_name)
    .insert ("server", global_okws_server_label)
    .insert ("jailed", int (jaildir && !uid))
    .insert ("logfmt", logd_parms.accesslog_fmt)
    .insert ("svclog", int (logd_parms.svclog))
    .insert ("gzip", ok_gzip ? 1 : 0)
    .insert ("gziplev", ok_gzip_compress_level)
    .insert ("gzipcsl", ok_gzip_cache_storelimit)
    .insert ("logtick", ok_log_tick)
    .insert ("logprd", ok_log_period)
    .insert ("clito", ok_clnt_timeout)
    .insert ("rsl", ok_recycle_suio_limit)
    .insert ("reqszlimit", ok_reqsize_limit)
    .insert ("filtercgi", ok_filter_cgi)
    .insert ("ssdi", ok_ssdi)
    .insert ("sendsin", ok_send_sin ? 1 : 0)
    .insert ("fdlw", ok_svc_fds_low_wat)
    .insert ("fdhw", ok_svc_fds_high_wat)
    .insert ("acmsg", ok_svc_accept_msgs)
    .insert ("dz", ok_dangerous_zbufs ? 1 : 0)
    .insert ("ps", ok_axprt_ps)
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

    if (svc->usr ()) {
      // in this case, the operator specified the user via a -u option
      // to the Service line.
      uegid = svc->usr ()->getid ();
      if (uids_in_use[uegid]) {
	warn << "The UID " << uegid
	     << " that you specified for service " << svc->servpath << "\n"
	     << "\t is in use by another, and I'm refusing to use it.\n";
	return false;
      } else if (owners[uegid]) {
	warn << "The UID " << uegid
	     << " that you specified for service " << svc->servpath << "\n"
	     << "\towns the executable of another service, so it's unsafe "
	     << "to use.\n";
	return false;
      }
    } else {
      // In this case, we autogenerate a user ID / group ID pair
      while ((getpwuid (uegid) ||  getgrgid (uegid) ||
	      uids_in_use[uegid] || owners[uegid]) && 
	     uegid <= ok_svc_uid_high) {
	warn << "UID/GID=" << uegid
	     << " is in use; refusing to use it.\n";
	uegid = ++nxtuid;
      }

      if (uegid > ok_svc_uid_high) {
	warn << "too many services / ran out of UIDs\n";
	return false;
      }
    }

    uids_in_use.insert (uegid);

    // because we check after the previous insert, we won't be able to
    // use a GID/UID pair in its entirety; this is precisely by design
    if (uids_in_use[ueuid_own]) {
      warn << svc->loc () << ": UID " << ueuid_own
	   << " in use / changing ownership on file to root\n";
      ueuid_own = ok_usr_t (ok_root).getid ();
    } 

    owners.insert (ueuid_own);

    // fixup ownership and permissions, and who to exec as
    // based on the old and new gid.uid pairs (and also the old mode).
    // note that scripts will do something different here from
    // compiled executables (some combination of making new copies
    // of the interpreter and fiddling bits).
    if (!svc->fixup_doall (ueuid_orig, ueuid_own, uegid_orig, uegid, mode))
      ret = false;
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

  okld_exit (status);

}

void
okld_t::okld_exit (int code)
{
  // turn of the clock daemon cleanly. not entirely necessary.
  if (mmcd_pid > 0) {
    kill (mmcd_pid, SIGTERM);
  }
  delete this;
  exit (code);
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
  argv.push_back ("-p");
  argv.push_back (strbuf () << listenport);
		  
  if (debug_stallfile) {
    argv.push_back ("-D");
    argv.push_back (strbuf (debug_stallfile) << ".okd");
  }

  // launch okd synchronously; no point in us running if okd puked.
  ptr<axprt_unix> x = axprt_unix_spawnv (prog, argv, 0, NULL, okdenv);
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
okld_t::check_service_ports ()
{
  bool ret = true;
  u_int lim = svcs.size ();
  for (u_int i = 0; i < lim; i++) {
    okws1_port_t p = svcs[i]->get_port ();
    if (p && ! allports_map[p]) {
      warn << svcs[i]->loc () 
	   << ": service uses a port (" << p << ") that OKD does not "
	   << "listen on!!\n";
      ret = false;
    }
  }
  return ret;
}

bool
okld_t::check_exes ()
{
  if (!will_jail ())
    return true;

  bool ret = true;
  u_int lim = svcs.size ();
  for (u_int i = 0; i < lim; i++) {
    // at this point in the boot process, we have not called chroot yet
    if (!svcs[i]->can_exec ())
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

  set_debug_flags ();

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
  if (!cf) 
    cf = get_okws_config ();

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
  if (!(check_exes () && fix_uids () && init_jaildir () 
	&& init_interpreters ()))
    okld_exit (1);

  used_ports.clear ();

  if (jaildir)
    warn ("JailDirectory: %s\n", jaildir.cstr ());
  launch_logd (wrap (this, &okld_t::launch_logd_cb));
}

void
okld_t::launch_logd_cb (bool rc)
{
  if (!rc) {
    warn << "launch of log daemon (oklogd) failed\n";
    okld_exit (1);
  }
  logd->clone (wrap (this, &okld_t::launch_logd_cb2));

}

void
okld_t::launch_logd_cb2 (int logfd)
{
  if (logfd < 0) {
    warn << "oklogd did not send a file descriptor; aborting\n";
    okld_exit (1);
  }
  launch2 (logfd);
}

void
okld_t::launch2 (int logfd)
{
  if (!launch_okd (logfd))
    okld_exit (1);
  
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
  for (u_int i = 0; i < okld_startup_batch_size && launchp < lim; i++) {
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
    delaycb (okld_startup_batch_wait, 0, 
	     wrap (this, &okld_t::launchservices));
  }
}

bool
okld_t::init_jaildir ()
{
  if (!will_jail ())
    return true;
  ok_usr_t root (ok_root);
  ok_grp_t wheel (ok_wheel);

  if (!root || !wheel) {
    warn << "cannot access root.wheel in /etc/passwd\n";
    return false;
  }

  root_coredir = apply_container_dir (coredumpdir, "0");

  if (!svc_grp) {
    warn << "cannot find service group (" << svc_grp.getname () 
	 << " does not exist)\n";
    return false;
  }

  int ret = true;
  if (!jail_mkdir ("/etc/", 0755, &root, &wheel) ||  
      !jail_cp ("/etc/resolv.conf", 0644, &root, &wheel) || 
      !jail_mkdir_p (coredumpdir, 0755, &root, &wheel) ||
      !jail_mkdir (root_coredir, 0700, &root, &wheel) ||
      !jail_mkdir_p (sockdir, 0755, &root, &wheel)) 
    ret = false;

  u_int lim = svcs.size ();
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
    str f = jail2real (ok_mmc_file);
    const char *args[] = { mmcd.cstr (), f.cstr (), NULL };

    // note that we're running the clock daemon as root.
    // this should be fine. 
    warn << "*unstable: launching mmcd: " << args[0] << " " << args[1] << "\n";
    if ((mmcd_pid = spawn (args[0], args)) < 0) {
      warn ("cannot start mmcd %s: %m\n", args[0]);
      clock_mode = SFS_CLOCK_GETTIME;
    } else {
      // okld does not change its clock type, since it's mainly idle.
      // but it should know when the clock daemon dies, so it knows
      // not to kill it on shutdown
      chldcb (mmcd_pid, wrap (this, &okld_t::clock_daemon_died));
    }
  }
}

static void
init_interpreter (bool *ok, okld_interpreter_t *i)
{
  if (!i->base_init ())
    *ok = false;
}

bool
okld_t::init_interpreters ()
{
  if (!will_jail ())
    return true;
  bool ok = true;
  interpreters.traverse (wrap (init_interpreter, &ok));
  if (!ok)
    return false;

  return true;
}
