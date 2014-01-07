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

#include "ok.h"
#include "pubutil.h"
#include "parseopt.h"
#include "resp.h"
#include "rxx.h"
#include <stdlib.h>
#include "okdbg.h"
#include "sfsmisc.h"
#include "ok_adebug.h"
#include "sfs_select.h"
#include "tame_trigger.h"
#include "resp2.h"
#include "okprotutil.h"
#include "okrfn.h"
#include "pub3expr.h"

#ifdef HAVE_LINUX_PRCTL_DUMP
# include <sys/prctl.h>
#endif /* HAVE_LINUX_PRCTL_DUMP */


extern char ** environ;

//-----------------------------------------------------------------------

void
init_syscall_stats ()
{
  if (ok_ssdi > 0) {
    global_syscall_stats = New syscall_stats_t ();
    global_ssd_last = sfs_get_timenow();
  }
}

//-----------------------------------------------------------------------

bool is_internal(const ptr<const ahttpcon> &con) {
  static const in_addr_t localhost_ip = inet_addr("127.0.0.1");
  if (!con)
    return false;
  const sockaddr_in *in_addr = con->get_sin();
  if (!in_addr)
    return false;
  return con->get_sin()->sin_addr.s_addr == localhost_ip;
}

//-----------------------------------------------------------------------

void
ok_httpsrv_t::malloc_init () {}

//-----------------------------------------------------------------------

ptr<cookie_t>
outcookie_holder_t::add_cookie (const str &h, const str &p)
{
  ptr<cookie_t> ret = New refcounted<cookie_t> (h, p);
  _oc.push_back (ret);
  return ret;
}

//-----------------------------------------------------------------------

tamed void 
okclnt_base_t::error_T (int n, const str &s, bool do_send_complete, 
			evv_t::ptr ev)
{ 
  set_status (n);
  twait { 
    oksrvc->error (_client_con, n, s, mkevent (), hdr_p (), this);
  }

  if (do_send_complete)
    send_complete ();

  if (ev) 
    ev->trigger ();
}

//-----------------------------------------------------------------------

void
okclnt_base_t::error (int n, const str &s, evv_t::ptr ev)
{
  error (n, s, true, ev);
}

//-----------------------------------------------------------------------

void
okclnt_base_t::error (int n)
{
  error (n, NULL, true, NULL);
}

//-----------------------------------------------------------------------

str
ok_httpsrv_t::servinfo () const
{
  if (si)
    return si;

  // IF not on network, need to have something here....
  str hn = hostname;
  if (!hn) { hn = "-"; }

  strbuf b;
  b << reported_name << "/" << version << " Server at " << hn 
    << " Port " << listenport;
  
  return (si = b);
}

//-----------------------------------------------------------------------

tamed void
ok_httpsrv_t::geterr_T (int n, str s, htpv_t v, bool gz,
			ok_httpsrv_t::http_resp_ev_t ev)
{
  tvars {
    ptr<http_response_base_t> ret;
    http_resp_attributes_t hra (n, v);
    compressible_t::opts_t co ((gz && ok_gzip_error_pages) ? 
			       GZIP_SMART : GZIP_NONE);
  }

  hra.set_content_delivery (co);
  
  twait { geterr (s, hra, mkevent (ret)); }
  ev->trigger (ret);
}

//-----------------------------------------------------------------------

tamed void
ok_httpsrv_t::geterr2_T (str s, const http_resp_attributes_t &hra,
			 ok_httpsrv_t::http_resp_ev_t ev)
{
  tvars {
    str *fnp;
    ptr<http_response_t> ret;
    ptr<http_pub_t> hp;
    pub3::obj_dict_t d;
    int n (hra.get_status ());
  }
  fnp = _errdocs[n];
  if ((fnp = _errdocs[n]) && *fnp && pub3 ()) {
    d ("STATUS") = n;
    d ("SERVINFO") = servinfo ();
    if (s) {
      d ("AUXSTR") = s;
    }
    twait { http_pub_t::alloc2 (pub3 (), hra, *fnp, mkevent (hp), d.dict ()); }
    ret = hp;
  }

  if (!ret) {
    ret = New refcounted<http_error_t> (hra, servinfo (), s);
  }

  ev->trigger (ret);
}

//-----------------------------------------------------------------------

tamed void
ok_httpsrv_t::error_T (ref<ahttpcon> x, int n, str s, 
		       evv_t::ptr ev, http_inhdr_t *h, okclnt_interface_t *b) 
{
  tvars {
    ptr<http_parser_raw_t> prs;
    int status2;
    htpv_t v;
    ptr<http_response_base_t> e;
  }
  if (!h && !x->closed ()) {
    prs = http_parser_raw_t::alloc (x);
    twait { prs->parse (mkevent (status2)); }
    if (status2 == HTTP_OK) {
      h = prs->hdr_p ();
    }
  }
  v = h ? h->get_vers () : 0;
  twait { geterr (n, s, v, h && h->takes_gzip (), mkevent (e)); }
  if (b) { b->fixup_log (e); }

  assert (e);
  if (svclog)
    log (x, h, e, s);
  if (!x->closed ()) {
    twait { e->send (x, mkevent ()); }
  }
  if (ev) { 
    ev->trigger ();
  }
}

//-----------------------------------------------------------------------

void
ok_httpsrv_t::init_sfs_clock (const str &f)
{
  if (clock_mode != SFS_CLOCK_GETTIME) {
#ifdef HAVE_SFS_SET_CLOCK
    warn << "*unstable: switching SFS core clock to mode: " 
	 << int (clock_mode) << "\n";
    sfs_set_clock (clock_mode, f);
#else
    warn << "Cannot disable SFS clock; this version of SFS does not "
	 << "support it\n";
#endif /* HAVE_SFS_CLOCKMODE */
  }
}

//-----------------------------------------------------------------------

oksrvc_t::~oksrvc_t ()
{
  okclnt_interface_t *n, *p;
  for (p = clients.first; p; p = n) {
    n = clients.next (p);
    delete p;
  }
}

//-----------------------------------------------------------------------

tamed void
oksrvc_t::keepalive (ahttpcon_wrapper_t<ahttpcon> acw)
{
  tvars {
    int fd(acw.con()->takefd ());
    int close_res;
    okctl_sendcon_arg2_t arg;
    okctl_sendcon_res_t res;
    clnt_stat err;
    ptr<aclnt> mycli;
  }

  if (ctlx && (mycli = clnt) && fd >= 0) {
    ctlx->sendfd (fd, false);
    acw.to_xdr (&arg);
    twait {
      RPC::okctl_program_1::okctl_keepalive (mycli, arg, &res, mkevent (err));
    }
    if (err) {
      strbuf b;
      b << "RPC error in keepalive passback : " << err;
      okdbg_warn (ERROR, b);
    } else if (res != OK_STATUS_OK) {
      strbuf b;
      b << "Non-ok result from keepalive passback: ";
      rpc_print (b, res);
      okdbg_warn (ERROR, b);
    }
    do {
      close_res = close(fd);
    }  while (close_res == -1 && errno == EINTR);
    if (close_res == -1) {
      perror("TV_ERROR: in oksrvc_t::keepalive while closing fd.");
    }
  }
}

//-----------------------------------------------------------------------

