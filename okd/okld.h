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
#include "svq.h"
#include "okconst.h"
#include "okclone.h"
#include "tame.h"
#include "tame_lock.h"

#define OK_LQ_SIZE_D    100
#define OK_LQ_SIZE_LL   5
#define OK_LQ_SIZE_UL   1000

class okld_t;

//=======================================================================

/*
 * A class to hold onto a process command/argv and its environment
 * so that it can be later launched.
 */
class okld_helper_t {
public:
  okld_helper_t (const str &n, const str &u, const str &g);
  ~okld_helper_t () {}
  void set_env (ptr<env_argv_t> e) { _env = e; }
  vec<str> &argv () { return _argv; }
  char* const* env () const ;
  void set_group (const str &g);
  void set_user (const str &u);
  const ok_usr_t &user () const { return _usr; }
  ok_usr_t &user () { return _usr; }
  const ok_grp_t &group () const { return _grp; }
  ok_grp_t &group () { return _grp; }

  void set_chldcb (cbi::ptr cb);
  void make_cli (const rpc_program &p, cbv::ptr eofcb);
  void make_srv (const rpc_program &p, callback<void, svccb *>::ref cb);
  void disconnect () { _x = NULL; _cli = NULL; _srv = NULL; }
  ptr<axprt_unix> x () { return _x; }
  ptr<aclnt> cli () { return _cli; }
  bool launch ();
  str dumpdir () const { return _dumpdir; }

  bool configure (jailable_t *j, const str &prfx);
  void activate ();
  bool active () const { return _active; }

protected:
  bool configure_user ();
  bool configure_group ();
  bool configure_dumpdir (jailable_t *j, const str &prfx);

  const str _name;

  vec<str> _argv;
  ptr<env_argv_t> _env;
  env_argv_t _empty_env;
  int _pid;

  ptr<axprt_unix> _x;
  ptr<aclnt> _cli;
  ptr<asrv> _srv;
  ok_usr_t _usr;
  ok_grp_t _grp;
  bool _active;
  str _dumpdir;
};

//=======================================================================

class okld_helper_okd_t : public okld_helper_t {
public:
  okld_helper_okd_t (const str &u, const str &g)
    : okld_helper_t ("okd", u, g) {}
private:
};

//=======================================================================

class okld_helper_ssl_t : public okld_helper_t {
public:
  okld_helper_ssl_t (const str &u, const str &g);

  str _certfile, _keyfile, _chainfile;
  u_int _ssl_timeout;
  str _cipher_list;
  bool _cipher_order;
  bool _cli_renog;
  bool _ssl_debug_startup;
  bool configure_keys ();
  str certfile_resolved () const { return _certfile_resolved; }
  str keyfile_resolved () const { return _keyfile_resolved; }
  str chainfile_resolved () const { return _chainfile_resolved; }
  str cipher_list () const;
  bool cipher_order() const;

  void parse_certfile(str s) { _certfile = s; }
  void parse_keyfile(str s) { _keyfile = s; }
  void parse_chainfile(str s) { _chainfile = s; }
  void parse_ssl_timeout(str s) { _ssl_timeout = atoi(s.cstr()); }
  void parse_cipher_list(str s) { _cipher_list = s; }
  void parse_cipher_order(str s) { _cipher_order = (bool)(atoi(s.cstr())); }
  void parse_cli_renog(str s) { _cli_renog = (bool)(atoi(s.cstr())); }
  void parse_ssl_debug_startup(str s) { 
      _ssl_debug_startup = (bool)(atoi(s.cstr())); }

  void add_port(okws1_port_t p) { _ports.insert(p); _port_list.push_back(p); }
  bool listening_on(okws1_port_t p) { return _ports[p]; }
  vec<okws1_port_t>& ports() { return _port_list; }

private:
  str resolve (const str &in, const char *which) const;
  bhash<okws1_port_t> _ports;
  vec<okws1_port_t> _port_list;
  str _certfile_resolved, _keyfile_resolved, _chainfile_resolved;

};

