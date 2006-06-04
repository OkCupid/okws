
#include "okxml.h"
#include "okxmlobj.h"

const xml_obj_ref_t &
xml_obj_ref_t::set_value (ptr<xml_element_t> e)
{
  ptr<xml_value_t> nv = xml_value_t::alloc (e);
  ptr<xml_param_t> p;
  if (_el_ref && (p = _el_ref->to_xml_param ())) {
    p->set_value (nv);
  } else {
    _el_ref = nv;
  }
  return (*this);
}

xml_obj_const_t
xml_obj_base_t::operator[] (size_t i) const
{
  ptr<const xml_container_t> c;
  ptr<const xml_element_t> e;
  c = to_xml_container ();
  if (c) e = c->get (i);
  else e = xml_null_t::alloc ();
  return xml_obj_const_t (e);
}

xml_obj_const_t
xml_obj_base_t::operator() (const str &i) const
{
  ptr<const xml_struct_t> s;
  ptr<const xml_element_t> e;
  s = to_xml_struct ();
  if (s) e = s->get (i);
  else e = xml_null_t::alloc ();
  return xml_obj_const_t (e);
}


xml_obj_t 
xml_obj_base_t::clone () const
{
  return xml_obj_t (el ()->clone ());
}