void
oksrvc_t::init (int argc, char *argv[])
{
  setprogname (argv[0]);
  name = argv[0];

  set_debug_flags ();

  if (OKDBG2(OKD_STARTUP))
    SVC_CHATTER ("starting up; OKWS version " << OKWS_PATCHLEVEL_STR <<
		 "; running as (" << getuid () << ", " << geteuid () << ")");

  str mmc_file = ok_mmc_file;
  str loggers;
  str dp_str;

  int seltmp = 0;
  int gztmp;

  if (OKDBG2(OKD_STARTUP)) {
    SVC_CHATTER("argc = " << argc);
    for (int i = 0; i < argc; i++) {
      SVC_CHATTER("argv[" << i << "] = " << argv[i]);
    }
  }

  if (argc == 2) {
    ptr<cgi_t> t (cgi_t::str_parse (argv[1]));
    t->lookup ("jaildir", &jaildir);
    t->lookup ("version", &version);
    t->lookup ("hostname", &hostname);
    t->lookup ("listenport", &listenport);
    t->lookup ("okwsname", &reported_name);
    t->lookup ("server", &global_okws_server_label);
    t->lookup ("logfd", &_log_fd);
    t->lookup ("pubfd", &_pub_fd);
    t->lookup ("logfmt", &logfmt);
    t->lookup ("gzip", &gztmp);
    t->lookup ("filtercgi", &ok_filter_cgi);
    t->lookup ("gziplev", &ok_gzip_compress_level);
    t->lookup ("gzipnlev", &ok_gzip_naive_compress_level);
    t->lookup ("gzipcsl", &ok_gzip_cache_storelimit);
    t->lookup ("logtick", &ok_log_tick);
    t->lookup ("logprd", &ok_log_period);
    t->lookup ("clito", &ok_clnt_timeout);
    t->lookup ("ps", &ok_axprt_ps);
    t->lookup ("reqszlimit", &ok_reqsize_limit);
    t->lookup ("cgilimit" , &ok_cgibuf_limit);
    t->lookup ("ssdi", &ok_ssdi);
    t->lookup ("fdlw", &ok_svc_fds_low_wat);
    t->lookup ("fdhw", &ok_svc_fds_high_wat);
    t->lookup ("mmcf", &mmc_file);
    t->lookup ("dz", &ok_dangerous_zbufs);
    t->lookup ("rsl", &ok_recycle_suio_limit);
    t->lookup ("lifetime", &ok_svc_life_time);
    t->lookup ("lifereqs", &ok_svc_life_reqs);
    t->lookup ("wss", &ok_pub3_wss);
    t->lookup ("caching", &ok_pub3_caching);
    t->lookup ("ncto", &ok_pub3_svc_neg_cache_timeout);
    t->lookup ("pub3chnk", &ok_pub3_max_datasz);
    t->lookup ("wfs", &wait_for_signal_in_startup);
    t->lookup ("loggers", &loggers);
    t->lookup ("lqm", &ok_listen_queue_max);
    t->lookup ("p3jse", &ok_pub3_json_strict_escaping);
    t->lookup ("jsibm", &ok_pub3_json_int_bitmax);
    t->lookup ("asr", &_aggressive_svc_restart);
    ok_svc_accept_msgs = t->blookup ("acmsg");
    svclog = t->blookup ("svclog");
    jailed = t->blookup ("jailed");
    ok_send_sin = t->blookup ("sendsin");
    t->lookup ("sel", &seltmp);
    t->lookup ("dps", &dp_str);
    t->lookup ("aczw", &ok_ahttpcon_zombie_warn);
    t->lookup ("aczt", &ok_ahttpcon_zombie_timeout);
    t->lookup ("bid", &_brother_id);
    t->lookup ("nch", &_n_children);
    t->lookup ("gzch", &ok_gzip_chunking);
    t->lookup ("gzchos", &ok_gzip_chunking_old_safaris);
    t->lookup ("gzep", &ok_gzip_error_pages);
    t->lookup ("dolc", &_die_on_logd_crash);
    t->lookup ("rcyclimitint", &ok_pub3_recycle_limit_int);
    t->lookup ("rcyclimitbt", &ok_pub3_recycle_limit_bindtab);
    t->lookup ("rcyclimitdict", &ok_pub3_recycle_limit_dict);
    t->lookup ("rcyclimitslot", &ok_pub3_recycle_limit_slot);

    ok_sys_sel_policy = sfs_core::select_policy_t (seltmp);
    ok_gzip_mode = gzip_mode_t (gztmp);

    set_sfs_select_policy ();

    if (loggers) {
      if (!ok::syslog_ctl.init_child (loggers)) {
	warn << "Cannot initialize sysloggers!\n";
      }
    }

    if (dp_str && dp_str.len () > 0) {
      if (!_direct_ports.parse (dp_str)) {
	warn << "failed to parse direct ports\n";
      }
    }

    _direct_ports.prepare ();

    // DK: These are statically initialized so set the new limits here
    pub3::get_int_recycler()->set_limit(ok_pub3_recycle_limit_int);
    pub3::get_bindtab_recycler()->set_limit(ok_pub3_recycle_limit_bindtab);
    pub3::get_dict_recycler()->set_limit(ok_pub3_recycle_limit_dict);
    pub3::get_slot_recycler()->set_limit(ok_pub3_recycle_limit_slot);

    int tmp = 0;
    if (t->lookup ("clock", &tmp))
      clock_mode = static_cast<sfs_clock_t> (tmp);

    if (OKDBG2 (SVC_ARGS))
      t->dump1 ();

  }

  // For those processes in a cluster, change the progname to reflect
  // the brother ID (and the pid)
  if (_n_children > 1) {
    strbuf b ("%s[%d:%zu]", progname.cstr (), int(getpid ()), _brother_id);
    str s = b;
    char *new_progname = strdup (s.cstr());
    b.clear (); // to stop suio-related checks in dmalloc
    setprogname (new_progname);
  }

  // Give virtual classes an opportunity to init constants after we've
  // read in all important constants from argv.
  init_constants ();

  init_syscall_stats ();
  init_sfs_clock (mmc_file);
  init_pub ( (ok_pub3_wss > 0 ? pub3::P_WSS : 0) |
	     (ok_pub3_viserr > 0 ? pub3::P_VISERR : 0) );

  zinit (ok_gzip_mode != GZIP_NONE, ok_gzip_compress_level);
}

//-----------------------------------------------------------------------

void
oksrvc_t::handle_diagnostic (svccb *sbp)
{
  RPC::okctl_program_1::okctl_diagnostic_srv_t<svccb> srv (sbp);
  const okctl_diagnostic_arg_t *arg = srv.getarg ();
  ok_xstatus_typ_t status = OK_STATUS_UNKNOWN_OPTION;

  switch (arg->domain) {
  case OK_DIAGNOSTIC_DOMAIN_LEAK_CHECKER:
    status = ok_toggle_leak_checker (arg->cmd);
    break;
  case OK_DIAGNOSTIC_DOMAIN_PROFILER:
    status = ok_toggle_profiler (arg->cmd);
    break;
  case OK_DIAGNOSTIC_DOMAIN_TAME_PROFILER:
    status = ok_toggle_tame_profiler (arg->cmd);
    break;
  case OK_DIAGNOSTIC_DOMAIN_PUB_PROFILER:
    status = ok_toggle_pub_profiler (arg->cmd);
    break;
  default:
    break;
  }

  srv.reply (status);
}

//-----------------------------------------------------------------------

