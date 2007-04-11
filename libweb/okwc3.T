// -*-c++-*-

#include "okwc3.h"
#include "okwc.h"
#include "tame_connectors.h"

static okwc3_dnscache_t dnscache;

//-----------------------------------------------------------------------

void
okwc3_dnscache_t::lookup (const str &n, ev_hent_t ev)
{
  ptr<okwc3_dnscache_entry_t> *entp, ent;
    
  if ((entp = _cache[n])) {
    ent = *entp;
  } else {
    ent = New refcounted<okwc3_dnscache_entry_t> (n);
    _cache.insert (n, ent);
  }
  ent->lookup (ev);
}

//-----------------------------------------------------------------------

//
// DNS resolution mechanics:
//   - Only one client should be looking up a given host at a given time.
//   - Thus, one looks up, and the rest wait for the lookup to complete.
//   - If the resolver is canceled while waiting, he should not totally
//     abandon the project; he should trigger to his caller, but keep
//     going, since the other waiters might have a longer timer.
//
tamed void
okwc3_dnscache_entry_t::lookup (ev_hent_t ev)
{
  tvars {
    ptr<hostent> he;
    int e (ARERR_TIMEOUT);
    bool completed;
    ptr<okwc3_dnscache_entry_t> hold;
  }
  hold = mkref (this);

  if (_resolving) {
    twait { wait_for_resolution (connector::cnc (mkevent (completed), ev)); }
  } else if (!_init || sfs_get_timenow() > _expires || _err != 0) {
    twait { do_resolution (connector::cnc (mkevent (completed), ev)); }
  }
  if (completed) {
    he = _he;
    e = _err;
  }
  ev->trigger (he, e);
}

//-----------------------------------------------------------------------

tamed void
okwc3_dnscache_entry_t::wait_for_resolution (evb_t ev)
{
  tvars {
    queued_cbv_t *qcb;
    outcome_t outc;
    ptr<okwc3_dnscache_entry_t> hold;
  }
  hold = mkref (this);

  twait { 
    qcb = New queued_cbv_t (connector::cnc (mkevent (), ev, &outc));
    _waiters.insert_tail (qcb);
  }

  _waiters.remove (qcb);
  if (outc == OUTCOME_CANCELLED && _waiter_remove_cb)
    (*_waiter_remove_cb) ();
  
  ev->trigger (outc == OUTCOME_SUCC);

  delete qcb;
}

//-----------------------------------------------------------------------


void
okwc3_dnscache_entry_t::commit_lookup (ptr<hostent> he, int e)
{
  _init = true;
  _he = he;
  _err = e;
  _expires = sfs_get_timenow() + _ttl;
  _resolving = false;
}

//-----------------------------------------------------------------------

/**
 * @brief Do a DNS resolution, on behalf of us and perhaps others.
 * @param cncl A canceler that might send a cancel trigger.
 * @param cb Callback to call, *even if cancelled!*.
 *
 * Note: this function calls its callback even if it gets cancelled.
 */
tamed void
okwc3_dnscache_entry_t::do_resolution (evb_t cb)
{
  tvars {
    dnsreq_t *dnsp;
    rendezvous_t<bool> rv (__FILE__, __LINE__);
    bool cancelled;
    ptr<hostent> he;
    int e (ARERR_TIMEOUT);
    bool committed_lookup (false);
    queued_cbv_t *p, *n;
    ptr<okwc3_dnscache_entry_t> hold;
  }
  hold = mkref (this);

  _resolving = true;

  dnsp = dns_hostbyname (_hostname, mkevent (rv, false, he, e));
  cb->set_cancel_notifier (mkevent (rv, true));
  twait (rv, cancelled);
  if (!cancelled) {
    committed_lookup = true;
    commit_lookup (he, e);
  }

  cb->trigger (!cancelled);

  // If we're cancelled, and there are still some waiters left, we still
  // have to wait on the outcome of the DNS resolution.  But we get
  // notified as the other waiters give up in the loop below.
  while (cancelled && _waiters.first) {
    _waiter_remove_cb = mkevent (rv, true);
    twait(rv, cancelled);
  }

  // Once all waiters have abandoned, or the answer has come back,
  // we can go ahead and commit the results of the lookup.
  if (!committed_lookup)
    commit_lookup (he, e);

  if (cancelled) {
    assert (_waiters.first == NULL);

    // Internally, stops listening on an fd and closes it.
    dnsreq_cancel (dnsp);
  } else {
    cb->set_cancel_notifier (NULL);

    if (_waiter_remove_cb) {
      _waiter_remove_cb = NULL;
    }
    for (p = _waiters.first; p; p = n) {
      n = _waiters.next (p);
      (*p->_cb) ();
    }
  }
  assert (rv.n_triggers_left () == 0);
}

