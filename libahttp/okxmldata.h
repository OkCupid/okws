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

#ifndef _LIBAHTTP_OKXML_DATA_H
#define _LIBAHTTP_OKXML_DATA_H

#include "async.h"
#include "qhash.h"
#include "zstr.h"

class xml_struct_t;
class xml_array_t;
class xml_int_t;
class xml_bool_t;
class xml_double_t;
class xml_str_t;
class xml_base64_t;
class xml_null_t;
class xml_method_call_t;
class xml_params_t;
class xml_param_t;
class xml_value_t;
class xml_name_t;
class xml_member_t;
class xml_data_t;
class xml_method_name_t;

class xml_element_t : public virtual refcount {
public:
  xml_element_t () {}
  virtual ~xml_element_t () {} 

  virtual ptr<xml_struct_t> to_xml_struct () { return NULL; }
  virtual ptr<xml_array_t> to_xml_array () { return NULL; }
  virtual ptr<xml_int_t> to_xml_int () { return NULL; }
  virtual ptr<xml_bool_t> to_xml_bool () { return NULL; }
  virtual ptr<xml_double_t> to_xml_double () { return NULL; }
  virtual ptr<xml_str_t> to_xml_str () { return NULL; }
  virtual ptr<xml_base64_t> to_xml_base64 () { return NULL; }
  virtual ptr<xml_null_t> to_xml_null () { return NULL; }
  virtual ptr<xml_method_call_t> to_xml_method_call () { return NULL; }
  virtual ptr<xml_params_t> to_xml_params () { return NULL; }
  virtual ptr<xml_param_t> to_xml_param () { return NULL; }
  virtual ptr<xml_value_t> to_xml_value () { return NULL; }
  virtual ptr<xml_name_t> to_xml_name () { return NULL; }
  virtual ptr<xml_member_t> to_xml_member () { return NULL; }
  virtual ptr<xml_data_t> to_xml_data () { return NULL; }
  virtual ptr<xml_method_name_t> to_xml_method_name () { return NULL; }

  virtual ptr<xml_element_t> get (const str &s) const;
  virtual ptr<xml_element_t> get (size_t i) const;
  virtual ptr<xml_element_t> &get_r (size_t i);
  virtual ptr<xml_element_t> &get_r (const str &s);
  virtual bool put (const str &s, ptr<xml_element_t> el) { return false; }
  virtual bool put (size_t i, ptr<xml_element_t> el) { return false; }

  virtual size_t len () const { return 0; }
  virtual int to_int () const { return 0; }
  virtual str to_str () const { return ""; }
  virtual str to_bool () const { return false; }
  virtual str to_base64 () const { return armor64 (NULL, 0); }
  virtual bool is_value () const { return false; }

  // should it be a strbuf or a zbuf?
  virtual void dump (zbuf &b, int lev);
  virtual void dump_data (zbuf &b, int lev) {}
  void dump (zbuf &b) { dump (b, 0); }

  operator int () const { return to_int (); }
  operator str () const { return to_str (); }
  operator bool () const { return to_bool (); }

  virtual ptr<xml_element_t> clone (const char *) const
  { return New refcounted<xml_element_t> (); }
  virtual const char *name () const { return NULL; }
  
  // used during parsing
  virtual bool is_a (const char *s) const { return !strcmp (name (), s); }
  virtual bool add (const char *buf, int len) { return false; }
  virtual bool gets_char_data () const { return false; }
  virtual bool add (ptr<xml_element_t> e) { return false; }
  virtual bool close_tag () { return true; }
};

class xml_container_t : public xml_element_t,
			public vec<ptr<xml_element_t> >
{
public:
  xml_container_t () : vec<ptr<xml_element_t> > () {}
  virtual ~xml_container_t () {}

  virtual bool add (ptr<xml_element_t> e);
  virtual bool can_contain (ptr<xml_element_t> e) { return false; }
  size_t len () const { return size (); }
  void dump_data (zbuf &b, int len);
};

class xml_top_level_t : public xml_container_t {
public:
  const char *name () const { return "topLevel"; }
  bool can_contain (ptr<xml_element_t> e) { return e->to_xml_method_call (); }
};


class xml_elwrap_t {
public:
  xml_elwrap_t (ptr<xml_element_t> e) : _el (e) {}

  operator int () const { return _el->to_int (); }
  operator str () const { return _el->to_str (); }
  operator bool () const { return _el->to_bool (); }

  xml_elwrap_t operator[] (const str &s) const 
  { return xml_elwrap_t (_el->get (s)); }
  xml_elwrap_t operator[] (size_t i) const 
  { return xml_elwrap_t (_el->get (i)); }

  size_t size () const { return _el->len (); }

private:
  ptr<xml_element_t> _el;
};

