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
#include <ctype.h>

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
    _params = p;
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

int
xml_value_wrapper_t::to_int () const 
{ return _value ? _value->to_int () : xml_element_t::to_int (); }

bool
xml_value_wrapper_t::to_bool () const
{ return _value ? _value->to_bool () : xml_element_t::to_bool (); }

str
xml_value_wrapper_t::to_str () const
{ return _value ? _value->to_str () : xml_element_t::to_str (); }

str 
xml_value_wrapper_t::to_base64 () const
{ return _value ? _value->to_base64 () : xml_element_t::to_base64 (); }

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

static void
python_str_print (strbuf &b, const str &in)
{
  const u_int8_t *cp = reinterpret_cast<const u_int8_t *> (in.cstr ());
  const u_int8_t *ep = cp + in.len ();
  for ( ; cp < ep; cp++) {
    if (*cp == '\\') {
      b << "\\\\";
    } else if (*cp == '\n') {
      b << "\\n";
    } else if (*cp == '\'') {
      b << "\\\'";
    } else if (isprint (*cp)) {
      b.fmt ("%c", *cp);
    } else {
      b.fmt ("\\x%02x", *cp);
    }
  }
}

bool
xml_base64_t::dump_to_python (strbuf &b) const
{
  if (_val) {
    b << "Binary('";
    python_str_print (b, decode ());
    b << "')";
    return true;
  }
  return false;
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
  _buf.tosuio ()->copy (b, len);
  return true;
}

static str 
strip_space (const str &in)
{
  str out;
  if (in) {
    mstr m (in.len ());
    const char *inp = in.cstr ();
    char *outp = m.cstr ();
    const char *endp = inp + in.len ();

    for ( ; inp < endp; inp++) {
      if (!isspace (*inp)) { *outp++ = *inp; }
    }
    m.setlen (outp - m.cstr ());
    out = m;
  }
  return out;
}

bool
xml_base64_t::close_tag ()
{
  bool ret = false;
  str tmp = _buf;
  static rxx strip_rxx ("\\s*(\\S+)\\s*");
  str stmp = strip_space (tmp);
  if (stmp && stmp.len ()) {
    _val = stmp;
    assert (_val);
    if (decode ()) {
      ret = true;
    } else {
      warn << "Failed to base-64 decode string: " << _val << "\n";
    }
  } else {
    _d_val = "";
    ret = true;
  }
  return ret;
}

