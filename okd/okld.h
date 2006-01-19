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
#include "okcgi.h"
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

/**
 * a class for things that will be jailed into OKWS's runtime directory,
 * such as compile scripts, and also Python interpreters. Mainly, it
 * will be involved with fiddling access mode bits, settings users/groups
 * and so on.
 */
class okld_jailed_exec_t {
public:
  okld_jailed_exec_t (const str &e, okld_t *o, const str &l) :
    rexecpath (e), okld (o), cfgfile_loc (l), have_ustat (false),
    exec_uid (-1), exec_gid (-1), mode (-1) {}
  ~okld_jailed_exec_t () {}

  bool get_unix_stat ();
  int get_exec_mode ();
  int get_exec_uid ();
  int get_exec_gid ();
  void assign_exec_ownership (int u, int g);
  bool chown ();
  const str &loc () const { return cfgfile_loc; }
  void assign_mode (int m) { mode = m; }
  bool chmod (int m);

  /**
   * get the execpath relative to current file system root, adjusted
   * to whether or not we are jailed.
   *
   */
  str get_execpath () const ;

  /**
   * Change the executable file to have the given uid, gid, and mode.
   * Set the object to have the appropriate intended values for
   * uid_new, gid_new, and mode_new, and then call fix_exec().
   *
   * @param uid_new new UID to assign
   * @param gid_new new GID to assign
   * @param mode_new new file mode to assign
   */
  bool fixup (int uid_new, int gid_new, int mode_new);

  const str & get_execpath_relative_to_chroot () { return rexecpath; }

protected:

  /**
   * Fix an executable to have the right ownership and permissions;
   * do this before every reboot of a service, in case it was replaced
   * and rebooted. This will actually call chmod()/chown() if needed.
   */
  bool fix_exec ();

  const str rexecpath;      // execpath relative to jaildir (starts with '/')
  okld_t *okld;
  str cfgfile_loc;
  bool have_ustat;
  struct stat ustat;

  int exec_uid, exec_gid;  // UID/GID of the executable!
  int mode;

};

/**
 * wrapper class around an interpreter such as Python. Note that
 * for each interpreter, we should have a new group and user.
 * The last thing we want is for a compiled Web service to launch
 * a Python shell if hacked.
 */
class okld_interpreter_t : public okld_jailed_exec_t {
public:
  okld_interpreter_t (const str &n, const str &u, const str &g, 
		      const vec<str> &e, const str &p,
		      const vec<str> &a, okld_t *ol,
		      const str &cfl)
    : okld_jailed_exec_t (p, ol, cfl),
      _name (n), _user (u), _group (g), _env (e), _args (a) {}

  /**
   * specialize an interpreter for a particular UID/GID pair.
   * Name the interpreter /bin/python-5234, where 5234 is the
   * gid the interpreter will be owned by.
   *
   * @param i the existing interpreter to base the new one on
   * @param uid the uid that the new interpreter will be owend by
   * @param gid the gid that the new interpreter will belong to
   */
  okld_interpreter_t (const okld_interpreter_t &i, int uid, int gid)
    : okld_jailed_exec_t (strbuf ("%s-%d", i.rexecpath.cstr (), gid), 
			  i.okld, i.cfgfile_loc),
      _name (i._name),
      _user (uid), _group (gid), _args (i._args) {}


  /**
   * to be called while parsing the configuration line from within
   * the main config file parse file
   */
  bool check (str *err);

  /**
   * Initialize the base interpreter (not the one copied over per Script)
   */
  bool base_init ();

  /**
   * examples include Python, PERL, PYTHON-2.3, etc...
   */
  const str _name;
  ihash_entry<okld_interpreter_t> _link;
  vec<str> get_env () const { return _env; }
private:
  ok_usr_t _user;
  ok_grp_t _group;
  vec<str> _env;
  vec<str> _args;
};

class okld_t;
class okld_ch_t : public okld_jailed_exec_t { // OK Launch Daemon Child Handle
public:
  okld_ch_t (const str &e, const str &s, okld_t *o, const str &cfl, 
	     ok_usr_t *u, vec<str> env, okws1_port_t p = 0) ;
  virtual ~okld_ch_t () { if (uid) delete uid ;  }
  void launch ();
  void sig_chld_cb (int status);

  int pid;

  // no longer const -- can change after we get the listen port
  str servpath;       // GET <servpath> HTTP/1.1 (starts with '/')

  // who we will setuid to after the spawn
  ok_usr_t *uid;           // UID of whoever will be running this thing
  int gid;                 // GID of whever will be running this thing


  void set_svc_ids ();
  void set_run_dir (const str &d) { rundir = d; }
  void chldcb (int status);
  void clean_dumps ();
  void add_args (const vec<str> &a);

