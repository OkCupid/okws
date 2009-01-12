// -*-c++-*-

#include "ok3.h"
#include "tame_connectors.h"

//-----------------------------------------------------------------------

okclnt3_t::req_t::req_t (ptr<ahttpcon> x, u_int to)
  : http_parser_cgi_t (x, to) {}

//-----------------------------------------------------------------------

okclnt3_t::req_t::~req_t () {}

//-----------------------------------------------------------------------

okclnt3_t::resp_t::~resp_t ()
{
}

//-----------------------------------------------------------------------

okclnt3_t::resp_t::resp_t (okclnt3_t *o)
  : _ok_clnt (o),
    _uid (0),
    _uid_set (false), 
    _rsp_gzip (false),
    _sent (false),
    _replied (false) {}

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
okclnt3_t::resp_t::reply (int status, ptr<compressible_t> body, str redir_url)
{
  assert (!_replied);
  if (_ok_clnt) {
    _status = status;
    _body = body;
    _redir_url = redir_url;
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
okclnt3_t::alloc_resp ()
{
  ptr<resp_t> r = New refcounted<resp_t> (this);
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
  }

  output_loop (ok_clnt_timeout, mkevent (rv));

  if (go) go = check_ssl ();

  while (go) {
    req = New refcounted<req_t> (_x, ok_clnt_timeout);
    twait { req->parse (mkevent (status, go)); }
    if (status == HTTP_OK) {
      process (req, alloc_resp (), mkevent (rv));
    } else {
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
  go = (status == HTTP_OK && cicmp (hdr["connection"],"keep-alive"));
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

void
okclnt3_t::resp_t::send (evb_t ev)
{

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
