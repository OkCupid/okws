
#include "pub3func.h"

//-----------------------------------------------------------------------

bool
pub3::for_t::add (ptr<arglist_t> l)
{
  bool ret = true;
  if (_iter || 
      !l || 
      l->size () != 2 ||
      !(_iter = ((*l)[0])->eval ()) ||
      !(_arr = ((*l)[1])->eval ())) {
    PWARN ("pub takes 2 arguments (formal variable and array)\n");
    ret = false;
  }
  return ret;
}

//-----------------------------------------------------------------------

void
pub3::for_t::output (output_t *o, penv_t *e) const
{
}

//-----------------------------------------------------------------------
