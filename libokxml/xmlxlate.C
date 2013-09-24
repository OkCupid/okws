
// -*-c++-*-
/* $Id: okcgi.h 1682 2006-04-26 19:17:22Z max $ */

#include "okxmlxlate.h"
#include "parseopt.h"

#define UI8_STR "ui8"
#define UI4_STR "ui4"
#define I8_STR  "i8"

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
  if (!top().is_struct ()) return error_wrong_type ("struct", top());

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
rpc_traverse (XML_RPC_obj_t *obj, bool &b)
{
  return obj->traverse (b);
}

bool 
rpc_traverse (XML_RPC_obj_t *obj, int32_t &i) 
{ 
  return obj->traverse (i);
}

bool 
rpc_traverse (XML_RPC_obj_t *obj, u_int32_t &i) 
{ 
  return obj->traverse (i);
}

bool
rpc_traverse (XML_RPC_obj_t *obj, int64_t &i)
{
  return obj->traverse (i);
}

bool
rpc_traverse (XML_RPC_obj_t *obj, u_int64_t &i)
{
  return obj->traverse (i);
}

template<class T> str
xml_int_repr (const char *x, T &t)
{
  strbuf b;
  b << x << ":" << t;
  return b;
}

template<class T> bool
xml_decode_int_repr (const str &s, const char *prfx, T &i, bool sig)
{
  size_t plen = strlen (prfx);
  
  // need at least 2 more characters in addition to the prefix,
  // which are the colon, and a digit
  if (s.len () < plen + 2) return false;

  const char *input = s.cstr ();
  if (strncmp (prfx, input, plen)) return false;
  const char *bp = input + plen;
  if (*bp != ':') return false;
  bp ++;
  char *ep;

  if (sig) {
    i = strtoll (bp, &ep, 0);
  } else {
    i = strtoull (bp, &ep, 0);
  }

  return (ep && !*ep);
}

template<class T> bool
XML_reader_t::t_traverse (const char *prfx, T &i, bool sig)
{
  if (is_empty ())      return error_empty ("string");
  if (!top().is_str ()) return error_wrong_type ("string", top());

  if (!xml_decode_int_repr (top (), prfx, i, sig)) {
    strbuf b ("%s decoding error", prfx);
    return error_generic (str (b).cstr());
  }
  return true;
}

bool
XML_creator_t::traverse (int32_t &i)
{
  top() = int (i);
  return true;
}

bool
XML_creator_t::traverse (bool &b)
{
  top() = b;
  return true;
}

bool
XML_creator_t::traverse (u_int32_t &i)
{
  top() = xml_int_repr (UI4_STR, i);
  return true;
}

bool
XML_creator_t::traverse (int64_t &i)
{
  top() = xml_int_repr (I8_STR, i);
  return true;
}

bool
XML_creator_t::traverse (u_int64_t &i)
{
  top() = xml_int_repr (UI8_STR, i);
  return true;
}

bool
XML_reader_t::traverse (int32_t &i)
{
  if (is_empty ())      return error_empty ("int");
  if (!top().is_int ()) return error_wrong_type ("int", top());
  i = top();
  return true;
}

bool
XML_reader_t::traverse (bool &b)
{
  if (is_empty ())      
    return error_empty ("bool");
  if (!top().is_bool() && !top().is_int ()) 
    return error_wrong_type ("bool", top());
  b = top();
  return true;
}

bool
XML_reader_t::traverse (u_int32_t &i)
{
  return bool(t_traverse (UI4_STR, i, false));
}

bool
XML_reader_t::traverse (int64_t &i)
{
  return bool(t_traverse (I8_STR, i, true));
}

bool
XML_reader_t::traverse (u_int64_t &i)
{
  return bool(t_traverse (UI8_STR, i, false));
}

int
XML_creator_t::push_array (size_t s, size_t capac, bool fixed, 
			   ssize_t *sz)
{
  if (s == 0) {
    // In this case, need to push something useful so that the 
    // higher-level object knows that something is here.
    top () = xml_empty_array_t ();
  }
  *sz = -1;
  return 0;
}

