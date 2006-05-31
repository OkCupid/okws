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
#include "okxmlwrap.h"

ptr<xml_null_t> null_element (New refcounted<xml_null_t> ());
ptr<xml_value_t> null_value (New refcounted<xml_value_t> ());

int xml_data_init::count;

void
xml_data_init::start ()
{
}

void xml_data_init::stop () {}

ptr<const xml_element_t>
xml_struct_t::get (const str &s) const
{
  const size_t *i = _members[s];
  ptr<xml_element_t> e;
  ptr<xml_member_t> m;
  ptr<const xml_element_t> v;
  if (i && (e = (*this)[*i]) && 
      (m = e->to_xml_member ()) && (v = m->member_value_const ()))
    return v;
  else
    return xml_null_t::alloc ();
}

ptr<xml_element_t> &
xml_struct_t::get_r (const str &s) 
{
  size_t *i = _members[s];
  ptr<xml_element_t> e;
  ptr<xml_member_t> m;
  ptr<xml_value_t> v;

  if (i) { 
    e = (*this)[*i]; 
    if (e) {
      assert ((m = e->to_xml_member ()));
    } else {
      _members.remove (s);
    }
  }

  if (!m) {
    m = New refcounted<xml_member_t> (s);
    push_back (m);
    _members.insert (s, size () - 1);
  }

  return m->member_value ();
}

ptr<xml_container_t>
xml_array_t::to_xml_container ()
{
  if (!_data) _data = New refcounted<xml_data_t> ();
  return _data;
}

ptr<xml_value_t>
xml_value_wrapper_t::mkvalue ()
{
  if (!_value) _value = New refcounted<xml_value_t> ();
  return _value;
}

ptr<xml_container_t>
xml_value_wrapper_t::to_xml_container ()
{
  return mkvalue ()->to_xml_container ();
}

ptr<xml_struct_t>
xml_value_wrapper_t::to_xml_struct ()
{
  return mkvalue ()->to_xml_struct ();
}

ptr<xml_container_t>
xml_value_t::to_xml_container ()
{
  ptr<xml_container_t> r;
  if (!_e || !(r = _e->to_xml_container ())) {
    _e = New refcounted<xml_array_t> ();
    r = _e->to_xml_container ();
  }
  return r;
}

ptr<xml_struct_t>
xml_value_t::to_xml_struct ()
{
  ptr<xml_struct_t> s;
  if (!_e || !(s = _e->to_xml_struct ())) {
    _e = New refcounted<xml_struct_t> ();
    s = _e->to_xml_struct ();
  }
  return s;
}

bool
xml_struct_t::put (const str &s, ptr<xml_element_t> e)
{
  get_r (s) = e;
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
      _method_name = m;
    }
  } else {
    ret = false;
  }
  return ret;
}

bool
xml_value_wrapper_t::add (ptr<xml_element_t> e)
{
  return (!_value && (_value = e->to_xml_value ()));
}


bool 
xml_method_response_t::add (ptr<xml_element_t> e)
{
  ptr<xml_params_t> p;
  if ((p = e->to_xml_params ())) {
    p = e->to_xml_params ();
    _body = e;
  } else if (e->to_xml_fault ()) {
    _body = e;
  } else {
    return false;
  }
  return true;
}

ptr<const xml_container_t> 
xml_value_wrapper_t::to_xml_container_const () const
{ return _value ? _value->to_xml_container_const () : NULL; }