void
oksrvc_t::handle_send_msg (svccb *sbp)
{
  RPC::okctl_program_1::okctl_send_msg_srv_t<svccb> srv (sbp);
  const okctl_send_msg_arg_t *arg = srv.getarg ();
  okctl_send_msg_res_t res (TRUE);
  str err = custom_handle_send_msg (arg->msg);
  if (err) {
    res.set_ok (FALSE);
    *res.err = err;
  }
  srv.reply (res);
}

//-----------------------------------------------------------------------

void
oksrvc_t::ctldispatch (svccb *v)
{
  if (!v) {
    okdbg_warn (ERROR, "oksrvc_t::ctldispatch: NULL RPC received (shutdown)");
    internal_reliable_shutdown ("EOF on channel to okd", ok_shutdown_timeout);
    return;
  }
  u_int p = v->proc ();
  switch (p) {
  case OKCTL_NULL:
    v->reply (NULL);
    break;
  case OKCTL_KILL:
    kill (v);
    break;
  case OKCTL_CUSTOM_1_OUT:
    custom1_rpc (v);
    break;
  case OKCTL_CUSTOM_2_OUT:
    custom2_rpc (v);
    break;
  case OKCTL_SEND_CON2:
    handle_new_con2 (v);
    break;
  case OKCTL_GET_STATS_FROM_SVC:
    handle_get_stats (v);
    break;
  case OKCTL_DIAGNOSTIC:
    handle_diagnostic(v);
    break;
  case OKCTL_SEND_MSG:
    handle_send_msg (v);
    break;
  default:
    v->reject (PROC_UNAVAIL);
    break;
  }
}

//-----------------------------------------------------------------------

str
oksrvc_t::custom_handle_send_msg (str s)
{
  return "no messages handled";
}

//-----------------------------------------------------------------------

void
oksrvc_t::remove (okclnt_interface_t *c)
{
  clients.remove (c);
  --nclients;

  if (ok_svc_life_reqs > 0 && ++n_reqs >= u_int (ok_svc_life_reqs)) {
    internal_reliable_shutdown ("served maximum number of requests",
				ok_shutdown_timeout_fast);
  } else if (sdflag) {
    if (!nclients) {
      end_program ();
    } else {
      OKDBG4(OKD_SHUTDOWN, CHATTER, 
	     "cli shutdown, still %u left, waiting...\n", nclients);
    }
  }
}

//-----------------------------------------------------------------------

void
oksrvc_t::kill (svccb *v)
{
  RPC::okctl_program_1::okctl_kill_srv_t<svccb> srv (v);
  oksig_t sig = *srv.getarg ();

  switch (sig) {
  case OK_SIG_HARDKILL:
    SVC_ERROR ("caught hard KILL trigger; exitting immediately");
    srv.reply ();
    end_program ();
    break;
  default:
    shutdown (true);
    srv.reply ();
    break;
  }
}

//-----------------------------------------------------------------------

void
oksrvc_t::log_connection_crashed ()
{
  SVC_ERROR ("connection to logd crashed");
  if (_die_on_logd_crash) {
    internal_reliable_shutdown ("logd crash", 0);
  }
}

//-----------------------------------------------------------------------

tamed void
oksrvc_t::internal_reliable_shutdown_T (str s, int t)
{
  SVC_CHATTER ("Internal Shutdown Initiated: " << s);
  delaycb (t, 0, wrap (this, &oksrvc_t::end_program));
  shutdown (false);
}

//-----------------------------------------------------------------------

tamed void
oksrvc_t::shutdown (bool end_of_okws_run)
{
  if (OKDBG2(OKD_SHUTDOWN))
    SVC_CHATTER ("caught shutdown trigger");

  sdflag = true;

  // Don't accept any more connections on direct ports....
  _direct_ports.close ();

  // If we're doing a regular okmgr-cycle OR an internal shutdown
  // via # of reqs exceeded OR time running exceded, then we
  // should be eager in ctlclosing our connections to the web server.
  if (_aggressive_svc_restart && !end_of_okws_run) {
    ctlclose ();
  } else {
    // In this case, we're being cautious.  In particular, pubd should
    // stay alive until the last service quits.  Keeping our ctls
    // open to okd will keep pubd alive, so we wait...
    clnt = NULL;
  }

  twait { pre_shutdown_hook (mkevent ()); }
  if (!nclients) 
    end_program ();
  else {
    OKDBG4(OKD_SHUTDOWN, CHATTER,
	   "not exitting yet; %u clients oustanding", nclients);
  }
}

//-----------------------------------------------------------------------

void
oksrvc_t::end_program ()
{
  SVC_CHATTER ("shutting down");
  call_exit (0);
}

//-----------------------------------------------------------------------

void
oksrvc_t::connect ()
{
  ctlx = axprt_unix::alloc (0, ok_axprt_ps);
  ctlcon (wrap (this, &oksrvc_t::ctldispatch));

  //
  // XXX core-0-2 
  // No need to enable_accept -- it's already on by default.
  //
  // enable_accept ();
}

//-----------------------------------------------------------------------

void
ok_con_t::ctlcon (callback<void, svccb *>::ref cb)
{
  srv = asrv_delayed_eof::alloc (ctlx, okctl_program_1, cb);
  clnt = aclnt::alloc (ctlx, okctl_program_1);
  if (!srv || !clnt) {
    warn << "Control file descriptor 0 is not a socket\n";
    fatal << "check that this service was launched by okld.\n";
  }
}

//-----------------------------------------------------------------------

void
ok_con_t::ctlclose ()
{
  clnt = NULL;
  srv = NULL;
  ctlx = NULL; 
}

//-----------------------------------------------------------------------

tamed void
oksrvc_t::debug_launch (evv_t ev)
{
  if (wait_for_signal_in_startup) {
    SVC_CHATTER ("waiting for SIGCONT before continuing...");
    twait { sigcb (SIGCONT, mkevent ()); }
    sigcb (SIGCONT, NULL);
  }
  ev->trigger ();
}

//-----------------------------------------------------------------------

static void
safeguard_stdout()
{
  // By default, SFS sets us up (in aspawn) so that fds 0 and fds 1 **both**
  // point to our parent (in this case okd).  This is fine so long as
  // no one tries to printf("foo").  If someone does, stdout (fd=1) will
  // send that data to okd, who will be expecting an XDR-RPC.  When it
  // gets ASCII text 'foo', it will get mad and close the RPC connection
  // via ::fail().  This will cause the service to initiate shutdown.

  // We redirect stdout -> stderr to prevent devs from making this mistake...

  close (STDOUT_FILENO);
  int rc = dup (STDERR_FILENO);
  assert (rc == STDOUT_FILENO);
}

//-----------------------------------------------------------------------

