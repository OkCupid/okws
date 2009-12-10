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

#include "okprot.h"
#include "ok.h"
#include "okd.h"
#include "okmgr.h"

static void
usage ()
{
  warnx << "usage: okmgr [ -l | -t | -1 ] [-av] [-s <socket> | -f <conf> ]"
	<< "<svc1> <svc2> ...\n"
	<< "       okmgr <-c|-p> [ enable | disable | print | clear ] "
	<< "[-s <socket> | -f <conf>] <svc>\n";
				     
  exit (1);
}

//-----------------------------------------------------------------------

okmgr_clnt_t::okmgr_clnt_t (const str &s) 
  : _err (false), _sockname (s) {}

//-----------------------------------------------------------------------

okmgr_leak_checker_t::okmgr_leak_checker_t (const str &s, const str &prog,
					    ok_diagnostic_cmd_t cmd)
  : okmgr_clnt_t (s), _prog (prog), _cmd (cmd) {}

//-----------------------------------------------------------------------

okmgr_profiler_t::okmgr_profiler_t (const str &s, const str &prog,
				    ok_diagnostic_cmd_t cmd)
  : okmgr_clnt_t (s), _prog (prog), _cmd (cmd) {}

//-----------------------------------------------------------------------

okmgr_launch_t::okmgr_launch_t (const str &s, const vec<str> &f,
				ok_set_typ_t t) 
  : okmgr_clnt_t (s), _progs (t)
{
  if (t == OK_SET_SOME) {
    _progs.progs->setsize (f.size ());
    for (size_t i = 0; i < f.size (); i++)
      (*_progs.progs)[i] = f[i];
  }
}

//-----------------------------------------------------------------------

okmgr_logturn_t::okmgr_logturn_t (const str &s)
  : okmgr_clnt_t (s) {}

//-----------------------------------------------------------------------

void
okmgr_logturn_t::do_host (helper_unix_t *h, ok_xstatus_t *s, aclnt_cb cb)
{
  h->call (OKMGR_TURNLOG, NULL, s, cb);
}

//-----------------------------------------------------------------------

void
okmgr_launch_t::do_host (helper_unix_t *h, ok_xstatus_t *s, aclnt_cb cb)
{
  h->call (OKMGR_RELAUNCH, &_progs, s, cb);
}

//-----------------------------------------------------------------------

static str
parse_config (const str &f)
{
  vec<str> av;
  int line;
  str s;
  str j;
  bool e;
  struct stat sb;

  conftab ct;
  
  ct.add ("OkMgrSocket", &s)
    .add ("JailDir", &j);

  if (!can_read (f))
    return NULL;

  parseargs pa (f);
  while (pa.getline (&av, &line)) {
    ct.match (av, f, line, &e);
  }

  if (j) {
    if (!s) { s = okd_mgr_socket; }
    str s2 = s;
    if (s2[0] == '/') { s2 = substr (s2, 1, s2.len() - 1); }
    str x = apply_container_dir (j, s2);
    if (stat (x.cstr (), &sb) == 0) {
      return x;
    }
  }

  return s;
}

//-----------------------------------------------------------------------

tamed void
okmgr_clnt_t::run ()
{
  tvars {
    helper_unix_t *hlp;
    bool ok;
    int rc (0);
    ok_xstatus_t s;
    clnt_stat err;
  }

  hlp = New helper_unix_t (okmgr_program_1, _sockname);
  
  twait { hlp->connect (mkevent (ok)); }
  if (ok) {
    twait { do_host (hlp, &s, mkevent (err)); }
    rc = -1;
    if (err) warn << "RPC Error: " << err;
    else if (s.status != OK_STATUS_OK) warn << *s.error << "\n";
    else rc = 0;
  } else {
    warn << "Cannot connect to socket: " << _sockname << "\n";
    rc = -1;
  }

  exit (rc);
}

//-----------------------------------------------------------------------

static ok_diagnostic_cmd_t 
optarg2cmd (const char *a)
{
  ok_diagnostic_cmd_t cmd;
  switch (a[0]) {
  case 'e': cmd = OK_DIAGNOSTIC_ENABLE; break;
  case 'd': cmd = OK_DIAGNOSTIC_DISABLE; break;
  case 'r': cmd = OK_DIAGNOSTIC_RESET; break;
  case 'p': cmd = OK_DIAGNOSTIC_REPORT; break;
  default: cmd = OK_DIAGNOSTIC_NONE; break;
  }
  return cmd;
}

