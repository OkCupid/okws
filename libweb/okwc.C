
#include "okwc.h"
#include "httpconst.h"
#include "parseopt.h"

okwc_req_t *
okwc_request (const str &host, u_int16_t port, const str &fn,
	      okwc_cb_t cb, int timeout, cgi_t *outcook)
{
  okwc_req_t *req = New okwc_req_t (host, port, fn, cb, timeout, outcook);
  req->launch ();
  return req;
}

void
okwc_cancel (okwc_req_t *req)
{
  req->cancel (HTTP_CLIENT_EOF);
}

okwc_req_t::okwc_req_t (const str &h, u_int16_t p, const str &f, 
			okwc_cb_t c, int to, cgi_t *ock)
  : hostname (h), port (p), filename (f),
    okwc_cb (c), timeout (to), outcookie (ock),
    tcpcon (NULL), fd (-1), http (NULL), timer (NULL) {}

void
okwc_req_t::launch ()
{
  tcpcon = tcpconnect (hostname, port, wrap (this, &okwc_req_t::tcpcon_cb));
  if (timeout > 0) 
    timer = delaycb (timeout, 0, 
		     wrap (this, &okwc_req_t::cancel, HTTP_TIMEOUT));
}

void
okwc_req_t::tcpcon_cb (int f)
{
  tcpcon = NULL;

  if (f < 0) {
    req_fail (HTTP_CONNECTION_FAILED);
    return;
  }
  fd = f;
  x = ahttpcon::alloc (fd);
  http = New okwc_http_t (x, filename, 
			  wrap (this, &okwc_req_t::http_cb), outcookie);
  http->make_req ();
}

void
okwc_req_t::http_cb (ptr<okwc_resp_t> res)
{
  (*okwc_cb) (res);
  delete this;
}

void
okwc_req_t::cancel (int status)
{
  if (tcpcon) {
    tcpconnect_cancel (tcpcon);
    tcpcon = NULL;
    assert (!http);
  } else {
    http->cancel ();
  }
  req_fail (status);
}

void
okwc_req_t::req_fail (int status)
{
  (*okwc_cb) (okwc_resp_t::alloc (status));
  delete this;
}

okwc_req_t::~okwc_req_t ()
{
  if (http) {
    delete http;
    http = NULL;
  }
  if (timer) {
    timecb_remove (timer);
    timer = NULL;
  }
}


void
okwc_http_t::make_req ()
{
  reqbuf << "GET " << filename << " HTTP/1.0" << HTTP_CRLF;
  if (outcook) {
    reqbuf << "Cookie: ";
    outcook->encode (&reqbuf);
    reqbuf << HTTP_CRLF;
  }
  reqbuf << HTTP_CRLF;
  x->send (reqbuf, wrap (this, &okwc_http_t::parse));
}

void
okwc_http_t::parse ()
{
  hdr.parse (wrap (this, &okwc_http_t::hdr_parsed));
}

void
okwc_http_t::hdr_parsed (int status)
{
  if (status != HTTP_OK) {
    finish (status);
  } else {
    body.dump (hdr.get_contlen (), wrap (this, &okwc_http_t::body_parsed));
  }
}

void
okwc_http_t::body_parsed (str bod)
{
  resp->body = bod;
  finish (HTTP_OK);
}

void
okwc_http_t::finish (int status)
{
  resp->status = status;
  (*okwc_cb) (resp);
}

okwc_http_t::okwc_http_t (ptr<ahttpcon> xx, const str &f, 
			  okwc_cb_t c, cgi_t *ock)
  : x (xx), filename (f), abuf (New abuf_con_t (xx), true),
    resp (okwc_resp_t::alloc (&abuf, OKWC_SCRATCH_SZ, scratch)),
    hdr (&abuf, &resp->cookies, OKWC_SCRATCH_SZ, scratch),
    body (&abuf), outcook (ock), okwc_cb (c)
{}

void
okwc_http_hdr_t::ext_parse_cb (int status)
{
  state = ret_state;
  resume ();
}

//
// XXX - eventually we should merge this in with
// http_inhdr_t::parse_guts -- there are many similarities.
//
void
okwc_http_hdr_t::parse_guts ()
{
  abuf_stat_t r = ABUF_OK;
  bool inc;
  int status_parse = HTTP_BAD_REQUEST;

  while (r == ABUF_OK) {
    inc = true;
    switch (state) {
    case OKWC_HDR_START:
      r = delimit_word (&vers);
      break;
    case OKWC_HDR_SPC1:
      r = abuf->skip_hws (1);
      break;
    case OKWC_HDR_STATUS_NUM: 
      {
	str status_str;
	r = delimit_word (&status_str);
	if (r == ABUF_OK && !convertint (status_str, &status))
	  r = ABUF_PARSE_ERR;
	break;
      }
    case OKWC_HDR_SPC2:
      r = abuf->skip_hws (1);
      break;
    case OKWC_HDR_STATUS_DESC:
      r = eol ();
      if (r == ABUF_NOMATCH) 
	r = delimit_word (&status_desc);
      break;
    case OKWC_HDR_EOL1:
      r = require_crlf ();
      break;
    case OKWC_HDR_KEY:
      r = gobble_crlf ();
      if (r == ABUF_OK) {
	status_parse = HTTP_OK;
	r = ABUF_EOF;
      } else if (r == ABUF_NOMATCH)
	r = delimit_key (&key);
      break;
    case OKWC_HDR_SPC3:
      r = abuf->skip_hws (1);
      break;
    case OKWC_HDR_VALUE:
      if (is_set_cookie ()) {
	noins = true;
	cgi_t *ck = cookie->push_back_new ();
	ret_state = OKWC_HDR_EOL2A;
	ck->parse (wrap (this, &okwc_http_hdr_t::ext_parse_cb));
	return;
      } 
      r = delimit_val (&val);
      break;
    case OKWC_HDR_EOL2A:
      if (noins)
	noins = false;
      else if (key)
	insert (key, val);
      else 
	r = ABUF_PARSE_ERR;
      key = val = NULL;
      break;
    case OKWC_HDR_EOL2B:
      r = require_crlf ();
      if (r == ABUF_OK) {
	state = OKWC_HDR_KEY;
	inc = false;
      }
      break;
    default:
      r = ABUF_PARSE_ERR;
      break;
    }
    if (r == ABUF_OK && inc)
      state = static_cast<state_t> (state + 1);
  }
  if (r != ABUF_WAIT) {
    if (status_parse != HTTP_OK)
      status = status_parse;
    if (status == HTTP_OK)
      fixup ();
    finish_parse (status);
  }
}

void
okwc_http_hdr_t::fixup ()
{
  if (!lookup ("content-length", &contlen))
    contlen = -1;
}


