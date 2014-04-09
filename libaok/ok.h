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
#include "tame.h"
#include "pub3.h"
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
	       OKC_STATE_BADPORTS = 11,
	       OKC_STATE_KILLED = 12 } okc_state_t;


//-----------------------------------------------------------------------

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

bool is_internal(const ptr<const ahttpcon>&);

//-----------------------------------------------------------------------

class demux_data_t {
private:
  static struct timespec timespec_null;

public:

  demux_data_t (okws1_port_t p, bool s, const str &i, 
		const struct timespec &b, 
		const struct timespec &f)
    : _port (p), 
      _ssl (s), 
      _ssl_info (i), 
      _born_on (b), 
      _forwarded_on (f) {}

  demux_data_t (okws1_port_t p, bool s, const str &i)
    : _port (p), 
      _ssl (s), 
      _ssl_info (i), 
      _born_on (timespec_null), 
      _forwarded_on (timespec_null) {}

  demux_data_t (okws1_port_t p, const ssl_ctx_t *ssl)
    : _port (p), 
      _ssl (ssl ? true : false),
      _born_on (sfs_get_tsnow ()),
      _forwarded_on (timespec_null) 
  {
    if (ssl) 
      _ssl_info = ssl->cipher;
  }


  static ptr<demux_data_t> alloc (const okctl_sendcon_arg2_t &x);
  void to_xdr (okctl_sendcon_arg2_t *x);

  okws1_port_t port () const { return _port; }
  bool ssl () const { return _ssl; }
  str ssl_info () const { return _ssl_info; }
  const struct timespec &born_on () const { return _born_on; }
  const struct timespec &forwarded_on () const { return _forwarded_on; }

  void set_forward_time ();

private:

  okws1_port_t _port;
  bool _ssl;
  str _ssl_info;
  const struct timespec _born_on;
  struct timespec _forwarded_on;
};

//-----------------------------------------------------------------------

template<class A>
class ahttpcon_wrapper_t {
public:
  ahttpcon_wrapper_t (ptr<A> c, ptr<demux_data_t> d = NULL) 
    : _con (c), _demux_data (d) {}
  ahttpcon_wrapper_t (ptr<A> c, const okctl_sendcon_arg2_t &x)
    : _con (c), _demux_data (demux_data_t::alloc (x)) {}
  ahttpcon_wrapper_t () {}
  ptr<A> con () { return _con; }
  ptr<const A> con () const { return _con; }
  ptr<demux_data_t> demux_data () { return _demux_data; }
  ptr<const demux_data_t> demux_data () const { return _demux_data; }

  void
  to_xdr (okctl_sendcon_arg2_t *x)
  {
    sockaddr_in *sin = _con->get_sin ();
    x->sin.setsize (sizeof (*sin));
    memcpy (x->sin.base (), (void *)sin, sizeof (*sin));
    _demux_data->to_xdr (x);
    x->reqno = _con->get_reqno ();
    _con->collect_scraps (x->scraps);
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
  int close () ;
  void disable_accept ();
  void report ();
  void enable_accept (ok_con_acceptor_t *s, int listenq);
  bool parse (const str &in);
  void prepare ();

  int _port;
  int _fd;
  bool _listening;
};

//-----------------------------------------------------------------------

struct ok_direct_ports_t {
public:
  ok_direct_ports_t () {}
  void init (const vec<int> &p);
  void add_port (int p);
  bool bind (const str &prog, u_int32_t listenaddr);
  str encode_as_str () const;
  void close ();
  void report ();
  bool parse (const str &s);
  void enable_accept (ok_con_acceptor_t *s, int listenq);
  void disable_accept ();
  bool operator[] (int p) const { return _map[p]; }
  void add_port_pair (const ok_portpair_t &p);
  void prepare ();

  // If child A and B both launch at around the same time, B might
  // inherit A's direct ports.  Work around that...
  void do_close_ports (str s);
private:
  vec<ok_portpair_t> _ports;
  bhash<int> _map;
  vec<ok_portpair_t> _close_me;
  bhash<int> _fds;
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
      logd (NULL), _log_fd (lfd), _pub_fd (pfd),
      bind_addr_set (false),
      _ssl_primary_port (ok_ssl_port)
      //jaildir_run (ok_jaildir_run) 
  {}

