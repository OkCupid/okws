// -*-c++-*-

#include "okwc2.h"
#include "okwc.h"

static okwc2_dnscache_t dnscache;


//
// Coding style:
//  -For functions that we can modify the signatures for, pass the
//   canceller object as a parameter.
//  -For those we can't, need to use cancellable_wait
//  -Canceled calls should not call their callbacks
//

//-----------------------------------------------------------------------

TAMED void
cancellable_wait (ptr<canceller_t> cncl, coordgroup_t<bool> G, cbb cb)
{
  VARS {
    bool cancelled;
  }

  cncl->wait (@[G,true]());
  WAIT (G, cancelled);
  G.remove_var ();
  if (!cancelled) cncl->clear ();

  SIGNAL (cb, cancelled);
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

TAMED void
okwc2_dnscache_entry_t::lookup (ptr<canceller_t> cncl, cbhent cb)
{
  VARS {
    ptr<hostent> he;
    int e (ARERR_TIMEOUT);
    bool completed;
  }

  if (_resolving) {
    BLOCK { wait_for_resolution (cncl, @(completed)); }
  } else if (!_init || timenow > _expires || _err != 0) {
    BLOCK { do_resolution (cncl, @(completed)); }
  }
  if (completed) {
    he = _he;
    e = _err;
  }
  SIGNAL (cb, he, e);
}

//-----------------------------------------------------------------------

TAMED void
okwc2_dnscache_entry_t::wait_for_resolution (ptr<canceller_t> cncl, cbb cb)
{
  VARS {
    coordgroup_t<bool> G (__FILE__, __LINE__);
    bool cancelled;
    queued_cbv_t *qcb;
  }
  qcb = New queued_cbv_t (@[G,false]());
  _waiters.insert_tail (qcb);
  BLOCK { cancellable_wait (cncl, G, @(cancelled)); }

  if (cancelled) {
    _waiters.remove (qcb);
    if (_waiter_remove_cb)
      (*_waiter_remove_cb) ();
  }

  SIGNAL (cb, !cancelled);
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

TAMED void
okwc2_dnscache_entry_t::do_resolution (ptr<canceller_t> cncl, cbb cb)
{
  VARS {
    dnsreq_t *dnsp;
    coordgroup_t<bool> G (__FILE__, __LINE__);
    bool cancelled;
    ptr<hostent> he;
    int e (ARERR_TIMEOUT);
    queued_cbv_t *qcb;
    bool committed_lookup (false);
  }

  _resolving = true;

  dnsp = dns_hostbyname (_hostname, @[G,false](he, e));
  cncl->wait (@[G,true]());
  WAIT (G, cancelled);
  if (!cancelled) {
    cncl->clear ();
    G.remove_var ();
    committed_lookup = true;
    commit_lookup (he, e);
  }
    
  SIGNAL (cb, !cancelled);

  while (cancelled && _waiters.first) {
    _waiter_remove_cb = @[G,true]();
    WAIT(G,cancelled);
  }

  if (!committed_lookup)
    commit_lookup (he, e);

  if (cancelled) {
    dnsreq_cancel (dnsp);
    G.remove_var ();
  } else {
    if (_waiter_remove_cb) {
      _waiter_remove_cb = NULL;
      G.remove_var ();
    }
    while ((qcb = _waiters.first)) {
      _waiters.remove (qcb);
      (*qcb->_cb) ();
    }
  }
  assert (G.n_vars_left () == 0);
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

TAMED void
okwc2_req_t::make_T (ptr<canceller_t> cncl, ptr<ahttpcon> x, cbi cb)
{
  VARS {
    strbuf req;
    int status (HTTP_OK);
    coordgroup_t<bool> G (__FILE__, __LINE__);
    bool cancelled;
  }

  format_req (req);
  
  x->send (req, @[G,false]());
  BLOCK { cancellable_wait (cncl, G, @(cancelled)); }

  if (cancelled) {
    status = CANCELLED_STATUS;
    x->set_drained_cb (NULL);
    x->cancel ();
  }
  SIGNAL (cb, status);
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

TAMED ptr<canceller_t>
okwc2_t::req_T (ptr<okwc2_req_t> req, ptr<okwc2_resp_t> resp, cbi cb)
{
  VARS {
    int status (0);
    ptr<okwc2_t> hold;
    ptr<hostent> he;
    int dns_err;
    coordgroup_t<bool> G (__FILE__, __LINE__);
    int fd;
    bool cancelled;
    ptr<ahttpcon> x;
    tcpconnect_t *tc;
    ptr<canceller_t> cncl (New refcounted<canceller_t> ());
  }
  DEFAULT_RETURN { return cncl; }
  
  // Keep this object from being freed from underneath us.
  hold = mkref (this);

  BLOCK { dnscache.lookup (cncl, _hostname, @(he, dns_err)); }

  if (dns_err || !he) {
    status = dnserr2httperr (dns_err);
  } else {
    
    tc = tcpconnect (*(in_addr *)he->h_addr, _port, @[G, false](fd));
    BLOCK { cancellable_wait (cncl, G, @(cancelled)); }

    if (cancelled) {
      status = CANCELLED_STATUS;
      tcpconnect_cancel (tc);
    } else if (fd < 0) {
      status = HTTP_CONNECTION_FAILED;
    } else {
      
      x = ahttpcon::alloc (fd);
      BLOCK { req->make (cncl, x, @(status)); }
      if (status == HTTP_OK) {
	resp->setx (x);
	BLOCK { resp->get (cncl, @(status)); }
      }
    }
  }

  // Always signal, even if cancelled...
  cncl->toolate ();
  SIGNAL (cb, status);
}

//-----------------------------------------------------------------------

TAMED void
okwc2_t::timed_req_T (ptr<okwc2_req_t> r, ptr<okwc2_resp_t> resp, int to, 
		      cbi cb)
{
  VARS {
    int status;
    coordgroup_t<bool> G (__FILE__, __LINE__);
    timecb_t *tcb;
    bool timedout ;
    ptr<canceller_t> cncl;
  }

  cncl = req (r, resp, @[G,false](status));
  if (to != 0) 
    tcb = delaycb (to, 0, @[G,true]() );

  WAIT (G, timedout);
  if (timedout) {

    cncl->cancel (); 

    // Wait for the callback from req to set status
    WAIT (G, timedout);
    assert (!timedout);

  } else {
    timecb_remove (tcb);
    G.remove_var ();
  }

  SIGNAL (cb, status);
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

TAMED void
okwc2_resp_t::get_T (ptr<canceller_t> cncl, cbi cb)
{
  VARS {
    int status (HTTP_OK);
    coordgroup_t<bool> G (__FILE__, __LINE__);
    bool cancelled;
  }

  _hdr.parse (@[G,false](status)); 
  BLOCK { cancellable_wait (cncl, G, @(cancelled)); }
  if (cancelled) {
    status = CANCELLED_STATUS;
    _hdr.cancel ();
  } else if (status == HTTP_OK) { 
    BLOCK { get_body (cncl, @(status)); }
  }
  SIGNAL (cb, status);
}

//-----------------------------------------------------------------------

TAMED void
okwc2_resp_t::get_body_T (ptr<canceller_t> cncl, cbi cb)
{
  VARS {
    int status (HTTP_OK);
  }
  if (_hdr.is_chunked ()) {
    BLOCK { run_chunker (cncl, @(status)); }
  } else {
    BLOCK { eat_chunk (cncl, _hdr.get_contlen (), @(status)); }
  }
  BLOCK { finished_meal (cncl, status, @(status)); }
  SIGNAL (cb, status);
}

//-----------------------------------------------------------------------

TAMED void
okwc2_resp_t::run_chunker_T (ptr<canceller_t> cncl, cbi cb)
{
  VARS {
    okwc_chunker_t *c;
    int status;
    size_t sz;
    bool cancelled;
    coordgroup_t<bool> G (__FILE__, __LINE__);
  }

  c = New okwc_chunker_t (&_abuf, OKWC_SCRATCH_SZ, _scratch);

  do { 
    c->parse (@[G,false] (status));
    BLOCK { cancellable_wait (cncl, G, @(cancelled)); }
    if (cancelled) {
      c->cancel ();
      status = CANCELLED_STATUS;

    } else if (status == HTTP_OK && (sz = c->get_sz ())) {
    
      BLOCK { eat_chunk (cncl, sz, @(status)); }
      c->next_chunk ();
    } 
  } while (status == HTTP_OK && sz);

  SIGNAL (cb, status);
}

//-----------------------------------------------------------------------

TAMED void
okwc2_resp_simple_t::eat_chunk_T (ptr<canceller_t> cncl, size_t sz, cbi cb)
{
  VARS {
    str chnk;
    coordgroup_t<bool> G (__FILE__, __LINE__);
    bool cancelled, dummy;
    int status (HTTP_OK);
  }

  _dumper.dump (sz, @[G, false](chnk));
  cncl->wait (@[G,true]());
  WAIT (G,cancelled);

  if (cancelled) {
    status = CANCELLED_STATUS;
    _dumper.cancel ();
  } else {
    G.remove_var ();
    cncl->clear ();
    if (chnk) _chunks.push_back (chnk);
  }

  SIGNAL (cb, status);

  // When you cancel the dumper, it still calls you back, so we need
  // to wait for this callback.
  if (G.n_vars_left ()) {
    assert (cancelled);
    WAIT (G, dummy);
    assert (! G.n_vars_left ());
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
  SIGNAL (cb, status);
}

//-----------------------------------------------------------------------

TAMED void
okwc2_simple_t::req (str fn, okwc2_simple_cb_t cb, int to, int v, cgi_t *c)
{
  VARS {
    ptr<okwc2_resp_simple_t> resp (New refcounted<okwc2_resp_simple_t> ());
    ptr<okwc2_req_t> req;
    int status;
  }
  req = New refcounted<okwc2_req_t> (_hostname, fn, v, c);
  BLOCK { timed_req (req, resp, to, @(status)); }
  SIGNAL (cb, status, resp);
}