//=======================================================================

/**
 * a class for things that will be jailed into OKWS's runtime directory,
 * such as compile scripts, and also Python interpreters. Mainly, it
 * will be involved with fiddling access mode bits, settings users/groups
 * and so on.
 */
class okld_jailed_exec_t {
public:
  okld_jailed_exec_t (const str &e, okld_t *o, const str &l) :
    _rexecpath (e), 
    _okld (o), 
    _cfgfile_loc (l), 
    _have_ustat (false),
    _exec_uid (-1), 
    _exec_gid (-1), 
    _mode (-1), 
    _unsafe (false) {}
  ~okld_jailed_exec_t () {}

  bool get_unix_stat ();
  int get_exec_mode ();
  int get_exec_uid ();
  int get_exec_gid ();
  void assign_exec_ownership (int u, int g);
  bool chown ();
  const str &loc () const { return _cfgfile_loc; }
  void assign_mode (int m) { _mode = m; }
  bool chmod (int m);
  void set_unsafe (bool b = true) { _unsafe = b; }

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

  const str & get_execpath_relative_to_chroot () { return _rexecpath; }

  okld_t *okld () { return _okld; }
  const okld_t *okld () const { return _okld; }

  /**
   * Fix an executable to have the right ownership and permissions;
   * do this before every reboot of a service, in case it was replaced
   * and rebooted. This will actually call chmod()/chown() if needed.
   */
  bool fix_exec ();

protected:
  const str _rexecpath;      // execpath relative to jaildir (starts with '/')
  okld_t *_okld;
  str _cfgfile_loc;
  bool _have_ustat;
  struct stat _ustat;

  int _exec_uid, _exec_gid;  // UID/GID of the executable!
  int _mode;
  bool _unsafe;

};

//=======================================================================

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
    : okld_jailed_exec_t (strbuf ("%s-%d", i._rexecpath.cstr (), gid), 
			  i._okld, i._cfgfile_loc),
      _name (i._name),
      _user (uid), 
      _group (gid), 
      _args (i._args) {}

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

//=======================================================================

struct svc_options_t {
  svc_options_t () 
    : svc_reqs (-1), 
      svc_time (-1), 
      wss (-1),
      pub3_caching (-1),
      pub3_viserr (-1),
      wait_for_signal (-1),
      hiwat (-1),
      lowat (-1),
      gzip (-1),
      gzip_level (-1),
      ahttpcon_zombie_warn (-1),
      ahttpcon_zombie_timeout (-1),
      _n_procs (1) {}

  bool apply_global_defaults (const str &svc);
  bool check_options (const str &loc) const;
  size_t n_procs () const { return _n_procs; }

  // service-specific options
  //
  // < 0   =>   no value set (default)
  // = 0   =>   value set, but unlimited
  // > 0   =>   value set, but limited by provided value
  int svc_reqs;
  int svc_time;

  // -1 => nothing specified
  //  0 => OFF
  //  1 => ON
  int wss;

  int pub3_caching;
  int pub3_viserr;

  int wait_for_signal;

  int hiwat, lowat;

  // < 0  => no value set
  // >=0  => value set, use this value
  int gzip;
  int gzip_level;

  int ahttpcon_zombie_warn;
  int ahttpcon_zombie_timeout;

  vec<int> ports;
  size_t _n_procs;
};

//=======================================================================

class okld_ch_cluster_t;

class okld_ch_t {
public:
  okld_ch_t (okld_ch_cluster_t *c, size_t i);

  void launch (bool startup, evv_t ev, CLOSURE);
  void bind_ports (bool retry, evb_t ev, CLOSURE);
  void sig_chld_cb (int status);
  void chldcb (int status) { chldcb_T (status); }
  str str_id () const;
  void post_fork_cb ();
  void set_pid (int i) { _pid = i; }

