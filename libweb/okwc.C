
#include "okwc.h"
#include "httpconst.h"

okwc_req_t::okwc_req_t (const str &h, u_int16_t p, const str &f, cgi_t *inc,
			okwc_cb_t c, int to)
  : hostname (h), port (p), filename (f),
    incookie (inc), okwc_cb (c), timeout (to),
    tcpcon (NULL), fd (-1), http (NULL), timer (NULL) {}

void
okwc_req_t::launch ()
{
  tcpcon = tcpconnect (hostname, port, wrap (this, &okwc_req_t::tcpcon_cb));
  if (timeout > 0) 
    timer = delaycb (timeout, 0, wrap (this, &okwc_req_t::timeout_cb));
}

void
okwc_req_t::tcpcon_cb (int f)
{
  tcpcon = NULL;

  if (f < 0) {
    finish (HTTP_CONNECTION_FAILED);
    return;
  }
  fd = f;
  x = ahttpcon::alloc (fd);
  http = New okwc_http_t (x, filename, wrap (this, &okwc_req_t::http_cb));
  http->make_req ();
}

void
okcw_req_t::http_cb (ptr<okwc_resp_t> res)
{
  (*okwc_cb) (res);
  delete this;
}

void
okwc_req_t::timeout_cb ()
{
  if (tcpcon) {
    tcpconnect_cancel (tcpcon);
    tcpcon = NULL;
    assert (!http);
  } else {
    http->cancel ();
  }
  finish (HTTP_TIMEOUT);

}

void
okwc_req_t::finish (int status)
{
  (*okwc_cb) (okwc_resp_t::alloc (status));
  delete this;
}

okwc_req_t::~okwc_req_t ()
{
  if (http)
    delete http;
  if (timer) {
    timecb_remove (timer);
    timer = NULL;
  }
}


void
okwc_http_t::make_req ()
{
  reqbuf << "GET " << filename << " HTTP/1.0" << HTTP_CRLF;
  if (cookie) {
    reqbuf << "Cookie: ";
    cookie->encode (&reqbuf);
    reqbuf << HTTP_CRLF;
  }
  reqbuf << HTTP_CRLF;
  x->send (reqbuf, wrap (this, &okwc_http_t::parse));
}

void
okwc_http_t::parse ()
{
  resp = okwc_resp_t::alloc (HTTP_OK);
  hdr.parse (resp, wrap (this, &okwc_http_t::hdr_parsed));
}

void
okwc_http_t::hdr_parsed (int status)
{
  if (status != HTTP_OK) {
    finish (status);
  } else {
    body.parse (resp, hdr.contlen, wrap (this, &okwc_http_t::body_parsed));
  }
}

void
okwc_http_t::body_parsed (int status)
{
  finish (status);
}
