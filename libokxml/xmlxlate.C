
// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "okxmlxlate.h"

bool
XML_creator_t::enter_field (const char *f)
{
  push (top()(f));
  debug_push (f);
  return true;
}

bool
XML_reader_t::enter_field (const char *f)
{
  if (is_empty ())         return error_empty ("struct");
  if (!top().is_struct ()) return error_wrong_type ("struct");

  push (top()(f));
  debug_push (f);
  return true;
}

bool
XML_creator_t::exit_field ()
{
  _stack.pop_back ();
  debug_pop ();
  return true;
}

bool
XML_reader_t::exit_field ()
{
  _stack.pop_back ();
  debug_pop();
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
  if (is_empty ())      return error_empty ("int");
  if (!top().is_int ()) return error_wrong_type ("int");
  i = top();
  return true;
}

bool
XML_reader_t::traverse (u_int32_t &i)
{
  if (is_empty ())      return error_empty ("u_int");
  if (!top().is_int ()) return error_wrong_type ("u_int");

  i = int (top());
  return true;
}

int
XML_creator_t::push_array (size_t s, size_t capac, bool fixed, 
			   ssize_t *sz)
{
  *sz = -1;
  return 0;
}

int
XML_reader_t::push_array (size_t s, size_t capac, bool fixed,
			  ssize_t *szp)
{
  *szp = -1;

  if (is_empty ())        return error_empty ("array/vector", -1);
  if (!top().is_array ()) return error_wrong_type ("array/vector", -1);

  size_t sz = top ().size ();
  
  if ((sz > capac) || (fixed && (sz != capac || s != sz))) {
    _err_msg = "Array/vector had bad size";
    return -1;
  }

  *szp = sz;
  return 0;
}

int
XML_reader_t::push_array_slot (int i)
{
  if (is_empty ()) return error_empty ("array slot", -1);
  push (top ()[i]);
  debug_push (i);
  return 1;
}

int
XML_creator_t::push_array_slot (int i)
{
  push (top ()[i]);
  debug_push (i);
  return 1;
}

bool
XML_reader_t::traverse_opaque (str &s)
{
  if (is_empty ())         return error_empty ("base64");
  if (!top().is_base64 ()) return error_wrong_type ("base64");
  s = top ();
  return true;
}

bool
XML_reader_t::traverse_string (str &s)
{
  if (is_empty ())         return error_empty ("string");
  if (!top().is_str ())    return error_wrong_type ("string");
  s = top ();
  return true;
}

bool
XML_creator_t::traverse_opaque (str &s)
{
  top () = base64_str_t (s);
  return true;
}

bool
XML_creator_t::traverse_string (str &s)
{
  top () = s;
  return true;
}

int
XML_reader_t::push_ptr (bool dummy, bool *alloc)
{
  int ret = -1;
  *alloc = false;
  if (is_empty ())         return error_empty ("array/ptr", -1);
  if (!top ().is_array ()) return error_wrong_type ("array/ptr", -1);

  switch (top ().size ()) {
  case 0: 
    ret = 0;
    break;
  case 1:
    *alloc = true;
    push (top ()[0]);
    ret = 1;
    break;
  default:
    ret = -1;
  }
  return ret;
}

int
XML_creator_t::push_ptr (bool exists, bool *alloc)
{
  int ret = 0;
  *alloc = false;
  if (exists) {
    push (top ()[0]);
    ret = 1;
  } else {
    top () = xml_empty_array_t ();
  }
  return ret;
}

void
XML_RPC_obj_t::debug_push (const str &s)
{
  _debug_stack.push_back (f);
}

void
XML_RPC_obj_t::debug_push (int i)
{
  _debug_stack.push_back (strbuf ("%d", i));
}

str
XML_RPC_obj_t::error_msg (const str &prfx)
{
  str x = _err_msg;
  if (!x) x = "generic error";
  strbuf b;
  for (size_t i = 0; i < _debug_stack.size (); i++) {
    if (i > 0) b << ":";
    b << _debug_stack[i];
  }

  b << ": ";
  if (prfx)
    b << prfx << ": ";
  b << x;

  return b;
}

int
XML_RPC_obj_t::error_empty (const char *f, int rc)
{
  strbuf b ("Expected type '%s'; got nothing", f);
  _err_msg = b;
  return rc;
}

int
XML_RPC_obj_t::error_wrong_type (const char *f, int rc)
{
  strbuf b ("Expected type '%s'; got something else instead", f);
  _err_msg = b;
  return rc;
}
