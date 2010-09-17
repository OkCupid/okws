
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