  bool got_generic_exec (vec<str> &s, str loc, bool *errp, ptr<env_argv_t> *ep);

  bool is_direct_port (int i) const { return _direct_ports[i]; }
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

  int _log_fd;
  int _pub_fd;
  //str jaildir_run;  // nested jaildir for okd and services
  bool bind_addr_set; // called after got_bindaddr;
  okws1_port_t _ssl_primary_port;
};

//-----------------------------------------------------------------------

class okclnt_interface_t;

//-----------------------------------------------------------------------

class ok_httpsrv_t : public ok_con_t, public ok_base_t { 
public:
  ok_httpsrv_t (const str &h = NULL, int fd = -1, int pub_fd = -1) 
    : ok_con_t (), ok_base_t (h, fd, pub_fd), svclog (true),
      accept_enabled (false), accept_msgs (true),
      clock_mode (SFS_CLOCK_GETTIME),
      mmc_file (ok_mmc_file) {}

  typedef event<ptr<http_response_base_t> >::ref http_resp_ev_t;
      
  virtual ~ok_httpsrv_t () { }
  virtual void add_pubfile (const str &s, bool conf = false) {}

  virtual void error (ref<ahttpcon> x, int n, str s = NULL, evv_t::ptr e = NULL,
		      http_inhdr_t *h = NULL, okclnt_interface_t *cli = NULL)
  { error_T (x, n, s, e, h, cli); }

  virtual str servinfo () const;
  
  virtual void geterr (int n, str s, htpv_t v, bool gz, http_resp_ev_t ev)
  { geterr_T (n, s, v, gz, ev); }

  virtual void 
  geterr (str s, const http_resp_attributes_t &hra, http_resp_ev_t ev) 
  { geterr2_T (s, hra, ev); }

  virtual void log (ref<ahttpcon> x, http_inhdr_t *req, 
		    http_response_base_t *res,
		    const str &s = NULL)
    const { if (svclog && logd) logd->log (x, req, res, s); }

  void enable_accept ();
  void disable_accept ();
  void malloc_init (); // init malloc.3

  // toggle clock modes for SFS
  void init_sfs_clock (const str &f); 


  // can overide this on a service-by-service basis
  virtual bool init_pub (u_int opts = 0);
  virtual void launch_pub (evb_t ev, CLOSURE);
  virtual void post_launch_pub (evb_t ev) { ev->trigger (true); }

  virtual ptr<pub3::remote_publisher_t> pub3 () { return _pub3; }
  virtual ptr<const pub3::remote_publisher_t> pub3 () const { return _pub3; }

private:
  void geterr_T (int n, str s, htpv_t v, bool gz, http_resp_ev_t ev, CLOSURE);
  void geterr2_T (str s, const http_resp_attributes_t &hra, 
		 http_resp_ev_t ev, CLOSURE);

  void error_T (ref<ahttpcon> x, int n, str s = NULL, evv_t::ptr c = NULL,
		http_inhdr_t *h = NULL, okclnt_interface_t *cli = NULL, 
		CLOSURE);

protected:

  virtual void enable_accept_guts () = 0;
  virtual void disable_accept_guts () = 0;

  qhash<int,str> _errdocs;
  xpub_errdoc_set_t _errdocs_x;
  mutable str si;
  str logfmt;
  bool svclog;
  bool accept_enabled;
  bool accept_msgs;

  // stuff for dealing with okws's clock mode
  sfs_clock_t clock_mode;
  str mmc_file;

  ptr<pub3::remote_publisher_t> _pub3;

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
  virtual void fixup_log (ptr<http_response_base_t> rsp) {}
  list_entry<okclnt_interface_t> lnk;

  virtual oksrvc_t *get_oksrvc () { return oksrvc; }
  virtual const oksrvc_t *get_oksrvc () const { return oksrvc; }

protected:
  oksrvc_t *oksrvc;
};

//-----------------------------------------------------------------------

void 
browser_specific_fixups (const http_inhdr_t &in, http_resp_attributes_t *out);

//
// OKRRP = OK Request/Response Pair
//
class okrrp_interface_t {
public:
  okrrp_interface_t () {}
  virtual ~okrrp_interface_t () {}

