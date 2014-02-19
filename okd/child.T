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
  warn << _servpath << ":" << _brother_id << ":" << _pid << ": " << x << "\n";

//-----------------------------------------------------------------------

okch_t::okch_t (okd_t *o, okch_cluster_t *c, size_t id, okc_state_t st)
  : _myokd (o), 
    _cluster (c),
    _brother_id (id),
    _servpath (c->_servpath),
    _pid (-1), 
    _state (st),
    _destroyed (New refcounted<bool> (false)),
    _per_svc_nfd_in_xit (0),
    _n_sent (0), 
    _last_restart (okwstime ()),
    _too_busy (false),
    _generation_id (0),
    _emerg_start (0),
    _emerg_killed (false) {}

//-----------------------------------------------------------------------

okch_t::~okch_t ()
{
  *_destroyed = true;
}

//-----------------------------------------------------------------------

okch_cluster_t::okch_cluster_t (okd_t *o, str p, okc_state_t st, size_t n)
  : _myokd (o),
    _servpath (p),
    _destroyed (New refcounted<bool> (false)),
    _is_ready_looping (false),
    _n_killed (0)
{
  _children.setsize (n);
  for (size_t i = 0; i < n; i++) {
    _children[i] = New okch_t (o, this, i, st);
  }
  _myokd->insert (this);
}

//-----------------------------------------------------------------------

okch_cluster_t::~okch_cluster_t ()
{
  if (OKDBG2(OKD_SHUTDOWN)) {
    CH_CL_CHATTER ("deleting child_cluster");
  }

  _myokd->remove (this);

  *_destroyed = true;
  while (_conqueue.size ())  {
    ahttpcon_wrapper_t<ahttpcon_clone> acw (_conqueue.pop_front ());
    ref<ahttpcon_clone> x (acw.con ());
    x->declone ();
    _myokd->error (x, HTTP_SRV_ERROR);
  }

  for (size_t i = 0; i < _children.size (); i++) {
    delete _children[i];
  }
}

//-----------------------------------------------------------------------

void
okch_t::handle_overload ()
{
  time_t now = sfs_get_timenow ();
  if (!_emerg_start) {
    _emerg_start = sfs_get_timenow ();
  } else if (now > _emerg_start &&
	     _myokd->handle_overload (now - _emerg_start, this)) {
    CH_CHATTER ("killing child after " << now - _emerg_start
		<< "s of unresponiveness");
    _emerg_start = 0;
    _emerg_killed = true;
  }
}

//-----------------------------------------------------------------------

bool
okch_t::sendcon_timeout () const
{
  return (okd_sendcon_time_budget &&
	  _dispatch_times.oldest () > okd_sendcon_time_budget);
}

//-----------------------------------------------------------------------

okch_t::status_t
okch_t::get_status () const
{
  status_t ret = OK;

  if (_state == OKC_STATE_STANDBY) {  
    ret = STANDBY; // might work after startup!

  } else if (_state == OKC_STATE_HOSED) {
    ret = CRASHED;

  } else if (!ctlx || ctlx->ateof () || _state != OKC_STATE_SERVE) {
    ret = MIGHT_WORK;

  } else if (_too_busy) {
    CH_ERROR ("**OVERLOAD: Service too busy to accept new connections");
    ret = BUSY_LOOP;

  } else if (sendcon_timeout ()) {
    CH_ERROR ("**OVERLOAD: Service took too long to accept new connections");
    ret = BUSY_LOOP;

  } else if (ok_svc_fd_quota && _per_svc_nfd_in_xit > int (ok_svc_fd_quota)) {
    CH_ERROR ("**OVERLOAD: Service appears unresponsive");
    ret = BUSY_LOOP;

  } else {
    ret = OK;
  }

  return ret;
}

//-----------------------------------------------------------------------

