
#include "okxml.h"

ptr<xml_element_t>
xml_struct_t::get (const str &s)
{
  ptr<xml_element_t> *e = _members[s];
  if (e) { return *e; }
  else { return xml_null_t::alloc (); }
}

ptr<xml_element_t>
xml_element_t::get (const str &dummy) const
{
  return xml_null_t::alloc ();
}

ptr<xml_element_t>
xml_element_t::get (int i) const
{
  return xml_null_t::alloc ();
}

bool
xml_struct_t::put (const str &s, ptr<xml_element_t> e)
{
  _members[s] = e;
  return true;
}
