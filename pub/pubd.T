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

#include "pub.h"
#include "pub_parse.h"
#include "xpub.h"
#include <unistd.h>
#include <stdlib.h>
#include "pubd.h"
#include "parseopt.h"
#include "pslave.h"
#include "pubd2.h"
#include "pubutil.h"
#include "okdbg.h"
#include "pub2.h"

// Version 1 - global variables used to time out connection if no 
// contact from parent
pslave_status_t slave_status;
bool parent_connect;
bool primary;

int version = 1;

template<class T> bool
config_var (pub_config_iface_t *p, const char *vname, T *out)
{
  str s = p->cfg (vname);
  return (s && convertint (s, out));
}

void
client_accept (ptr<axprt_stream> x)
{
  assert (version == 1);
  if (!x)
    fatal ("EOF from parent process.\n");
  parent_connect = true;
  bool p = primary && slave_status == PSLAVE_SLAVE;
  vNew pubserv_t (x, p);
  primary = false;
}

static void 
pubshutdown (int sig)
{
  warn << "Caught trigger: " << sig << "; shutting down\n";
  exit (0);
}

static void
set_signals ()
{
  sigcb (SIGTERM, wrap (pubshutdown, SIGTERM));
  sigcb (SIGINT,  wrap (pubshutdown, SIGINT));
}

static void
usage ()
{
  warn << "usage: pubd [-p port] [-w] [XXX other undocumented opts XXX]\n";
  exit (1);
}

static void
pubd_slave_cb ()
{
  if (!parent_connect)
    fatal << "exitting; no parent process found\n";
}

