
// -*-c++-*-
/* $Id$ */

#ifndef _LIBAOK_OK_H
#define _LIBAOK_OK_H

#include "arpc.h"
#include "ahttp.h"
#include "cgi.h"
#include "resp.h"
#include "pub.h"
#include "okprot.h"
#include "inhdr.h"
#include "pslave.h"
#include "pubutil.h"
#include "oklog.h"
#include "zstr.h"
#include "ahparse.h"
#include "pjail.h"
#include "lbalance.h"

typedef enum { OKC_STATE_NONE = 0,
	       OKC_STATE_LAUNCH = 1,
	       OKC_STATE_SERVE = 2,
	       OKC_STATE_HOSED = 3,
	       OKC_STATE_CRASH = 4,
	       OKC_STATE_DELAY = 5,
	       OKC_STATE_LAUNCH_SEQ_1 = 6,
	       OKC_STATE_LAUNCH_SEQ_2 = 7 } okc_state_t;

#define SVCWARN(x) \
  warn << "pid " << pid << ": " << x << "\n";

struct errdoc_t {
  errdoc_t (int n, const str &f) : status (n), fn (f) {}
  int status;
  str fn;
  ihash_entry<errdoc_t> lnk;
};

class ok_con_t {
public:
  ok_con_t () {}
protected:
  void ctlcon (callback<void, svccb *>::ref cb);
  void ctlclose ();

  ptr<axprt_unix> ctlx;
  ptr<aclnt> clnt;
  ptr<asrv> srv;
};

class ok_base_t : public jailable_t {
public:
  ok_base_t (const str &h = NULL, int fd = -1)
    : jailable_t (ok_jaildir_top), version (ok_version),
      listenport (ok_dport),
      listenaddr (INADDR_ANY),
      topdir (ok_topdir),
      reported_name (ok_dname),
      logd (NULL), logfd (fd)
      //jaildir_run (ok_jaildir_run) 
  {}

  log_t *get_logd () { return logd; }
  void got_bindaddr (vec<str> s, str loc, bool *errp);
  str okws_exec (const str &path) const;
  //str doubly_jail_rundir () const { return nest_jails (jaildir_run); }

  // copies of system-wide constants local to this instantiation; not
  // necessary now, but mabye down the road......
  str version;
  str hostname;
  u_int16_t listenport;
  str listenaddr_str;
  u_int32_t listenaddr;
  str topdir;
  str reported_name; // name reported in HTTP headers and ERR docs
  str debug_stallfile;


protected:
  log_t *logd;
  int logfd;
  //str jaildir_run;  // nested jaildir for okd and services
};

class ok_httpsrv_t : public ok_con_t, public ok_base_t { 
public:
  ok_httpsrv_t (const str &h = NULL, int fd = -1) 
    : ok_con_t (), ok_base_t (h, fd), svclog (true),
      accept_enabled (false), accept_msgs (true),
      clock_mode (SFS_CLOCK_GETTIME),
      mmc_file (ok_mmc_file) {}
  virtual ~ok_httpsrv_t () { errdocs.deleteall (); }
  inline bool add_errdoc (int n, const str &f);
  void add_errdocs (const xpub_errdoc_set_t &eds);
  virtual void add_pubfile (const str &s) {}

  virtual void error (ref<ahttpcon> x, int n, str s = NULL, cbv::ptr c = NULL,
		      http_inhdr_t *h = NULL) const;
  virtual str servinfo () const;
  virtual ptr<http_response_t> geterr (int n, str s, htpv_t v) const;
  virtual void log (ref<ahttpcon> x, http_inhdr_t *req, http_response_t *res,
		    const str &s = NULL)
    const { if (svclog && logd) logd->log (x, req, res, s); }

  void enable_accept ();
  void disable_accept ();
  void malloc_init (); // init malloc.3

  // toggle clock modes for SFS
  void init_sfs_clock (const str &f); 

protected:
  void error2 (ref<ahttpcon> x, int n, str s, cbv::ptr c, http_inhdr_t *h) 
    const;
  void error_cb1 (ptr<http_parser_raw_t> p, int n, str s, cbv::ptr c,
		  int s2) const;
  void error_cb2 (ptr<ahttpcon> x, ptr<http_response_t> e, cbv::ptr c) 
    const { if (c) (*c) (); }

  virtual void enable_accept_guts () = 0;
  virtual void disable_accept_guts () = 0;

  ihash<int, errdoc_t, &errdoc_t::status, &errdoc_t::lnk> errdocs;
  mutable str si;
  str logfmt;
  bool svclog;
  bool accept_enabled;
  bool accept_msgs;

  // stuff for dealing with okws's clock mode
  sfs_clock_t clock_mode;
  str mmc_file;
};

#define OKCLNT_BUFLEN 0x10400
#define OKCLNT_BUFLEN2 0x4000

class oksrvc_t;
class okclnt_t 
  : public http_parser_cgi_t { // One for each external HTTP client
public:
  okclnt_t (ptr<ahttpcon> xx, oksrvc_t *o, u_int to = 0) : 
    http_parser_cgi_t (xx, to), x (xx), oksrvc (o),
    process_flag (false), uid_set (false), rsp_gzip (true)
  {}

  virtual ~okclnt_t ();
  virtual void serve ();
  virtual void error (int n, const str &s = NULL);
  virtual void process () = 0;
  virtual void parse ();
  virtual void output (zbuf &b);
  virtual void redirect (const str &s, int status = HTTP_MOVEDPERM);
  virtual void send (ptr<http_response_t> rsp);
  virtual cookie_t *add_cookie (const str &h = NULL, const str &p = "/");
  void set_uid (u_int64_t i) { uid = i; uid_set = true; }

  //
  // set these for different HTTP response configurations;
  // should of course have more of them.
  //
  void set_content_type (const str &s) { contenttype = s; }
  void set_cache_control (const str &s) { cachecontrol = s; }
  void set_expires (const str &s) { expires = s; }
  void disable_gzip () { rsp_gzip = false; }

  list_entry<okclnt_t> lnk;
protected:
  void http_parse_cb (int status);
  void delcb () { delete this; }
		
  cbv::ptr cb;
  ref<ahttpcon> x;
  oksrvc_t *oksrvc;

  //strbuf out;
  zbuf out;
  ptr<http_response_t> rsp;
  vec<cookie_t *> outcookies;
  bool process_flag;
  u_int64_t uid; // hacked in for now;
  bool uid_set;

  str contenttype, cachecontrol, expires;
  bool rsp_gzip;
};

