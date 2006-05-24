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

#ifndef _LIBAHTTP_OKXML_H
#define _LIBAHTTP_OKXML_H

#include <async.h>
#include <ihash.h>
#include <ctype.h>
#include "aparse.h"

class xml_struct_t;
class xml_array_t;
class xml_int_t;
class xml_bool_t;
class xml_double_t;
class xml_str_t;
class xml_base64_t;
class xml_null_t;

class xml_element_t : public virtual refcnt {
public:
  xml_element_t () {}
  virtual ~xml_element_t () {} 

  virtual ptr<xml_struct_t> to_struct () { return NULL; }
  virtual ptr<xml_array_t> to_array () { return NULL; }
  virtual ptr<xml_int_t> to_int () { return NULL; }
  virtual ptr<xml_bool_t> to_bool () { return NULL; }
  virtual ptr<xml_double_t> to_double () { return NULL; }
  virtual ptr<xml_str_t> to_str () { return NULL; }
  virtual ptr<xml_base64_t> to_base64 () { return NULL; }
  virtual ptr<xml_null_t> to_null () { return NULL; }

  virtual ptr<xml_element_t> get (const str &s) const;
  virtual ptr<xml_element_t> get (int i) const;
  ptr<xml_element_t> operator[] (const str &s) const { return get (s); }
  ptr<xml_element_t> operator[] (int i) const { return get (s); }
  virtual bool put (const str &s, ptr<xml_element_t> el) { return false; }
};

class xml_null_t : public xml_element_t {
public:
  xml_null_t () {}
  static ptr<xml_null_t> alloc () 
  { return New refcounted <xml_null_t> (); }
};

class xml_struct_t : public xml_element_t {
public:
  bool put (const str &s, ptr<xml_element_t> el);
private:
  qhash<str, ptr<xml_element_t> > _members;
};

class xml_array_t : public xml_element_t {
public:
  ptr<xml_element_t> get (int i) const;
private:
  vec<ptr<xml_element_t> > _elements;
};

class xml_req_parser_t : public async_parser_t {
public:
  xml_req_parser_t (abuf_src *s) : async_parser_t (s) {}
  xml_req_parser_t (abuf_t *a) : async_parser_t (a) {}
private:
  virtual void parse_guts ();

};


#endif /* _LIBAHTTP_OKXML_H */
