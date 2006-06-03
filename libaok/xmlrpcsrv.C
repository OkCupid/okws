
#include "xmlrpcsrv.h"

#ifdef HAVE_EXPAT

void
okclnt_xmlrpc_base_t::reply (xml_resp_t r)
{
  r.output (out);
  output (out);
}

#endif /* HAVE_EXPAT */
