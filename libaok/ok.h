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
#include "oklocale.h"

//-----------------------------------------------------------------------

typedef enum { OKC_STATE_NONE = 0,
	       OKC_STATE_LAUNCH = 1,
	       OKC_STATE_SERVE = 2,
	       OKC_STATE_HOSED = 3,
	       OKC_STATE_CRASH = 4,
	       OKC_STATE_DELAY = 5,
	       OKC_STATE_LAUNCH_SEQ_1 = 6,
	       OKC_STATE_LAUNCH_SEQ_2 = 7,
               OKC_STATE_KILLING = 8,
	       OKC_STATE_TOOBUSY = 9,
	       OKC_STATE_STANDBY = 10,
		   OKC_STATE_BADPORTS = 11 } okc_state_t;


//-----------------------------------------------------------------------

struct errdoc_t {
  errdoc_t (int n, const str &f) : status (n), fn (f) {}
  int status;
  str fn;
  ihash_entry<errdoc_t> lnk;
};

//-----------------------------------------------------------------------

typedef u_int16_t okws1_port_t;
#define PORT_MAX USHRT_MAX

typedef event<ok_xstatus_typ_t>::ref okstat_ev_t;

//-----------------------------------------------------------------------

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

//-----------------------------------------------------------------------

class config_parser_t {
public:
  config_parser_t () {}
  virtual ~config_parser_t () {}
  bool run_configs (const str &fn);
  void include (vec<str> s, str loc, bool *errp);
protected:
  bool do_file (const str &fn);
  virtual bool parse_file (const str &fn) = 0;
  virtual bool post_config (const str &fn) { return true; }
private:
  bhash<str> _seen; // files seen
};

//-----------------------------------------------------------------------

class demux_data_t {
public:

  demux_data_t (okws1_port_t p, bool s, const str &i)
    : _port (p), _ssl (s), _ssl_info (i) {}

  demux_data_t (okws1_port_t p, const ssl_ctx_t *ssl)
    : _port (p), 
      _ssl (ssl ? true : false)
  {
    if (ssl) 
      _ssl_info = ssl->cipher;
  }

  static ptr<demux_data_t> alloc (const okctl_sendcon_arg_t &x);
  void to_xdr (okctl_sendcon_arg_t *x);

  okws1_port_t port () const { return _port; }
  bool ssl () const { return _ssl; }
  str ssl_info () const { return _ssl_info; }

private:

  okws1_port_t _port;
  bool _ssl;
  str _ssl_info;
};

//-----------------------------------------------------------------------

template<class A>
class ahttpcon_wrapper_t {
public:
  ahttpcon_wrapper_t (ptr<A> c, ptr<demux_data_t> d = NULL) 
    : _con (c), _demux_data (d) {}
  ahttpcon_wrapper_t (ptr<A> c, const okctl_sendcon_arg_t &x)
    : _con (c), _demux_data (demux_data_t::alloc (x)) {}
  ptr<A> con () { return _con; }
  ptr<const A> con () const { return _con; }
  ptr<demux_data_t> demux_data () { return _demux_data; }
  ptr<const demux_data_t> demux_data () const { return _demux_data; }

  void
  to_xdr (okctl_sendcon_arg_t *x)
  {
    sockaddr_in *sin = _con->get_sin ();
    x->sin.setsize (sizeof (*sin));
    memcpy (x->sin.base (), (void *)sin, sizeof (*sin));
    _demux_data->to_xdr (x);
  }

private:
  ptr<A> _con;
  ptr<demux_data_t> _demux_data;
};


//-----------------------------------------------------------------------

class ok_portpair_t;

class ok_con_acceptor_t {
public:
  ok_con_acceptor_t () {}
  virtual ~ok_con_acceptor_t () {}
  virtual void accept_new_con (ok_portpair_t *p) = 0 ;
};

//-----------------------------------------------------------------------

struct ok_portpair_t {
  ok_portpair_t (int port = -1, int fd = -1) 
    : _port (port), _fd (fd), _listening (false) {}
  str encode_as_str () const;
  void close () ;
  void disable_accept ();
  void report ();
  void enable_accept (ok_con_acceptor_t *s, int listenq);
  bool parse (const str &in);

  int _port;
  int _fd;
  bool _listening;
};

//-----------------------------------------------------------------------

