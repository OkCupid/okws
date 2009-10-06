
#include "pub3eval.h"

namespace pub3 {

  //-----------------------------------------------------------------------

  env_t::env_t  (ptr<bindtab_t> u) : 
    _universals (u), _globals (New refcounted<bindtab_t> ())
  {
    _stack.push_back (stack_layer_t (_universals, LAYER_UNIVERSALS));
    _stack.push_back (stack_layer_t (_globals, LAYER_GLOBALS));
  }

  //-----------------------------------------------------------------------

  size_t
  env_t::push_locals (ptr<bindtab_t> t)
  {
    size_t ret = stack_size ();
    _stack.push_back (stack_layer_t (t, LAYER_LOCALS));
    return ret;
  }

  //-----------------------------------------------------------------------

  size_t 
  env_t::push_universals_refs (ptr<bindtab_t> t)
  {
    size_t ret = stack_size ();
    _stack.push_back (stack_layer_t (t, LAYER_UNIREFS));
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

  ptr<const expr_t>
  eval_t::lookup_val (const str &nm) const
  {
    return _env->lookup_val (nm);
  }

  //-----------------------------------------------------------------------

  ptr<const expr_t>
  env_t::lookup_val (const str &nm) const
  {
    ptr<const expr_t> x;
    ptr<const expr_t> *xp;

    for (ssize_t i = _stack.size () - 1; !x && i >= 0; i--) {
      stack_layer_t l = _stack[i];
      if (l._bindings && (xp = (*l._bindings)[nm])) {
	if (l._typ == LAYER_UNIREFS) { x = (*_universals)[nm]; } 
	else { x = *xp; }
	if (!x) x = expr_null_t::alloc ();
      }
    }
    return x;
  }

  //-----------------------------------------------------------------------

  ptr<mref_t>
  env_t::lookup_ref (const str &nm) const
  {
    ptr<bindtab_t> found;
    for (ssize_t i = _stack.size () - 1; !found && i >= 0; i--) {
      stack_layer_t l = _stack[i];
      if (l._bindings && (xp = (*l._bindings)[nm])) {
	if (l._typ == LAYER_UNIREFS) { found = _universals; }
	else { found = l._bindings; }
      }
    }
    if (!found) { found = _globals; }
    return New refcounted<mref_dict_t> (found, nm);
  }

  //-----------------------------------------------------------------------

  void
  expr_t::report_error (eval_t e, str msg) const
  {
    penv_t *env = e.penv ();
    output_t *out = e.output ();
    env->setlineno (_lineno);
    env->warning (msg);
    if (out) {
      out->output_err (env, msg);
    }
    env->unsetlineno ();
  }
  
  //-----------------------------------------------------------------------
  
  bool
  eval_t::set_loud (bool b)
  {
    bool c = _loud;
    _loud = b;
    return c;
  }
  
  //-----------------------------------------------------------------------
  
  bool
  eval_t::set_silent (bool b)
  {
    bool c = _silent;
    _silent = b;
    return c;
  }

  //-----------------------------------------------------------------------

};
