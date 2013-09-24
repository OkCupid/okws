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
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "okdbg.h"
#include "ok_adebug.h"
#include "okprot.h"

#include <limits.h>
#include <stdlib.h>

extern char ** environ;

//-----------------------------------------------------------------------

okld_ch_t::okld_ch_t (okld_ch_cluster_t *c, size_t i)
  : _cluster (c), 
    _id (i), 
    _brother_id (i), // XXX need this too for CH_ERROR in ok.h
    _servpath (c->servpath ()),
    _pid (-1),
    _state (OKC_STATE_NONE),
    _rcb (NULL),
    _nsent (0),
    _lazy_lock (tame::lock_t::OPEN),
    _startup_time (0) {}

//-----------------------------------------------------------------------

okld_ch_cluster_t::okld_ch_cluster_t (const str &e, const str &s, okld_t *o, 
				      const str &cfl, ok_usr_t *u, 
				      vec<str> env_in, u_int16_t p, 
				      size_t n) 
  : okld_jailed_exec_t (e, o, cfl),
    _servpath (s), 
    _port (p),
    _uid (u), 
    _gid (-1),
    _env (env_in) 
{
  // Set up all n children
  for (size_t i = 0; i < n; i++) {
    ptr<okld_ch_t> ch = New refcounted<okld_ch_t> (this, i);
    _children.push_back (ch);
  }
}

//-----------------------------------------------------------------------

okld_ch_cluster_t::~okld_ch_cluster_t () {}

//-----------------------------------------------------------------------

str
okld_jailed_exec_t::get_execpath () const
{
  return okld ()->jail2real (_rexecpath);
}

//-----------------------------------------------------------------------

bool
okld_jailed_exec_t::get_unix_stat ()
{
  if (_have_ustat)
    return true;
  if (stat (get_execpath ().cstr (), &_ustat) != 0) {
    strbuf b;
    b << _cfgfile_loc << ": cannot access service binary: "
      << get_execpath () << "\n";
    okdbg_warn (ERROR, b);
    return false;
  }
  _have_ustat = true;
  return true;
}

//-----------------------------------------------------------------------