struct ok_direct_ports_t {
public:
  ok_direct_ports_t () {}
  void init (const vec<int> &p);
  bool bind (const str &prog, u_int32_t listenaddr);
  str encode_as_str () const;
  void close ();
  void report ();
  bool parse (const str &s);
  void enable_accept (ok_con_acceptor_t *s, int listenq);
  void disable_accept ();
private:
  vec<ok_portpair_t> _ports;
};

//-----------------------------------------------------------------------

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
      bind_addr_set (false),
      _ssl_primary_port (ok_ssl_port)
      //jaildir_run (ok_jaildir_run) 
  {}

  bool got_generic_exec (vec<str> &s, str loc, bool *errp, ptr<argv_t> *ep);

  log_t *get_logd () { return logd; }
  void got_bindaddr (vec<str> s, str loc, bool *errp);
  void got_ports (bool ssl, vec<str> s, str loc, bool *errp);
  void add_port (okws1_port_t p, bool ssl);
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

  vec<okws1_port_t> _https_ports, _http_ports;
  qhash<okws1_port_t, bool> _all_ports_map;

  ok_direct_ports_t _direct_ports;

protected:
  str fix_uri (const str &in) const;

  log_t *logd;

  int logfd;
  int pub2fd;
  //str jaildir_run;  // nested jaildir for okd and services
  bool bind_addr_set; // called after got_bindaddr;
  okws1_port_t _ssl_primary_port;
};

//-----------------------------------------------------------------------

class okclnt_interface_t;

//-----------------------------------------------------------------------

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
		      http_inhdr_t *h = NULL, okclnt_interface_t *cli = NULL)
  { error_T (x, n, s, c, h, cli); }

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
  virtual ptr<const pub2::remote_publisher_t> pub2 () const { return _pub2; }

private:
  void geterr_T (int n, str s, htpv_t v, bool gz, http_resp_cb_t cb, CLOSURE);

  void error_T (ref<ahttpcon> x, int n, str s = NULL, cbv::ptr c = NULL,
		http_inhdr_t *h = NULL, okclnt_interface_t *cli = NULL, 
		CLOSURE);

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

//-----------------------------------------------------------------------

class okclnt_interface_t {
public:
  okclnt_interface_t (oksrvc_t *o);
  virtual ~okclnt_interface_t ();
  virtual void set_union_cgi_mode (bool b) = 0;
  virtual void set_demux_data (ptr<demux_data_t> d) = 0;
  virtual void serve () = 0;
  virtual void fixup_response (ptr<http_response_t> rsp) {}
  list_entry<okclnt_interface_t> lnk;

  virtual oksrvc_t *get_oksrvc () { return oksrvc; }
  virtual const oksrvc_t *get_oksrvc () const { return oksrvc; }

protected:
  oksrvc_t *oksrvc;
};

//-----------------------------------------------------------------------

class okresp_interface_t {
public:
  okresp_interface_t () {}
  virtual ~okresp_interface_t () {}

  virtual void set_custom_log2 (const str &log) = 0;
  virtual void disable_gzip () = 0;
  virtual void set_expires (const str &s) = 0;
  virtual void set_hdr_field (const str &k, const str &v) = 0;
  virtual void set_cache_control (const str &s) = 0;
  virtual void set_content_type (const str &s) = 0;
};

//-----------------------------------------------------------------------

typedef callback<okclnt_interface_t *, ptr<ahttpcon>, oksrvc_t *>::ref 
nclntcb_t;

//-----------------------------------------------------------------------