//-----------------------------------------------------------------------

int 
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  int ch;
  ctl_mode_t m = CTL_MODE_LOGTURN;
  str sockname;
  str sockname_arg;
  str configfile;
  vec<str> files;
  ok_set_typ_t set_typ = OK_SET_SOME;
  bool no_config = false;
  ok_diagnostic_cmd_t cmd = OK_DIAGNOSTIC_NONE;
  bool verbose = false;

  while ((ch = getopt (argc, argv, "c:p:lts:f:Fv")) != -1)
    switch (ch) {
    case 't':
      m = CTL_MODE_LOGTURN;
      break;
    case 'l':
      m = CTL_MODE_LAUNCH;
      break;
    case 'c':
      m = CTL_MODE_LEAK_CHECKER;
      if ((cmd = optarg2cmd (optarg)) == OK_DIAGNOSTIC_NONE) {
	warn << "Unknown subcommand for argument -c\n";
	usage ();
	break;
      }
      break;
    case 'p':
      m = CTL_MODE_PROFILER;
      if ((cmd = optarg2cmd (optarg)) == OK_DIAGNOSTIC_NONE) {
	warn << "Unknown subcommand for argument -c\n";
	usage ();
	break;
      }
      break;
    case 's':
      sockname_arg = optarg;
      break;
    case 'f':
      configfile = optarg;
      break;
    case 'F':
      no_config = true;
      break;
    case 'a':
      set_typ = OK_SET_ALL;
      break;
    case 'v':
      verbose = true;
      break;
    default:
      usage ();
      break;
    }

  for (int i = optind; i < argc; i++) 
    files.push_back (argv[i]);

  if (m == CTL_MODE_LEAK_CHECKER && files.size () != 1) {
    warn << "Option -c only applies to one service at a time\n";
    usage ();
  }

  if (((set_typ == OK_SET_ALL) && (m != CTL_MODE_LAUNCH || files.size ()))
      || (m != CTL_MODE_LOGTURN && 
	  set_typ == OK_SET_SOME && 
	  files.size () == 0))
    usage ();

  if (sockname_arg && configfile) usage ();
  if (no_config && configfile) usage ();

  if (sockname_arg) 
    sockname = sockname_arg;
  else if (no_config) {
    sockname = okd_mgr_socket;
  } else {
    if (!configfile) configfile = get_okws_config (true);
    sockname = parse_config (configfile);
  }
    
  if (!sockname) fatal << "No socket found to open\n";
  if (verbose) { warn << "Using socket: " << sockname <<"\n"; }

  okmgr_clnt_t *t = NULL;
  switch (m) {
  case CTL_MODE_LEAK_CHECKER:
    t = New okmgr_leak_checker_t (sockname, files[0], cmd);
    break;
  case CTL_MODE_PROFILER:
    t = New okmgr_profiler_t (sockname, files[0], cmd);
    break;
  case CTL_MODE_LAUNCH:
    t = New okmgr_launch_t (sockname, files, set_typ);
    break;
  case CTL_MODE_LOGTURN:
    t = New okmgr_logturn_t (sockname);
    break;
  default:
    usage ();
  }

  if (t) 
    t->run ();

  amain ();
}

//-----------------------------------------------------------------------

void
okmgr_leak_checker_t::do_host (helper_unix_t *h, ok_xstatus_t *s, aclnt_cb cb)
{
  okmgr_diagnostic_arg_t arg;
  arg.prog = _prog;
  arg.cmd = _cmd;

  RPC::okmgr_program_1::okmgr_leak_checker (h, arg, s, cb);
}

//-----------------------------------------------------------------------

void
okmgr_profiler_t::do_host (helper_unix_t *h, ok_xstatus_t *s, aclnt_cb cb)
{
  okmgr_diagnostic_arg_t arg;
  arg.prog = _prog;
  arg.cmd = _cmd;

  RPC::okmgr_program_1::okmgr_profiler (h, arg, s, cb);
}

//-----------------------------------------------------------------------
