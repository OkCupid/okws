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
#include "okprot.h"
#include "okerr.h"
#include "pubutil.h"
#include "parseopt.h"
#include "resp.h"
#include "rxx.h"
#include <stdlib.h>
#include "okdbg.h"
#include "sfsmisc.h"


void
init_syscall_stats ()
{
  if (ok_ssdi > 0) {
    global_syscall_stats = New syscall_stats_t ();
    global_ssd_last = timenow;
  }
}

void
ok_httpsrv_t::malloc_init ()
{
}


okclnt_base_t::~okclnt_base_t () 
{ 
  oksrvc->remove (this); 
  while (outcookies.size ())
    delete outcookies.pop_front ();
}

cookie_t *
okclnt_base_t::add_cookie (const str &h, const str &p)
{
  cookie_t *ret = New cookie_t (h, p);
  outcookies.push_back (ret);
  return ret;
}

void 
okclnt_base_t::error (int n, const str &s)
{ 
  oksrvc->error (x, n, s, wrap (this, &okclnt_base_t::delcb), hdr_p ());
}

str
ok_httpsrv_t::servinfo () const
{
  if (si)
    return si;

  strbuf b;
  b << reported_name << "/" << version << " Server at " << hostname 
    << " Port " << listenport;
  
  return (si = b);
}

TAMED void
ok_httpsrv_t::geterr_T (int n, str s, htpv_t v, bool gz,
			ok_httpsrv_t::http_resp_cb_t cb)
{
  VARS {
    errdoc_t *e;
    ptr<http_response_t> ret;
    ptr<http_pub2_t> hp2;
    aarr_t ar;
  }
  e = errdocs[n];
  if (e && pub2 ()) {
    ar.add ("STATUS", n)
      .add ("SERVINFO", servinfo ());
    if (s)
      ar.add ("AUXSTR", s);
    BLOCK { http_pub2_t::alloc (pub2 (), n, e->fn, @(hp2), &ar, v, gz); }
    ret = hp2;
  }
  if (!ret)
    ret = New refcounted<http_error_t> (n, servinfo (), s, v);
  SIGNAL (cb, ret);
}

TAMED void
ok_httpsrv_t::error_T (ref<ahttpcon> x, int n, str s, 
		       cbv::ptr c, http_inhdr_t *h) 
{
  VARS {
    ptr<http_parser_raw_t> prs;
    int status2;
    htpv_t v;
    ptr<http_response_t> e;
  }
  if (!h && !x->closed ()) {
    prs = http_parser_raw_t::alloc (x);
    BLOCK { prs->parse (@(status2)); }
    if (status2 == HTTP_OK) {
      h = prs->hdr_p ();
    }
  }
  v = h ? h->get_vers () : 0;
  BLOCK { geterr (n, s, v, h && h->takes_gzip (), @(e)); }
  assert (e);
  if (svclog)
    log (x, h, e, s);
  if (!x->closed ()) {
    BLOCK { e->send (x, @()); }
  }
  if (c) 
    (*c) ();
}

/*
static void
usage ()
{
  fatal << "usage: <oksrvc> <cgi-encoded-args>\n";
}
*/

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

oksrvc_t::~oksrvc_t ()
{
  okclnt_base_t *n, *p;
  for (p = clients.first; p; p = n) {
    n = clients.next (p);
    delete p;
  }
}