int
XML_reader_t::push_array (size_t s, size_t capac, bool fixed,
			  ssize_t *szp)
{
  *szp = -1;

  if (is_empty ())        return error_empty ("array/vector", -1);
  if (!top().is_array ()) return error_wrong_type ("array/vector", top(), -1);

  size_t sz = top ().size ();
  
  if ((sz > capac) || (fixed && (sz != capac || s != sz))) {
    return error_generic ("Array/vector had bad size", -1);
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
  if (!top().is_base64 ()) return error_wrong_type ("base64", top());
  s = top ();
  return true;
}

bool
XML_reader_t::traverse_string (str &s)
{
  if (is_empty ())         return error_empty ("string");
  if (!top().is_str ())    return error_wrong_type ("string", top());
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
  if (!top ().is_array ()) return error_wrong_type ("array/ptr", top(), -1);

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
  _debug_stack.push_back (s);
}

void
XML_RPC_obj_t::debug_push (int i)
{
  _debug_stack.push_back (strbuf ("%d", i));
}

str
XML_RPC_obj_t::error_msg (const str &arg)
{
  if (!_err_msg) {
    return "generic error";
  }
  const char *x = "";
  if (arg)
    x = arg;
  return strbuf (_err_msg.cstr (), x);
}

void
XML_RPC_obj_t::freeze_err_msg (str s)
{
  if (!s) s = "generic error";
  strbuf b;
  for (size_t i = 0; i < _debug_stack.size (); i++) {
    if (i > 0) b << ":";
    b << _debug_stack[i];
  }

  b << ": %s: ";
  b << s;

  _err_msg = b;
}

int
XML_RPC_obj_t::error_generic (const char *f, int rc)
{
  freeze_err_msg (f);
  return rc;
}

int
XML_RPC_obj_t::error_empty (const char *f, int rc)
{
  strbuf b ("Expected type '%s'; got nothing", f);
  freeze_err_msg (b);
  return rc;
}

int
XML_reader_t::error_wrong_type (const char *f, const xml_obj_base_t &o, int rc)
{
  const char *e = o.xml_typename (true).cstr();
  return XML_RPC_obj_t::error_wrong_type (f, e, rc);
}

int
XML_RPC_obj_t::error_wrong_type (const char *f, const char *e, int rc)
{
  strbuf b ("Expected type '%s'; got '%s' instead", f, e);
  freeze_err_msg (b);
  return rc;
}

#define PRIM(x) xml_typeinfo_t xml_typeinfo_ ## x =	\
    { #x, xml_typeinfo_t::PRIMITIVE, NULL }

#define DEF(x, y) \
  static xml_struct_entry_t _xml_typedef_ ## x =			\
    { NULL, #x, &xml_typeinfo_ ## y, 0, 0 };				\
  xml_typeinfo_t xml_typeinfo_ ## x =					\
    { #x, xml_typeinfo_t::TYPEDEF, &_xml_typedef_ ## x }

PRIM(void);
PRIM(bool);
PRIM(int);
PRIM(u_int32_t);
PRIM(hyper);
PRIM(u_int64_t);
PRIM(string);
PRIM(opaque);
PRIM(false); // just to make macros expand w/o compile errors

DEF(int32_t, int);
DEF(int64_t, hyper);
DEF(uint32_t, u_int32_t);
DEF(uint64_t, u_int64_t);

const xml_typeinfo_t *xml_typeinfo_base_types[] = {
    &xml_typeinfo_void,
    &xml_typeinfo_bool,
    &xml_typeinfo_int,
    &xml_typeinfo_u_int32_t,
    &xml_typeinfo_hyper,
    &xml_typeinfo_u_int64_t,
    &xml_typeinfo_string,
    &xml_typeinfo_opaque,
    &xml_typeinfo_int32_t,
    &xml_typeinfo_int64_t,
    &xml_typeinfo_uint32_t,
    &xml_typeinfo_uint64_t,
};