typedef callback<okclnt_t *, ptr<ahttpcon>, oksrvc_t *>::ref nclntcb_t;

class dbcon_t : public helper_inet_t {
public:
  dbcon_t (const rpc_program &g, const str &h, u_int p)
    : helper_inet_t (g, h, p, 0) {}


  str getname () const { return strbuf ("database: ") << 
			   helper_inet_t::getname () ;}
};

class oksrvc_t : public ok_httpsrv_t { // OK Service
public:
  oksrvc_t (int argc, char *argv[]) 
    : nclients (0), sdflag (false), pid (getpid ()), n_fd_out (0)
  { 
    init (argc, argv);
    accept_msgs = ok_svc_accept_msgs;
  }

  virtual void launch ();
  virtual okclnt_t *make_newclnt (ptr<ahttpcon> lx) = 0;
  virtual void init_publist () {}
  virtual u_int get_andmask () const { return 0xffffffff; }
  virtual u_int get_ormask () const { return 0; }
  virtual void custom_init (cbv cb) { (*cb) (); }

  ~oksrvc_t () {}
  void init (int argc, char *argv[]);
  void shutdown ();
  void connect ();
  void ctldispatch (svccb *c);
  void remove (okclnt_t *c);
  void add (okclnt_t *c);
  void end_program ();

  void add_pubfiles (const char *arr[], u_int sz, bool conf = false);
  void add_pubfiles (const char *arr[], bool conf = false);
  void add_pubfile (const str &s, bool conf = false);

  str cfg (const str &n) const { return rpcli->cfg (n); }
  template<class C> bool cfg (const str &n, C *v) const 
  { return rpcli->cfg (n, v); }
  template<typename T> parr_err_t cfg (const str &n, u_int i, T *p) const;
  void pubfiles (cbb cb);
  dbcon_t *add_db (const str &host, u_int port, const rpc_program &p,
		   int32_t txa_login_rpc = -1);
  lblnc_t *add_lb (const str &i, const rpc_program &p, int port = -1);

  pval_w_t operator[] (const str &s) const { return (*rpcli)[s]; }

protected:
  void closed_fd ();
  void enable_accept_guts ();
  void disable_accept_guts ();

  void pubbed (cbb cb, ptr<pub_res_t> res);

  void launch_log_cb (bool rc);
  void launch2 (bool rc);
  void launch_pub_cb0 (ptr<pub_res_t> r);
  void launch_pub_cb1 (ptr<xpub_errdoc_set_t> xd, clnt_stat err);
  void launch_pub_cb2 (bool rc);
  void launch3 ();
  void launch4 ();
  void launch5 (clnt_stat err);
  void launch_dbs (cbb b);
  void launch_dbcb (cbb b, u_int i, bool r);

  void newclnt (ptr<ahttpcon> lx);
  void update (svccb *sbp);
  void kill (svccb *v);
  void update_cb (svccb *sbp, ptr<pub_res_t> pr);
  void ready_call (bool rc);

  str name;
  list<okclnt_t, &okclnt_t::lnk> clients;
  ptr<ahttpcon_listen> x;

  u_int nclients;
  bool sdflag;
  pub_rclient_t *rpcli;

  vec<helper_base_t *> dbs;
  bool dbstatus;
  u_int dbl;
  u_int lnum;
  int pid;

  vec<str> authtoks;
  int n_fd_out;
};

class oksrvcw_t : public oksrvc_t { // OK Service Wrapped
public:
  oksrvcw_t (int argc, char *argv[], nclntcb_t c) : 
    oksrvc_t (argc, argv), nccb (c) {}
  okclnt_t *make_newclnt (ptr<ahttpcon> lx) { return (*nccb) (lx, this); }
private:
  nclntcb_t nccb;
}; 

template<typename T> parr_err_t 
oksrvc_t::cfg (const str &n, u_int i, T *p) const
{
  pval_t *v;
  const parr_ival_t *arr;
  if (!rpcli->cfg (n, &v))
    return PARR_NOT_FOUND;
  if (!(arr = v->to_int_arr ()))
    return PARR_BAD_TYPE;
  return arr->val (i, p);
}
  
str okws_exec (const str &x);
void init_syscall_stats ();

inline void do_syscall_stats ()
{
  if (ok_ssdi > 0 && int (timenow - global_ssd_last) > int (ok_ssdi)) {
    time_t diff = timenow - global_ssd_last;
    global_ssd_last = timenow;
    global_syscall_stats->dump (diff);
    global_syscall_stats->clear ();
  }
}

//
// XXX - hack - this is used by both okch_t and okld_ch_t - just happens
// that they have similar internal variables; the might be put into a 
// class tree, but they share little functionality in common.
//
#define CH_WARN(x) \
  warn << servpath << ":" << pid << ": " << x << "\n";

#endif /* _LIBAOK_OKBASE_H */
