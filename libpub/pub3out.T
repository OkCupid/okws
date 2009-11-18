// -*-c++-*-

#include "pub3out.h"


namespace pub3 {

  //====================================================================

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

  //====================================================================

  ptr<output_silent_t> output_silent_t::alloc () 
  { return New refcounted<output_silent_t> (); }

  //====================================================================

};