tamed void 
oksrvc_t::launch_T ()
{
  tvars {
    bool logd_rc, dbs_rc (true), pub_rc (true);
    clnt_stat err;
  }

  enable_coredumps ();
  twait { debug_launch (mkevent ()); }

  // initialize a set of library routines in pub3 to use for this 
  // service;  they are customizable..
  // Note, this cannot be done from the constructor (or ::init(), which
  // is called from the constructor).
  init_pub3_runtime ();

  setsid ();
  connect ();
  
  safeguard_stdout();

  // initialization and connect to logging daemon
  if (_log_fd >= 0) {
    logd = New fast_log_t (_log_fd, logfmt);
    logd->set_fail_cb (wrap (this, &oksrvc_t::log_connection_crashed));
  } else {
    fatal << "No logging mechanism specified.\n";
  }

  if (OKDBG2(SVC_STARTUP)) SVC_CHATTER("+ launching pub and log\n");

  twait {
    launch_pub (mkevent (pub_rc));
    logd->connect (mkevent (logd_rc));
  }

  if (OKDBG2(SVC_STARTUP)) SVC_CHATTER("- launched pub and log\n");

  if (!pub_rc) {
    SVC_FATAL_ERROR ("could not establish connection with pubd version 2");
    exit (1);
  }
  if (!logd_rc) {
    SVC_FATAL_ERROR ("Could not establish connection to logd\n");
    exit (1);
  }


  // Allow work after pub init, but before we connect to DBs.
  if (OKDBG2(SVC_STARTUP)) SVC_CHATTER("+ custom_init0\n");
  twait { custom_init0 (mkevent ()); }
  if (OKDBG2(SVC_STARTUP)) SVC_CHATTER("+ launch_dbs\n");
  twait { launch_dbs (mkevent (dbs_rc)); }
  if (OKDBG2(SVC_STARTUP)) SVC_CHATTER("+ custom_init\n");
  twait { custom_init (mkevent ()); }
  if (OKDBG2(SVC_STARTUP)) SVC_CHATTER("- custom_init\n");

  if (OKDBG2 (OKD_STARTUP) || OKDBG2(SVC_STARTUP))
    SVC_CHATTER ("calling READY RPC to okd");

  twait { RPC::okctl_program_1::okctl_ready (clnt, mkevent (err)); }
  enable_direct_ports ();
  _direct_ports.report ();

  if (err) {
    SVC_FATAL_ERROR ("OK Child Initialization: " << err);
    exit (1);
  }

  SVC_CHATTER ("service readied; OKWS version " << VERSION <<
	       "; running as (" << getuid () << ", " << geteuid () << ")");

  if (ok_svc_life_time > 0) {
    delaycb (ok_svc_life_time, 0,
	     wrap (this, &oksrvc_t::internal_reliable_shutdown,
		   "service lifetime expired", 
		   ok_shutdown_timeout_fast));
  }
}

//-----------------------------------------------------------------------

tamed void 
oksrvc_t::launch_dbs (evb_t ev)
{
  tvars { 
    vec<bool> rc; 
    bool ret (true);
    rendezvous_t<size_t> rv (__FILE__, __LINE__);
    size_t i,j;
  }

  if (OKDBG2(SVC_DATABASES)) {
    SVC_CHATTER ("+ launching databases\n");
  }

  rc.setsize (dbs.size ());

  for (i = 0; i < dbs.size (); i++)  {
    dbs[i]->connect (mkevent (rv, i, rc[i]));
    if (OKDBG2(SVC_DATABASES)) {
      SVC_CHATTER ("++ launching " << dbs[i]->getname () << "\n");
    }
  }

  for (i = 0; i < dbs.size (); i++) {
    twait (rv, j);
    SVC_CHATTER ("-- launched " << dbs[j]->getname () << " -> " 
		 << rc[j] << "\n");
    if (!rc[j])
      ret = false;
  }

  if (!ret) 
    SVC_ERROR ("not all databases properly initalized\n");

  if (OKDBG2(SVC_DATABASES)) {
    SVC_CHATTER ("- launched databases\n");
  }
  ev->trigger (ret);
}

//-----------------------------------------------------------------------

dbcon_t *
oksrvc_t::add_db (const str &host, u_int port, const rpc_program &p)
{
  dbcon_t *d = New dbcon_t (p, host, port);
  dbs.push_back (d);
  return d;
}

//-----------------------------------------------------------------------

void
ok_httpsrv_t::disable_accept ()
{
  if (!accept_enabled) {
    if (accept_msgs)
      okdbg_warn (ERROR, "accept already disabled");
  } else {
    if (accept_msgs)
      okdbg_warn (ERROR, "disabling accept");
    accept_enabled = false;
    disable_accept_guts ();
  }
}

//-----------------------------------------------------------------------

void
ok_httpsrv_t::enable_accept ()
{
  if (accept_enabled) {
    if (accept_msgs)
      okdbg_warn (ERROR, "accept already enabled");
  } else {
    accept_enabled = true;
    if (accept_msgs)
      okdbg_warn (CHATTER, "enabling accept");
    enable_accept_guts ();
  }
}

//-----------------------------------------------------------------------

void
oksrvc_t::enable_accept_guts ()
{
  RPC::okctl_program_1::okctl_reenable_accept (clnt, aclnt_cb_null);
  enable_direct_ports ();
}

//-----------------------------------------------------------------------

void 
oksrvc_t::disable_accept_guts () 
{
  disable_direct_ports ();
}

//-----------------------------------------------------------------------

void
oksrvc_t::closed_fd ()
{
  n_fd_out --;
  if (n_fd_out < int (ok_svc_fds_low_wat) && !accept_enabled)
    enable_accept ();
}

//-----------------------------------------------------------------------

okctl_sendcon_res_t
oksrvc_t::handle_new_con_common (const okclnt_sin_t &sin_in, 
				 ptr<ahttpcon> *x_out)
{
  okctl_sendcon_res_t res = OK_STATUS_OK;
  sockaddr_in *sin = NULL;
  int fd = ctlx->recvfd ();

  if (fd < 0) {
    warn << "Got bad file descriptor instead of new connection\n";
    res = OK_STATUS_BADFD;
  } else {
    if (sin_in.size () != sizeof (sockaddr_in)) {
      warn << "Got bad sockaddr_in from okd (wrong size)\n";
    } else {
      sin = (sockaddr_in *)xmalloc (sizeof (sockaddr_in));
      memcpy ((void *)sin, sin_in.base (), sizeof (*sin));
    }
    // fd -- the new file descriptor
    // sin -- the sockaddr info
    // -1   -- use SUIOLITE_DEF_BUFLEN for length of incoming buffer
    // -1   -- use default receive channel limit
    // false-- no close on exec
    // true -- reset the socket flags 
    ptr<ahttpcon> x = ahttpcon::alloc (fd, sin, -1, -1, false, true);
    *x_out = x;
  }

  return res;
}

//-----------------------------------------------------------------------

void
oksrvc_t::handle_new_con2 (svccb *sbp)
{
  RPC::okctl_program_1::okctl_send_con2_srv_t<svccb> srv (sbp);
  const okctl_sendcon_arg2_t *arg = srv.getarg ();
  ptr<ahttpcon> x;
  okctl_sendcon_res_t res = handle_new_con_common (arg->sin, &x);
  if (res == OK_STATUS_OK) {
    keepalive_data_t kad;
    if (populate_keepalive_data (&kad, *arg)) {
      x->set_keepalive_data (kad);
    }
    ahttpcon_wrapper_t<ahttpcon> acw (x, *arg);
    if (!newclnt (acw))
      res = OK_STATUS_NOMORE;
  }
  srv.reply (res);
}

//-----------------------------------------------------------------------

