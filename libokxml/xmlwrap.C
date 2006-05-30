
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

xml_const_wrap_t
xml_wrap_base_t::operator[] (size_t i) const
{
  ptr<const xml_container_t> c;
  ptr<const xml_element_t> e;
  c = to_xml_container_const ();
  if (c) e = c->get (i);
  else e = xml_null_t::alloc ();
  return xml_const_wrap_t (e);
}

xml_const_wrap_t
xml_wrap_base_t::operator() (const str &i) const
{
  ptr<const xml_struct_t> s;
  ptr<const xml_element_t> e;
  s = to_xml_struct_const ();
  if (s) e = s->get (i);
  else e = xml_null_t::alloc ();
  return xml_const_wrap_t (e);
}