void
oksrvc_t::init (int argc, char *argv[])
{
  setprogname (argv[0]);
  name = argv[0];

  set_debug_flags ();

  if (OKDBG2(OKD_STARTUP))
    SVC_CHATTER ("starting up; OKWS version " << VERSION <<
		 "; running as (" << getuid () << ", " << geteuid () << ")");

  str mmc_file = ok_mmc_file;

  int pub1tmp;

  if (argc == 2) {
    ptr<cgi_t> t (cgi_t::str_parse (argv[1]));
    t->lookup ("jaildir", &jaildir);
    t->lookup ("version", &version);
    t->lookup ("hostname", &hostname);
    t->lookup ("listenport", &listenport);
    t->lookup ("okwsname", &reported_name);
    t->lookup ("server", &global_okws_server_label);
    t->lookup ("logfd", &logfd);
    t->lookup ("pub2fd", &pub2fd);
    t->lookup ("pub1", &pub1tmp);
    t->lookup ("logfmt", &logfmt);
    t->lookup ("gzip", &ok_gzip);
    t->lookup ("filtercgi", &ok_filter_cgi);
    t->lookup ("gziplev", &ok_gzip_compress_level);
    t->lookup ("gzipcsl", &ok_gzip_cache_storelimit);
    t->lookup ("logtick", &ok_log_tick);
    t->lookup ("logprd", &ok_log_period);
    t->lookup ("clito", &ok_clnt_timeout);
    t->lookup ("ps", &ok_axprt_ps);
    t->lookup ("reqszlimit", &ok_reqsize_limit);
    t->lookup ("ssdi", &ok_ssdi);
    t->lookup ("fdlw", &ok_svc_fds_low_wat);
    t->lookup ("fdhw", &ok_svc_fds_high_wat);
    t->lookup ("mmcf", &mmc_file);
    t->lookup ("dz", &ok_dangerous_zbufs);
    t->lookup ("rsl", &ok_recycle_suio_limit);
    t->lookup ("lifetime", &ok_svc_life_time);
    t->lookup ("lifereqs", &ok_svc_life_reqs);
    t->lookup ("wss", &ok_pub2_wss);
    t->lookup ("caching", &ok_pub2_caching);
    t->lookup ("ncto", &ok_pub2_svc_neg_cache_timeout);
    t->lookup ("pub2chnk", &ok_pub2_max_datasz);
    ok_svc_accept_msgs = t->blookup ("acmsg");
    svclog = t->blookup ("svclog");
    jailed = t->blookup ("jailed");
    ok_send_sin = t->blookup ("sendsin");

    int tmp = 0;
    if (t->lookup ("clock", &tmp))
      clock_mode = static_cast<sfs_clock_t> (tmp);

    pub1_supported = pub1tmp;

    if (OKDBG2 (SVC_ARGS))
      t->dump1 ();

  }
  init_syscall_stats ();
  init_sfs_clock (mmc_file);
  init_pub2 ( (ok_pub2_wss > 0 ? P_WSS : 0) |
	      (ok_pub2_viserr > 0 ? P_VISERR : 0) );

  zinit (ok_gzip, ok_gzip_compress_level);
}

void
oksrvc_t::ctldispatch (svccb *v)
{
  if (!v) {
    okdbg_warn (ERROR, "oksrvc_t::ctldispatch: NULL RPC received (shutdown)");
    shutdown ();
    return;
  }
  u_int p = v->proc ();
  switch (p) {
  case OKCTL_NULL:
    v->reply (NULL);
    break;
  case OKCTL_UPDATE:
    update (v);
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
  default:
    v->reject (PROC_UNAVAIL);
    break;
  }
}

void
oksrvc_t::update (svccb *sbp)
{
  if (supports_pub1 ()) {
    xpub_fnset_t *s = sbp->Xtmpl getarg <xpub_fnset_t> ();
    rpcli->publish (*s, wrap (this, &oksrvc_t::update_cb, sbp));
  } else {
    SVC_ERROR ("Cannot answer UPDATE RPC without Pub v1 support.\n");
    sbp->reject (PROC_UNAVAIL);
  }
}

void
oksrvc_t::update_cb (svccb *sbp, ptr<pub_res_t> pr)
{
  ok_res_t okr (pr);
  sbp->replyref (okr.to_xdr ());
}

void
oksrvc_t::remove (okclnt_base_t *c)
{
  clients.remove (c);

  if (ok_svc_life_reqs > 0 && ++n_reqs >= u_int (ok_svc_life_reqs)) {
    internal_reliable_shutdown ("served maximum number of requests",
				ok_shutdown_timeout_fast);
  } else if (!--nclients && sdflag)
    end_program ();
}

void
oksrvc_t::kill (svccb *v)
{
  oksig_t *sig = v->Xtmpl getarg<oksig_t> ();
  switch (*sig) {
  case OK_SIG_HARDKILL:
    SVC_ERROR ("caught hard KILL signal; exitting immediately");
    end_program ();
    break;
  default:
    shutdown ();
    v->reply (NULL);
    break;
  }
}

