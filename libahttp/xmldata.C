// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

/*
 *
 * Copyright (C) 2002-2004 Maxwell Krohn (max@okcupid.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#include "okxml.h"
#include "parseopt.h"
#include <stdlib.h>

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
  if (_data && i < _data->size ()) { return (*_data)[i]; } 
  else { return xml_null_t::alloc (); }
}

ptr<xml_element_t> & 
xml_array_t::get_r (size_t i) 
{
  if (_data && i < _data->size ()) { return (*_data)[i]; } 
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
  if (!_data) 
    _data = xml_data_t::alloc ();

  if (i >= _data->size ())
    _data->setsize (i + 1);
  (*_data)[i] = e;
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
  bool ret = true;
  if ((v = e->to_xml_value ())) {
    if (_member_value) {
      // error, duplicate
      ret = false;
    } else {
      _member_value = v;
    }
  } else if ((n = e->to_xml_name ())) {
    if (_member_name) {
      ret = false;
    } else {
      _member_name = n;
    }
  } else {
    ret = false;
  }
  return ret;
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

bool
xml_array_t::add (ptr<xml_element_t> e)
{
  return (!_data && (_data = e->to_xml_data ()));
}

bool
xml_int_t::close_tag ()
{
  str s (_buf);
  return convertint (s, &_val);
}

bool
xml_str_t::close_tag ()
{
  _val = _buf;
  return true;
}

bool
xml_double_t::close_tag ()
{
  str s (_buf);
  char *ep;
  _val = strtod (s.cstr (), &ep);
  return (*ep == '\0' && errno != ERANGE);
}

bool
xml_scalar_t::add (const char *b, int i)
{
  _buf.tosuio ()->copy (b, i);
  return true;
}