// Try to find the child that fits best (among those available in the 
// cluster.)  Use the passed in preference if possible.  Alternatively,
// try source-IP mapping..
okch_t *
okch_cluster_t::find_best_fit_child (ref<ahttpcon_clone> xc, 
				     okch_t::status_t *statusp, 
				     int pref)
{
  okch_t *ret = NULL;
  okch_t::status_t status = okch_t::CRASHED;
  okch_t::status_t ret_status = status;
  size_t id = 0;
  size_t n = _children.size ();
  bool ok = true;

  if (pref >= 0 && size_t (pref) < n) { id = pref; }
  else if (n == 1) { id = 0; }
  else if (n > 1) { id = xc->source_hash_ip_only () % n; } 
  else {
    ok = false;
    CH_CL_ERROR ("Cannot map to service --- no such process exists");
  }

  for (size_t tries = 0; 
       ok && tries < n && ret_status != okch_t::OK; 
       tries++, id++) {

    okch_t *ch = _children[id % n];

    if (!ch) {  
      /* noop */ 
    } else if ((status = ch->get_status ()) == okch_t::CRASHED) {
      /* noop */ 
    } else if (status == okch_t::BUSY_LOOP) { 
      ch->handle_overload ();

    } else if (!ret || (ret_status != okch_t::OK && status == okch_t::OK)) {
      ret_status = status;
      ret = ch;
    }
  }

  *statusp = status;
  return ret;
}

//-----------------------------------------------------------------------

tamed void
okch_cluster_t::is_ready ()
{
  tvars {
    ptr<bool> df;
    okch_t *ch;
    ahttpcon_wrapper_t<ahttpcon_clone> acw;
    ptr<ahttpcon_clone> x;
    okch_t::status_t status;
  }
  
  if (_is_ready_looping) { return; }
  _is_ready_looping = true;
  df = _destroyed;

  status = okch_t::OK;
  while (!*df && _conqueue.size () && status == okch_t::OK) {
    acw = _conqueue[0];
    x = acw.con ();
    if ((ch = find_best_fit_child (x, &status)) && status == okch_t::OK) {
      _conqueue.pop_front ();
      twait { ch->send_con_to_service (acw, mkevent ()); }
    }
  }
  _is_ready_looping = false;
}

//-----------------------------------------------------------------------

tamed void
okch_cluster_t::clone (ahttpcon_wrapper_t<ahttpcon_clone> acw, evv_t ev,
		       int sib)
{
  tvars {
    ref<ahttpcon_clone> xc (acw.con ());
    okch_t *ch;
    okch_t::status_t status;
    int http_status (HTTP_OK);
    bool handled (false);
    bool ok;
    bool do_queue (false);
  }

  ch = find_best_fit_child (xc, &status, sib);

  switch (status) {
  case okch_t::BUSY_LOOP: http_status = HTTP_UNAVAILABLE; break;
  case okch_t::CRASHED: http_status = HTTP_SRV_ERROR; break;
  case okch_t::STANDBY:
    {
      assert (ch);
      twait { ch->awaken (mkevent (ok)); }
      if (!ok) { http_status = HTTP_SRV_ERROR; }
      else { do_queue = true; }
    }
    break;

  case okch_t::OK:
    {
      assert (ch);
      twait { ch->send_con_to_service (acw, mkevent ()); }
      handled = true;
    }
    break;
  default:
    do_queue = true;
    break;
  }

  if (!do_queue) { /* noop */ }
  else if (_conqueue.size () >= ok_con_queue_max) { 
    http_status = HTTP_SRV_ERROR; 
  } else {
    _conqueue.push_back (acw);
    handled = true;
  }

  if (!handled) {
    xc->declone ();
    _myokd->error (xc, http_status, make_generic_http_req (_servpath));
  }
  ev->trigger ();
}

//-----------------------------------------------------------------------

tamed void
okch_t::shutdown (oksig_t g, evv_t ev)
{
  tvars {
    ptr<bool> df;
  }
  df = _destroyed;

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
      _state = OKC_STATE_KILLING;

      if (clnt) {
	twait { clnt->seteofcb (mkevent ()); }
      }
    }
  }

  if (!*df && OKDBG2(OKD_SHUTDOWN)) {
    CH_CHATTER ("client shutdown completed");
  }

  ev->trigger ();
}

//-----------------------------------------------------------------------

void
okch_t::got_new_ctlx_fd (int fd, int p)
{
  _pid = p;
  ctlx = axprt_unix::alloc (fd, ok_axprt_ps);
  ctlcon (wrap (this, &okch_t::dispatch, _destroyed));
  _state = OKC_STATE_LAUNCH_SEQ_1;
  start_chld ();
}

//-----------------------------------------------------------------------

time_node_t::time_node_t () : _time (sfs_get_timenow ()) {}

//-----------------------------------------------------------------------

time_t time_list_t::oldest () const 
{
  time_node_t *n = first;
  time_t ret = 0;
  time_t now = sfs_get_timenow ();
  if (n && now > n->_time) { ret = now - n->_time; }
  return ret;
}

//-----------------------------------------------------------------------