bool
oksrvc_t::newclnt (ahttpcon_wrapper_t<ahttpcon> acw)
{
  ptr<ahttpcon> lx = acw.con ();
  bool sendmore = true;
  if (!lx) { 
    warn << "oksrvc_t::newclnt: NULL request encountered\n";
  } else {
    n_fd_out ++;
    lx->set_close_fd_cb (wrap (this, &oksrvc_t::closed_fd));
    if (sdflag) {
      error (lx, HTTP_UNAVAILABLE, NULL);
    } else {
      _n_newcli ++;
      okclnt_interface_t *c = make_newclnt (lx);
      c->set_demux_data (acw.demux_data ());
      if (use_union_cgi ())
	  c->set_union_cgi_mode (true);
      c->serve ();
    }
    if (ok_svc_fds_high_wat != 0 && n_fd_out >= int (ok_svc_fds_high_wat)) {
      disable_accept ();
      sendmore = false;
    }
  }
  do_syscall_stats ();
  return sendmore;
}

//-----------------------------------------------------------------------

void
oksrvc_t::add (okclnt_interface_t *c)
{
  nclients++;
  clients.insert_head (c);
}

//-----------------------------------------------------------------------

void
okclnt2_t::set_keepalive_attributes (http_resp_attributes_t *hra)
{
  if (do_keepalive ()) {
    str tmp = hdr_cr().get_connection ();
    if (tmp) {
      hra->set_connection (tmp);
    }
  }
}


//-----------------------------------------------------------------------

bool okclnt_base_t::is_ssl() const {
  return _demux_data && _demux_data->ssl();
}

//-----------------------------------------------------------------------

bool okclnt2_t::is_ssl() const {
  str proxy_info;
  if (is_internal(client_con()) && (proxy_info = (*this)["x-forwarded-proto"])) {
    return proxy_info == "https";
  }
  return _demux_data && _demux_data->ssl();
}

//-----------------------------------------------------------------------

uint32_t okclnt2_t::get_ip() const {
  str proxy_info;
  if (is_internal(client_con()) && (proxy_info = (*this)["x-real-ip"])) {
    struct sockaddr_in addr;
    if (inet_aton(proxy_info.cstr(), &addr.sin_addr)) {
      return addr.sin_addr.s_addr;
    }
    warn <<  __FILE__ "[" << __LINE__ <<  "] in " << __func__ << ": "
      "Could not read proxy info properly." << proxy_info << "\n";
  }
  return client_con()->get_sin()->sin_addr.s_addr;
}

//-----------------------------------------------------------------------

str okclnt2_t::get_ip_str() const {
  str proxy_info;
  if (is_internal(client_con()) && (proxy_info = (*this)["x-real-ip"])) {
    return proxy_info;
  }
  return inet_ntoa(client_con()->get_sin()->sin_addr);
}

//-----------------------------------------------------------------------

tamed void
okclnt2_t::serve_T ()
{
  tvars {
    int status;
    bool did_output (false);
    str s;
  }

  if (ssl_only () && !is_ssl ()) {

    if ((s = ssl_redirect_str ())) {
      twait { redirect (s, HTTP_MOVEDPERM, mkevent ()); }
      did_output = true;
    } else {
      status = HTTP_FORBIDDEN;
    }

  } else {

    twait { parse (mkevent (status)); }
    if (status == HTTP_OK) {
      if (process_flag)
	panic ("duplicate process called!\n");
      else {
	twait { process (mkevent (did_output, status)); }
	if (status <= 0) { status = get_status (); }
      }
    }
  }
    
  if (!did_output) {
    if (status != HTTP_OK) {
      twait { error (status, NULL, true, mkevent ()); }
    } else {
      twait { output (out, mkevent ()); }
    }
  }

  if (do_keepalive () && hdr_cr().get_conn_mode () == HTTP_CONN_KEEPALIVE) {
    ahttpcon_wrapper_t<ahttpcon> acw (client_con (), demux_data ());
    get_oksrvc ()->keepalive (acw);
  }

  serve_complete ();
}

//-----------------------------------------------------------------------

tamed void
okclnt_base_t::serve_T ()
{
  tvars {
    int status;
  }

  if (ssl_only () && !is_ssl ()) {

    str s = ssl_redirect_str ();
    if (s) {
      redirect (s);
    } else {
      error (HTTP_FORBIDDEN);
    }

  } else {

    twait { parse (mkevent (status)); }
    if (status == HTTP_OK) {
      if (process_flag)
	panic ("duplicate process called!\n");
      else {
	process_flag = true;
	if (pre_process ()) {
	  process ();
	}
      }
    } else {
      error (status);
    }
  }
}

//-----------------------------------------------------------------------

tamed void
okclnt_base_t::redirect_T (const str &l, int ht_in, evv_t::ptr ev)
{
  tvars {
    holdvar int ht (ht_in > 0 ? ht_in : HTTP_MOVEDPERM);
    http_resp_attributes_t hra (ht, _self->hdr_cr ().get_vers ());
  }

  // set any other field that might have been asked for.
  // Note, we must do this before the following line.
  hra.set_others (hdr_fields);

  rsp = New refcounted<http_response_redirect_t> (l, hra);
  if (uid_set) rsp->set_uid (uid);
  fixup_log (rsp);
  twait { send (rsp, mkevent ()); }

  send_complete ();

  if (ev) {
    ev->trigger ();
  }
}

//-----------------------------------------------------------------------

void
okclnt_base_t::redirect (const str &l)
{
  redirect (l, HTTP_MOVEDPERM, NULL);
}

//-----------------------------------------------------------------------

void
okclnt_base_t::set_attributes (http_resp_attributes_t *hra)
{
  if (cachecontrol) hra->set_cache_control (cachecontrol);
  if (contenttype) hra->set_content_type (contenttype);
  if (expires) hra->set_expires (expires);
  if (contdisp) hra->set_content_disposition (contdisp);

  // set anything else you want
  hra->set_others (hdr_fields);

  browser_specific_fixups (hdr_cr(), hra);
}

//-----------------------------------------------------------------------

void
browser_specific_fixups (const http_inhdr_t &in, http_resp_attributes_t *out)
{
  if (in.has_broken_chunking ()) {
    out->set_bad_chunking (true);
  }
}

//-----------------------------------------------------------------------

void
okclnt_base_t::fixup_log (ptr<http_response_base_t> rsp)
{
  if (_custom_log2) rsp->set_custom_log2 (_custom_log2);
}

//-----------------------------------------------------------------------

bool
okclnt_base_t::output_hdr (ssize_t len)
{
  assert (output_state == ALL_AT_ONCE);
  output_state = STREAMING_HDRS;

  if (_client_con->closed ()) {
    output_state = CLIENT_EOF;
    return false;
  }

  http_resp_attributes_t hra (get_status (), hdr_cr ().get_vers ());
  set_attributes (&hra);

  // use the default compressible options!

  rsp = New refcounted<http_response_ok_t> (len, hra);
  fixup_log (rsp);

  send (rsp, NULL);
  return true;
}

//-----------------------------------------------------------------------

