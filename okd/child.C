
/* $Id$ */

#include "okd.h"
#include "okprot.h"
#include "ahutil.h"
#include "pub.h"
#include "xpub.h"
#include "svq.h"

okch_t::okch_t (const str &e, const str &s, okd_t *o, const str &l)
  : pid (-1), rexecpath (e), servpath (s), state (OKC_STATE_NONE), myokd (o),
    cfgfile_loc (l), destroyed (New refcounted<bool> (false)), rcb (NULL) 
{
  myokd->insert (this);
}

okch_t::~okch_t ()
{
  while (conqueue.size ()) 
    myokd->error (conqueue.pop_front (), 500);
  myokd->remove (this);
  *destroyed = true;
}

void
okch_t::clone (ref<ahttpcon_clone> xc)
{
  if (!x || x->ateof () || state != OKC_STATE_SERVE) {
    if (state == OKC_STATE_HOSED || state == OKC_STATE_DELAY || 
	conqueue.size () > ok_con_queue_max) {
      myokd->error (xc, 500, make_generic_http_req (servpath));
    } else {
      conqueue.push_back (xc);
    }
  } else {
    x->clone (xc);
  }
}

void
okch_t::shutdown (oksig_t g, cbv cb)
{
  if (clnt) {
    clnt->seteofcb (wrap (this, &okch_t::shutdown_cb1, cb));
    clnt->call (OKCTL_KILL, &g, NULL, aclnt_cb_null);
  } else {
    shutdown_cb1 (cb);
  }
}

void
okch_t::shutdown_cb1 (cbv cb)
{
  delete this;
  (*cb) ();
}

static inline bool
secdiff (struct timeval *tv0, struct timeval *tv1, int diff)
{
  long sd = tv1->tv_sec - tv0->tv_sec;
  return (sd > diff || (sd == diff && tv1->tv_usec > tv0->tv_usec));
}

void
okch_t::resurrect ()
{
  rcb = NULL;
  if (state == OKC_STATE_LAUNCH)
    return;

  struct timeval *tp = (struct timeval *) xmalloc (sizeof (struct timeval));
  gettimeofday (tp, NULL);

  // myokd->csi = crash sampling interval
  // myokd->max_cp = max crashed processes
  while (timevals.size () && secdiff (timevals[0], tp, ok_csi))
    xfree (timevals.pop_front ());
  timevals.push_back (tp);
  if (timevals.size () > ok_crashes_max) {
    state = OKC_STATE_HOSED;
  } else {
    launch ();
  }
}

void
okch_t::relaunch (ptr<ok_res_t> res)
{
  str execpath = myokd->make_execpath (rexecpath, true);
  if (access (execpath.cstr (), X_OK) != 0) {
    *res << (strbuf (execpath) << ": cannot access for execution");
    return;
  }
  if (state == OKC_STATE_LAUNCH)
    return;
  kill ();
  launch ();
}

void
okch_t::dispatch (ptr<bool> dfp, svccb *sbp)
{
  if (*dfp) {
    warn << "dispatch function ignored for destroyed child\n";
    return;
  }
  if (!sbp) {
    fdcon_eof (destroyed);
    x = NULL;
    return ;
  }
  u_int p = sbp->proc ();
  switch (p) {
  case OKCTL_PUBCONF:
    myokd->pubconf (sbp);
    break;
  case OKCTL_READY:
    state = OKC_STATE_SERVE;
    sbp->reply (NULL);
    break;
  case OKCTL_REQ_ERRDOCS:
    myokd->req_errdocs (sbp);
    break;
  case OKCTL_GETFILE:
    myokd->getfile (sbp);
    break;
  case OKCTL_LOOKUP:
    myokd->lookup (sbp);
    break;
  default:
    sbp->reject (PROC_UNAVAIL);
    break;
  }
}

void
okd_t::pubconf (svccb *sbp)
{
  ptr<xpub_file_t> cf;
  if ((cf = pprox->get_pubconf ())) {
    xpub_getfile_res_t x (XPUB_STATUS_OK);
    *x.file = *cf;
    sbp->replyref (x);
  } else { 
    if (!cfq.blocked (sbp)) {
      ptr<xpub_getfile_res_t> r = New refcounted<xpub_getfile_res_t> ();
      pubd->call (PUB_CONFIG, NULL, r, wrap (this, &okd_t::pubconfed, r));
    }
  }
}

void
okd_t::pubconfed (ptr<xpub_getfile_res_t> r, clnt_stat err)
{
  if (err) {
    r->set_status (XPUB_STATUS_ERR);
    *r->error = strbuf () << err;
  } else if (r->status == XPUB_STATUS_OK) {
    pprox->cache_pubconf (*r->file);
  }
  cfq.finish (r);
}


void
okd_t::lookup (svccb *sbp)
{
  xpub_fn_t *x = sbp->template getarg<xpub_fn_t> ();
  xpub_lookup_res_t res;
  if (pprox->lookup (*x, &res)) 
    sbp->replyref (res);
  else {
    pfnm_t fn = *x;
    if (!luq.inq (fn, sbp)) {
      ptr<xpub_lookup_res_t> resp = New refcounted<xpub_lookup_res_t> ();
      pubd->call (PUB_LOOKUP, x, resp, 
		  wrap (this, &okd_t::lookedup, fn, resp));
    }
  }
}

