
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

#endif /* HAVE_EXPAT */
