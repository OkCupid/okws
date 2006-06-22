
// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "okwcxml.h"

#ifdef HAVE_EXPAT

TAMED void
okwc2_xml_t::call_T (xml_outreq_t req, okwc2_xml_cb_t cb, int to)
{
  VARS {
    ptr<okwc2_req_xml_t> hreq;
  }
  hreq = New refcounted<okwc2_req_xml_t> (_hostname, _url);
  req.output (hreq->zb ());

}


#endif /* HAVE_EXPAT */
