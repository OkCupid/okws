// -*-c++-*-
// $Revision$

#include "okxml.h"
#include <expat.h>

static void 
start_element_handler (void *pv, const char *name, const char **atts)
{
  xml_req_parser_t *parser = static_cast<xml_req_parser_t *> (pv);
  parser->start_element (name, atts);
}

static void
end_element_handler (void *pv, const char *name)
{
  xml_req_parser_t *parser = static_cast<xml_req_parser_t *> (pv);
  parser->end_element (name);
}

static void
character_data_handler (void *pv, const char *buf, int len)
{
  xml_req_parser_t *parser = static_cast<xml_req_parser_t *> (pv);
  parser->found_data (buf, len);
}


xml_req_parser_t::~xml_req_parser_t ()
{
  if (_xml_parser_init) {
    XML_ParserFree (_xml_parser);
  }
}

void
xml_req_parser_t::init (const char *encoding)
{
  assert (!_xml_parser_init);
  _xml_parser = XML_ParserCreate (encoding);
  XML_SetUserData (_xml_parser, this);
  XML_SetElementHandler (_xml_parser, start_element_handler, 
			 end_element_handler);
  XML_SetCharacterDataHandler (_xml_parser, character_data_handler);
}

void
xml_req_parser_t::parse_guts ()
{
  char *b;
  ssize_t sz;
  enum XML_Status xstat;

  do {
    sz = abuf->stream (&b);
    if (sz == ABUF_EOFCHAR) {
      xstat = XML_Parse (_xml_parser, NULL, 0, 1);
      finish_parse (0);
    } else if (sz >= 0) {
      xstat = XML_Parse (_xml_parser, b, sz, 0);
    } else {
      assert (sz == ABUF_WAITCHAR);
      // else, we're just waiting...
    }
  } while (sz >= 0);

}
