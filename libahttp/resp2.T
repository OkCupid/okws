
// -*-c++-*-
#include "resp2.h"

//-----------------------------------------------------------------------

http_response_ok2_t::http_response_ok2_t (const http_resp_attributes_t &a, 
					  ptr<compressible_t> x)
    : _header (a),
      _body (x),
      _uid (0),
      _n_bytes (0)
{
  fill ();
}

//-----------------------------------------------------------------------

void
http_response_ok2_t::fill ()
{
  if (_body) {
    make_body ();
    _n_bytes = _body_compressed.len ();
    set_inflated_len (_body->inflated_len ());
  } else {
    _n_bytes = 0;
  }

  _header.fill_outer (_n_bytes);
  _header.fill_strbuf (_out);
  if (!_header.is_head_request ()) {
    _out.take (_body_compressed);
  }
}


//-----------------------------------------------------------------------

tamed void
http_response_ok2_t::send2_T (ptr<ahttpcon> x, ev_ssize_t ev)
{
  tvars {
    holdvar ptr<http_response_ok2_t> hold (mkref (_self));
    ssize_t rc;
  }
  twait { x->send2 (_out, mkevent (rc)); }
  ev->trigger (rc);
}

//-----------------------------------------------------------------------

void
http_response_ok2_t::make_body ()
{
    compressible_t::opts_t o 
      = _header.get_attributes ().get_content_delivery ();
    _body->to_strbuf (&_body_compressed, o);
}

//-----------------------------------------------------------------------

u_int 
http_response_ok2_t::send (ptr<ahttpcon> x, cbv::ptr cb)
{
  if (_body) {
    make_body ();
  }
  u_int ret = _body_compressed.len ();
  x->send (_body_compressed, cb); 
  return ret;
}

//-----------------------------------------------------------------------