  // manipulate output parameters
  virtual void set_custom_log2 (const str &log) = 0;
  virtual void disable_gzip () = 0;
  virtual void set_expires (const str &s) = 0;
  virtual void set_hdr_field (const str &k, const str &v) = 0;
  virtual void set_cache_control (const str &s) = 0;
  virtual void set_content_type (const str &s) = 0;

  virtual void set_log_fixup_cb (cbv::ptr ev) {}

  // access input parameters
  virtual const http_inhdr_t &hdr_cr () const = 0;
  virtual http_inhdr_t *hdr_p () = 0;

  // output paths...
  virtual void okreply (ptr<compressible_t> c, evv_t::ptr ev) = 0;
  virtual void redirect (const str &s, int status, evv_t::ptr ev) = 0;
  virtual void error (int n, const str &s, evv_t::ptr ev) = 0;

  virtual ptr<demux_data_t> demux_data () = 0;
  virtual ptr<const demux_data_t> demux_data () const = 0;

};

//-----------------------------------------------------------------------

class outcookie_holder_t {
public:
  outcookie_holder_t () {}
  ~outcookie_holder_t () {}
  vec<ptr<cookie_t> > *get_outcookies () { return &_oc; }
  const vec<ptr<cookie_t> > *get_outcookies () const { return &_oc; }
  void fixup_cookies (ptr<http_response_base_t> resp);
  ptr<cookie_t> add_cookie (const str &h = NULL, const str &p = "/");
public:
  vec<ptr<cookie_t> > _oc;
  
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
    public outcookie_holder_t, 
    public virtual okrrp_interface_t {
public:
  okclnt_base_t (ptr<ahttpcon> xx, oksrvc_t *o, u_int to = 0) :
    okclnt_interface_t (o),
    _client_con (xx), 
    process_flag (false), 
    uid_set (false), 
    rsp_gzip (true),
    output_state (ALL_AT_ONCE),
    _timeout (to),
    _status (HTTP_OK)
  {}

  typedef enum { ALL_AT_ONCE = 0, 
		 STREAMING_HDRS = 1, // piece-meal
		 STREAMING_BODY = 2,
		 DONE = 3,
		 CLIENT_EOF = 4 } output_state_t;

  virtual ~okclnt_base_t ();
  virtual void serve () { serve_T (); }

  //-----------------------------------------------------------------------

  void error (int n, const str &s, bool do_send_complete, evv_t::ptr ev)
  { error_T (n, s, do_send_complete, ev); }
  void error (int n, const str &s, evv_t::ptr ev = NULL);
  void error (int n);

  //-----------------------------------------------------------------------

  virtual void process () = 0;
  virtual bool pre_process () { return true; }

  //-----------------------------------------------------------------------

  virtual void output (compressible_t *b, evv_t::ptr ev = NULL);
  void okreply (ptr<compressible_t> p, evv_t::ptr ev);
  virtual void output (compressible_t &b, evv_t::ptr ev = NULL);

  //-----------------------------------------------------------------------

  void redirect (const str &s);
  virtual void redirect (const str &s, int status, evv_t::ptr ev)
  { redirect_T (s, status, ev); }

  //-----------------------------------------------------------------------

  virtual void send_complete () { delete this; }
  virtual void serve_complete () {}

  //-----------------------------------------------------------------------

  // okclnt2_t and others might do something useful here to set
  // keepalive headers.
  virtual void set_keepalive_attributes (http_resp_attributes_t *hra) {}

  //-----------------------------------------------------------------------

  ptr<demux_data_t> demux_data () { return _demux_data; }
  ptr<const demux_data_t> demux_data () const { return _demux_data; }

  //-----------------------------------------------------------------------

  virtual void send (ptr<http_response_t> rsp, cbv::ptr cb);
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
  void output_file (const char *fn, evb_t::ptr cb = NULL, 
		    ptr<pub3::dict_t> a = NULL,
		    pub3::opts_t opt = 0, CLOSURE);

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

  void set_demux_data(ptr<demux_data_t> d) { _demux_data = d; }

  virtual bool is_ssl() const;

  str ssl_cipher() const;
  int get_reqno() const;

  virtual ptr<pub3::ok_iface_t> pub3 () ;
  virtual ptr<pub3::ok_iface_t> pub3_local ();
  void set_localizer (ptr<const pub3::localizer_t> l);

  ptr<ahttpcon> client_con () { return _client_con; }
  ptr<const ahttpcon> client_con () const { return _client_con; }

  // The following 2 ought be protected, but are not to handle
  // tame warts.
  virtual gzip_mode_t do_gzip (const compressible_t *b) const;

  void fixup_log (ptr<http_response_base_t> rsp);

  //-----------------------------------------------------------------------

  // get/set the HTTP status
  void set_status (int s) { _status = s; }
  int get_status () const { return _status; }

private:
  void serve_T (CLOSURE);
  void output_fragment_T (str s, CLOSURE);
  void error_T (int n, const str &s, bool complete, evv_t::ptr ev, CLOSURE);
  void output_T (compressible_t *b, evv_t::ptr ev, CLOSURE);
  void redirect_T (const str &s, int status, evv_t::ptr ev, CLOSURE);

  ref<ahttpcon> _client_con;

protected:
  void set_attributes (http_resp_attributes_t *hra);

  virtual void parse (cbi cb) = 0;
  bool output_frag_prepare ();
  
  cbv::ptr cb;

  zbuf out;
  ptr<http_response_t> rsp;
  bool process_flag;
  u_int64_t uid; // hacked in for now;
  bool uid_set;

  str contenttype, cachecontrol, expires, contdisp;
  bool rsp_gzip;
  ptr<vec<http_hdr_field_t> > hdr_fields;

  output_state_t output_state;
  u_int _timeout;
  ptr<pub3::ok_iface_t> _p3_locale;
  ptr<demux_data_t> _demux_data;
  str _custom_log2;
  int _status;
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

  // for clients that are stuck with okclnt_t, but would like to
  // try the reduced-copy output path, try output2. output2 obligates
  // the caller to 'delete this' and to to provide a refcounted 
  // compressible reply object.
  void output2 (ptr<compressible_t> c, evv_t ev, CLOSURE);

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

  bool is_ssl() const override;
  uint32_t get_ip() const;
  str get_ip_str() const;

  void serve() { serve_T(); }
  void process() {}
  virtual void process(proc_ev_t ev) = 0;
  void send_complete() {}
  void serve_complete() { delete this; }

  // okclnt2_t allows use of keepalive connections, but only
  // if this flag is toggled to true...
  virtual bool do_keepalive () { return false; }
  virtual void set_keepalive_attributes (http_resp_attributes_t *hra);
private:
  void serve_T (CLOSURE);
};

//-----------------------------------------------------------------------

class dbcon_t : public helper_inet_t {
public:
  dbcon_t (const rpc_program &g, const str &h, u_int p)
    : helper_inet_t (g, h, p, 0) {}
  dbcon_t (const rpc_program &g, const str &h, u_int p, u_int o)
    : helper_inet_t (g, h, p, o) {}

