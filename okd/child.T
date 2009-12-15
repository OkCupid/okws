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

#include "okd.h"
#include "okprot.h"
#include "ahutil.h"
#include "pub.h"
#include "svq.h"
#include "okdbg.h"

#define LDCH_ERROR(x) \
  warn << servpath << ":" << pid << ": " << x << "\n";

//-----------------------------------------------------------------------


okch_t::okch_t (okd_t *o, const str &s, okc_state_t st)
  : myokd (o), pid (-1), servpath (s), state (st),
    destroyed (New refcounted<bool> (false)),
    per_svc_nfd_in_xit (0),
    _n_sent (0), _last_restart (okwstime ()),
    _too_busy (false),
    _generation_id (0)
	       
{
  myokd->insert (this);
}

//-----------------------------------------------------------------------

okch_t::~okch_t ()
{
  while (conqueue.size ())  {
    ahttpcon_wrapper_t<ahttpcon_clone> acw (conqueue.pop_front ());
    ref<ahttpcon_clone> x (acw.con ());
    x->declone ();
    myokd->error (x, HTTP_SRV_ERROR);
  }
  myokd->remove (this);
  *destroyed = true;
}

//-----------------------------------------------------------------------

void
okch_t::closed_fd (u_int64_t gen)
{
  // warn << "debug: dec:  " << per_svc_nfd_in_xit << "\n";
  if (gen != _generation_id) {
    CH_CHATTER ("stale service generation; was there just a restart?");
  }
  per_svc_nfd_in_xit --;
  myokd->closed_fd ();
}

//-----------------------------------------------------------------------

tamed void
okch_t::clone (ahttpcon_wrapper_t<ahttpcon_clone> acw)
{
  tvars {
    ref<ahttpcon_clone> xc (acw.con ());
    bool ok (true);
  }

  if (state == OKC_STATE_STANDBY) {
    twait { awaken (mkevent (ok)); }
  }

  if (!ok) {
    warn << "**WARNING: Cannot awaken service " << servpath << "\n";
    xc->declone ();
    myokd->error (xc, HTTP_SRV_ERROR, make_generic_http_req (servpath));

  } else if (!ctlx || ctlx->ateof () || state != OKC_STATE_SERVE) {

    if (state == OKC_STATE_CRASH || state == OKC_STATE_HOSED 
	|| conqueue.size () >= ok_con_queue_max) {

      xc->declone ();
      myokd->error (xc, HTTP_SRV_ERROR, make_generic_http_req (servpath));

    } else {
      //warn << "queued con\n"; // XX debug
      conqueue.push_back (acw);
    }

  } else if ((ok_svc_fd_quota && per_svc_nfd_in_xit > int (ok_svc_fd_quota))
	     || _too_busy) {
    // warn << "debug: fail: " << per_svc_nfd_in_xit << "\n";

    if (_too_busy)
      warn << "**WARNING: Service " << servpath 
	   << " too busy to accept new connections\n";
    else
      warn << "**WARNING: Service " << servpath << " appears unresponsive!\n";

    xc->declone ();
    myokd->error (xc, HTTP_UNAVAILABLE, make_generic_http_req (servpath));

  } else {
    if (ok_svc_fd_quota) {
      // warn << "debug: inc:  " << per_svc_nfd_in_xit << "\n";
      per_svc_nfd_in_xit ++;
      xc->reset_close_fd_cb (wrap (this, &okch_t::closed_fd, _generation_id));
    }
    assert (ctlx);
    assert (clnt);
    send_con_to_service (acw);
  }
}

//-----------------------------------------------------------------------

tamed void
okch_t::shutdown (oksig_t g, evv_t ev)
{
  tvars {
    ptr<bool> df;
  }
  df = destroyed;

  if (!*df && clnt) {
    // note that no authentication needed for this kill signal.
    if (g == OK_SIG_ABORT) {
      CH_ERROR ("aborting unresponsive child\n");
      kill ();
    } else {
      if (OKDBG2(OKD_SHUTDOWN)) {
	CH_CHATTER ("sending OKCTL_KILL to client");
      }
	
      RPC::okctl_program_1::okctl_kill (clnt, &g, aclnt_cb_null);
      // don't answer any more pub messages
      state = OKC_STATE_KILLING;
      twait { clnt->seteofcb (mkevent ()); }
    }
  }

  if (!*df && OKDBG2(OKD_SHUTDOWN)) {
    CH_CHATTER ("clent shutdown completed");
  }

  ev->trigger ();
}

//-----------------------------------------------------------------------

void
okch_t::got_new_ctlx_fd (int fd, int p)
{
  pid = p;
  ctlx = axprt_unix::alloc (fd, ok_axprt_ps);
  ctlcon (wrap (this, &okch_t::dispatch, destroyed));
  state = OKC_STATE_LAUNCH_SEQ_1;
  start_chld ();
}

//-----------------------------------------------------------------------

