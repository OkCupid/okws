
#include "xmlrpcsrv.h"

#ifdef HAVE_EXPAT

void
okclnt_xmlrpc_base_t::reply (xml_resp_t r)
{
  set_content_type ("text/xml");
  out << "<?xml version=\"1.0\"?>\n";
  r.output (out);
  output (out);
}

u_int64_t xml_rpc_id = 0;

u_int64_t
xml_rpc_new_global_id ()
{
  return ++xml_rpc_id;
}

TAMED void
okclnt_xmlrpc_t::handle_mutlicall (xml_req_t q, xml_resp_cb_t cb)
{
  VARS {
    size_t i;
    str nm;
    vec<xml_resp_t> r2;
    xml_resp_t r;
    ptr<const xml_container_t> xc;
    typename oksrvc_xmlrpc_t<C,S>::handler_t *h;
    C *cli (reinterpret_cast<C *> (c));
    vec<size_t> calls;
  }

  r2.setsize (w.size ());

  BLOCK {
    for (i = 0; i < w.size (); i++) {
      if (!(nm = w[i]("methodName"))) {
	r[i] = xml_fault_obj_t (OK_XMLRPC_ERR_NO_METHOD_CALL,
			     "No methodCall for mutlicall call");
      } else if (nm == MULTICALL) {
	r[i] = xml_fault_obj_t (OK_XMLRPC_ERR_RECURSIVE,
			     "Cannot make recursive multicalls");
      } else if (!(xc = w[i]("params").to_xml_container ())) {
	r[i] = xml_fault_obj_t (OK_XMLRPC_ERR_BAD_PARAMS,
				"Bad parameters to mutlicall");
      } else {
	calls.push_back (i);
	_srvc->handle (this, nm, xml_req_t (xc), @(r2[i]));
      }
    }
  }

  for (i = 0; i < calls.size (); i++)
    r[calls[i]][0] = r2[calls[i]];

  (*cb) (r);
}

void
oksrvc_xmlrpc_base_t::set_debug_level ()
{}

void
oksrvc_xmlrpc_base_t::set_debug_level (int i)
{}

#endif /* HAVE_EXPAT */
