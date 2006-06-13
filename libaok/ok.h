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

#ifndef _LIBAOK_OK_H
#define _LIBAOK_OK_H

#include "arpc.h"
#include "pub.h"
#include "parr.h"
#include "ahttp.h"
#include "okcgi.h"
#include "resp.h"
#include "okprot.h"
#include "inhdr.h"
#include "pslave.h"
#include "pubutil.h"
#include "oklog.h"
#include "zstr.h"
#include "ahparse.h"
#include "pjail.h"
#include "lbalance.h"
#include "tame.h"
#include "pub2.h"

typedef enum { OKC_STATE_NONE = 0,
	       OKC_STATE_LAUNCH = 1,
	       OKC_STATE_SERVE = 2,
	       OKC_STATE_HOSED = 3,
	       OKC_STATE_CRASH = 4,
	       OKC_STATE_DELAY = 5,
	       OKC_STATE_LAUNCH_SEQ_1 = 6,
	       OKC_STATE_LAUNCH_SEQ_2 = 7,
               OKC_STATE_KILLING = 8 } okc_state_t;


struct errdoc_t {
  errdoc_t (int n, const str &f) : status (n), fn (f) {}
  int status;
  str fn;
  ihash_entry<errdoc_t> lnk;
};

typedef u_int16_t okws1_port_t;
#define PORT_MAX USHRT_MAX

class ok_con_t {
public:
  ok_con_t () {}
protected:
  void ctlcon (callback<void, svccb *>::ref cb);
  void ctlclose ();

  ptr<axprt_unix> ctlx;
  ptr<asrv> srv;
  ptr<aclnt> clnt; 
};

class ok_base_t : public jailable_t {
public:
  ok_base_t (const str &h = NULL, int lfd = -1, int pfd = -1)
    : jailable_t (ok_jaildir_top), version (ok_version),
      listenport (ok_dport),
      listenaddr_str ("*"),
      listenaddr (INADDR_ANY),
      topdir (ok_topdir),
      reported_name (ok_wsname),
      logd (NULL), logfd (lfd), pub2fd (pfd),
      bind_addr_set (false)
      //jaildir_run (ok_jaildir_run) 
  {}

  log_t *get_logd () { return logd; }
  void got_bindaddr (vec<str> s, str loc, bool *errp);
  void got_ports (vec<str> s, str loc, bool *errp);
  str okws_exec (const str &path) const;
  //str doubly_jail_rundir () const { return nest_jails (jaildir_run); }

  // copies of system-wide constants local to this instantiation; not
  // necessary now, but mabye down the road......
  str version;
  str hostname;
  okws1_port_t listenport;
  str listenaddr_str;
  u_int32_t listenaddr;
  str topdir;
  str reported_name; // name reported in HTTP headers and ERR docs
  str debug_stallfile;
  str server_id;

  vec<okws1_port_t> allports;
  bhash<okws1_port_t> allports_map;


protected:
  str fix_uri (const str &in) const;

  log_t *logd;

  int logfd;
  int pub2fd;
  //str jaildir_run;  // nested jaildir for okd and services
  bool bind_addr_set; // called after got_bindaddr;
};

class ok_httpsrv_t : public ok_con_t, public ok_base_t { 
public:
  ok_httpsrv_t (const str &h = NULL, int fd = -1, int pub2fd = -1) 
    : ok_con_t (), ok_base_t (h, fd, pub2fd), svclog (true),
      accept_enabled (false), accept_msgs (true),
      clock_mode (SFS_CLOCK_GETTIME),
      mmc_file (ok_mmc_file) {}

  typedef callback<void, ptr<http_response_t> >::ref http_resp_cb_t;
      
  virtual ~ok_httpsrv_t () { errdocs.deleteall (); }
  virtual void add_pubfile (const str &s, bool conf = false) {}

  virtual void error (ref<ahttpcon> x, int n, str s = NULL, cbv::ptr c = NULL,
	      http_inhdr_t *h = NULL)
  { error_T (x, n, s, c, h); }

  virtual str servinfo () const;
  
  virtual void geterr (int n, str s, htpv_t v, bool gz, http_resp_cb_t cb)
  { geterr_T (n, s, v, gz, cb); }

  virtual void log (ref<ahttpcon> x, http_inhdr_t *req, http_response_t *res,
		    const str &s = NULL)
    const { if (svclog && logd) logd->log (x, req, res, s); }

  void enable_accept ();
  void disable_accept ();
  void malloc_init (); // init malloc.3

  // toggle clock modes for SFS
  void init_sfs_clock (const str &f); 

  // can overide this on a service-by-service basis
  virtual bool init_pub2 (u_int opts = 0);
  virtual void launch_pub2 (cbb cb) { launch_pub2_T (cb); }
  virtual void post_launch_pub2 (cbb cb) { (*cb) (true); }