bool
okld_ch_cluster_t::can_exec ()
{
  if (!get_unix_stat ())
    return false;
  if (!_uid)
    return true;
  str err = ::can_exec (get_execpath ());
  if (err) {
    warn << _cfgfile_loc << ": " << err << " (" << get_execpath () << ")\n";
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------

int
okld_jailed_exec_t::get_exec_mode ()
{
  assert (_have_ustat);
  return _ustat.st_mode;
}

//-----------------------------------------------------------------------

int
okld_jailed_exec_t::get_exec_uid ()
{
  assert (_have_ustat);
  return _ustat.st_uid;
}

//-----------------------------------------------------------------------

int 
okld_jailed_exec_t::get_exec_gid ()
{
  assert (_have_ustat);
  return _ustat.st_gid;
}

//-----------------------------------------------------------------------

void
okld_jailed_exec_t::assign_exec_ownership (int u, int g)
{
  _exec_uid = u;
  _exec_gid = g;
}

//-----------------------------------------------------------------------

bool
okld_jailed_exec_t::chown ()
{
  assert (_exec_uid >= 0 && _exec_gid >= 0);
  if (::chown (get_execpath ().cstr (), _exec_uid, _exec_gid) != 0) {
    strbuf b;
    b << _cfgfile_loc << ": cannot chown binary: " 
      << get_execpath () << "\n";
    okdbg_warn (FATAL_ERROR, b);
    return false;
  }
  strbuf b;
  b << "Changing owner of executable " 
    << get_execpath () << "; UID/GID:" << _ustat.st_uid << "/" 
    << _ustat.st_gid << " --> " << _exec_uid << "/" << _exec_gid <<  "\n";
  okdbg_warn (CHATTER, b);
  return true;
}

//-----------------------------------------------------------------------

void
okld_ch_cluster_t::assign_uid (int new_uid)
{
  if (_uid)
    assert (_uid->getid () == new_uid);
  else
    _uid = New ok_usr_t (new_uid);
}

//-----------------------------------------------------------------------


bool
okld_jailed_exec_t::chmod (int currmode)
{
  if ((currmode & 07777) == _mode)
    return true;

  assert (_mode >= 0);
  if (::chmod (get_execpath ().cstr (), _mode) != 0) {
    strbuf b;
    b << _cfgfile_loc << ": cannot chmod binary: " << get_execpath () << "\n";
    okdbg_warn (FATAL_ERROR, b);
    return false;
  }
  strbuf b ("Changing mode of executable %s; MODE: 0%o --> 0%o\n",
	    get_execpath ().cstr (), _ustat.st_mode & 07777, _mode);
  str s = b;
  okdbg_warn (CHATTER, s);
  return true;
}

//-----------------------------------------------------------------------

tamed void
okld_ch_cluster_t::launch (evv_t ev)
{
  tvars { size_t i; }
  twait { 
    for (i = 0; i < _children.size (); i++) {
      _children[i]->launch (true, mkevent ()); 
    }
  }
  ev->trigger ();
}

//-----------------------------------------------------------------------

ok_xstatus_typ_t
okld_ch_t::kill (int sig)
{
  ok_xstatus_typ_t ret = OK_STATUS_OK;
  int rc = ::kill (_pid, sig);
  strbuf b;
  b << "issuing kill with signal=" << sig << " => ";
  if (rc == 0) {
    b << "success";
  } else {
    b.fmt (" failed (%m)");
    switch (errno) {
    case EINVAL: ret = OK_STATUS_UNKNOWN_OPTION; break;
    case ESRCH: ret = OK_STATUS_NOSUCHCHILD; break;
    case EPERM: ret = OK_STATUS_BADFD; break;
    default: ret = OK_STATUS_ERR;
    }
  }
  str s = b;
  CH_ERROR(s);
  return ret;
}

//-----------------------------------------------------------------------


str
okld_ch_t::str_id () const
{
  str ret;
  if (_cluster->n_children () > 1) {
    ret = strbuf ("%s:%zd", _servpath.cstr (), _id);
  } else {
    ret = _servpath;
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<axprt_unix>
okld_ch_cluster_t::spawn_proc (okld_ch_t *ch, int logfd, int pubfd) 
{
  str report_exe;
  str exe;
  str service_exe;
  str i;
  vec<str> argv;
  ptr<axprt_unix> x;
  argv_t env_tmp;
  cgi_t *e;
  int pid;

  str s_id = ch->str_id ();
  size_t bid = ch->id ();

  // the first argument to aspawn ()...
  report_exe = service_exe = exe = get_execpath ();

  if (okld()->get_config_resolve_bins()) {
      char real_exe[PATH_MAX];
      if (realpath(exe.cstr(), real_exe)) {
          if (exe != real_exe) {
               OKDBG4(OKD_STARTUP, CHATTER, "Resolved executable [%s] to [%s]\n",
                       exe.cstr(), real_exe);
          }

          exe = real_exe;
      }
      else {
          okdbg_warn (ERROR, "Can't get real path for binary [%s]: %m\n", exe.cstr());
      }
  }
  
  if ((i = get_interpreter ())) {
    exe = i;
    argv.push_back (i);
  }
      
  argv.push_back (service_exe);

  e = okld ()->env ();
      
  e->insert ("logfd", logfd, false);
  e->insert ("pubfd", pubfd, false);
  
  // for Service2-style command lands, we can have many arguments
  // here specified in okws_config.  make sure to add the encoded
  // parameters last
  for (size_t i = 0; i < _args.size (); i++) {
    argv.push_back (_args[i]);
  }
      
  // per-service options might override global options here
  if (!_svc_options.apply_global_defaults (_servpath)) {
    okdbg_warn (ERROR, "%s: cannot apply conf params\n", s_id.cstr ());
  } else {
    
    e->insert ("lifereqs", _svc_options.svc_reqs, false);
    e->insert ("lifetime", _svc_options.svc_time, false);
    e->insert ("wss", _svc_options.wss, false);
    e->insert ("caching", _svc_options.pub3_caching, false);
    e->insert ("wfs", _svc_options.wait_for_signal, false);
    e->insert ("dps", ch->direct_ports().encode_as_str (), false);
    e->insert ("fdlw", _svc_options.lowat, false);
    e->insert ("fdhw", _svc_options.hiwat, false);
    e->insert ("gzip", _svc_options.gzip, false);
    e->insert ("gziplev", _svc_options.gzip_level, false);
    e->insert ("aczw", _svc_options.ahttpcon_zombie_warn, false);
    e->insert ("aczt", _svc_options.ahttpcon_zombie_timeout, false);
    e->insert ("bid", bid, false);
    e->insert ("nch", n_children (), false);
    e->insert ("rcyclimitint", ok_pub3_recycle_limit_int, false);
    e->insert ("rcyclimitbt", ok_pub3_recycle_limit_bindtab, false);
    e->insert ("rcyclimitdict", ok_pub3_recycle_limit_dict, false);
    e->insert ("rcyclimitslot", ok_pub3_recycle_limit_slot, false);
    
    argv.push_back (e->encode ());
    
    // undo all of the damage
    e->remove ("logfd");
    e->remove ("pubfd");
    e->remove ("lifetime");
    e->remove ("lifereqs");
    e->remove ("wss");
    e->remove ("caching");
    e->remove ("wfs");
    e->remove ("dps");
    e->remove ("fdlw");
    e->remove ("fdhw");
    e->remove ("gzip");
    e->remove ("gziplev");
    e->remove ("aczw");
    e->remove ("bid");
    e->remove ("nch");
    e->remove ("rcyclimitint");
    e->remove ("rcyclimitbt");
    e->remove ("rcyclimitdict");
    e->remove ("rcyclimitslot");
      
    OKDBG4(OKD_STARTUP, CHATTER, "calling launch on exe='%s'", 
	   report_exe.cstr ());
    
    env_tmp.init (_env, environ);
    x = axprt_unix_aspawnv (exe, argv, ok_axprt_ps,
			    wrap (ch, &okld_ch_t::post_fork_cb),
			    env_tmp);
    pid = axprt_unix_spawn_pid;
    ch->set_pid (pid);
    if (!x) {
      okdbg_warn (ERROR, "%s: cannot launch service\n", s_id.cstr ());
    } else {
      OKDBG4(OKD_STARTUP, CHATTER, "launch succeeded on exe='%s'; pid=%d", 
	     report_exe.cstr (), pid);
    }
  }
  return x;
}

//-----------------------------------------------------------------------

tamed void
okld_ch_t::bind_ports (bool retry, evb_t ev)
{
  tvars {
    bool go (true);
    vec<time_t> sched;
    size_t i (0);
    time_t delay (0);
    bool ret (false);
    str s_id;
  }
  sched = okld ()->bind_reattempt_schedule ();
  s_id = str_id ();
  
  while (!ret && go) {
    
    if (!_direct_ports.bind (_servpath, okld ()->listenaddr)) {
      okdbg_warn (ERROR, "%s: cannot bind to all direct ports\n", 
		  s_id.cstr ());

      if (!retry) { 
	// We're not allowed to retry, so don't bother
	go = false; 
      } else if (i == sched.size () && i > 0 && delay) {
	// We've gone off the end of the schedule; keep retrying 
	// indefinitely...
      } else if (i < sched.size ()) {
	delay = sched[i++];
      } else {
	// We're out of options?  Ok, then kill it....
	go = false;
      }

      if (go) {
	okdbg_warn (ERROR, "%s: wait %ds for reattempt\n",
		    s_id.cstr (), int (delay));
	twait { delaycb (delay, 0, mkevent ()); }
      }

    } else {
      ret = true;
    }

  }
  ev->trigger (ret);
}

//-----------------------------------------------------------------------


tamed void 
okld_ch_t::launch (bool startup, evv_t ev)
{
  tvars {
    bool ok (true);
    str s_id;
    ptr<axprt_unix> x;
    int logfd (-1), pubfd (-1);
  }


  s_id = str_id ();

  if (_startup_time == 0) _startup_time = sfs_get_timenow();
  _state = OKC_STATE_LAUNCH;

  twait { okld ()->gather_helper_fds (s_id, &logfd, &pubfd, mkevent (ok)); }
  if (!ok) {
    _state = OKC_STATE_HOSED;
  }
  
  if (ok) { 
    twait { bind_ports (true, mkevent (ok)); } 
    if (!ok) {
      _state = OKC_STATE_BADPORTS; 
    }
  }

  if (ok && !(x = _cluster->spawn_proc (this, logfd, pubfd))) {
    _state = OKC_STATE_HOSED; 
    ok = false;
  }

  if (ok) {
    assert (x);
    twait { post_spawn (x->reclaim (), mkevent (ok)); }
    if (!ok) {
      _state = OKC_STATE_HOSED;
    }
  }

  if (ok) { _state = OKC_STATE_SERVE; }

  if (logfd >= 0) close (logfd);
  if (pubfd >= 0) close (pubfd);
  _direct_ports.close ();


  ev->trigger ();
}

//-----------------------------------------------------------------------

tamed void
okld_ch_t::post_spawn (int fd, evb_t ev)
{
  tvars {
    clnt_stat err;
    ok_xstatus_typ_t res;
    oksvc_descriptor_t arg;
    bool ok (false);
    ptr<aclnt> cli;
    ptr<axprt_unix> x;
    str s_id;
  }
  
  s_id = str_id ();

  ::chldcb (_pid, wrap (this, &okld_ch_t::chldcb));

  cli = okld ()->okd ().cli ();
  x = okld ()->okd ().x ();
    
  // if we're shutting down, okld->_okd_cli will be NULL
  if (!cli) {
    okdbg_warn (ERROR, "%s: cannot send to okd due to EOF\n", s_id.cstr ());
  } else {
   
    arg.pid = _pid;
    arg.proc.brother_id = _id;
    arg.proc.num_brothers = _cluster->n_children ();
    arg.proc.name = _servpath;

    x->sendfd (fd);

    twait {
      RPC::okld_program_1::okld_new_service (cli, arg, &res, mkevent (err));
    }
    
    if (err) {
      strbuf b;
      b << s_id << ": cannot cannot send service to okd: " << err;
      okdbg_warn (ERROR, b);
    } else if (res != OK_STATUS_OK) {
      okdbg_warn (ERROR, "%s: okd rejected service with code=%d\n",
		  s_id.cstr (), int (res));
    } else {
      ok = true;
    }
  }
  ev->trigger (ok);
}

//-----------------------------------------------------------------------

void
okld_ch_t::post_fork_cb ()
{
  // close any ports that we were instructed to close earlier
  direct_ports ().do_close_ports (str_id ());

  // after fork, we need to set our PID
  _pid = getpid ();

  // If the parent okld process is running with root privileges,
  // strip privileges from the child proc, but do the same for each
  // in the "cluster" of sibling processes.
  if (okld ()->is_superuser ()) {
    _cluster->set_svc_ids (_pid);
  }
}

//-----------------------------------------------------------------------

void
okld_ch_cluster_t::set_svc_ids (int _pid)
{

  // unsubscribe from all groups
  setgroups (0, NULL);

  if (setgid (_gid) != 0) {
    CH_CL_ERROR ("could not change gid to " << _gid);
    exit (1);
  }

  if (setuid (_uid->getid ()) != 0) {
    CH_CL_ERROR ("could not change uid to " << _uid->getname ());
    exit (1);
  }

  if (_rundir) {
    str d = okld ()->jail2real (_rundir, true);
    if (chdir (d.cstr()) != 0) {
      CH_CL_ERROR ("change dir failed: %m\n");
    } else if (OKDBG2(OKD_STARTUP)) {
      CH_CL_CHATTER ("changing directories to " << d  );
    }
  } else if (OKDBG2(OKD_STARTUP)) {
    CH_CL_CHATTER ("no rundir specified; not changing directories\n");
  }
}

//-----------------------------------------------------------------------

tamed void
okld_ch_t::chldcb_T (int status)
{
  tvars {
    bool relaunch (false);
  }

  if (status != 0) { _cluster->clean_dumps (); }

  if (!okld ()->in_shutdown ()) {

    CH_ERROR ("child process died with status=" << status);

    // child chrashing at boot-up is a bad thing; nothing to do here
    if (_state != OKC_STATE_SERVE) {
      CH_ERROR ("Child found in wrong state (" << _state << ")");
      _state = OKC_STATE_HOSED;

    } else if (okld ()->safe_startup () && 
	       (sfs_get_timenow() - _startup_time < 
		int (ok_chld_startup_time))) {
      CH_ERROR ("Not restarting due to crash directly after startup");
      _state = OKC_STATE_HOSED;

    } else if (status != 0 || _timevals.size () > (ok_crashes_max/2 + 2) ) {
      _state = OKC_STATE_DELAY;
      relaunch = true;
      twait { 
	delaycb (ok_resurrect_delay, ok_resurrect_delay_ms * 1000000, 
		 mkevent ()); 
      }
    } else {
      relaunch = true;
    }
  }

  if (!okld ()->in_shutdown () && relaunch) {
    twait { resurrect (mkevent ()); }
  }
}

//-----------------------------------------------------------------------

static inline bool
secdiff (struct timespec &tv0, struct timespec &tv1, int diff)
{
  int sd = tv1.tv_sec - tv0.tv_sec;
  return (sd > diff || (sd == diff && tv1.tv_nsec > tv0.tv_nsec));
}

//-----------------------------------------------------------------------

bool
okld_jailed_exec_t::fix_exec ()
{
  // force a redo in the case of resurrecting a new service!
  _have_ustat = false;
  if (_unsafe) 
    return true;
  if (!get_unix_stat ())
    return false;
  if ((get_exec_uid () != _exec_uid || get_exec_gid () != _exec_gid) && 
      !chown ())
    return false;
  if (!chmod (get_exec_mode ()))
    return false;
  return true;
}

//-----------------------------------------------------------------------

tamed void
okld_ch_t::resurrect (evv_t ev)
{
  tvars {
    struct timespec ts;
  }
  if (_state != OKC_STATE_LAUNCH) {

    ts = sfs_get_tsnow ();
    while (_timevals.size () && secdiff (_timevals[0], ts, ok_csi)) {
      _timevals.pop_front ();
    }
    _timevals.push_back (ts);

    if (_timevals.size () > ok_crashes_max) {
      _state = OKC_STATE_HOSED;
      CH_ERROR ("Execeeded maximum number of crashes; will no longer restart!");

    } else if (okld ()->will_jail () && !_cluster->fix_exec ()) {
      _state = OKC_STATE_HOSED;
      CH_ERROR ("Failed to fix permissions on executable during relaunch!");
    } else {
      twait { launch (false, mkevent ()); }
    }
  }
  ev->trigger ();
}

//-----------------------------------------------------------------------

void
okld_ch_cluster_t::clean_dumps ()
{
  DIR *dirp;
  if (!_rundir) { /* noop */
  } else if (!(dirp = opendir (_rundir.cstr ()))) {
    warn << "Cannot clean out coredump directory: " << _rundir << "\n";
  } else { 
    struct dirent *de;
    while ((de = readdir (dirp))) {
      if (de->d_type != DT_REG)
	continue;
      str corefile = de->d_name;
      str from = apply_container_dir (_rundir, corefile);
      str to = apply_container_dir (okld ()->get_root_coredir (), 
				    strbuf (corefile) << "." 
				    << sfs_get_timenow());
      if (link (from.cstr (), to.cstr ()) != 0) 
	warn ("could not move file %s: %m\n", from.cstr ());
      else if (unlink (from.cstr ()) != 0)
	warn ("unlink failed for file %s: %m\n", from.cstr ());
      
      if (::chown (to.cstr (), 
		   okld ()->coredump_usr ().getid (), 
		   okld ()->coredump_grp ().getid ()) != 0)
	warn ("chown failed for file %s: %m\n", to.cstr ());
      
      if (::chmod (to.cstr (), okld ()->coredump_mode ()) != 0)
	warn ("chmod failed for file %s: %m\n", to.cstr ());
      
    }
    if (closedir (dirp) != 0) {
      warn ("close directory failed for %s: %m\n", _rundir.cstr ());
    }
  }
}

//-----------------------------------------------------------------------

bool
okld_jailed_exec_t::fixup (int uid_new, int gid_new, int new_mode)
{
  assign_exec_ownership (uid_new, gid_new);
  assign_mode (new_mode);

  // XXX might issue an additional stat() system call, but doesn't
  // really matter....
  return fix_exec ();
}

//-----------------------------------------------------------------------

bool
okld_ch_cluster_t::fixup_doall (int uid_orig, int uid_new, int gid_orig,
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

//-----------------------------------------------------------------------

tamed void
okld_ch_t::lazy_startup (evb_t ev)
{
  tvars {
    bool ret (true);
  }
  twait { _lazy_lock.acquire (tame::lock_t::EXCLUSIVE, mkevent ()); }

  if (_state == OKC_STATE_SERVE) {
    ret = true;
  } else if (_state == OKC_STATE_STANDBY) {
    twait { launch (true, mkevent ());  }
    ret = (_state == OKC_STATE_SERVE);
  } else {
    ret = false;
  }

  _lazy_lock.release ();

  ev->trigger (ret);
}

//-----------------------------------------------------------------------

tamed void
okld_ch_cluster_t::reserve (bool lazy, evb_t ev)
{
  tvars {
    ptr<aclnt> cli;
    clnt_stat err;
    oksvc_reserve_arg_t arg;
    ok_xstatus_typ_t res;
    bool ret (false);
  }
  cli = okld ()->okd ().cli ();

  if (lazy) { set_states (OKC_STATE_STANDBY); }

  if (cli) {
    arg.proc.name = _servpath;
    arg.proc.num_brothers = _children.size ();
    arg.proc.brother_id = 0;
    arg.lazy = lazy;
    twait { RPC::okld_program_1::okld_reserve (cli, arg, &res, mkevent (err)); }
    if (err) {
      str e = strbuf () << err;
      okdbg_warn (ERROR, "%s: cannot reserve space for service: %s\n", 
		  _servpath.cstr(), e.cstr ());
    } else if (res != OK_STATUS_OK) {
      okdbg_warn (ERROR, "%s: okd rejected reservation with code=%d\n", 
		  _servpath.cstr (), int (res));
    } else {
      set_states (OKC_STATE_STANDBY);
      ret = true;
    }
  } else {
    okdbg_warn (ERROR, "%s: cannot launch service since okd went away",
		_servpath.cstr ());
  }

  if (!ret) { set_states (OKC_STATE_HOSED); }

  ev->trigger (ret);
}

//-----------------------------------------------------------------------

void
okld_ch_cluster_t::set_states (okc_state_t s)
{
  for (size_t i = 0; i < _children.size (); i++) {
    _children[i]->set_state (s);
  }
}

//-----------------------------------------------------------------------

 void 
 okld_ch_cluster_t::set_service_options (const svc_options_t &so)
 { 
   _svc_options = so; 

   // distribute the ports event about the running processes
   size_t n = _children.size ();
   for (size_t i = 0; i < so.ports.size (); i++) {
     _children[i % n]->add_direct_port (so.ports[i]);
   }
 }

//-----------------------------------------------------------------------

void okld_ch_t::add_direct_port (int p) { _direct_ports.add_port (p); }

//-----------------------------------------------------------------------

ptr<okld_ch_t>
okld_ch_cluster_t::get_child (size_t i)
{
  ptr<okld_ch_t> ret;
  if (i < _children.size ()) { ret = _children[i]; }
  return ret;
}

//-----------------------------------------------------------------------

