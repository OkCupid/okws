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
#include "okwc.h"
#include "httpconst.h"
#include "parseopt.h"
#include "rxx.h"

static okwc_dnscache_t dnscache;

void
okwc_dnscache_entry_t::lookup (cbhent cb)
{
  if (resolving) {
    cbq.push_back (cb);
    return;
  }

  if (!init || sfs_get_timenow() > expires || err != 0) {
    resolving = true;
    cbq.push_back (cb);
    dnsp = dns_hostbyname (hostname, 
			   wrap (this, &okwc_dnscache_entry_t::name_cb), 
			   false);
  } else {
    (*cb) (he, err);
  }
}

void
okwc_dnscache_entry_t::name_cb (ptr<hostent> h, int e)
{
  if (e || !h) 
    warn << hostname << ": DNS lookup failed with error=" << e << "\n";
  init = true;
  err = e;
  he = h;
  resolving = false;
  expires = sfs_get_timenow() + ttl;
  cbhent::ptr c;
  while (cbq.size ()) {
    c = cbq.pop_front ();
    (*c) (he, err);
  }
}

void
okwc_dnscache_t::lookup (const str &n, cbhent cb)
{
  ptr<okwc_dnscache_entry_t> *entp = cache[n];
  ptr<okwc_dnscache_entry_t> ent;
  if (entp) {
    ent = *entp;
  } else {
    ent = New refcounted<okwc_dnscache_entry_t> (n);
    cache.insert (n, ent);
  } 
  ent->lookup (cb);
}

okwc_req_t *
okwc_request (const str &host, u_int16_t port, const str &fn,
	      okwc_cb_t cb, int vrs, int timeout, cgi_t *outcook,
	      const str &body, const str &type)
{
  okwc_req_t *req = 
    New okwc_req_bigstr_t (host, port, fn, cb, vrs, timeout, outcook,
			   body, type);
  req->launch ();
  return req;
}

okwc_req_t *
okwc_request_proxied (const str &proxy_hostname,
		      u_int16_t proxy_port,
		      const str &url,
		      okwc_cb_t cb,
		      int vers,
		      int timeout,
		      cgi_t *outcook,
		      const str &post,
		      const str &type)
{
  static rxx url_rxx ("http://([^:/\\s]+(:\\d+)?)(/\\S*)");
  okwc_req_t *req =  NULL;
  if (!url_rxx.match (url)) {
    (*cb) (New refcounted<okwc_resp_t> (HTTP_CLIENT_BAD_PROXY));
  } else {
  
    req = New okwc_req_bigstr_t (proxy_hostname, 
				 proxy_port, url, 
				 cb, vers, timeout, outcook,
				 post, type);
    req->set_proxy_mode (url_rxx[1]);
    req->launch ();
  }
  return req;
}

void
okwc_req_t::set_proxy_mode (const str &hn)
{
  _proxy_mode = true;
  _proxy_hdr_hostname = hn;
}


void
okwc_cancel (okwc_req_t *req)
{
  req->cancel (HTTP_CLIENT_EOF);
}

bool
okwc_http_hdr_t::is_chunked () const
{
  str v;
  return (lookup ("transfer-encoding", &v) && cicmp (v, "chunked"));
}

okwc_req_t::okwc_req_t (const str &h, u_int16_t p, const str &f, 
			int v, int to, cgi_t *ock, const str &post,
			const str &t)
  : hostname (h), port (p), filename (f),
    vers (v), timeout (to), outcookie (ock),
    _proxy_mode (false),
    tcpcon (NULL), waiting_for_dns (false), fd (-1), 
    http (NULL), timer (NULL),
    _post (post), _type (t) {}

void
okwc_req_t::launch ()
{
  if (timeout > 0) 
    timer = delaycb (timeout, 0, 
		     wrap (this, &okwc_req_t::cancel, HTTP_TIMEOUT, true));
  waiting_for_dns = true;
  dnscache.lookup (hostname, wrap (this, &okwc_req_t::dns_cb));
}

void
okwc_req_t::dns_cb (ptr<hostent> he, int err)
{
  waiting_for_dns = false;
  if (err) {
    req_fail (HTTP_CONNECTION_FAILED);
    return;
  }
  tcpcon = tcpconnect (*(in_addr *)he->h_addr, port, 
		       wrap (this, &okwc_req_t::tcpcon_cb));
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
  http = okwc_http_alloc ();
  http->make_req ();
}

okwc_http_t *
okwc_req_bigstr_t::okwc_http_alloc () 
{
  return New okwc_http_bigstr_t (x, filename,  hostname, port,
				 wrap (this, &okwc_req_bigstr_t::http_cb), 
				 vers, outcookie, _post, _type,
				 _proxy_mode, _proxy_hdr_hostname);
}

void
okwc_req_bigstr_t::http_cb (ptr<okwc_resp_t> res)
{
  (*okwc_cb) (res);
  delete this;
}

void
okwc_req_t::cancel (int status, bool from_dcb)
{
  if (from_dcb) 
    timer = NULL;

  if (tcpcon) {
    tcpconnect_cancel (tcpcon);
    tcpcon = NULL;
    assert (!http);
  } else if (!waiting_for_dns) {
    http->cancel ();
  }
  req_fail (status);
}

