// -*-c++-*-

#include "ok3.h"

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
    _union_cgi_mode (false),
    _output_done (false) {}

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
okclnt3_t::poke ()
{
  if (_poke_ev) {
    evv_t tmp = _poke_ev;
    _poke_ev = NULL;
    tmp->trigger ();
  }
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
  }

  output_loop ();

  if (go) go = check_ssl ();

  while (go) {
    req = New refcounted<req_t> (_x, ok_clnt_timeout);
    twait { req->parse (mkevent (status, go)); }
    if (status == HTTP_OK) {
      process (req, alloc_resp ());
    } else {
      error (status);
    }
  }

  // wait for the output loop to flush completely
  twait { finish_output (mkevent ()); }

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
okclnt3_t::output_loop ()
{
  tvars {
    ptr<okclnt3_t::resp_t> rsp;
    bool go (true);
  }

  while (go && (!_output_done_ev || _resps.size ())) {
    rsp = _resps.pop_front ();
    while (!rsp->is_ready ()) {
      twait { await_poke (mkevent ()); }
    }
    twait { rsp->send (mkevent (go)); }
  }

  if (_output_done_ev)
    _output_done_ev->trigger ();
}

//-----------------------------------------------------------------------

void
okclnt3_t::finish_output (evv_t ev)
{
  if (_output_done) ev->trigger ();
  else _output_done_ev = ev;
}

//-----------------------------------------------------------------------

void
okclnt3_t::await_poke (evv_t ev)
{
  if (_poke_ev) ev->trigger ();
  else _poke_ev = ev;
}

//-----------------------------------------------------------------------

void
okclnt3_t::resp_t::send (evb_t ev)
{

}

//-----------------------------------------------------------------------
