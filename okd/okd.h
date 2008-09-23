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

#ifndef _OKD_OKD_H
#define _OKD_OKD_H

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
#include "fd_prot.h"
#include "arpc.h"
#include "rxx.h"
#include "tame.h"

#define OK_LQ_SIZE_D    100
#define OK_LQ_SIZE_LL   5
#define OK_LQ_SIZE_UL   1000


typedef enum { OKD_JAILED_NEVER = 0,
	       OKD_JAILED_DEPENDS = 1,
	       OKD_JAILED_ALWAYS = 2 } okd_jailtyp_t;

class okch_t;
typedef callback<void, okch_t *>::ref cb_okch_t;
typedef callback<cb_okch_t, ptr<ok_res_t> >::ref okch_apply_cb_t;

struct ok_repub_t {
  ok_repub_t (const xpub_fnset_t &f, okrescb c)
    : fnset (f), cb (c), res (New refcounted<ok_res_t> ()), cookie (0) {}
  ~ok_repub_t () { (*cb) (res); } 
  void set_new_fnset ();
  const xpub_fnset_t &fnset;
  xpub_fnset_t new_fnset;
  const okrescb cb;
  ptr<ok_res_t> res;
  xpub_result_t xpr;
  ok_xstatus_t xst;

  // for use with the second version of the repub protocol
  xpub_cookie_t cookie;           // "session cookie" during repub
  u_int nfiles;                   // the number of files we need to fetch
  xpub_result2_t xpr2;            // put the initial result in here
  vec<xpub_getfile_res_t> cache;  // temp array to stick solutions into
};

class ok_custom2_trig_t {
public:
  ok_custom2_trig_t () :
    _ok_res (New refcounted<ok_res_t> ()),
    _custom_res (New refcounted<ok_custom_res_set_t> ()) {}

  void add_err (const str &svc, ok_xstatus_typ_t t);
  void add_succ (const str &svc, const ok_custom_data_t &dat);
  ~ok_custom2_trig_t () { _cb (); }
  ptr<ok_res_t> get_ok_res () { return _ok_res; }
  ptr<ok_custom_res_set_t> get_custom_res () { return _custom_res; }
  void setcb (cbv c) { _cb = c; }
private:
  cbv::ptr _cb;
  ptr<ok_res_t> _ok_res;
  ptr<ok_custom_res_set_t> _custom_res;
};

class okd_t;
class okch_t : public ok_con_t {  // OK Child Handle
public:
  okch_t (okd_t *o, const str &e);
  ~okch_t ();
  void launch ();
  void clone (ref<ahttpcon_clone> xc);
  void send_con_to_service (ref<ahttpcon_clone> xc, CLOSURE);
  void shutdown (oksig_t sig, cbv cb);

  void got_new_ctlx_fd (int fd, int p);

  void dispatch (ptr<bool> destroyed, svccb *b);
  void repub (ptr<ok_repub_t> rpb, CLOSURE);

  void fdcon_eof (ptr<bool> destroyed);
  void kill ();
  void custom1_out (const ok_custom_data_t &x);
  void custom2_out (ptr<ok_custom2_trig_t> trig, const ok_custom_data_t &x);
  void chld_eof ();
  void custom2_out_cb (ptr<ok_custom2_trig_t> trig, 
		       ptr<ok_custom_data_t> res,
		       clnt_stat err);

  void to_status_xdr (oksvc_status_t *out);

  inline int get_n_sent () const { return _n_sent; }
  void reset_n_sent () ;
  inline int inc_n_sent () { return (_n_sent ++) ; }
  
  okd_t *myokd;
  int pid;
  const str servpath;       // GET <servpath> HTTP/1.1 (starts with '/')
  ihash_entry<okch_t> lnk;
protected:
  void handle_reenable_accept (svccb *sbp);
  void start_chld ();
private:
  void shutdown_cb1 (cbv cb);
  void closed_fd ();

  vec<ptr<ahttpcon_clone> > conqueue;

  okc_state_t state;
  ptr<bool> destroyed;
  bool srv_disabled;
  int per_svc_nfd_in_xit;  // per service number FD in transit
  int _n_sent;             // N sent since reboot
  time_t _last_restart;    // time when started;
  
