
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
  _stack.back () = int (i);
  return true;
}

bool
XML_creator_t::traverse (u_int32_t &i)
{
  _stack.back () = int (i);
  return true;
}

bool
XML_reader_t::traverse (int32_t &i)
{
  if (!_stack.size ()) return false;
  ptr<const xml_int_t> x = _stack.back ().el ()->to_xml_int ();
  if (!x)
    return false;
  i = *x;
  return true;
}

bool
XML_reader_t::traverse (u_int32_t &i)
{
  if (!_stack.size ()) return false;
  ptr<const xml_int_t> x = _stack.back ().el ()->to_xml_int ();
  if (!x)
    return false;
  i = int (*x);
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
    _stack.back () = e;
    _stack.push_back (xml_obj_ref_t (e));
    ret = 1;
  }

  return ret;
}

int
XML_creator_t::push_array (size_t s, size_t capac, bool fixed, 
			   ssize_t *sz)
{
  ptr<xml_element_t> e = xml_array_t::alloc ();
  _stack.back () = e;
  _stack.push_back (xml_obj_ref_t (e));
  *sz = -1;
  return 1;
}

int
XML_reader_t::push_array (size_t s, size_t capac, bool fixed,
			  ssize_t *szp)
{
  *szp = -1;
  if (!_stack.size ()) return -1;

  ptr<const xml_array_t> a = _stack.back ().el ()->to_xml_array ();
  ptr<const xml_data_t> d;
  size_t sz;

  if (!a || !(d = a->data ()) || (sz = d->size ()) > capac)
    return -1;
     
  if (fixed && (sz != capac || s != sz))
    return -1;

  *szp = sz;
  return 0;
}

int
XML_reader_t::push_array_slot (int i)
{
  if (!_stack.size ()) return -1;
  xml_obj_const_t o = _stack.back ();
  _stack.push_back (o[i]);
  return 1;
}

int
XML_creator_t::push_array_slot (int i)
{
  if (!_stack.size ()) return -1;
  xml_obj_ref_t o = _stack.back ();
  _stack.push_back (o[i]);
  return 1;
}

