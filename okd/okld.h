// -*-c++-*-

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
  bool can_exec (bool chrt);
  void chldcb (int status);
  bool chmod (int mode);
  bool chown (int uid, int gid);
  void clean_dumps ();
private:
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
};

class okld_t : public ok_base_t 
{
public:
  okld_t () 
    : svc_grp (ok_svc_gname),
      nxtuid (ok_svc_uid_low), logexc (NULL), 
      coredumpdir (ok_coredumpdir), sockdir (ok_sockdir), okd_pid (-1),
      sdflag (false), service_bin (ok_service_bin),
      unsafe_mode (false), safe_startup_fl (true) {}

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
};

#endif /* _OKD_OKD_H */
