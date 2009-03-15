
// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "okwcxml.h"
#include "tame_connectors.h"

#ifdef HAVE_EXPAT

namespace okwc3 {

//-----------------------------------------------------------------------

ptr<resp_t>
resp_xml_factory_t::alloc_resp (ptr<ok_xprt_base_t> x, ptr<abuf_t> b)
{
  ptr<resp_xml_t> r = New refcounted<resp_xml_t> (x, b, _id);
  _tab.insert (_id, r);
  _id++;
  return r;
}

//-----------------------------------------------------------------------

ptr<resp_xml_t>
resp_xml_factory_t::fetch (int id)
{
  ptr<resp_xml_t> *p = _tab[id];
  ptr<resp_xml_t> ret;
  if (p) {

    // D'oh! Make sure to grab a reference to this return value before
    // deallocating it.
    ret = *p; 
    _tab.remove (id);
  }
  return ret;
}

//-----------------------------------------------------------------------

tamed void
agent_xml_t::call_T (xml_outreq_t req, xml_ev_t ev)
{
  tvars {
    ptr<resp_xml_t> hresp;
    ptr<resp_t> gresp;
    xml_inresp_t resp;
    int status;
  }

  // Specify false here -> use the object allocator that parses XML
  // bodies and so on.  This is, do **not** use the simple allocator.
  // Unfortunately, don't want to upcast the return value from make_req, 
  // so need to kludge around it by fetching the upcasted object object 
  // out of the allocator.  Might want the revisit this interface ...
  // again :(.  okwc5 anyone?
  twait { make_req (req, false, connector::cnc (mkevent (status, gresp), ev)); }

  if (!gresp) {
    status = HTTP_BAD_REQUEST;
  } else {
    hresp = _xml_factory->fetch (gresp->get_id ());
    assert (hresp);
  }

  if (status == HTTP_OK) {
    resp = xml_inresp_t (hresp->top_level ()->get (0));
  }
  ev->trigger (status, resp);
}

//-----------------------------------------------------------------------

tamed void
agent_xml_t::call_to_T (xml_outreq_t req, xml_ev_t ev, int to)
{
  tvars {
    xml_inresp_t resp;
    int status;
  }
  if (to == 0) {
    twait { call_T (req, mkevent (status, resp)); }
  } else {
    status = HTTP_TIMEOUT;
    twait { call_T (req, connector::timeout (mkevent (status, resp), to, 0)); }
  }
  ev->trigger (status, resp);
}



//-----------------------------------------------------------------------

tamed void
agent_xml_t::make_req (xml_outreq_t req, bool simple, resp_ev_t ev)
{
  tvars {
    int status;
    ptr<req_xml_t> hreq;
    ptr<resp_t> resp;
    ptr<resp_factory_t> fct;
  }
  hreq = New refcounted<req_xml_t> (_reqinfo);
  req.output (hreq->zb ());

  if (simple) { fct = _simple_factory; } 
  else { fct = _xml_factory; }

  twait { 
    agent_t::req (hreq, fct , connector::cnc (mkevent (status, resp), ev)); 
  }
  ev->trigger (status, resp);
}

//-----------------------------------------------------------------------

tamed void
agent_xml_t::call_dump_T (xml_outreq_t req, evis_t ev)
{
  tvars {
    int status;
    str res;
    ptr<resp_t> resp;
  }

  twait { make_req (req, false, connector::cnc (mkevent (status, resp), ev)); }
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
  _abuf->setlim (sz);
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

agent_xml_t::agent_xml_t (const str &hn, int port, const str &u, bool proxied)
  : agent_t (hn, port),
    _reqinfo (proxied ? 
	      reqinfo_proxied_t::alloc (u) : 
	      reqinfo_direct_t::alloc (hn, port, u)),
    _xml_factory (New refcounted<resp_xml_factory_t> ()),
    _simple_factory (New refcounted<obj_factory_t> ())
{}

//-----------------------------------------------------------------------

};

#endif /* HAVE_EXPAT */