ptr<const xml_struct_t> 
xml_value_wrapper_t::to_xml_struct_const () const
{ return _value ? _value->to_xml_struct_const () : NULL; }

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
  for (size_t i = 0; i < _members.size (); i++) {
    ptr<xml_member_t> m = (*this)[i]->to_xml_member ();
    assert (m);
    str n;
    if (m->member_name ()) {
      n = m->member_name ()->value ();
      _members.insert (n, i);
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
xml_bool_t::close_tag ()
{
  str s (_buf);
  int tmp;
  if (convertint (s, &tmp)) {
    _val = tmp;
  } else if (!strcasecmp (s.cstr (), "true")) {
    _val = true;
  } else if (!strcasecmp (s.cstr (), "false")) {
    _val = false;
  } else {
    return false;
  }
  return true;
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
xml_scalar_t::add (const char *b, int len)
{
  if (!has_non_ws (b, len))
    return false;

  _buf.tosuio ()->copy (b, len);
  return true;
}

bool
xml_base64_t::close_tag ()
{
  _val = _buf;
  return true;
}

static void spaces (zbuf &b, int n)
{
  for (int i = 0; i < n; i++) {
    b << " ";
  }
}

void
xml_element_t::dump (zbuf &b, int lev) const
{
  if (name ()) {
    spaces (b, lev);
    b << "<" << name () << ">";
    if (!gets_char_data ()) b << "\n";
  }
  dump_data (b, lev + 1);
  if (name ()) {
    if (!gets_char_data ())
      spaces (b, lev);
    b << "</" << name () << ">\n";
  }
}

void
xml_container_t::dump_data (zbuf &b, int lev) const
{
  for (size_t i = 0; i < size (); i++) {
    ptr<xml_element_t> el = (*this)[i];
    if (el) {
      el->dump (b, lev);
    } else {
      null_value->dump (b, lev);
    }
  }
}

void
xml_method_call_t::dump_data (zbuf &b, int lev) const
{
  if (_method_name) _method_name->dump (b, lev);
  if (_params)      _params->dump (b, lev);
}

void
xml_member_t::dump_data (zbuf &b, int lev) const
{
  if (_member_name)  _member_name->dump (b, lev);
  if (_member_value) _member_value->dump (b, lev);
}

void
xml_value_wrapper_t::dump_data (zbuf &b, int lev) const
{
  if (_value) { _value->dump (b, lev); }
}

void
xml_double_t::dump_data (zbuf &b, int lev) const
{
#define DOUBLEBUFLEN 0x40
  static char buf[DOUBLEBUFLEN];
  int rc = snprintf (buf, DOUBLEBUFLEN, "%g", _val);
  if (rc < DOUBLEBUFLEN) {
    b.cat (buf, rc, true);
  } else {
    b << "NaN";
  }
#undef DOUBLEBUFLEN
}

bool
xml_method_name_t::add (const char *buf, int len)
{
  if (!has_non_ws (buf, len))
    return false;
  _value = str (buf, len);
  return true;
}

bool
has_non_ws (const char *buf, int len)
{
  for (const char *ep = buf + len, *cp = buf; cp < ep; cp ++) {
    if (!isspace (*cp)) {
      return true;
    }
  }
  return false;
}

ptr<xml_container_t> 
xml_method_response_t::to_xml_container ()
{
  if (!_params) {
    _params = New refcounted<xml_params_t> ();
    _body = _params;
  }
  return _params;
}

ptr<xml_element_t> &
xml_container_t::get_r (size_t i)
{
  if (i >= size ())
    setsize (i+1);
  return (*this)[i];
}

ptr<xml_element_t> &
xml_params_t::get_r (size_t i)
{
  ptr<xml_element_t> &r = xml_container_t::get_r (i);
  if (!r) {
    r = New refcounted<xml_param_t> ();
  }
  return r;
}

ptr<const xml_element_t> 
xml_container_t::get (size_t i) const
{
  if (i >= size ()) { return xml_null_t::alloc (); }
  else return (*this)[i];
}

void 
xml_array_t::dump_data (zbuf &z, int lev) const 
{ if (_data) _data->dump (z, lev); }

ptr<xml_fault_t>
xml_fault_t::alloc (int rc, const str &s)
{
  ptr<xml_fault_t> f (New refcounted<xml_fault_t> ());
  ptr<xml_element_t> e = f;

  xml_wrap_t w (e);
  w("faultCode") = rc;
  w("faultString") = s;

  return f;
}
