
#include "okld.h"
#include "fd_prot.h"

okld_ch_t::okld_ch_t (const str &e, const str &s, okld_t *o, const str &cfl, 
		      ok_usr_t *u)
  : rexecpath (e), servpath (s), 
    okld (o), cfgfile_loc (cfl), uid (u), state (OKC_STATE_NONE), rcb (NULL),
    have_ustat (false), startup_time (0)
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
okld_ch_t::get_exec_uid ()
{
  assert (have_ustat);
  return ustat.st_uid;
}

bool
okld_ch_t::chown (int uid, int gid)
{
  if (::chown (execpath.cstr (), uid, gid) != 0) {
    warn << cfgfile_loc << ": cannot chown binary: " << execpath << "\n";
    return false;
  }
  warn << "Changing owner of executable " 
       << execpath << "; UID:" << ustat.st_uid 
       << " --> " << uid << "\n";
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
okld_ch_t::chmod (int mode)
{
  if (::chmod (execpath.cstr (), mode) != 0) {
    warn << cfgfile_loc << ": cannot chmod binary: " << execpath << "\n";
    return false;
  }
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

  vec<str> argv;
  execpath = okld->jail2real (rexecpath, true);
  argv.push_back (execpath);

  okld->env.insert ("logfd", logfd, false);
  argv.push_back (okld->env.encode ());
  okld->env.remove ("logfd");
  
  int fd, ctlfd;
  fd = ahttpcon_aspawn (execpath, argv, 
			wrap (this, &okld_ch_t::set_svc_ids), 
			&ctlfd);

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
  if (!okld->is_superuser ())
    return;

  // no need to chroot; okld should have already done that for us

  // unsubscribe from all groups
  setgroups (0, NULL);

  if (setgid (okld->svc_grp.getid ()) != 0) {
    CH_WARN ("could not change gid to " << okld->svc_grp.getname ());
    exit (1);
  }
  if (setuid (uid->getid ()) != 0) {
    CH_WARN ("could not change uid to " << uid->getname ());
    exit (1);
  }
}

void
okld_ch_t::chldcb (int status)
{
  if (okld->in_shutdown ())
    return;

  CH_WARN ("child process died with status=" << status);

  // child chrashing at boot-up is a bad thing; nothing to do here
  if (state != OKC_STATE_SERVE) {
    state = OKC_STATE_HOSED;
    return;
  }
  if (timenow - startup_time < int (ok_chld_startup_time)) {
    CH_WARN ("not restarting due to crash directly after startup");
    state = OKC_STATE_HOSED;
    return;
  }
  state = OKC_STATE_DELAY;

  if (status == 0)
    resurrect ();
  else 
    rcb = delaycb (ok_resurrect_delay, ok_resurrect_delay_ms * 1000000,
		   wrap (this, &okld_ch_t::resurrect));
}

static inline bool
secdiff (struct timeval *tv0, struct timeval *tv1, int diff)
{
  long sd = tv1->tv_sec - tv0->tv_sec;
  return (sd > diff || (sd == diff && tv1->tv_usec > tv0->tv_usec));
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
    launch ();
  }
}

