
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

#endif /* HAVE_EXPAT */
