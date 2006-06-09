
#include "okxml.h"
#include "okxmlobj.h"

const xml_obj_ref_t &
xml_obj_ref_t::set_value (ptr<xml_element_t> e)
{
  if (!_el_ref || !_el_ref->assign_to (e))
    _el_ref = e;
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

ptr<xml_container_t>
xml_obj_ref_t::coerce_to_container ()
{
  ptr<xml_container_t> c;
  if (!_el_ref || !(c = _el_ref->to_xml_container ())) {
    _el_ref = New refcounted<xml_array_t> ();
    c = _el_ref->to_xml_container ();
  }
  return c;
}

xml_obj_ref_t 
xml_obj_ref_t::operator[] (size_t i) 
{ 
  return xml_obj_ref_t (coerce_to_container ()->get_r (i));
}

void
xml_obj_ref_t::setsize (size_t i)
{
  coerce_to_container ()->setsize (i);
}

xml_obj_ref_t 
xml_obj_ref_t::operator() (const str &i) 
{ 
  ptr<xml_struct_t> s; 

  // corce object to a struct if not one already
  if (!_el_ref || !(s = _el_ref->to_xml_struct ())) {
    _el_ref = New refcounted<xml_struct_t> ();
    s = _el_ref->to_xml_struct ();
  }
  return xml_obj_ref_t (s->get_r (i));
}

xml_obj_t 
xml_obj_base_t::clone () const
{
  return xml_obj_t (el ()->clone ());
}
