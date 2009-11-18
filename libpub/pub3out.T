// -*-c++-*-

#include "pub3out.h"


namespace pub3 {

  //========================================== output_t ==================

  ptr<output_t>
  output_t::alloc (zbuf *z)
  {
    ptr<output_t> ret;
    if (z) { ret = New refcounted<output_std_t> (z);   }
    else   { ret = New refcounted<output_silent_t> (); }
    return ret;
  }

  //--------------------------------------------------------------------

  bool
  output_t::push_muzzle (bool b)
  {
    bool ret = _muzzle;
    if (b && !_muzzle) { _muzzle = b; }
    return ret;
  }

  //--------------------------------------------------------------------

  void output_t::pop_muzzle (bool b) { _muzzle = b; }

  //--------------------------------------------------------------------

  static const char *
  typ2str (err_type_t typ)
  {
    const char *ret = "other";
    switch (typ) {
    case P_ERR_WARNING: ret = "warning"; break;
    case P_ERR_PARSE: ret = "parse"; break;
    case P_ERR_ERROR: ret = "error"; break;
    case P_ERR_EVAL: ret = "eval"; break;
    default: break;
    }
    return ret;
  }

  //--------------------------------------------------------------------

  void
  output_t::pub3_add_error (runloc_t loc, str msg, err_type_t typ)
  {
    pub3::obj_dict_t o;
    loc.pub (o);
    o("error") = msg;
    o("type") = typ2str (typ);
    _err_obj.push_back (o);
  }

  //=================================== output_std_t ====================

  void output_std_t::output (zstr z) { _out->cat (z); }
  void output_std_t::output (str s) { _out->cat (s); }

  //--------------------------------------------------------------------

  void
  output_std_t::output_err (runloc_t loc, str msg, err_type_t t)
  {

  }

  //--------------------------------------------------------------------

  void
  output_std_t::output_err (const loc_stack_t &stk, str msg, err_type_t t)
  {


  }

  //=================================== output_silent_t ================

  ptr<output_silent_t> output_silent_t::alloc () 
  { return New refcounted<output_silent_t> (); }

  //--------------------------------------------------------------------

  void output_silent_t::output (zstr z) {}
  void output_silent_t::output (str s) {}

  //--------------------------------------------------------------------

  void
  output_silent_t::output_err (runloc_t loc, str msg, err_type_t t)
  {

  }

  //--------------------------------------------------------------------

  void
  output_silent_t::output_err (const loc_stack_t &stk, str msg, err_type_t t)
  {


  }

  //====================================================================

};
