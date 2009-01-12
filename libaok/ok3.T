// -*-c++-*-

#include "ok3.h"
#include "tame_connectors.h"

//-----------------------------------------------------------------------

okclnt3_t::req_t::req_t (ptr<ahttpcon> x, u_int reqno, htpv_t prev_vers, 
			 u_int to)
  : http_parser_cgi_t (x, to) 
{
  hdr.set_reqno (reqno, true, prev_vers);
}

//-----------------------------------------------------------------------

okclnt3_t::req_t::~req_t () {}

//-----------------------------------------------------------------------

okclnt3_t::resp_t::~resp_t ()
{
}

//-----------------------------------------------------------------------

okclnt3_t::resp_t::resp_t (okclnt3_t *o, ptr<req_t> q)
  : _ok_clnt (o),
    _uid (0),
    _uid_set (false), 
    _rsp_gzip (false),
    _sent (false),
    _replied (false),
    _req (q) {}

//-----------------------------------------------------------------------

okclnt3_t::okclnt3_t (ptr<ahttpcon> xx, oksrvc_t *o, u_int to)
  : okclnt_interface_t (o),
    _x (xx),
    _timeout (to),
    _union_cgi_mode (false) {}

//-----------------------------------------------------------------------

okclnt3_t::~okclnt3_t ()
{
  for (size_t i = 0; i < _resps.size(); i++) {
    _resps[i]->mark_defunct ();
  }
}

//-----------------------------------------------------------------------

void
okclnt3_t::resp_t::reply (int st, ptr<compressible_t> b, str url, str es)
{
  assert (!_replied);
  if (_ok_clnt) {
    _status = st;
    _body = b;
    _redir_url = url;
    _error_str = es;
    _replied = true;

    _ok_clnt->poke ();
  }
}

//-----------------------------------------------------------------------

ptr<cookie_t>
okclnt3_t::resp_t::add_cookie (const str &h, const str &p)
{
  ptr<cookie_t> ret = New refcounted<cookie_t> (h, p);
  _outcookies.push_back (ret);
  return ret;
}

//-----------------------------------------------------------------------

str
okclnt3_t::ssl_cipher () const 
{
  if (_demux_data) return _demux_data->ssl_info ();
  else return NULL;
}

//-----------------------------------------------------------------------

void
okclnt3_t::set_localizer (ptr<const pub_localizer_t> l)
{
  _p2_locale = New refcounted<pub2::locale_specific_publisher_t> (pub2 (), l);
}

//-----------------------------------------------------------------------

ptr<pub2::ok_iface_t>
okclnt3_t::pub2 ()
{
  return oksrvc->pub2 ();
}

//-----------------------------------------------------------------------

ptr<pub2::ok_iface_t>
okclnt3_t::pub2_local ()
{
  if (!_p2_locale)
    _p2_locale = New refcounted<pub2::locale_specific_publisher_t> (pub2 ());
  return _p2_locale;
}

//-----------------------------------------------------------------------

void
okclnt3_t::redirect (int status, const str &u)
{
  alloc_resp ()->redirect (status, u);
  poke ();
}

//-----------------------------------------------------------------------

void
okclnt3_t::error (int status)
{
  alloc_resp ()->error (status);
  poke ();
}

//-----------------------------------------------------------------------

bool
okclnt3_t::check_ssl ()
{
  str s;
  bool ok (true);

  if (ssl_only () && !is_ssl ()) {
    if ((s = ssl_redirect_str ())) redirect (HTTP_MOVEDPERM, s);
    else error (HTTP_FORBIDDEN);
    ok = false;
  }
  return ok;
}

//-----------------------------------------------------------------------

ptr<okclnt3_t::resp_t>
okclnt3_t::alloc_resp (ptr<req_t> req)
{
  ptr<resp_t> r = New refcounted<resp_t> (this, req);
  _resps.push_back (r);
  return r;
}

//-----------------------------------------------------------------------

tamed void
okclnt3_t::serve_T ()
{
  tvars {
    bool go (true);
    ptr<okclnt3_t::req_t> req;
    int status;
    rendezvous_t<> rv (__FILE__, __LINE__);
    u_int reqno (1);
    htpv_t prev (0);
  }

  output_loop (ok_clnt_timeout, mkevent (rv));

  if (go) go = check_ssl ();

  while (go) {
    req = New refcounted<req_t> (_x, reqno, prev, ok_clnt_timeout);
    twait { req->parse (mkevent (status, go)); }

    if (status == HTTP_OK) {
      prev = req->http_vers ();
      process (req, alloc_resp (req), mkevent (rv));
    } else if (status != HTTP_PIPELINE_EOF) {
      error (status);
    }

  }

  while (rv.need_wait ())
    twait (rv);

  finish_serve ();
}

//-----------------------------------------------------------------------

tamed void
okclnt3_t::req_t::parse (okclnt3_t::req_t::parse_ev_t ev)
{
  tvars {
    int status;
    bool go;
  }

  twait { http_parser_cgi_t::parse (mkevent (status)); }
  go = (status == HTTP_OK && hdr.get_conn_mode () == HTTP_CONN_KEEPALIVE);
  ev->trigger (status, go);
}

//-----------------------------------------------------------------------

