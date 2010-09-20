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
  JSON_creator_t () { clear (); }
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

  void clear ();

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
public:
  JSON_reader_t (ptr<const pub3::expr_t> x);
  ~JSON_reader_t ();

  void setroot (ptr<const pub3::expr_t> x);

  bool enter_field (const char *f);
  bool exit_field () ;

  bool traverse (bool &b);
  bool traverse (int64_t &i);
  bool traverse (u_int64_t &i);
  bool traverse (int32_t &i);
  bool traverse (u_int32_t &i);
  bool traverse_opaque (str &s);
  bool traverse_string (str &s);

  XML_RPC_op_t mode () const { return XML_2_XDR; }
  bool pop (int i) { _stack.popn_back (i); return true; }

  int push_array_slot (int i);
  int push_array (size_t s, size_t capac, bool fixed, 
		  ssize_t *rsz);
  int push_ptr (bool exists, bool *alloc);
private:
  int error_wrong_type (const char *f, ptr<const pub3::expr_t> x, int rc = 0);
  ptr<const pub3::expr_t> top () const { return _stack.back (); }
  bool is_empty () const { return _stack.size () == 0 || !_stack.back (); }
  void push (ptr<const pub3::expr_t> x) { _stack.push_back (x); }
  ptr<const pub3::expr_t> _root;
  vec<ptr<const pub3::expr_t> > _stack;
};

//-----------------------------------------------------------------------
