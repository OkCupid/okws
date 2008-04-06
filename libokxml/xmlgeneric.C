
// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "okxmlgeneric.h"

#include "okwsconf.h"
#ifdef HAVE_EXPAT

ptr<xml_element_t>
xml_req_parser_generic_t::generate (const char *nm, const char **atts)
{
  return New refcounted<xml_generic_t> (nm, atts);
}

void
xml_req_parser_generic_t::v_init ()
{
  const char **e = NULL;
  _top_level_g = New refcounted<xml_generic_t> ("TopLevel", e);
  push_el (_top_level_g);
}

#endif /* HAVE_EXPAT */
