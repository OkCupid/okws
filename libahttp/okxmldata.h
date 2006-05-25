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

class xml_struct_t;
class xml_array_t;
class xml_int_t;
class xml_bool_t;
class xml_double_t;
class xml_str_t;
class xml_base64_t;
class xml_null_t;
class xml_method_call_t;

class xml_element_t : public virtual refcnt {
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
  virtual ptr<xml_params_t> get_params () { return NULL; }

  virtual ptr<xml_element_t> get (const str &s) const;
  virtual ptr<xml_element_t> get (int i) const;
  virtual ptr<xml_element_t> &get_r (int i);
  virtual ptr<xml_element_t> &get_r (const str &s);
  virtual bool put (const str &s, ptr<xml_element_t> el) { return false; }
  virtual bool put (int i, ptr<xml_element_t> el) { return false; }

  virtual size_t size () const { return 0; }
  virtual int to_int () const { return 0; }
  virtual str to_str () const { return ""; }
  virtual str to_bool () const { return false; }
  virtual str to_base64 () const { return armor64 (NULL, 0); }

  const ptr<xml_element_t> &operator[] (const str &s) const { return get (s); }
  const ptr<xml_element_t> &operator[] (int i) const { return get (i); }
  ptr<xml_element_t> &operator[] (const str &s) { return get_m (s); }
  ptr<xml_element_t> &operator[] (int i) { return get_m (i); }

  operator int () const { return to_int (); }
  operator str () const { return to_str (); }
  operator bool () const { return to_bool (); }
};

class xml_elwrap_t {
public:
  xml_elwrap_t (ptr<xml_element_t> e) : _el (e) {}

  operator int () const { return _el->to_int (); }
  operator str () const { return _el->to_str (); }
  operator bool () const { return _el->to_bool (); }

  const xml_elwrap_t &operator[] (const str &s) const 
  { return xml_elwrap_t (_el->get (s)); }
  const xml_elwrap_t &operator[] (int i) const 
  { return xml_elwrap_t (_el->get (i)); }

  size_t size () const { return _el->size (); }

private:
  ptr<xml_element_t> _el;
};

class xml_method_call_t : public xml_element_t {
public:
  xml_method_call_t (const str &n) : _method_name (n) {}
  xml_mehtod_call_t () {}

  str get_method_name () const { return _method_name; }
  void set_method_name (const str &s) { _method_name = s; }
  ptr<xml_params_t> get_params () const { return _params; }
  void set_params (ptr<xml_params_t> p) { _params = p ;}
  
private:
  str _method_name;
  ptr<xml_params_t> _params;
};

class xml_param_t {
public:
  xml_param_t () {}
  void set_value (ptr<xml_element_t> x) { _value = x; }
  ptr<xml_element_t> get_value () { return x; }
private:
  ptr<xml_element_t> _value;
};

class xml_params_t : public vec<ptr<xml_element_t> > {
public:
  xml_params_t () : vec<ptr<xml_param_t> > () {}
};

class xml_null_t : public xml_element_t {
public:
  xml_null_t () {}
  static ptr<xml_null_t> alloc () 
  { return New refcounted<xml_null_t> (); }
  ptr<xml_null_t> to_xml_null () { return mkref (this); }
};

class xml_int_t : public xml_element_t {
public:
  xml_int_t (int i = 0) : _val (i) {}
  ptr<xml_int_t> to_xml_int () { return mkref (this); }
  int to_int () const { return _val; }
  void set (int i) { _val = i; }
private:
  int _val;
};

class xml_str_t : public xml_element_t {
public:
  xml_str_t (const str &s) : _val (s) {}
  xml_str_t () {}
  ptr<xml_str_t> to_xml_str () { return mkref (this); }
  str to_str () const { return _val; }
  void set (const str &v) { _val = v; }
private:
  str _val;
};

class xml_base64_t : public xml_element_t {
public:
  xml_base64_t (const str &b) : _val (b) {}
  xml_base64_t () : _val (armor64 (NULL, 0)) {}
  ptr<xml_base64_t> to_xml_base64 () { return mkref (this); }
  str to_base64 () const { return _val; }
  str decode () const { return dearmor64 (_val.cstr (), _val.len ()); }
  void encode (const str &s) { _val = armor64 (s.cstr (), s.len ()); }
  void set (const str &v) { _val = v; }
private:
  str _val;
};

class xml_struct_t : public xml_element_t {
public:
  ptr<xml_element_t> get (int i) const;
  ptr<xml_element_t> &get_r (int i) ;
  bool put (const str &s, ptr<xml_element_t> el);
  ptr<xml_struct_t> to_xml_struct () { return mkref (this); }
private:
  qhash<str, ptr<xml_element_t> > _members;
};

class xml_array_t : public xml_element_t {
public:
  ptr<xml_element_t> get (int i) const;
  ptr<xml_element_t> &get_r (int i);
  bool put (int i, ptr<xml_element_t> el);
  size_t size () const { return _elements.size (); }
  ptr<xml_array_t> to_xml_array () { return mkref (this); }
private:
  vec<ptr<xml_element_t> > _elements;
};

#endif /* _LIBAHTTP_OKXML_DATA_H */