void
oksrvc_t::internal_reliable_shutdown (str s, int t)
{
  SVC_CHATTER ("Internal Shutdown Initiated: " << s);
  delaycb (t, 0, wrap (this, &oksrvc_t::end_program));
  shutdown ();
}

void
oksrvc_t::shutdown ()
{
  if (OKDBG2(OKD_SHUTDOWN))
    SVC_CHATTER ("caught shutdown signal");

  sdflag = true;
  clnt = NULL; // don't ctlclose -- we might still need more messages.
  if (!nclients) 
    end_program ();
}

void
oksrvc_t::end_program ()
{
  SVC_CHATTER ("shutting down");
  call_exit (0);
}

void
oksrvc_t::connect ()
{
  ctlx = axprt_unix::alloc (1, ok_axprt_ps);
  x = ahttpcon_listen::alloc (0);
  ctlcon (wrap (this, &oksrvc_t::ctldispatch));

  //
  // setting the listen callback on this ahttpcon_listen object 
  // will turn on its ability to read data from upstream.
  //
  x->setlcb (wrap (this, &oksrvc_t::newclnt));

  //
  // must be called after setlcb; this will turn on our own 
  // accept_enabled flag; it will also wind up calling enable_selread
  // on the underlying object (for the second time!) but this will 
  // simply be a no-op, since the enable_selread is idempotent 
  // in the world of ahttpcon_listen and associated objects.
  //
  enable_accept ();
}

void
ok_con_t::ctlcon (callback<void, svccb *>::ref cb)
{
  srv = asrv_delayed_eof::alloc (ctlx, okctl_program_1, cb);
  clnt = aclnt::alloc (ctlx, okctl_program_1);
  if (!srv || !clnt) {
    warn << "Control file descriptor 1 is not a socket\n";
    fatal << "check that this service was launched by okld.\n";
  }
}

void
ok_con_t::ctlclose ()
{
  clnt = NULL;
  srv = NULL;
  ctlx = NULL; 
}

