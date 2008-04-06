
// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */


#ifndef _LIBOKXML_OKXMLGENERIC_H_
#define _LIBOKXML_OKXMLGENERIC_H_

// Only use this library if we have EXPAT support
#include "okwsconf.h"
# ifdef HAVE_EXPAT

#include "okxmlparse.h"

class xml_req_parser_generic_t : public xml_req_parser_t {
public:
  xml_req_parser_generic_t (abuf_src_t *s) :
    xml_req_parser_t (s) {}
  xml_req_parser_generic_t (abuf_t *a) :
    xml_req_parser_t (a) {}

  void v_init ();
  ptr<xml_generic_t> top_level_g () { return _top_level_g; }
  ptr<const xml_generic_t> top_level_g () const { return _top_level_g; }

  ptr<xml_element_t> generate (const char *nm, const char **atts);

private:
  ptr<xml_generic_t> _top_level_g;

};


# endif /* HAVE_EXPAT */
#endif /* _LIBOOKXML_OKXMLGENERIC_H _*/
