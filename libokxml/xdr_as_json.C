
#include "xdr_as_json.h"

//-----------------------------------------------------------------------

void
JSON_creator_t::push_ref (ptr<pub3::obj_ref_t> r)
{
  _obj_stack.push_back (NULL);
  _ref_stack.push_back (r);
}

//-----------------------------------------------------------------------

void JSON_creator_t::pop_ref () { pop (1); }

//-----------------------------------------------------------------------

bool
JSON_creator_t::pop (int i)
{
  _obj_stack.popn_back (i);
  _ref_stack.popn_back (i);
  return true;
}

//-----------------------------------------------------------------------

void
JSON_creator_t::set_top (ptr<pub3::expr_t> x)
{
  _obj_stack.back () = x;
  _ref_stack.back ()->set (x);
}

//-----------------------------------------------------------------------

ptr<pub3::expr_dict_t>
JSON_creator_t::top_dict ()
{
  ptr<pub3::expr_dict_t> ret;
  ptr<pub3::expr_t> &back = _obj_stack.back ();
  if (!back) {
    ret = pub3::expr_dict_t::alloc ();
    back = ret;
    set_top (ret);
  } else {
    ret = back->to_dict ();
    assert (ret);
  }
  return ret;
}

//-----------------------------------------------------------------------

ptr<pub3::expr_list_t>
JSON_creator_t::top_list ()
{
  ptr<pub3::expr_list_t> ret;
  ptr<pub3::expr_t> &back = _obj_stack.back ();
  if (!back) {
    ret = pub3::expr_list_t::alloc ();
    back = ret;
    set_top (ret);
  } else {
    ret = back->to_list ();
    assert (ret);
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
JSON_creator_t::enter_field (const char *f)
{
  push_ref (pub3::obj_ref_dict_t::alloc (top_dict (), f));
  debug_push (f);
  return true;
}

//-----------------------------------------------------------------------

bool
JSON_creator_t::exit_field ()
{
  pop_ref ();
  debug_pop ();
  return true;
}

//-----------------------------------------------------------------------

bool
JSON_creator_t::traverse (int32_t &i)
{
  set_top (pub3::expr_int_t::alloc (i));
  return true;
}

//-----------------------------------------------------------------------

bool
JSON_creator_t::traverse (bool &b)
{
  set_top (pub3::expr_bool_t::alloc (b)); 
  return true;
}

//-----------------------------------------------------------------------

bool 
JSON_creator_t::traverse (u_int32_t &i) 
{ 
  set_top (pub3::expr_int_t::alloc (i));
  return true;
}

//-----------------------------------------------------------------------

bool
JSON_creator_t::traverse (int64_t &i)
{
  set_top (pub3::expr_int_t::alloc (i));
  return true;
}

//-----------------------------------------------------------------------

bool
JSON_creator_t::traverse (u_int64_t &i)
{
  set_top (pub3::expr_uint_t::alloc (i));
  return true;
}

//-----------------------------------------------------------------------

int
JSON_creator_t::push_array (size_t s, size_t capac, bool fixed, 
			    ssize_t *sz)
{
  if (s == 0) {
    // In this case, need to push something useful so that the 
    // higher-level object knows that something is here.
    set_top (pub3::expr_list_t::alloc ());
  }
  *sz = -1;
  return 0;
}

//-----------------------------------------------------------------------

int
JSON_creator_t::push_array_slot (int i)
{
  push_ref (pub3::obj_ref_list_t::alloc (top_list (), i));
  debug_push (i);
  return 1;
}

//-----------------------------------------------------------------------

bool
JSON_creator_t::traverse_opaque (str &s)
{
  set_top (pub3::expr_str_t::alloc (s));
  return true;
}

//-----------------------------------------------------------------------

bool
JSON_creator_t::traverse_string (str &s)
{
  set_top (pub3::expr_str_t::alloc (s));
  return true;
}

//-----------------------------------------------------------------------

int
JSON_creator_t::push_ptr (bool exists, bool *alloc)
{
  int ret = 0;
  *alloc = false;
  ptr<pub3::expr_list_t> l = pub3::expr_list_t::alloc ();
  set_top (l);

  if (exists) {
    push_array_slot (0);
    ret = 1;
  }
  return ret;
}

//-----------------------------------------------------------------------

void
JSON_creator_t::clear ()
{
  _ref_stack.clear ();
  _obj_stack.clear ();
  push_ref (pub3::plain_obj_ref_t::alloc ());
}

//=======================================================================

JSON_reader_t::JSON_reader_t (ptr<const pub3::expr_t> x) { setroot (x); }

//-----------------------------------------------------------------------

JSON_reader_t::~JSON_reader_t () {}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

void
JSON_reader_t::setroot (ptr<const pub3::expr_t> x)
{
  _stack.clear ();
  _root = x;
  _stack.push_back (x); 
}

//-----------------------------------------------------------------------

bool
JSON_reader_t::enter_field (const char *f)
{
  bool ret = true;
  ptr<const pub3::expr_dict_t> d;
  if (is_empty ()) { 
    ret = error_empty ("struct"); 
  } else if (!(d = top()->to_dict ())) { 
    ret = error_wrong_type ("struct", top());
  } else {
    ptr<const pub3::expr_t> slot = d->lookup (f);
    push (slot);
  }
  debug_push (f);
  return ret;
}

//-----------------------------------------------------------------------

int
JSON_reader_t::error_wrong_type (const char *f, ptr<const pub3::expr_t> x,
				 int rc)
{
  str n;
  if (!x) { n = "None"; }
  else { n = x->type_to_str (); }
  return XML_RPC_obj_t::error_wrong_type (f, n.cstr (), rc);
}


//-----------------------------------------------------------------------

bool
JSON_reader_t::exit_field ()
{
  _stack.pop_back ();
  debug_pop ();
  return true;
}

//-----------------------------------------------------------------------

bool 
JSON_reader_t::traverse (int64_t &i)
{
  bool ret = true;
  if (is_empty ()) { ret = error_empty ("int"); }
  else if (!top ()->to_int (&i)) { ret = error_wrong_type ("int", top ()); }
  else { ret = true; }
  return ret;
}

//-----------------------------------------------------------------------

bool
JSON_reader_t::traverse (bool &b)
{
  bool ret = true;
  if (is_empty ()) { ret = error_empty ("bool"); }
  else { b = top ()->to_bool (); }
  return ret;
}

//-----------------------------------------------------------------------

bool 
JSON_reader_t::traverse (int32_t &i)
{
  int64_t tmp;
  bool ret = traverse (tmp);
  if (ret)  { i = tmp; }
  return ret;
}


//-----------------------------------------------------------------------

bool 
JSON_reader_t::traverse (u_int32_t &i)
{
  int64_t tmp;
  bool ret = traverse (tmp);
  if (ret)  { i = tmp; }
  return ret;
}

//-----------------------------------------------------------------------

bool 
JSON_reader_t::traverse (u_int64_t &i)
{
  bool ret = true;
  if (is_empty ()) { ret = error_empty ("uint"); }
  else if (!top ()->to_uint (&i)) { ret = error_wrong_type ("uint", top ()); }
  else { ret = true; }
  return ret;
}

//-----------------------------------------------------------------------

int
JSON_reader_t::push_array (size_t s, size_t capac, bool fixed, ssize_t *szp)
{
  *szp = -1;
  ptr<const pub3::expr_list_t> l;
  int rc = 0;
  size_t sz = 0;

  if (is_empty ()) {
    rc = error_empty ("list", -1);
  } else if (!(l = top()->to_list ())) {
    rc = error_wrong_type ("list", top(), -1);
  } else if (((sz = l->size()) > capac) 
	     || (fixed && (sz != capac || s != sz))) {
    rc = error_generic ("list had bad size", -1);
  }
  *szp = sz;
  return rc;
}

//-----------------------------------------------------------------------

int 
JSON_reader_t::push_array_slot (int i)
{
  int rc = 0;
  ptr<const pub3::expr_list_t> l;
  if (is_empty ()) {
    rc = error_empty ("array slot", -1);
  } else if (!(l = top()->to_list ())) {
    rc = error_wrong_type ("list", top (), -1);
  } else {
    push ((*l)[i]);
    debug_push (i);
    rc = 1;
  }
  return rc;
}

//-----------------------------------------------------------------------

int 
JSON_reader_t::push_ptr (bool exists, bool *alloc)
{
  int ret = -1;
  *alloc = false;
  ptr<const pub3::expr_list_t> l;

  if (is_empty ()) { 
    ret = error_empty("array/ptr", -1); 
  } else if (!(l = top()->to_list ())) { 
    ret = error_wrong_type ("array/ptr", top(), -1);
  } else {
    switch (l->size ()) {
    case 0:
      ret = 0;
      break;
    case 1:
      *alloc = true;
      push ((*l)[0]);
      ret = 1;
      break;
    default:
      ret = -1;
      break;
    }
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
JSON_reader_t::traverse_opaque (str &s)
{
  bool ret = true;
  if (is_empty ()) { 
    error_empty ("string"); 
    ret = false;
  } else { 
    s = top()->to_str (false);
  }
  return ret;
}

//-----------------------------------------------------------------------

bool
JSON_reader_t::traverse_string (str &s)
{
  bool ret = true;
  if (is_empty () || !(s = top()->to_str (false))) { 
    ret = error_empty ("string"); 
  } else if (s.len () != strlen (s.cstr ())) {
    error_generic ("string had a NULL byte");
    ret = false;
  }
  else { ret = true; }
  return ret;
}

//-----------------------------------------------------------------------