tamed void
okch_t::send_con_to_service (ahttpcon_wrapper_t<ahttpcon_clone> acw)
{
  tvars {
    okctl_sendcon_arg2_t arg;
    okctl_sendcon_res_t res;
    clnt_stat err;
    ptr<bool> df;
    ref<ahttpcon_clone> xc (acw.con ());
  }

  df = destroyed;

  assert (ctlx);
  assert (clnt);

  if (xc->timed_out ()) {
    CH_ERROR ("Connection timed out (fd=" << xc->getfd () 
	     << "): not forwarding to child");
  } else if (xc->getfd () < 0) {
    CH_ERROR ("Dead file descriptor encountered");
  } else {
    inc_n_sent ();

    // add in extra SSL and port information; also the remote address,
    // and timestamps
    acw.demux_data ()->set_forward_time ();
    acw.to_xdr (&arg);

    // Hack: reclaim the FD, but hold onto the ahttpcon until
    // after the server has acked it.
    ctlx->sendfd (xc->takefd ());

    // Note! Calling sendfd above might have revealed an EOF condition,
    // which will callback immediately with ctlx = NULL and clnt = NULL
    // for this object.  Thus, we have no guarantees that clnt is non-
    // NULL after the above call.
    if (!clnt) {

      // XXX: Just don't reply to user at all; should potentially 
      // respond with an error message, but since we've already taken
      // the FD out of xc, that would be a nuisance...
      CH_ERROR ("Lost child in between sending FD and sending RPC");

    } else {
      twait { 
	RPC::okctl_program_1::okctl_send_con2 (clnt, &arg, &res, mkevent(err)); 
      }
      if (err) {
	CH_ERROR ("Error in RPC for sending connection: " << err);
      } else if (res == OK_STATUS_NOMORE) {
	if (*df) {
	  CH_ERROR ("No more connections; but service died..\n");
	} else {
	  CH_ERROR ("Service is busy; disabling incoming connections\n");
	  _too_busy = true;
	}
      } else if (res != OK_STATUS_OK) {
	CH_ERROR ("Service rejected new connection: " << res);
      }
    }

    //
    // XXX core-0-2 note:
    //
    // Hold on xc until we've heard back, which will trigger a callback
    // saying that one fewer FD is in transit; however, we'd do best
    // to simplify this mechanism.
    //

  }
}

//-----------------------------------------------------------------------


//
// Need two things before we can start dispatching connections
// (and set state == OKC_STATE_SERVE):
//    (1) ptr<axprt_unix> ctlx to be handed to us by okld.
//    (2) the child to call OKCLNT_READY
//
void
okch_t::start_chld ()
{
  if (state == OKC_STATE_LAUNCH_SEQ_2 && ctlx) {

    // debug messages
    if (OKDBG2 (OKD_STARTUP))
      CH_CHATTER ("child changed to serve status; conQsize=" 
		  << conqueue.size ());
    
    state = OKC_STATE_SERVE;

    reset_accounting ();

    // need to check that ctlx is still here every time through the 
    // loop; the service might have crashed as we were servicing
    // queued connections.
    while (conqueue.size () && ctlx)
      send_con_to_service (conqueue.pop_front ());

  }
}

//-----------------------------------------------------------------------

void
okch_t::reset_accounting ()
{
  reset_n_sent ();
  _too_busy = false;
  _generation_id ++;

  // MK 2009/06/08: Thought about this:
  //
  //    per_svc_nfd_in_xit = 0;
  //
  // and changing closed_fd() above to only decrement if the
  // generation was same as the last.  However, i don't think
  // it's required.  In order to really know, we'll have to track
  // down if the sent FDs that never get delivered ever timeout
  // and therefore get decremented appropriately.
}

//-----------------------------------------------------------------------

void
okch_t::reset_n_sent ()
{
    _n_sent = 0; 
    _last_restart = okwstime ();
}

//-----------------------------------------------------------------------

void
okch_t::handle_reenable_accept (svccb *sbp)
{
  CH_ERROR ("Service reenabled accept; can now serve new connections");
  _too_busy = false;
  sbp->reply (NULL);
}

//-----------------------------------------------------------------------

void
okch_t::dispatch (ptr<bool> dfp, svccb *sbp)
{
  if (*dfp) {
    okdbg_warn (ERROR, "dispatch function ignored for destroyed child\n");
    return;
  }
  
  if (!sbp) {
    chld_eof ();
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
  case OKCTL_READY:
    if (state == OKC_STATE_LAUNCH_SEQ_1) {
      state = OKC_STATE_LAUNCH_SEQ_2;
      start_chld ();
    } else {
      CH_ERROR ("Cannot process READY message; in wrong state: " << state);
    }
    sbp->reply (NULL);
    break;
  case OKCTL_REQ_ERRDOCS_2:
    myokd->req_errdoc_set_2 (sbp);
    break;
  case OKCTL_CUSTOM_1_IN: 
    myokd->custom1_in (sbp);
    break;
  case OKCTL_CUSTOM_2_IN:
    myokd->custom2_in (sbp);
    break;
  case OKCTL_GET_STATS:
    myokd->okctl_get_stats (sbp);
    break;
  case OKCTL_REENABLE_ACCEPT:
    handle_reenable_accept (sbp);
    break;
  default:
    sbp->reject (PROC_UNAVAIL);
    break;
  }
}