  okld_t *okld ();
  void add_direct_port (int p);
  void post_spawn (int fd, evb_t ev, CLOSURE);
  size_t id () const { return _id; }
  void lazy_startup (evb_t ev, CLOSURE);
  void set_state (okc_state_t s) { _state = s; }
  const ok_direct_ports_t &direct_ports() const { return _direct_ports; }
  ok_direct_ports_t &direct_ports () { return _direct_ports; }
  ok_xstatus_typ_t kill (int sig);

private:
  void resurrect (evv_t ev, CLOSURE);
  void chldcb_T (int status, CLOSURE);

  okld_ch_cluster_t *_cluster;
  size_t _id, _brother_id;
  str _servpath;
  int _pid;
  okc_state_t _state;
  timecb_t *_rcb;
  vec<struct timespec> _timevals;
  int _nsent;
  ok_direct_ports_t _direct_ports;
  tame::lock_t _lazy_lock;
  time_t _startup_time;
};

//=======================================================================

class okld_t;
class okld_ch_cluster_t : 
  public okld_jailed_exec_t { // OK Launch Daemon Child Handle
public:
  okld_ch_cluster_t (const str &e, const str &s, okld_t *o, const str &cfl, 
		     ok_usr_t *u, vec<str> env, okws1_port_t p, size_t n = 1);
  virtual ~okld_ch_cluster_t ();

  str servpath () const { return _servpath; }
  okws1_port_t get_port () const { return _port; }

  // no longer const -- can change after we get the listen port
  str _servpath;       // GET <servpath> HTTP/1.1 (starts with '/')
  okws1_port_t _port;  // Which port to accept as in :81/foo

  // who we will setuid to after the spawn
  ok_usr_t *_uid;           // UID of whoever will be running this thing
  int _gid;                 // GID of whever will be running this thing

  void set_run_dir (const str &d) { _rundir = d; }
  void add_args (const vec<str> &a);
  bool has_direct_ports () const { return _svc_options.ports.size () > 0; }

  void set_service_options (const svc_options_t &so);

  // add more arguments as we can parse for more options
  //bool parse_service_options (vec<str> *v, ok_usr_t **u, const str &loc);

  void gather_helper_fds (str s, int *log, int *pub, evb_t ev, CLOSURE);
  ptr<axprt_unix> spawn_proc (okld_ch_t *ch, int lfd, int pfd);
  void launch (evv_t ev, CLOSURE);
  bool can_exec ();
  void assign_uid (int u);
  void assign_gid (int u) { _gid = u; }
  void set_svc_ids (int _pid);
  ok_usr_t *usr () { return _uid; }
  virtual int get_desired_execfile_mode () const { return ok_svc_mode; }
  virtual str get_interpreter () const { return NULL; }
  virtual bool fixup_doall (int uo, int un, int go, int gn, int mo);
  size_t n_children () const { return _children.size (); }
  ptr<okld_ch_t> get_child (size_t s);
  void reserve (bool lazy, evb_t ev, CLOSURE);
  void clean_dumps ();
  void set_states (okc_state_t s);
 
protected:
  svc_options_t _svc_options;

private:
  bool _have_ustat;
  struct stat _ustat;
  str _rundir;

  vec<str> _env;

  // arguments given to the executable (such as 'python filename')
  vec<str> _args;
  vec<ptr<okld_ch_t> > _children;
  okld_t *_okld;
  ok_direct_ports_t _direct_ports;
};

//=======================================================================

