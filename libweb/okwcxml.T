
// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "okwcxml.h"
#include "tame_connectors.h"

#ifdef HAVE_EXPAT

//-----------------------------------------------------------------------

tamed void
okwc3_xml_t::call_T (xml_outreq_t req, okwc3_xml_ev_t ev)
{
  tvars {
    ptr<okwc3_resp_xml_t> hresp (New refcounted<okwc3_resp_xml_t> ());
    xml_inresp_t resp;
    int status;
  }
  twait { make_req (req, hresp, connector::cnc (mkevent (status), ev)); }
  if (status == HTTP_OK) {
    resp = xml_inresp_t (hresp->top_level ()->get (0));
  }
  ev->trigger (status, resp);
}

//-----------------------------------------------------------------------

tamed void
okwc3_xml_t::make_req (xml_outreq_t req, ptr<okwc3_resp_t> resp, evi_t ev)
{

  tvars {
    int status;
    ptr<okwc3_req_xml_t> hreq;
  }
  hreq = New refcounted<okwc3_req_xml_t> (_hostname, _url);
  req.output (hreq->zb ());
  twait { okwc3_t::req (hreq, resp, connector::cnc (mkevent (status), ev)); }
  ev->trigger (status);
}

//-----------------------------------------------------------------------

tamed void
okwc3_xml_t::call_dump_T (xml_outreq_t req, evis_t ev)
{
  tvars {
    int status;
    str res;
    ptr<okwc3_resp_simple_t> resp (New refcounted<okwc3_resp_simple_t> ());
  }

  twait { make_req (req, resp, connector::cnc (mkevent (status), ev)); }
  if (status == HTTP_OK) {
    res = resp->body ();
  }
  ev->trigger (status, res);
}

//-----------------------------------------------------------------------

tamed void
okwc3_resp_xml_t::eat_chunk_T (size_t sz, evi_t ev)
{
  tvars {
    int status;
    outcome_t outc;
  }
  _abuf.setlim (sz);
  _parser.init ();
  twait { _parser.parse (connector::cnc (mkevent (status), ev, &outc)); }
  if (outc == OUTCOME_CANCELLED) {
    _parser.cancel ();
    status = CANCELLED_STATUS;
  }
  ev->trigger (status);
}

//-----------------------------------------------------------------------

void
okwc3_resp_xml_t::finished_meal (int status, evi_t ev)
{
  ev->trigger (status);
}

#endif /* HAVE_EXPAT */
