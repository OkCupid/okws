// -*-c++-*-

#include "okwc3.h"
#include "okwc.h"
#include "tame_connectors.h"
#include "rxx.h"
#include "oksslcon.h"
#include "okconst.h"

namespace okwc3 {

static dnscache_t dnscache;

//-----------------------------------------------------------------------

void
dnscache_t::lookup (const str &n, ev_hent_t ev)
{
  ptr<dnscache_entry_t> *entp, ent;
  
  if ((entp = _cache[n])) {
    ent = *entp;
  } else {
    ent = New refcounted<dnscache_entry_t> (n);
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
dnscache_entry_t::lookup (ev_hent_t ev)
{
  tvars {
    ptr<hostent> he;
    int e (ARERR_TIMEOUT);
    bool completed (false);
    ptr<dnscache_entry_t> hold;
  }
  hold = mkref (this);

  if (_resolving) {
    twait { wait_for_resolution (connector::cnc (mkevent (completed), ev)); }
  } else if (!_init || sfs_get_timenow() > _expires || _err != 0) {
    twait { do_resolution (connector::cnc (mkevent (completed), ev)); }
  } else {
    completed = true; // hit cache!
  }

  if (completed) {
    he = _he;
    e = _err;
  }
  ev->trigger (he, e);
}

//-----------------------------------------------------------------------

tamed void
dnscache_entry_t::wait_for_resolution (evb_t ev)
{
  tvars {
    queued_cbv_t *qcb;
    outcome_t outc;
    ptr<dnscache_entry_t> hold;
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
dnscache_entry_t::commit_lookup (ptr<hostent> he, int e)
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
dnscache_entry_t::do_resolution (evb_t cb)
{
  tvars {
    dnsreq_t *dnsp;
    rendezvous_t<bool> rv (__FILE__, __LINE__);
    bool cancelled;
    ptr<hostent> he;
    int e (ARERR_TIMEOUT);
    bool committed_lookup (false);
    queued_cbv_t *p, *n;
    ptr<dnscache_entry_t> hold;
  }
  hold = mkref (this);

  _resolving = true;

  dnsp = dns_hostbyname (_hostname, mkevent (rv, false, he, e));
  cb->set_cancel_notifier (mkevent (rv, true));
  twait (rv, cancelled);
  if (!cancelled) {
    committed_lookup = true;
    commit_lookup (he, e);

    // since we're about to trigger cb, don't need to worry about whether
    // it's cancelled any more.  In the case of cancelation, don't the
    // cancel notifier is automatically NULLed out.
    cb->set_cancel_notifier (NULL);
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

str
reqinfo_direct_t::get_hdr_hostname () const
{
  str ret;
  if (_port == std_port (false)) {
    ret = _hostname;
  } else {
    strbuf b;
    b << _hostname << ":" << _port;
    ret = b;
  }
  return ret;
}

//-----------------------------------------------------------------------

void
req_t::format_req (strbuf &b, bool ka)
{

  str fn = get_filename ();
  str mth = get_post () ? "POST" : "GET";
  htpv_t vers = get_version ();

  b << mth << " " << fn << " HTTP/1." << vers << HTTP_CRLF;
  if (vers == 1) {
    const char *conn = ka ? "keep-alive" : "closed";
    b << "Connection: " << conn  << HTTP_CRLF;
  }

  /*
   * Be polite like wget and always send the following two headers
   */
  str hn = get_hdr_hostname ();
  b << "Host: " << hn << HTTP_CRLF
    << "User-agent: okwc/" << VERSION << HTTP_CRLF;

  const cgi_t *oc = get_outcookie ();
  
  if (oc) {
    b << "Cookie: ";
    oc->encode (&b);
    b << HTTP_CRLF;
  }

  str t = get_type ();
  if (!t && get_post ()) t = "application/x-www-form-urlencoded";
  if (t) b << "Content-Type: " << t << HTTP_CRLF;

  const post_t *p = get_post ();
  size_t len = 0;
  if (p) {
    len = p->len ();
  }
  b << "Content-Length: " << len << HTTP_CRLF;

  const vec<str> *v = get_extra_headers ();
  for (size_t i = 0; v && i < v->size (); i++) {
    b << (*v)[i] << HTTP_CRLF;
  }
  b << HTTP_CRLF;

  if (p) p->output (b);
}

//-----------------------------------------------------------------------

tamed void
req_t::make_T (ptr<ok_xprt_base_t> x, bool ka, evi_t ev)
{
  tvars {
    ptr<strbuf> req;
    int status (HTTP_OK);
    outcome_t outc;
    ptr<req_t> hold;
    bool r;
  }
  hold = mkref (this);
  req = New refcounted<strbuf> ();

  format_req (*req, ka);

  twait { x->drain_to_network (req, connector::cnc (mkevent (r), ev, &outc)); }
  if (outc == OUTCOME_CANCELLED) {
    status = CANCELLED_STATUS;
    x->drain_cancel ();
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
  default: return HTTP_DNS_FAILED;
  }
}


//-----------------------------------------------------------------------


/**
 * @brief Do the main workflow of the HTTP request
 * @param req The user's request
 * @param resp Where to stick the response
 * @param cb The callback to call when completed.
 *
 * note: Disobeys above convention, in that it does call its callback
 * even when cancelled.  Callers should take note.
 */
tamed void
agent_t::req_oneshot (ptr<req_t> req, ptr<resp_factory_t> of, resp_ev_t ev)
{
  tvars {
    int status (HTTP_TIMEOUT);
    ptr<agent_t> hold;
    ptr<ok_xprt_base_t> x;
    bool dummy;
    ptr<resp_t> resp;
    ptr<abuf_t> abuf;
  }
  
  // Keep this object from being freed from underneath us.
  hold = mkref (this);

  twait { get_x (NULL, connector::cnc (mkevent (status, x, dummy), ev)); }

  if (x) {
    twait { req->make (x, false, connector::cnc (mkevent (status), ev)); }
    if (status == HTTP_OK) {
      abuf = New refcounted<abuf_t> (x->alloc_abuf_src (), true);
      resp = of->alloc_resp (x, abuf);
      twait { resp->get (connector::cnc (mkevent (status), ev)); }
    }
  }
  
  ev->trigger (status, resp);
}

//-----------------------------------------------------------------------

void
agent_t::req (ptr<req_t> req, ptr<resp_factory_t> resp, resp_ev_t ev)
{
  req_oneshot (req, resp, ev);
}

//-----------------------------------------------------------------------

bool
resp_t::do_get_body (int status) const
{
  return (status == HTTP_OK);
}


//-----------------------------------------------------------------------

resp_t::resp_t (ptr<ok_xprt_base_t> x, ptr<abuf_t> a)
  : _x (x),
    _abuf (a),
    _scratch (ok::alloc_scratch (okwc_scratch_sz)),
    _incookies (_abuf, _scratch),
    _hdr (_abuf, &_incookies, _scratch) 
{
  assert (_abuf);
}

//-----------------------------------------------------------------------

tamed void
resp_t::get_T (evi_t ev)
{
  tvars {
    int status (HTTP_OK), status2;
    outcome_t outc;
    ptr<resp_t> hold;
  }
  hold = mkref (this);

  // Call into the OKWS HTTP header parsing classes...
  twait { _hdr.parse (connector::cnc (mkevent (status), ev, &outc)); }

  if (outc == OUTCOME_CANCELLED) {
    status = CANCELLED_STATUS;
    _hdr.cancel ();
  } else if (do_get_body (status) || _hdr.has_body ()) {
    twait { get_body (connector::cnc (mkevent (status2), ev)); }
    if (status == HTTP_OK) { status = status2; }
  }
  ev->trigger (status);
}

//-----------------------------------------------------------------------

tamed void
resp_t::get_body_T (evi_t ev)
{
  tvars {
    int status (HTTP_OK);
    ptr<resp_t> hold;
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
resp_t::run_chunker_T (evi_t ev)
{
  tvars {
    okwc_chunker_t *c;
    int status;
    size_t sz;
    outcome_t outc;
    ptr<resp_t> hold;
  }
  hold = mkref (this);

  c = New okwc_chunker_t (_abuf, _scratch);

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

  delete c;

  ev->trigger (status);
}

//-----------------------------------------------------------------------

tamed void
resp_simple_t::eat_chunk_T (size_t sz, evi_t ev)
{
  tvars {
    str chnk;
    int status (HTTP_OK);
    outcome_t outc;
    ptr<resp_simple_t> hold;
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
resp_simple_t::finished_meal (int status, evi_t ev)
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

reqinfo_proxied_t::reqinfo_proxied_t (const str &url)
  : reqinfo_t (),
    _url (url),
    _valid (false)
{
  static rxx url_rxx ("http://([^:/\\s]+(:\\d+)?)(/.*)?");
  if (url_rxx.match (url)) {
    _hdr_hostname = url_rxx[1];
    _valid = true;
  }
}


//-----------------------------------------------------------------------

const post_t *
req_t::get_post () const
{
  const post_t *ret = NULL;
  if (get_simple_post_str ()) {
    _post_obj = New refcounted<simple_post_t> (get_simple_post_str ());
    ret = _post_obj;
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<resp_t>
obj_factory_t::alloc_resp (ptr<ok_xprt_base_t> x, ptr<abuf_t> b)
{
  return New refcounted<resp_simple_t> (x, b);
}

//-----------------------------------------------------------------------

ptr<req_t>
obj_factory_t::alloc_req (ptr<reqinfo_t> ri, int v, cgi_t *c)
{
  return New refcounted<req3_t> (ri, v, c);
}

//-----------------------------------------------------------------------

tamed void
agent_get_direct_t::get_T (const str &fn, simple_ev_t ev, int v, cgi_t *c,
			   str post, vec<str> *eh)
{
  tvars {
    ptr<resp_t> resp;
    ptr<req_t> rq;
    ptr<reqinfo_t> ri;
    int status;
  }

  ri = reqinfo_direct_t::alloc (_hostname, _port, fn);
  assert (ri->validate ());
  rq = _obj_factory->alloc_req (ri, v, c);
  if (post) rq->set_post (post);
  if (eh && eh->size ()) rq->set_extra_headers (*eh);

  twait { 
    agent_t::req (rq, _obj_factory, 
		  connector::cnc (mkevent (status, resp), ev)); 
  }
  ev->trigger (status, resp);
}

//-----------------------------------------------------------------------

tamed void
agent_get_proxied_t::get_T (const str &url, simple_ev_t ev, int v, cgi_t *c,
			    str post, vec<str> *eh)
{
  tvars {
    ptr<resp_t> resp;
    ptr<req_t> rq;
    ptr<reqinfo_t> ri (reqinfo_proxied_t::alloc (url));
    int status;
  }

  if (!ri->validate ()) {
    status = HTTP_CLIENT_BAD_PROXY;
  } else {
    rq = New refcounted<req3_t> (ri, v, c);
    if (post) rq->set_post (post);
    if (eh && eh->size ()) rq->set_extra_headers (*eh);
    twait { 
      agent_t::req (rq, _obj_factory, 
		    connector::cnc (mkevent (status, resp), ev)); 
    }
  }
  ev->trigger (status, resp);
}

//-----------------------------------------------------------------------

void
req3_t::set_extra_headers (const vec<str> &v)
{
  for (size_t i = 0; i < v.size (); i++) {
    _extra_headers.push_back (v[i]);
  }
}

//-----------------------------------------------------------------------

const vec<str> *
req3_t::get_extra_headers () const
{
  if (_extra_headers.size ()) { return &_extra_headers; }
  else { return NULL; }
}

//-----------------------------------------------------------------------

tamed void
agent_t::get_x (ptr<ok_xprt_base_t> x, evixb_t ev)
{
  tvars {
    ptr<agent_t> hold;
    ptr<hostent> he;
    int status (HTTP_OK);
    int dns_err;
    tcpconnect_t *tc;
    outcome_t outc;
    int fd;
    bool newone (false);
  }

  // Keep this object from being freed from underneath us.
  hold = mkref (this);

  if (!x || x->ateof ()) {

    newone = true;

    twait { 
      dnscache.lookup (hostname (), connector::cnc (mkevent (he, dns_err), ev));
    }
    
    if (dns_err || !he) {
      status = dnserr2httperr (dns_err);
    } else {

      twait { 
	tc = tcpconnect (*(in_addr *)he->h_addr, port (), 
			 connector::cnc (mkevent (fd), ev, &outc));
      }
      
      if (outc == OUTCOME_CANCELLED) {
	status = CANCELLED_STATUS;
	tcpconnect_cancel (tc);
      } else if (fd < 0) {
	status = HTTP_CONNECTION_FAILED;
      } else if (use_ssl ()) {
	x = okssl::factory ()->alloc_con (fd);
      } else {
	x = ahttpcon::alloc (fd);
      }
    }

  }
  ev->trigger (status, x, newone);

}

//-----------------------------------------------------------------------

str 
fix_url_filename (const str &in) 
{
  int len;
  str ret = in;
  if (!ret || (len = ret.len ()) == 0) {
    // empty file names --> "/"
    ret = "/";
  } else if (ret[0] != '/') {
    // insert leading slash if not there.
    ret = strbuf ("/") << in;
  } else {
    // trunc all but the first leading slash
    const char *fn = ret.cstr ();
    const char *cp;
    for (cp = fn; *cp == '/'; cp++)  ;
    cp--;
    ret = str (cp, len - (cp - fn)); 
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
parse_http_url (str in, bool *https, str *host, okws1_port_t *port, 
		str *filename)
{
  static rxx x ("http(s)?://([^:/]+)(:(\\d+))?(/(.*))?");
  bool ret = true;
  str ps;

  if (!x.match (in)) { ret = false; }
  else {
    bool ssl = false;
    if (x[1] && x[1] == "s") { ssl = *https = true; }
   
    if ((ps = x[4]) && ps.len ()) {
      if (!convertint (ps, port)) {
	warn << "parse_http_url: cannot convert port '" << ps
	     << "' in URL '" << in << "'\n";
	ret = false;
      }
    } else {
      *port = std_port (ssl);
    }

    if (ret) {
      *host = x[2];
      *filename = fix_url_filename (x[5]);
    }
  }
  return ret;
}

//-----------------------------------------------------------------------

};
