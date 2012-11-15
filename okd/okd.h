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

#pragma once

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
#include "arpc.h"
#include "rxx.h"
#include "tame.h"
#include "list.h"

#define OK_LQ_SIZE_D    100
#define OK_LQ_SIZE_LL   5
#define OK_LQ_SIZE_UL   1000


typedef enum { OKD_JAILED_NEVER = 0,
	       OKD_JAILED_DEPENDS = 1,
	       OKD_JAILED_ALWAYS = 2 } okd_jailtyp_t;

class okch_t;
typedef callback<void, okch_t *>::ref cb_okch_t;

//=======================================================================

struct okd_stats_t {
  void to_strbuf (strbuf &b) const;
  time_t _uptime;
  size_t _n_req;
  size_t _n_recv;
  size_t _n_sent;
  size_t _n_tot;
};

//=======================================================================

class ok_custom2_trig_t {
public:
  ok_custom2_trig_t () :
    _custom_res (New refcounted<ok_custom_res_set_t> ()) {}

  void add_err (const str &svc, ok_xstatus_typ_t t);
  void add_succ (const str &svc, const ok_custom_data_t &dat);
  ~ok_custom2_trig_t () { _cb (); }
  ptr<ok_custom_res_set_t> get_custom_res () { return _custom_res; }
  void setcb (cbv c) { _cb = c; }
private:
  cbv::ptr _cb;
  ptr<ok_custom_res_set_t> _custom_res;
};

//=======================================================================

struct time_node_t {
  time_node_t ();
  time_t _time;
  tailq_entry<time_node_t> _lnk;
};

struct time_list_t : public tailq<time_node_t, &time_node_t::_lnk> {
  time_t oldest () const;
  time_node_t *launch ();
  void finished (time_node_t *tn);
};

//=======================================================================

class okd_t;
class okch_cluster_t;
class okch_t : public ok_con_t {  // OK Child Handle
public:
  okch_t (okd_t *o, okch_cluster_t *clust, size_t id, okc_state_t st);
  ~okch_t ();
  void launch ();
  void clone (ahttpcon_wrapper_t<ahttpcon_clone> acw, CLOSURE);
  void send_con_to_service (ahttpcon_wrapper_t<ahttpcon_clone> acw, 
			    evv_t ev, CLOSURE);
  void shutdown (oksig_t sig, evv_t ev, CLOSURE);

  void got_new_ctlx_fd (int fd, int p);
  void dispatch (ptr<bool> destroyed, svccb *b);
  void fdcon_eof (ptr<bool> destroyed);
  void kill ();
  void killed ();
  void custom1_out (const ok_custom_data_t &x, evs_t ev, CLOSURE);
  void custom2_out (const ok_custom_data_t &in, ok_custom_res_union_t *out, 
		    evs_t ev, CLOSURE);

  void chld_eof ();
  void to_status_xdr (oksvc_status_t *out);
  void to_svc_descriptor (oksvc_descriptor_t *d) const;

  inline int get_n_sent () const { return _n_sent; }
  void reset_n_sent () ;
  void reset_accounting ();
  inline int inc_n_sent () { return (_n_sent ++) ; }
  void awaken (evb_t ev, CLOSURE);
  void set_state (okc_state_t s) { _state = s; }

  void stats_collect (okd_stats_t *s, evv_t ev, CLOSURE);

  void diagnostic (ok_diagnostic_domain_t dd, ok_diagnostic_cmd_t cmd, 
		   event<ok_xstatus_typ_t>::ref ev, CLOSURE);
  ptr<bool> get_destroyed_flag () { return _destroyed; }

  void proc_to_xdr (oksvc_proc_t *x) const;
  void handle_overload ();
  bool sendcon_timeout () const;

  typedef enum {
    OK = 0,
    MIGHT_WORK = 1,
    BUSY_LOOP = 2,
    CRASHED = 3,
    STANDBY = 4
  } status_t;

  status_t get_status () const;
  void send_msg (str m, evs_t ev, CLOSURE);
  
  okd_t *_myokd;
  okch_cluster_t *_cluster;
  const size_t _brother_id;
  const str _servpath;
  int _pid;
protected:
  void handle_reenable_accept (svccb *sbp);
  void start_chld ();
private:

  okc_state_t _state;
  ptr<bool> _destroyed;
  bool _srv_disabled;
  int _per_svc_nfd_in_xit;   // per service number FD in transit
  int _n_sent;              // N sent since reboot
  time_t _last_restart;     // time when started;
  
  bool _too_busy;           // the server is potentially too busy to get more
  u_int64_t _generation_id; // which generation of service this is.
  time_t _emerg_start;      // if the service is unresponsive, when it happened
  bool _emerg_killed;       // whether it's been emergency killed
  