  str getname () const 
  { return strbuf ("database: ") << helper_inet_t::getname (); }
};

template<> struct hashfn<dbcon_t*> {
    hashfn () {}
    hash_t operator() (dbcon_t *const c) const {
        size_t i = (size_t)c;
        return (i >> 32) ^ (i & 0xFFFFFFFF);
    }
};

template<> struct equals<dbcon_t*> {
    equals() {}
    bool operator() (dbcon_t *const a, dbcon_t *const b) const
    {
        return a == b;
    }
};

//-----------------------------------------------------------------------

class oksrvc_t : public ok_httpsrv_t, public ok_con_acceptor_t  { // OK Service
public:
  oksrvc_t (int argc, char *argv[]);

  typedef okclnt_interface_t newclnt_t;

  virtual void launch () { launch_T (); }
  virtual newclnt_t *make_newclnt (ptr<ahttpcon> lx) = 0;

  // Initialized the pub3 runtime library; by default, it's 
  // the standard librfn library, but you can add your own,
  // or disable as you see fit.
  virtual void init_pub3_runtime ();

  virtual u_int get_andmask () const { return 0xffffffff; }
  virtual u_int get_ormask () const { return 0; }
  virtual void custom_init (cbv cb) { (*cb) (); }
  virtual void custom_init0 (cbv cb) { (*cb) (); }
  virtual void init_constants () {}
  virtual void log_connection_crashed ();

  virtual void post_launch_pub (evb_t ev) { post_launch_pub_T (ev); }
  void post_launch_pub_T (evb_t ev, CLOSURE);

  virtual void custom1_rpc (svccb *v) { v->reject (PROC_UNAVAIL); }
  virtual void custom2_rpc (svccb *v) { v->reject (PROC_UNAVAIL); }

