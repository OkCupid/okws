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
#include "rxx.h"

static void
usage ()
{
  warnx << "usage: okmgr [-l | -p | -t | -2] [-a?] "
	<< "[-h <host>] <file1> <file2> ...\n";
  exit (1);
}

okmgr_clnt_t::okmgr_clnt_t (const vec<str> &h) : err (false)
{
  bool f = false;
  for (u_int i = 0; i < h.size (); i++) 
    if (!add_host (h[i])) {
      warn << "invalid hostname specified: " << h[i] << "\n";
      f = true;
    }
  if (f) exit (1);
}

static rxx host_rxx ("([^:]+)(:([0-9]+))?");

bool
okmgr_clnt_t::add_host (const str &s)
{
  if (!host_rxx.match (s))
    return false;
  str hn = host_rxx[1];
  u_int p;
  if (host_rxx[3]) {
    if (!convertint (host_rxx[3], &p))
      return false;
  } else {
    p = ok_mgr_port;
  }
  if (hn == "-")
    hn = "localhost";
  hosts.push_back (New okmgr_host_t (hn, p));
  return true;
}

void
okmgr_host_t::connect (cbb c)
{
  tcpconnect (hostname, port, wrap (this, &okmgr_host_t::connect_cb, c));
}

void
okmgr_host_t::hostwarn (const str &s, bool nl) const
{
  if (s) {
    warn << hostname << ":" << port << ": " << s;
    if (nl) warn << "\n";
  }
  else {
    warn << "okmgr_host_t::hostwarn called w/ null s (" << hostname
         << ":" << port << ")";
    if (nl) warn << "\n";
  }
}

void
okmgr_host_t::connect_cb (cbb cb, int i)
{
  if (i < 0) {
    hostwarn ("connect failed");
    (*cb) (false);
  }
  fd = i;
  x = axprt_stream::alloc (i);
  c = aclnt::alloc (x, okmgr_program_1);
  (*cb) (true);
}

okmgr_launch_t::okmgr_launch_t (const vec<str> &h, const vec<str> &f,
				ok_set_typ_t t) 
  : okmgr_clnt_t (h), progs (t)
{
  if (t == OK_SET_SOME) {
    progs.progs->setsize (f.size ());
    for (u_int i = 0; i < f.size (); i++)
      (*progs.progs)[i] = f[i];
  }
}

okmgr_logturn_t::okmgr_logturn_t (const vec<str> &h)
  : okmgr_clnt_t (h) {}


okmgr_pub_t::okmgr_pub_t (const vec<str> &h, const vec<str> &f, int v)
  : okmgr_clnt_t (h), version (v)
{
  fns.rebind = true;
  fns.files.setsize (f.size ());
  for (u_int i = 0; i < f.size (); i++)
    fns.files[i] = f[i];
}

void
okmgr_clnt_t::run ()
{
  ncli = hosts.size ();
  if (!ncli) {
    finish (true);
    return;
  }
  for (u_int i = 0; i < hosts.size (); i++)
    hosts[i]->connect (wrap (this, &okmgr_clnt_t::connect_cb, i));
}

void
okmgr_clnt_t::connect_cb (u_int i, bool ok)
{
  if (!ok)
    finish (false);
  ptr<ok_xstatus_t> s = New refcounted<ok_xstatus_t> ();
  do_host (hosts[i], s);
}

void
okmgr_clnt_t::finish (bool rc)
{
  if (!rc)
    err = true;
  if (!--ncli)
    exit (err ? 1 : 0);
}

void
okmgr_pub_t::do_host (okmgr_host_t *h, ptr<ok_xstatus_t> s)
{
  int procno = (version == 2 ? OKMGR_REPUB2 : OKMGR_REPUB);
  h->cli ()->call (procno, &fns, s, 
		   wrap ((okmgr_clnt_t *)this, &okmgr_clnt_t::did_host, 
			 h, s));
}

void
okmgr_logturn_t::do_host (okmgr_host_t *h, ptr<ok_xstatus_t> s)
{
  h->cli ()->call (OKMGR_TURNLOG, NULL, s,
		   wrap ((okmgr_clnt_t *)this, &okmgr_clnt_t::did_host,
			 h, s));
}


void
okmgr_launch_t::do_host (okmgr_host_t *h, ptr<ok_xstatus_t> s)
{
  h->cli ()->call (OKMGR_RELAUNCH, &progs, s,
		   wrap ((okmgr_clnt_t *)this, &okmgr_clnt_t::did_host, 
			 h, s));
}

void
okmgr_clnt_t::did_host (okmgr_host_t *h, ptr<ok_xstatus_t> s, clnt_stat err)
{
  bool rc = false;
  if (err) h->hostwarn (strbuf () << "RPC Error: " << err);
  else if (s->status != OK_STATUS_OK) h->hostwarn (*s->error, false);
  else rc = true;
  finish (rc);
}

int 
main (int argc, char *argv[])
{
  setprogname (argv[0]);
  int ch;
  vec<str> hosts;
  vec<str> files;
  ctl_mode_t m = CTL_MODE_PUB;
  ok_set_typ_t set_typ = OK_SET_SOME;
  int version = 1;
  while ((ch = getopt (argc, argv, "a2lpth:?")) != -1)
    switch (ch) {
    case '2':
      version = 2;
      break;
    case 't':
      m = CTL_MODE_LOGTURN;
      break;
    case 'l':
      m = CTL_MODE_LAUNCH;
      break;
    case 'p':
      m = CTL_MODE_PUB;
      break;
    case 'h':
      hosts.push_back (optarg);
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


  if ((set_typ == OK_SET_ALL) && (m != CTL_MODE_LAUNCH || files.size ()))
    usage ();

  if (!hosts.size ())
    hosts.push_back ("-");

  okmgr_clnt_t *t = NULL;
  switch (m) {
  case CTL_MODE_PUB:
    t = New okmgr_pub_t (hosts, files, version);
    break;
  case CTL_MODE_LAUNCH:
    t = New okmgr_launch_t (hosts, files, set_typ);
    break;
  case CTL_MODE_LOGTURN:
    t = New okmgr_logturn_t (hosts);
    break;
  default:
    usage ();
  }
  if (t) {
    t->run ();
  }
  amain ();
}
