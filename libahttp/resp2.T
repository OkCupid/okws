
// -*-c++-*-
#include "resp2.h"

//-----------------------------------------------------------------------

tamed void
http_response_ok2_t::send2_T (ptr<ahttpcon> x, ev_ssize_t ev)
{
  tvars {
    strbuf b;
  }
  _header.fill_strbuf (b);
  _body->to_strbuf (&b, _compressed);
}

//-----------------------------------------------------------------------