void
okwc_req_bigstr_t::req_fail (int status)
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
  int len;

  // Fixup filename if not in proxy mode.
  if (!_proxy_mode) {
    if (!filename || (len = filename.len ()) == 0) {
      // empty file names --> "/"
      filename = "/";
    } else if (filename[0] != '/') {
      // insert leading slash if not there.
      filename = strbuf ("/") << filename;
    } else {
      // trunc all but the first leading slash
      const char *fn = filename.cstr ();
      const char *cp;
      for (cp = fn; *cp == '/'; cp++)  ;
      cp--;
      filename = str (cp, len - (cp - fn)); 
    }
  }

  //
  // hacked in for now -- eventually we should be asking for HTTP/1.1
  // requests, and parse chunked content returns. for now, we'll do
  // this instead.
  //
  str mth = _post ? "POST" : "GET";

  reqbuf << mth << " " << filename << " HTTP/1." << vers  << HTTP_CRLF;

  str hn;
  if (_proxy_mode) {
    hn = _proxy_hdr_hostname;
  } else {
    strbuf b;
    b << hostname;
    if (port != 80)
      b << ":" << port;
    hn = b;
  }

  //
  // In either HTTP/1.0 or HTTP/1.1, be polite like wget and output the
  // hostname and user-agent
  //
  reqbuf << "Host: " << hn << HTTP_CRLF
	 << "User-agent: okwc/" << VERSION << HTTP_CRLF;

  if (vers == 1) {
    reqbuf << "Connection: close" << HTTP_CRLF;
  }

  if (outcook) {
    reqbuf << "Cookie: ";
    outcook->encode (&reqbuf);
    reqbuf << HTTP_CRLF;
  }

  str typ = _type;

  if (_post && !typ)
    typ = ok_http_urlencoded;

  if (typ) {
    reqbuf << "Content-Type: " << typ << HTTP_CRLF;
  } 

  if (_post) {
    reqbuf << "Content-Length: " << _post.len () << HTTP_CRLF;
  }

  reqbuf << HTTP_CRLF;
  if (_post) {
    reqbuf << _post;
  }
  state = OKWC_HTTP_REQ;
  cancel_flag = New refcounted<bool> (false);
  x->send (reqbuf, wrap (this, &okwc_http_t::parse, cancel_flag));
}

void
okwc_http_t::parse (ptr<bool> local_cancel_flag)
{
  if (*local_cancel_flag)
    return;
  cancel_flag = NULL; // will dealloc cancel_flag at return
  state = OKWC_HTTP_HDR;
  hdr ()->parse (wrap (this, &okwc_http_t::hdr_parsed));
}

void
okwc_http_t::hdr_parsed (int status)
{
  if (status != HTTP_OK) {
    finish (status);
  } else {
    state = OKWC_HTTP_BODY;
    body_parse ();
  }
}

void
okwc_http_bigstr_t::eat_chunk (size_t s)
{
  body.dump (s, wrap (this, &okwc_http_bigstr_t::ate_chunk));
}

void
okwc_http_t::start_chunker ()
{
  chunker->parse (wrap (this, &okwc_http_t::parsed_chunk_hdr));
}

void
okwc_http_t::body_parse ()
{
  if (hdr ()->is_chunked ()) {
    assert (!chunker);
    chunker = New okwc_chunker_t (&abuf, _scratch);
    start_chunker ();
  } else {
    eat_chunk (hdr ()->get_contlen ());
  }
}

void
okwc_http_t::parsed_chunk_hdr (int status)
{
  if (status == HTTP_OK) {
    size_t sz = chunker->get_sz ();
    if (sz == 0) {
      body_chunk_finish ();
    } else {
      eat_chunk (sz);
    }
  } else {
    finish (status);
  }
}

static bool
convertint16 (const str &s, u_int *i)
{
  char *end;
  u_int res = strtoi64 (s.cstr (), &end, 16);
  if (*end)
    return false;
  *i = res;
  return true;
}

void
okwc_chunker_t::parse_guts ()
{
  abuf_stat_t r = ABUF_OK;
  while (r == ABUF_OK) {
    switch (state) {
    case FINISH_PREV:
      r = require_crlf ();
      break;
    case START: 
      {
	u_int tmp = 0;
	str sz_str;
	r = delimit_word (&sz_str);
	if (r == ABUF_OK && (!sz_str.len () || !convertint16 (sz_str, &tmp)))
	  r = ABUF_PARSE_ERR;
	sz = tmp;
	break;
      }
    case SPC1:
      r = abuf->skip_hws (0);
      break;
    case EOL1:
      r = require_crlf ();
      break;
    case EOL2:
      // seems as if this second CRLF is optional ?!?!?!
      r = gobble_crlf ();
      if (r == ABUF_NOMATCH) 
	r = ABUF_OK;
      break;
    default:
      r = ABUF_COMPLETE;
      break;
    }
    if (r == ABUF_OK)
      state = static_cast<state_t> (state + 1);
  }

  if (r != ABUF_WAIT) 
    finish_parse (r == ABUF_COMPLETE ? HTTP_OK : HTTP_SRV_ERROR);
}

