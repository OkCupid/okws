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
#include "okxmlobj.h"
#include "rxx.h"

ptr<xml_null_t> null_element (New refcounted<xml_null_t> ());
ptr<xml_value_t> null_value (New refcounted<xml_value_t> ());

ptr<const xml_element_t>
xml_struct_t::get (const str &s) const
{
  const size_t *i = _members[s];
  ptr<xml_element_t> e;
  ptr<const xml_member_t> m;
  ptr<const xml_element_t> v;

  if (i && (e = (*this)[*i]) && (m = e->to_xml_member ()) && 
      (v = m->member_value ()))
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
xml_value_wrapper_t::to_xml_container () const
{ return _value ? _value->to_xml_container () : NULL; }

ptr<const xml_struct_t> 
xml_value_wrapper_t::to_xml_struct () const
{ return _value ? _value->to_xml_struct () : NULL; }

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
  for (size_t i = 0; i < size (); i++) {
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

bool
xml_container_t::dump_to_python (strbuf &b) const
{
  b << "(";
  for (size_t i = 0; i < size (); i++) {
    ptr<xml_element_t> el = (*this)[i];
    if (i > 0) b << ", ";
    if (el) { el->dump_to_python (b); }
    else { b << "None"; }
  }
  b << ")";
  return true;
}

void
xml_method_call_t::dump_data (zbuf &b, int lev) const
{
  if (_method_name) _method_name->dump (b, lev);
  if (_params)      _params->dump (b, lev);
}

bool
xml_method_call_t::dump_to_python (strbuf &b) const
{
  if (!_method_name  || !_method_name->dump_to_python (b))
    b << "unknownMethod"; 

  if (!_params || !_params->dump_to_python (b))
    b << "()";
  return true;
}

bool
xml_method_name_t::dump_to_python (strbuf &b) const
{
  bool ret = true;
  if (_value) {
    b << _value;
  } else {
    ret = false;
  }
  return ret;
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
  int len;
  const char *cp = to_const_char (&len);
  b.cat (cp, len, true);
}

str
xml_double_t::to_str () const
{
  int len;
  const char *cp = to_const_char (&len);
  return str (cp, len);
}

const char *
xml_double_t::to_const_char (int *rc) const
{
#define DOUBLEBUFLEN 0x40
  static char buf[DOUBLEBUFLEN];
  const char *ret = buf;
  *rc = snprintf (buf, DOUBLEBUFLEN, "%g", _val);
  if (*rc >= DOUBLEBUFLEN) {
    ret = "NaN";
    *rc = strlen (ret);
  }
  return ret;
#undef DOUBLEBUFLEN
}


bool
xml_name_t::add (const char *buf, int len)
{
  static rxx name_rxx ("[a-zA-Z0-9\\._-]+");
  str s (buf, len);
  if (!name_rxx.match (s))
    return false;
  _value = s;
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
xml_container_t::get_r (size_t i, bool mk)
{
  if (i >= size ())
    setsize (i+1);
  ptr<xml_element_t> &r = (*this)[i];
  if (mk && !r) r = xml_value_t::alloc ();
  return r;
}

ptr<xml_element_t> &
xml_params_t::get_r (size_t i, bool mk)
{
  ptr<xml_element_t> &r = xml_container_t::get_r (i, false);
  if (mk && !r) r = xml_param_t::alloc ();
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
  xml_obj_t w (f);

  w("faultCode") = rc;
  w("faultString") = s;

  return f;
}

ptr<xml_method_call_t>
xml_method_call_t::clone_typed () const
{
  return New refcounted<xml_method_call_t> 
    ( _method_name ? _method_name->clone_typed (): _method_name,
      _params ? _params->clone_typed () : _params);
}

ptr<xml_value_t> 
xml_value_wrapper_t::cpvalue () const
{ return _value ? _value->clone ()->to_xml_value () : _value; }

ptr<xml_method_response_t>
xml_method_response_t::clone_typed () const
{
  return New refcounted<xml_method_response_t>
    ( _params ? _params->clone_typed () : _params,
     _body ? _body->clone () : _body );
}

ptr<xml_member_t>
xml_member_t::clone_typed () const
{
  return New refcounted<xml_member_t> 
    ( _member_name ? _member_name->clone_typed () : _member_name,
      _member_value ? _member_value->clone () : _member_value) ;
}

xml_struct_t::xml_struct_t (const xml_struct_t &s)
  : xml_container_t (s)
{
  ptr<const xml_element_t> e;
  ptr<const xml_member_t> m;
  str nm;
  for (size_t i = 0; i < s.size (); i++) {
    if ((e = s.xml_container_t::get (i)) && (m = e->to_xml_member ()) &&
	(nm = m->member_name_str ())) {
      _members.insert (nm, i);
    }
  }
}

ptr<xml_array_t> 
xml_array_t::clone_typed () const 
{ return New refcounted<xml_array_t> (_data ? _data->clone_typed () : _data); }

bool
xml_value_wrapper_t::assign_to (ptr<xml_element_t> to)
{
  return to->set_pointer_to_me (&_value);
}

bool
xml_array_t::assign_to (ptr<xml_element_t> to)
{
  return to->set_pointer_to_me (&_data);
}

bool
xml_data_t::set_pointer_to_me (ptr<xml_data_t> *d)
{
  *d = mkref (this);
  return true;
}

bool
xml_array_t::set_pointer_to_me (ptr<xml_data_t> *d)
{
  *d = _data;
  return true;
}

bool
xml_value_t::assign_to (ptr<xml_element_t> to)
{
  ptr<xml_value_t> v;
  if (to->set_pointer_to_me (&v)) {
    set_element (v->element ());
    return true;
  }
  return false;
}

bool
xml_value_wrapper_t::set_pointer_to_me (ptr<xml_value_t> *v)
{
  *v = _value;
  return true;
}

bool
xml_value_t::set_pointer_to_me (ptr<xml_value_t> *v)
{
  *v = mkref (this);
  return true;
}

bool
xml_scalar_t::set_pointer_to_me (ptr<xml_value_t> *v)
{
  *v = xml_value_t::alloc (mkref (this));
  return true;
}


bool
xml_struct_t::set_pointer_to_me (ptr<xml_value_t> *v)
{
  *v = xml_value_t::alloc (mkref (this));
  return true;
}

bool
xml_array_t::set_pointer_to_me (ptr<xml_value_t> *v)
{
  *v = xml_value_t::alloc (mkref (this));
  return true;
}

str
xml_str_t::escape (const str &in)
{
  if (!in) return in;

  strbuf b;
  const char *cp, *p1;
  for (cp = p1 = in.cstr (); *cp; cp++) {
    if (*cp == '&' || *cp == '<') {
      if (cp > p1) b.tosuio ()->copy (p1, cp - p1);
      b << (*cp == '&' ? "&amp;" : "&lt;");
      p1 = cp + 1;
    }
  }

  if (p1 == in.cstr ())
    return in;
  else {
    if (cp > p1) b.tosuio ()->copy (p1, cp - p1);
    return b;
  }
}