time_node_t *
time_list_t::launch () 
{ 
  time_node_t *ret = New time_node_t ();
  insert_tail (ret);
  return ret;
}

//-----------------------------------------------------------------------

void time_list_t::finished (time_node_t *n) { remove (n); delete (n); }

//-----------------------------------------------------------------------

tamed void
okch_t::send_con_to_service (ahttpcon_wrapper_t<ahttpcon_clone> acw, evv_t ev)
{
  tvars {
    okctl_sendcon_arg2_t arg;
    okctl_sendcon_res_t res;
    clnt_stat err;
    ptr<bool> df;
    ref<ahttpcon_clone> xc (acw.con ());
    int fd (-1);
    time_node_t *tn;
  }

  df = _destroyed;
  _per_svc_nfd_in_xit ++;

  assert (clnt);
  assert (ctlx);

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

    arg.scraps = acw.con()->request_bytes;

    // Take the FD away from the ahttpcon; is OURS now.
    fd = xc->takefd ();

    // Second argument false: DON'T close the FD inside of sfs libs,
    // do it explicitly below...
    ctlx->sendfd (fd, false);

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

      // Keep track of last send and last recv, that way, we know
      // if the server is F'ed!!
      tn = _dispatch_times.launch ();
      twait { 
	RPC::okctl_program_1::okctl_send_con2 (clnt, &arg, &res, mkevent(err)); 
      }
      _dispatch_times.finished (tn);

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
  }

  if (!*df) {
    _per_svc_nfd_in_xit --;
  }

  // Only close the FD once it's been ACK'ed by the service (i.e., the RPC
  // has returned).
  if (fd >= 0) { 
    close (fd); 
  }

  ev->trigger ();

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
  if (_state == OKC_STATE_LAUNCH_SEQ_2 && ctlx) {

    // debug messages
    if (OKDBG2 (OKD_STARTUP))
      CH_CHATTER ("child changed to serve status; conQsize=" 
		  << _cluster->qlen ());
    
    _state = OKC_STATE_SERVE;

    reset_accounting ();
    _cluster->is_ready ();
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

  if (_state == OKC_STATE_KILLING) {
    if (OKDBG2(OKD_SHUTDOWN))
      CH_CHATTER ("ignore RPC sent after child killed");
    sbp->ignore ();
    return;
  }

  switch (p) {
  case OKCTL_READY:
    if (_state == OKC_STATE_LAUNCH_SEQ_1) {
      _state = OKC_STATE_LAUNCH_SEQ_2;
      start_chld ();
    } else {
      CH_ERROR ("Cannot process READY message; in wrong state: " << _state);
    }
    sbp->reply (NULL);
    break;
  case OKCTL_REQ_ERRDOCS_2:
    _myokd->req_errdoc_set_2 (sbp);
    break;
  case OKCTL_CUSTOM_1_IN: 
    _myokd->custom1_in (sbp);
    break;
  case OKCTL_CUSTOM_2_IN:
    _myokd->custom2_in (sbp);
    break;
  case OKCTL_GET_STATS:
    _myokd->okctl_get_stats (sbp);
    break;
  case OKCTL_KEEPALIVE:
    {
      int fd = ctlx->recvfd();
      if (fd < 0) {
	CH_ERROR ("Got invalid FD in keepalive passback");
      } 
      _myokd->handle_keepalive (fd, sbp);
    }
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
  _state = OKC_STATE_NONE;
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
    b << _state << "swallowing OKCTL_CUSTOM_1_OUT RPC.";
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
    b << _state << "swallowing OKCTL_CUSTOM_1_OUT RPC.";
    ret = b;
  }
  if (ret) CH_ERROR(ret);
  ev->trigger (ret);
}

//-----------------------------------------------------------------------

void
okch_t::chld_eof ()
{
  warn << _servpath << ": child process died (pid " << _pid << ")\n";
  ctlx = NULL;
  srv = NULL;
  clnt = NULL;

  if (_myokd && !_myokd->in_shutdown ()) {
    _state = OKC_STATE_CRASH;
  } else {
    _state = OKC_STATE_NONE;
  }
}

//-----------------------------------------------------------------------

void
okch_t::to_status_xdr (oksvc_status_t *st)
{
  memset ((void *)st, 0, sizeof (*st));
  st->pid = _pid;
  proc_to_xdr (&st->proc);
  st->n_served = _n_sent;
  st->uptime = okwstime () - _last_restart;
}

//-----------------------------------------------------------------------