  okws1_port_t get_port () const { return port; }

  // add more arguments as we can parse for more options
  //bool parse_service_options (vec<str> *v, ok_usr_t **u, const str &loc);

  bool can_exec ();
  void assign_uid (int u);
  void assign_gid (int u) { gid = u; }
  ok_usr_t *usr () { return uid; }
  virtual int get_desired_execfile_mode () const { return ok_svc_mode; }
  virtual str get_interpreter () const { return NULL; }
  virtual bool fixup_doall (int uo, int un, int go, int gn, int mo);

  void set_svc_life_reqs (int i) { svc_life_reqs = i; }
  void set_svc_life_time (int i) { svc_life_time = i; }

protected:

  // service-specific options
  //
  // < 0   =>   no value set (default)
  // = 0   =>   value set, but unlimited
  // > 0   =>   value set, but limited by provided value
  int svc_life_reqs;
  int svc_life_time;

private:
  void resurrect ();
  void relaunch ();

  void launch_cb (int logfd);

  int xfd;
  int cltxfd;
  okc_state_t state;
  timecb_t *rcb;
  vec<struct timeval *> timevals;
  bool have_ustat;
  struct stat ustat;
  time_t startup_time;
  str rundir;

  vec<str> env;
  okws1_port_t port;
  int nsent;

  // arguments given to the executable (such as 'python filename')
  vec<str> args;

};

class okld_ch_script_t : public okld_ch_t {
public:
  okld_ch_script_t (const str &e, const str &s, okld_t *o, const str &cfl, 
		    okld_interpreter_t *ipret,
		    ok_usr_t *u, vec<str> env,
		    okws1_port_t p = 0)
    : okld_ch_t (e, s, o, cfl, u, env, p), _ipret (ipret), 
      _free_ipret (false) {}
  ~okld_ch_script_t () { if (_free_ipret && _ipret) delete _ipret; }
  int get_desired_execfile_mode () const { return ok_script_mode; }
  str get_interpreter () const { return _ipret->get_execpath (); }
  bool fixup_doall (int uo, int un, int go, int gn, int mo);
private:
  okld_interpreter_t *_ipret;
  bool _free_ipret;
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
      okd_dumpdir ("/tmp"), 
      clock_mode (SFS_CLOCK_GETTIME),
      mmcd (ok_mmcd), mmcd_pid (-1), launchp (0),
      used_primary_port (false) {}
      

  ~okld_t () { if (logexc) delete logexc; }

  void got_service (bool script, vec<str> s, str loc, bool *errp);
  void got_service2 (vec<str> s, str loc, bool *errp);
  void got_okd_exec (vec<str> s, str loc, bool *errp);
  void got_logd_exec (vec<str> s, str log, bool *errp);
  void got_interpreter (vec<str> s, str log, bool *errp);
  
  void okld_exit (int rc);

  void launch (const str &cf);
  void launch_logd (cbb cb);
  bool launch_okd (int logfd);

  void parseconfig (const str &cf);
  void set_signals ();
  void shutdown (int sig);
  void shutdown2 (int status);
  bool init_jaildir ();
  bool init_interpreters ();
  bool in_shutdown () const { return sdflag; }
  str get_root_coredir () const { return root_coredir; }

  logd_parms_t logd_parms;

  cgi_t env;    // execution environment
  ptr<fdsink_t> okdx;
  ok_grp_t svc_grp;
  bool safe_startup () const { return safe_startup_fl ;}

private:

  struct alias_t {
    alias_t (const str &t, const str &f, const str &l, okws1_port_t p)
      : to (t), from (f), loc (l), port (p) {}
    str to_str () const { strbuf b; b << from << " -> " << to; return b; }
    str to;
    str from;
    const str loc;
    const okws1_port_t port;
  };

  // 
  vec<alias_t> aliases_tmp;
  bhash<okws1_port_t> used_ports; // ports specified with services, etc..

  bool check_exes ();
  bool check_services_and_aliases ();
  bool check_service_ports ();
  bool check_ports ();
  void got_alias (vec<str> s, str loc, bool *errp);

  bool fix_uids ();
  bool config_jaildir ();
  void init_clock_daemon ();
  void relaunch_clock_daemon (int sig);
  void clock_daemon_died (int sig);
  bool check_uri (const str &loc, const str &uri, okws1_port_t *port = NULL) 
    const;

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

  bool used_primary_port;
  argv_t okdenv;

  ihash<const str, okld_interpreter_t, 
	&okld_interpreter_t::_name,
	&okld_interpreter_t::_link> interpreters;
};

#endif /* _OKD_OKD_H */