  virtual ptr<pub2::remote_publisher_t> pub2 () { return _pub2; }

private:
  void geterr_T (int n, str s, htpv_t v, bool gz, http_resp_cb_t cb, CLOSURE);
  void error_T (ref<ahttpcon> x, int n, str s = NULL, cbv::ptr c = NULL,
		http_inhdr_t *h = NULL, CLOSURE);
  void launch_pub2_T (cbb cb, CLOSURE);

protected:

  virtual void enable_accept_guts () = 0;
  virtual void disable_accept_guts () = 0;

  ihash<int, errdoc_t, &errdoc_t::status, &errdoc_t::lnk> errdocs;
  xpub_errdoc_set_t errdocs_x;
  mutable str si;
  str logfmt;
  bool svclog;
  bool accept_enabled;
  bool accept_msgs;

  // stuff for dealing with okws's clock mode
  sfs_clock_t clock_mode;
  str mmc_file;

  ptr<pub2::remote_publisher_t> _pub2;
};

#define OKCLNT_BUFLEN 0x10400
#define OKCLNT_BUFLEN2 0x4000

class oksrvc_t;

//
// There should be one okclnt_base_t per external HTTP request.
// We've split the logic up between stuff that goes mainly here,
// and the x-URL-encoded-specific parsing stuff, as seen in 
// class okclnt_t.  That is, whoever inherits from this class can
// specify the amount and type of parsing done in response to
// a request, by implementing the virtual parse() method.
//
class okclnt_base_t {
public:
  okclnt_base_t (ptr<ahttpcon> xx, oksrvc_t *o, u_int to = 0) :
    x (xx), 
    oksrvc (o),
    process_flag (false), 
    uid_set (false), 
    rsp_gzip (true),
    output_state (ALL_AT_ONCE),
    _timeout (to)
  {}

  typedef enum { ALL_AT_ONCE = 0, 
		 STREAMING_HDRS = 1, // piece-meal
		 STREAMING_BODY = 2,
		 DONE = 3,
		 CLIENT_EOF = 4 } output_state_t;

  virtual ~okclnt_base_t ();
  virtual void serve () { serve_T (); }
  virtual void error (int n, const str &s = NULL);
  virtual void process () = 0;
  virtual void output (compressible_t &b);
  virtual void output (compressible_t *b);
  virtual void redirect (const str &s, int status = HTTP_MOVEDPERM);
  virtual void send (ptr<http_response_t> rsp, cbv::ptr cb);
  virtual cookie_t *add_cookie (const str &h = NULL, const str &p = "/");
  void set_uid (u_int64_t i) { uid = i; uid_set = true; }

  // stuff for piecemeal output
  bool output_hdr (ssize_t sz = -1);
  bool output_fragment (str s);
  bool output_done ();

  //
  // set these for different HTTP response configurations;
  // should of course have more of them.
  //
  void set_content_type (const str &s) { contenttype = s; }
  void set_cache_control (const str &s) { cachecontrol = s; }
  void set_expires (const str &s) { expires = s; }
  void set_content_disposition (const str &s) { contdisp = s; }
  void disable_gzip () { rsp_gzip = false; }

  void set_hdr_field (const str &k, const str &v);
  ptr<pub2::remote_publisher_t> pub2 () ;

  list_entry<okclnt_base_t> lnk;
private:
  void serve_T (CLOSURE);

protected:
  virtual void delcb () { delete this; }
  void set_attributes (http_resp_attributes_t *hra);
  bool do_gzip () const;

  virtual void parse (cbi status) = 0;
  virtual http_inhdr_t *hdr_p () = 0;
  virtual const http_inhdr_t &hdr_cr () const = 0;
		
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

  str contenttype, cachecontrol, expires, contdisp;
  bool rsp_gzip;
  ptr<vec<http_hdr_field_t> > hdr_fields;

  output_state_t output_state;
  u_int _timeout;
};

// 
// This is the standard okclnt_t, used for parsing regular HTTP requests,
// with x-URL-encoded GET, POST of mutlipart form data.
//
class okclnt_t : public okclnt_base_t, 
		 public http_parser_cgi_t 
{ 
public:
  okclnt_t (ptr<ahttpcon> xx, oksrvc_t *o, u_int to = 0) : 
    okclnt_base_t (xx, o),
    http_parser_cgi_t (xx, to) {}

  void parse (cbi cb) { http_parser_cgi_t::parse (cb); }
  http_inhdr_t *hdr_p () { return http_parser_cgi_t::hdr_p (); }
  const http_inhdr_t &hdr_cr () const { return http_parser_cgi_t::hdr_cr (); }
};