void
okwc_http_t::body_parse_continue ()
{
  if (hdr ()->is_chunked ()) {
    chunker->next_chunk ();
    start_chunker ();
  } else
    body_chunk_finish ();
}

void
okwc_http_t::body_chunk_finish ()
{
  finished_meal ();
  finish (HTTP_OK);
}

void
okwc_http_bigstr_t::ate_chunk (str bod)
{
  chunks.push_back (bod);
  body_parse_continue ();
}

void
okwc_http_bigstr_t::finished_meal ()
{
  if (chunks.size () == 1) {
    resp->body = chunks[0];
  } else if (chunks.size () > 1) {
    strbuf b;
    while (chunks.size ())
      b << chunks.pop_front ();
    resp->body = b;
  }
}

void
okwc_http_t::finish (int status)
{
  state = OKWC_HTTP_NONE;
  finish2 (status);
}

void
okwc_http_bigstr_t::finish2 (int status)
{
  resp->status = status;
  (*okwc_cb) (resp);
}

okwc_http_t::okwc_http_t (ptr<ahttpcon> xx, const str &f, const str &h,
			  int port_in,
			  int v, cgi_t *ock, okwc_cookie_set_t *incook,
			  const str &p, const str &t, bool proxy_mode,
			  const str &proxy_hdr_hostname)
  : x (xx), filename (f), hostname (h), port (port_in),
    abuf (New abuf_con_t (xx), true),
    _scratch (ok::alloc_scratch (okwc_scratch_sz)),
    resp (okwc_resp_t::alloc (&abuf, _scratch, incook)),
    vers (v), 
    outcook (ock), 
    state (OKWC_HTTP_NONE), 
    chunker (NULL),
    _post (p), _type (t),
    _proxy_mode (proxy_mode),
    _proxy_hdr_hostname (proxy_hdr_hostname)
{}

okwc_http_bigstr_t::okwc_http_bigstr_t (ptr<ahttpcon> xx, const str &f,
					const str &h, int port, okwc_cb_t cb,
					int v, cgi_t *okc, const str &p,
					const str &t,
					bool proxy_mode,
					const str &proxy_hdr_hostname)
  : okwc_http_t (xx,f,h,port,v,okc,NULL,p,t,proxy_mode,proxy_hdr_hostname), 
    body (&abuf), okwc_cb (cb)
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
  int status_combined = HTTP_NO_STATUS;

  while (r == ABUF_OK) {
    inc = true;
    switch (state) {
    case OKWC_HDR_START:
      r = delimit_word (&vers);
      if (r == ABUF_OK && _status_parse == HTTP_NO_STATUS) {
	_status_parse = HTTP_OK;
      }
      break;
    case OKWC_HDR_SPC1:
      r = abuf->skip_hws (1);
      break;
    case OKWC_HDR_STATUS_NUM: 
      {
	str status_str;
	r = delimit_word (&status_str);
	if (r == ABUF_OK && !convertint (status_str, &_status_header))
	  r = ABUF_PARSE_ERR;
	break;
      }
    case OKWC_HDR_SPC2:
      // maybe there's an EOL here (not status desc)
      if ((r = eol ()) != ABUF_OK)
	r = abuf->skip_hws (1);
      break;
    case OKWC_HDR_STATUS_DESC:
      r = eol ();
      if (r == ABUF_NOMATCH) 
	r = delimit_status (&status_desc);
      break;
    case OKWC_HDR_EOL1:
      r = require_crlf ();
      break;
    case OKWC_HDR_KEY:
      r = gobble_crlf ();
      if (r == ABUF_OK) {
	r = ABUF_EOF;
	_clean_eoh = true;
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
      else if (key) {
	insert (key, val);
      } else 
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
    if (!_clean_eoh) {
      _status_parse = HTTP_BAD_REQUEST;
    }
    status_combined = (_status_header != HTTP_NO_STATUS 
		       ? _status_header : _status_parse);
    fixup (); // fixup all the time, not just on success.
    finish_parse (status_combined);
  }
}

void
okwc_http_hdr_t::fixup ()
{
  if (lookup ("content-length", &contlen))
    _has_body = true;
  if (contlen == 0) 
    contlen = okwc_def_contlen;

  str tmp;
  if (lookup ("connection", &tmp) && cicmp (tmp, "keep-alive")) {
    _conn = HTTP_CONN_KEEPALIVE;
  } else {
    _conn = HTTP_CONN_CLOSED;
  }

}

void
okwc_http_t::cancel ()
{
  switch (state) {
  case OKWC_HTTP_REQ:
    *cancel_flag = true;
    break;
  case OKWC_HTTP_HDR:
    hdr ()->cancel ();
    break;
  case OKWC_HTTP_BODY:
    cancel2 ();
    break;
  default:
    break;
  }
}

void
okwc_http_bigstr_t::cancel2 ()
{
  body.cancel ();
}

