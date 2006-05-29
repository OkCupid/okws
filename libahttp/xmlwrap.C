
#include "okxml.h"
#include "okxmlwrap.h"

const xml_wrap_t &
xml_wrap_t::set_value (ptr<xml_element_t> e)
{
  ptr<xml_value_t> nv = xml_value_t::alloc (e);
  ptr<xml_param_t> p;
  if (_el && (p = _el->to_xml_param ())) {
    p->set_value (nv);
  } else {
    _el = nv;
  }
  return (*this);
}
