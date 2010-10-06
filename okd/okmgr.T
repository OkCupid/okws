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
  warnx << "usage: okmgr [ -l | -t ] [-av] [-s <socket> | -f <conf> ]"
	<< "<svc1> <svc2> ...\n"
	<< "       okmgr <-c|-p|-T> [ enable | disable | print | reset ] "
	<< "[-s <socket> | -f <conf>] <svc>\n"
	<< "       okmgr [-s <socket> | -f <conf>] -m<msg> <svc1> <svc2>..\n";
				     
  exit (1);
}

//-----------------------------------------------------------------------

okmgr_clnt_t::okmgr_clnt_t (const str &s) 
  : _err (false), _sockname (s) {}

//-----------------------------------------------------------------------

okmgr_leak_checker_t::okmgr_leak_checker_t (const str &s, oksvc_proc_t p,
					    ok_diagnostic_cmd_t cmd)
  : okmgr_clnt_t (s), _proc (p), _cmd (cmd) {}

//-----------------------------------------------------------------------

okmgr_profiler_t::okmgr_profiler_t (const str &s, oksvc_proc_t p,
				    ok_diagnostic_cmd_t cmd)
  : okmgr_clnt_t (s), _proc (p), _cmd (cmd) {}

//-----------------------------------------------------------------------

okmgr_tame_profiler_t::okmgr_tame_profiler_t (const str &s, oksvc_proc_t p,
					      ok_diagnostic_cmd_t cmd)
  : okmgr_clnt_t (s), _proc (p), _cmd (cmd) {}

//-----------------------------------------------------------------------

okmgr_launch_t::okmgr_launch_t (const str &s, oksvc_procs_t p,
				ok_set_typ_t t) 
  : okmgr_clnt_t (s), _procs (p)
{}

//-----------------------------------------------------------------------

static oksvc_proc_t
str2proc (const str &in, int def)
{
  oksvc_proc_t ret;
  ret.num_brothers = 0;
  static rxx x ("^(\\S+):([0-9]+)$");
  if (x.match (in) && convertint (x[2], &ret.brother_id)) {
    ret.name = x[1];
  } else {
    ret.name = in;
    ret.brother_id = def;
  }
  return ret;
}

//-----------------------------------------------------------------------

static oksvc_procs_t 
strs2procs (const vec<str> &files, int def)
{
  oksvc_procs_t ret (OK_SET_SOME);
  ret.procs->setsize (files.size ());
  for (size_t i = 0; i < files.size (); i++) {
    (*ret.procs)[i] = str2proc (files[i], def);
  }
  return ret;
}

//-----------------------------------------------------------------------

okmgr_msg_t::okmgr_msg_t (const str &s, oksvc_procs_t p, ok_set_typ_t t, str m)
  : okmgr_clnt_t (s),
    _procs (p),
    _msg (m)
{}

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
  h->call (OKMGR_RELAUNCH, &_procs, s, cb);
}

//-----------------------------------------------------------------------

void
okmgr_msg_t::do_host (helper_unix_t *h, ok_xstatus_t *s, aclnt_cb cb)
{
  okmgr_send_msg_arg_t arg;
  arg.procs = _procs;
  arg.msg = _msg;
  h->call (OKMGR_SEND_MSG, &arg, s, cb);
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
  str msg;

  while ((ch = getopt (argc, argv, "m:c:p:lts:f:FvT:")) != -1)
    switch (ch) {
    case 'm':
      m = CTL_MODE_SEND_MSG;
      msg = optarg;
      break;
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
      }
      break;
    case 'p':
      m = CTL_MODE_PROFILER;
      if ((cmd = optarg2cmd (optarg)) == OK_DIAGNOSTIC_NONE) {
	warn << "Unknown subcommand for argument -c\n";
	usage ();
      }
      break;
    case 'T':
      m = CTL_MODE_TAME_PROFILER;
      if ((cmd = optarg2cmd (optarg)) == OK_DIAGNOSTIC_NONE) {
	warn << "Unknown subcommand for argument -c\n";
	usage ();
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

  if ((m == CTL_MODE_LEAK_CHECKER ||
       m == CTL_MODE_TAME_PROFILER ||
       m == CTL_MODE_PROFILER) && files.size () != 1) {
    warn << "Option -c only applies to one service at a time\n";
    usage ();
  }

  // Can either set SET_ALL or some, but not both.
  if (set_typ == OK_SET_ALL && files.size ())
    usage ();

  // Can only use SET_ALL in LAUNCH or SEND_MSG mode
  if (set_typ == OK_SET_ALL && m != CTL_MODE_LAUNCH && m != CTL_MODE_SEND_MSG)
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
    t = New okmgr_leak_checker_t (sockname, str2proc (files[0], 0), cmd);
    break;
  case CTL_MODE_SEND_MSG:
    t = New okmgr_msg_t (sockname, strs2procs (files, -1), set_typ, msg);
    break;
  case CTL_MODE_PROFILER:
    t = New okmgr_profiler_t (sockname, str2proc (files[0], 0), cmd);
    break;
  case CTL_MODE_TAME_PROFILER:
    t = New okmgr_tame_profiler_t (sockname, str2proc (files[0], 0), cmd);
    break;
  case CTL_MODE_LAUNCH:
    t = New okmgr_launch_t (sockname, strs2procs (files, -1), set_typ);
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
  arg.proc = _proc;
  arg.cmd = _cmd;

  RPC::okmgr_program_1::okmgr_leak_checker (h, arg, s, cb);
}

//-----------------------------------------------------------------------

void
okmgr_profiler_t::do_host (helper_unix_t *h, ok_xstatus_t *s, aclnt_cb cb)
{
  okmgr_diagnostic_arg_t arg;
  arg.proc = _proc;
  arg.cmd = _cmd;

  RPC::okmgr_program_1::okmgr_profiler (h, arg, s, cb);
}

//-----------------------------------------------------------------------

void
okmgr_tame_profiler_t::do_host (helper_unix_t *h, ok_xstatus_t *s, aclnt_cb cb)
{
  okmgr_diagnostic_arg_t arg;
  arg.proc = _proc;
  arg.cmd = _cmd;

  RPC::okmgr_program_1::okmgr_tame_profiler (h, arg, s, cb);
}

//-----------------------------------------------------------------------
