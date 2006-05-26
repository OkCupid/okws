
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

bool
xml_container_t::add (ptr<xml_element_t> e)
{
  bool ret = false;
  if (can_contain (e)) {
    push_back (e);
    ret = true;
  }
  return ret;
}

bool
xml_method_call_t::add (ptr<xml_element_t> e)
{
  ptr<xml_params_t> p;
  ptr<xml_method_name_t> m;
  bool ret = true;

  if ((p = e->to_xml_params ())) {
    if (_params) {
      // error -- already set
      ret = false;
    } else {
      _params = p;
    }
  } else if ((m = e->to_xml_method_name ())) {
    if (_method_name) {
      // error -- already set
      ret = false;
    } else {
      _method_name = m->value ();
    }
  } else {
    ret = false;
  }
  return ret;
}

bool
xml_param_t::add (ptr<xml_element_t> e)
{
  return (!_value && (_value = e->to_xml_value ()));
}

bool
xml_value_t::add (ptr<xml_element_t> e)
{
  return (!_e && e->is_value () && (_e = e));
}

bool
xml_member_t::add (ptr<xml_element_t> e)
{
  ptr<xml_value_t> v;
  ptr<xml_name_t> n;
  if ((v = e->to_xml_value ())) {

  } else if ((n = e->to_xml_name ())) {

  } else {

  }

}


bool
xml_struct_t::close_tag ()
{
  bool succ = true;
  while (size ()) {
    ptr<xml_member_t> m = pop_back ()->to_xml_member ();
    str n;
    ptr<xml_value_t> v;
    assert (m);
    if (m->member_name ()) n = m->member_name ()->value ();
    v = m->member_value ();
    if (n && v) {
      _members.insert (n, v);
    } else {
      // an error;
      succ = false;
    }
  }
  return succ;
}