  // Your service can handle this RPC any which way it pleases.
  // A NULL string means "OK", and everything else means
  // an error (self-describing of course).
  virtual str custom_handle_send_msg (str s);

  virtual ~oksrvc_t () ;

  virtual bool use_union_cgi () const { return false; }

  void init (int argc, char *argv[]);
  void shutdown (bool end_of_okws_run, CLOSURE);
  void connect ();
  void ctldispatch (svccb *c);
  void remove (okclnt_interface_t *c);
  void add (okclnt_interface_t *c);
  void end_program (); 

  dbcon_t *add_db (const str &host, u_int port, const rpc_program &p);
  ptr<aclnt> get_okd_aclnt () { return clnt; }

  // for accept direct connections (not via okd)
  void accept_new_con (ok_portpair_t *p);

  void keepalive (ahttpcon_wrapper_t<ahttpcon> x, CLOSURE);

private:
  void launch_T (CLOSURE);

protected:
  void closed_fd ();
  void enable_accept_guts ();
  void disable_accept_guts ();
  void enable_direct_ports ();
  void disable_direct_ports ();
  void enable_coredumps ();

  void internal_reliable_shutdown (str s, int t)
  { internal_reliable_shutdown_T (s, t); }
  void internal_reliable_shutdown_T (str s, int t, CLOSURE);

  // Subclasses can implement this hook to do some work before
  // the shutdown sequence actually starts.
  virtual void pre_shutdown_hook (evv_t ev) { ev->trigger (); }

  virtual void call_exit (int rc) 
	{ exit (rc); } // Python needs to override this

  void launch_dbs (evb_t ev, CLOSURE);

  void handle_new_con2 (svccb *sbp);
  void handle_get_stats (svccb *v);
  void handle_send_msg (svccb *sbp);
  void handle_diagnostic (svccb *sbp);
  bool newclnt (ahttpcon_wrapper_t<ahttpcon> acw);
  void kill (svccb *v);
  void ready_call (bool rc);

  okctl_sendcon_res_t
  handle_new_con_common (const okclnt_sin_t &sin_in, ptr<ahttpcon> *x_out);

  // debug initialization procedure
  void debug_launch (evv_t ev, CLOSURE);

  str name;
  list<okclnt_interface_t, &okclnt_interface_t::lnk> clients;

  u_int nclients;
  bool sdflag;

  vec<helper_base_t *> dbs;
  bool dbstatus;
  u_int dbl;
  u_int lnum;
  int pid;

  int n_fd_out;
  u_int n_reqs; // total number of requests served
  bool wait_for_signal_in_startup;
  int _n_newcli;
  size_t _brother_id;
  size_t _n_children;
  bool _aggressive_svc_restart;
  bool _die_on_logd_crash;

private:
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

//
// XXX - hack - this is used by both okch_t and okld_ch_t - just happens
// that they have similar internal variables; the might be put into a 
// class tree, but they share little functionality in common.
//
#define CH_MSG(M,x)							\
  do {									\
    strbuf b;								\
    b << _servpath << ":" << _brother_id << ":" << _pid << ": " << x ;	\
    okdbg_warn (M, b);							\
  } while (0)

#define CH_CHATTER(x) CH_MSG(CHATTER, x)
#define CH_ERROR(x)   CH_MSG(ERROR, x)


//
// XXX - hack - this is used by both okch_t and okld_ch_t - just happens
// that they have similar internal variables; the might be put into a 
// class tree, but they share little functionality in common.
//
#define CH_CL_MSG(M,x)							\
  do {									\
    strbuf b;								\
    b << _servpath << ":" x ;						\
    okdbg_warn (M, b);							\
  } while (0)

#define CH_CL_CHATTER(x) CH_CL_MSG(CHATTER, x)
#define CH_CL_ERROR(x)   CH_CL_MSG(ERROR, x)

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

void timespec_to_xdr (const struct timespec &ts, okctl_timespec_t *x);
void xdr_to_timespec (const okctl_timespec_t &x, struct timespec *ts);
bool populate_keepalive_data (keepalive_data_t *d, 
			      const okctl_sendcon_arg2_t &a);

//-----------------------------------------------------------------------

#endif /* _LIBAOK_OKBASE_H */
