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

#include "okld.h"
#include "fd_prot.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "okdbg.h"
#include "ok_adebug.h"

extern char ** environ;

okld_ch_t::okld_ch_t (const str &e, const str &s, okld_t *o, const str &cfl, 
		      ok_usr_t *u, vec<str> env_in, u_int16_t p)
  : okld_jailed_exec_t (e, o, cfl),
    servpath (s), uid (u), gid (-1),
    state (OKC_STATE_NONE), rcb (NULL),
    startup_time (0), 
    env (env_in), port (p)
{}

str
okld_jailed_exec_t::get_execpath () const
{
  return okld->jail2real (rexecpath);
}

bool
okld_jailed_exec_t::get_unix_stat ()
{
  if (have_ustat)
    return true;
  if (stat (get_execpath ().cstr (), &ustat) != 0) {
    strbuf b;
    b << cfgfile_loc << ": cannot access service binary: "
      << get_execpath () << "\n";
    okdbg_warn (ERROR, b);
    return false;
  }
  have_ustat = true;
  return true;
}

bool
okld_ch_t::can_exec ()
{
  if (!get_unix_stat ())
    return false;
  if (!uid)
    return true;
  str err = ::can_exec (get_execpath ());
  if (err) {
    warn << cfgfile_loc << ": " << err << " (" << get_execpath () << ")\n";
    return false;
  }
  return true;
}

int
okld_jailed_exec_t::get_exec_mode ()
{
  assert (have_ustat);
  return ustat.st_mode;
}

int
okld_jailed_exec_t::get_exec_uid ()
{
  assert (have_ustat);
  return ustat.st_uid;
}

int 
okld_jailed_exec_t::get_exec_gid ()
{
  assert (have_ustat);
  return ustat.st_gid;
}

void
okld_jailed_exec_t::assign_exec_ownership (int u, int g)
{
  exec_uid = u;
  exec_gid = g;
}

bool
okld_jailed_exec_t::chown ()
{
  assert (exec_uid >= 0 && exec_gid >= 0);
  if (::chown (get_execpath ().cstr (), exec_uid, exec_gid) != 0) {
    strbuf b;
    b << cfgfile_loc << ": cannot chown binary: " 
      << get_execpath () << "\n";
    okdbg_warn (FATAL_ERROR, b);
    return false;
  }
  strbuf b;
  b << "Changing owner of executable " 
    << get_execpath () << "; UID/GID:" << ustat.st_uid << "/" 
    << ustat.st_gid << " --> " << exec_uid << "/" << exec_gid <<  "\n";
  okdbg_warn (CHATTER, b);
  return true;
}

void
okld_ch_t::assign_uid (int new_uid)
{
  if (uid)
    assert (uid->getid () == new_uid);
  else
    uid = New ok_usr_t (new_uid);
}


bool
okld_jailed_exec_t::chmod (int currmode)
{
  if ((currmode & 07777) == mode)
    return true;

  assert (mode >= 0);
  if (::chmod (get_execpath ().cstr (), mode) != 0) {
    strbuf b;
    b << cfgfile_loc << ": cannot chmod binary: " << get_execpath () << "\n";
    okdbg_warn (FATAL_ERROR, b);
    return false;
  }
  strbuf b ("Changing mode of executable %s; MODE: 0%o --> 0%o\n",
	    get_execpath ().cstr (), ustat.st_mode & 07777, mode);
  okdbg_warn (CHATTER, b);
  return true;
}


