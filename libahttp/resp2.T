
// -*-c++-*-
#include "resp2.h"

//-----------------------------------------------------------------------

http_response_ok2_t::http_response_ok2_t (const http_resp_attributes_t &a, 
					  ptr<compressible_t> x)
    : _header (a),
      _body (x),
      _compressed (a.get_gzip ()),
      _uid (0),
      _n_bytes (0) 
{
  fill ();
}

//-----------------------------------------------------------------------

void
http_response_ok2_t::fill ()
{
  strbuf b2;
  if (_body) {
    _body->to_strbuf (&b2, _compressed);
    _n_bytes = b2.tosuio ()->resid ();
    set_inflated_len (_body->inflated_len ());
  } else {
    _n_bytes = 0;
    _compressed = false;
  }

  _header.fill (_compressed, _n_bytes);
  _header.fill_strbuf (_out);
  _out.borrow_data (b2);
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
