// -*-c++-*-

#include "okwc2.h"
#include "okwc.h"

//-----------------------------------------------------------------------

TAMED static void
cancellable_wait (coordgroup_t<bool> G, okwc2_cb_t cb, cbb callercv)
{
  VARS {
    bool cancelled;
    bool dummy;
  }
  WAIT (G, cancelled);
  if (cancelled) {
    SIGNAL (cb, HTTP_TIMEOUT, NULL);
  }
  SIGNAL (callercv, cancelled);
  if (cancelled) {
    WAIT (G, dummy);
  }
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

  okwc2_post_t *p = get_post ();
  if (p) b << "Content-Length: " << p->len () << HTTP_CRLF;
  b << HTTP_CRLF;
  if (p) p->output (b);

}

//-----------------------------------------------------------------------

TAMED ptr<canceller_t> 
okwc2_req_t::make_T (ptr<ahttpcon> x, cbi cb)
{
  VARS {
    strbuf req;
    int status (HTTP_TIMEOUT);
    ptr<canceller_t> ret (New refcounted<canceller_t> ()); 
    coordgroup_t<bool> G;
    bool cancelled;
    bool dummy;
  }
  DEFAULT_RETURN { return ret; }

  format_req (req);
  
  x->send (req, @[G,false]());
  ret->wait (@[G,true] ());

  WAIT (G, cancelled);
  if (cancelled) {
    // This will cause the callback issued with send to fire eventually,
    // so no need to remove the variable manually from the group here.
    x->cancel ();
  } else {
    ret->toolate ();
    G.remove_var ();
  }
  SIGNAL (cb, status);

  while (G.n_vars_left ()) {
    WAIT (G, dummy);
  }
}

//-----------------------------------------------------------------------

TAMED void
okwc2_t::req_T (ptr<okwc2_req_t> req, okwc2_cb_t cb)
{
  VARS {
    int status (0);
    ptr<okwc2_resp_t> resp;
    ptr<okwc2_t> hold;
    ptr<hostent> he;
    int dns_err;
    coordgroup_t<bool> G (__FILE__, __LINE__);
    int fd;
    bool cancelled;
    ptr<ahttpcon> x;
    tcpconnect_t *tc;
    ptr<canceller_t> cancel_send;
  }
  
  // Keep this object from being freed from underneath us.
  hold = mkref (this);

  req->notify_on_cancel (@[G, true]());
  dnscache.lookup (_hostname, @[G, false](he, dns_err)); 

  BLOCK { cancellable_wait (G, cb, @(cancelled)); }
  if (cancelled) return;

  if (dns_err || !he) {
    status = HTTP_CONNECTION_FAILED;
  } else {
    tc = tcpconnect (*(in_addr *)he->h_addr, _port, @[G, false](fd));

    BLOCK { cancellable_wait (G, cb, @(cancelled)); }
    if (cancelled) {
      tcpconnect_cancel (tc);
      return;
    }

    if (fd < 0) {
      status = HTTP_CONNECTION_FAILED;
    } else {
      x = ahttpcon::alloc (fd);
      cancel_send = req->make (x, @[G,false](status)); 

      BLOCK { cancellable_wait (G, cb, @(cancelled)); }
      if (cancelled) {
	cancel_send->cancel ();
	return;
      }

      if (status == HTTP_OK) {
	resp = alloc_resp (x);
	resp->get (@[G,false](status));

	BLOCK { cancellable_wait (G, cb, @(cancelled)); }
	if (cancelled) return;
      }
    }
  }
  // remove the canceller
  req->too_late_to_cancel ();
  G.remove_var ();
  SIGNAL (cb, status, resp);
}

//-----------------------------------------------------------------------

TAMED void
okwc2_t::timed_req_T (ptr<okwc2_req_t> r, int to, okwc2_cb_t cb)
{
  VARS {
    int status (HTTP_TIMEOUT);
    coordgroup_t<bool> G (__FILE__, __LINE__);
    timecb_t *tcb;
    bool timedout ;
    ptr<okwc2_resp_t> resp;
    bool dummy;
  }

  if (to == 0) {
    BLOCK { req (r, @(status, resp)); }
  } else {
    tcb = delaycb (to, 0, @[G,true]() );
    req (r, @[G,false](status, resp) );
    
    WAIT (G, timedout);
    if (timedout) {
      r->cancel ();
    } else {
      timecb_remove (tcb);
      G.remove_var ();
    }
  }

  SIGNAL (cb, status, resp);
  while (G.n_signals_left ()) {
    WAIT (G, dummy);
  }
}

//-----------------------------------------------------------------------

okwc2_resp_t::okwc2_resp_t (ptr<ahttpcon> x)
  : _x (x),
    _abuf (New abuf_con_t (_x), true),
    _incookies (&_abuf, OKWC_SCRATCH_SZ, _scratch),
    _hdr (&_abuf, &_incookies, OKWC_SCRATCH_SZ, _scratch) {}

//-----------------------------------------------------------------------

TAMED void
okwc2_resp_t::get_T (cbi cb)
{
  VARS {
    int status (HTTP_OK);
  }
  BLOCK { _hdr.parse (@(status)); }
  if (status == HTTP_OK) { BLOCK { get_body (@(status)); } }

  SIGNAL (cb, status);
}

//-----------------------------------------------------------------------

TAMED void
okwc2_resp_t::get_body_T (cbi cb)
{
  VARS {
    int status (HTTP_OK);
  }
  if (_hdr.is_chunked ()) {
    BLOCK { run_chunker (@(status)); }
  } else {
    BLOCK { eat_chunk (_hdr.get_contlen (), @(status)); }
  }
  SIGNAL (cb, status);
}

//-----------------------------------------------------------------------

TAMED void
okwc2_resp_t::run_chunker_T (cbi cb)
{
  VARS {
    okwc_chunker_t *c;
    int status;
    size_t sz;
  }
  c = New okwc_chunker_t (&_abuf, OKWC_SCRATCH_SZ, _scratch);

  do { 
    BLOCK { c->parse (@(status)); }
    if (status == HTTP_OK && (sz = c->get_sz ())) {
      BLOCK { eat_chunk (sz, @(status)); }
      c->next_chunk ();
    } 
  } while (status == HTTP_OK && sz);

  BLOCK { finished_meal (status, @(status)); }
  SIGNAL (cb, status);
}

//-----------------------------------------------------------------------

TAMED void
okwc2_resp_bigstr_t::eat_chunk_T (size_t sz, cbi cb)
{
  VARS {
    str chnk;
  }
  BLOCK { _dumper.dump (sz, @(chnk)); }
  if (chnk) _chunks.push_back (chnk);
  SIGNAL (cb, HTTP_OK);
}

//-----------------------------------------------------------------------

void
okwc2_resp_bigstr_t::finished_meal (int status, cbi cb)
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
