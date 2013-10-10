// -*-c++-*-
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
#include "pubutil.h"
#include "rxx.h"
#include "ok_adebug.h"
#include "sfs_select.h"

extern char ** environ;
extern int optind;

#define PUBD_KEY "PubdExecPath"

//-----------------------------------------------------------------------

void
okld_t::set_signals ()
{
  sigcb (SIGTERM, wrap (this, &okld_t::caught_signal, SIGTERM));
  sigcb (SIGINT,  wrap (this, &okld_t::caught_signal, SIGINT));
}

//-----------------------------------------------------------------------

static void
get_env_params (vec<str> *in, vec<str> *out)
{
  while (in->size () && strchr ((*in)[0].cstr (), '='))
    out->push_back (in->pop_front ());
}


//-----------------------------------------------------------------------

void
okld_ch_cluster_t::add_args (const vec<str> &v)
{
  for (u_int i = 0; i < v.size (); i++) 
    _args.push_back (v[i]);
}

//-----------------------------------------------------------------------

bool
okld_t::check_uri (const str &loc, const str &in, okws1_port_t *port) const
{
  u_int32_t t (0);
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

//-----------------------------------------------------------------------

void
okld_t::got_okd_exec (vec<str> s, str loc, bool *errp)
{
  okld_t::got_generic_exec (&_okd, s, loc, errp);
  _okd.activate ();
}

//-----------------------------------------------------------------------

void
okld_t::got_generic_exec (okld_helper_t *out, vec<str> s, str loc, bool *errp)
{
  ptr<env_argv_t> env;
  if (ok_base_t::got_generic_exec (s, loc, errp, &env)) {
    out->argv () += s;
    out->set_env (env);
  }
}

//-----------------------------------------------------------------------

void
okld_t::got_okssl_exec (vec<str> s, str loc, bool *errp) {
    qhash_const_iterator_t<str, ptr<okld_helper_ssl_t> > it(_okssls);
    const str* k;
    while ((k = it.next())) {
        ptr<okld_helper_ssl_t> okssl = *_okssls[*k];
        okld_t::got_generic_exec (okssl, s, loc, errp);
        okssl->activate ();
    }
    _auto_activate = true;
    _ssl_exec_params = s;
}

//-----------------------------------------------------------------------

static bool
parse_service_options (vec<str> *v, ok_usr_t **u, const str &loc,
		       svc_options_t *so)
{
  int svc_uid = -1;
  optind = 1;
  bool rc = true;
  int ch;

  argv_t argv (*v);
  const char *opts = "eEcCWwu:r:sSt:p:l:h:T:y:z:Zn:";
  while ((ch = getopt (argv.size (), argv, opts)) != -1 && rc) {
    switch (ch) {
    case 'T':
      {
	int tmp;
	if (convertint (optarg, &tmp)) {
	  so->ahttpcon_zombie_timeout = tmp;
	} else {
	  warn << loc << ": -t option needs a timeout in seconds\n";
	}
      }
      break;
    case 'Z':
      so->ahttpcon_zombie_warn = 1;
      break;
    case 'p':
      {
	int tmp_port;
	if (convertint (optarg, &tmp_port)) {
	  so->ports.push_back (tmp_port);
	} else {
	  warn << loc << ": -p option needs a port (integer)\n";
	  rc = false;
	}
      }
      break;
    case 'u':
      if (*u) {
	warn << loc << ": -u option specified more than once!\n";
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
    case 'h':
      if (!convertint (optarg, &so->hiwat)) {
	warn << loc << ": -h expects an integer argument\n";
	rc = false;
      }
      break;
    case 'l':
      if (!convertint (optarg, &so->lowat)) {
	warn << loc << ": -l expects an integer argument\n";
	rc = false;
      }
      break;
    case 't':
      if (!convertint (optarg, &so->svc_time)) {
	warn << loc << ": -t expects an integer argument\n";
	rc = false;
      }
      break;
    case 'r':
      if (!convertint (optarg, &so->svc_reqs)) {
	warn << loc << ": -r expects an integer argument\n";
	rc = false;
      }
      break;
    case 'n':
      {
	size_t tmp; 
	if (!convertint (optarg, &tmp)) {
	  warn << loc << ": -n expects an integer argument\n";
	  rc = false;
	} else if (tmp < 1 || tmp > ok_max_brother_procs) {
	  warn << loc << ": -n must be greater than 0 and no more than "
	       << ok_max_brother_procs << "\n";
	  rc = false;
	} else {
	  so->_n_procs = tmp;
	}
      }
      break;
    case 'e':
      so->pub3_viserr = 1;
      break;
    case 'E':
      so->pub3_viserr = 0;
      break;
    case 'w':
      so->wss = 1;
      break;
    case 'W':
      so->wss = 0;
      break;
    case 'C':
      so->pub3_caching = 0;
      break;
    case 'c':
      so->pub3_caching = 1;
      break;
    case 'S':
      so->wait_for_signal = 0;
      break;
    case 's':
      so->wait_for_signal = 1;
      break;
    case 'y':
      if (!convertint (optarg, &so->gzip_level)) {
	warn << "loc" << ": -y expects an integer argument\n";
	rc = false;
      }
      break;
    case 'z': 
      {
	bool ok;
	gzip_mode_t m = ok_gzip_str_to_mode (optarg, &ok); 
	if (!ok) {
	  warn << loc << ": -z expects 0,1,2 or (smart, naive)\n";
	  rc = false;
	} else {
	  so->gzip = int (m);
	}
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

  if (!so->check_options (loc)) {
    rc = false;
  }
  return rc;
}

//-----------------------------------------------------------------------

void
okld_t::got_interpreter (vec<str> s, str loc, bool *errp)
{
  strip_comments (&s);

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

//-----------------------------------------------------------------------

void
okld_t::got_service2 (vec<str> s, str loc, bool *errp)
{
  ok_usr_t *u = NULL;

  str bin;
  str uri;
  vec<str> env;
  str exe, httppath;

  okws1_port_t port;
  ptr<okld_ch_cluster_t> chld;
  svc_options_t so;

  strip_comments (&s);

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

  bin = s.front (); // pop it off in parse_serviec_options
  if (!is_safe (bin)) {
    warn << loc << ": Service path (" << bin 
	 << ") contains unsafe substrings\n";
    goto err;
  }
  exe = apply_container_dir (service_bin, bin);

  if (!parse_service_options (&s, &u, loc, &so))
    goto err;
  
  //
  // if a port was specified for this URI, then there is no need
  // to prepend it with a leading slash.
  //
  if (port) {
    httppath = uri;
    used_ports.insert (port);
  } else {
    httppath = re_fslash (uri.cstr());
    used_primary_port = true;
  }
  
  warn << "Service2: URI(" << httppath << ") --> unix(" << exe;
  for (u_int i = 0; i < s.size (); i++) 
    warnx << " " << s[i];
  warnx << ")\n";

  chld = New refcounted<okld_ch_cluster_t> (exe, httppath, this, loc, u, 
					    env, port, so.n_procs ());
  chld->add_args (s);
  chld->set_service_options (so);
  return;

 usage:
  warn << loc << ": usage: Service2 [<env>] <URI> [<options>] [<exec>]\n";
 err:
  *errp = true;
  if (u) delete u;
}

//-----------------------------------------------------------------------

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
  ptr<okld_ch_cluster_t> ch;
  svc_options_t so;

  // Remove the '#' and everything after...
  strip_comments (&s);

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

  bin = s.front (); // pop it from s in 'parse_service_options' , below
  if (!is_safe (bin)) {
    warn << loc << ": Service path (" << bin
	 << ") contains unsafe substrings\n";
    goto err;
  }

  // there might be a -u option specified (and others TK)
  if (!parse_service_options (&s, &u, loc, &so))
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
    httppath = re_fslash (uri.cstr());
    used_primary_port = true;
  }

  warn << cmd << ": URI(" << httppath << ") --> unix(" << exe << ")\n";

  if (script) {
    ch = New refcounted<okld_ch_script_t> (exe, httppath, this, loc, 
					   ipret, u, env, port, so.n_procs ());
  } else {
    ch = New refcounted<okld_ch_cluster_t> (exe, httppath, this, loc, u, 
					    env, port, so.n_procs ());
  }
  ch->set_service_options (so);
  add_svc (ch);

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

//-----------------------------------------------------------------------

void
okld_t::got_regex_alias (vec<str> s, str loc, bool *errp)
{
  strip_comments (&s);
  if (s.size () != 3 && s.size () != 4) {
    warn << loc << ": usage: RegexAlias <to-URI> <regex> [<opts>]\n";
    *errp = true;
    return;
  }
  if (!check_uri (loc, s[1])) {
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

  regex_aliases_tmp.push_back (alias_t (s[1], s[2], loc, 0));
}

//-----------------------------------------------------------------------

void
okld_t::got_alias (vec<str> s, str loc, bool *errp)
{
  strip_comments (&s);
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

//-----------------------------------------------------------------------

bool
okld_t::check_services_and_aliases ()
{
  bhash<str> exetab;
  bhash<str> svctab;
  bhash<str> aliases_bmap;
  bool ret = true;

  // first perform sanity check among services
  size_t sz = _svcs.size ();
  for (size_t i = 0; i < sz; i++) {
    okld_ch_cluster_t *svc = _svcs[i];
    str s = fix_uri (svc->_servpath);
    str loc = svc->loc ();
    if (svctab[s]) {
      warn << loc << ": service (" << s << ") specified more than once\n";
      ret = false;
    } else {
      svctab.insert (s);
    }
    svc->_servpath = s;

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
    } else if (a.port && !_all_ports_map[a.port]) {
      warn << a.loc << ": alias uses port (" << a.port << ") that OKD "
	   << "does not listen on\n";
      ret = false;
    } else {
      aliases_bmap.insert (a.from);
    }
  }

  while (regex_aliases_tmp.size ()) {

    alias_t a = regex_aliases_tmp.pop_front ();
    a.to = fix_uri (a.to);
    
    if (!svctab[a.to]) {
      warn << a.loc << ": No service found for RegexAlias: " 
	   << a.to_str () << "\n";
      ret = false;
    }
  }

  return ret;
}

//-----------------------------------------------------------------------------

bool okld_t::is_ssl_port_active(okws1_port_t port) {
    qhash_const_iterator_t<str, ptr<okld_helper_ssl_t> > it(_okssls);
    const str* k;
    bool ret = false;
    while ((k = it.next())) {
        ptr<okld_helper_ssl_t> okssl = *_okssls[*k];
        ret |= (okssl->active() && okssl->listening_on(port));
    }   
    return ret;
}

//-----------------------------------------------------------------------

bool
okld_t::check_ports ()
{
    bool ret = true;
    qhash_const_iterator_t<okws1_port_t, bool> it (_all_ports_map);
    const okws1_port_t *pp;
    while ((pp = it.next ())) {
        okws1_port_t p = *pp;
        if (!(used_ports[p] ||
              (used_primary_port &&
               (p == listenport || 
                (is_ssl_port_active(p)))))) {
            warn << "OKD will listen on port " << p << ", but no "
                << "services will be using it!\n";
            ret = false;
        }
    }

    return ret;
}

//-----------------------------------------------------------------------

static
void got_syslog_levels (vec<str> s, str loc, bool *errp)
{
  for (size_t i = 1; i < s.size (); i++) {
    if (!ok::syslog_ctl.enable_level (s[i])) {
      warn << loc << ": bad syslog level: " << s[i] << "\n";
      *errp = true;
    }
  }
}

//-----------------------------------------------------------------------

bool
okld_t::parse_file (const str &cf)
{
  conftab ct;
  _config_no_pub_v2_support = false;
  str sys_sel_tmp;
  str gzip_tmp;
  str tmp_dump_user, tmp_dump_group;

  ct.add ("Service", wrap (this, &okld_t::got_service, false))
    .add ("Service2", wrap (this, &okld_t::got_service2))
    .add ("Interpreter", wrap (this, &okld_t::got_interpreter))
    .add ("Script", wrap (this, &okld_t::got_service, true))
    .add ("Alias", wrap (this, &okld_t::got_alias))
    .add ("RegexAlias", wrap (this, &okld_t::got_regex_alias))
    .add ("TopDir", wrap (got_dir, &topdir))
    .add ("JailDir", wrap (got_dir, &jaildir))
    .add ("ServiceBin", wrap (got_dir, &service_bin))

    .add ("CrashSamplingInterval", &ok_csi, 1, 60)
    .add ("MaxCrashedProcesses", &ok_crashes_max, 1, 200)
    .add ("ServiceLowUid", &ok_svc_uid_low, OK_UID_MIN, OK_UID_MAX)
    .add ("ServiceHighUid", &ok_svc_uid_high, OK_UID_MIN, OK_UID_MAX)
    .add ("ServiceMode", &ok_svc_mode, 0, 0777)
    .add ("ServiceGroup", &_config_grp)
    .add ("OkdExecPath", wrap (this, &okld_t::got_okd_exec))
    .add ("CoreDumpDir", wrap (got_dir, &coredumpdir))
    .add ("CoreDumpUser", &tmp_dump_user)
    .add ("CoreDumpGroup", &tmp_dump_group)
    .add ("CoreDumpMode", &_coredump_mode, 0, 0777)
    .add ("SocketDir", wrap (got_dir, &sockdir))
    .add ("BindAddr", wrap (static_cast<ok_base_t *> (this), 
			    &ok_base_t::got_bindaddr))
    .add ("ListenPorts", wrap (static_cast<ok_base_t *> (this),
			       &ok_base_t::got_ports, false))

    .add ("OkssldExecPath", wrap (this, &okld_t::got_okssl_exec))
    .add ("SslCreateChannel", wrap(this, &okld_t::got_ssl_create_channel))
    .add ("SslCertFile", wrap(this, &okld_t::got_certfile))
    .add ("SslKeyFile", wrap(this, &okld_t::got_keyfile))
    .add ("SslCertChainFile", wrap(this, &okld_t::got_chainfile))
    .add ("SslPrimaryPort", wrap (this, (&okld_t::got_ssl_primary_port)))
    .add ("SslListenPorts", wrap(this, &okld_t::got_ssl_listen_port))
    .add ("SslTimeout", wrap(this, &okld_t::got_ssl_timeout))
    .add ("SslUser", &_config_ssl_un)
    .add ("SslGroup", &_config_ssl_gr)

    .add ("OklogdExecPath", wrap (this, &okld_t::got_logd_exec))
    .add (PUBD_KEY, wrap (this, &okld_t::got_pubd_exec))
    .add ("PubWSS", &ok_pub3_wss, -1, 1)
    .add ("PubCaching" , &ok_pub3_caching, -1, 1)
    .add ("PubVisibileErrors", &ok_pub3_viserr, -1, 1)
    .add ("PubSvcNegCacheTimeout", &ok_pub3_svc_neg_cache_timeout,
	  0, INT_MAX)
    .add ("PubJsonStrictEscaping", &ok_pub3_json_strict_escaping)
    .add ("LogDir", &logd_parms.logdir)
    .add ("AccessLog", &logd_parms.accesslog)
    .add ("ErrorLog", &logd_parms.errorlog)
    .add ("OklogdPidfile", &logd_parms.pidfile)
    .add ("SslLog", &logd_parms.ssllog)
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
    .add ("SyslogTag", &ok_syslog_tag)
    .add ("SyslogDomain", &ok_syslog_domain)
    .add ("SyslogLevels", wrap (got_syslog_levels))
    .add ("AxprtPacketSize", &ok_axprt_ps, 0, INT_MAX)
    
    .add ("ServiceLifeRequests", &ok_svc_life_reqs, 0, INT_MAX)
    .add ("ServiceLifeTime", &ok_svc_life_time, 0, INT_MAX)

    .add ("RootUser", &_config_root)
    .add ("RootGroup", &_config_wheel)

    .add ("SyscallStatDumpInterval", &ok_ssdi, 0, 1000)

    .add ("OkdUser", &_config_okd_un)
    .add ("OkdGroup", &_config_okd_gr)
    
    .add ("ServerName", &reported_name)
    .add ("ServerVersion", &version)
    .add ("HostName", &hostname)
    // as reported in HTTP headers
    .add ("ServerNameHTTP", &global_okws_server_label) 

    .add ("LogTick", &ok_log_tick, 1, 4000)
    .add ("LogPeriod", &ok_log_period, 1, 100)

    .add ("ClientTimeout", &ok_clnt_timeout, 1, 400)

    .add ("Gzip", &gzip_tmp)
    .add ("GzipLevel", &ok_gzip_compress_level, Z_NO_COMPRESSION,
	  Z_BEST_COMPRESSION)
    .add ("GzipNaiveLevel", &ok_gzip_naive_compress_level, Z_NO_COMPRESSION,
	  Z_BEST_COMPRESSION)
    .add ("GzipSmallStrLen", &ok_gzip_smallstr, 0, 0x8000)
    .add ("GzipCacheMin", &ok_gzip_cache_minstr, 0, 0x10000)
    .add ("GzipCacheMax", &ok_gzip_cache_maxstr, 0, 0x1000000)
    .add ("GzipCacheSize", &ok_gzip_cache_storelimit, 0, 0x10000000)
    .add ("GzipMemLevel", &ok_gzip_mem_level, 0, 9)
    .add ("ChannelLimit", &ok_reqsize_limit, OK_RQSZLMT_MIN, OK_RQSZLMT_MAX)
    .add ("CgiValueLenLimit", &ok_cgibuf_limit, OK_RQSZLMT_MIN, OK_RQSZLMT_MAX)
    .add ("SendSockAddrIn", &ok_send_sin)
    .add ("RecycleSuioLimit", &ok_recycle_suio_limit, OK_RSL_LL, OK_RSL_UL)

    .add ("ServiceFDHighWat", &ok_svc_fds_high_wat, 0, OK_SVC_FD_HIGH_WAT_UL)
    .add ("ServiceFDLowWat", &ok_svc_fds_low_wat, 0, OK_SVC_FD_LOW_WAT_UL)
    .add ("ServiceAcceptMessages", &ok_svc_accept_msgs)

    .add ("include", wrap (static_cast<config_parser_t *> (this), 
			   &config_parser_t::include))

    .add ("OkdFDHighWat", &okd_fds_high_wat, 
	  OKD_FDS_HIGH_WAT_LL, OKD_FDS_HIGH_WAT_UL)
    .add ("OkdFDLowWat", &okd_fds_low_wat, 
	  OKD_FDS_LOW_WAT_LL, OKD_FDS_LOW_WAT_UL)
    .add ("ServiceFDQuota", &ok_svc_fd_quota, 
	  OK_SVC_FD_QUOTA_LL, OK_SVC_FD_QUOTA_UL)

    .add ("StartupBatchSize", &okld_startup_batch_size, 0, 128)
    .add ("StartupBatchWait", &okld_startup_batch_wait, 0, 128)
    .add ("LazyStartup", &_lazy_startup)
    .add ("OkMgrSocket", &_okd_mgr_socket)

    .add ("PubChunkSize", &ok_pub3_max_datasz, 0, INT_MAX)
    .add ("SelectPolicy", &sys_sel_tmp)
    .add ("AhttpconZombieWarn", &ok_ahttpcon_zombie_warn)
    .add ("AhttpconZombieTimeout", &ok_ahttpcon_zombie_timeout, 
	  time_t (0), time_t (INT_MAX))
    .add ("JsonIntBitMax", &ok_pub3_json_int_bitmax, -4, 128)
    .add ("AggressiveServiceRestart", &_aggressive_svc_restart)
    .add ("DieOnLogdCrash", &_die_on_logd_crash)
    .add ("BindReattemptSchedule", &_bind_reattempt_schedule_str)
    .add ("EmergencyKillEnabled", &_emerg_kill)
    .add ("SSLCipherList", wrap(this, &okld_t::got_cipher_list))
    .add ("SSLHonorCipherOrder", wrap(this, &okld_t::got_cipher_order))
    .add ("SSLAllowClientRenog", wrap(this, &okld_t::got_cli_renog))
    .add ("SSLDebugStartup", wrap(this, &okld_t::got_ssl_debug_startup))
    .add ("GzipChunking", &ok_gzip_chunking)
    .add ("GzipChunkingForOldSafaris", &ok_gzip_chunking_old_safaris)
    .add ("GzipErrorPages", &ok_gzip_error_pages)
    .add ("Pub3RecycleLimitInt", &ok_pub3_recycle_limit_int, 0, INT_MAX)
    .add ("Pub3RecycleLimitBindtab", &ok_pub3_recycle_limit_bindtab, 0, INT_MAX)
    .add ("Pub3RecycleLimitDict", &ok_pub3_recycle_limit_dict, 0, INT_MAX)
    .add ("Pub3RecycleLimitSlot", &ok_pub3_recycle_limit_slot, 0, INT_MAX)

    .add ("ResolveBinaryPaths", &_config_resolve_bins)

    .ignore ("LogBuffers") // Transitional....
    .ignore ("OkMgrSocketOwner")
    .ignore ("StatPageURL")
    .ignore ("OkMgrSocketGroup")
    .ignore ("OkMgrSocketAccessMode")
    .ignore ("MaxConQueueSize")
    .ignore ("ListenQueueSize")
    .ignore ("ErrorDoc")
    .ignore ("OkdAcceptMessages")
    .ignore ("OkdChildSelectDisable")
    .ignore ("ShutdownRetries")
    .ignore ("ShutdownTimeout")
    .ignore ("OkdDebugMsgFreq")
    .ignore ("DemuxTimeout")
    .ignore ("AcceptDelay")
    .ignore ("TcpNoDelay")
    .ignore ("ClusterAddressing")
    .ignore ("EmergencyKillWaitTime")
    .ignore ("EmergencyKillSignal")
    .ignore ("SendConnectionTimeBudget")
    ;

  bool ret = ct.run (cf);
  if (ret) {
    ok_sys_sel_policy = sfs_core::select_policy_from_str (sys_sel_tmp);
  }
  if (gzip_tmp) {
    ok_gzip_mode = ok_gzip_str_to_mode (gzip_tmp);
  }

  if (tmp_dump_user) { _coredump_usr = ok_usr_t (tmp_dump_user); }
  if (tmp_dump_group) { _coredump_grp = ok_grp_t (tmp_dump_group); }

  return ret;
}

//-----------------------------------------------------------------------

bool
okld_t::parse_bind_reattempt_schedule ()
{
  bool ok = true;

  if (!_bind_reattempt_schedule_str) {
    // Defaut schedule
    _bind_reattempt_schedule_str = "1,1,2,4,8";
  }

  if (_bind_reattempt_schedule_str == "none" ||
      _bind_reattempt_schedule_str == "0") {
    _bind_reattempt_schedule_str = NULL;
  } else {
    vec<str> s;
    static rxx x ("\\s*,\\s*");
    split (&s, x, _bind_reattempt_schedule_str);
    for (size_t i = 0; i < s.size (); i++) {
      time_t tmp = 0;
      if (!convertint (s[i], &tmp) && tmp > 1000) {
	warn << "BindReattemptSchedule: bad interval: " << s[i] << "\n";
	ok = false;
      } else {
	_bind_reattempt_schedule.push_back (tmp);
      }
    }
  }
  return ok;
}

//-----------------------------------------------------------------------

bool
okld_t::post_config (const str &cf)
{
  bool errors = false;

  // allow these to be set
  if (_config_root) ok_root = _config_root;
  if (_config_wheel) ok_wheel = _config_wheel;

  if (_config_grp) {
    svc_grp = ok_grp_t (_config_grp);
  } else {
    warn << "no 'ServiceGroup' provided; using default: " 
	 << svc_grp.getname () << "\n";
  }
  if (_config_okd_un) _okd.set_user (_config_okd_un);
  if (_config_okd_gr) _okd.set_group (_config_okd_gr);

  const str* k;
  qhash_const_iterator_t<str, ptr<okld_helper_ssl_t> > it(_okssls);
  while ((k = it.next())) {
      ptr<okld_helper_ssl_t> okssl = *_okssls[*k];
      if (_config_ssl_un) okssl->set_user (_config_ssl_un);
      if (_config_ssl_gr) okssl->set_group (_config_ssl_gr);
  }

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
    warn << "ServiceFDHighWat must be greater than ServiceFDLowWat!\n";
    errors = true;
  }

  if (!_pubd_exc) { guess_pubd (cf); }
  if (!_pubd_exc) {
    warn << "Cannot continue without pubd support\n";
    errors = true;
  }

  if (_all_ports_map.size () == 0) {
    warn << "No listen ports given; assuming default port (80)\n";
    add_port (listenport, false);
  }

  if (!parse_bind_reattempt_schedule ()) {
    errors = true;
  }

  if (!check_services_and_aliases () || 
      !fixup_ssl_ports () ||
      !check_ports () || 
      !check_service_ports ()) 
    errors = true;

  if (!hostname)
    hostname = myname ();

  return (!errors);
}

//-----------------------------------------------------------------------

tamed void 
okld_t::launch_pubd (evi_t ev)
{
  tvars {
    bool b;
    int ret (-1);
  }
  assert (_pubd_exc);
  _pubd = New clone_only_client_t (_pubd_exc, PUB3_CLONE);

  twait { _pubd->connect (mkevent (b)); }
  if (b) {
    twait { _pubd->clone (mkevent (ret)); }
    if (ret < 0) {
      warn << "pubd did not send a file descriptor; failing\n";
    }
  } else {
    warn << "cannot connect to pubd\n";
  }
  ev->trigger (ret);
}

//-----------------------------------------------------------------------

void
okld_t::logd_crashed ()
{
  if (_die_on_logd_crash) {
    warn << "Shutting down on logd crash\n";
    shutdown1 ();
  } else {
    warn << "logd crashed, but taking no action\n";
  }
}

//-----------------------------------------------------------------------

tamed void 
okld_t::launch_logd (evi_t cb)
{
  tvars {
    vec<str> *argv;
    bool b;
    int ret (-1);
  }
  if (!logexc) 
    logexc = New helper_exec_t (oklog_program_1, "oklogd", 0,
				HLP_OPT_PING|HLP_OPT_QUEUE|HLP_OPT_NORETRY);
  argv = logexc->get_argv ();
  assert (argv);
  argv->push_back (logd_parms.encode ());

  _log_primary = New log_primary_t (logexc);
  _log_primary->set_fail_cb (wrap (this, &okld_t::logd_crashed));

  twait { _log_primary->connect (mkevent (b)); }
  if (b) {
    twait { _log_primary->clone(mkevent (ret)); }
    if (ret < 0) {
      warn << "oklogd did not send a file descriptor; failing\n";
    }
  } else {
    warn << "cannot connect to launched log daemon (oklogd)\n";
  }
  
  cb->trigger (ret);
}

//-----------------------------------------------------------------------

void
okld_t::encode_env ()
{
  _env.insert ("hostname", hostname)
    .insert ("jaildir", jaildir)
    .insert ("version", version)
    .insert ("listenport", listenport)
    .insert ("okwsname", reported_name)
    .insert ("server", global_okws_server_label)
    .insert ("jailed", int (jaildir && !uid))
    .insert ("logfmt", logd_parms.accesslog_fmt)
    .insert ("svclog", int (logd_parms.svclog))
    .insert ("gzip", int (ok_gzip_mode))
    .insert ("gziplev", ok_gzip_compress_level)
    .insert ("gzipnlev", ok_gzip_naive_compress_level)
    .insert ("gzipcsl", ok_gzip_cache_storelimit)
    .insert ("logtick", ok_log_tick)
    .insert ("logprd", ok_log_period)
    .insert ("clito", ok_clnt_timeout)
    .insert ("rsl", ok_recycle_suio_limit)
    .insert ("reqszlimit", ok_reqsize_limit)
    .insert ("cgilimit", ok_cgibuf_limit)
    .insert ("filtercgi", ok_filter_cgi)
    .insert ("ssdi", ok_ssdi)
    .insert ("sendsin", ok_send_sin ? 1 : 0)
    .insert ("fdlw", ok_svc_fds_low_wat)
    .insert ("fdhw", ok_svc_fds_high_wat)
    .insert ("acmsg", ok_svc_accept_msgs)
    .insert ("dz", ok_dangerous_zbufs ? 1 : 0)
    .insert ("ps", ok_axprt_ps)
    .insert ("clock", int (clock_mode))
    .insert ("ncto", ok_pub3_svc_neg_cache_timeout)
    .insert ("pub3chnk", ok_pub3_max_datasz)
    .insert ("sel", int (ok_sys_sel_policy))
    .insert ("p3jse", ok_pub3_json_strict_escaping)
    .insert ("lqm", ok_listen_queue_max)
    .insert ("jsibm", ok_pub3_json_int_bitmax)
    .insert ("asr", _aggressive_svc_restart)
    .insert ("gzch", ok_gzip_chunking)
    .insert ("gzchos", ok_gzip_chunking_old_safaris)
    .insert ("gzep", ok_gzip_error_pages)
    .insert ("dolc", _die_on_logd_crash)
    ;
  

  if (_opt_daemon) {
    str l = ok::syslog_ctl.to_str ();
    _env.insert ("loggers", l, false);
  }

  if (mmc_file) {
    _env.insert ("mmcf", mmc_file);
  }
}

//-----------------------------------------------------------------------


void
okld_t::got_logd_exec (vec<str> s, str loc, bool *errp)
{
  ptr<env_argv_t> env;
  if (ok_base_t::got_generic_exec (s, loc, errp, &env)) 
    logexc = New helper_exec_t (oklog_program_1, s, 0,
				HLP_OPT_PING|HLP_OPT_QUEUE|HLP_OPT_NORETRY,
				env);
}

//-----------------------------------------------------------------------

bool
okld_t::guess_pubd (const str &cf)
{
  bool err (false);
  vec<str> v;
  
  warn << "No '" << PUBD_KEY << "' given; guessing where to find pubd.\n";
  v.push_back (PUBD_KEY);
  v.push_back ("pubd");
  got_pubd_exec (v, cf, &err);
  
  return !err;
}

//-----------------------------------------------------------------------

void
okld_t::got_pubd_exec (vec<str> s, str loc, bool *errp)
{
  ptr<env_argv_t> env;
  if (ok_base_t::got_generic_exec (s, loc, errp, &env)) {
    _pubd_exc = New helper_exec_t (pub_prog_3, s, 1,
				   HLP_OPT_PING|HLP_OPT_QUEUE|HLP_OPT_NORETRY,
				   env);
    
    // pubd already uses -s, so we supply a different command line flag
    // to pass pubd its end of the Unix domain socket
    _pubd_exc->set_command_line_flag ("-x");
  }
}

//-----------------------------------------------------------------------

bool
okld_t::fix_uids ()
{
  if (!will_jail ())
    return true;

  bool ret = true;
  nxtuid = ok_svc_uid_low;
  size_t lim = _svcs.size ();
  bhash<u_int> uids_in_use;
  bhash<u_int> owners;
  if (int (lim) > ok_svc_uid_high - ok_svc_uid_low && !unsafe_mode) {
    warn << "too many services / ran out of UIDs\n";
    return false;
  }

  for (size_t i = 0; i < lim; i++) {
    okld_ch_cluster_t *svc = _svcs[i];

    int ueuid_orig = svc->get_exec_uid ();
    int ueuid_own = ueuid_orig;
    int uegid_orig = svc->get_exec_gid ();
    int uegid = uegid_orig;
    int mode = svc->get_exec_mode ();

    if (unsafe_mode) {
      svc->assign_uid (ueuid_own);
      svc->set_unsafe ();
    } else {
      if (svc->usr ()) {
	// in this case, the operator specified the user via a -u option
	// to the Service line.
	uegid = svc->usr ()->getid ();
	if (uids_in_use[uegid]) {
	  warn << "The UID " << uegid
	       << " that you specified for service " << svc->_servpath << "\n"
	       << "\t is in use by another, and I'm refusing to use it.\n";
	  return false;
	} else if (owners[uegid]) {
	  warn << "The UID " << uegid
	       << " that you specified for service " << svc->_servpath << "\n"
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
      }
      
      if (uegid > ok_svc_uid_high) {
	warn << "too many services / ran out of UIDs\n";
	return false;
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
  }

  if (!fix_coredump_uids ()) {
    ret = false;
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
okld_t::fix_coredump_uids ()
{
  bool ret = true;
  if (will_jail ()) {

    if (!_coredump_usr) {
      str s = _coredump_usr.getname ();
      warn << "Cannot find coredump user: " << s << "\n";
      ret = false;
    }

    if (!_coredump_grp) {
      str s = _coredump_grp.getname ();
      warn << "Cannot find coredump group: " << s << "\n";
      ret = false;
    }
  }
  return ret;
}

//-----------------------------------------------------------------------

void
okld_t::caught_signal (int sig)
{
  warn << "received shutdown signal (" << sig << "); shutting down\n";
  shutdown1 ();
}

//-----------------------------------------------------------------------

void
okld_t::caught_okd_eof ()
{
  warn << "Caught okd EOF on internal socket.\n";
  shutdown1 ();
}

//-----------------------------------------------------------------------

tamed void 
okld_t::poke_lazy_service_2 (const oksvc_proc_t &p, okstat_ev_t ev)
{
  tvars {
    ptr<okld_ch_cluster_t> *cl;
    ptr<okld_ch_t> ch;
    ok_xstatus_typ_t ret;
    bool ok;
  }

  if (sdflag) {
    warn << "Cannot start service " << p.name << " due to okld shutdown\n";
    ret = OK_STATUS_DEADCHILD;
  } else if ((cl = _svc_lookup[p.name]) && 
	     (ch = (*cl)->get_child (p.brother_id))) {
    twait { ch->lazy_startup (mkevent (ok)); }
    ret = ok ? OK_STATUS_OK : OK_STATUS_BADWAKEUP;
  } else {
    ret = OK_STATUS_NOSUCHCHILD;
  }
  ev->trigger (ret);
}

//-----------------------------------------------------------------------

void
okld_t::okd_dispatch (svccb *sbp)
{
  if (!sbp) {
    // Handle the shutdown in the client part of the library.
  } else {
    switch (sbp->proc ()) {
    case OKLD_POKE_LAZY_SERVICE:
      poke_lazy_service (sbp);
      break;
    case OKLD_EMERGENCY_KILL:
      emergency_kill (sbp);
      break;
    default:
      sbp->reject (PROC_UNAVAIL);
    }
  }
}

//-----------------------------------------------------------------------

tamed void
okld_t::emergency_kill (svccb *sbp)
{
  tvars {
    RPC::okld_program_1::okld_emergency_kill_srv_t<svccb> srv (sbp);
    holdvar const emergency_kill_arg_t *arg (srv.getarg ());
    ok_xstatus_typ_t ret (OK_STATUS_OK);
    ptr<okld_ch_t> ch;
    ptr<okld_ch_cluster_t> *cl;
    oksvc_proc_t p (arg->svc.proc);
    int sig (arg->signal);
  }


  if (sdflag) {
    warn << "Cannot kill service " << p.name << " due to okld shutdown\n";
    ret = OK_STATUS_DEADCHILD;
  } else if ((cl = _svc_lookup[p.name]) && 
	     (ch = (*cl)->get_child (p.brother_id))) {
    ret = ch->kill (sig);
  } else {
    ret = OK_STATUS_NOSUCHCHILD;
  }

  warn << "emergency_kill(" << p.name << ":" << p.brother_id <<") called => ";
  rpc_print (warnx, ret);

  srv.reply (ret);
}

//-----------------------------------------------------------------------

tamed void
okld_t::poke_lazy_service (svccb *sbp)
{
  tvars {
    RPC::okld_program_1::okld_poke_lazy_service_srv_t<svccb> srv (sbp);
    const oksvc_descriptor_t *arg (srv.getarg ());
    ok_xstatus_typ_t typ;
  }
  twait { poke_lazy_service_2 (arg->proc, mkevent (typ)); }
  srv.reply (typ);
}

//-----------------------------------------------------------------------

void
okld_t::shutdown1 ()
{
  warn << "Starting shutdown, Phase 1\n";
  sdflag = true;
  _okd.disconnect ();
}

//-----------------------------------------------------------------------

void
okld_t::shutdown2 (int status)
{
  warn << "Shutdown, Phase 2\n";
  struct stat sb;
  if (status != 0)
    warn << "okd exitted uncleanly, with status=" << status << "\n";
  else
    warn << "caught clean okd exit\n";

  str s = jail2real (_okd_mgr_socket);

  if (logexc) {
    if (!logexc->remove_pidfile ()) {
      warn ("Oklogd pidfile removal failed: %m\n");
    }
  }

  if (stat (s.cstr (), &sb) == 0) {
    if (status != (NO_SOCKET_ALLOCATED * 256)) {
      warn << "Deleting management socket: " << s << "\n";
      unlink (s.cstr());
    } else {
      warn << "Leaving management socket (" << s 
	   << ") since it's likely not ours.\n";
    }
  }
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

//-----------------------------------------------------------------------

bool
okld_t::fixup_ssl_ports ()
{
  bool ret = true;
  ptr<okld_helper_ssl_t> okssl = okssl_by_name();
  if (okssl && okssl->active ()) {
    bool *sslp = _all_ports_map[ok_ssl_port];
    if (!sslp) { 
      add_ssl_port(ok_ssl_port); 
    } else if (!*sslp && _https_ports.size () == 0) {
      warn << "Cannot add default SSL port (" << ok_ssl_port 
	   << ") since it's already listening for HTTPS\n";
      ret = false;
    }
  }
  return ret;
}

//-----------------------------------------------------------------------

tamed void 
okld_t::launch_okssl (evb_t ev) {

    tvars {
        vec<str> argv;
        int logfd;
        bool rc (false);
        int fds[2];
        okws_send_ssl_arg_t arg;
        ok_xstatus_typ_t res;
        clnt_stat err;
        str cl;
        qhash_const_iterator_t<str, ptr<okld_helper_ssl_t> > 
            it(_self->okssls());
        const str* k;
        ptr<okld_helper_ssl_t> okssl;
        vec<okws1_port_t> ports;
    }

    while ((k = it.next())) {
        okssl = *_okssls[*k];
        warn << "starting okssld channel: " << *k << "\n";
        argv.clear();
        if (!okssl->active ()) {
            rc = true;
        } else {
            twait { get_log_primary ()->clone (mkevent (logfd)); }
            if (logfd < 0) {
                warn << "oklogd did not send a file descriptor; failing\n";
            } else if (socketpair (AF_UNIX, SOCK_STREAM, 0, fds) != 0) {
                warn ("socketpair failed when start okssld: %m\n");
            } else {
                argv.push_back ("-m");
                argv.push_back (strbuf () << okssl->_ssl_timeout);
                if (will_jail ()) {
                    argv.push_back ("-u");
                    argv.push_back (strbuf () << okssl->user ().getid ());
                    argv.push_back ("-g");
                    argv.push_back (strbuf () << okssl->group ().getid ());
                }
                argv.push_back ("-j");
                argv.push_back (jaildir);
                argv.push_back ("-d");
                argv.push_back (okssl->dumpdir ());
                argv.push_back ("-l");
                argv.push_back (strbuf () << logfd);
                argv.push_back ("-t");
                argv.push_back (strbuf () << fds[0]);
                if (okssl->certfile_resolved ()) {
                    argv.push_back ("-c");
                    argv.push_back (okssl->certfile_resolved ());
                }
                argv.push_back ("-k");
                argv.push_back (okssl->keyfile_resolved ());
                if (okssl->chainfile_resolved ()) {
                    argv.push_back ("-n");
                    argv.push_back (okssl->chainfile_resolved ()); 
                }

                ports = okssl->ports();
                for (size_t i = 0; i < ports.size (); i++) {
                    argv.push_back ("-p");
                    argv.push_back (strbuf () << ports[i]);
                }

                if ((cl = okssl->cipher_list ())) {
                    argv.push_back ("-L");
                    argv.push_back (cl);
                }

                if (okssl->cipher_order ()) {
                    argv.push_back("-O");
                }

                if (okssl->_cli_renog) {
                    argv.push_back("-R");
                }

                if (okssl->_ssl_debug_startup) {
                    argv.push_back("-D");
                }

                okssl->argv () += argv;

                if (okssl->launch ()) {
                    okssl->set_chldcb (wrap (this, &okld_t::shutdown_ssl));
                    okssl->make_cli (okld_program_1, 
                                     wrap (this, &okld_t::shutdown_ssl, 0));
                    twait {
                        _okd.x ()->sendfd (fds[1]);
                        arg.dummy = 0;
                        RPC::okld_program_1::okld_send_ssl_socket 
                            (_okd.cli (), arg, &res, mkevent (err));
                    }
                    if (err) {
                        warn << "Error sending SSL socket to okd: " << err << "\n";
                    } else if (res != OK_STATUS_OK) {
                        warn << "okd rejected SSL socket (";
                        rpc_print (warnx, res, 0, NULL, NULL);
                        warnx << ")\n";
                    } else {
                        rc = true;
                    }
                } else {
                    close (fds[1]);
                }
                close (fds[0]);
            }
        }
    }
    ev->trigger (rc);
}

//-----------------------------------------------------------------------

bool
okld_t::launch_okd (int logfd, int pub2fd)
{
  vec<str> argv;

  argv.push_back ("-f");
  argv.push_back (configfile);
  argv.push_back ("-l");
  argv.push_back (strbuf () << logfd);
  argv.push_back ("-c");
  argv.push_back (_okd.dumpdir ());
  argv.push_back ("-p");
  argv.push_back (strbuf () << listenport);
  argv.push_back ("-x");
  argv.push_back (strbuf () << pub2fd);
		  
  if (debug_stallfile) {
    argv.push_back ("-D");
    argv.push_back (strbuf (debug_stallfile) << ".okd");
  }

  _okd.argv () += argv;

  // launch okd synchronously; no point in us running if okd puked.
  if (!_okd.launch ()) {
    return false;
  }

  // we've passed this to OKD; don't need it anymore
  close (logfd);
  close (pub2fd);

  // two shutdown events -- first on close of fdsource_t<> on
  // okd.  Second, is okd's actual exit, which we'll wait on.
  _okd.set_chldcb (wrap (this, &okld_t::shutdown2));
  _okd.make_cli (okld_program_1, wrap (this, &okld_t::caught_okd_eof));

  // We only need an okd_dispatch in some cases.
  if (need_okd_rpc ()) {
    _okd.make_srv (okld_program_1, wrap (this, &okld_t::okd_dispatch));
  }

  return true;
}

//-----------------------------------------------------------------------

bool
okld_t::check_service_ports ()
{
  bool ret = true;
  size_t lim = _svcs.size ();
  for (size_t i = 0; i < lim; i++) {
    okws1_port_t p = _svcs[i]->get_port ();
    if (p && ! _all_ports_map[p]) {
      warn << _svcs[i]->loc () 
	   << ": service uses a port (" << p << ") that OKD does not "
	   << "listen on!!\n";
      ret = false;
    }
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
okld_t::check_exes ()
{
  if (!will_jail ())
    return true;

  bool ret = true;
  size_t lim = _svcs.size ();
  for (size_t i = 0; i < lim; i++) {
    // at this point in the boot process, we have not called chroot yet
    if (!_svcs[i]->can_exec ()) {
      ret = false;
    }
  }
  return ret;
}

//-----------------------------------------------------------------------

#if !SFSLITE_AT_VERSION(1,2,6,107)
static bool
check_syslog_logger (str s, vec<str> *out)
{
  bool ret = false;
  static rxx x ("\\s+");
  split (out, x, s);
  if (!out->size ()) {
    warn << "NULL logger speficified\n";
  } else {
    str cmd = (*out)[0];
    if (access (cmd.cstr (), X_OK) != 0) {
      warn ("Cannot access logger '%s': %m\n", cmd.cstr ());
    } else {
      ret = true;
    }
  }
  return ret;
}
#endif

//-----------------------------------------------------------------------

static void
usage ()
{
  warnx << "usage: okld [-q?] [-D <dbg>] [-f <configfile>] [-l <logger>]\n";
  exit (1);
}

//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  bool opt_daemon = false;
  str cf;
  setprogname (argv[0]);
  str dbf;
  str pidfile;
  str logger_cmd;
  vec<str> logger_argv;
  set_debug_flags ();

  int ch;
  while ((ch = getopt (argc, argv, "qf:D:?p:l:")) != -1)
    switch (ch) {
    case 'l':
      logger_cmd = optarg;
      break;
    case 'p':
      pidfile = optarg;
      break;
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

  sfsconst_init (true);
  if (!cf) 
    cf = get_okws_config ();


  if (logger_cmd) {
#if !SFSLITE_AT_VERSION(1,2,9,106)
    warn << "You must upgrade to sfslite v1.2.9.6 or better to use "
	 << "custom loggers\n";
    usage ();
#endif
    if (!opt_daemon) { 
      warn << "-l<logger> flag only can be used in daemon mode\n";
      usage (); 
    }
    else if (!check_syslog_logger (logger_cmd, &logger_argv)) { 
      warn ("logger '%s' is not available: %m\n", logger_cmd.cstr ());
      usage (); 
    }
  }

  // Need to daemonize before parsing config file, since the output
  // of the config file parsing operation should go into the syslog.
  // XXX bug -- this means we can't set SyslogpPriority from in okws_config,
  // and it means we must set the system logger (if non-standard)
  // via command-line flag.
  if (opt_daemon) {
    syslog_priority = ok_syslog_priority;

#if SFSLITE_AT_VERSION(1,2,9,106)
    if (logger_argv.size ()) { set_syslog_logger (logger_argv); }
#else
    assert (!logger_argv.size ());
#endif

    syslog_tag = ok_syslog_tag;
    daemonize (pidfile);
  }

  okld_t *okld = New okld_t ();
  okld->debug_stallfile = dbf;
  okld->set_signals ();

  okld->set_opt_daemon (opt_daemon);

  warn ("version %s, pid %d\n", OKWS_PATCHLEVEL_STR, int (getpid ()));
  okld->launch (cf);
  amain ();
}

//-----------------------------------------------------------------------

bool
okld_t::init_ssl () {
    qhash_const_iterator_t<str, ptr<okld_helper_ssl_t> > it(_okssls);
    const str* k;
    while ((k = it.next())) {
        ptr<okld_helper_ssl_t> okssl = *_okssls[*k];
        if (!okssl->configure_keys())
            return false;
    }
    return true;
}

//-----------------------------------------------------------------------

tamed void 
okld_t::launch (str cf)
{
  tvars {
    bool ok;
  }

  configfile = cf;
  if (!run_configs (cf))
    okld_exit (1);

  // Don't change selcet policy in okld for now.
  //set_sfs_select_policy ();

  if (_opt_daemon) {
    ok::syslog_ctl.start_loggers ();
  }

  init_clock_daemon ();

  encode_env ();
  if (!(check_exes () && 
	fix_uids () && 
	init_jaildir () && 
	init_interpreters () &&
	init_ssl ()))
    okld_exit (1);

  used_ports.clear ();

  if (jaildir)
    warn ("JailDirectory: %s\n", jaildir.cstr ());

  _pub_fd = -1;
  twait {
    launch_logd (mkevent (_log_fd));
    launch_pubd (mkevent (_pub_fd));
  }

  if (_log_fd < 0) {
    warn << "launch of log daemon (oklogd) failed\n";
    okld_exit (1);
  }

  // If specified, pubd2 must succeed in its initalization
  if (_pub_fd < 0) {
    warn << "launch of pub daemon (pubd) failed\n";
    okld_exit (1);
  }

  if (!launch_okd (_log_fd, _pub_fd))
    okld_exit (1);

  twait { launch_okssl (mkevent (ok)); }
  if (!ok) 
    okld_exit (1);
  
  chroot ();
  twait { launch_services (mkevent ()); }
}
  
//-----------------------------------------------------------------------

tamed void 
okld_t::launch_services (evv_t ev)
{
  tvars {
    size_t i, j (0);
    bool ok;
  }

  // First reserve a slot for all services.
  for (i = 0; !sdflag && i < _svcs.size (); i++) {
    if (sdflag) {
      warn << "Skipping service reservation due to shutdown flag.\n";
    } else {
      twait { _svcs[i]->reserve (lazy_startup (), mkevent (ok)); }
    }
  }

  // XXX - hack for now; there is a problem when we launch too many
  // services at once, in that we lose file descriptors around the
  // 128th FD is sent.  this is probably not a coincindence, and is
  // most likely limited to some kernel limit as to the number of
  // outstanding FDs allowed.  Note that **okd** is failing to 
  // retrieve the 128th file descriptor. Some small changes
  // to david's libraries might be able to fix this. For now,
  // we'll hack in a timeout to let okd catch up, if possible.
  for (i = 0; !sdflag && i < _svcs.size (); i++, j++) {
    if (okld_startup_batch_size && j >= okld_startup_batch_size) {
      twait { delaycb (okld_startup_batch_wait, 0, mkevent ()); }
      j = 0;
    }
    if (sdflag) { 
      warn << "not launching due to shutdown flag\n";
      return;
    }
    if (!lazy_startup() || _svcs[i]->has_direct_ports ()) {
      twait { _svcs[i]->launch (mkevent ()); }
    }
  }
  ev->trigger ();
}

//-----------------------------------------------------------------------

bool
okld_t::init_okssl_jaildirs() {
    qhash_const_iterator_t<str, ptr<okld_helper_ssl_t> > it(_okssls);
    const str* k;
    while ((k = it.next())) {
        ptr<okld_helper_ssl_t> okssl = *_okssls[*k];
        if (!okssl->configure(this, coredumpdir))
            return false;
    }
    return true;
}

//-----------------------------------------------------------------------------

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
      !jail_mkdir (root_coredir, 0750, &_coredump_usr, &_coredump_grp) ||
      !jail_mkdir_p (sockdir, 0755, &root, &wheel)) 
    ret = false;

  // try to get the right timezone. it's /etc/localtime
  // on FreeBSD, might want to do the right thing on linux too.
  jail_cp ("/etc/localtime", 0644, &root, &wheel);

  size_t lim = _svcs.size ();

  if (!_okd.active ()) {
    warn << "No okd was specified; cannot run!\n";
    ret = false;
  }

  if (!_okd.configure (this, coredumpdir) || !init_okssl_jaildirs())
    ret = false;

  // separate coredump directory for each service UID
  for (size_t i = 0; i < lim; i++) {
    str d = apply_container_dir (coredumpdir, 
				 strbuf () << _svcs[i]->usr ()->getid ());
    _svcs[i]->set_run_dir (d);
    if (!jail_mkdir (d, 0700, _svcs[i]->usr (), &svc_grp))
      ret = false;
  }

  return ret;
}

//-----------------------------------------------------------------------

void
okld_t::clock_daemon_died (int sig)
{
  warn << "mmcd died with status=" << sig << "\n";
  assert (mmcd_pid > 0);
  chldcb (mmcd_pid, NULL);
  mmcd_pid = -1;
}

//-----------------------------------------------------------------------

void
okld_t::relaunch_clock_daemon (int sig)
{
  clock_daemon_died (sig);
  init_clock_daemon ();
}

//-----------------------------------------------------------------------

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

//-----------------------------------------------------------------------

static void
init_interpreter (bool *ok, okld_interpreter_t *i)
{
  if (!i->base_init ())
    *ok = false;
}

//-----------------------------------------------------------------------

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

//-----------------------------------------------------------------------

bool
svc_options_t::apply_global_defaults (const str &s)
{
  bool ret = true;

  if (svc_reqs < 0) svc_reqs = ok_svc_life_reqs;
  if (svc_time < 0) svc_time = ok_svc_life_time;
  if (wss < 0) wss = ok_pub3_wss;
  if (pub3_caching < 0) pub3_caching = ok_pub3_caching;
  if (pub3_viserr < 0) pub3_viserr = ok_pub3_viserr;
  if (wait_for_signal < 0) wait_for_signal = 0;
  if (hiwat < 0) hiwat = ok_svc_fds_high_wat;
  if (lowat < 0) lowat = ok_svc_fds_low_wat;
  if (gzip < 0) gzip = ok_gzip_mode;
  if (gzip_level < 0) gzip_level = ok_gzip_compress_level;
  if (ahttpcon_zombie_warn < 0) ahttpcon_zombie_warn = ok_ahttpcon_zombie_warn;
  if (ahttpcon_zombie_timeout < 0)
    ahttpcon_zombie_timeout = ok_ahttpcon_zombie_timeout;

  if (lowat >= hiwat) {
    warn << "watermark error for service " << s 
	 << "; low (" << lowat << ") is not less than high (" << hiwat 
	 <<  ")\n";
    ret = false;
  }
  return ret;
}

//-----------------------------------------------------------------------

okld_helper_t::okld_helper_t (const str &n, const str &u, const str &g)
  : _name (n),
    _pid (-1),
    _usr (u),
    _grp (g),
    _active (false),
    _dumpdir ("/tmp") {}

//-----------------------------------------------------------------------

char *const *
okld_helper_t::env () const 
{
  if (_env) return *_env;
  else return _empty_env;
}

//-----------------------------------------------------------------------

void
okld_helper_t::set_group (const str &g)
{
  _grp = ok_grp_t (g);
}

//-----------------------------------------------------------------------

void
okld_helper_t::set_user (const str &u)
{
  _usr = ok_usr_t (u);
}

//-----------------------------------------------------------------------

bool
okld_helper_t::launch ()
{
  _x = axprt_unix_spawnv (_argv[0], _argv, 0, NULL, env ());
  _pid = axprt_unix_spawn_pid;
  return _x;
}

//-----------------------------------------------------------------------

void
okld_helper_t::set_chldcb (cbi::ptr cb)
{
  chldcb (_pid, cb);
}

//-----------------------------------------------------------------------

void
okld_helper_t::make_cli (const rpc_program &p, cbv::ptr eofcb)
{
  _cli = aclnt::alloc (_x, p);
  if (eofcb) 
    _cli->seteofcb (eofcb);
}

//-----------------------------------------------------------------------

void
okld_helper_t::make_srv (const rpc_program &p, callback<void, svccb *>::ref cb)
{
  _srv = asrv::alloc (_x, p, cb);
}

//-----------------------------------------------------------------------

void okld_helper_t::activate () { _active = true; }

//-----------------------------------------------------------------------

bool
okld_helper_t::configure (jailable_t *j, const str &prefix)
{
  return (!_active ||
	  (configure_user () &&
	   configure_group () &&
	   configure_dumpdir (j, prefix)));
}

//-----------------------------------------------------------------------

bool
okld_helper_t::configure_user ()
{
  bool rc = true;
  if (!_usr) {
    warn << "cannot find a user for " << _name << " (" << _usr.getname () 
	 << " does not exist)\n";
    rc = false;
  }
  return rc;
}

//-----------------------------------------------------------------------

bool
okld_helper_t::configure_group ()
{
  bool rc = true;
  if (!_grp) {
    warn << "cannot find group for okd (" << _grp.getname ()
	 << "does not exist)\n";
    rc = false;
  }
  return rc;
}

//-----------------------------------------------------------------------

bool
okld_helper_t::configure_dumpdir (jailable_t *j, const str &d)
{
  bool ret = true;
  _dumpdir = apply_container_dir (d, strbuf () << _usr.getid ());
  if (!j->jail_mkdir (_dumpdir, 0700, &_usr, &_grp)) {
    warn << "cannot allocate jail directory for " << _name << "\n";
    ret = false;
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
okld_helper_ssl_t::configure_keys ()
{
  bool ret = true;
  if (_active) {

    if (_chainfile)  {
      _chainfile_resolved = resolve (_chainfile, "chain file");
    } else if (_certfile) {
      _certfile_resolved = resolve (_certfile, "certificate");
    }

    if (!_chainfile_resolved && !_certfile_resolved) {
      ret = false;
    }

    if (!_keyfile) {
      ret = false;
    } else {
      _keyfile_resolved = resolve (_keyfile, "private key");
    }

  }
  return ret;
    
}

//-----------------------------------------------------------------------

okld_helper_ssl_t::okld_helper_ssl_t (const str &u, const str &g)
  : okld_helper_t ("okssld", u, g),
    _certfile (ok_ssl_certfile),
    _keyfile (ok_ssl_keyfile),
    _ssl_timeout (ok_ssl_timeout),
    _cipher_order (false),
    _cli_renog (false),
    _ssl_debug_startup (false) {}

//-----------------------------------------------------------------------

str
okld_helper_ssl_t::cipher_list () const
{
  str ret;
  if (_cipher_list && !cicmp (_cipher_list, "default")) {
    ret = _cipher_list;
  }
  return ret;
}

//-----------------------------------------------------------------------------

bool
okld_helper_ssl_t::cipher_order () const {
    return _cipher_order;
}

//-----------------------------------------------------------------------

str
okld_helper_ssl_t::resolve (const str &in, const char *which) const
{
  str out;
  if (!in || !in.len ()) {
    warn ("No file given for SSL %s\n", which);
  } else if  (in[0] == '/') {
    int rc = access (in.cstr (), R_OK);
    if (rc != 0) {
      warn ("cannot access %s %s: %m\n", which, in.cstr ());
    } else {
      out = in;
    }
  } else {
    out = okws_etcfile_required (in.cstr(), NULL, true);
  }
  return out;
}

//-----------------------------------------------------------------------

void
okld_t::shutdown_ssl (int i)
{


}

//-----------------------------------------------------------------------------

ptr<okld_helper_ssl_t> okld_t::okssl_by_name(str s) {
    ptr<okld_helper_ssl_t>* t;
    return (NULL != (t = _okssls[s])) ? *t : NULL;
}

//-----------------------------------------------------------------------------

void
okld_t::add_ssl_port(okws1_port_t port, str channel) {
    add_port(port, true);
    ptr<okld_helper_ssl_t> okssl = okssl_by_name(channel);
    if (!okssl) {
        warn << "trying to add port to invalid SSL channel: " << channel
             << "\n";
    } else
        okssl->add_port(port);
}

//-----------------------------------------------------------------------------

static str
get_ssl_channel(vec<str> v) {
    if (v.size() < 3)
        return "default";
    else 
        return v[1];
}

//-----------------------------------------------------------------------

void
okld_t::got_ssl_primary_port (vec<str> v, str loc, bool *errp)
{
    bool err = true;
    u_int port;
    if (v.size () < 2 || !convertint (v[v.size()-1], &port)) {
        warn << loc << ": usage: " << v[0] << " <port>\n";
    } else if (port > OK_PORT_MAX || port < OK_PORT_MIN) {
        warn << loc << ": port out of range (" << port << ")\n";
    } else if (_all_ports_map[port]) {
        warn << loc << ": report port #: " << port << "\n";
    } else {
        if (v.size() < 3)
            ok_ssl_port = port;
        add_ssl_port(port, get_ssl_channel(v));
        err = false;
    }
    if (err) *errp = true;
}

//-----------------------------------------------------------------------

void
okld_t::got_ssl_listen_port (vec<str> v, str loc, bool *errp)
{
    bool err = true;
    u_int port;
    if (v.size () < 2 || !convertint (v[v.size()-1], &port)) {
        warn << loc << ": usage: " << v[0] << " <port>\n";
    } else if (port > OK_PORT_MAX || port < OK_PORT_MIN) {
        warn << loc << ": port out of range (" << port << ")\n";
    } else if (_all_ports_map[port]) {
        warn << loc << ": report port #: " << port << "\n";
    } else {
        add_ssl_port(port, get_ssl_channel(v));
        err = false;
    }
    if (err) *errp = true;
}

//-----------------------------------------------------------------------------

// macro for creating a channel based SSL handler
#define HANDLE_SSL_CHANNEL_OPT(name) \
    void okld_t::got_##name(vec<str> s, str loc, bool *errp) {    \
        str key;                                                  \
        str data;                                                 \
        switch (s.size()) {                                       \
        case 2:                                                   \
            okssl_by_name("default")->parse_##name(s[1]);             \
            *errp = false;                                        \
            return;                                               \
        case 0:                                                   \
        case 1:                                                   \
            warn << loc << ": missing parameter for: "<< s[0] << "\n"; \
            *errp = true;                                         \
            return;                                               \
        case 3:                                                   \
            key = s[1];                                           \
            data = s[2];                                          \
            okssl_by_name(key)->parse_##name(data);                   \
            *errp = false;                                        \
            return;                                               \
        default:                                                  \
            warn << loc << ": too many parameters for: " << s[0] << "\n"; \
            return;                                               \
        };                                                        \
    } 

HANDLE_SSL_CHANNEL_OPT(certfile)
HANDLE_SSL_CHANNEL_OPT(keyfile)
HANDLE_SSL_CHANNEL_OPT(chainfile)
HANDLE_SSL_CHANNEL_OPT(ssl_timeout)
HANDLE_SSL_CHANNEL_OPT(cipher_list)
HANDLE_SSL_CHANNEL_OPT(cipher_order)
HANDLE_SSL_CHANNEL_OPT(cli_renog)
HANDLE_SSL_CHANNEL_OPT(ssl_debug_startup)

//-----------------------------------------------------------------------------

void
okld_t::got_ssl_create_channel(vec<str> s, str loc, bool* errp) {
    
    if (s.size() < 2) {
        warn << loc << ": no channel name specified for: "
             << s[0] << "\n";
    } else {
        warn << "creating SSL channel: " << s[1] << "\n";
        ptr<okld_helper_ssl_t> okssl = 
            New refcounted<okld_helper_ssl_t>(ok_ssl_uname, ok_ssl_gname);
        _okssls.insert(s[1], okssl);
        if (_auto_activate) {
            okld_t::got_generic_exec (okssl, _ssl_exec_params, loc, errp);
            okssl->activate ();
        }
    }
}

//-----------------------------------------------------------------------

void
okld_t::add_svc (ptr<okld_ch_cluster_t> ch)
{
  _svcs.push_back (ch);
  _svc_lookup.insert (ch->servpath (), ch);
}

//-----------------------------------------------------------------------

bool
svc_options_t::check_options (const str &loc) const
{
  bool ok = true;
  if (lowat >= 0 && hiwat >= 0 && lowat >= hiwat) {
    warn << loc <<  ": watermark error: "
	 << "low watermark (" << lowat << ") "
	 << "must be less than high watermark (" << hiwat << ")\n";
    ok = false;
  }

  return ok;
}

//-----------------------------------------------------------------------

tamed void
okld_t::gather_helper_fds (str s, int *lp, int *pp, evb_t ev)
{
  tvars {
    clone_only_client_t *p;
    int logfd (-1), pubfd (-1);
    bool ok (true);
  }

  p = get_pubd ();

  twait { 
    get_log_primary ()->clone (mkevent (logfd));
    if (p) p->clone (mkevent (pubfd));
  }
  
  if (logfd < 0) {
    okdbg_warn (ERROR, "%s: cannot connect to oklogd\n", s.cstr ());
    ok = false;
  } 

  if (p && pubfd < 0) {
    okdbg_warn (ERROR, "%s: cannot connect to pubd\n", s.cstr ());
    ok = false;
  }
  
  if (in_shutdown ()) {
    okdbg_warn (ERROR, "%s: shutdown received while relaunch service\n", 
		s.cstr ());
    ok = false;
  }

  *lp = logfd;
  *pp = pubfd;
  ev->trigger (ok);
}

//-----------------------------------------------------------------------

okld_t * okld_ch_t::okld () { return _cluster->okld (); }

//-----------------------------------------------------------------------

bool 
okld_t::need_okd_rpc () const { return lazy_startup () || _emerg_kill; }

//-----------------------------------------------------------------------

okld_t::okld_t () 
  : config_parser_t (), 
    _log_primary (NULL),
    svc_grp (ok_okd_gname),
    nxtuid (ok_svc_uid_low), 
    logexc (NULL), 
    _pubd_exc (NULL),
    coredumpdir (ok_coredumpdir), 
    sockdir (ok_sockdir), 
    sdflag (false), 
    service_bin (ok_service_bin),
    unsafe_mode (false), 
    safe_startup_fl (true),
    _okd ("okd", ok_okd_uname, ok_okd_gname),
    _auto_activate(false),
    clock_mode (SFS_CLOCK_GETTIME),
    mmcd (ok_mmcd), 
    mmcd_pid (-1), 
    launchp (0),
    used_primary_port (false),
    _pubd (NULL),
    _okd_mgr_socket (okd_mgr_socket),
    _opt_daemon (false),
    _lazy_startup (false),
    _coredump_usr (ok_coredump_user),
    _coredump_grp (ok_coredump_group),
    _coredump_mode (ok_coredump_mode),
    _aggressive_svc_restart (false),
    _die_on_logd_crash (false),
    _emerg_kill (false),
    _config_resolve_bins (false) {
        _okssls.insert("default", (New refcounted<okld_helper_ssl_t>
                                   (ok_ssl_uname, ok_ssl_gname)));
    }

//-----------------------------------------------------------------------

