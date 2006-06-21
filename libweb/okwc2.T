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

TAMED void
okwc2_req_t::make_T (ptr<ahttpcon> x, cbi cb)
{
  VARS {
    strbuf req;
    int status (HTTP_OK);
  }
  format_req (req);
  BLOCK { x->send (req, @()); }

  (*cb) (status);
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
    coordgroup_t<bool> G;
    int fd;
    bool cancelled;
    ptr<ahttpcon> x;
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
    tcpconnect (*(in_addr *)he->h_addr, _port, @[G, false](fd));

    BLOCK { cancellable_wait (G, cb, @(cancelled)); }
    if (cancelled) return;

    if (fd < 0) {
      status = HTTP_CONNECTION_FAILED;
    } else {
      x = ahttpcon::alloc (fd);
      req->make (x, @[G,false](status)); 

      BLOCK { cancellable_wait (G, cb, @(cancelled)); }
      if (cancelled) return;

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
  G.remove_join ();
  SIGNAL (cb, status, resp);
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
okwc2_resp_t::get_body (cbi cb)
{
  VARS {
    int status (HTTP_OK);
  }
  if (_hdr.is_chunked ()) {
    BLOCK { run_chunker (@(status)); }
  } else {
    BLOCK { eat_chunk (_hdr.get_contlen (), @()); }
  }
  SIGNAL (cb, status);
}
