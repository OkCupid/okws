// -*-c++-*-

#include "okwc2.h"
#include "okwc.h"

static okwc2_dnscache_t dnscache;

//
// Coding style / convention:
//
//  -For functions that we can modify the signatures for, pass the
//   canceller object as a parameter.
//
//  -For those we can't, need to use cancellable_wait
//    - Examples include DM's DNS resolution code, OKWS's 
//      HTTP transport objects, and DM's tcpconnect.
//
//  - Canceled calls should not call their callbacks (with certain exceptions)
//
//-----------------------------------------------------------------------

tamed void
cancellable_wait (ptr<canceller_t> cncl, rendezvous_t<bool> G, cbb cb)
{
  tvars {
    bool cancelled;
  }

  cncl->wait (mkevent (G,true));
  twait (G, cancelled);
  if (!cancelled) cncl->clear ();

  TRIGGER (cb, cancelled);
}


//-----------------------------------------------------------------------

void
okwc2_dnscache_t::lookup (ptr<canceller_t> cncl, const str &n, cbhent cb)
{
  ptr<okwc2_dnscache_entry_t> *entp, ent;
    
  if ((entp = _cache[n])) {
    ent = *entp;
  } else {
    ent = New refcounted<okwc2_dnscache_entry_t> (n);
    _cache.insert (n, ent);
  }
  ent->lookup (cncl, cb);
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
okwc2_dnscache_entry_t::lookup (ptr<canceller_t> cncl, cbhent cb)
{
  tvars {
    ptr<hostent> he;
    int e (ARERR_TIMEOUT);
    bool completed;
  }

  if (_resolving) {
    twait { wait_for_resolution (cncl, mkevent (completed)); }
  } else if (!_init || timenow > _expires || _err != 0) {
    twait { do_resolution (cncl, mkevent (completed)); }
  }
  if (completed) {
    he = _he;
    e = _err;
  }
  TRIGGER (cb, he, e);
}

//-----------------------------------------------------------------------

tamed void
okwc2_dnscache_entry_t::wait_for_resolution (ptr<canceller_t> cncl, cbb cb)
{
  tvars {
    rendezvous_t<bool> G (__FILE__, __LINE__);
    bool cancelled;
    queued_cbv_t *qcb;
  }
  qcb = New queued_cbv_t (mkevent (G,false));
  _waiters.insert_tail (qcb);
  twait { cancellable_wait (cncl, G, mkevent (cancelled)); }

  if (cancelled) {
    _waiters.remove (qcb);
    if (_waiter_remove_cb)
      (*_waiter_remove_cb) ();
  }

  TRIGGER (cb, !cancelled);
  delete qcb;
}

//-----------------------------------------------------------------------


void
okwc2_dnscache_entry_t::commit_lookup (ptr<hostent> he, int e)
{
  _init = true;
  _he = he;
  _err = e;
  _expires = timenow + _ttl;
  _resolving = false;
}

/**
 * @brief Do a DNS resolution, on behalf of us and perhaps others.
 * @param cncl A canceler that might send a cancel trigger.
 * @param cb Callback to call, *even if cancelled!*.
 *
 * Note: this function calls its callback even if it gets cancelled.
 */
tamed void
okwc2_dnscache_entry_t::do_resolution (ptr<canceller_t> cncl, cbb cb)
{
  tvars {
    dnsreq_t *dnsp;
    rendezvous_t<bool> G (__FILE__, __LINE__);
    bool cancelled;
    ptr<hostent> he;
    int e (ARERR_TIMEOUT);
    queued_cbv_t *qcb;
    bool committed_lookup (false);
  }

  _resolving = true;

  dnsp = dns_hostbyname (_hostname, mkevent (G,false, he, e));
  cncl->wait (mkevent (G,true));
  twait (G, cancelled);
  if (!cancelled) {
    cncl->clear ();
    committed_lookup = true;
    commit_lookup (he, e);
  }
    
  TRIGGER (cb, !cancelled);

  // If we're cancelled, and there are still some waiters left, we still
  // have to wait on the outcome of the DNS resolution.  But we get
  // notified as the other waiters give up in the loop below.
  while (cancelled && _waiters.first) {
    _waiter_remove_cb = mkevent (G,true);
    twait(G,cancelled);
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
    if (_waiter_remove_cb) {
      _waiter_remove_cb = NULL;
    }
    while ((qcb = _waiters.first)) {
      _waiters.remove (qcb);
      (*qcb->_cb) ();
    }
  }
  assert (G.n_triggers_left () == 0);
}

//-----------------------------------------------------------------------

void
okwc2_req_t::fix_filename ()
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
okwc2_req_t::format_req (strbuf &b)
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

  const okwc2_post_t *p = get_post ();
  if (p) b << "Content-Length: " << p->len () << HTTP_CRLF;
  b << HTTP_CRLF;
  if (p) p->output (b);

}