class xml_method_name_t : public xml_element_t {
public:
  xml_method_name_t () {}
  xml_method_name_t (const str &s) : _value (s) {}
  ptr<xml_element_t> clone (const char *) const 
  { return New refcounted<xml_method_name_t> (); }
  const char *name () const { return "methodName"; }
  str value () const { return _value ; }
  void set_value (const str &s) { _value = s; }
  ptr<xml_method_name_t> to_xml_method_name () { return mkref (this); }
  bool add (const char *buf, int len);
  bool gets_char_data () const { return true; }
  static ptr<xml_method_name_t> alloc (const str &s)
  { return New refcounted<xml_method_name_t> (s); }
  void dump_data (zbuf &b, int l) { if (_value) b << _value; }
  
private:
  str _value;
};

class xml_method_call_t : public xml_element_t {
public:
  xml_method_call_t (const str &n) 
    : _method_name (xml_method_name_t::alloc (n)) {}
  xml_method_call_t () {}

  str method_name () const 
  { return _method_name ? _method_name->value () : NULL; }

  void set_method_name (const str &s) 
  { _method_name = xml_method_name_t::alloc (s); }

  ptr<xml_params_t> params () const { return _params; }
  void set_params (ptr<xml_params_t> p) { _params = p ;}
  
  ptr<xml_element_t> clone (const char *) const 
  { return New refcounted<xml_method_call_t> (); }
  const char *name () const { return "methodCall"; }

  ptr<xml_method_call_t> to_xml_method_call () { return mkref (this); }

  bool add (ptr<xml_element_t> e);
  void dump_data (zbuf &b, int lev);
private:
  ptr<xml_method_name_t> _method_name;
  ptr<xml_params_t> _params;
};

class xml_param_t : public xml_element_t {
public:
  xml_param_t () {}
  void set_value (ptr<xml_value_t> x) { _value = x; }
  ptr<xml_value_t> value () { return _value; }
  ptr<xml_param_t> to_xml_param () { return mkref (this); }

  ptr<xml_element_t> clone (const char *) const 
  { return New refcounted<xml_param_t> (); }
  const char *name () const { return "param"; }
  bool add (ptr<xml_element_t> e);
  void dump_data (zbuf &b, int lev);
private:
  ptr<xml_value_t> _value;
};

class xml_params_t : public xml_container_t {
public:
  xml_params_t () {}

  ptr<xml_element_t> clone (const char *) const 
  { return New refcounted<xml_params_t> (); }
  const char *name () const { return "params"; }
  bool can_contain (ptr<xml_element_t> e) { return e->to_xml_param (); }
  ptr<xml_params_t> to_xml_params () { return mkref (this); }
};

class xml_null_t : public xml_element_t {
public:
  xml_null_t () {}

  ptr<xml_null_t> to_xml_null () { return mkref (this); }

  static ptr<xml_null_t> alloc () { return New refcounted<xml_null_t> (); }
  ptr<xml_element_t> clone (const char *) const 
  { return New refcounted<xml_params_t> (); }
  const char *name () const { return "null"; }
};

class xml_scalar_t : public xml_element_t {
public:
  xml_scalar_t () {}
  bool add (const char *b, int s);
  bool gets_char_data () const { return true; }
  bool is_value () const { return true; }
protected:
  strbuf _buf;
};

class xml_double_t : public xml_scalar_t {
public:
  xml_double_t () : _val (0) {}
  ptr<xml_double_t> to_xml_double () { return mkref (this); }
  double to_double () const { return _val; }
  void set (double d) { _val = d; }

  ptr<xml_element_t> clone (const char *n) const 
  { return New refcounted<xml_double_t> (); }

  const char *name () const { return "double"; }
  bool close_tag ();
  void dump_data (zbuf &b, int lev);
private:
  double _val;
};

class xml_int_t : public xml_scalar_t {
public:
  xml_int_t (const char *tag) : _tag (tag), _val (0) {}
  xml_int_t (int i = 0) : _val (i) {}
  ptr<xml_int_t> to_xml_int () { return mkref (this); }
  int to_int () const { return _val; }
  void set (int i) { _val = i; }
  bool is_value () const { return true; }

  ptr<xml_element_t> clone (const char *n) const 
  { return New refcounted<xml_int_t> (n); }

  const char *name () const { return "int"; }
  bool is_a (const char *n) const { return !strcmp (_tag.cstr (), n); }
  bool close_tag ();
  void dump_data (zbuf &b, int lev) { b << _val; }
private:
  const str _tag;
  int _val;
};

class xml_str_t : public xml_scalar_t {
public:
  xml_str_t (const str &s) : _val (s) {}
  xml_str_t () {}
  ptr<xml_str_t> to_xml_str () { return mkref (this); }
  str to_str () const { return _val; }
  void set (const str &v) { _val = v; }
  const char *name () const { return "string"; }
  bool close_tag ();
  ptr<xml_element_t> clone (const char *n) const 
  { return New refcounted<xml_str_t> (); }
  void dump_data (zbuf &z, int level) { if (_val) z << _val; }
private:
  str _val;
};

