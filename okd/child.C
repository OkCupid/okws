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

#include "okd.h"
#include "okprot.h"
#include "ahutil.h"
#include "pub.h"
#include "xpub.h"
#include "svq.h"
#include "okdbg.h"

#define LDCH_WARN(x) \
  warn << servpath << ":" << pid << ": " << x << "\n";

#define CH_CHATTER(x)                          \
{                                              \
  strbuf b;                                    \
  b << servpath << ":" << pid << ": " << x ;   \
  okdbg_warn (CHATTER, b);                     \
}

okch_t::okch_t (okd_t *o, const str &s)
  : myokd (o), pid (-1), servpath (s), state (OKC_STATE_NONE),
    destroyed (New refcounted<bool> (false)),
    srv_disabled (false), per_svc_nfd_in_xit (0)
{
  myokd->insert (this);
}

okch_t::~okch_t ()
{
  while (conqueue.size ()) 
    myokd->error (conqueue.pop_front (), HTTP_SRV_ERROR);
  myokd->remove (this);
  *destroyed = true;
}

void
okch_t::closed_fd ()
{
  // warn << "debug: dec:  " << per_svc_nfd_in_xit << "\n";
  per_svc_nfd_in_xit --;
  myokd->closed_fd ();
}

void
okch_t::clone (ref<ahttpcon_clone> xc)
{
  if (!x || x->ateof () || state != OKC_STATE_SERVE) {
    if (state == OKC_STATE_CRASH || state == OKC_STATE_HOSED || 
	conqueue.size () >= ok_con_queue_max) {
      myokd->error (xc, HTTP_SRV_ERROR, make_generic_http_req (servpath));
    } else {
      //warn << "queued con\n"; // XX debug
      conqueue.push_back (xc);
    }
  } else if (ok_svc_fd_quota && per_svc_nfd_in_xit > int (ok_svc_fd_quota)) {
    // warn << "debug: fail: " << per_svc_nfd_in_xit << "\n";
    warn << "**WARNING: Service " << servpath << " appears unresponsive!\n";
    myokd->error (xc, HTTP_UNAVAILABLE, make_generic_http_req (servpath));
  } else {
    if (ok_svc_fd_quota) {
      // warn << "debug: inc:  " << per_svc_nfd_in_xit << "\n";
      per_svc_nfd_in_xit ++;
      xc->reset_close_fd_cb (wrap (this, &okch_t::closed_fd));
    }
    send_con_to_service (xc);
  }
}

void
okch_t::shutdown (oksig_t g, cbv cb)
{
  if (okd_child_sel_disable && srv_disabled) {
    warn << "*broken: restarting stopped ASRV for child\n";
    srv_disabled = false;
    xhinfo::xon (ctlx, true);
  }

  if (clnt) {
    // note that no authentication needed for this kill signal.
    if (g == OK_SIG_ABORT) {
      CH_WARN ("aborting unresponsive child\n");
      kill ();
      (*cb) ();
    } else {
      if (OKDBG2(OKD_SHUTDOWN)) {
	CH_CHATTER ("sending OKCTL_KILL to client");
      }
	
      clnt->seteofcb (wrap (this, &okch_t::shutdown_cb1, cb));
      clnt->call (OKCTL_KILL, &g, NULL, aclnt_cb_null);

      // don't answer any more pub messages
      state = OKC_STATE_KILLING;
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

void
okch_t::send_con_to_service (ref<ahttpcon_clone> xc)
{
  if (xc->timed_out ()) {
    CH_WARN ("Connection timed out (fd=" << xc->getfd () 
	     << "): not forwarding to child");
  } else if (xc->getfd () < 0) {
    CH_WARN ("XXX: Dead file descriptor encountered");
  } else {
    inc_n_sent ();
    x->clone (xc);
  }
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

    // debug messages
    CH_WARN ("child changed to serve status; conQsize=" << conqueue.size ());
    
    state = OKC_STATE_SERVE;

    reset_n_sent ();

    // need to check that x is still here every time through the 
    // loop; the service might have crashed as we were servicing
    // queued connections.
    while (conqueue.size () && x) 
      send_con_to_service (conqueue.pop_front ());

    //
    // XXX - this doesn't work yet.  To make it work, we need to do the
    // following:
    //
    //    - renable the server upon new publication pushes.
    //    - handle EOF conditions wisely; meaning, if we get a new 
    //      child for the same service, then we should assume an EOF;
    //      if we're not selecting on the FD, we might not see the
    //      EOF messages come in.
    //
    if (okd_child_sel_disable) {
      // disable the read on this socket.
      warn << "*broken: turning of child select on CTL socket\n";
      srv_disabled = true;
      ctlx->setrcb (NULL);
    }
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
  x->seteofcb (wrap (this, &okch_t::chld_eof, destroyed, true));

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
  if (OKDBG2(OKD_SHUTDOWN)) {
    CH_CHATTER ("in shutdown_cb1");
  }
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
    // need to track down a bug, so adding code instrumentation
    warn << "calling okch_t::chld_eof from okch_t::dispatch\n";
    chld_eof (destroyed);
    return ;
  }
  u_int p = sbp->proc ();

  if (state == OKC_STATE_KILLING) {
    if (OKDBG2(OKD_SHUTDOWN))
      CH_CHATTER ("ignore RPC sent after child killed");
    sbp->ignore ();
    return;
  }

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
    myokd->send_errdoc_set (sbp);
    break;
  case OKCTL_GETFILE:
    myokd->getfile (sbp);
    break;
  case OKCTL_LOOKUP:
    myokd->lookup (sbp);
    break;
  case OKCTL_CUSTOM_1_IN: 
    myokd->custom1_in (sbp);
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
  xpub_fn_t *x = sbp->Xtmpl getarg<xpub_fn_t> ();
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
  if (!clnt) {
    *rpb->res << (strbuf ("Cannot repub; client not yet initialized: ")
		  << servpath);
  } else {
    clnt->call (OKCTL_UPDATE, &rpb->new_fnset, &rpb->xst,
		wrap (this, &okch_t::repub_cb, rpb));
  }
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
  xpubhash_t *xh = sbp->Xtmpl getarg<xpubhash_t> ();
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
okd_t::send_errdoc_set (svccb *sbp)
{
  sbp->replyref (xeds);
}

void
okch_t::kill ()
{
  warn << servpath << ": disconnecting from child (pid " << pid << ")\n";
  x->seteofcb (cbv_null);
  x = NULL;
  ctlx = NULL;
  clnt = NULL;
  srv = NULL;
  state = OKC_STATE_NONE;
}

//
// route a CUSTOM1 message out to the appropriate child
//
void
okch_t::custom1_out (const ok_custom_data_t &x)
{
  // XXX want to collect success information and so on from this guy
  // (as in repub)
  if (clnt) {
    clnt->call (OKCTL_CUSTOM_1_OUT, &x, NULL, aclnt_cb_null);
  } else {
    warn << servpath << ": child in state=" << state << 
      " (pid=" << pid << 
      "); swallowing OKCTL_CUSTOM_1_OUT RPC.\n";
  }
}

void
okch_t::chld_eof (ptr<bool> dfp, bool debug)
{
  if (debug) {
    warn << "okch_t::chld_eof called from ahttpcon/eofcb\n";
  }
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

