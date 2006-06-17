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
#include "okerr.h"
#include "okmgr.h"
#include "okd.h"

static void
usage ()
{
  warnx << "usage: okmgr [-l | -p | -t | -1] [-a?] "
	<< "<file1> <file2> ...\n";
  exit (1);
}

okmgr_clnt_t::okmgr_clnt_t (const str &s) 
  : _err (false), _sockname (s) {}


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

okmgr_logturn_t::okmgr_logturn_t (const str &s)
  : okmgr_clnt_t (s) {}

okmgr_pub_t::okmgr_pub_t (const str &s, const vec<str> &f, int v)
  : okmgr_clnt_t (s), _version (v)
{
  _fns.rebind = true;
  _fns.files.setsize (f.size ());
  for (size_t i = 0; i < f.size (); i++)
    _fns.files[i] = f[i];
}

void
okmgr_pub_t::do_host (helper_unix_t *h, ok_xstatus_t *s, aclnt_cb cb)
{
  int procno = (_version == 2 ? OKMGR_REPUB2 : OKMGR_REPUB);
  h->call (procno, &_fns, s, cb);
}

void
okmgr_logturn_t::do_host (helper_unix_t *h, ok_xstatus_t *s, aclnt_cb cb)
{
  h->call (OKMGR_TURNLOG, NULL, s, cb);
}

void
okmgr_launch_t::do_host (helper_unix_t *h, ok_xstatus_t *s, aclnt_cb cb)
{
  h->call (OKMGR_RELAUNCH, &_progs, s, cb);
}

static str
parse_config (const str &f)
{
  vec<str> av;
  int line;
  str s;
  bool e;

  conftab ct;
  ct.add ("OkMgrSocket", &s);

  if (!can_read (f))
    return NULL;

  parseargs pa (f);
  while (pa.getline (&av, &line)) {
    if (ct.match (av, f, line, &e)) {
      return s;
    }
  }
  return NULL;
}

TAMED void
okmgr_clnt_t::run ()
{
  VARS {
    helper_unix_t *hlp;
    bool ok;
    int rc (0);
    ok_xstatus_t s;
    clnt_stat err;
  }

  hlp = New helper_unix_t (okmgr_program_1, _sockname);
  
  BLOCK { hlp->connect (@(ok)); }
  if (ok) {
    BLOCK { do_host (hlp, &s, @(err)); }
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

int 
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  int ch;
  ctl_mode_t m = CTL_MODE_LOGTURN;
  str sockname = okd_mgr_socket;
  str configfile;
  vec<str> files;
  ok_set_typ_t set_typ = OK_SET_SOME;
  int version = 2;

  while ((ch = getopt (argc, argv, "lts:f:")) != -1)
    switch (ch) {
    case 't':
      m = CTL_MODE_LOGTURN;
      break;
    case 'l':
      m = CTL_MODE_LAUNCH;
      break;
    case 's':
      sockname = optarg;
      break;
    case 'f':
      configfile = optarg;
      break;
    case 'a':
      set_typ = OK_SET_ALL;
      break;
    default:
      usage ();
      break;
    }

  for (int i = optind; i < argc; i++) 
    files.push_back (argv[i]);

  if ((set_typ == OK_SET_ALL) && (m != CTL_MODE_LAUNCH || files.size ())
      || (m != CTL_MODE_LOGTURN && set_typ == OK_SET_SOME && 
	  files.size () == 0))
    usage ();

  if (!configfile && !sockname) configfile = get_okws_config (false);
  if (!sockname) sockname = parse_config (configfile);
  if (!sockname) fatal << "No socket found to open\n";

  okmgr_clnt_t *t = NULL;
  switch (m) {
  case CTL_MODE_PUB:
    t = New okmgr_pub_t (sockname, files, version);
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
