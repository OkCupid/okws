// -*-c++-*-

#pragma once

#include "pub3.h"
#include "okxmlxlate.h"

//-----------------------------------------------------------------------

class plain_obj_ref_t : public pub3::obj_ref_t {
public:
  plain_obj_ref_t () {}
  ptr<pub3::expr_t> get () { return m_x; }
  ptr<const pub3::expr_t> get () const { return m_x; }
  void set (ptr<pub3::expr_t> x) { m_x = x; }
  static ptr<plain_obj_ref_t> alloc () 
  { return New refcounted<plain_obj_ref_t> (); }
private:
  ptr<pub3::expr_t> m_x;
};

//-----------------------------------------------------------------------

// From an input XDR object, create a JSON representation of it.
class JSON_creator_t : public XML_RPC_obj_t {
public:
  JSON_creator_t () { push_ref (plain_obj_ref_t::alloc ()); }
  ~JSON_creator_t () {}

  bool enter_field (const char *f);
  bool exit_field ();

  bool traverse (bool &b);
  bool traverse (int32_t &i);
  bool traverse (u_int32_t &u);
  bool traverse (int64_t &i);
  bool traverse (u_int64_t &i);
  bool traverse_opaque (str &s);
  bool traverse_string (str &s);
  XML_RPC_op_t mode () const { return XDR_2_XML; }

  // 
  // Push routines return # of stack frames pushed on success,
  // and -1 on failure.
  //
  int push_array_slot (int i);
  int push_array (size_t s, size_t capac, bool fixed, ssize_t *rsz);
  int push_ptr (bool exists, bool *alloc);

  // Pop that many frames
  bool pop (int i);

  ptr<pub3::expr_t> result () { return _obj_stack[0]; }
  ptr<const pub3::expr_t> result () const { return _obj_stack[0]; }

private:
  void push_ref (ptr<pub3::obj_ref_t> r);
  void pop_ref ();
  ptr<pub3::expr_dict_t> top_dict ();
  ptr<pub3::expr_list_t> top_list ();
  void set_top (ptr<pub3::expr_t> x);

  vec<ptr<pub3::obj_ref_t> > _ref_stack;
  vec<ptr<pub3::expr_t> > _obj_stack;
};

//-----------------------------------------------------------------------

// From an input JSON object, create an XDR equivalent.
class JSON_reader_t : public XML_RPC_obj_t {

};