tamed void 
okld_ch_t::launch ()
{
  tvars {
    int logfd;
    int pub2fd (-1);
    clone_only_client_t *p;
  }

  if (startup_time == 0)
    startup_time = sfs_get_timenow();
  state = OKC_STATE_LAUNCH;
  p = okld->get_pubd2 ();

  twait { 
    okld->get_logd ()->clone (mkevent (logfd));
    if (p) p->clone (mkevent (pub2fd));
  }

  if (logfd < 0) {
    strbuf b;
    b << "HOSED: Cannot connect to oklogd for server: (" << servpath << ","
      << rexecpath << ")\n";
    okdbg_warn (ERROR, b);
    state = OKC_STATE_HOSED;
    return; 
  }

  if (p && pub2fd < 0) {
    strbuf b;
    b << "HOSED: Cannot connect to pubd -2 for server: (" << servpath << ","
      << rexecpath << ")\n";
    okdbg_warn (ERROR, b);
    state = OKC_STATE_HOSED;
    return;
  }

  if (okld->in_shutdown ()) {
    close (logfd);
    if (pub2fd >= 0) close (pub2fd);
    strbuf b;
    b << "HOSED: shutdown received while relaunching service: ("
      << servpath << "," << rexecpath << ")\n";
    okdbg_warn (ERROR, b);
    state = OKC_STATE_HOSED;
    return;
  }

  vec<str> argv;

  // the first argument to aspawn ()...
  str exe = get_execpath ();

  // only do something in the case of a script; true => we are now
  // chrooted
  str i = get_interpreter ();
  if (i) {
    exe = i;
    argv.push_back (i);
  }

  // pass true signifying that we've been jailed (if we are to be jailed)
  argv.push_back (get_execpath ());

  okld->env.insert ("logfd", logfd, false);
  okld->env.insert ("pub2fd", pub2fd, false);

  // for Service2-style command lands, we can have many arguments
  // here specified in okws_config.  make sure to add the encoded
  // parameters last
  for (u_int i = 0; i < args.size (); i++)
    argv.push_back (args[i]);

  // per-service options might override global options here
  _svc_options.apply_global_defaults ();

  okld->env.insert ("lifereqs", _svc_options.svc_reqs, false);
  okld->env.insert ("lifetime", _svc_options.svc_time, false);
  okld->env.insert ("wss", _svc_options.wss, false);
  okld->env.insert ("caching", _svc_options.pub2_caching, false);
  okld->env.insert ("wfs", _svc_options.wait_for_signal, false);

  argv.push_back (okld->env.encode ());

  // undo all of the damage
  okld->env.remove ("logfd");
  okld->env.remove ("pub2fd");
  okld->env.remove ("lifetime");
  okld->env.remove ("lifereqs");
  okld->env.remove ("wss");
  okld->env.remove ("caching");
  okld->env.remove ("wfs");

  argv_t env_tmp;
  env_tmp.init (env, environ);

  ptr<axprt_unix> x = 
    axprt_unix_aspawnv (exe, argv, ok_axprt_ps,
			wrap (this, &okld_ch_t::set_svc_ids), 
			env_tmp);

  pid = axprt_unix_spawn_pid;
  close (logfd);
  if (pub2fd >= 0) close (pub2fd);

  if (!x) {
    warn << "HOSED: Cannot launch service: (" << servpath << "," 
	 << rexecpath << ")\n";
    state = OKC_STATE_HOSED;
    return;
  }

  ::chldcb (pid, wrap (this, &okld_ch_t::chldcb));

  int ctlfd = x->reclaim ();

  okws_fd_t fdx (OKWS_SVC_CTL_X);

  fdx.ctlx->pid = pid;
  fdx.ctlx->name = servpath;

  // if we're shutting down, okld->okdx will be NULL
  if (!okld->okdx || !okld->okdx->send (ctlfd, fdx)) {
    close (ctlfd);
    warn << "HOSED: Cannot clone CTL file descriptor: (" << servpath << ")\n";
    state = OKC_STATE_HOSED;
    return;
  }

  state = OKC_STATE_SERVE;
  return;
}

void
okld_ch_t::set_svc_ids ()
{
  // after fork, we need to set our PID
  pid = getpid ();

  if (!okld->is_superuser ())
    return;

  // unsubscribe from all groups
  setgroups (0, NULL);

  if (setgid (gid) != 0) {
    CH_ERROR ("could not change gid to " << gid);
    exit (1);
  }

  if (setuid (uid->getid ()) != 0) {
    CH_ERROR ("could not change uid to " << uid->getname ());
    exit (1);
  }

  if (rundir) {
    str d = okld->jail2real (rundir, true);
    if (chdir (d) != 0)
      CH_ERROR ("change dir failed: %m\n");
    else {
      if (OKDBG2(OKD_STARTUP))
	CH_CHATTER ("changing directories to " << d  );
    }
  }
}