tamed void
okclnt3_t::output_loop (int time_budget, evv_t ev)
{
  tvars {
    ptr<okclnt3_t::resp_t> rsp;
    bool go (true);
    outcome_t outc;
    time_t start, stop;
  }

  twait { _output_cv.wait (mkevent ()); }

  while (go && _resps.size ()) {

    rsp = _resps.pop_front ();
    while (!rsp->is_ready ()) {
      twait { _output_cv.wait (mkevent ()); }
    }

    start = sfs_get_timenow ();
    twait { 
      rsp->send (connector::timeout (mkevent (go), time_budget, 0, &outc)); 
    }
    stop = sfs_get_timenow ();

    // update the time budget for next time around....
    time_budget = max<int> (1, time_budget - (stop - start));

    if (outc == OUTCOME_TIMEDOUT) {
      str i = _x->all_info ();
      go = false;
      warn << "timeout in send: " << i << "\n";
    }

  }

  ev->trigger ();
}

//-----------------------------------------------------------------------

bool
okclnt3_t::resp_t::do_gzip () const
{
  return (_req && _req->hdr_cr ().takes_gzip () && ok_gzip && _rsp_gzip);
}

//-----------------------------------------------------------------------

void
okclnt3_t::resp_t::set_attributes (http_resp_attributes_t *hra)
{
  if (_cache_control) hra->set_cache_control (_cache_control);
  if (_content_type) hra->set_content_type (_content_type);
  if (_expires) hra->set_expires (_expires);
  if (_cont_disp) hra->set_content_disposition (_cont_disp);

  // set anything else you want
  hra->set_others (_hdr_fields);
}
//-----------------------------------------------------------------------

oksrvc_t *
okclnt3_t::resp_t::svc ()
{
  oksrvc_t *ret = NULL;
  if (_ok_clnt) ret = _ok_clnt->get_oksrvc ();
  return ret;
}

//-----------------------------------------------------------------------

tamed void
okclnt3_t::resp_t::send (evb_t ev)
{
  tvars {
    ptr<http_response_t> rsp;
    ptr<okclnt3_t::req_t> req (_self->req ());
    http_inhdr_t *inhdrs (req ? req->hdr_p () : NULL);
    int vers (req ? req->hdr_cr ().get_vers () : 0);
    http_resp_attributes_t hra (_self->status (), vers);
    bool gz (_self->do_gzip ());
    bool ok (false);
    oksrvc_t *svc (_self->svc ());
    strbuf body_buf;
    ptr<ahttpcon> x (_self->con ());
  }

  hra.set_others (_hdr_fields);

  if (!x || x->closed ()) {
    _status = HTTP_CLIENT_EOF;
    ok = false;
  }

  // Handle Redirects
  if (_redir_url)  {
    if (httpconst::is_redirect (_status)) {
      rsp = New refcounted<http_response_redirect_t> (_redir_url, hra);
    } else {
      _status = HTTP_SRV_ERROR;
      hra.set_status (_status);
    }
  }

  // Handle Errors
  if (httpconst::is_error (_status)) {
    if (svc) {
      twait { svc->geterr (_status, _error_str, vers, gz, mkevent (rsp)); }
    } else {
      ok = false;
    }

  } else if (!rsp) {

    // Handle the case in which the response is OK to send out,
    // but the response must be generated...

    bool head (false);
    ssize_t prelen;
    
    head = (req && req->hdr_cr ().mthd == HTTP_MTHD_HEAD);

    if (head) { gz = false; }
    if (_body) { 
      _body->to_strbuf (&body_buf, gz); 
      prelen = gz ? _body->inflated_len () : -1;
    }

    size_t contlen = body_buf.tosuio ()->resid ();

    hra.set_gzip (false);
    set_attributes (&hra);

    if (head) {
      rsp = New refcounted<http_response_head_t> (contlen, hra);
    } else {
      rsp = New refcounted<http_response_ok_t> (body_buf, hra);
    }
    if (prelen > 0) { rsp->set_inflated_len (prelen); }

  }

  // now make a final pass
  fixup_response (rsp);
  if (_uid_set) rsp->set_uid (_uid);
  if (!httpconst::is_error (_status)) { fixup_cookies (rsp); }

  svc->log (x, inhdrs, rsp);

  if (x && ok) {
    twait { rsp->send (x, mkevent ()); }
  }

  ev->trigger (ok);
}

//-----------------------------------------------------------------------

void
okclnt3_t::poke ()
{
  _output_cv.poke ();
}

//-----------------------------------------------------------------------

void
okclnt3_t::cv_t::wait (evv_t ev)
{
  if (_go) {
    assert (!_ev);
    _go = false;
    ev->trigger ();
  } else {
    _ev = ev;
  }
}

//-----------------------------------------------------------------------

void
okclnt3_t::cv_t::poke ()
{
  if (_ev) {
    assert (!_go);
    evv_t tmp (_ev);
    _ev = NULL;
    tmp->trigger ();
  } else {
    _go = true;
  }
}

//-----------------------------------------------------------------------

ptr<ahttpcon>
okclnt3_t::resp_t::con ()
{
  ptr<ahttpcon> ret;
  if (_ok_clnt) { ret = _ok_clnt->con (); }
  return ret;
}

//-----------------------------------------------------------------------

void
okclnt3_t::resp_t::fixup_cookies (ptr<http_response_t> rsp)
{
  for (size_t i = 0; i < _outcookies.size (); i++) {
    rsp->header.add (http_hdr_cookie_t (_outcookies[i]->to_str ()));
  }
}

//-----------------------------------------------------------------------

void
okclnt3_t::resp_t::fixup_response (ptr<http_response_t> rsp)
{
  if (_custom_log2) rsp->set_custom_log2 (_custom_log2);
}

//-----------------------------------------------------------------------

void
okclnt3_t::resp_t::set_hdr_field (const str &k, const str &v)
{
  if (!_hdr_fields)
    _hdr_fields = New refcounted<vec<http_hdr_field_t> > ();
  _hdr_fields->push_back (http_hdr_field_t (k,v));
}

//-----------------------------------------------------------------------
