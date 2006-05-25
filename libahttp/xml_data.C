
#include "okxml.h"

ptr<xml_element_t> _dummy;

ptr<xml_element_t> xml_element_t::get (const str &dummy) const
{ return xml_null_t::alloc (); } 

ptr<xml_element_t> xml_element_t::get (int i) const
{ return xml_null_t::alloc (); }

ptr<xml_element_t> &xml_element_t::get_r (int i) { return _dummy; }

ptr<xml_element_t> &xml_element_t::get_r (const str &s) const
{ return _dummy; }

ptr<xml_element_t>
xml_struct_t::get (const str &s) const
{
  ptr<xml_element_t> *e = _members[s];
  if (e) { return *e; }
  else { return xml_null_t::alloc (); }
}

ptr<xml_element_t> &
xml_struct_t::get_r (const str &s) const
{
  ptr<xml_element_t> *e = _members[s];
  if (e) { return *e; }
  else { return _dummy; }
}

ptr<xml_element_t> 
xml_array_t::get (int i) const
{
  if (i < _elements.size ()) { return _elements[i]; } 
  else { return xml_null_t::alloc (); }
}

ptr<xml_element_t> & 
xml_array_t::get_r (int i) 
{
  if (i < _elements.size ()) { return _elements[i]; } 
  else { return _dummy; } 
}

bool
xml_struct_t::put (const str &s, ptr<xml_element_t> e)
{
  _members[s] = e;
  return true;
}

bool
xml_array_t::put (int i, ptr<xml_element_t> e)
{
  if (i >= _elements.size ())
    _elements.setsize (i + 1);
  _elements[i] = e;
  return true;
}


