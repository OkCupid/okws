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

#ifndef _OKD_OKLD_H
#define _OKD_OKLD_H

#include <sys/time.h>
#include "async.h"
#include "arpc.h"
#include "ahttp.h"
#include "cgi.h"
#include "resp.h"
#include "ok.h"
#include "pslave.h"
#include "okerr.h"
#include "svq.h"
#include "okconst.h"
#include "axprtfd.h"

#define OK_LQ_SIZE_D    100
#define OK_LQ_SIZE_LL   5
#define OK_LQ_SIZE_UL   1000

class okld_t;
class okld_ch_t { // OK Launch Daemon Child Handle
public:
  okld_ch_t (const str &e, const str &s, okld_t *o, const str &cfl, 
	     ok_usr_t *u = NULL);
  ~okld_ch_t () { if (uid) delete (uid); }
  void launch ();
  void sig_chld_cb (int status);

  int pid;
  const str rexecpath;      // execpath relative to jaildir (starts with '/')
  const str servpath;       // GET <servpath> HTTP/1.1 (starts with '/')

  str execpath;             // temporary variable

  void assign_uid (int u);
  void assign_gid (int u) { gid = u; }
  ok_usr_t *usr () { return uid; }
  str loc () const { return cfgfile_loc; }
  void set_svc_ids ();
  void set_run_dir (const str &d) { rundir = d; }
  int get_exec_uid ();
  int get_exec_gid ();
  int get_exec_mode ();
  bool can_exec (bool chrt);
  void chldcb (int status);
  bool chmod (int m);
  bool chown ();
  void clean_dumps ();

  void assign_exec_ownership (int u, int g);
  void assign_mode (int m) { mode = m; }
private:
  bool fix_exec (bool jail);
  void resurrect ();
  void relaunch ();
  bool get_unix_stat (bool chrt);

  void launch_cb (int logfd);

  okld_t *okld;

  str cfgfile_loc;
  int xfd;
  int cltxfd;
  ok_usr_t *uid;
  okc_state_t state;
  timecb_t *rcb;
  vec<struct timeval *> timevals;
  bool have_ustat;
  struct stat ustat;
  time_t startup_time;
  str rundir;
  int gid;

  int exec_uid, exec_gid;
  int mode;
};

class okld_t : public ok_base_t 
{
public:
  okld_t () 
    : svc_grp (ok_okd_gname),
      nxtuid (ok_svc_uid_low), logexc (NULL), 
      coredumpdir (ok_coredumpdir), sockdir (ok_sockdir), okd_pid (-1),
      sdflag (false), service_bin (ok_service_bin),
      unsafe_mode (false), safe_startup_fl (true),
      okd_usr (ok_okd_uname), okd_grp (ok_okd_gname),
      okd_dumpdir ("/tmp"), clock_mode (SFS_CLOCK_GETTIME),
      mmcd (ok_mmcd), mmcd_pid (-1), launchp (0) {}

  ~okld_t () { if (logexc) delete logexc; }

  void insert (okld_ch_t *c) { svcs.push_back (c); }
  void got_service (vec<str> s, str loc, bool *errp);
  void got_okd_exec (vec<str> s, str loc, bool *errp);
  void got_logd_exec (vec<str> s, str log, bool *errp);

  void launch (const str &cf);
  void launch_logd (cbb cb);
  bool launch_okd (int logfd);
  bool checkservices ();

  void parseconfig (const str &cf);
  void set_signals ();
  void shutdown (int sig);
  void shutdown2 (int status);
  bool init_jaildir ();
  bool in_shutdown () const { return sdflag; }
  str get_root_coredir () const { return root_coredir; }

  logd_parms_t logd_parms;

  cgi_t env;    // execution environment
  ptr<fdsink_t> okdx;
  ok_grp_t svc_grp;
  bool safe_startup () const { return safe_startup_fl ;}

private:
  bool fix_uids ();
  bool config_jaildir ();
  void init_clock_daemon ();
  void relaunch_clock_daemon (int sig);

  void launch_logd ();
  void launch_logd_cb (bool err);
  void launch_logd_cb2 (int logfd);
  void encode_env ();
  void launch2 (int fd);
  void launchservices ();

  vec<okld_ch_t *> svcs;
  int nxtuid;
  str okd_exec;
  str coredump_path;
  helper_exec_t *logexc;

  str okdexecpath;
  str coredumpdir;
  str sockdir;

  str configfile;
  int okd_pid;
  bool sdflag;

  str service_bin;       // directory where service exes are kept
  bool unsafe_mode;      // for lazy bastards
  bool safe_startup_fl;  // allows bad children to keep restarting 

  str root_coredir;      // privileged core directory

  ok_usr_t okd_usr;
  ok_grp_t okd_grp;
  str okd_dumpdir;
  sfs_clock_t clock_mode;
  str mmc_file;
  str mmcd;
  pid_t mmcd_pid;
  int mmcd_ctl_fd;
  u_int launchp;
};

#endif /* _OKD_OKD_H */