//-----------------------------------------------------------------------

tamed void
okwc2_req_t::make_T (ptr<canceller_t> cncl, ptr<ahttpcon> x, cbi cb)
{
  tvars {
    strbuf req;
    int status (HTTP_OK);
    rendezvous_t<bool> G (__FILE__, __LINE__);
    bool cancelled;
  }

  format_req (req);
  
  x->send (req, mkevent (G,false));
  twait { cancellable_wait (cncl, G, mkevent (cancelled)); }

  if (cancelled) {
    status = CANCELLED_STATUS;
    x->set_drained_cb (NULL);
    x->cancel ();
  }
  TRIGGER (cb, status);
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
tamed ptr<canceller_t>
okwc2_t::req_T (ptr<okwc2_req_t> req, ptr<okwc2_resp_t> resp, cbi cb)
{
  tvars {
    int status (0);
    ptr<okwc2_t> hold;
    ptr<hostent> he;
    int dns_err;
    rendezvous_t<bool> G (__FILE__, __LINE__);
    int fd;
    bool cancelled;
    ptr<ahttpcon> x;
    tcpconnect_t *tc;
    ptr<canceller_t> cncl (New refcounted<canceller_t> ());
  }
  DEFAULT_RETURN { return cncl; }
  
  // Keep this object from being freed from underneath us.
  hold = mkref (this);

  twait { dnscache.lookup (cncl, _hostname, mkevent (he, dns_err)); }

  if (dns_err || !he) {
    status = dnserr2httperr (dns_err);
  } else {
    
    tc = tcpconnect (*(in_addr *)he->h_addr, _port, mkevent (G, false, fd));
    twait { cancellable_wait (cncl, G, mkevent (cancelled)); }

    if (cancelled) {
      status = CANCELLED_STATUS;
      tcpconnect_cancel (tc);
    } else if (fd < 0) {
      status = HTTP_CONNECTION_FAILED;
    } else {
      
      x = ahttpcon::alloc (fd);
      twait { req->make (cncl, x, mkevent (status)); }
      if (status == HTTP_OK) {
	resp->setx (x);
	twait { resp->get (cncl, mkevent (status)); }
      }
    }
  }

  // Always trigger, even if cancelled...
  cncl->toolate ();
  TRIGGER (cb, status);
}

//-----------------------------------------------------------------------

/**
 * @brief Make an HTTP req on a timer.
 * @param r The req to make
 * @param resp The response to stick the result into.
 * @param to The timeout value in seconds.
 * @param cbi The callback to call when finished.
 */
tamed void
okwc2_t::timed_req_T (ptr<okwc2_req_t> r, ptr<okwc2_resp_t> resp, int to, 
		      cbi cb)
{
  tvars {
    int status;
    rendezvous_t<bool> G (__FILE__, __LINE__);
    timecb_t *tcb (NULL);
    bool timedout ;
    ptr<canceller_t> cncl;
  }

  cncl = req (r, resp, mkevent (G,false, status));
  if (to != 0) 
    tcb = delaycb (to, 0, mkevent (G,true) );

  twait (G, timedout);
  if (timedout) {

    cncl->cancel (); 

    // Wait for the callback from req to set status
    twait (G, timedout);
    assert (!timedout);

  } else if (tcb) {
    timecb_remove (tcb);
  }

  TRIGGER (cb, status);
}

//-----------------------------------------------------------------------

okwc2_resp_t::okwc2_resp_t ()
  : _incookies (&_abuf, OKWC_SCRATCH_SZ, _scratch),
    _hdr (&_abuf, &_incookies, OKWC_SCRATCH_SZ, _scratch) {}

void
okwc2_resp_t::setx (ptr<ahttpcon> x)
{
  _x = x;
  _abuf.setsrc (New abuf_con_t (_x), true);
}

//-----------------------------------------------------------------------

tamed void
okwc2_resp_t::get_T (ptr<canceller_t> cncl, cbi cb)
{
  tvars {
    int status (HTTP_OK);
    rendezvous_t<bool> G (__FILE__, __LINE__);
    bool cancelled;
  }

  // Call into the OKWS HTTP header parsing classes...
  _hdr.parse (mkevent (G,false, status)); 

  twait { cancellable_wait (cncl, G, mkevent (cancelled)); }

  if (cancelled) {
    status = CANCELLED_STATUS;
    _hdr.cancel ();
  } else if (status == HTTP_OK) { 
    twait { get_body (cncl, mkevent (status)); }
  }
  TRIGGER (cb, status);
}

//-----------------------------------------------------------------------

tamed void
okwc2_resp_t::get_body_T (ptr<canceller_t> cncl, cbi cb)
{
  tvars {
    int status (HTTP_OK);
  }

  // Certain HTTP 1.1 responses are chunked, so we need to accommodate
  // them with our 'chunker' class.
  if (_hdr.is_chunked ()) {
    twait { run_chunker (cncl, mkevent (status)); }
  } else {
    // Unchunked responses are eaten in a single gulp.
    twait { eat_chunk (cncl, _hdr.get_contlen (), mkevent (status)); }
  }
  twait { finished_meal (cncl, status, mkevent (status)); }
  TRIGGER (cb, status);
}

//-----------------------------------------------------------------------

tamed void
okwc2_resp_t::run_chunker_T (ptr<canceller_t> cncl, cbi cb)
{
  tvars {
    okwc_chunker_t *c;
    int status;
    size_t sz;
    bool cancelled;
    rendezvous_t<bool> G (__FILE__, __LINE__);
  }

  c = New okwc_chunker_t (&_abuf, OKWC_SCRATCH_SZ, _scratch);

  do { 
    c->parse (mkevent (G,false, status));
    twait { cancellable_wait (cncl, G, mkevent (cancelled)); }
    if (cancelled) {
      c->cancel ();
      status = CANCELLED_STATUS;

    } else if (status == HTTP_OK && (sz = c->get_sz ())) {
    
      twait { eat_chunk (cncl, sz, mkevent (status)); }
      c->next_chunk ();
    } 
  } while (status == HTTP_OK && sz);

  TRIGGER (cb, status);
}

//-----------------------------------------------------------------------

tamed void
okwc2_resp_simple_t::eat_chunk_T (ptr<canceller_t> cncl, size_t sz, cbi cb)
{
  tvars {
    str chnk;
    rendezvous_t<bool> G (__FILE__, __LINE__);
    bool cancelled, dummy;
    int status (HTTP_OK);
  }

  _dumper.dump (sz, mkevent (G, false, chnk));
  cncl->wait (mkevent (G,true));
  twait (G,cancelled);

  if (cancelled) {
    status = CANCELLED_STATUS;
    _dumper.cancel ();
  } else {
    cncl->clear ();
    if (chnk) _chunks.push_back (chnk);
  }

  TRIGGER (cb, status);

  // When you cancel the dumper, it still calls you back, so we need
  // to wait for this callback.
  if (G.n_triggers_left ()) {
    assert (cancelled);
    twait (G, dummy);
    assert (! G.n_triggers_left ());
  }
}

//-----------------------------------------------------------------------

void
okwc2_resp_simple_t::finished_meal (ptr<canceller_t> cncl, int status, cbi cb)
{
  if (status == HTTP_OK) {
    strbuf b;
    for (size_t i = 0; i < _chunks.size (); i++) {
      b << _chunks[i];
    }
    _body = b;
    _chunks.clear ();
  }
  TRIGGER (cb, status);
}

//-----------------------------------------------------------------------

tamed void
okwc2_simple_t::req (str fn, okwc2_simple_cb_t cb, int to, int v, cgi_t *c)
{
  tvars {
    ptr<okwc2_resp_simple_t> resp (New refcounted<okwc2_resp_simple_t> ());
    ptr<okwc2_req_t> req;
    int status;
  }
  req = New refcounted<okwc2_req_t> (_hostname, fn, v, c);
  twait { timed_req (req, resp, to, mkevent (status)); }
  TRIGGER (cb, status, resp);
}