void
okld_ch_t::chldcb (int status)
{
  if (status != 0)
    clean_dumps ();
  if (okld->in_shutdown ())
    return;

  CH_ERROR ("child process died with status=" << status);

  // child chrashing at boot-up is a bad thing; nothing to do here
  if (state != OKC_STATE_SERVE) {
    CH_ERROR ("HOSED: child found in wrong state (" << state << ")");
    state = OKC_STATE_HOSED;
    return;
  }
  if (okld->safe_startup () && 
      (sfs_get_timenow() - startup_time < int (ok_chld_startup_time))) {
    CH_ERROR ("HOSED: not restarting due to crash directly after startup");
    state = OKC_STATE_HOSED;
    return;
  }
  state = OKC_STATE_DELAY;

  if (status == 0 && timevals.size () <= (ok_crashes_max/2 + 2) )
    resurrect ();
  else {
    rcb = delaycb (ok_resurrect_delay, ok_resurrect_delay_ms * 1000000,
		   wrap (this, &okld_ch_t::resurrect));
  }
}

static inline bool
secdiff (struct timeval *tv0, struct timeval *tv1, int diff)
{
  long sd = tv1->tv_sec - tv0->tv_sec;
  return (sd > diff || (sd == diff && tv1->tv_usec > tv0->tv_usec));
}

bool
okld_jailed_exec_t::fix_exec ()
{
  // force a redo in the case of resurrecting a new service!
  have_ustat = false;
  if (!get_unix_stat ())
    return false;
  if ((get_exec_uid () != exec_uid || get_exec_gid () != exec_gid) && 
      !chown ())
    return false;
  if (!chmod (get_exec_mode ()))
    return false;
  return true;
}

void
okld_ch_t::resurrect ()
{
  rcb = NULL;
  if (state == OKC_STATE_LAUNCH)
    return;

  struct timeval *tp = (struct timeval *) xmalloc (sizeof (struct timeval));
  gettimeofday (tp, NULL);

  while (timevals.size () && secdiff (timevals[0], tp, ok_csi))
    xfree (timevals.pop_front ());
  timevals.push_back (tp);
  if (timevals.size () > ok_crashes_max) {
    state = OKC_STATE_HOSED;
    CH_ERROR ("HOSED: execeeded maximum number of crashes; "
	      "will no longer restart!");
  } else {
    if (okld->will_jail () && !fix_exec ()) {
      CH_ERROR ("HOSED: failed to fix permissions on executable "
		"during relaunch");
      state = OKC_STATE_HOSED;
    } else {
      launch ();
    }
  }
}


void
okld_ch_t::clean_dumps ()
{
  if (!rundir)
    return;
  DIR *dirp = opendir (rundir.cstr ());
  if (!dirp) {
    warn << "Cannot clean out coredump directory: " << rundir << "\n";
    return;
  }
  struct dirent *de;
  while ((de = readdir (dirp))) {
    if (de->d_type != DT_REG)
      continue;
    str corefile = de->d_name;
    str from = apply_container_dir (rundir, corefile);
    str to = apply_container_dir (okld->get_root_coredir (), 
				  strbuf (corefile) << "." 
				  << sfs_get_timenow());
    if (link (from.cstr (), to.cstr ()) != 0) 
      warn ("could not move file %s: %m\n", from.cstr ());
    else if (unlink (from.cstr ()) != 0)
      warn ("unlink failed for file %s: %m\n", from.cstr ());
    if (::chown (to.cstr (), 0, 0) != 0)
      warn ("chown failed for file %s: %m\n", to.cstr ());
    if (::chmod (to.cstr (), 0400) != 0)
      warn ("chmod failed for file %s: %m\n", to.cstr ());
  }
  if (closedir (dirp) != 0) {
    warn ("close directory failed for %s: %m\n", rundir.cstr ());
  }
}

bool
okld_jailed_exec_t::fixup (int uid_new, int gid_new, int new_mode)
{
  assign_exec_ownership (uid_new, gid_new);
  assign_mode (new_mode);

  // XXX might issue an additional stat() system call, but doesn't
  // really matter....
  return fix_exec ();
}

bool
okld_ch_t::fixup_doall (int uid_orig, int uid_new, int gid_orig,
			int gid_new, int mode_orig)
{
  if (!fixup (uid_new, gid_new, get_desired_execfile_mode ()))
    return false;

  // DO NOT run as the owner of the file!! that's the whole
  // point of this!
  assign_uid (gid_new);
  assign_gid (gid_new);

  return true;
}


