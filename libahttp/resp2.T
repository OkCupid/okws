
// -*-c++-*-
#include "resp2.h"

//-----------------------------------------------------------------------

tamed void
http_response_ok2_t::send2_T (ptr<ahttpcon> x, ev_ssize_t ev)
{
  tvars {
    strbuf b, b2;
    ssize_t rc;
  }

  if (_body) {
    _body->to_strbuf (&b2, _compressed);
    _n_bytes = b2.tosuio ()->resid ();
  } else {
    _n_bytes = 0;
    _compressed = false;
  }

  _header.fill (_compressed, _n_bytes);
  _header.fill_strbuf (b);
  b << b2;

  twait { x->send2 (b, mkevent (rc)); }
  ev->trigger (rc);
}

//-----------------------------------------------------------------------

u_int 
http_response_ok2_t::send (ptr<ahttpcon> x, cbv::ptr cb)
{
  strbuf b;
  if (_body) {
    _body->to_strbuf (&b, _compressed);
  }
  u_int ret = b.tosuio ()->resid ();
  x->send (b, cb); 
  return ret;
}

//-----------------------------------------------------------------------
