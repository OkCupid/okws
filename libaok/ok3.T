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

okclnt3_t::resp_t::resp_t ()
  : _uid (0),
    _uid_set (false), 
    _rsp_gzip (false),
    _sent (false)
{

}

//-----------------------------------------------------------------------

tamed void
okclnt3_t::resp_t::send (int status, zbuf *body, evi_t ev)
{

}

//-----------------------------------------------------------------------

tamed void
okclnt3_t::resp_t::redirect (const str &loc, int status, evi_t ev)
{

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