//
// There should be one okclnt_base_t per external HTTP request.
// We've split the logic up between stuff that goes mainly here,
// and the x-URL-encoded-specific parsing stuff, as seen in 
// class okclnt_t.  That is, whoever inherits from this class can
// specify the amount and type of parsing done in response to
// a request, by implementing the virtual parse() method.
//
class okclnt_base_t 
  : public okclnt_interface_t, 
    public okresp_interface_t {
public:
  okclnt_base_t (ptr<ahttpcon> xx, oksrvc_t *o, u_int to = 0) :
    okclnt_interface_t (o),
    _client_con (xx), 
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

  virtual void error (int n, const str &s = NULL, 
		      bool do_send_complete = true, evv_t::ptr ev = NULL)
  { error_T (n, s, do_send_complete, ev); }

  virtual void process () = 0;
  virtual bool pre_process () { return true; }
  virtual void output (compressible_t &b, evv_t::ptr ev = NULL);
  virtual void output (compressible_t *b, evv_t::ptr ev = NULL);

  virtual void redirect (const str &s, int status = -1,
			 evv_t::ptr ev = NULL, CLOSURE);

  virtual void send_complete () { delete this; }
  virtual void serve_complete () {}

  virtual void send (ptr<http_response_t> rsp, cbv::ptr cb);
  virtual cookie_t *add_cookie (const str &h = NULL, const str &p = "/");
  void set_uid (u_int64_t i) { uid = i; uid_set = true; }

  virtual bool ssl_only () const { return false; } 
  virtual str  ssl_redirect_str () const { return NULL; }

  // Kludge; this won't do anything except for subclasses that actually
  // use CGI.
  virtual void set_union_cgi_mode (bool b) {}

  // stuff for piecemeal output
  bool output_hdr (ssize_t sz = -1);
  bool output_fragment (str s);
  bool output_fragment (compressible_t &b, cbv::ptr done = NULL);
  void output_file (const char *fn, cbb::ptr cb = NULL, aarr_t *a = NULL,
		    u_int opt = 0, penv_t *e = NULL, CLOSURE);

  void output_done (evb_t::ptr ev, CLOSURE);

  //
  // set these for different HTTP response configurations;
  // should of course have more of them.
  //
  void set_content_type (const str &s) { contenttype = s; }
  void set_cache_control (const str &s) { cachecontrol = s; }
  void set_expires (const str &s) { expires = s; }
  void set_content_disposition (const str &s) { contdisp = s; }
  void disable_gzip () { rsp_gzip = false; }
  void set_custom_log2 (const str &s) { _custom_log2 = s; }

  void set_hdr_field (const str &k, const str &v);

  void set_demux_data (ptr<demux_data_t> d) { _demux_data = d; }
  bool is_ssl () const { return _demux_data && _demux_data->ssl (); }
  str ssl_cipher () const;

  virtual ptr<pub2::ok_iface_t> pub2 () ;
  virtual ptr<pub2::ok_iface_t> pub2_local ();
  void set_localizer (ptr<const pub_localizer_t> l);

  ptr<ahttpcon> client_con () { return _client_con; }
  ptr<const ahttpcon> client_con () const { return _client_con; }

  // The following 2 ought be protected, but are not to handle
  // tame warts.
  virtual const http_inhdr_t &hdr_cr () const = 0;
  bool do_gzip () const;

  void fixup_response (ptr<http_response_t> rsp);

private:
  void serve_T (CLOSURE);
  void output_fragment_T (str s, CLOSURE);
  void error_T (int n, const str &s, bool complete, evv_t::ptr ev, CLOSURE);
  void output_T (compressible_t *b, evv_t::ptr ev, CLOSURE);

  ref<ahttpcon> _client_con;

protected:
  void set_attributes (http_resp_attributes_t *hra);

  virtual void parse (cbi cb) = 0;
  virtual http_inhdr_t *hdr_p () = 0;
  bool output_frag_prepare ();
  
  cbv::ptr cb;

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
  ptr<pub2::locale_specific_publisher_t> _p2_locale;
  ptr<demux_data_t> _demux_data;
  str _custom_log2;
};

//-----------------------------------------------------------------------

// 
// This is the standard okclnt_t, used for parsing regular HTTP requests,
// with x-URL-encoded GET, POST of multipart form data.
//
class okclnt_t : public okclnt_base_t, 
		 public http_parser_cgi_t 
{ 
public:
  okclnt_t (ptr<ahttpcon> xx, oksrvc_t *o, u_int to = 0) : 
    okclnt_base_t (xx, o),
    http_parser_cgi_t (xx, to) 
  {}

  void parse (cbi cb) { http_parser_cgi_t::parse (cb); }
  http_inhdr_t *hdr_p () { return http_parser_cgi_t::hdr_p (); }
  const http_inhdr_t &hdr_cr () const { return http_parser_cgi_t::hdr_cr (); }

  void set_union_cgi_mode (bool b)
  { http_parser_cgi_t::set_union_mode (b); }
};

//-----------------------------------------------------------------------

//
// Upgraded version of okclnt2, with a better state machine architecture
//
class okclnt2_t : public okclnt_t {
public:
  typedef event<bool, int>::ref proc_ev_t;

  okclnt2_t (ptr<ahttpcon> x, oksrvc_t *c, u_int to = 0) :
    okclnt_t (x, c, to) {}

  void serve () { serve_T (); }
  void process () {}
  virtual void process (proc_ev_t ev) = 0;
  void send_complete () {}
  void serve_complete () { delete this; }
private:
  void serve_T (CLOSURE);
};

//-----------------------------------------------------------------------