typedef callback<okclnt_base_t *, ptr<ahttpcon>, oksrvc_t *>::ref nclntcb_t;

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
    : nclients (0), sdflag (false), pid (getpid ()), n_fd_out (0), n_reqs (0),
      pub1_supported (true)
  { 
    init (argc, argv);
    accept_msgs = ok_svc_accept_msgs;
  }

  typedef okclnt_base_t newclnt_t;

  virtual void launch () { launch_T (); }
  virtual newclnt_t *make_newclnt (ptr<ahttpcon> lx) = 0;
  virtual void init_publist () {}
  virtual u_int get_andmask () const { return 0xffffffff; }
  virtual u_int get_ormask () const { return 0; }
  virtual void custom_init (cbv cb) { (*cb) (); }
  virtual void custom_init0 (cbv cb) { (*cb) (); }

  virtual void post_launch_pub2 (cbb cb) { post_launch_pub2_T (cb); }

  virtual void custom1_rpc (svccb *v) { v->reject (PROC_UNAVAIL); }
  virtual void custom2_rpc (svccb *v) { v->reject (PROC_UNAVAIL); }

  virtual ~oksrvc_t () ;

  void init (int argc, char *argv[]);
  void shutdown ();
  void connect ();
  void ctldispatch (svccb *c);
  void remove (okclnt_base_t *c);
  void add (okclnt_base_t *c);
  void end_program (); 

  void add_pubfiles (const char *arr[], u_int sz, bool conf = false);
  void add_pubfiles (const char *arr[], bool conf = false);
  void add_pubfile (const str &s, bool conf = false);

  str cfg (const str &n) const ;
  template<class C> bool cfg (const str &n, C *v) const ;
  template<typename T> parr_err_t cfg (const str &n, u_int i, T *p) const;

  void pubfiles (cbb cb);
  dbcon_t *add_db (const str &host, u_int port, const rpc_program &p,
		   int32_t txa_login_rpc = -1);
  lblnc_t *add_lb (const str &i, const rpc_program &p, int port = -1);

  pval_w_t operator[] (const str &s) const;
    

  ptr<aclnt> get_okd_aclnt () { return clnt; }
  pub_rclient_t *get_rpcli () { return rpcli; }
  bool supports_pub1 () const { return pub1_supported; }

private:
  void launch_T (CLOSURE);

protected:
  void closed_fd ();
  void enable_accept_guts ();
  void disable_accept_guts ();

  void internal_reliable_shutdown (str s, int t);

  virtual void call_exit (int rc) 
	{ exit (rc); } // Python needs to override this

  void pubbed (cbb cb, ptr<pub_res_t> res);

  void launch_pub1 (cbb cb, CLOSURE);  // legacy
  void launch_dbs (cbb cb, CLOSURE);


  void newclnt (ptr<ahttpcon> lx);
  void update (svccb *sbp);
  void kill (svccb *v);
  void update_cb (svccb *sbp, ptr<pub_res_t> pr);
  void ready_call (bool rc);

  str name;
  list<okclnt_base_t, &okclnt_base_t::lnk> clients;
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
  u_int n_reqs; // total number of requests served
  bool pub1_supported;

private:
  void post_launch_pub2_T (cbb cb, CLOSURE);
};


class oksrvcw_t : public oksrvc_t { // OK Service Wrapped
public:
  oksrvcw_t (int argc, char *argv[], nclntcb_t c) : 
    oksrvc_t (argc, argv), nccb (c) {}
  newclnt_t *make_newclnt (ptr<ahttpcon> lx) { return (*nccb) (lx, this); }
private:
  nclntcb_t nccb;
}; 

/**
 * Service-Specific error messages
 */
#define SVC_MSG(M,x)                           \
do {                                           \
  strbuf b;                                    \
  b << "pid " << pid << ": " << x << "\n";     \
  okdbg_warn (M, b);                           \
} while (0)                                    \

#define SVC_ERROR(x) SVC_MSG(ERROR, x)
#define SVC_CHATTER(x) SVC_MSG(CHATTER,x)
#define SVC_FATAL_ERROR(x) SVC_MSG(FATAL_ERROR,x)


template<typename T> parr_err_t 
oksrvc_t::cfg (const str &n, u_int i, T *p) const
{
  pval_t *v;
  const parr_ival_t *arr;
  if (!supports_pub1 ()) {
    SVC_ERROR ("Cannot call oksrvc_t::cfg() without Pub v1 support.\n");
    return PARR_NOT_FOUND;
  }
  if (!rpcli->cfg (n, &v))
    return PARR_NOT_FOUND;
  if (!(arr = v->to_int_arr ()))
    return PARR_BAD_TYPE;
  return arr->val (i, p);
}

template<class C> bool 
oksrvc_t::cfg (const str &n, C *v) const 
{ 
  if (!supports_pub1 ()) {
    SVC_ERROR ("Cannot call cfg() without Pub v1 support.");
    return sNULL;
  }
  return rpcli->cfg (n, v);
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

  
#define CH_MSG(M,x)                            \
do {                                           \
  strbuf b;                                    \
  b << servpath << ":" << pid << ": " << x ;   \
  okdbg_warn (M, b);                           \
} while (0)

#define CH_CHATTER(x) CH_MSG(CHATTER, x)
#define CH_ERROR(x)    CH_MSG(ERROR, x)



/**
 * get_okws_config ()
 * 
 * look for either okws_config or okd_config in the given etc directories,
 * or call fatal if not.
 */
str get_okws_config ();

#endif /* _LIBAOK_OKBASE_H */
