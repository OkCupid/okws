
// -*-c++-*-
#include "resp2.h"

//-----------------------------------------------------------------------

tamed void
http_response_ok2_t::send2_T (ptr<ahttpcon> x, ev_ssize_t ev)
{
  tvars {
    strbuf b;
    ssize_t rc;
  }

  {
    _header.fill_strbuf (b);
    suio *uio = b.tosuio ();
    size_t pre = uio->resid ();
    _body->to_strbuf (&b, _compressed);
    size_t post = uio->resid ();
    _n_bytes = (post - pre);
  }
    
  twait { x->send2 (b, mkevent (rc)); }
  ev->trigger (rc);
}

//-----------------------------------------------------------------------