class okld_ch_script_t : public okld_ch_cluster_t {
public:
  okld_ch_script_t (const str &e, const str &s, okld_t *o, const str &cfl, 
		    okld_interpreter_t *ipret,
		    ok_usr_t *u, vec<str> env,
		    okws1_port_t p = 0, size_t n = 1)
    : okld_ch_cluster_t (e, s, o, cfl, u, env, p, n),
      _ipret (ipret), 
      _free_ipret (false) {}
  ~okld_ch_script_t () { if (_free_ipret && _ipret) delete _ipret; }
  int get_desired_execfile_mode () const { return ok_script_mode; }
  str get_interpreter () const { return _ipret->get_execpath (); }
  bool fixup_doall (int uo, int un, int go, int gn, int mo);
private:
  okld_interpreter_t *_ipret;
  bool _free_ipret;
};

//=======================================================================

class okld_t : public ok_base_t , public config_parser_t
{
public:
  okld_t () ;
  ~okld_t () { if (logexc) delete logexc; }

  void got_service (bool script, vec<str> s, str loc, bool *errp);
  void got_ssl_primary_port (vec<str> s, str loc, bool *errp);
  void got_service2 (vec<str> s, str loc, bool *errp);
  void got_okd_exec (vec<str> s, str loc, bool *errp);
  void got_okssl_exec (vec<str> s, str loc, bool *errp);
  void got_generic_exec (okld_helper_t *h, vec<str> s, str loc, bool *errp);
  void got_logd_exec (vec<str> s, str log, bool *errp);
  void got_pubd_exec (vec<str> s, str log, bool *errp);
  void got_interpreter (vec<str> s, str log, bool *errp);

  // SSL config processors
  void got_ssl_create_channel(vec<str> s, str log, bool* errp);
  void got_certfile(vec<str> s, str log, bool* errp);
  void got_keyfile(vec<str> s, str log, bool* errp);
  void got_chainfile(vec<str> s, str log, bool* errp);
  void got_ssl_listen_port(vec<str> s, str log, bool* errp);
  void got_ssl_timeout(vec<str> s, str log, bool* errp);
  void got_cipher_list(vec<str> s, str log, bool* errp);
  void got_cipher_order(vec<str> s, str log, bool* errp);
  void got_cli_renog(vec<str> s, str log, bool* errp);
  void got_ssl_debug_startup(vec<str> s, str log, bool* errp);

  void okld_exit (int rc);

  void launch (str s, CLOSURE);
  void launch_logd (evi_t ev, CLOSURE);
  void launch_pubd (evi_t ev, CLOSURE);

  bool launch_okd (int logfd, int pubd);
  void launch_okssl (evb_t ev, CLOSURE);

  bool parseconfig (const str &cf);
  bool lazy_startup () const { return _lazy_startup; }

  // XXX to fix --- don't duplicate this logic here and in okd
  bool need_okd_rpc () const;

  void set_signals ();
  void caught_signal (int sig);
  void caught_okd_eof ();
  void okd_dispatch (svccb *sbp);
  void shutdown1 ();
  void shutdown2 (int status);
  void shutdown_ssl (int status);
  bool init_jaildir ();
  bool init_okssl_jaildirs();
  bool init_interpreters ();
  bool in_shutdown () const { return sdflag; }
  str get_root_coredir () const { return root_coredir; }
  bool init_ssl ();
  void gather_helper_fds (str ch, int *lfd, int *pfd, evb_t ev, CLOSURE);
  log_primary_t *get_log_primary () { return _log_primary; }

  clone_only_client_t *get_pubd () const { return _pubd; }

  logd_parms_t logd_parms;
  log_primary_t *_log_primary;

  cgi_t *env () { return &_env; }

  cgi_t _env;    // execution environment

  ok_grp_t svc_grp;
  bool safe_startup () const { return safe_startup_fl ;}
  void set_opt_daemon (bool b) { _opt_daemon = b; }
  bool opt_daemon () const { return _opt_daemon; }
  void emergency_kill (svccb *sbp, CLOSURE);

  okld_helper_t &okd () { return _okd; }
  const okld_helper_t &okd () const { return _okd; }
  const ok_grp_t &coredump_grp () const { return _coredump_grp; }
  const ok_usr_t &coredump_usr () const { return _coredump_usr; }
  int coredump_mode () const { return _coredump_mode; }
  qhash<str, ptr<okld_helper_ssl_t> >& okssls() { return _okssls; } 

