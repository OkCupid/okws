
// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "okxmlxlate.h"

bool
XML_creator_t::enter_field (const char *f)
{
  if (is_empty () || !top().is_struct ()) return false;
  push (top()(f));
  return true;
}

bool
XML_reader_t::enter_field (const char *f)
{
  if (is_empty () || !top().is_struct ()) return false;
  push (top()(f));
  return true;
}

bool 
rpc_traverse (XML_RPC_obj_t *obj, u_int32_t &i) 
{ 
  return obj->traverse (i);
}

bool 
rpc_traverse (XML_RPC_obj_t *obj, int32_t &i) 
{ 
  return obj->traverse (i);
}

bool
XML_creator_t::traverse (int32_t &i)
{
  top() = int (i);
  return true;
}

bool
XML_creator_t::traverse (u_int32_t &i)
{
  top() = int (i);
  return true;
}

bool
XML_reader_t::traverse (int32_t &i)
{
  if (is_empty () || !top().is_int ()) return false;
  i = top();
  return true;
}

bool
XML_reader_t::traverse (u_int32_t &i)
{
  if (is_empty () || !top().is_int ()) return false;
  i = int (top());
  return true;
}

int
XML_reader_t::push (const char *typenam, xdr_phylum_t phy, 
		    const char *fieldname)
{
  return 0;
}

int
XML_creator_t::push (const char *typenam, xdr_phylum_t phy,
		     const char *fieldname)
{
  int ret = -1;
  ptr<xml_element_t> e;
  switch (phy) {
  case XDR_STRUCT:
  case XDR_UNION:
    e = xml_struct_t::alloc ();
    break;
  default:
    break;
  }

  if (e) {
    top() = e;
    push (xml_obj_ref_t (e));
    ret = 1;
  }

  return ret;
}

int
XML_creator_t::push_array (size_t s, size_t capac, bool fixed, 
			   ssize_t *sz)
{
  ptr<xml_element_t> e = xml_array_t::alloc ();
  top() = e;
  push (xml_obj_ref_t (e));
  *sz = -1;
  return 1;
}

int
XML_reader_t::push_array (size_t s, size_t capac, bool fixed,
			  ssize_t *szp)
{
  *szp = -1;

  if (is_empty () || !top().is_array ()) return -1;

  size_t sz = top ().size ();
  
  if ((sz > capac) || (fixed && (sz != capac || s != sz)))
    return -1;

  *szp = sz;
  return 0;
}

int
XML_reader_t::push_array_slot (int i)
{
  if (is_empty ()) return -1;
  xml_obj_const_t o = top ();
  push (o[i]);
  return 1;
}

int
XML_creator_t::push_array_slot (int i)
{
  if (is_empty ()) return -1;
  xml_obj_ref_t o = _stack.back ();
  push (o[i]);
  return 1;
}

bool
XML_reader_t::traverse_opaque (str &s)
{
  if (is_empty () || !top().is_base64 ()) return false;
  s = top ();
  return true;
}

bool
XML_reader_t::traverse_string (str &s)
{
  if (is_empty () || !top().is_str ()) return false;
  s = top ();
  return true;
}

bool
XML_creator_t::traverse_opaque (str &s)
{
  if (is_empty () || !top().is_base64 ()) return false;
  top () = base64_str_t (s);
  return true;
}

bool
XML_creator_t::traverse_string (str &s)
{
  if (is_empty () || !top().is_str ()) return false;
  top () = s;
  return true;
}
