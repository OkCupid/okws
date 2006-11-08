
// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "okwcxml.h"

#ifdef HAVE_EXPAT

//-----------------------------------------------------------------------

tamed void
okwc2_xml_t::call_T (xml_outreq_t req, okwc2_xml_cb_t cb, int to)
{
  tvars {
    ptr<okwc2_resp_xml_t> hresp (New refcounted<okwc2_resp_xml_t> ());
    xml_inresp_t resp;
    int status;
  }
  twait { make_req (req, hresp, to, mkevent (status)); }
  if (status == HTTP_OK) {
    resp = xml_inresp_t (hresp->top_level ()->get (0));
  }
  TRIGGER (cb, status, resp);
}

//-----------------------------------------------------------------------

tamed void
okwc2_xml_t::make_req (xml_outreq_t req, ptr<okwc2_resp_t> resp, int to,
		       cbi cb)
{

  tvars {
    int status;
    ptr<okwc2_req_xml_t> hreq;
  }
  hreq = New refcounted<okwc2_req_xml_t> (_hostname, _url);
  req.output (hreq->zb ());
  twait { timed_req (hreq, resp, to, mkevent (status)); }
  TRIGGER (cb, status);

}

//-----------------------------------------------------------------------

tamed void
okwc2_xml_t::call_dump_T (xml_outreq_t req, cbis cb, int to)
{
  tvars {
    int status;
    str res;
    ptr<okwc2_resp_simple_t> resp (New refcounted<okwc2_resp_simple_t> ());
  }

  twait { make_req (req, resp, to, mkevent (status)); }
  if (status == HTTP_OK) {
    res = resp->body ();
  }
  TRIGGER (cb, status, res);
}

//-----------------------------------------------------------------------

tamed void
okwc2_resp_xml_t::eat_chunk_T (ptr<canceller_t> cncl, size_t sz, cbi cb)
{
  tvars {
    rendezvous_t<bool> G (__FILE__, __LINE__);
    int status;
    bool cancelled;
  }
  _abuf.setlim (sz);
  _parser.init ();
  _parser.parse (mkevent (G,false, status));
  twait { cancellable_wait (cncl, G, mkevent (cancelled)); }
  if (cancelled) {
    _parser.cancel ();
    status = CANCELLED_STATUS;
  }
  TRIGGER (cb, status);
}

//-----------------------------------------------------------------------

void
okwc2_resp_xml_t::finished_meal (ptr<canceller_t> cncl, int status, cbi cb)
{
  TRIGGER (cb, status);
}

#endif /* HAVE_EXPAT */