  // To be triggered when this service is ready to go.
  evv_t::ptr _ready_trigger; 

  time_list_t _dispatch_times;
};

//=======================================================================

class okch_cluster_t {
public:
  okch_cluster_t (okd_t *o, str p, okc_state_t st, size_t n);
  ~okch_cluster_t ();
  void insert_child (okch_t *ch);
  void clone (ahttpcon_wrapper_t<ahttpcon_clone> acw, evv_t ev, int sib, 
	      CLOSURE);
  okch_t *child (size_t i) { return _children[i]; }
  const okch_t *child (size_t i) const { return _children[i]; }
  okch_t *find_best_fit_child (ref<ahttpcon_clone> xc, okch_t::status_t *sp,
			       int pref = -1);
  void killed (size_t i, okch_t *ch);

  void set_states (okc_state_t st);
  size_t n_children () const { return _children.size (); }
  void is_ready (CLOSURE);
  size_t qlen () const { return _conqueue.size(); }
  
  okd_t *_myokd;
  const str _servpath;           // GET <servpath> HTTP/1.1 (starts with '/')
  vec<okch_t *> _children;
  ihash_entry<okch_cluster_t> _lnk;
  vec<ahttpcon_wrapper_t<ahttpcon_clone> > _conqueue;
  ptr<bool> _destroyed;
  bool _is_ready_looping;
  size_t _n_killed;
};

//=======================================================================

class okd_ssl_t {
public:
  okd_ssl_t (okd_t *o) : _okd (o), _fd (-1) {}
  ~okd_ssl_t () { hangup (); }
  bool init (int fd);
  void enable_accept () { toggle_accept (true); }
  void disable_accept () { toggle_accept (false); }
  void dispatch (svccb *sbp);
  void hangup ();
private:
  void toggle_accept (bool b, CLOSURE);
  okd_t *_okd;
  int _fd;
  ptr<axprt_unix> _x;
  ptr<aclnt> _cli;
  ptr<asrv> _srv;
};

//=======================================================================

class servtab_t : public ihash<const str, okch_cluster_t, 
			       &okch_cluster_t::_servpath, 
			       &okch_cluster_t::_lnk> 
{
public:
  servtab_t () {}
  void dump (vec<okch_t *> *out);

  okch_t *get (const oksvc_proc_t &p);
  size_t mget (const oksvc_proc_t &p, vec<okch_t *> *out);
};

//=======================================================================

class okd_t : public ok_httpsrv_t, public config_parser_t 
{
public:
  okd_t (const str &cf, int logfd_in, int okldfd_in, const str &cdd, 
	 okws1_port_t p, int pub2fd_in) : 
    ok_httpsrv_t (NULL, logfd_in, pub2fd_in),
    config_parser_t (),
    okd_usr (ok_okd_uname), okd_grp (ok_okd_gname),
    pubd (NULL), 
    configfile (cf),
    okldfd (okldfd_in),
    sdflag (false), 
    cntr (0),
    coredumpdir (cdd),
    nfd_in_xit (0),
    reqid (0),
    _startup_time (time (NULL)),
    xtab (2),
    _socket_filename (okd_mgr_socket),
    _socket_mode (okd_mgr_socket_mode),
    _accept_ready (false),
    _lazy_startup (false),
    _okd_nodelay (okd_tcp_nodelay),
    _cluster_addressing (false),
    _emerg_kill_enabled (false),
    _emerg_kill_wait (okd_emergency_kill_wait_time),
    _emerg_kill_signal (okd_emergency_kill_signal)
  {
    listenport = p;
  }


  ~okd_t ();

  void abort ();

  void insert (okch_cluster_t *c) { servtab.insert (c); }
  void remove (okch_cluster_t *c) { servtab.remove (c); }

  void got_alias (vec<str> s, str loc, bool *errp);
  void got_regex_alias (vec<str> s, str loc, bool *errp);
  void got_err_doc (vec<str> s, str loc, bool *errp);
  void got_service (vec<str> s, str loc, bool *errp);
  void got_service2 (vec<str> s, str loc, bool *errp);
  void got_script (vec<str> s, str loc, bool *errp);
  void got_ssl_port(vec<str> v, str loc, bool* errp);

  void okld_dispatch (svccb *sbp);

  // Well, no one ever said event-driven programming was pretty
  void launch (CLOSURE);

  void launch_logd (evb_t ev, CLOSURE);