class dbcon_t : public helper_inet_t {
public:
  dbcon_t (const rpc_program &g, const str &h, u_int p)
    : helper_inet_t (g, h, p, 0) {}

  str getname () const 
  { return strbuf ("database: ") << helper_inet_t::getname (); }
};

//-----------------------------------------------------------------------

class oksrvc_t : public ok_httpsrv_t, public ok_con_acceptor_t  { // OK Service
public:
  oksrvc_t (int argc, char *argv[]) 
    : nclients (0), sdflag (false), pid (getpid ()), n_fd_out (0), n_reqs (0),
      pub1_supported (true),
      wait_for_signal_in_startup (false),
      _n_newcli (0),
      _pub1_cfg (true)
  { 
    init (argc, argv);
    accept_msgs = ok_svc_accept_msgs;
    accept_enabled = true;
  }

  typedef okclnt_interface_t newclnt_t;

  virtual void launch () { launch_T (); }
  virtual newclnt_t *make_newclnt (ptr<ahttpcon> lx) = 0;

  // if we didn't give init_publist, then we're using pub2 for
  // everything, including CFG variables.
  virtual void init_publist () { _pub1_cfg = false; }

  // Subclasses that specialize this method to true can
  // always use pub2 configuration.
  virtual bool use_pub2_cfg () const { return false; }

  virtual u_int get_andmask () const { return 0xffffffff; }
  virtual u_int get_ormask () const { return 0; }
  virtual void custom_init (cbv cb) { (*cb) (); }
  virtual void custom_init0 (cbv cb) { (*cb) (); }
  virtual void init_constants () {}

  virtual void post_launch_pub2 (cbb cb) { post_launch_pub2_T (cb); }

  virtual void custom1_rpc (svccb *v) { v->reject (PROC_UNAVAIL); }
  virtual void custom2_rpc (svccb *v) { v->reject (PROC_UNAVAIL); }

  virtual ~oksrvc_t () ;

  virtual bool use_union_cgi () const { return false; }

  void init (int argc, char *argv[]);
  void shutdown ();
  void connect ();
  void ctldispatch (svccb *c);
  void remove (okclnt_interface_t *c);
  void add (okclnt_interface_t *c);
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

  // for accept direct connections (not via okd)
  void accept_new_con (ok_portpair_t *p);

private:
  void launch_T (CLOSURE);

protected:
  void closed_fd ();
  void enable_accept_guts ();
  void disable_accept_guts ();
  void enable_direct_ports ();
  void disable_direct_ports ();
  void enable_coredumps ();

  void internal_reliable_shutdown (str s, int t);

  virtual void call_exit (int rc) 
	{ exit (rc); } // Python needs to override this

  void pubbed (cbb cb, ptr<pub_res_t> res);

  void launch_pub1 (cbb cb, CLOSURE);  // legacy
  void launch_dbs (cbb cb, CLOSURE);


  void handle_new_con (svccb *sbp);
  void handle_get_stats (svccb *v);
  bool newclnt (ahttpcon_wrapper_t<ahttpcon> acw);
  void update (svccb *sbp, CLOSURE);
  void kill (svccb *v);
  void ready_call (bool rc);

  // debug initialization procedure
  void debug_launch (cbv cb, CLOSURE);

  str name;
  list<okclnt_interface_t, &okclnt_interface_t::lnk> clients;

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
  bool wait_for_signal_in_startup;
  int _n_newcli;
  bool _pub1_cfg;

private:
  void post_launch_pub2_T (cbb cb, CLOSURE);
};

//-----------------------------------------------------------------------

str okws_exec (const str &x);
void init_syscall_stats ();
void do_syscall_stats ();

//-----------------------------------------------------------------------

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

//-----------------------------------------------------------------------

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

//-----------------------------------------------------------------------

template<class C> bool 
oksrvc_t::cfg (const str &n, C *v) const 
{ 
  if (!supports_pub1 ()) {
    SVC_ERROR ("Cannot call cfg() without Pub v1 support.");
    return sNULL;
  }
  return rpcli->cfg (n, v);
}

//-----------------------------------------------------------------------

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

#define NO_SOCKET_ALLOCATED 7

//-----------------------------------------------------------------------


/**
 * get_okws_config ()
 * 
 * look for either okws_config or okd_config in the given etc directories,
 * or call fatal if not.
 */
str get_okws_config (bool make_fatal = true);

/**
 * 
 * switch the SFS core select policy
 *
 */
void set_sfs_select_policy ();


//-----------------------------------------------------------------------

#endif /* _LIBAOK_OKBASE_H */