tamed static void
main_T (int argc, char **argv)
{
  tvars {
    u_int port (ok_pubd_port);
    int ch;
    str jaildir;
    str uname;
    str gname;
    str configfile;
    u_int opts (P_DAEMON);
    const char *e, *v;
    bool pub2_cache (true);
    bool have_cfg (false);

    // Is activated only if we are doing caching and in pub2
    pubserv2::cache_t *cache (NULL);
    pubserv2::file_lookup_t *fl (NULL);

    str tss, tsh; // treestat sentinel and heartbeat files
    pub_parser_t *ppt;
    ptr<pub2::local_publisher_t> cfgparse;
    int fdfd (1);
  }

  parent_connect = false;
  primary = true;

  /* need this call to setup global gzip table */
  zinit ();

  /* turn on debug flags set in the OKWS_DEBUG_OPTS environment variable */
  set_debug_flags ();
  if (OKDBG2(STALL_SIGCONT)) {
    warn << getpid () << ": blocking until SIGCONT trigger..\n";
    twait { sigcb (SIGCONT, mkevent ()); }
    warn << getpid () << ": unblocked\n";
    sigcb (SIGCONT, NULL);
  }

  /* 
   * set up global pub variables 
   * arg true = this pubd is an exporter.
   */
  ppt = pub_parser_t::alloc (true);

  if ((e = getenv ("PUBCONF")) && (v = getenvval (e)) && *v)
    configfile = v;

  while ((ch = getopt (argc, argv, "2vp:wj:u:g:f:s:x:CH:S:")) != -1)
    switch (ch) {
    case '2':
      version = 2;
      break;
    case 'p':
      if (!convertint (optarg, &port))
	usage ();
      break;
    case 'C':
      pub2_cache = false;
      break;
    case 's':
      if (!convertint (optarg, &ok_axprt_ps))
	usage ();
      break;
    case 'w':
      opts |= P_WSS;
      break;
    case 'j':
      jaildir = optarg;
      break;
    case 'u':
      uname = optarg;
      break;
    case 'g':
      gname = optarg;
      break;
    case 'f':
      configfile = optarg;
      break;
    case 'v':
      opts |= P_VERBOSE;
      break;
    case 'x':
      if (!convertint (optarg, &fdfd)) {
	warn << "-x expects an integer argument\n";
	usage ();
      }
      break;
    case 'S':
      tss = optarg;
      break;
    case 'H':
      tsh = optarg;
      break;
    default:
      usage ();
    }
  ppt->set_opts (opts);

  if (!configfile) 
    configfile = okws_etcfile (ok_pub_config);

  if (version == 2) {
    if (fdfd < 0) {
      warn << "pubd -2 also needs a -x with a Unix FD\n";
      exit (1);
    }
  }
  
  /*
   * A parse for parsing config files
   */
  cfgparse = New refcounted<pub2::local_publisher_t> (ppt, opts);

  have_cfg = false;
  if (configfile) {
    twait { cfgparse->run_cfg (configfile, mkevent (have_cfg)); }
  }
  if (!have_cfg) 
    warn << "pubd running without default variable bindings\n";

  if (version == 1) {
    if (!configfile || !ppt->parse_config (configfile))
      warn << "v1 error: cannot answer CONFIG RPC's\n";

    if (port == ok_pubd_port) {
      config_var (cfgparse, "PORT", &port);
    }
  }

  if (version == 2) {
    if (pub2_cache) {
      config_var (cfgparse, "RefreshIntervalMin", &ok_pub2_refresh_min);
      config_var (cfgparse, "RefreshIntervalMax", &ok_pub2_refresh_max);
      config_var (cfgparse, "RefreshIntervalIncr", &ok_pub2_refresh_incr);
      config_var (cfgparse, "NegativeLookupCacheTimeout",
		  &ok_pub2_neg_cache_timeout);
      config_var (cfgparse, "CleanCacheInterval", 
		  &ok_pub2_clean_cache_interval);
      config_var (cfgparse, "GetfileObjectLifetime", 
		  &ok_pub2_getfile_object_lifetime);
      config_var (cfgparse, "TreestatCheckInterval",
		  &ok_pub2_treestat_interval);

      fl = cache = New pubserv2::cache_t ();
    } else {
      fl = New pubserv2::chunkholder_t ();
    }
    
  } else if (!pub2_cache) {
    warn << "Can only use -C with -2!\n";
    usage ();
  }

  if (!jaildir) {
    if (!cfgparse->cfg ("JailDir", &jaildir, true)) {
      warn << "Cannot find a jail directory; none was specified with -j\n"
	   << "  and no 'JailDir' found in config file.\n";
      exit (1);
    }
  }

  if (!uname) uname = cfgparse->cfg ("RunAsUser", true);
  if (!gname) gname = cfgparse->cfg ("RunAsGroup", true);


  warn ("OKWS version %s, Pub v%d, pid %d\n", 
	OKWS_PATCHLEVEL_STR, version, int (getpid ()));

  if (version == 1) {
    if ((slave_status = pub_slave (wrap (client_accept), port, &slave_status)) 
	== PSLAVE_ERR) {
      warn << port << ": port already in use / can't bind to port\n";
      exit (1);
    }
  } else {
    // Since we're spawned via axprt_unix_dospawnv, that means
    // that our parent is going to be sending us RPCs on
    // standard input.
    ptr<axprt_unix> x (axprt_unix::alloc (0, ok_axprt_ps));
    vNew pubserv2::primary_srv_t (x, fl, fdfd);
  }
	  

  setsid ();

  // set the privileges for the regular parse running
  ppt->setprivs (jaildir, uname, gname);

  // After we set the jail directory, then we can start messing around
  // with sentinel files.
  if (cache) {
    
    if (!tss && !cfgparse->cfg ("TreeStatSentinel",  &tss, true))
      tss = ppt->jail2real (ok_pub2_treestat_sentinel);
    if (!tsh && !cfgparse->cfg ("TreeStatHeartbeat", &tsh, true))
      tsh = ppt->jail2real (ok_pub2_treestat_heartbeat);
    
    OKDBG4(PUB2_CACHE, CHATTER, 
	   "treestat sentinel file: %s\n"
	   "treestat heartbeat file: %s\n",
	   tss.cstr (), tsh.cstr ());
    cache->set_ts_files (tss, tsh);
    cache->start_timer ();
  }

  cfgparse = NULL;
  set_signals ();
  if (version == 1 && slave_status == PSLAVE_SLAVE)
    delaycb (ok_connect_wait, 0, wrap (pubd_slave_cb));
}


int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  main_T (argc, argv);
  amain ();
}