class xml_base64_t : public xml_scalar_t {
public:
  xml_base64_t (const str &b) : _val (b) {}
  xml_base64_t () : _val (armor64 (NULL, 0)) {}
  ptr<xml_base64_t> to_xml_base64 () { return mkref (this); }
  str to_base64 () const { return _val; }
  str decode () const { return dearmor64 (_val.cstr (), _val.len ()); }
  void encode (const str &s) { _val = armor64 (s.cstr (), s.len ()); }
  void set (const str &v) { _val = v; }
  
  ptr<xml_element_t> clone (const char *) const 
  { return New refcounted<xml_base64_t>(); }
  const char *name () const { return "base64"; }
  bool close_tag ();
private:
  str _val;
};

class xml_struct_t : public xml_container_t {
public:
  xml_struct_t () {}
  ptr<xml_element_t> get (const str &s) const;
  ptr<xml_element_t> &get_r (const str &s) ;
  bool put (const str &s, ptr<xml_element_t> el);
  ptr<xml_struct_t> to_xml_struct () { return mkref (this); }

  ptr<xml_element_t> clone (const char *) const 
  { return New refcounted<xml_struct_t>(); }
  const char *name () const { return "struct"; }
  bool is_value () const { return true; }
  bool can_contain (ptr<xml_element_t> e) const { return e->to_xml_member (); }
  bool close_tag ();
private:
  qhash<str, ptr<xml_element_t> > _members;
};

class xml_array_t : public xml_element_t {
public:
  xml_array_t () {}
  ptr<xml_element_t> get (size_t i) const;
  ptr<xml_element_t> &get_r (size_t i);
  bool put (size_t i, ptr<xml_element_t> el);
  ptr<xml_array_t> to_xml_array () { return mkref (this); }

  ptr<xml_element_t> clone (const char *) const 
  { return New refcounted<xml_array_t>(); }
  const char *name () const { return "array"; }
  bool is_value () const { return true; }
  ptr<xml_data_t> data () { return _data; }
  bool add (ptr<xml_element_t> e);
private:
  ptr<xml_data_t> _data;
};

class xml_value_t : public xml_element_t {
public:
  xml_value_t () {}
  ptr<xml_element_t> clone (const char *) const 
  { return New refcounted<xml_value_t> (); }
  const char *name () const { return "value"; }
  ptr<xml_element_t> element () { return _e; }
  void set_element (ptr<xml_element_t> e) { _e = e; }
  bool add (ptr<xml_element_t> e);
  ptr<xml_value_t> to_xml_value () { return mkref (this); }

  size_t len () const { return _e ? _e->len () : xml_element_t::len (); }
  int to_int () const { return _e ? _e->to_int () : xml_element_t::to_int (); }
  str to_str () const { return _e ? _e->to_str () : xml_element_t::to_str (); }
  str to_base64 () const 
  { return _e ? _e->to_base64 () : xml_element_t::to_base64(); }
  void dump_data (zbuf &b, int level) { if (_e) _e->dump (b, level); }
  
private:
  ptr<xml_element_t> _e;
};

class xml_member_t : public xml_element_t {
public:
  xml_member_t () {}
  ptr<xml_element_t> clone (const char *) const 
  { return New refcounted<xml_member_t> (); }
  const char *name () const { return "member"; }
  ptr<xml_member_t> to_xml_member () { return mkref (this); }

  ptr<xml_name_t> member_name () const { return _member_name ; }
  ptr<xml_value_t> member_value () const { return _member_value; }
  bool add (ptr<xml_element_t> e);

private:
  ptr<xml_name_t> _member_name;
  ptr<xml_value_t> _member_value;
};

class xml_name_t : public xml_element_t {
public:
  xml_name_t () {} 
  ptr<xml_element_t> clone (const char *) const 
  { return New refcounted<xml_name_t> (); }
  const char *name () const { return "name"; }
  ptr<xml_name_t> to_xml_name () { return mkref (this); }
  str value () const { return _value ;}
  void set_value (const str &v) { _value = v;}
private:
  str _value;
};

class xml_data_t : public xml_container_t {
public:
  xml_data_t () {}
  ptr<xml_element_t> clone (const char *) const 
  { return New refcounted<xml_data_t> (); }
  const char *name () const { return "data"; }
  ptr<xml_data_t> to_xml_data () { return mkref (this); }
  bool can_contain (ptr<xml_element_t> e) { return e->to_xml_value (); }
  static ptr<xml_data_t> alloc () { return New refcounted<xml_data_t> (); }
};

class xml_bool_t : public xml_scalar_t {
public:
  xml_bool_t () {}
  ptr<xml_element_t> clone (const char *) const 
  { return New refcounted<xml_bool_t> (); }
  const char *name () const { return "boolean"; }
  ptr<xml_bool_t> to_xml_bool () { return mkref (this); }
  bool close_tag ();
private:
  bool _val;
};

bool has_non_ws (const char *buf, int len);



#endif /* _LIBAHTTP_OKXML_DATA_H */
