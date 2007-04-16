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

#ifndef _LIBAHTTP_OKXMLOBJ_H_
#define _LIBAHTTP_OKXMLOBJ_H_

#include "okxmldata.h"

//
// What's this?
//
//   Wrappers around the XML data elements declared in okxmldata.h; 
//   they can be manipulated as regular objects, and the refcounting is
//   done underneath the covers.
//

class xml_obj_const_t;
class xml_obj_t;

class xml_obj_base_t {
public:
  xml_obj_base_t () {}
  virtual ~xml_obj_base_t () {}

  operator int () const { return el ()->to_int (); }
  operator str () const { return el ()->to_str (); }
  operator bool () const { return el ()->to_bool (); }

  bool is_int () const { return el()->is_type (XML_INT); }
  bool is_str () const { return el()->is_type (XML_STR); }
  bool is_bool () const { return el()->is_type (XML_BOOL); }
  bool is_struct () const { return el()->is_type (XML_STRUCT); }
  bool is_array () const { return el()->is_type (XML_ARRAY); }
  bool is_base64() const { return el()->is_type (XML_BASE64); }

  xml_obj_const_t operator[] (size_t i) const ;
  xml_obj_const_t operator() (const str &s) const ;

  ptr<const xml_container_t> to_xml_container () const
  { return el () ? el ()->to_xml_container () : NULL; }
  ptr<const xml_struct_t> to_xml_struct () const 
  { return el () ? el ()->to_xml_struct () : NULL; }
  
  size_t size () const 
  {
    ptr<const xml_container_t> c = to_xml_container ();
    return c ? c->size () : 0;
  }

  virtual ptr<const xml_element_t> el () const = 0;
  virtual void output (zbuf &z) const { el ()->dump (z); }
  void output_python (strbuf &b) const { el ()->dump_to_python (b); }

  str name () const;
  xml_obj_const_t value () const;
  
  xml_obj_t clone () const;
};

class xml_obj_const_t : public xml_obj_base_t {
public:
  xml_obj_const_t (ptr<const xml_element_t> e) : _el (e) {}
  xml_obj_const_t () {}
  xml_obj_const_t (const xml_obj_base_t &w) : _el (w.el ()) {}

  ptr<const xml_element_t> el () const { return _el; }
private:
  ptr<const xml_element_t> _el;
};

class base64_str_t
{
public:
  base64_str_t (const char *c, size_t len) : _s (c, len), _encoded (false) {}

  base64_str_t (const str &s, bool e = false) : _s (s), _encoded (e) {}
  operator str() const { return _s; }
  bool encoded () const { return _encoded; }
private:
  const str _s;
  bool _encoded;
};

class xml_fault_obj_t {
public:
  xml_fault_obj_t (int c, const str &s) : 
    _fault (xml_fault_t::alloc (c, s)) {}
  ptr<xml_fault_t> _fault;
};

class xml_empty_array_t {
public:
  xml_empty_array_t () {}
};

class xml_obj_ref_t : public xml_obj_base_t  {
public:
  xml_obj_ref_t (ptr<xml_element_t> &e) : _el_ref (e) {}
  ptr<const xml_element_t> el () const { return _el_ref; }

  xml_obj_ref_t operator[] (size_t i);
  xml_obj_ref_t operator() (const str &i);

  const xml_obj_ref_t &set_value (ptr<xml_element_t> e);
  const xml_obj_ref_t &set_fault (const xml_fault_obj_t &w)
  { _el_ref->fault (w._fault); return *this; }

  ptr<xml_struct_t> to_xml_struct ()
  { return _el_ref ? _el_ref->to_xml_struct () : NULL; }
  
  const xml_obj_ref_t &operator=(bool b) 
  { return set_value (xml_bool_t::alloc (b)); }
  const xml_obj_ref_t &operator=(const char *s)
  { return set_value (xml_str_t::alloc (xml_str_t::escape (s))); }
    
  const xml_obj_ref_t &operator=(int i) 
  { return set_value (xml_int_t::alloc (i)); }
  const xml_obj_ref_t &operator=(str s)
  { return set_value (xml_str_t::alloc (xml_str_t::escape (s))); }
  const xml_obj_ref_t &operator=(const base64_str_t &b)
  { return set_value (xml_base64_t::alloc (b, b.encoded ())); }
  const xml_obj_ref_t &operator=(ptr<xml_element_t> e)
  { return set_value (e); }
  const xml_obj_ref_t &operator=(const xml_obj_ref_t &w)
  { return set_value (w._el_ref); }
  const xml_obj_ref_t &operator=(const xml_obj_const_t &w)
  { return set_value (w.el ()->clone ()); }
  const xml_obj_ref_t &operator=(const xml_fault_obj_t &w)
  { return set_fault (w); }
  const xml_obj_ref_t &operator=(const xml_empty_array_t &a)
  { (void)coerce_to_container (); return (*this); }

  void setsize (size_t s);
  xml_obj_ref_t value ();

protected:
  ptr<xml_container_t> coerce_to_container ();
  ptr<xml_element_t> &_el_ref;

};

class xml_obj_t : public xml_obj_ref_t {
public:
  xml_obj_t (ptr<xml_element_t> p) : xml_obj_ref_t (_el), _el (p) {}
  const xml_obj_ref_t &operator=(const xml_fault_obj_t &w)
  { return set_fault (w); }
protected:
  ptr<xml_element_t> _el;
};

class xml_resp_t : public xml_obj_t {
public:
  xml_resp_t () : xml_obj_t (New refcounted<xml_method_response_t> ()) {}
  const xml_obj_ref_t &operator=(const xml_fault_obj_t &w)
  { return set_fault (w); }
};

typedef xml_obj_const_t xml_req_t;

class xml_outreq_t : public xml_obj_t {
public:
  xml_outreq_t (const str &mn) : 
    xml_obj_t (New refcounted<xml_method_call_t> (mn)) {}
  xml_outreq_t () : xml_obj_t (New refcounted<xml_method_call_t> ()) {}
  void set_method_name (const str &m) 
  { _el->to_xml_method_call ()->set_method_name (m); }
  void output (zbuf &b) const; 
};

class xml_inresp_t : public xml_obj_const_t {
public:
  xml_inresp_t (ptr<const xml_element_t> e) : xml_obj_const_t (e) {}
  xml_inresp_t () : xml_obj_const_t () {}
  xml_inresp_t (const xml_obj_base_t &w) : xml_obj_const_t (w) {}
  bool is_fault (int *code, str *msg) const;
};

typedef callback<void, xml_resp_t>::ref xml_resp_cb_t;

#endif /* _LIBAHTTP_OKXMLOBJ_H */
