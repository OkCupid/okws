
/* $Id$ */

#include "okd.h"
#include "okprot.h"
#include "ahutil.h"
#include "pub.h"
#include "xpub.h"
#include "svq.h"

#define LDCH_WARN(x) \
  warn << servpath << ":" << pid << ": " << x << "\n";

okch_t::okch_t (okd_t *o, const str &s)
  : myokd (o), pid (-1), servpath (s), state (OKC_STATE_NONE),
    destroyed (New refcounted<bool> (false)) 
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
    if (state == OKC_STATE_CRASH || state == OKC_STATE_HOSED || 
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
    // note that no authentication needed for this kill signal.
    if (g == OK_SIG_ABORT) {
      CH_WARN ("aborting unresponsive child\n");
      kill ();
      (*cb) ();
    } else {
      clnt->seteofcb (wrap (this, &okch_t::shutdown_cb1, cb));
      clnt->call (OKCTL_KILL, &g, NULL, aclnt_cb_null);
    }
  } else {
    shutdown_cb1 (cb);
  }
}

void
okch_t::got_new_ctlx_fd (int fd, int p)
{
  pid = p;
  ctlx = axprt_unix::alloc (fd);
  ctlcon (wrap (this, &okch_t::dispatch, destroyed));
  state = OKC_STATE_LAUNCH_SEQ_1;
}

//
// Need two things before we can start dispatching connections
// (and set state == OKC_STATE_SERVE):
//    (1) ptr<ahttpcon> x  to be handed to us by okld.
//    (2) the child to call OKCLNT_READY
//
void
okch_t::start_chld ()
{
  if (state == OKC_STATE_LAUNCH_SEQ_2 && x) {
    state == OKC_STATE_SERVE;
    while (conqueue.size ())
      x->clone (conqueue.pop_front ());
  }
}

void
okch_t::got_new_x_fd (int fd, int p)
{
  if (pid != p) {
    warn << "mismatching process IDs; hosed child: " << servpath << "\n";
    goto xfail;
  }

  if (!ctlx) {
    warn << "improper connection status; hosed child: " << servpath << "\n";
    goto xfail;
  }

  x = ahttpcon_dispatch::alloc (fd);
  x->seteofcb (wrap (this, &okch_t::chld_eof, destroyed));

  start_chld ();

  return;

 xfail:
  state = OKC_STATE_HOSED;
  close (fd);
  chld_eof (destroyed);
}

void
okch_t::shutdown_cb1 (cbv cb)
{
  delete this;
  (*cb) ();
}

void
okch_t::dispatch (ptr<bool> dfp, svccb *sbp)
{
  if (*dfp) {
    warn << "dispatch function ignored for destroyed child\n";
    return;
  }
  if (!sbp) {
    chld_eof (destroyed);
    return ;
  }
  u_int p = sbp->proc ();
  switch (p) {
  case OKCTL_PUBCONF:
    myokd->pubconf (sbp);
    break;
  case OKCTL_READY:
    if (state == OKC_STATE_LAUNCH_SEQ_1) {
      state = OKC_STATE_LAUNCH_SEQ_2;
      start_chld ();
    } else {
      CH_WARN ("Cannot process READY message; in wrong state: " << state);
    }
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
okch_t::kill ()
{
  warn << servpath << ": killing child (pid " << pid << ")\n";
  x->seteofcb (cbv_null);
  x = NULL;
  ctlx = NULL;
  clnt = NULL;
  srv = NULL;
  state = OKC_STATE_NONE;
}

void
okch_t::chld_eof (ptr<bool> dfp)
{
  if (*dfp) {
    warn << "destroyed child process died\n";
    return;
  }
  warn << servpath << ": child process died (pid " << pid << ")\n";
  ctlx = NULL;
  srv = NULL;
  clnt = NULL;
  x = NULL;
  if (myokd && !myokd->in_shutdown ()) {
    state = OKC_STATE_CRASH;
  } else 
    state = OKC_STATE_NONE;
}

