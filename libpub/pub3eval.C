
#include "pub3eval.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  size_t
  env_t::push_frame (ptr<bindtab_t> t)
  {
    size_t ret = stack_size ();
    _locals_stack.push_back (t);
    return ret;
  }

  //-----------------------------------------------------------------------

  size_t
  env_t::pop_to_frame (size_t i)
  {
    _locals_stack.setsize (i);
  }

  //-----------------------------------------------------------------------

  size_t env_t::stack_size () const { return _locals_stack.size (); }

  //-----------------------------------------------------------------------


};