bool
okclnt_base_t::output_frag_prepare ()
{
  if (output_state == CLIENT_EOF)
    return false;

  assert (output_state == STREAMING_HDRS || output_state == STREAMING_BODY);
  output_state = STREAMING_BODY;

  if (_client_con->closed ()) {
    output_state = CLIENT_EOF;
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------

bool
okclnt_base_t::output_fragment (str s)
{
  if (!output_frag_prepare ())
    return false;
  output_fragment_T (s);
  return true;
}

//-----------------------------------------------------------------------

tamed void
okclnt_base_t::output_fragment_T (str s)
{
  tvars {
    strbuf b (s);
  }
  twait { _client_con->send (b, NULL, mkevent ()); }
}

//-----------------------------------------------------------------------

bool
okclnt_base_t::output_fragment (compressible_t &b, cbv::ptr done)
{
  bool ret = true;
  if (!output_frag_prepare ())
    ret = false;

  // Use the default options, without compression
  const strbuf &sb = b.to_strbuf (compressible_t::opts_t ());

  _client_con->send (sb, NULL, done);
  b.clear();
  return ret;
}

//-----------------------------------------------------------------------

tamed void
okclnt_base_t::output_file (const char *f, evb_t::ptr ev, ptr<pub3::dict_t> a, 
			    pub3::opts_t opt)
{
  tvars {
    zbuf z;
    bool ret (true);
    strbuf b;
  }

  if (!output_frag_prepare ()) {
    ret = false;
  } else {
    twait { pub3 ()->run (&z, f, mkevent (ret), a, opt); }
    if (ret) {
      z.to_strbuf (&b, false);
      twait { _client_con->send (b, NULL, mkevent ()); }
    }
  }

  send_complete ();

  if (ev)
    ev->trigger (ret);
}

//-----------------------------------------------------------------------

tamed void
okclnt_base_t::output_done (evb_t::ptr ev)
{
  tvars {
    bool ret;
  }

  if (output_state == CLIENT_EOF) {
    twait { error (HTTP_CLIENT_EOF, NULL, false, mkevent ()); }
    ret = false;
  } else {
    assert (output_state == STREAMING_HDRS || output_state == STREAMING_BODY);
    output_state = DONE;
    twait { _client_con->set_drained_cb (mkevent ()); }
    ret = true;
  }

  send_complete ();

  if (ev) {
    ev->trigger (ret);
  } 
}

//-----------------------------------------------------------------------

gzip_mode_t 
okclnt_base_t::do_gzip (const compressible_t *b) const
{
  gzip_mode_t ret = GZIP_NONE;
  if (hdr_cr ().takes_gzip () && rsp_gzip && b && b->inflated_len () > 0) {
    ret = ok_gzip_mode;
  }
  return ret;
}

//-----------------------------------------------------------------------

void
okclnt_base_t::output (compressible_t *b, evv_t::ptr ev)
{
  output_T (b, ev);
}

//-----------------------------------------------------------------------

tamed void
okclnt_base_t::output_T (compressible_t *b, evv_t::ptr ev)
{
  tvars {
    gzip_mode_t gz (_self->do_gzip (b));
    ssize_t prelen (gz != GZIP_NONE ? b->inflated_len () : ssize_t (-1));
    strbuf sb;
    holdvar int status (_self->get_status ());
    holdvar http_method_t meth (_self->hdr_cr ().mthd);
    http_resp_attributes_t hra (status, _self->hdr_cr ().get_vers (), meth);
    compressible_t::opts_t opts;
  }


  assert (output_state == ALL_AT_ONCE);
  output_state = DONE;

  // client might have cancelled as we were waiting for DB
  if (_client_con->closed ()) {

    // 3rd argument false --- don't issue send_complete() within
    // error(), since we only want to call send_complete() once.
    twait { error (HTTP_CLIENT_EOF, NULL, false, mkevent ()); }
  } else {

    set_keepalive_attributes (&hra);
    set_attributes (&hra);

    // We can only do this after the attributes are set!
    opts = compressible_t::opts_t (gz, hra.get_chunking_support());
    b->to_strbuf (&sb, opts);
    hra.set_content_delivery (opts);
    rsp = New refcounted<http_response_ok_t> (sb, hra);
    fixup_log (rsp);

    if (uid_set) rsp->set_uid (uid);
    if (prelen > 0) rsp->set_inflated_len (prelen);
    twait { send (rsp, mkevent ()); }
  }

  // in okclnt_t versions 1s, will call delete this..
  send_complete ();

  // make sure this trigger is the last thing done in this function
  // and that we don't access this afterwards...
  if (ev) ev->trigger ();
}

//-----------------------------------------------------------------------

void
okclnt_base_t::send (ptr<http_response_t> rsp, cbv::ptr cb)
{
  fixup_cookies (rsp);

  oksrvc->log (_client_con, hdr_p (), rsp);
  rsp->send (_client_con, cb);
}

//-----------------------------------------------------------------------

str
ok_base_t::okws_exec (const str &s) const
{
  return (s[0] == '/' ? s : str (strbuf (topdir) << "/" << s));
}

//-----------------------------------------------------------------------

// configuration parsing helper routine.
void
ok_base_t::got_bindaddr (vec<str> s, str loc, bool *errp)
{
  strip_comments (&s);

  static rxx addr_rxx ("(\\*|[0-9.]+)(:([0-9]+))?");

  in_addr addr;
  if (s.size () == 3) {
    if (!inet_aton (s[1].cstr(), &addr) || !convertint (s[2], &listenport) ||
	listenport > OK_PORT_MAX) {
      warn << loc << ": usage: BindAddr addr [port]\n";
      *errp = true;
    } else {
      listenaddr_str = s[1];
      listenaddr = ntohl (addr.s_addr);
    }
  } else if (s.size () == 2) {
    if (!addr_rxx.match (s[1]) || 
	(addr_rxx[3] && (!convertint (addr_rxx[3], &listenport) || 
			 listenport > OK_PORT_MAX)) ||
    (addr_rxx[1] != "*" && !inet_aton (addr_rxx[1].cstr(), &addr))) {
      warn << loc << ": usage: BindAddr (<addr>|*)(:<port>)?\n";
      *errp = true;
    } else {
      if (addr_rxx[1] != "*") {
	listenaddr_str = addr_rxx[1];
	warn << "addr: " << listenaddr_str << "\n";
	listenaddr = ntohl (addr.s_addr);
      }
    }
  } else {
    warn << loc << ": usage: BindAddr (<addr>|*)(:<port>)?\n";
    *errp = true;
  }
  if (!*errp) {
    if (_all_ports_map[listenport]) {
      warn << loc << ": repeated port #: " << listenport << "\n";
      *errp = true;
      return;
    }
    add_port (listenport, false);
    
    bind_addr_set = true;
  }
}

//-----------------------------------------------------------------------

void
ok_base_t::got_ports (bool ssl, vec<str> s, str loc, bool *errp)
{
  strip_comments (&s);

  str cmd = s.pop_front ();
  while (s.size ()) {
    u_int32_t t (0);
    okws1_port_t port = 0;
    if (!convertint (s.pop_front (), &t) || !(port = t)) {
      warn << loc << ": usage: " << cmd << " <ports>\n";
      *errp = true;
    } else if (t > PORT_MAX) {
      warn << loc << ": port out of range: " << t << "\n";
      *errp = true;
    } else if (_all_ports_map[port]) {
      warn << loc << ": repeated port #: " << t << "\n";
      *errp = true;
    } else {
      add_port (port, ssl);
    }
  }
}

//-----------------------------------------------------------------------

void
ok_base_t::add_port (okws1_port_t port, bool ssl)
{
  assert (!_all_ports_map[port]);
  _all_ports_map.insert (port, ssl);
  if (ssl) 
    _https_ports.push_back (port);
  else
    _http_ports.push_back (port);
}

//-----------------------------------------------------------------------


//
// splits a URI of the form
//
//   <port>:/<path>
//
// into an integer port (returned) and path, which is output
// via the second pointer argument.

static okws1_port_t
split_uri (str in, str *out)
{
  okws1_port_t port = 0;
  const char *p = in.cstr();
  if (p[0] == ':') {
    const char *e = strchr (p, '/');
    assert (e && e > p);
    assert (convertint (str (p + 1, e - p - 1), &port));
    *out = str (e, in.len () - (e-p));
  } else {
    *out = in;
  }
  return port;
}

//-----------------------------------------------------------------------

//
// 80:/foo --> /foo if listening on port 80 already...
//
str
ok_base_t::fix_uri (const str &in) const
{
  str out;
  okws1_port_t p = split_uri (in, &out);
  out = (p == 0 || p != listenport) ? in : out;

  //debug
  //warn << "fix_uri: " << in << " --> " << out << "\n";

  return out;
}

//-----------------------------------------------------------------------

void
okclnt_base_t::set_hdr_field (const str &k, const str &v)
{
  if (!hdr_fields)
    hdr_fields = New refcounted<vec<http_hdr_field_t> > ();
  hdr_fields->push_back (http_hdr_field_t (k,v));
}

//-----------------------------------------------------------------------

str 
get_okws_config (bool fat)
{
  str cf = okws_etcfile ("okws_config");
  if (!cf) {
    if (fat) {
      cf = okws_etcfile_required ("okd_config");
    } else {
      cf = okws_etcfile ("okd_config");
    }
  }
  return cf;
}

//-----------------------------------------------------------------------

bool
ok_httpsrv_t::init_pub (u_int opts)
{
  bool ret = false;
  if (_pub_fd >= 0) {
    ptr<axprt_stream> x (axprt_stream::alloc (_pub_fd, ok_axprt_ps));

    if (ok_pub3_caching) {
      _pub3 = New refcounted<pub3::caching_remote_publisher_t> (x, opts);
    } else {
      _pub3 = New refcounted<pub3::remote_publisher_t> (x, opts);
    }
    ret = true;
  } else {
    okdbg_warn (ERROR, "Cannot init_pub with (got fd=%d)", _pub_fd);
  }
  return ret;
}

//-----------------------------------------------------------------------

tamed void
ok_httpsrv_t::launch_pub (evb_t ev)
{
  tvars {
    bool rc (false);
  }
  twait { pub3 ()->connect (mkevent (rc)); }
  if (rc) {
    twait { post_launch_pub (mkevent (rc)); }
  }
  if (!rc) 
    okdbg_warn (ERROR, "Connection/initialization to pub3 failed!");
  ev->trigger (rc);
}

//-----------------------------------------------------------------------

tamed void
oksrvc_t::post_launch_pub_T (evb_t ev)
{
  tvars {
    clnt_stat s;
    bool rc (true);
  }
  twait {
    RPC::okctl_program_1::okctl_req_errdocs_2 (clnt, &_errdocs_x, mkevent (s)); 
  }
  if (s) {
    strbuf b;
    b << "In getting error documents: " << s;
    okdbg_warn (ERROR, b);
    rc = false;
  } else {
    for (size_t i = 0; i < _errdocs_x.docs.size (); i++) {
      _errdocs.insert (_errdocs_x.docs[i].status, _errdocs_x.docs[i].fn);
    }
  }
  ev->trigger (rc);
}

//-----------------------------------------------------------------------

ptr<pub3::ok_iface_t> okclnt_base_t::pub3 () { return oksrvc->pub3 (); }

//-----------------------------------------------------------------------

ptr<pub3::ok_iface_t>
okclnt_base_t::pub3_local ()
{
  if (!_p3_locale) { _p3_locale = pub3 (); }
  return _p3_locale;
}

//-----------------------------------------------------------------------

void
okclnt_base_t::set_localizer (ptr<const pub3::localizer_t> l)
{
  // Clone the service object's remote publisher....
  ptr<pub3::remote_publisher_t> r = 
    New refcounted<pub3::remote_publisher_t> (*oksrvc->pub3 ());

  // In our own private copy, set the localizer...
  r->set_localizer (l);

  // And store it away in our slot for a generic publisher...
  _p3_locale = r;
}

//-----------------------------------------------------------------------

bool
ok_base_t::got_generic_exec (vec<str> &s, str loc, bool *errp, 
			     ptr<env_argv_t> *ep)
{
  str cmd, err, prog;
  vec<str> env;
  bool fnd_dbg = false;
  const char *cp;

  strip_comments (&s);

  assert (s.size () > 0);
  
  cmd = s.pop_front ();

  while (s.size () && (cp = strchr (s[0].cstr (), '=')))  {
    if (strncmp (OKWS_DEBUG_OPTIONS, s[0].cstr(), cp - s[0]) == 0)
      fnd_dbg = true;
    env.push_back (s.pop_front ());
  }

  str opts = getenv (OKWS_DEBUG_OPTIONS);
  if (!fnd_dbg && opts) {
    strbuf b;
    b.fmt ("%s=%s", OKWS_DEBUG_OPTIONS, opts.cstr ());
    env.push_back (b);
  }

  if (s.size ()) {

    *ep = New refcounted<env_argv_t> (env, environ);

    // prune out repeated/overloaded environment variables
    (*ep)->prune ();
  }

  if (s.size () < 1) 
    goto usage;

  prog = s[0];
  if (!is_safe (prog)) {
    warn << loc << ": specified program " << prog
	 << ") contains unsafe substrings\n";
    goto fail;
  }

  prog = okws_exec (prog);
  err = can_exec (prog);
  if (err) {
    warn << loc << ": cannot open program '" << prog 
	 << "': " << err << "\n";
    *errp = true;
  } else {
    s[0] = prog;
    return true;
  }

 usage:
  warn << loc << ": usage: " << cmd << "[ENV] <path-to-bin> <args>\n";
 fail:
  *errp = true;
  return false;
}

//-----------------------------------------------------------------------

bool
config_parser_t::do_file (const str &fn)
{
  bool rc = false;
  ptr<pub3::fhash_t> ph;
  if (!(ph = pub3::file2hash (fn))) {
    warn << "Cannot open config file: " << fn << "\n";
  } else {
    str s = ph->to_str ();
    if (_seen[s]) {
      warn << "File included more than once: " << fn << "\n";
    } else {
      _seen.insert (s);
      warn << "Using config file: " << fn << "\n";
      if (!parse_file (fn)) {
	warn << "Config file parse failed: " << fn << "\n";
      } else {
	rc = true;
      }
    }
  }
  return rc;
}

//-----------------------------------------------------------------------

bool
config_parser_t::run_configs (const str &fn)
{
  return (do_file (fn) && post_config (fn));
}

//-----------------------------------------------------------------------

void
config_parser_t::include (vec<str> s, str loc, bool *errp)
{
  if (s.size () != 2) {
    warn << loc << ": usage: include <file>\n";
    *errp = true;
  } else if (!do_file (s[1])) {
    *errp = true;
  }
}

//-----------------------------------------------------------------------

void
set_sfs_select_policy ()
{
  if (ok_sys_sel_policy != sfs_core::SELECT_NONE &&
      sfs_core::set_select_policy (ok_sys_sel_policy) < 0)
    warn << "Failed to set select policy to KQ\n";
}

//-----------------------------------------------------------------------

void
demux_data_t::to_xdr (okctl_sendcon_arg2_t *arg)
{
  arg->port = _port;
  if (_ssl) {
    arg->ssl.alloc ();
    if (_ssl_info) {
      arg->ssl->cipher = _ssl_info;
    }
  }
  timespec_to_xdr (_born_on, &arg->time_recv);
  timespec_to_xdr (_forwarded_on, &arg->time_sent);
}

//-----------------------------------------------------------------------

ptr<demux_data_t>
demux_data_t::alloc (const okctl_sendcon_arg2_t &arg)
{
  str ssl;
  if (arg.ssl) ssl = arg.ssl->cipher;
  struct timespec recv, sent;
  xdr_to_timespec (arg.time_sent, &sent);
  xdr_to_timespec (arg.time_recv, &recv);
  return New refcounted<demux_data_t> (arg.port, arg.ssl, ssl, recv, sent);
}

//-----------------------------------------------------------------------

int
okclnt_base_t::get_reqno () const
{
  int ret = client_con() ? client_con()->get_reqno () : 0;
  return ret;
}

//-----------------------------------------------------------------------

str 
okclnt_base_t::ssl_cipher () const 
{
  if (_demux_data) return _demux_data->ssl_info ();
  else return NULL;
}

//-----------------------------------------------------------------------

void
oksrvc_t::handle_get_stats (svccb *v)
{
  RPC::okctl_program_1::okctl_get_stats_from_svc_srv_t<svccb> srv (v);
  oksvc_stats_t res;
  res.n_sent = ahttpcon_byte_counter.get_bytes_sent ();
  res.n_recv = ahttpcon_byte_counter.get_bytes_recv ();
  srv.reply (res);
}

//-----------------------------------------------------------------------

void
oksrvc_t::enable_coredumps ()
{
#ifdef HAVE_LINUX_PRCTL_DUMP
  if (prctl (PR_SET_DUMPABLE, 1) != 0) {
    warn ("prctl(PR_SET_DUMPABLE, 1) failed: %m\n");
  }
#endif /* HAVE_LINUX_PRCTL_DUMP */
}

//-----------------------------------------------------------------------

okclnt_interface_t::okclnt_interface_t (oksrvc_t *o)
  : oksrvc (o)
{ o->add (this); }

//-----------------------------------------------------------------------

okclnt_interface_t::~okclnt_interface_t ()
{ oksrvc->remove (this); }

//-----------------------------------------------------------------------

void do_syscall_stats ()
{
  if (ok_ssdi > 0 && 
      int (sfs_get_timenow ()- global_ssd_last) > int (ok_ssdi)) {
    time_t diff = sfs_get_timenow () - global_ssd_last;
    global_ssd_last = sfs_get_timenow();
    global_syscall_stats->dump (diff);
    global_syscall_stats->clear ();
  }
}

//-----------------------------------------------------------------------

void
okclnt_base_t::output (compressible_t &b, evv_t::ptr ev)
{
  output (&b, ev);
}

//-----------------------------------------------------------------------

void
okclnt_base_t::okreply (ptr<compressible_t> b, evv_t::ptr ev)
{
  output (b, ev);
}

//-----------------------------------------------------------------------

void
outcookie_holder_t::fixup_cookies (ptr<http_response_base_t> rsp)
{
  const vec<ptr<cookie_t> > *v = get_outcookies ();
  for (size_t i = 0; i < v->size(); i++) {
    rsp->get_header ()->add (http_hdr_cookie_t ((*v)[i]->to_str ()));
  }
}

//-----------------------------------------------------------------------

tamed void
okclnt_t::output2 (ptr<compressible_t> b, evv_t ev)
{
  tvars {
    gzip_mode_t gz (_self->do_gzip (b));
    ssize_t prelen (gz != GZIP_NONE && b ? b->inflated_len () : ssize_t (-1));
    holdvar int status (_self->get_status ());
    holdvar http_method_t meth (_self->hdr_cr ().mthd);
    http_resp_attributes_t hra (status, _self->hdr_cr ().get_vers (), meth);
    ptr<ahttpcon> x (_self->client_con ());
    ptr<http_response_base_t> rsp;
    ssize_t rc;
    compressible_t::opts_t opts;
  }

  assert (output_state == ALL_AT_ONCE);
  output_state = DONE;

  // client might have cancelled as we were waiting for DB
  if (x->closed ()) {

    // 3rd argument false --- don't issue send_complete() within
    // error(), since we only want to call send_complete() once.
    twait { error (HTTP_CLIENT_EOF, NULL, false, mkevent ()); }
  } else {
   
    set_attributes (&hra);
    opts = compressible_t::opts_t (gz, hra.get_chunking_support ());
    hra.set_content_delivery (opts);

    rsp = New refcounted<http_response_ok2_t> (hra, b);
    fixup_log (rsp);

    if (uid_set) rsp->set_uid (uid);
    if (prelen > 0) rsp->set_inflated_len (prelen);
    fixup_cookies (rsp);
    oksrvc->log (x, hdr_p (), rsp);
    twait { rsp->send2 (x, mkevent (rc)); }
  }

  ev->trigger ();
}

//-----------------------------------------------------------------------

void
oksrvc_t::init_pub3_runtime ()
{
  pub3::library_t::import (rfn3::lib_t::alloc ());
}

//-----------------------------------------------------------------------

okclnt_base_t::~okclnt_base_t () {}

//-----------------------------------------------------------------------

void 
timespec_to_xdr (const struct timespec &ts, okctl_timespec_t *x)
{
  x->ts_sec  = ts.tv_sec;
  x->ts_nsec = ts.tv_nsec;
}

//-----------------------------------------------------------------------

void 
xdr_to_timespec (const okctl_timespec_t &x, struct timespec *ts)
{
  ts->tv_sec = x.ts_sec;
  ts->tv_nsec = x.ts_nsec;
}

//-----------------------------------------------------------------------

void
demux_data_t::set_forward_time ()
{
  _forwarded_on = sfs_get_tsnow ();
}

//-----------------------------------------------------------------------

struct timespec demux_data_t::timespec_null = { 0, 0 } ;

//-----------------------------------------------------------------------

bool
populate_keepalive_data (keepalive_data_t *out,
			 const okctl_sendcon_arg2_t &in)
{
  size_t ss = in.scraps.size ();
  int rn = in.reqno;
  bool ret = rn || ss;
  out->_reqno = rn;
  if (ss) {
    out->_buf = in.scraps.base ();
    out->_len = ss;
  }
  if (ret && OKDBG2 (OKD_KEEPALIVE)) {
    str data = opaque2str (in.scraps);
    okdbg_warn (CHATTER, strbuf ("incoming keepalive request(%d): %s", 
				 rn, data.cstr ()));
  }
    
  return ret;
}

//-----------------------------------------------------------------------


oksrvc_t::oksrvc_t (int argc, char *argv[]) 
  : nclients (0), 
    sdflag (false), 
    pid (getpid ()), 
    n_fd_out (0), 
    n_reqs (0),
    wait_for_signal_in_startup (false),
    _n_newcli (0), 
    _brother_id (0), 
    _n_children (1),
    _aggressive_svc_restart (false),
    _die_on_logd_crash (false)
{ 
  init (argc, argv);
  accept_msgs = ok_svc_accept_msgs;
  accept_enabled = true;
}

//-----------------------------------------------------------------------
