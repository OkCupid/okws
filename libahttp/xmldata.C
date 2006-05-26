
#include "okxml.h"

ptr<xml_element_t> _dummy;

ptr<xml_element_t> xml_element_t::get (const str &dummy) const
{ return xml_null_t::alloc (); } 
ptr<xml_element_t> xml_element_t::get (size_t i) const
{ return xml_null_t::alloc (); }
ptr<xml_element_t> &xml_element_t::get_r (size_t i) { return _dummy; }
ptr<xml_element_t> &xml_element_t::get_r (const str &s) { return _dummy; }

ptr<xml_element_t>
xml_struct_t::get (const str &s) const
{
  const ptr<xml_element_t> *e = _members[s];
  if (e) { return *e; }
  else { return xml_null_t::alloc (); }
}

ptr<xml_element_t> &
xml_struct_t::get_r (const str &s) 
{
  ptr<xml_element_t> *e = _members[s];
  if (e) { return *e; }
  else { return _dummy; }
}

ptr<xml_element_t> 
xml_array_t::get (size_t i) const
{
  if (i < size ()) { return (*this)[i]; } 
  else { return xml_null_t::alloc (); }
}

ptr<xml_element_t> & 
xml_array_t::get_r (size_t i) 
{
  if (i < size ()) { return (*this)[i]; } 
  else { return _dummy; } 
}

bool
xml_struct_t::put (const str &s, ptr<xml_element_t> e)
{
  _members.insert (s, e);
  return true;
}

bool
xml_array_t::put (size_t i, ptr<xml_element_t> e)
{
  if (i >= size ())
    setsize (i + 1);
  (*this)[i] = e;
  return true;
}

