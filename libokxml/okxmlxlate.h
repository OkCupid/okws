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

#ifndef _LIBAHTTP_OKXMLXLATE_H
#define _LIBAHTTP_OKXMLXLATE_H

#include "async.h"
#include "xdrmisc.h"
#include "rpctypes.h"
#include "okxmldata.h"
#include "okxmlobj.h"

//
// Runtime classes required for running XML<->XDR translation;
// generate stubs via rpcc and xmlrpcc, and then plug in to this
// class (automatically) via includes.
//

typedef enum { XML_2_XDR = 1, XDR_2_XML = 2 } XML_RPC_op_t;

typedef enum {
  XDR_STRUCT = 1,
  XDR_ENUM = 2,
  XDR_UNION = 3,
  XDR_POINTER = 4,
  XDR_VECTOR = 5,
  XDR_SCALAR = 6
} xdr_phylum_t;

class XML_RPC_obj_t {
public:
  virtual bool enter_field (const char *f) = 0;
  virtual bool exit_field () = 0;
  virtual bool traverse (int32_t &i) = 0;
  virtual bool traverse (u_int32_t &u) = 0;
  virtual bool traverse_opaque (str &s) = 0;
  virtual bool traverse_string (str &s) = 0;
  virtual ~XML_RPC_obj_t () {}
  virtual XML_RPC_op_t mode () const = 0;

  // 
  // Push routines return # of stack frames pushed on success,
  // and -1 on failure.
  //
  virtual int push (const char *typname, xdr_phylum_t ph,
		    const char *fieldname) = 0;
  virtual int push_array_slot (int i) = 0;
  virtual int push_array (size_t s, size_t capac, bool fixed, 
			  ssize_t *rsz) = 0;

  // Pop that many frames
  virtual bool pop (int i) = 0;
};

class XML_creator_t : public XML_RPC_obj_t {
public:
  XML_creator_t () {}

  bool enter_field (const char *f);
  bool exit_field () { _stack.pop_back (); return true; }
  bool traverse (int32_t &i);
  bool traverse (u_int32_t &i);
  XML_RPC_op_t mode () const { return XDR_2_XML; }

  bool traverse_opaque (str &s);
  bool traverse_string (str &s);

  void setroot (const xml_obj_ref_t &o)
  {
    _stack.push_back (o);
  }

  int push (const char *typname, xdr_phylum_t ph, 
	    const char *fieldname);
  bool pop (int i) { _stack.popn_back (i); return true; }

  int push_array (size_t s, size_t capac, bool fixed, ssize_t *rsz);
  int push_array_slot (int i);

private:

  xml_obj_ref_t &top () { return _stack.back (); }
  bool is_empty () const { return _stack.size () == 0; }
  void push (const xml_obj_ref_t &o) { _stack.push_back (o); }

  vec<xml_obj_ref_t> _stack;
};

class XML_reader_t : public XML_RPC_obj_t {
public:
  XML_reader_t (xml_obj_const_t r) : _root (r)
  { _stack.push_back (_root); }

  XML_reader_t () {}

  void setroot (xml_obj_const_t r)
  {
    _stack.clear ();
    _root = r;
    _stack.push_back (r);
  }

  bool enter_field (const char *f);
  bool exit_field () { _stack.pop_back (); return true; }
  bool traverse (int32_t &i);
  bool traverse (u_int32_t &i);
  XML_RPC_op_t mode () const { return XML_2_XDR; }

  bool traverse_opaque (str &s);
  bool traverse_string (str &s);

  bool pop (int i) { _stack.popn_back (i); return true; }
  int push (const char *typname, xdr_phylum_t ph, 
		      const char *fieldname);

  int push_array (size_t s, size_t capac, bool fixed, ssize_t *rsz);
  int push_array_slot (int i);

private:
  
  const xml_obj_const_t &top () const { return _stack.back (); }
  bool is_empty () const { return _stack.size () == 0; }
  void push (const xml_obj_const_t &o) { _stack.push_back (o); }

  xml_obj_const_t _root;
  vec<xml_obj_const_t> _stack;
};

typedef bool (*xml_xdrproc_t) (XML_RPC_obj_t *, void *);

struct xml_rpc_const_t {
  const char *name;
  int val;
};

struct xml_rpcgen_table {
  const char *name;
  const std::type_info *type_arg;
  xml_xdrproc_t xml_arg_proc;
  const std::type_info *type_res;
  xml_xdrproc_t xml_res_proc;
};

