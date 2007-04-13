
// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "okxmlxlate.h"

bool
XML_creator_t::enter_field (const char *f)
{
  ptr<xml_struct_t> s = _stack.back ().to_xml_struct ();
  if (!s)
    return false;
  _stack.push_back (xml_obj_ref_t (s->get_r (f)));
  return true;
}

bool
XML_reader_t::enter_field (const char *f)
{
  ptr<const xml_struct_t> s = _stack.back ().to_xml_struct ();
  if (!s)
    return false;
  _stack.push_back (xml_obj_const_t (s->get (f)));
  return true;
}

bool 
rpc_traverse (XML_RPC_obj_t *obj, u_int32_t i) 
{ return false; }

bool 
rpc_traverse (XML_RPC_obj_t *obj, int32_t i) 
{ return false; }
