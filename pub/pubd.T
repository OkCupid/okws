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
#include <unistd.h>
#include <stdlib.h>
#include "pubd.h"
#include "parseopt.h"
#include "pslave.h"
#include "pubd.h"
#include "pubutil.h"
#include "okdbg.h"
#include "pub3.h"
#include "pub3obj.h"

extern int yydebug;

//-----------------------------------------------------------------------

static void 
pubshutdown (int sig)
{
  warn << "Caught trigger: " << sig << "; shutting down\n";
  exit (0);
}

//-----------------------------------------------------------------------

static void
set_signals ()
{
  sigcb (SIGTERM, wrap (pubshutdown, SIGTERM));
  sigcb (SIGINT,  wrap (pubshutdown, SIGINT));
}

//-----------------------------------------------------------------------

static void
usage ()
{
  warn << "usage: pubd [-p port] [-w] [XXX other undocumented opts XXX]\n";
  exit (1);
}

//-----------------------------------------------------------------------

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
    pub3::opts_t opts (0);
    const char  *v;
    bool use_cache (true);
    bool have_cfg (false);

    // Is activated only if we are doing caching and in pub2
    ptr<pub3::srv_cache_t> cache;
    ptr<pub3::srv_file_lookup_t> fl;

    str tss, tsh; // treestat sentinel and heartbeat files
    ptr<pub3::pub_parser_t> ppt;
    ptr<pub3::local_publisher_t> cfgparse;
    int fdfd (1);
    pub3::obj_dict_t cfg_d;
    ptr<pub3::master_srv_t> srv;
    int rc;
  }

#ifdef PDEBUG
  if (OKDBG2(PUB_PARSE)) {
    yydebug = 1;
  }
#endif

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

  ppt = pub3::pub_parser_t::alloc ();

  if ((v = getenv ("OKWS_PUB_CONFFILE")) && strlen (v)) {
    configfile = v;
  }
  
  if ((v = getenv ("OKWS_PUB_JAILDIR")) && strlen (v)) {
    jaildir = v;
  }
 
  while ((ch = getopt (argc, argv, "vp:j:u:g:f:s:x:CH:S:")) != -1) {
    switch (ch) {
    case 'p':
      if (!convertint (optarg, &port)) 
	usage ();
      break;
    case 'C':
      use_cache = false;
      break;
    case 's':
      if (!convertint (optarg, &ok_axprt_ps))
	usage ();
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
      opts |= pub3::P_VERBOSE;
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
  }

  if (!configfile) 
    configfile = okws_etcfile (ok_pub_config);

  if (fdfd < 0) {
    warn << "pubd needs a -x with a Unix FD\n";
    exit (1);
  }
  
  /*
   * A parse for parsing config files
   */
  cfgparse = New refcounted<pub3::local_publisher_t> (ppt, opts);

  have_cfg = false;
  if (configfile) {
    twait { cfgparse->run_cfg (configfile, mkevent (have_cfg), cfg_d.dict ()); }
  }

  if (!have_cfg) 
    warn << "pubd running without default variable bindings\n";

  if (use_cache) {
    cfg_d ("RefreshIntervalMin").to_int (&ok_pub3_refresh_min);
    cfg_d ("RefreshIntervalMax").to_int (&ok_pub3_refresh_max);
    cfg_d ("RefreshIntervalIncr").to_int (&ok_pub3_refresh_incr);
    cfg_d ("NegativeLookupCacheTimeout").to_int (&ok_pub3_neg_cache_timeout);
    cfg_d ("CleanCacheInterval").to_int (&ok_pub3_clean_cache_interval);
    cfg_d ("GetfileObjectLifetime").to_int (&ok_pub3_getfile_object_lifetime);
    cfg_d ("TreestatCheckInterval").to_int (&ok_pub3_treestat_interval);
    
    fl = cache = pub3::srv_cache_t::alloc ();
  } else {
    fl = pub3::chunkholder_t::alloc ();
  }

  if (!jaildir) {
    if (!cfg_d ("JailDir").to_str (&jaildir)) {
      warn << "Cannot find a jail directory; none was specified with -j\n"
	   << "  and no 'JailDir' found in config file.\n";
      exit (1);
    }
  }

  struct stat sb;
  if (stat (jaildir.cstr (), &sb) != 0) {
    warn << "No directory found for JailDir: " << jaildir << "\n";
    exit (1);
  } else if (!S_ISDIR(sb.st_mode)) {
    warn << "Supposed 'JailDir' is not a directory: " << jaildir << "\n";
    exit (1);
  }

  if (!uname) uname = cfg_d ("RunAsUser").to_str ();
  if (!gname) gname = cfg_d ("RunAsGroup").to_str ();

  warn ("OKWS version %s, Pub v3, pid %d\n", 
	OKWS_PATCHLEVEL_STR, int (getpid ()));


  // Since we're spawned via axprt_unix_dospawnv, that means
  // that our parent is going to be sending us RPCs on
  // standard input.
  {
    ptr<axprt_unix> x = axprt_unix::alloc (0, ok_axprt_ps);
    ptr<pub3::local_publisher_t> pub = 
      New refcounted<pub3::local_publisher_t> (ppt, opts, fl);

    srv = New refcounted<pub3::master_srv_t> (x, pub, fl, fdfd);
  }

  setsid ();

  // set the privileges for the regular parse running
  if (!srv->do_chroot (jaildir, uname, gname)) {
    warn << "Failed to jail and drop privileges\n";
    exit (1);
  }

  // After we set the jail directory, then we can start messing around
  // with sentinel files.
  if (cache) {
    
    if (!tss && !cfg_d("TreeStatSentinel").to_str (&tss)) {
      tss = srv->jail2real (ok_pub3_treestat_sentinel);
    }
    if (!tsh && !cfg_d("TreeStatHeartbeat").to_str (&tsh)) {
      tsh = srv->jail2real (ok_pub3_treestat_heartbeat);
    }
    
    OKDBG4(PUB3_CACHE, CHATTER, 
	   "treestat sentinel file: %s\n"
	   "treestat heartbeat file: %s\n",
	   tss.cstr (), tsh.cstr ());
    cache->set_ts_files (tss, tsh);
    cache->start_timer ();
  }

  cfgparse = NULL;
  set_signals ();

  twait { srv->run (mkevent (rc)); }
  exit (rc);
}

//-----------------------------------------------------------------------

int
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  main_T (argc, argv);
  amain ();
}

//-----------------------------------------------------------------------
