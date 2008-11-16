
// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "okwcxml.h"
#include "tame_connectors.h"

#ifdef HAVE_EXPAT

namespace okwc3 {

//-----------------------------------------------------------------------

tamed void
agent_xml_t::call_T (xml_outreq_t req, xml_ev_t ev)
{
  tvars {
    ptr<resp_xml_t> hresp (New refcounted<resp_xml_t> ());
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
agent_xml_t::make_req (xml_outreq_t req, ptr<resp_t> resp, evi_t ev)
{

  tvars {
    int status;
    ptr<req_xml_t> hreq;
  }
  hreq = New refcounted<req_xml_t> (_reqinfo);
  req.output (hreq->zb ());
  twait { agent_t::req (hreq, resp, connector::cnc (mkevent (status), ev)); }
  ev->trigger (status);
}

//-----------------------------------------------------------------------

tamed void
agent_xml_t::call_dump_T (xml_outreq_t req, evis_t ev)
{
  tvars {
    int status;
    str res;
    ptr<resp_simple_t> resp (New refcounted<resp_simple_t> ());
  }

  twait { make_req (req, resp, connector::cnc (mkevent (status), ev)); }
  if (status == HTTP_OK) {
    res = resp->body ();
  }
  ev->trigger (status, res);
}

//-----------------------------------------------------------------------

tamed void
resp_xml_t::eat_chunk_T (size_t sz, evi_t ev)
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
resp_xml_t::finished_meal (int status, evi_t ev)
{
  ev->trigger (status);
}

//-----------------------------------------------------------------------

agent_xml_t::agent_xml_t (const str &hn, int port, const str &u, bool proxied,
			  bool https)
  : agent_t (hn, port),
    _reqinfo (proxied ? 
	      reqinfo_proxied_t::alloc (u, https) :
	      reqinfo_direct_t::alloc (hn, port, u, https))
{}



//-----------------------------------------------------------------------

};

#endif /* HAVE_EXPAT */