  void sclone (ahttpcon_wrapper_t<ahttpcon_clone> acw, str s, int status,
	       evv_t ev, CLOSURE);
  void newserv (int fd);
  void newserv2 (int port, int nfd, sockaddr_in *sin, bool prx, 
		 const ssl_ctx_t *ssl, const keepalive_data_t *kad, CLOSURE);
  void shutdown (int sig) { shutdown_T (sig); }
  void shutdown_T (int sig, CLOSURE);
  void awaken (const oksvc_proc_t &p, evb_t ev, CLOSURE);
  bool handle_overload (time_t dur, okch_t *ch);
  void emerg_kill (oksvc_descriptor_t d, evv_t::ptr ev = NULL, CLOSURE);
  bool need_okld_rpc () const;

  servtab_t servtab;
  qhash<str, str> aliases;

  struct regex_alias_t {
    regex_alias_t (str t, const rxx &x) : _target (t), _rxx (x) {}
    const str _target;
    rxx _rxx;
  };

  vec<regex_alias_t> regex_aliases;

  ok_usr_t okd_usr; 
  ok_grp_t okd_grp;

  helper_t *pubd;

  void open_mgr_socket ();

  // functions for the OKMGR interface
  void relaunch (svccb *sbp, CLOSURE);
  void custom1_in (svccb *sbp, CLOSURE);
  void custom2_in (svccb *sbp, CLOSURE);
  void diagnostic (svccb *sbp, CLOSURE);
  void okctl_get_stats (svccb *sbp);
  void turnlog (svccb *sbp, CLOSURE);
  void strip_privileges ();
  void send_msg (svccb *sbp, CLOSURE);
  void handle_keepalive (int fd, svccb *sbp);

  bool in_shutdown () const { return sdflag; }
  void set_signals ();

  void send_errdoc_set (svccb *sbp);
  void req_errdoc_set_2 (svccb *sbp);

  typedef rendezvous_t<okch_t *,ptr<bool> > shutdown_rv_t;

protected:
  // queueing stuff
  void enable_accept_guts ();
  void disable_accept_guts ();
  bool parse_file (const str &fn);
  bool post_config (const str &fn);
  bool lazy_startup () const { return _lazy_startup; }
  void render_stat_page (ptr<ahttpcon_clone> x, CLOSURE);
  ok_xstatus_typ_t reserve_child (const oksvc_proc_t &p, bool lzy);

  /* statistics */
  void stats_collect (okd_stats_t *s, evv_t ev, CLOSURE);
  void render_stats_page (ptr<ahttpcon_clone> x, evv_t ev, CLOSURE);
  void send_stats_reply (ptr<ahttpcon> x, const okd_stats_t &stats, htpv_t v,
			 evv_t ev, CLOSURE);

private:
  void custom1_in (const ok_custom_arg_t &x, evs_t cb);

  void newmgrsrv (ptr<axprt_stream> x);
  void check_runas ();

  // shutdown functions
  void kill_srvcs (oksig_t sig, shutdown_rv_t *rv);
  void shutdown_wait (shutdown_rv_t *rv, evb_t ev, CLOSURE);
  void stop_listening ();

  void got_child_fd (int fd, const oksvc_descriptor_t &d);
  bool listen_from_ssl (int fd);
  bool is_ssl_port(okws1_port_t port);

  str configfile;
  int okldfd;
  
  bool bdlnch;
  u_int chkcnt;
  u_int sdcbcnt;
  bool sdflag;
  bool jailed;
  int cntr;

  ptr<axprt_unix> _okld_x;
  ptr<asrv> _okld_srv;
  ptr<aclnt> _okld_cli;

  str coredumpdir;
  xpub_errdoc_set_t xeds;

  int nfd_in_xit;       // number of FDs in transit
  u_int reqid;
  time_t _startup_time;
  ahttp_tab_t xtab;

  qhash<int, okws1_port_t> portmap;
  vec<int> listenfds;
  str _socket_filename;
  str _socket_group, _socket_owner;
  int _socket_mode;

  str _config_grp, _config_user;
  bool _accept_ready;
  vec<ptr<okd_ssl_t> > _ssls;
  bool _lazy_startup;
  str _stat_page_url;
  bool _okd_nodelay;
  bool _cluster_addressing;
  bool _emerg_kill_enabled;
  time_t _emerg_kill_wait;
  int _emerg_kill_signal;

  vec<okws1_port_t> _ssl_ports;
};

class okd_mgrsrv_t 
{
public:
  okd_mgrsrv_t (ptr<axprt_stream> xx, okd_t *o);
  void dispatch (svccb *b);
private:
  void repub (svccb *b, int v);
  void turnlog (svccb *b);
  ptr<asrv> srv;
  ptr<axprt_stream> x;
  okd_t *myokd;
};

class common_404_t {
public:
  common_404_t ();
  bool operator[] (const str &s) { return tab[s]; }
  bhash<str> tab;
};

