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


okclnt_t::~okclnt_t () 
{ 
  oksrvc->remove (this); 
  while (outcookies.size ())
    delete outcookies.pop_front ();
}

cookie_t *
okclnt_t::add_cookie (const str &h, const str &p)
{
  cookie_t *ret = New cookie_t (h, p);
  outcookies.push_back (ret);
  return ret;
}

void 
okclnt_t::error (int n, const str &s)
{ 
  oksrvc->error (x, n, s, wrap (this, &okclnt_t::delcb), &hdr);
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

ptr<http_response_t>
ok_httpsrv_t::geterr (int n, str s, htpv_t v) const
{
  errdoc_t *e = errdocs[n];
  ptr<http_response_t> ret;
  if (e) {
    aarr_t ar;
    ar.add ("STATUS", n)
      .add ("SERVINFO", servinfo ());
    if (s)
      ar.add ("AUXSTR", s);
    ret = http_pub_t::alloc (n, *pubincluder, e->fn, &ar, v);
  }
  if (!ret)
    ret = New refcounted<http_error_t> (n, servinfo (), s, v);
  return ret;
}

void
ok_httpsrv_t::error (ref<ahttpcon> x, int n, str s, 
		     cbv::ptr c, http_inhdr_t *h) const
{
  if (h || x->closed ()) 
    error2 (x, n, s, c, h);
  else {
    ptr<http_parser_raw_t> prs = http_parser_raw_t::alloc (x);
    prs->parse (wrap (this, &ok_httpsrv_t::error_cb1, prs, n, s, c));
  }
}

void
ok_httpsrv_t::error_cb1 (ptr<http_parser_raw_t> prs, int n, str s,
		      cbv::ptr c, int s2) const
{
  error2 (prs->get_x (), n, s, c, prs->hdr_p ());
}

void
ok_httpsrv_t::error2 (ref<ahttpcon> x, int n, str s, 
		   cbv::ptr c, http_inhdr_t *h) const
{
  htpv_t v = h ? h->get_vers () : 0;
  ptr<http_response_t> e = geterr (n, s, v);
  if (svclog)
    log (x, h, e, s);
  if (x->closed ()) {
    if (c) (*c) ();
  } else {
    e->send (x, wrap (this, &ok_httpsrv_t::error_cb2, x, e, c));
  }
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
  okclnt_t *n, *p;
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

  SVCWARN ("starting up; OKD version " << VERSION <<
	   "; running as (" << getuid () << ", " << geteuid () << ")");

  str mmc_file = ok_mmc_file;

  if (argc == 2) {
    ptr<cgi_t> t (cgi_t::str_parse (argv[1]));
    t->lookup ("jaildir", &jaildir);
    t->lookup ("version", &version);
    t->lookup ("hostname", &hostname);
    t->lookup ("listenport", &listenport);
    t->lookup ("okwsname", &reported_name);
    t->lookup ("server", &global_okws_server_label);
    t->lookup ("logfd", &logfd);
    t->lookup ("logfmt", &logfmt);
    t->lookup ("gzip", &ok_gzip);
    t->lookup ("filtercgi", &ok_filter_cgi);
    t->lookup ("gziplev", &ok_gzip_compress_level);
    t->lookup ("gzipcsl", &ok_gzip_cache_storelimit);
    t->lookup ("logtick", &ok_log_tick);
    t->lookup ("logprd", &ok_log_period);
    t->lookup ("clito", &ok_clnt_timeout);
    t->lookup ("reqszlimit", &ok_reqsize_limit);
    t->lookup ("ssdi", &ok_ssdi);
    t->lookup ("fdlw", &ok_svc_fds_low_wat);
    t->lookup ("fdhw", &ok_svc_fds_high_wat);
    t->lookup ("mmcf", &mmc_file);
    ok_svc_accept_msgs = t->blookup ("acmsg");
    svclog = t->blookup ("svclog");
    jailed = t->blookup ("jailed");
    ok_send_sin = t->blookup ("sendsin");

    int tmp;
    if (t->lookup ("clock", &tmp))
      clock_mode = static_cast<sfs_clock_t> (tmp);

    if (getenv ("ARG_DEBUG"))
      t->dump1 ();
  }
  init_syscall_stats ();
  init_sfs_clock (mmc_file);

  zinit (ok_gzip, ok_gzip_compress_level);
}

bool
ok_httpsrv_t::add_errdoc (int n, const str &f)
{
  if (errdocs[n])
    return false;
  errdocs.insert (New errdoc_t (n, f));
  add_pubfile (f);
  return true;
}

void 
ok_httpsrv_t::add_errdocs (const xpub_errdoc_set_t &eds)
{
  u_int lim = eds.docs.size ();
  for (u_int i = 0; i < lim; i++)
    add_errdoc (eds.docs[i].status, eds.docs[i].fn);
}

void
oksrvc_t::ctldispatch (svccb *v)
{
  if (!v) {
    warn << "oksrvc_t::ctldispatch: NULL RPC received (shutdown)\n";
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
  default:
    v->reject (PROC_UNAVAIL);
    break;
  }
}

void
oksrvc_t::update (svccb *sbp)
{
  xpub_fnset_t *s = sbp->Xtmpl getarg <xpub_fnset_t> ();
  rpcli->publish (*s, wrap (this, &oksrvc_t::update_cb, sbp));
}

void
oksrvc_t::update_cb (svccb *sbp, ptr<pub_res_t> pr)
{
  ok_res_t okr (pr);
  sbp->replyref (okr.to_xdr ());
}

void
oksrvc_t::remove (okclnt_t *c)
{
  clients.remove (c);
  if (!--nclients && sdflag)
    end_program ();
}

void
oksrvc_t::kill (svccb *v)
{
  oksig_t *sig = v->Xtmpl getarg<oksig_t> ();
  switch (*sig) {
  case OK_SIG_HARDKILL:
    SVCWARN ("caught hard KILL signal; exitting immediately");
    end_program ();
    break;
  default:
    shutdown ();
    v->reply (NULL);
    break;
  }
}

void
oksrvc_t::shutdown ()
{
  SVCWARN ("caught shutdown signal");
  sdflag = true;
  clnt = NULL; // don't ctlclose -- we might still need more messages.
  if (!nclients) 
    end_program ();
}

void
oksrvc_t::end_program ()
{
  SVCWARN ("shutting down");
  exit (0);
}

void
oksrvc_t::connect ()
{
  ctlx = axprt_unix::alloc (1);
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
  srv = asrv::alloc (ctlx, okctl_program_1, cb);
  clnt = aclnt::alloc (ctlx, okctl_program_1);
  if (!srv || !clnt) {
    warn << "Control file descriptor 1 is not a socket\n";
    fatal << "check that child is launched by okd.\n";
  }
}

void
ok_con_t::ctlclose ()
{
  clnt = NULL;
  srv = NULL;
  ctlx = NULL; 
}

void
oksrvc_t::launch ()
{
  setsid ();
  connect ();
  lnum = 2;

  if (dbs.size ()) lnum++;

  // setup and launch Remote Pub Client
  rpcli = pub_rclient_t::alloc (clnt, OKCTL_LOOKUP, OKCTL_GETFILE,
				OKCTL_PUBCONF, get_andmask (), get_ormask ());
  rpcli->config (wrap (this, &oksrvc_t::launch_pub_cb0));

  // initialization and connect to logging daemon
  if (logfd >= 0) {
    logd = New fast_log_t (logfd, logfmt);
  } else {
    fatal << "No logging mechanism specified.\n";
  }
  logd->connect (wrap (this, &oksrvc_t::launch_log_cb));

  // database connections
  if (dbs.size ()) {
    launch_dbs (wrap (this, &oksrvc_t::launch2));
  }
}

void
oksrvc_t::launch_log_cb (bool rc)
{
  if (!rc) {
    SVCWARN ("Connection to log service failed");
    exit (1);
  }
  launch3 ();
}

void
oksrvc_t::launch3 ()
{
  if (!--lnum) {
    custom_init (wrap (this, &oksrvc_t::launch4));
  }
}

void
oksrvc_t::launch_pub_cb0 (ptr<pub_res_t> pr)
{
  if (!*pr) 
    SVCWARN (pr->to_str ());

  ptr<xpub_errdoc_set_t> xd = New refcounted<xpub_errdoc_set_t> ();
  clnt->call (OKCTL_REQ_ERRDOCS, NULL, xd, 
	      wrap (this, &oksrvc_t::launch_pub_cb1, xd));
}

void
oksrvc_t::launch_pub_cb1 (ptr<xpub_errdoc_set_t> xd, clnt_stat err)
{
  if (err) {
    SVCWARN ("ErrorDocInitError: " << err);
    exit (1);
  }

  //
  // will add error documents to the pub file list
  //
  add_errdocs (*xd);
  init_publist ();
  pubfiles (wrap (this, &oksrvc_t::launch_pub_cb2));
}

void 
oksrvc_t::launch_pub_cb2 (bool rc)
{
  if (!rc) 
    exit (1);
  launch3 ();
}

void
oksrvc_t::launch_dbs (cbb cb)
{
  dbl = dbs.size ();
  u_int lim = dbl;
  dbstatus = true;

  if (!lim)
    (*cb) (dbstatus);

  for (u_int i = 0; i < lim; i++) 
    dbs[i]->connect (wrap (this, &oksrvc_t::launch_dbcb, cb, i));
}

void
oksrvc_t::launch_dbcb (cbb cb, u_int i, bool r)
{
  if (!r) 
    dbstatus = false;
  if (!--dbl)
    (*cb) (dbstatus);
}

void
oksrvc_t::launch2 (bool b)
{
  if (!b)
    SVCWARN ("not all databases properly initalized");
  launch3 ();
}

void
oksrvc_t::launch4 ()
{
  // debug code
  SVCWARN ("calling READY RPC to okd");

  clnt->call (OKCTL_READY, NULL, NULL, wrap (this, &oksrvc_t::launch5));
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
  lblnc_t *l = New lblnc_t (rpcli, i, p, port);
  dbs.push_back (l);
  return l;
}

void
oksrvc_t::launch5 (clnt_stat err)
{
  // debug code
  SVCWARN ("Servided readied");

  if (err) {
    SVCWARN ("OK Child Initialization: " << err);
    exit (1);
  }
}

void
ok_httpsrv_t::disable_accept ()
{
  if (!accept_enabled) {
    if (accept_msgs)
      warn << "accept already disabled\n";
  } else {
    if (accept_msgs)
      warn << "disabling accept\n";
    accept_enabled = false;
    disable_accept_guts ();
  }
}

void
ok_httpsrv_t::enable_accept ()
{
  if (accept_enabled) {
    if (accept_msgs)
      warn << "accept already enabled\n";
  } else {
    accept_enabled = true;
    if (accept_msgs)
      warn << "enabling accept\n";
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
      okclnt_t *c = make_newclnt (lx);
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
oksrvc_t::add (okclnt_t *c)
{
  nclients++;
  clients.insert_head (c);
}

void
okclnt_t::serve ()
{
  parse ();
}

void
okclnt_t::parse ()
{
  http_parser_cgi_t::parse (wrap (this, &okclnt_t::http_parse_cb));
}

void
okclnt_t::http_parse_cb (int status)
{
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
okclnt_t::redirect (const str &l, int ht)
{
  http_resp_attributes_t hra (ht, hdr.get_vers ());
  rsp = New refcounted<http_response_redirect_t> (l, hra);
  send (rsp);
}

void
okclnt_t::output (compressible_t &b)
{
  // client might have cancelled as we were waiting for DB
  if (x->closed ()) {
    error (HTTP_CLIENT_EOF);
    return;
  }

  ssize_t prelen = -1;
  bool gz = hdr.takes_gzip () && ok_gzip && rsp_gzip;
  if (gz) 
    prelen = b.inflated_len ();
  const strbuf &sb = b.to_strbuf (gz);

  http_resp_attributes_t hra (HTTP_OK, hdr.get_vers ());

  hra.set_gzip (gz);
  if (cachecontrol) hra.set_cache_control (cachecontrol);
  if (contenttype) hra.set_content_type (contenttype);
  if (expires) hra.set_expires (expires);
    
  rsp = New refcounted<http_response_ok_t> (sb, hra);
  if (uid_set) rsp->set_uid (uid);
  if (prelen > 0) rsp->set_inflated_len (prelen);
  send (rsp);
}

void
okclnt_t::send (ptr<http_response_t> rsp)
{

  // do cookies
  for (u_int i = 0; i < outcookies.size(); i++) {
    rsp->header.add (http_hdr_cookie_t (outcookies[i]->to_str ()));
  }

  oksrvc->log (x, &hdr, rsp);
  rsp->send (x, wrap (this, &okclnt_t::delcb));
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
  rpcli->add_rootfile (s, c);
}

void
oksrvc_t::pubfiles (cbb cb) 
{
  rpcli->publish (wrap (this, &oksrvc_t::pubbed, cb));
}

void
oksrvc_t::pubbed (cbb cb, ptr<pub_res_t> res)
{
  if (!*res) {
    SVCWARN ("Pub of Rootfiles Failed");
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
  str cmd = s.pop_front ();
  while (s.size ()) {
    u_int32_t t;
    okws1_port_t port;
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