//-----------------------------------------------------------------------

void
okch_t::kill ()
{
  CH_CHATTER ("disconnecting from child");
  ctlx = NULL;
  clnt = NULL;
  srv = NULL;
  state = OKC_STATE_NONE;
}

//-----------------------------------------------------------------------
//

tamed void
okch_t::custom2_out (const ok_custom_data_t &in, ok_custom_res_union_t *out, 
		     evs_t ev)
{
  tvars {
    clnt_stat err;
    ok_custom_data_t dat;
    str ret;
  }

  if (clnt && !clnt->xprt ()->ateof ()) {
    twait { 
      RPC::okctl_program_1::okctl_custom_2_out(clnt, in, &dat, mkevent (err));
    }
    if (err) {
      strbuf b ("RPC error in custom2_out: ");
      b << err;
      ret = b;
      out->set_status (OK_STATUS_DEADCHILD);
    } else {
      out->set_status (OK_STATUS_OK);
      *out->dat = dat;
    }
  } else {
    strbuf b ("child in state=");
    b << state << "swallowing OKCTL_CUSTOM_1_OUT RPC.";
    ret = b;
  }
  ev->trigger (ret);
}

//-----------------------------------------------------------------------
//
// route a CUSTOM1 message out to the appropriate child
//
tamed void
okch_t::custom1_out (const ok_custom_data_t &x, evs_t ev)
{
  tvars {
    clnt_stat err;
    str ret;
    ok_xstatus_t st;
  }
  // XXX want to collect success information and so on from this guy
  // (as in repub)
  if (clnt && !clnt->xprt ()->ateof ()) {
    twait {
      RPC::okctl_program_1::okctl_custom_1_out (clnt, x, &st, mkevent (err));
    }
    if (err) {
      strbuf b ("RPC error in custom1_out: ");
      b << err;
      ret = b;
    } else if (st.status != OK_STATUS_OK) {
      strbuf b ("OKCTL error; status=");
      b << int (st.status);
      ret = b;
    }
  } else {
    strbuf b ("child in state=");
    b << state << "swallowing OKCTL_CUSTOM_1_OUT RPC.";
    ret = b;
  }
  if (ret) CH_ERROR(ret);
  ev->trigger (ret);
}

//-----------------------------------------------------------------------

void
okch_t::chld_eof ()
{
  warn << servpath << ": child process died (pid " << pid << ")\n";
  ctlx = NULL;
  srv = NULL;
  clnt = NULL;

  if (myokd && !myokd->in_shutdown ()) {
    state = OKC_STATE_CRASH;
  } else 
    state = OKC_STATE_NONE;
}

void
okch_t::to_status_xdr (oksvc_status_t *st)
{
  memset ((void *)st, 0, sizeof (*st));
  st->pid = pid;
  st->servpath = servpath;
  st->n_served = _n_sent;
  st->uptime = okwstime () - _last_restart;
}

//-----------------------------------------------------------------------

tamed void
okch_t::awaken (evb_t ev)
{
  tvars {
    bool ok;
  }
  twait { myokd->awaken (servpath, mkevent (ok));}
  ev->trigger (ok);
}

//-----------------------------------------------------------------------

tamed void
okch_t::toggle_leak_checker (ok_diagnostic_cmd_t cmd, 
			     event<ok_xstatus_typ_t>::ref ev)
{
  tvars {
    clnt_stat err;
    ok_xstatus_typ_t status;
  }
  
  if (!clnt) {
    LDCH_ERROR("cannot connect to client for leak_checker toggle");
    status = OK_STATUS_DEADCHILD;
  } else {
   
    twait {
      RPC::okctl_program_1::
	okctl_leak_checker (clnt, cmd, &status, mkevent (err));
    }

    if (err) {
      LDCH_ERROR ("RPC failure in leak_checker: " << err);
      status = OK_STATUS_ERR;
    }
  }
  ev->trigger (status);
}

//-----------------------------------------------------------------------

tamed void
okch_t::toggle_profiler (ok_diagnostic_cmd_t cmd, 
			 event<ok_xstatus_typ_t>::ref ev)
{
  tvars {
    clnt_stat err;
    ok_xstatus_typ_t status;
  }
  
  if (!clnt) {
    LDCH_ERROR("cannot connect to client for leak_checker toggle");
    status = OK_STATUS_DEADCHILD;
  } else {
   
    twait {
      RPC::okctl_program_1::
	okctl_profiler (clnt, cmd, &status, mkevent (err));
    }

    if (err) {
      LDCH_ERROR ("RPC failure in leak_checker: " << err);
      status = OK_STATUS_ERR;
    }
  }
  ev->trigger (status);
}

//-----------------------------------------------------------------------