  const vec<time_t> &bind_reattempt_schedule () const 
  { return _bind_reattempt_schedule; }

  bool get_config_resolve_bins () { return _config_resolve_bins; }

protected:
  bool parse_file (const str &fn);
  bool post_config (const str &fn);
  void poke_lazy_service_2 (const oksvc_proc_t &p, okstat_ev_t ev, CLOSURE);
  void poke_lazy_service (svccb *sbp, CLOSURE);

private:

  bool guess_pubd (const str &cf);
  bool parse_bind_reattempt_schedule ();
  void logd_crashed ();

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
  vec<alias_t> regex_aliases_tmp;
  bhash<okws1_port_t> used_ports; // ports specified with services, etc..

  bool check_exes ();
  bool check_services_and_aliases ();
  bool check_service_ports ();
  bool check_ports ();
  bool is_ssl_port_active(okws1_port_t port);
  void got_alias (vec<str> s, str loc, bool *errp);
  void got_regex_alias (vec<str> s, str loc, bool *errp);
  bool fixup_ssl_ports ();

  bool fix_uids ();
  bool fix_coredump_uids ();
  void add_svc (ptr<okld_ch_cluster_t> c);
  bool config_jaildir ();
  void init_clock_daemon ();
  void relaunch_clock_daemon (int sig);
  void clock_daemon_died (int sig);
  bool check_uri (const str &loc, const str &uri, okws1_port_t *port = NULL) 
    const;
  ptr<okld_helper_ssl_t> okssl_by_name(str s = "default");
  void add_ssl_port(okws1_port_t port, str channel = "default");

  void launch_logd ();
  void launch_logd_cb (bool err);
  void launch_logd_cb2 (int logfd);
  void encode_env ();
  void launch2 (int fd);
  void launch_services (evv_t ev, CLOSURE);

  vec<ptr<okld_ch_cluster_t> > _svcs;
  qhash<str, ptr<okld_ch_cluster_t> > _svc_lookup;

  int nxtuid;
  str coredump_path;
  helper_exec_t *logexc, *_pubd_exc;

  str coredumpdir;
  str sockdir;

  str configfile;
  bool sdflag;

  str service_bin;       // directory where service exes are kept
  bool unsafe_mode;      // for lazy bastards
  bool safe_startup_fl;  // allows bad children to keep restarting 

  str root_coredir;      // privileged core directory


  okld_helper_t _okd;
  qhash<str, ptr<okld_helper_ssl_t> > _okssls;
  bool _auto_activate;
  vec<str> _ssl_exec_params;

  sfs_clock_t clock_mode;
  str mmc_file;
  str mmcd;
  pid_t mmcd_pid;
  int mmcd_ctl_fd;
  u_int launchp;

  bool used_primary_port;

  ihash<const str, okld_interpreter_t, 
	&okld_interpreter_t::_name,
	&okld_interpreter_t::_link> interpreters;

  clone_only_client_t *_pubd;
  str _okd_mgr_socket;
  
  strbuf _errs;

  // variables set during configuration stage
  str _config_grp, _config_okd_gr, _config_okd_un;
  str _config_ssl_gr, _config_ssl_un;
  str _config_root, _config_wheel;
  bool _config_no_pub_v2_support;
  bool _opt_daemon;
  bool _lazy_startup;

  ok_usr_t _coredump_usr;
  ok_grp_t _coredump_grp;
  int _coredump_mode;
  bool _aggressive_svc_restart;
  bool _die_on_logd_crash;

  str _bind_reattempt_schedule_str;
  vec<time_t> _bind_reattempt_schedule;
  bool _emerg_kill;

  bool _config_resolve_bins;
};

//=======================================================================

#endif /* _OKD_OKD_H */
