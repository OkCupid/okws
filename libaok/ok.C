
/* $Id$ */

#include "ok.h"
#include "okprot.h"
#include "okerr.h"
#include "pubutil.h"
#include "parseopt.h"


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
ok_base_t::servinfo () const
{
  if (si)
    return si;

  strbuf b;
  b << okdname << "/" << version << " Server at " << hostname << " Port " 
    << listenport;
  
  return (si = b);
}

ptr<http_response_t>
ok_base_t::geterr (int n, str s, htpv_t v) const
{
  errdoc_t *e = errdocs[n];
  ptr<http_response_t> ret;
  if (e) {
    aarr_t ar;
    ar.add ("STATUS", n)
      .add ("SERVINFO", servinfo ());
    if (s)
      ar.add ("AUXSTR", s);
    ret = http_pub_t::alloc (n, pub, e->fn, &ar, v);
  }
  if (!ret)
    ret = New refcounted<http_error_t> (n, servinfo (), s, v);
  return ret;
}

void
ok_base_t::error (ref<ahttpcon> x, int n, str s, 
		  cbv::ptr c, http_inhdr_t *h) const
{
  if (h || x->closed ()) 
    error2 (x, n, s, c, h);
  else {
    ptr<http_parser_raw_t> prs = http_parser_raw_t::alloc (x);
    prs->parse (wrap (this, &ok_base_t::error_cb1, prs, n, s, c));

  }
}

void
ok_base_t::error_cb1 (ptr<http_parser_raw_t> prs, int n, str s,
		      cbv::ptr c, int s2) const
{
  error2 (prs->get_x (), n, s, c, prs->hdr_p ());
}

void
ok_base_t::error2 (ref<ahttpcon> x, int n, str s, 
		   cbv::ptr c, http_inhdr_t *h) const
{
  htpv_t v = h ? h->get_vers () : 0;
  ptr<http_response_t> e = geterr (n, s, v);
  log (x, h, e, s);
  if (x->closed ()) {
    if (c) (*c) ();
  } else {
    e->send (x, wrap (this, &ok_base_t::error_cb2, x, e, c));
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
oksrvc_t::init (int argc, char *argv[])
{
  setprogname (argv[0]);
  name = argv[0];

  SVCWARN ("starting up; OKD version " << VERSION <<
	   "; running as (" << getuid () << ", " << geteuid () << ")");

  if (argc == 2) {
    ptr<cgi_t> t (cgi_t::parse (argv[1]));
    t->lookup ("jaildir", &jaildir);
    t->lookup ("version", &version);
    t->lookup ("hostname", &hostname);
    t->lookup ("listenport", &listenport);
    t->lookup ("okdname", &okdname);
    t->lookup ("logfd", &logfd);
    t->lookup ("logfmt", &logfmt);
    t->lookup ("gzip", &ok_gzip);
    t->lookup ("gziplev", &ok_gzip_compress_level);
    t->lookup ("gzipcsl", &ok_gzip_cache_storelimit);
    t->lookup ("logtick", &ok_log_tick);
    t->lookup ("logprd", &ok_log_period);
    t->lookup ("clito", &ok_clnt_timeout);
    jailed = t->blookup ("jailed");
  }

  zinit ();
}

str
ok_base_t::jail2real (const str &fn) const
{
  if (jaildir && !jailed) return strbuf (jaildir) << fn;
  else return fn;
}

bool
ok_base_t::add_errdoc (int n, const str &f)
{
  if (errdocs[n])
    return false;
  errdocs.insert (New errdoc_t (n, f));
  add_pubfile (f);
  return true;
}

void 
ok_base_t::add_errdocs (const xpub_errdoc_set_t &eds)
{
  u_int lim = eds.docs.size ();
  for (u_int i = 0; i < lim; i++)
    add_errdoc (eds.docs[i].status, eds.docs[i].fn);
}

void
oksrvc_t::ctldispatch (svccb *v)
{
  if (!v) {
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
  default:
    v->reject (PROC_UNAVAIL);
    break;
  }
}

void
oksrvc_t::update (svccb *sbp)
{
  xpub_fnset_t *s = sbp->template getarg <xpub_fnset_t> ();
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
  oksig_t *sig = v->template getarg<oksig_t> ();
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
  ctlclose ();
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
  x->setlcb (wrap (this, &oksrvc_t::newclnt));
}

void
ok_con_t::ctlcon (callback<void, svccb *>::ref cb)
{
  srv = asrv::alloc (ctlx, okctl_program_1, cb);
  clnt = aclnt::alloc (ctlx, okctl_program_1);
  if (!srv || !clnt) {
    warn << "Control file descriptor 0 is not a socket\n";
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
  if (!--lnum)
    custom_init (wrap (this, &oksrvc_t::launch4));
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
  clnt->call (OKCTL_READY, NULL, NULL, wrap (this, &oksrvc_t::launch5));
}

dbcon_t *
oksrvc_t::add_db (const str &host, u_int port, const rpc_program &p)
{
  dbcon_t *d = New dbcon_t (p, host, port);
  dbs.push_back (d);
  return d;
}

void
oksrvc_t::launch5 (clnt_stat err)
{
  if (err) {
    SVCWARN ("OK Child Initialization: " << err);
    exit (1);
  }
}

void
oksrvc_t::newclnt (ptr<ahttpcon> lx)
{
  if (sdflag) {
    error (lx, HTTP_UNAVAILABLE, NULL);
  } else {
    okclnt_t *c = make_newclnt (lx);
    c->serve ();
    add (c);
  }
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
    process ();
  else
    error (status);
}

void
okclnt_t::redirect (const str &l)
{
  rsp = New refcounted<http_response_redirect_t> (l, hdr.get_vers());
  send (rsp);
}

void
okclnt_t::output (zbuf &b)
{
  bool gz = hdr.takes_gzip () && ok_gzip;
  const strbuf &sb = b.to_strbuf (gz);
    
  rsp = New refcounted<http_response_ok_t> (sb, hdr.get_vers (), gz);
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


