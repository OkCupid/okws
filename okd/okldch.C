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

okld_ch_t::okld_ch_t (const str &e, const str &s, okld_t *o, const str &cfl, 
		      ok_usr_t *u, char *const *env_in)
  : rexecpath (e), servpath (s), okld (o), cfgfile_loc (cfl), uid (u), 
  state (OKC_STATE_NONE), rcb (NULL),
  have_ustat (false), startup_time (0), 
  exec_uid (-1), exec_gid (-1), mode (-1),
  env (env_in)
{
  o->insert (this);
}

bool
okld_ch_t::get_unix_stat (bool chrted)
{
  if (have_ustat)
    return false;
  execpath = okld->jail2real (rexecpath, chrted);
  if (stat (execpath.cstr (), &ustat) != 0) {
    warn << cfgfile_loc << ": cannot access service binary: "
	 << execpath << "\n";
    return false;
  }
  have_ustat = true;
  return true;
}

bool
okld_ch_t::can_exec (bool chrted)
{
  if (!get_unix_stat (chrted))
    return false;
  if (!uid)
    return true;
  str err = ::can_exec (execpath);
  if (err) {
    warn << cfgfile_loc << ": " << err << " (" << execpath << ")\n";
    return false;
  }
  return true;
}

int
okld_ch_t::get_exec_mode ()
{
  assert (have_ustat);
  return ustat.st_mode;
}

int
okld_ch_t::get_exec_uid ()
{
  assert (have_ustat);
  return ustat.st_uid;
}

int 
okld_ch_t::get_exec_gid ()
{
  assert (have_ustat);
  return ustat.st_gid;
}

void
okld_ch_t::assign_exec_ownership (int u, int g)
{
  exec_uid = u;
  exec_gid = g;
}

bool
okld_ch_t::chown ()
{
  assert (exec_uid >= 0 && exec_gid >= 0);
  if (::chown (execpath.cstr (), exec_uid, exec_gid) != 0) {
    warn << cfgfile_loc << ": cannot chown binary: " << execpath << "\n";
    return false;
  }
  warn << "Changing owner of executable " 
       << execpath << "; UID/GID:" << ustat.st_uid << "/" 
       << ustat.st_gid << " --> " << exec_uid << "/" << exec_gid <<  "\n";
  return true;
}

void
okld_ch_t::assign_uid (int new_uid)
{
  assert (!uid);
  uid = New ok_usr_t (new_uid);
}

void
okld_ch_t::launch ()
{
  if (startup_time == 0)
    startup_time = timenow;
  state = OKC_STATE_LAUNCH;
  okld->get_logd ()->clone (wrap (this, &okld_ch_t::launch_cb));
}

bool
okld_ch_t::chmod (int currmode)
{
  if ((currmode & 07777) == mode)
    return true;

  assert (mode >= 0);
  if (::chmod (execpath.cstr (), mode) != 0) {
    warn << cfgfile_loc << ": cannot chmod binary: " << execpath << "\n";
    return false;
  }
  warn ("Changing mode of executable %s; MODE: 0%o --> 0%o\n",
	execpath.cstr (), ustat.st_mode & 07777, mode);
  return true;
}

void
okld_ch_t::launch_cb (int logfd)
{
  if (logfd < 0) {
    warn << "Cannot connect to oklogd for server: (" << servpath << ","
	 << rexecpath << ")\n";
    state = OKC_STATE_HOSED;
    return; 
  }

  if (okld->in_shutdown ()) {
    close (logfd);
    warn << "shutdown received while relaunching service: ("
	 << servpath << "," << rexecpath << ")\n";
    state = OKC_STATE_HOSED;
    return;
  }

  vec<str> argv;
  execpath = okld->jail2real (rexecpath, true);
  argv.push_back (execpath);

  okld->env.insert ("logfd", logfd, false);
  argv.push_back (okld->env.encode ());
  okld->env.remove ("logfd");

  int fd, ctlfd;
  fd = ahttpcon_aspawn (execpath, argv, 
			wrap (this, &okld_ch_t::set_svc_ids), 
			&ctlfd, env);

  pid = ahttpcon_spawn_pid;
  close (logfd);
  if (fd < 0 || ctlfd < 0) {
    warn << "Cannot launch service: (" << servpath << "," 
	 << rexecpath << ")\n";
    state = OKC_STATE_HOSED;
    return;
  }

  ::chldcb (pid, wrap (this, &okld_ch_t::chldcb));

  okws_fd_t fdx (OKWS_SVC_CTL_X);

  fdx.ctlx->pid = pid;
  fdx.ctlx->name = servpath;
  if (!okld->okdx->send (ctlfd, fdx)) {
    close (ctlfd);
    close (fd);
    warn << "Cannot clone CTL file descriptor: (" << servpath << ")\n";
    state = OKC_STATE_HOSED;
    return;
  }

  fdx.set_fdtyp (OKWS_SVC_X);
  fdx.x->pid = pid;
  fdx.x->name = servpath;
  if (!okld->okdx->send (fd, fdx)) {
    close (ctlfd);
    close (fd);
    state = OKC_STATE_HOSED;
    warn << "Cannot clone HTTP file descriptor: (" << servpath << ")\n";
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
    CH_WARN ("could not change gid to " << gid);
    exit (1);
  }

  if (setuid (uid->getid ()) != 0) {
    CH_WARN ("could not change uid to " << uid->getname ());
    exit (1);
  }

  if (rundir) {
    str d = okld->jail2real (rundir, true);
    if (chdir (d) != 0)
      warn ("change dir failed: %m\n");
    else {
      CH_WARN (  "changing directories to " << d  );
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

  CH_WARN ("child process died with status=" << status);

  // child chrashing at boot-up is a bad thing; nothing to do here
  if (state != OKC_STATE_SERVE) {
    state = OKC_STATE_HOSED;
    return;
  }
  if (okld->safe_startup () && 
      (timenow - startup_time < int (ok_chld_startup_time))) {
    CH_WARN ("not restarting due to crash directly after startup");
    state = OKC_STATE_HOSED;
    return;
  }
  state = OKC_STATE_DELAY;

  if (status == 0)
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
okld_ch_t::fix_exec (bool jail)
{
  have_ustat = false;
  if (!get_unix_stat (jail))
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
  } else {
    if (okld->will_jail () && !fix_exec (true)) {
      CH_WARN ("failed to fix permissions on executable during relaunch");
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
				  strbuf (corefile) << "." << timenow);
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