struct xml_rpc_program {
  u_int32_t progno;
  u_int32_t versno;
  const rpc_program *xdr_prog;
  const struct xml_rpcgen_table *xml_tbl;
  size_t nproc;
  const char *name;
};

struct xml_rpc_file {
  const struct xml_rpc_program **programs;
  const struct xml_rpc_const_t *constants;
  const char *filename;
};

#define XMLTBL_DECL(proc, arg, res)                               \
{                                                                 \
  #proc,                                                          \
  &typeid (arg), xml_##arg,                                       \
  &typeid (res), xml_##res                                        \
},                                                                \

ptr<xml_element_t>
xml_enter_field (XML_RPC_obj_t *obj, const char *field_name);

void xml_exit_field (XML_RPC_obj_t *obj, ptr<xml_element_t> el);

template<class T> bool
xml_rpc_traverse (XML_RPC_obj_t *obj, T &t, const char *field_name)
{
  ptr<xml_element_t> old;
  bool ret = true;

  if (field_name && !obj->enter_field (field_name))
    return false;
  
  ret = rpc_traverse (obj, t);

  if (field_name) {
    obj->exit_field ();
  }

  return ret;
}

template<class T, size_t n> void
_array_setsize (array<T,n> &a, size_t nsz) { /* noop */ }

template<class T, size_t n> void
_array_setsize (rpc_vec<T,n> &v, size_t nsz) { v.setsize (nsz); }

template<class V> bool
_rpc_traverse_array (XML_RPC_obj_t *xml, V &v, size_t n, bool fixed)
{
  ssize_t nsz;
  int n_frames;
  int a;

  if ((n_frames = xml->push_array (v.size (), n, fixed, &nsz)) < 0)
    return false;

  if (nsz >= 0)
    _array_setsize (v, nsz);

  for (size_t i = 0; i < v.size (); i++) {
    if ((a = xml->push_array_slot (i)) < 0 ||
	!xml_rpc_traverse (xml, v[i], NULL) ||
	!xml->pop (a))
      return false;
  }

  if (!xml->pop (n_frames))
    return false;

  return true;
}

template<class T, size_t n> bool
rpc_traverse (XML_RPC_obj_t *xml, rpc_vec<T,n> &v)
{
  return _rpc_traverse_array (xml, v, n, false);
}

template<class T, size_t n> bool
rpc_traverse (XML_RPC_obj_t *xml, array<T,n> &v)
{
  return _rpc_traverse_array (xml, v, n, true);
}

bool rpc_traverse (XML_RPC_obj_t *obj, u_int32_t &i) ;
bool rpc_traverse (XML_RPC_obj_t *obj, int32_t &i) ; 

template<size_t n> bool
rpc_traverse (XML_RPC_obj_t *xml, rpc_str<n> &obj)
{
  bool ret = false;
  switch (xml->mode ()) {
  case XML_2_XDR:
    {
      str s;
      ret = xml->traverse_string (s);
      if (ret && s.len () <= n) {
	obj = s;
      } else {
	ret = false;
      }
    }
    break;
  case XDR_2_XML:
    ret = xml->traverse_string (obj);
    break;
  default:
    panic ("Unexpected XML conversion mode\n");
    break;
  }
  return ret;
}


template<class T> bool
rpc_traverse (XML_RPC_obj_t *xml, T &obj, size_t n)
{
  bool ret = false;
  switch (xml->mode ()) {
  case XML_2_XDR:
    {
      str s;
      ret = xml->traverse_opaque (s);
      if (ret && s.len () <= n) {
	obj.setsize (s.len ());
	memcpy (obj.base (), s.cstr (), s.len ());
      } else {
	ret = false;
      }
    }
    break;
	
  case XDR_2_XML:
    {
      str s (obj.base (), obj.size ());
      ret = xml->traverse_opaque (s);
    }
    break;

  default:
    panic ("Unexpected XML conversion mode\n");
    break;
  }
  return ret;
}

template<size_t n> bool
rpc_traverse (XML_RPC_obj_t *xml, rpc_opaque<n> &obj)
{
  return rpc_traverse (xml, obj, n);
}

template<size_t n> bool
rpc_traverse (XML_RPC_obj_t *xml, rpc_bytes<n> &obj)
{
  return rpc_traverse (xml, obj, n);
}

inline bool
xml_void (XML_RPC_obj_t *obj, void *v)
{
  return true;
}



#endif /* _LIBAHTTP_OKXMLXLATE_H */
