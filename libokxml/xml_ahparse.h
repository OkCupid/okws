// -*-c++-*-
#ifndef __LIBXML_XML_AHPARSE__
#define  __LIBXML_XML_AHPARSE__

#include "okwsconf.h"
#ifdef HAVE_EXPAT

#include "ahparse.h"
#include "okxmlparse.h"

class http_parser_xml_t : public http_parser_full_t {
public:
  http_parser_xml_t (ptr<ahttpcon> xx, int to = 0)
    : http_parser_full_t (xx, to), _xml (_abuf) {}
  ~http_parser_xml_t () {}

  void v_cancel () ;
  void v_parse_cb1 (int status) ;

  ptr<xml_top_level_t> top_level () { return _xml.top_level (); }

  ptr<const xml_top_level_t> top_level () const 
  { return _xml.top_level (); }

  str errmsg () const { return _xml.errmsg (); }
  int errcode () const { return _xml.errcode (); }

private:
  xml_req_parser_t _xml;

};
#endif /* HAVE_EXPAT */

#endif /* __LIBXML_XML_AHPARSE__ */
