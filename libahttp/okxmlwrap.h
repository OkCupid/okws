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

#ifndef _LIBAHTTP_OKXML_WRAP_H_
#define _LIBAHTTP_OKXML_WRAP_H

class xml_const_wrap_t;

class wrap_index_t {
public:
  wrap_index_t (const str &s) : _s (s), _i (0) {}
  //wrap_index_t (const char *c) : _s (c), _i (0) {}
  wrap_index_t (size_t i) : _i (i) {}
  str to_str () const { return _s; }
  size_t to_int () const { return _i; }
private:
  const str _s;
  const size_t _i;
};

#define STR(x) str (#x)

class xml_wrap_base_t {
public:
  xml_wrap_base_t () {}
  virtual ~xml_wrap_base_t () {}

  operator int () const { return el ()->to_int (); }
  operator str () const { return el ()->to_str (); }
  operator bool () const { return el ()->to_bool (); }
  size_t size () const { return el ()->len (); }

  xml_const_wrap_t operator[] (size_t i) const ;
  xml_const_wrap_t operator() (const str &s) const ;

  virtual ptr<xml_element_t> el () const = 0;
};

class xml_const_wrap_t : public xml_wrap_base_t {
public:
  xml_const_wrap_t (ptr<xml_element_t> e) : _el (e) {}
  ptr<xml_element_t> el () const { return _el; }


private:
  ptr<xml_element_t> _el;
};

xml_const_wrap_t
xml_wrap_base_t::operator[] (size_t i) const
{
  return xml_const_wrap_t (el ()->get (i)); 
}

xml_const_wrap_t
xml_wrap_base_t::operator() (const str &s) const
{
  return xml_const_wrap_t (el ()->get (s)); 
}

class base64_str_t
{
public:
  base64_str_t (const str &s) : _s (s) {}
  operator str() const { return _s; }
private:
  const str _s;
};

class xml_wrap_t : public xml_wrap_base_t  {
public:
  xml_wrap_t (ptr<xml_element_t> &e) : _el (e) {}
  ptr<xml_element_t> el () const { return _el; }

  xml_wrap_t operator[] (size_t i) 
  { 
    if (!_el || !_el->is_int_indexable ()) 
      _el = New refcounted<xml_array_t> ();
    return xml_wrap_t (_el->get_r (i)); 
  }

  xml_wrap_t operator() (const str &s) 
  { 
    if (!_el || !_el->is_str_indexable ())
      _el = New refcounted<xml_struct_t> ();
    return xml_wrap_t (_el->get_r (s)); 
  }
  
  const xml_wrap_t &operator=(bool b)
  { _el = xml_value_t::alloc (xml_bool_t::alloc (b)); return (*this); }
  const xml_wrap_t &operator=(int i) 
  { _el = xml_value_t::alloc (xml_int_t::alloc (i)); return (*this); }
  const xml_wrap_t &operator=(str s)
  { _el = xml_value_t::alloc (xml_str_t::alloc (s)); return (*this); }
  const xml_wrap_t &operator=(const base64_str_t &b)
  { _el = xml_value_t::alloc (xml_base64_t::alloc (b)); return (*this); }
  const xml_wrap_t &operator=(ptr<xml_element_t> e)
  { _el = e; return (*this); }
  const xml_wrap_t &operator=(const xml_wrap_t &w)
  { _el = w._el; return (*this); }

private:
  ptr<xml_element_t> &_el;

};

#endif /* _LIBAHTTP_OKXML_WRAP_H */
