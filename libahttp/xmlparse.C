// -*-c++-*-
// $Revision$

#include "okxml.h"
#include <expat.h>


TAMED void
xml_req_parser_t::parse_guts ()
{
  char *b;
  ssize_t sz;
  enum XML_Status xstat;

  do {
    sz = abuf->stream (&b);
    if (sz == ABUF_EOFCHAR) {
      xstat = XML_Parser (_xml_parser, NULL, 0, 1);
      finish_parse (0);
    } else if (sz >= 0) {
      xstat = XML_Parse (_xml_parser, b, sz, 0);
    } else {
      assert (sz == ABUF_WAITCHAR);
      // else, we're just waiting...
    }
  } while (sz >= 0);

}
