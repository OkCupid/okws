
#include "xml_ahparse.h"

#ifdef HAVE_EXPAT

void
http_parser_xml_t::v_cancel ()
{
  hdr.cancel ();
  _xml.cancel ();
}

void
http_parser_xml_t::v_parse_cb1 (int status)
{
  if (hdr.mthd == HTTP_MTHD_POST) {

    cbi::ptr pcb;
    if (!(pcb = prepare_post_parse (status)))
      return;
    
    _xml.init ();
    _xml.parse (pcb);

  } else {
    finish (HTTP_NOT_ACCEPTABLE);
  }
}

#endif /* HAVE_EXPAT */
