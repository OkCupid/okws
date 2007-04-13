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

//
// Runtime classes required for running XML<->XDR translation;
// generate stubs via rpcc and xmlrpcc, and then plug in to this
// class (automatically) via includes.
//

class XML_RPC_obj_t {
public:

};

typedef bool (*xml_xdrproc_t) (XML_RPC_obj_t *, void *);

typedef enum {
  XDR_STRUCT = 1,
  XDR_ENUM = 2,
  XDR_UNION = 3,
  XDR_POINTER = 4,
  XDR_VECTOR = 5,
  XDR_SCALAR = 6
} xdr_phylum_t;

struct xml_rpc_const_t {
  const char *name;
  int val;
};

struct xml_rpcgen_table {
  const char *name;
  const std::type_info *type_arg;
  xml_xdrproc_t xdr_arg;
  const std::type_info *type_res;
  xml_xdrproc_t xdr_res;
};

struct xml_rpc_program {
  u_int32_t progno;
  u_int32_t versno;
  const struct xml_rpcgen_table *tbl;
  size_t nproc;
  const char *name;
};


#define XMLTBL_DECL(proc, arg, res)                               \
{                                                                 \
  #proc,                                                          \
  &typeid (arg), xml_##arg,                                       \
  &typeid (res), xml_##res                                        \
},                                                                \

template<class T> bool
xml_rpc_traverse (XML_RPC_obj_t *obj, T &t, const char *field_name)
{
  return true;
}

template<class T> bool
xml_rpc_traverse_push (XML_RPC_obj_t *obj, T &t, const char *class_name,
		       xdr_phylum_t phy, const char *field_name)
{
  return true;
}

template<class T> bool
xml_rpc_traverse_pop (XML_RPC_obj_t *obj, T &t)
{
  return true;
}

inline bool
xml_void (XML_RPC_obj_t *obj, void *v)
{
  return true;
}



#endif /* _LIBAHTTP_OKXMLXLATE_H */