void
okd_t::lookedup (str fn, ptr<xpub_lookup_res_t> r, clnt_stat err)
{
  if (err) {
    r->set_status (XPUB_STATUS_ERR);
    *r->error = strbuf () << err;
  } else if (r->status == XPUB_STATUS_OK) {
    phashp_t hsh = phash_t::alloc (*r->hash);
    pprox->cache (fn, hsh);
  }
  luq.finish (fn, r);
}

void
okch_t::repub (ptr<ok_repub_t> rpb)
{
  clnt->call (OKCTL_UPDATE, &rpb->new_fnset, &rpb->xst,
	      wrap (this, &okch_t::repub_cb, rpb));
}

void
okch_t::repub_cb (ptr<ok_repub_t> rpb, clnt_stat err)
{
  if (err) 
    *rpb->res << (strbuf ("repub error: ") << err);
  else 
    rpb->res->add (rpb->xst);
}

void
okd_t::getfile (svccb *sbp)
{
  xpubhash_t *xh = sbp->template getarg<xpubhash_t> ();
  phashp_t hsh = phash_t::alloc (*xh);
  xpub_getfile_res_t res;
  if (pprox->getfile (hsh, &res)) 
    sbp->replyref (res);
  else {
    if (!gfq.inq (hsh, sbp)) {
      ptr<xpub_getfile_res_t> resp = New refcounted<xpub_getfile_res_t> ();
      pubd->call (PUB_GETFILE, hsh, resp, 
		  wrap (this, &okd_t::gotfile, hsh, resp));
    }
  }
}

void
okd_t::gotfile (phashp_t hsh, ptr<xpub_getfile_res_t> res, clnt_stat err)
{
  if (err) {
    res->set_status (XPUB_STATUS_ERR);
    *res->error = strbuf () << err;
  } else if (res->status == XPUB_STATUS_OK) {
    pprox->cache (*res->file);
  }
  gfq.finish (hsh, res);
}

void
okd_t::req_errdocs (svccb *sbp)
{
  // XXX - return empty set for now
  xpub_errdoc_set_t eds;
  sbp->replyref (eds);
}

void
okch_t::launch ()
{
  state = OKC_STATE_LAUNCH;
  myokd->get_logd ()->clone (wrap (this, &okch_t::launch_cb, destroyed));
}

void
okch_t::launch_cb (ptr<bool> dfp, int logfd)
{
  if (*dfp) {
    warn << "child killed before it could even launch\n";
    return;
  }
  if (logfd < 0) {
    warn << "Cannot connect to oklogd for server: (" << servpath << "," 
	 << rexecpath << ")\n";
    state = OKC_STATE_HOSED;
    return;
  }
  vec<str> argv;
  str execpath = myokd->make_execpath (rexecpath, true);
  argv.push_back (execpath);

  myokd->env.insert ("logfd", logfd, false);
  argv.push_back (myokd->env.encode ());
  myokd->env.remove ("logfd");

  x = ahttpcon_aspawn (execpath, argv, 
		       wrap (myokd, &okd_t::set_svc_ids), &ctlx);
  pid = ahttpcon_spawn_pid;
  close (logfd);
  if (!x || !ctlx) {
    warn << "Cannot launch server: (" << servpath << "," << execpath << ")\n";
    state = OKC_STATE_HOSED;
    return;
  }
  ctlcon (wrap (this, &okch_t::dispatch, destroyed));
  x->seteofcb (wrap (this, &okch_t::fdcon_eof, destroyed));
 
  while (conqueue.size ())
    x->clone (conqueue.pop_front ());
}

void
okch_t::kill ()
{
  warn << servpath << ": killing child (pid " << pid << ")\n";
  x->seteofcb (cbv_null);
  x = NULL;
  ctlx = NULL;
  clnt = NULL;
  state = OKC_STATE_NONE;
}

void
okch_t::fdcon_eof (ptr<bool> dfp)
{
  if (*dfp) {
    warn << "destroyed child process died\n";
    return;
  }
  warn << servpath << ": child process died (pid " << pid << ")\n";
  ctlx = NULL;
  srv = NULL;
  clnt = NULL;
  if (myokd && !myokd->in_shutdown ()) {
    state = OKC_STATE_DELAY;
    rcb = delaycb (ok_resurrect_delay, ok_resurrect_delay_ms * 1000000, 
		   wrap (this, &okch_t::resurrect));
  } else 
    state = OKC_STATE_NONE;
}

bool
okch_t::can_exec ()
{
  str execpath = myokd->make_execpath (rexecpath, true);
  if (access (execpath.cstr (), R_OK)) {
    warn << cfgfile_loc << ": cannot access service (" << execpath << ")\n";
    return false;
  } else if (access (execpath.cstr (), X_OK)) {
    warn << cfgfile_loc << ": cannot execute service (" << execpath << ")\n";
    return false;
  }
  return true;
}