str 
xml_base64_t::decode () const 
{ 
  if (!_d_val && _val)
    _d_val = dearmor64 (_val.cstr (), _val.len ());
  return _d_val;
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
  if (xml_typename ()) {
    spaces (b, lev);
    b << "<" << dump_typename () << ">";
    if (!has_data ()) b << "\n";
  }
  dump_data (b, lev + 1);
  if (xml_typename ()) {
    if (!has_data ())
      spaces (b, lev);
    b << "</" << xml_typename () << ">\n";
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
  b << py_open_container ();
  for (size_t i = 0; i < size (); i++) {
    ptr<xml_element_t> el = (*this)[i];
    if (i > 0) b << ", ";
    if (el) { el->dump_to_python (b); }
    else { b << "None"; }
  }
  b << py_close_container ();
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
  // Python program should have a variable 'srv'
  b << "srv.";
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

bool
xml_member_t::dump_to_python (strbuf &b) const
{
  if (_member_name)  _member_name->dump_to_python (b);
  b << " : ";
  if (!_member_value || !_member_value->dump_to_python (b))
    b << "None"; 
  return true;
}

void
xml_value_wrapper_t::dump_data (zbuf &b, int lev) const
{
  if (_value) { _value->dump (b, lev); }
}

bool
xml_value_wrapper_t::dump_to_python (strbuf &b) const
{
  bool ret = true;
  if (_value) { _value->dump_to_python (b); }
  else ret = false;
  return ret;
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

bool
xml_double_t::dump_to_python (strbuf &b) const
{
  int len;
  const char *cp = to_const_char (&len);
  b.tosuio ()->copy (cp, len);
  return true;
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
xml_name_t::add (const char *dat, int len)
{
  _buf.tosuio ()->copy (dat, len);
  return true;
}

bool
xml_name_t::close_tag ()
{
  str v = _buf;
  static rxx name_rxx ("[a-zA-Z0-9\\._-]+");
  if (!name_rxx.match (v))
    return false;
  _value = v;
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

bool
xml_method_response_t::dump_to_python (strbuf &b) const
{
  if (_body) {
    _body->dump_to_python (b);
    return true;
  } else {
    return false;
  }
}

bool
xml_name_t::dump_to_python (strbuf &b) const
{
  bool ret = true;
  if (_value) b << "'" << _value << "'" ;
  else ret = false;
  return ret;
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

bool
xml_array_t::dump_to_python (strbuf &b) const
{
  return (_data ? _data->dump_to_python (b) : false);
}

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
  ptr<xml_params_t> p;
  ptr<xml_element_t> b;

  if (_params) {
    p = _params->clone_typed ();
    b = p;
  } else if (_body) {
    b = _body->clone ();
  }
		 
  return New refcounted<xml_method_response_t> (p, b);
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

bool
xml_value_t::dump_to_python (strbuf &b) const
{
  if (_e) {
    _e->dump_to_python (b);
    return true;
  } else {
    b << "None";
    return false;
  }
}

bool
xml_str_t::dump_to_python (strbuf &b) const
{
  if (_val) {
    b << "'" << _val << "'";
    return true;
  } else {
    b << "''";
    return false;
  }
}

bool
xml_bool_t::dump_to_python (strbuf &b) const
{
  b << (_val ? "True" : "False");
  return true;
}

bool
xml_top_level_t::dump_to_python (strbuf &b) const
{
  if (size () >= 1 && (*this)[0]) {
    (*this)[0]->dump_to_python (b);
    return true;
  }
  return false;
}

ptr<xml_container_t>
xml_method_call_t::to_xml_container ()
{
  if (!_params) _params = New refcounted<xml_params_t> ();
  return _params;
}

bool 
xml_top_level_t::can_contain (ptr<xml_element_t> e) const 
{ return e->to_xml_method_call () || e->to_xml_method_response (); }

const char *
xml_value_t::xml_typename_coerce () const
{ return _e ? _e->xml_typename_coerce () : "unbound value"; }


//=======================================================================
//-----------------------------------------------------------------------
// Generic data types (not for XML-RPC)

xml_attributes_t::xml_attributes_t (const char **atts)
{
  for (const char **p = atts; p && *p; p += 2) {
    _t.insert (str (p[0]), scalar_obj_t (p[1]));
  }
}

xml_attributes_t::xml_attributes_t (const vec<str>& atts)
{
  for (size_t i = 0; i < atts.size(); i += 2) {
    _t.insert (atts[i], scalar_obj_t (atts[i + 1]));
  }
}

scalar_obj_t _null_so;

scalar_obj_t
xml_attributes_t::lookup (const str &k) const
{
  const scalar_obj_t *o = _t[k];

  if (o) { return *o; }
  else { return _null_so; }
}

bool
xml_attributes_t::lookup (const str &k, scalar_obj_t *op) const
{
  const scalar_obj_t *o = _t[k];
  if (o) { *op = *o; }
  return (o != NULL);
}

bool
xml_generic_t::add (ptr<xml_element_t> e)
{
  bool ret = true;
  ptr<xml_generic_t> g;
  if ((g = e->to_xml_generic ())) {
    str n = g->tagname ();
    ptr<vec<ptr<xml_generic_t> > > *vp = _tab[n];
    ptr<vec<ptr<xml_generic_t> > > v;
    if (vp) {
      v = *vp;
    } else {
      v = New refcounted<vec<ptr<xml_generic_t> > > ();
      _tab.insert (n, v);
    }
    v->push_back (g);
  } else {
    ret = false;
  }
  return ret;
}

ptr<xml_generic_t>
xml_generic_t::alloc_null()
{
  return New refcounted<xml_generic_t> ();
}

ptr<const xml_generic_t> xml_generic_t::_null_generic;

ptr<const xml_generic_t>
xml_generic_t::const_alloc_null()
{
  if (!_null_generic) {
    _null_generic = New refcounted<xml_generic_t> ();
  }
  return _null_generic;
}

const char *
xml_generic_t::xml_typename () const 
{
  if (_class) return _class.cstr ();
  else return NULL;
}

bool
xml_generic_t::is_a (const char *t) const
{
  const char *me = xml_typename ();
  return me && t && strcmp (me, t) == 0;
}

bool
xml_generic_t::close_tag () 
{
  if (_so)
    _so->freeze ();
  return true;
}

bool
xml_generic_t::add (const char *s, int l)
{
  bool nonws = has_non_ws (s, l);

  if (!_so && nonws)
    _so = New refcounted<xml_scalar_obj_w_t> ();
  if (_so && (nonws || !_so->is_frozen ()))
    _so->add (s, l);
  return true;
}

bool
xml_scalar_obj_w_t::strip_add (const char *s, int l) const
{
  return has_non_ws (s, l);
}

void
xml_scalar_obj_w_t::dump (zbuf &b) const
{
  if (!is_null ()) {
    b << to_str ();
  }
}

void
xml_cdata_t::dump (zbuf &b) const
{
  str s = to_str ();
  b << "<![CDATA[";
  if (s)
    b << s;
  b << "]]>";
}

void 
xml_generic_t::dump_data (zbuf &b, int lev) const
{
  xml_generic_const_item_iterator_t i (mkref (this));
  ptr<const xml_generic_t> e;

  if (_so) _so->dump (b);
  
  while ((e = i.next ())) { e->dump (b, lev + 1); }
}

ptr<xml_generic_t>
xml_generic_item_iterator_t::next ()
{
  ptr<xml_generic_t> r;
  if (!_eof && (!_v || _i >= _v->size ())) {
    _v = NULL;
    _i = 0;
    if (!_it.next (&_v)) _eof = true;
    else assert (_v->size () > 0);
  }
  if (_v) r = (*_v)[_i++];
  return r;
}

ptr<const xml_generic_t>
xml_generic_const_item_iterator_t::next ()
{
  ptr<const xml_generic_t> r;
  if (!_eof && (!_v || _i >= _v->size ())) {
    _v = NULL;
    _i = 0;
    if (!_it.next (&_v)) _eof = true;
    else assert (_v->size () > 0);
  }
  if (_v) r = (*_v)[_i++];
  return r;
}

str
xml_attributes_t::to_str () const
{
  strbuf b;
  qhash_const_iterator_t<str, scalar_obj_t> it (_t);
  const str *k;
  scalar_obj_t v;
  bool first = true;
  vec<str> s;
  while ((k = it.next (&v))) {
    if (!first) b << " ";
    else first = false;
    s.push_back (v.to_str ());
    b << *k << "=\"" << s.back () << "\"";
  }
  return b;
}

str
xml_generic_t::dump_typename () const
{
  strbuf b;
  b << xml_typename ();
  str a = attributes ().to_str ();
  if (a && a.len () > 0) {
    b << " " << a;
  }
  return b;
}

bool xml_generic_t::has_data () const { return _so && !_so->is_null (); } 

bool
xml_generic_t::start_cdata ()
{
  _so = New refcounted<xml_cdata_t> ();
  return true;
}

bool
xml_generic_t::end_cdata ()
{
  _so->freeze ();
  return true;
}

scalar_obj_t
xml_generic_t::data () const
{
  if (_so) { return *_so; }
  else { return scalar_obj_t (); }

}

ptr<xml_scalar_obj_w_t>
xml_generic_t::make_so ()
{
  if (!_so) 
    _so = New refcounted<xml_scalar_obj_w_t> ();
  return _so;
}

//-----------------------------------------------------------------------

void
xml_array_t::to_pub3 (pub3::obj_t *out) const
{
  for (size_t i = 0; _data && i < _data->size (); i++) {
    ptr<const xml_element_t> el = (*_data)[i];
    pub3::obj_t o;
    if (el) { el->to_pub3 (&o); }
    out->push_back  (o);
  }
}

//-----------------------------------------------------------------------

void
xml_struct_t::to_pub3 (pub3::obj_t *out) const
{
  qhash_const_iterator_t<str, size_t> it (_members);
  const str *k;
  size_t v;
  while ((k = it.next (&v))) {
    ptr<const xml_element_t> el = (*this)[v];
    if (el) { el->to_pub3 (out); }
  }
}

//-----------------------------------------------------------------------

void
xml_member_t::to_pub3 (pub3::obj_t *out) const
{
  str n;
  pub3::obj_t val;
  if (_member_name && (n = _member_name->value ()) && _member_value) {
    _member_value->to_pub3 (&val);
    (*out)(n) = val;
  }
}

//-----------------------------------------------------------------------

void
xml_element_t::to_pub3 (pub3::obj_t *o) const
{
  warn << "failed to convert to pub3: " << xml_typename () << "\n";
}

//-----------------------------------------------------------------------

void
xml_value_t::to_pub3 (pub3::obj_t *o) const
{
  if (_e) { _e->to_pub3 (o); }
}


//=======================================================================