//-----------------------------------------------------------------------

void
okwc3_req_t::fix_filename ()
{
  int len;
  if (!_filename || (len = _filename.len ()) == 0) {
    // empty file names --> "/"
    _filename = "/";
  } else if (_filename[0] != '/') {
    // insert leading slash if not there.
    _filename = strbuf ("/") << _filename;
  } else {
    // trunc all but the first leading slash
    const char *fn = _filename.cstr ();
    const char *cp;
    for (cp = fn; *cp == '/'; cp++)  ;
    cp--;
    _filename = str (cp, len - (cp - fn)); 
  }
}

//-----------------------------------------------------------------------

void
okwc3_req_t::format_req (strbuf &b)
{

  fix_filename ();
  str mth = get_post () ? "POST" : "GET";

  b << mth << " " << _filename << " HTTP/1." << _vers << HTTP_CRLF;
  if (_vers == 1) {
    b << "Connection: close" << HTTP_CRLF
      << "Host: " << _hostname << HTTP_CRLF
      << "User-agent: okwc/" << VERSION << HTTP_CRLF;
  }
  if (_outcookie) {
    b << "Cookie: ";
    _outcookie->encode (&b);
    b << HTTP_CRLF;
  }

  str t = get_type ();
  if (t) b << "Content-Type: " << t << HTTP_CRLF;

  const okwc3_post_t *p = get_post ();
  if (p) b << "Content-Length: " << p->len () << HTTP_CRLF;
  b << HTTP_CRLF;
  if (p) p->output (b);

}

//-----------------------------------------------------------------------

tamed void
okwc3_req_t::make_T (ptr<ahttpcon> x, evi_t ev)
{
  tvars {
    strbuf req;
    int status (HTTP_OK);
    outcome_t outc;
    ptr<okwc3_req_t> hold;
  }
  hold = mkref (this);

  format_req (req);

  twait { x->send (req, connector::cnc (mkevent (), ev, &outc)); }
  if (outc == OUTCOME_CANCELLED) {
    status = CANCELLED_STATUS;
    x->set_drained_cb (NULL);
    x->cancel ();
  }
  ev->trigger (status);
}

//-----------------------------------------------------------------------

static int
dnserr2httperr (int i)
{
  switch (i) {
  case NOERROR: return HTTP_OK;
  case ARERR_TIMEOUT: return HTTP_TIMEOUT;
  default: return HTTP_CONNECTION_FAILED;
  }
}


//-----------------------------------------------------------------------

/**
 * @brief Do the main workflow of the HTTP request
 * @param req The user's request
 * @param resp Where to stick the response
 * @param cb The callback to call when completed.
 * @return A canceller, which the caller can poke to cancel this operation
 *
 * note: Disobeys above convention, in that it does call its callback
 * even when cancelled.  Callers should take note.
 */
tamed void
okwc3_t::req_T (ptr<okwc3_req_t> req, ptr<okwc3_resp_t> resp, evi_t ev)
{
  tvars {
    int status (0);
    ptr<okwc3_t> hold;
    ptr<hostent> he;
    int dns_err;
    int fd;
    ptr<ahttpcon> x;
    tcpconnect_t *tc;
    outcome_t outc;
  }
  
  // Keep this object from being freed from underneath us.
  hold = mkref (this);
  
  twait { 
    dnscache.lookup (_hostname, connector::cnc(mkevent (he, dns_err), ev));
  }

  if (dns_err || !he) {
    status = dnserr2httperr (dns_err);
  } else {

    twait { 
      tc = tcpconnect (*(in_addr *)he->h_addr, _port, 
		       connector::cnc (mkevent (fd), ev, &outc));
    }

    if (outc == OUTCOME_CANCELLED) {
      status = CANCELLED_STATUS;
      tcpconnect_cancel (tc);
    } else if (fd < 0) {
      status = HTTP_CONNECTION_FAILED;
    } else {
      
      x = ahttpcon::alloc (fd);
      twait { req->make (x, connector::cnc (mkevent (status), ev)); }
      if (status == HTTP_OK) {
	resp->setx (x);
	twait { resp->get (connector::cnc (mkevent (status), ev)); }
      }
    }
  }

  ev->trigger (status);
}