tamed void
okch_t::awaken (evb_t ev)
{
  tvars {
    bool ok;
    oksvc_proc_t p;
  }
  proc_to_xdr (&p);
  twait { _myokd->awaken (p, mkevent (ok));}
  if (ok && (!ctlx || !clnt)) {
    CH_ERROR ("connection to child died during lazy wakeup");
    ok = false;
  }
  ev->trigger (ok);
}

//-----------------------------------------------------------------------

tamed void
okch_t::send_msg (str m, evs_t ev)
{
  tvars {
    clnt_stat err;
    okctl_send_msg_arg_t arg;
    okctl_send_msg_res_t res;
    str status;
  }
    
  if (!clnt) {
    LDCH_ERROR("cannot connect to client for send_msg");
    status = "dead child";
  } else {
    arg.msg = m;
    twait {
      RPC::okctl_program_1::okctl_send_msg (clnt, arg, &res, mkevent (err));
    }
    
    if (err) {
      strbuf b;
      b << "RPC failure in send_msg: " << err << "\n";
      LDCH_ERROR (b);
      status = b;
    } else if (!res.ok) {
      status = *res.err;
    }
  }
  ev->trigger (status);
}

//-----------------------------------------------------------------------

tamed void
okch_t::diagnostic (ok_diagnostic_domain_t dd, ok_diagnostic_cmd_t cmd, 
		    event<ok_xstatus_typ_t>::ref ev)
{
  tvars {
    clnt_stat err;
    ok_xstatus_typ_t status;
    okctl_diagnostic_arg_t arg;
  }
  
  if (!clnt) {
    LDCH_ERROR("cannot connect to client for leak_checker toggle");
    status = OK_STATUS_DEADCHILD;
  } else {
    arg.domain = dd;
    arg.cmd = cmd;
   
    twait {
      RPC::okctl_program_1::
	okctl_diagnostic (clnt, arg, &status, mkevent (err));
    }

    if (err) {
      LDCH_ERROR ("RPC failure in leak_checker: " << err);
      status = OK_STATUS_ERR;
    }
  }
  ev->trigger (status);
}

//-----------------------------------------------------------------------

void
okch_cluster_t::set_states (okc_state_t s)
{
  for (size_t i = 0; i < _children.size (); i++) {
    _children[i]->set_state (s);
  }
}

//-----------------------------------------------------------------------

void
servtab_t::dump (vec<okch_t *> *out)
{
  ihash_iterator_t<okch_cluster_t, servtab_t> it (*this);
  okch_cluster_t *o;
  while ((o = it.next ())) {
    for (size_t i = 0; i < o->n_children (); i++) {
      out->push_back (o->child (i));
    }
  }
} 

//-----------------------------------------------------------------------

okch_t *
servtab_t::get (const oksvc_proc_t &p)
{
  okch_cluster_t *cl = (*this)[p.name];
  int id = p.brother_id;
  okch_t *ret = NULL;
  if (id >= 0 && cl && id < int (cl->n_children ())) { ret = cl->child (id); }
  return ret;
}

//-----------------------------------------------------------------------

size_t
servtab_t::mget (const oksvc_proc_t &p, vec<okch_t *> *out)
{
  okch_cluster_t *cl = (*this)[p.name];
  size_t n = 0;
  if (!cl) { /* noop */ }
  else if (p.brother_id < 0){
    for (size_t i = 0; i < cl->n_children (); i++) { 
      out->push_back (cl->child (i)); 
      n++;
    }
  } else if (p.brother_id < int (cl->n_children ())) {
    out->push_back (cl->child (p.brother_id));
    n++;
  }
  return n;
}

//-----------------------------------------------------------------------

void
okch_t::proc_to_xdr (oksvc_proc_t *p) const
{
  p->name = _servpath;
  p->brother_id = _brother_id;
  p->num_brothers = 0;
}

//-----------------------------------------------------------------------

void
okch_t::killed ()
{
  _state = OKC_STATE_KILLED;
  _cluster->killed (_brother_id, this);
}

//-----------------------------------------------------------------------

void
okch_cluster_t::killed (size_t i, okch_t *ch)
{
  assert (_children[i] == ch);
  if ( ++ _n_killed == _children.size ()) {
    delete this;
  }
}

//-----------------------------------------------------------------------

void 
okch_t::to_svc_descriptor (oksvc_descriptor_t *d) const
{
  proc_to_xdr (&d->proc);
  d->pid = _pid;
}

//-----------------------------------------------------------------------