  bool _too_busy;          // the server is potentially too busy to get more
};

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
    pprox (pub_proxy_t::alloc ()),
    sdflag (false), sd2 (false), dcb (NULL), 
    sdattempt (0),
    cntr (0),
    coredumpdir (cdd),
    nfd_in_xit (0),
    reqid (0),
    xtab (2),
    _socket_filename (okd_mgr_socket),
    _socket_mode (okd_mgr_socket_mode),
    _accept_ready (false)
  {
    listenport = p;
  }


  ~okd_t ();

  void abort ();

  void insert (okch_t *c) { servtab.insert (c); }
  void remove (okch_t *c) { servtab.remove (c); }

  void got_alias (vec<str> s, str loc, bool *errp);
  void got_regex_alias (vec<str> s, str loc, bool *errp);
  void got_pubd_unix (vec<str> s, str loc, bool *errp);
  void got_pubd_inet (vec<str> s, str loc, bool *errp);
  void got_pubd_exec (vec<str> s, str loc, bool *errp);
  void got_err_doc (vec<str> s, str loc, bool *errp);

  void gotfd (int fd, ptr<okws_fd_t> desc);

  void pubconf (svccb *sbp);
  void getfile (svccb *sbp);
  void lookup (svccb *sbp);

  void pubconfed (ptr<xpub_getfile_res_t> r, clnt_stat err);
  void lookedup (str fn, ptr<xpub_lookup_res_t> r, clnt_stat err);
  void gotfile (phashp_t hsh, ptr<xpub_getfile_res_t> r, clnt_stat err);

  // Well, no one ever said event-driven programming was pretty
  void launch (CLOSURE);

  void launch_logd (cbb cb, CLOSURE);
  void launch_pubd (cbb cb, CLOSURE);

  void sclone (ref<ahttpcon_clone> x, okws1_port_t port, str s, int status);
  void newserv (int fd);
  void shutdown (int sig);

  ihash<const str, okch_t, &okch_t::servpath, &okch_t::lnk> servtab;
  qhash<str, str> aliases;

  struct regex_alias_t {
    regex_alias_t (str t, const rxx &x) : _target (t), _rxx (x) {}
    const str _target;
    rxx _rxx;
  };

  vec<regex_alias_t> regex_aliases;

  ok_usr_t okd_usr; 
  ok_grp_t okd_grp;

  bool supports_pub1 () const { return pubd ? true : false; }

  helper_t *pubd;

  void open_mgr_socket ();

  // functions for the OKMGR interface
  void repub (const xpub_fnset_t &x, okrescb cb);
  void repub2 (const xpub_fnset_t &x, okrescb cb);
  void relaunch (const ok_progs_t &x, okrescb cb);
  void custom1_in (svccb *sbp);
  void custom2_in (svccb *sbp);
  void okctl_get_stats (svccb *sbp);
  void turnlog (okrescb cb);

  void strip_privileges ();

  void kill_srvcs_cb ();
  bool in_shutdown () const { return sdflag; }
  void set_signals ();

  void send_errdoc_set (svccb *sbp);
  void req_errdoc_set_2 (svccb *sbp);

  void closed_fd ();

protected:
  // queueing stuff
  void enable_accept_guts ();
  void disable_accept_guts ();
  bool parse_file (const str &fn);
  bool post_config (const str &fn);

private:
  // Callbacks for Repub, Version 1
  void repub_cb1 (ptr<ok_repub_t> rpb, clnt_stat err);
  void repub_cb2 (ptr<int> i, okrescb cb, ptr<ok_res_t> res);

  // Callbacks for Repub, Version 2
  void repub2_cb1 (ptr<ok_repub_t> rpb, clnt_stat err);
  void repub2_getfiles (ptr<ok_repub_t> rpb);
  bool repub2_getfile (ptr<ok_repub_t> rpb, int i, aclnt_cb cb);
  bool repub2_gotfile (ptr<ok_repub_t> rpb, int i, clnt_stat err);
  void repub2_done (ptr<ok_repub_t> rpb, bool rc);

  void custom1_in (const ok_custom_arg_t &x, okrescb cb);

  void apply_to_children (const ok_progs_t &x, cb_okch_t acb,
			  ptr<ok_res_t> res, cbs::ptr notfoundcb = NULL);

  void newmgrsrv (ptr<axprt_stream> x);
  void check_runas ();

  // shutdown functions
  void kill_srvcs (oksig_t sig);
  void stop_listening ();
  void shutdown_retry ();
  void shutdown2 ();
  void shutdown3 ();
  void shutdown_cb1 ();

  void got_chld_fd (int fd, ptr<okws_fd_t> desc);

  str configfile;
  int okldfd;
  
  pub_proxy_t *pprox;

  svq_t   <ptr<xpub_getfile_res_t> >            cfq;  // conf fetch Q
  svqtab_t<pfnm_t, ptr<xpub_lookup_res_t> >     luq;  // lookup Q
  svqtab_t<phashp_t, ptr<xpub_getfile_res_t> >  gfq;  // Getfile Q

  bool bdlnch;
  u_int chkcnt;
  u_int sdcbcnt;
  bool sdflag, sd2;
  timecb_t *dcb;
  bool jailed;
  int sdattempt;
  int cntr;

  ptr<fdsource_t<okws_fd_t> > okldx;
  str coredumpdir;
  xpub_errdoc_set_t xeds;

  int nfd_in_xit;       // number of FDs in transit
  u_int reqid;
  ahttp_tab_t xtab;

  qhash<int, okws1_port_t> portmap;
  vec<int> listenfds;
  str _socket_filename;
  str _socket_group, _socket_owner;
  int _socket_mode;

  str _config_grp, _config_user;
  bool _accept_ready;
};

class okd_mgrsrv_t 
{
public:
  okd_mgrsrv_t (ptr<axprt_stream> xx, okd_t *o);
  void dispatch (svccb *b);
private:
  void repub (svccb *b, int v);
  void relaunch (svccb *b);
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

inline void 
replystatus (svccb *s, ptr<ok_res_t> res) 
{ 
  s->replyref (res->to_xdr ()); 
}

#endif /* _OKD_OKD_H */