//-----------------------------------------------------------------------

okwc3_resp_t::okwc3_resp_t ()
  : _incookies (&_abuf, OKWC_SCRATCH_SZ, _scratch),
    _hdr (&_abuf, &_incookies, OKWC_SCRATCH_SZ, _scratch) {}

//-----------------------------------------------------------------------

void
okwc3_resp_t::setx (ptr<ahttpcon> x)
{
  _x = x;
  _abuf.setsrc (New abuf_con_t (_x), true);
}

//-----------------------------------------------------------------------

tamed void
okwc3_resp_t::get_T (evi_t ev)
{
  tvars {
    int status (HTTP_OK);
    outcome_t outc;
    ptr<okwc3_resp_t> hold;
  }
  hold = mkref (this);

  // Call into the OKWS HTTP header parsing classes...
  twait { _hdr.parse (connector::cnc (mkevent (status), ev, &outc)); }

  if (outc == OUTCOME_CANCELLED) {
    status = CANCELLED_STATUS;
    _hdr.cancel ();
  } else if (status == HTTP_OK) { 
    twait { get_body (connector::cnc (mkevent (status), ev)); }
  }
  ev->trigger (status);
}

//-----------------------------------------------------------------------

tamed void
okwc3_resp_t::get_body_T (evi_t ev)
{
  tvars {
    int status (HTTP_OK);
    ptr<okwc3_resp_t> hold;
  }
  hold = mkref (this);

  // Certain HTTP 1.1 responses are chunked, so we need to accommodate
  // them with our 'chunker' class.
  if (_hdr.is_chunked ()) {
    twait { run_chunker (connector::cnc (mkevent (status), ev)); }
  } else {
    // Unchunked responses are eaten in a single gulp.
    twait { 
      eat_chunk (_hdr.get_contlen (), 
		 connector::cnc (mkevent (status), ev));
    }
  }
  twait { finished_meal (status, connector::cnc (mkevent (status), ev)); }
  ev->trigger (status);
}

//-----------------------------------------------------------------------

tamed void
okwc3_resp_t::run_chunker_T (evi_t ev)
{
  tvars {
    okwc_chunker_t *c;
    int status;
    size_t sz;
    outcome_t outc;
    ptr<okwc3_resp_t> hold;
  }
  hold = mkref (this);

  c = New okwc_chunker_t (&_abuf, OKWC_SCRATCH_SZ, _scratch);

  do { 
    twait { c->parse (connector::cnc (mkevent (status), ev, &outc)); }
    if (outc == OUTCOME_CANCELLED) {
      c->cancel ();
      status = CANCELLED_STATUS;

    } else if (status == HTTP_OK && (sz = c->get_sz ())) {
    
      twait { eat_chunk (sz, connector::cnc (mkevent (status), ev)); }
      c->next_chunk ();
    } 
  } while (status == HTTP_OK && sz);

  ev->trigger (status);
}

//-----------------------------------------------------------------------

tamed void
okwc3_resp_simple_t::eat_chunk_T (size_t sz, evi_t ev)
{
  tvars {
    str chnk;
    int status (HTTP_OK);
    outcome_t outc;
    ptr<okwc3_resp_simple_t> hold;
  }
  hold = mkref (this);

  twait { _dumper.dump (sz, connector::cnc (mkevent (chnk), ev, &outc)); }

  if (outc == OUTCOME_CANCELLED) {
    status = CANCELLED_STATUS;
    _dumper.cancel ();
  } else {
    if (chnk) _chunks.push_back (chnk);
  }

  ev->trigger (status);
}

//-----------------------------------------------------------------------

void
okwc3_resp_simple_t::finished_meal (int status, evi_t ev)
{
  if (status == HTTP_OK) {
    strbuf b;
    for (size_t i = 0; i < _chunks.size (); i++) {
      b << _chunks[i];
    }
    _body = b;
    _chunks.clear ();
  }
  ev->trigger (status);
}

//-----------------------------------------------------------------------

tamed void
okwc3_simple_t::req (str fn, okwc3_simple_ev_t ev, int v, cgi_t *c)
{
  tvars {
    ptr<okwc3_resp_simple_t> resp (New refcounted<okwc3_resp_simple_t> ());
    ptr<okwc3_req_t> rq;
    int status;
  }
  rq = New refcounted<okwc3_req_t> (_hostname, fn, v, c);
  twait { okwc3_t::req (rq, resp, connector::cnc (mkevent (status), ev)); }
  ev->trigger (status, resp);
}