TAMED void 
oksrvc_t::launch_T ()
{
  VARS {
    bool pub1_rc, logd_rc, dbs_rc (true), pub2_rc (true);
    clnt_stat err;
  }

  setsid ();
  connect ();

  // initialization and connect to logging daemon
  if (logfd >= 0) {
    logd = New fast_log_t (logfd, logfmt);
  } else {
    fatal << "No logging mechanism specified.\n";
  }

  BLOCK {
    launch_pub2 (@(pub2_rc));
    launch_pub1 (@(pub1_rc));
    logd->connect (@(logd_rc));
  }
  if (!pub2_rc) {
    SVC_FATAL_ERROR ("could not establish connection with pubd version 2");
    exit (1);
  }
  if (!logd_rc) {
    SVC_FATAL_ERROR ("Could not establish connection to logd\n");
    exit (1);
  }
  if (!pub1_rc) {
    SVC_FATAL_ERROR ("Launch of legacy publisher (pubd v1) failed\n");
    exit (1);
  }

  // Allow work after pub2 init, but before we connect to DBs.
  BLOCK { custom_init0 (@()); }
  BLOCK { launch_dbs (@(dbs_rc)); }
  BLOCK { custom_init (@()); }

  if (OKDBG2 (OKD_STARTUP))
    SVC_CHATTER ("calling READY RPC to okd");

  BLOCK { clnt->call (OKCTL_READY, NULL, NULL, @(err)); }

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

TAMED void 
oksrvc_t::launch_pub1 (cbb cb)
{
  VARS {
    ptr<pub_res_t> pr;
    bool rc;
    bool res (true);
  }

  // Still need to allocate this even for Pub v1 disabled, since
  // otherwise the legacy calls to pub->include will segfault (with
  // pub unitialized).  When pub v1 phased out, we can remove this.
  rpcli = pub_rclient_t::alloc (clnt, OKCTL_LOOKUP, OKCTL_GETFILE,
				OKCTL_PUBCONF, 
				get_andmask (), get_ormask ());

  if (pub1_supported) {
    // setup and launch Remote Pub Client
    
    BLOCK { rpcli->config (@(pr)); }
    if (!*pr) {
      strbuf b;
      b << "Pub v1: config failed: " << pr->to_str ();
      str s = b;
      SVC_ERROR (s);
    }

    init_publist ();
    
    BLOCK { pubfiles (@(rc)); }
    if (!rc) res = false;
  } else {

    // Call anyway to force warnings. This function should be empty.
    init_publist ();
  }

  SIGNAL (cb, res);
}

TAMED void 
oksrvc_t::launch_dbs (cbb cb)
{
  VARS { 
    vec<bool> rc; 
    bool ret (true);
  }

  rc.setsize (dbs.size ());
  BLOCK {
    for (u_int i = 0; i < dbs.size (); i++) 
      dbs[i]->connect (@(rc[i]));
  }
  for (u_int i = 0; i < dbs.size (); i++) {
    if (!rc[i])
      ret = false;
  }
  if (!ret) 
    SVC_ERROR ("not all databases properly initalized");
  SIGNAL (cb, ret);
}


dbcon_t *
oksrvc_t::add_db (const str &host, u_int port, const rpc_program &p,
		  int32_t l)
{
  dbcon_t *d = New dbcon_t (p, host, port);
  if (authtoks.size () > 0 && l > 0) {
    d->set_txa (l, &authtoks);
  }

  dbs.push_back (d);
  return d;
}

lblnc_t *
oksrvc_t::add_lb (const str &i, const rpc_program &p, int port)
{
  if (supports_pub1 ()) {
    lblnc_t *l = New lblnc_t (rpcli, i, p, port);
    dbs.push_back (l);
    return l;
  } else {
    SVC_ERROR ("Cannot use load balacned services with Pub v1");
    return NULL;
  }
}

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


void
oksrvc_t::enable_accept_guts ()
{
  x->enable_fd_accept ();
}

void
oksrvc_t::disable_accept_guts ()
{
  x->disable_fd_accept ();
}

void
oksrvc_t::closed_fd ()
{
  n_fd_out --;
  if (n_fd_out < int (ok_svc_fds_low_wat) && !accept_enabled)
    enable_accept ();
}

void
oksrvc_t::newclnt (ptr<ahttpcon> lx)
{
  if (!lx) { 
    warn << "oksrvc_t::newclnt: NULL request encountered\n";
  } else {
    n_fd_out ++;
    lx->set_close_fd_cb (wrap (this, &oksrvc_t::closed_fd));
    if (sdflag) {
      error (lx, HTTP_UNAVAILABLE, NULL);
    } else {
      okclnt_base_t *c = make_newclnt (lx);
      c->serve ();
      add (c);
    }
    if (ok_svc_fds_high_wat != 0 && n_fd_out >= int (ok_svc_fds_high_wat)) {
      disable_accept ();
    }
  }
  do_syscall_stats ();
}

void
oksrvc_t::add (okclnt_base_t *c)
{
  nclients++;
  clients.insert_head (c);
}

TAMED void
okclnt_base_t::serve_T ()
{
  VARS {
    int status;
  }
  BLOCK { parse (@(status)); }
  if (status == HTTP_OK)
    if (process_flag)
      panic ("duplicate process called!\n");
    else {
      process_flag = true;
      process ();
    }
  else
    error (status);
}

void
okclnt_base_t::redirect (const str &l, int ht)
{
  http_resp_attributes_t hra (ht, hdr_cr ().get_vers ());
  rsp = New refcounted<http_response_redirect_t> (l, hra);
  send (rsp, wrap (this, &okclnt_base_t::delcb));
}

void
okclnt_base_t::set_attributes (http_resp_attributes_t *hra)
{
  if (cachecontrol) hra->set_cache_control (cachecontrol);
  if (contenttype) hra->set_content_type (contenttype);
  if (expires) hra->set_expires (expires);
  if (contdisp) hra->set_content_disposition (contdisp);

  // set anything else you want
  hra->set_others (hdr_fields);
}

bool
okclnt_base_t::output_hdr (ssize_t len)
{
  assert (output_state == ALL_AT_ONCE);
  output_state = STREAMING_HDRS;

  if (x->closed ()) {
    output_state = CLIENT_EOF;
    return false;
  }

  http_resp_attributes_t hra (HTTP_OK, hdr_cr ().get_vers ());
  set_attributes (&hra);
  hra.set_gzip (false);
  rsp = New refcounted<http_response_ok_t> (len, hra);
  send (rsp, NULL);
  return true;
}

static void hold_str (str s) {}

bool
okclnt_base_t::output_fragment (str s)
{
  if (output_state == CLIENT_EOF)
    return false;

  assert (output_state == STREAMING_HDRS || output_state == STREAMING_BODY);
  output_state = STREAMING_BODY;

  if (x->closed ()) {
    output_state = CLIENT_EOF;
    return false;
  }

  strbuf b (s);
  x->send (b, NULL, wrap (hold_str, s));

  return true;
}

bool
okclnt_base_t::output_done ()
{
  if (output_state == CLIENT_EOF) {
    error (HTTP_CLIENT_EOF);
    return false;
  }
  assert (output_state == STREAMING_HDRS || output_state == STREAMING_BODY);
  output_state = DONE;
  x->set_drained_cb (wrap (this, &okclnt_base_t::delcb));
  return true;
}

bool
okclnt_base_t::do_gzip () const
{
  return hdr_cr ().takes_gzip () && ok_gzip && rsp_gzip;
}

void
okclnt_base_t::output (compressible_t *b)
{
  output (*b);
}

void
okclnt_base_t::output (compressible_t &b)
{
  assert (output_state == ALL_AT_ONCE);
  output_state = DONE;

  // client might have cancelled as we were waiting for DB
  if (x->closed ()) {
    error (HTTP_CLIENT_EOF);
    return;
  }

  ssize_t prelen = -1;
  bool gz = do_gzip ();
  if (gz) 
    prelen = b.inflated_len ();
  const strbuf &sb = b.to_strbuf (gz);

  http_resp_attributes_t hra (HTTP_OK, hdr_cr ().get_vers ());
  set_attributes (&hra);

  hra.set_gzip (gz);
    
  rsp = New refcounted<http_response_ok_t> (sb, hra);
  if (uid_set) rsp->set_uid (uid);
  if (prelen > 0) rsp->set_inflated_len (prelen);
  send (rsp, wrap (this, &okclnt_base_t::delcb));
}

void
okclnt_base_t::send (ptr<http_response_t> rsp, cbv::ptr cb)
{

  // do cookies
  for (u_int i = 0; i < outcookies.size(); i++) {
    rsp->header.add (http_hdr_cookie_t (outcookies[i]->to_str ()));
  }

  oksrvc->log (x, hdr_p (), rsp);
  rsp->send (x, cb);
}

void
oksrvc_t::add_pubfiles (const char *arr[], u_int sz, bool c)
{
  for (u_int i = 0; i < sz; i++) 
    add_pubfile (arr[i]);
}

void
oksrvc_t::add_pubfiles (const char *arr[], bool c)
{
  for (const char **p = arr; p && *p; p++)
    add_pubfile (*p, c);
}

void
oksrvc_t::add_pubfile (const str &s, bool c)
{
  // warn << "add_pubfile:" << s << "\n"; // debug
  if (supports_pub1 ())
    rpcli->add_rootfile (s, c);
  else {
    SVC_ERROR ("Cannot call add_pubfile() without Pub v1 support");
  }
}

void
oksrvc_t::pubfiles (cbb cb) 
{
  if (supports_pub1 ()) {
    rpcli->publish (wrap (this, &oksrvc_t::pubbed, cb));
  } else {
    SVC_ERROR ("Cannot call pubfiles() without Pub v1 support");
  }
}

void
oksrvc_t::pubbed (cbb cb, ptr<pub_res_t> res)
{
  if (!*res) {
    SVC_ERROR ("Pub of Rootfiles Failed");
    res->cluck ();
    (*cb) (false);
  }
  (*cb) (true);
}

str
ok_base_t::okws_exec (const str &s) const
{
  return (s[0] == '/' ? s : str (strbuf (topdir) << "/" << s));
}

// configuration parsing helper routine.
void
ok_base_t::got_bindaddr (vec<str> s, str loc, bool *errp)
{
  strip_comments (&s);

  static rxx addr_rxx ("(\\*|[0-9.]+)(:([0-9]+))?");

  in_addr addr;
  if (s.size () == 3) {
    if (!inet_aton (s[1], &addr) || !convertint (s[2], &listenport) ||
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
	(addr_rxx[1] != "*" && !inet_aton (addr_rxx[1], &addr))) {
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
    if (allports_map[listenport]) {
      warn << loc << ": repeated port #: " << listenport << "\n";
      *errp = true;
      return;
    }
    allports.push_back (listenport);
    allports_map.insert (listenport);
    
    bind_addr_set = true;
  }
}

void
ok_base_t::got_ports (vec<str> s, str loc, bool *errp)
{
  strip_comments (&s);

  str cmd = s.pop_front ();
  while (s.size ()) {
    u_int32_t t;
    okws1_port_t port = 0;
    if (!convertint (s.pop_front (), &t) || !(port = t)) {
      warn << loc << ": usage: " << cmd << " <ports>\n";
      *errp = true;
    } else if (t > PORT_MAX) {
      warn << loc << ": port out of range: " << t << "\n";
      *errp = true;
    } else if (allports_map[port]) {
      warn << loc << ": repeated port #: " << t << "\n";
      *errp = true;
    } else {
      allports.push_back (port);
      allports_map.insert (port);
    }
  }
}


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
  okws1_port_t port;
  const char *p = in;
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

void
okclnt_base_t::set_hdr_field (const str &k, const str &v)
{
  if (!hdr_fields)
    hdr_fields = New refcounted<vec<http_hdr_field_t> > ();
  hdr_fields->push_back (http_hdr_field_t (k,v));
}

str 
get_okws_config (bool fat)
{
  str cf = okws_etcfile ("okws_config");
  if (!cf)
    if (fat) {
      cf = okws_etcfile_required ("okd_config");
    } else {
      cf = okws_etcfile ("okd_config");
    }
  
  return cf;
}

bool
ok_httpsrv_t::init_pub2 (u_int opts)
{
  bool ret = false;
  if (pub2fd > 0) {
    ptr<axprt_stream> x (axprt_stream::alloc (pub2fd, ok_axprt_ps));

    if (ok_pub2_caching) 
      _pub2 = New refcounted<pub2::caching_remote_publisher_t> (x, opts);
    else
      _pub2 = New refcounted<pub2::remote_publisher_t> (x, opts);

    ret = true;
  }
  return ret;
}

TAMED void
ok_httpsrv_t::launch_pub2_T (cbb cb)
{
  VARS {
    bool rc (false);
  }
  if (pub2 ()) {
    BLOCK { pub2 ()->connect (@(rc)); }
    if (rc) {
      BLOCK { post_launch_pub2 (@(rc)); }
    }
    if (!rc) 
      okdbg_warn (ERROR, "Connection/initialization to pub2 failed!");
  }
  SIGNAL (cb, rc);
}

TAMED void
oksrvc_t::post_launch_pub2_T (cbb cb)
{
  VARS {
    clnt_stat s;
    bool rc (true);
  }
  assert (pub2 ());
  BLOCK { clnt->call (OKCTL_REQ_ERRDOCS_2, NULL, &errdocs_x, @(s)); }
  if (s) {
    strbuf b;
    b << "In getting error documents: " << s;
    okdbg_warn (ERROR, b);
    rc = false;
  }
  SIGNAL (cb, rc);
}

ptr<pub2::remote_publisher_t>
okclnt_base_t::pub2 ()
{
  return oksrvc->pub2 ();
}

pval_w_t 
oksrvc_t::operator[] (const str &s) const 
{ 
  if (!supports_pub1 ()) {
    SVC_ERROR ("Cannot call operator[] without Pub v1 support.");
    return pval_w_t ();
  } 
  return (*rpcli)[s] ;
}


str 
oksrvc_t::cfg (const str &n) const 
{ 
  if (!supports_pub1 ()) {
    SVC_ERROR ("Cannot call cfg() without Pub v1 support.");
    return sNULL;
  }
  return rpcli->cfg (n);
}

bool
ok_base_t::got_generic_exec (vec<str> &s, str loc, bool *errp, 
			      ptr<argv_t> *ep)
{
  str cmd, err, prog;
  vec<str> env;

  strip_comments (&s);

  assert (s.size () > 0);
  
  cmd = s.pop_front ();

  while (s.size () && strchr (s[0].cstr (), '=')) 
    env.push_back (s.pop_front ());

  if (s.size ()) {
    *ep = New refcounted<argv_t> (env, environ);
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
